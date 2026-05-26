#include "platform/vulkan/ZCRenderDeviceVulkan.h"

#include "base/ZCStd.h"

#include "platform/vulkan/ZCVulkanMinimalSpv.inl"

namespace zocos {

namespace {

constexpr mstd::array<const char*, 1> kRequiredDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

constexpr VkDeviceSize kInitialVertexBufferSize = 64 * 1024;

float clamp01(float value) { return mstd::max(0.0f, mstd::min(1.0f, value)); }

} // namespace

RenderDeviceVulkan::RenderDeviceVulkan(GLFWwindow* window) : _window(window) {
    _ready = initVulkan();
    if (!_ready) {
        cleanup();
    }
}

RenderDeviceVulkan::~RenderDeviceVulkan() { cleanup(); }

void RenderDeviceVulkan::beginFrame(const Mat4& projection, int framebufferWidth,
                                    int framebufferHeight) {
    _projection = projection;
    _pendingVertices.clear();
    _pendingDraws.clear();
    (void)framebufferWidth;
    (void)framebufferHeight;

    if (!_ready || _frameActive || _device == VK_NULL_HANDLE || _swapchain == VK_NULL_HANDLE) {
        return;
    }

    VkFence frameFence = _inFlightFences[_currentFrame];
    if (vkWaitForFences(_device, 1, &frameFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkWaitForFences failed.\n");
        return;
    }

    VkResult acquireResult = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX,
                                                   _imageAvailableSemaphores[_currentFrame],
                                                   VK_NULL_HANDLE, &_currentImageIndex);
    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        mstd::fprintf(stderr, "[Vulkan] vkAcquireNextImageKHR failed (%d).\n", acquireResult);
        return;
    }

    if (_currentImageIndex >= _framebuffers.size()) {
        mstd::fprintf(stderr, "[Vulkan] Swapchain image index out of range.\n");
        recreateSwapchain();
        return;
    }

    VkCommandBuffer commandBuffer = _commandBuffers[_currentFrame];
    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkBeginCommandBuffer failed.\n");
        return;
    }

    VkClearValue clearValue{};
    clearValue.color.float32[0] = 0.12f;
    clearValue.color.float32[1] = 0.12f;
    clearValue.color.float32[2] = 0.15f;
    clearValue.color.float32[3] = 1.0f;

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = _renderPass;
    renderPassBeginInfo.framebuffer = _framebuffers[_currentImageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = _swapchainExtent;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    _frameActive = true;
}

void RenderDeviceVulkan::submit(const RenderCommand& command) {
    if (!_frameActive) {
        return;
    }

    switch (command.type) {
    case RenderCommandType::DrawSprite:
        drawSprite(command.sprite);
        break;
    case RenderCommandType::DrawQuads:
        drawQuads(command.quads);
        break;
    }
}

void RenderDeviceVulkan::endFrame() {
    if (!_ready || !_frameActive) {
        return;
    }

    VkCommandBuffer commandBuffer = _commandBuffers[_currentFrame];
    flushDrawCommands();
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkEndCommandBuffer failed.\n");
        _frameActive = false;
        return;
    }

    VkFence frameFence = _inFlightFences[_currentFrame];
    if (vkResetFences(_device, 1, &frameFence) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkResetFences failed.\n");
        _frameActive = false;
        return;
    }

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &_imageAvailableSemaphores[_currentFrame];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &_renderFinishedSemaphores[_currentFrame];

    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, frameFence) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkQueueSubmit failed.\n");
        _frameActive = false;
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &_renderFinishedSemaphores[_currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &_currentImageIndex;
    presentInfo.pResults = nullptr;

    const VkResult presentResult = vkQueuePresentKHR(_presentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else if (presentResult != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkQueuePresentKHR failed (%d).\n", presentResult);
    }

    _currentFrame = (_currentFrame + 1) % kMaxFramesInFlight;
    _frameActive = false;
    _pendingVertices.clear();
    _pendingDraws.clear();
}

TextureHandle RenderDeviceVulkan::createTexture(const TextureCreateInfo& createInfo) {
    if (!_ready || _device == VK_NULL_HANDLE || _textureDescriptorSetLayout == VK_NULL_HANDLE ||
        _textureDescriptorPool == VK_NULL_HANDLE || _textureSampler == VK_NULL_HANDLE) {
        return {};
    }

    if (createInfo.width <= 0 || createInfo.height <= 0 || !createInfo.initialData.pixels) {
        return {};
    }

    int kBytesPerPixel = 4;
    VkFormat vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
    bool isAlphaOnly = false;
    switch (createInfo.format) {
    case TextureFormat::RGBA8Unorm:
        kBytesPerPixel = 4;
        vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        break;
    case TextureFormat::A8Unorm:
        kBytesPerPixel = 1;
        vkFormat = VK_FORMAT_R8_UNORM;
        isAlphaOnly = true;
        break;
    default:
        return {};
    }

    const int tightRowPitch = createInfo.width * kBytesPerPixel;
    const int srcRowPitch = createInfo.initialData.rowPitchBytes > 0
                                ? createInfo.initialData.rowPitchBytes
                                : tightRowPitch;
    if (srcRowPitch < tightRowPitch) {
        return {};
    }

    const unsigned char* uploadPixels = createInfo.initialData.pixels;
    mstd::vector<unsigned char> packedPixels;
    const bool shouldFlipY = createInfo.initialData.origin == TextureDataOrigin::TopLeft;
    const bool shouldPackRows = srcRowPitch != tightRowPitch;
    if (shouldFlipY || shouldPackRows) {
        packedPixels.resize(static_cast<mstd::size_t>(tightRowPitch * createInfo.height));
        for (int dstY = 0; dstY < createInfo.height; ++dstY) {
            int srcY = dstY;
            if (shouldFlipY) {
                srcY = (createInfo.height - 1) - dstY;
            }

            const auto* src =
                createInfo.initialData.pixels + static_cast<mstd::size_t>(srcY) * srcRowPitch;
            auto* dst = packedPixels.data() + static_cast<mstd::size_t>(dstY) * tightRowPitch;
            mstd::memcpy(dst, src, static_cast<mstd::size_t>(tightRowPitch));
        }
        uploadPixels = packedPixels.data();
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    TextureResource texture{};

    auto cleanupTexture = [&]() {
        if (texture.descriptorSet != VK_NULL_HANDLE && _textureDescriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(_device, _textureDescriptorPool, 1, &texture.descriptorSet);
            texture.descriptorSet = VK_NULL_HANDLE;
        }
        if (texture.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_device, texture.imageView, nullptr);
            texture.imageView = VK_NULL_HANDLE;
        }
        if (texture.image != VK_NULL_HANDLE) {
            vkDestroyImage(_device, texture.image, nullptr);
            texture.image = VK_NULL_HANDLE;
        }
        if (texture.memory != VK_NULL_HANDLE) {
            vkFreeMemory(_device, texture.memory, nullptr);
            texture.memory = VK_NULL_HANDLE;
        }
    };

    auto cleanupStaging = [&]() {
        if (stagingBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(_device, stagingBuffer, nullptr);
            stagingBuffer = VK_NULL_HANDLE;
        }
        if (stagingBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(_device, stagingBufferMemory, nullptr);
            stagingBufferMemory = VK_NULL_HANDLE;
        }
    };

    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(tightRowPitch) * createInfo.height;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &stagingBufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to create texture staging buffer.\n");
        return {};
    }

    VkMemoryRequirements stagingRequirements{};
    vkGetBufferMemoryRequirements(_device, stagingBuffer, &stagingRequirements);

    VkMemoryAllocateInfo stagingAllocInfo{};
    stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAllocInfo.allocationSize = stagingRequirements.size;
    stagingAllocInfo.memoryTypeIndex =
        findMemoryType(stagingRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (stagingAllocInfo.memoryTypeIndex == UINT32_MAX) {
        mstd::fprintf(stderr, "[Vulkan] Missing host-visible memory type for texture upload.\n");
        cleanupStaging();
        return {};
    }

    if (vkAllocateMemory(_device, &stagingAllocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to allocate texture staging memory.\n");
        cleanupStaging();
        return {};
    }

    if (vkBindBufferMemory(_device, stagingBuffer, stagingBufferMemory, 0) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to bind texture staging memory.\n");
        cleanupStaging();
        return {};
    }

    void* mapped = nullptr;
    if (vkMapMemory(_device, stagingBufferMemory, 0, imageSize, 0, &mapped) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to map texture staging memory.\n");
        cleanupStaging();
        return {};
    }
    mstd::memcpy(mapped, uploadPixels, static_cast<mstd::size_t>(imageSize));
    vkUnmapMemory(_device, stagingBufferMemory);

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(createInfo.width);
    imageInfo.extent.height = static_cast<uint32_t>(createInfo.height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = vkFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(_device, &imageInfo, nullptr, &texture.image) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to create texture image.\n");
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    VkMemoryRequirements imageRequirements{};
    vkGetImageMemoryRequirements(_device, texture.image, &imageRequirements);

    VkMemoryAllocateInfo imageAllocInfo{};
    imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    imageAllocInfo.allocationSize = imageRequirements.size;
    imageAllocInfo.memoryTypeIndex =
        findMemoryType(imageRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (imageAllocInfo.memoryTypeIndex == UINT32_MAX) {
        imageAllocInfo.memoryTypeIndex = findMemoryType(imageRequirements.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    if (imageAllocInfo.memoryTypeIndex == UINT32_MAX) {
        mstd::fprintf(stderr, "[Vulkan] Missing memory type for texture image.\n");
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    if (vkAllocateMemory(_device, &imageAllocInfo, nullptr, &texture.memory) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to allocate texture image memory.\n");
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    if (vkBindImageMemory(_device, texture.image, texture.memory, 0) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to bind texture image memory.\n");
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    VkImageViewCreateInfo imageViewInfo{};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.image = texture.image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = vkFormat;
    if (isAlphaOnly) {
        // Make a single-channel R8 image sample as (1, 1, 1, r) so the
        // shader's `color * texture(...)` formula works without branching.
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_ONE;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_ONE;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_ONE;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_R;
    }
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(_device, &imageViewInfo, nullptr, &texture.imageView) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to create texture image view.\n");
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    VkDescriptorSetAllocateInfo descriptorAllocInfo{};
    descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocInfo.descriptorPool = _textureDescriptorPool;
    descriptorAllocInfo.descriptorSetCount = 1;
    descriptorAllocInfo.pSetLayouts = &_textureDescriptorSetLayout;

    if (vkAllocateDescriptorSets(_device, &descriptorAllocInfo, &texture.descriptorSet) !=
        VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to allocate texture descriptor set.\n");
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = _commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(_device, &cmdAllocInfo, &cmd) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to allocate command buffer for texture upload.\n");
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to begin texture upload command buffer.\n");
        vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    VkImageMemoryBarrier toTransferBarrier{};
    toTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferBarrier.image = texture.image;
    toTransferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransferBarrier.subresourceRange.baseMipLevel = 0;
    toTransferBarrier.subresourceRange.levelCount = 1;
    toTransferBarrier.subresourceRange.baseArrayLayer = 0;
    toTransferBarrier.subresourceRange.layerCount = 1;
    toTransferBarrier.srcAccessMask = 0;
    toTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &toTransferBarrier);

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent = {static_cast<uint32_t>(createInfo.width),
                              static_cast<uint32_t>(createInfo.height), 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &copyRegion);

    VkImageMemoryBarrier toShaderReadBarrier{};
    toShaderReadBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toShaderReadBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShaderReadBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShaderReadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderReadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderReadBarrier.image = texture.image;
    toShaderReadBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toShaderReadBarrier.subresourceRange.baseMipLevel = 0;
    toShaderReadBarrier.subresourceRange.levelCount = 1;
    toShaderReadBarrier.subresourceRange.baseArrayLayer = 0;
    toShaderReadBarrier.subresourceRange.layerCount = 1;
    toShaderReadBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toShaderReadBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toShaderReadBarrier);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to end texture upload command buffer.\n");
        vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] Failed to submit texture upload.\n");
        vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);
        cleanupStaging();
        cleanupTexture();
        return {};
    }

    vkQueueWaitIdle(_graphicsQueue);
    vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);

    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.sampler = _textureSampler;
    descriptorImageInfo.imageView = texture.imageView;
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = texture.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &descriptorImageInfo;

    vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);

    cleanupStaging();

    TextureHandle handle;
    handle.value = _nextTextureHandle++;
    texture.width = createInfo.width;
    texture.height = createInfo.height;
    texture.bytesPerPixel = kBytesPerPixel;
    _textures.emplace(handle.value, texture);
    return handle;
}

void RenderDeviceVulkan::destroyTexture(TextureHandle texture) {
    if (!texture.isValid()) {
        return;
    }

    const auto it = _textures.find(texture.value);
    if (it == _textures.end()) {
        return;
    }

    TextureResource& resource = it->second;
    if (resource.descriptorSet != VK_NULL_HANDLE && _textureDescriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(_device, _textureDescriptorPool, 1, &resource.descriptorSet);
        resource.descriptorSet = VK_NULL_HANDLE;
    }
    if (resource.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_device, resource.imageView, nullptr);
        resource.imageView = VK_NULL_HANDLE;
    }
    if (resource.image != VK_NULL_HANDLE) {
        vkDestroyImage(_device, resource.image, nullptr);
        resource.image = VK_NULL_HANDLE;
    }
    if (resource.memory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, resource.memory, nullptr);
        resource.memory = VK_NULL_HANDLE;
    }

    _textures.erase(it);
}

void RenderDeviceVulkan::updateTextureRegion(TextureHandle texture, int x, int y, int width,
                                             int height, const TextureUploadData& data) {
    if (!_ready || _device == VK_NULL_HANDLE) {
        return;
    }
    if (!texture.isValid() || width <= 0 || height <= 0 || !data.pixels) {
        return;
    }
    const auto it = _textures.find(texture.value);
    if (it == _textures.end()) {
        return;
    }
    TextureResource& resource = it->second;
    if (x < 0 || y < 0 || x + width > resource.width || y + height > resource.height) {
        return;
    }

    // Match createTexture: textures are stored Y-flipped relative to TopLeft
    // origin sources so that the sampler-side UVs line up with GL. Translate
    // the destination y and reorder the source rows accordingly.
    const int kBytesPerPixel = resource.bytesPerPixel;
    const int tightRowPitch = width * kBytesPerPixel;
    const int srcRowPitch = data.rowPitchBytes > 0 ? data.rowPitchBytes : tightRowPitch;
    const bool shouldFlipY = data.origin == TextureDataOrigin::TopLeft;
    const int dstY = shouldFlipY ? (resource.height - y - height) : y;

    const VkDeviceSize uploadSize = static_cast<VkDeviceSize>(tightRowPitch) * height;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    auto cleanupStaging = [&]() {
        if (stagingBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(_device, stagingBuffer, nullptr);
            stagingBuffer = VK_NULL_HANDLE;
        }
        if (stagingMemory != VK_NULL_HANDLE) {
            vkFreeMemory(_device, stagingMemory, nullptr);
            stagingMemory = VK_NULL_HANDLE;
        }
    };

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = uploadSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        return;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(_device, stagingBuffer, &requirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(requirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX ||
        vkAllocateMemory(_device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        cleanupStaging();
        return;
    }
    if (vkBindBufferMemory(_device, stagingBuffer, stagingMemory, 0) != VK_SUCCESS) {
        cleanupStaging();
        return;
    }

    void* mapped = nullptr;
    if (vkMapMemory(_device, stagingMemory, 0, uploadSize, 0, &mapped) != VK_SUCCESS) {
        cleanupStaging();
        return;
    }
    auto* dst = static_cast<unsigned char*>(mapped);
    for (int row = 0; row < height; ++row) {
        const int srcRow = shouldFlipY ? (height - 1 - row) : row;
        mstd::memcpy(dst + static_cast<mstd::size_t>(row) * tightRowPitch,
                    data.pixels + static_cast<mstd::size_t>(srcRow) * srcRowPitch,
                    static_cast<mstd::size_t>(tightRowPitch));
    }
    vkUnmapMemory(_device, stagingMemory);

    // Wait for any in-flight frame to finish sampling the texture before we
    // transition it to TRANSFER_DST and overwrite a region of it.
    vkDeviceWaitIdle(_device);

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool = _commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(_device, &cmdAllocInfo, &cmd) != VK_SUCCESS) {
        cleanupStaging();
        return;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);
        cleanupStaging();
        return;
    }

    VkImageMemoryBarrier toTransfer{};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = resource.image;
    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransfer.subresourceRange.baseMipLevel = 0;
    toTransfer.subresourceRange.levelCount = 1;
    toTransfer.subresourceRange.baseArrayLayer = 0;
    toTransfer.subresourceRange.layerCount = 1;
    toTransfer.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toTransfer);

    VkBufferImageCopy copy{};
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = {x, dstY, 0};
    copy.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    vkCmdCopyBufferToImage(cmd, stagingBuffer, resource.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &copy);

    VkImageMemoryBarrier toShaderRead = toTransfer;
    toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &toShaderRead);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_graphicsQueue);
    vkFreeCommandBuffers(_device, _commandPool, 1, &cmd);

    cleanupStaging();
}

void RenderDeviceVulkan::drawSprite(const DrawSpriteCommand& command) {
    if (_textures.find(command.texture.value) == _textures.end()) {
        return;
    }

    const float w = command.contentSize.width;
    const float h = command.contentSize.height;
    const float u0 = command.uvRect.x;
    const float v0 = command.uvRect.y;
    const float u1 = command.uvRect.x + command.uvRect.width;
    const float v1 = command.uvRect.y + command.uvRect.height;

    const QuadVertex verts[] = {
        {{0.f, 0.f}, {u0, v0}}, {{w, 0.f}, {u1, v0}}, {{w, h}, {u1, v1}},
        {{0.f, 0.f}, {u0, v0}}, {{w, h}, {u1, v1}},   {{0.f, h}, {u0, v1}},
    };

    PendingDraw draw{};
    draw.textureHandle = command.texture.value;
    draw.firstVertex = appendQuadVertices(verts, 6, command.opacity);
    draw.vertexCount = 6;
    draw.mvp = _projection * command.world;
    _pendingDraws.push_back(draw);
}

void RenderDeviceVulkan::drawQuads(const DrawQuadsCommand& command) {
    if (_textures.find(command.texture.value) == _textures.end() || command.vertices.empty()) {
        return;
    }

    PendingDraw draw{};
    draw.textureHandle = command.texture.value;
    draw.firstVertex =
        appendQuadVertices(command.vertices.data(), command.vertices.size(), command.opacity);
    draw.vertexCount = static_cast<uint32_t>(command.vertices.size());
    draw.mvp = _projection * command.world;
    _pendingDraws.push_back(draw);
}

uint32_t RenderDeviceVulkan::appendQuadVertices(const QuadVertex* vertices, mstd::size_t vertexCount,
                                                float opacity) {
    if (!vertices || vertexCount == 0) {
        return 0;
    }

    const uint32_t firstVertex = static_cast<uint32_t>(_pendingVertices.size());
    const float intensity = clamp01(opacity);

    _pendingVertices.reserve(_pendingVertices.size() + vertexCount);
    for (mstd::size_t i = 0; i < vertexCount; ++i) {
        VulkanVertex vertex{};
        vertex.position[0] = vertices[i].position.x;
        vertex.position[1] = vertices[i].position.y;
        vertex.uv[0] = vertices[i].uv.x;
        vertex.uv[1] = vertices[i].uv.y;
        // The shader does `color * texture(...)`. Fold the per-command opacity
        // into the per-vertex color so a single draw can carry mixed tints.
        vertex.color[0] = vertices[i].color.r / 255.f;
        vertex.color[1] = vertices[i].color.g / 255.f;
        vertex.color[2] = vertices[i].color.b / 255.f;
        vertex.color[3] = (vertices[i].color.a / 255.f) * intensity;
        _pendingVertices.push_back(vertex);
    }

    return firstVertex;
}

void RenderDeviceVulkan::flushDrawCommands() {
    if (_pendingVertices.empty() || _pendingDraws.empty() || !_frameActive) {
        return;
    }

    if (_graphicsPipeline == VK_NULL_HANDLE && !createGraphicsPipeline()) {
        _pendingVertices.clear();
        _pendingDraws.clear();
        return;
    }

    const mstd::size_t vertexBytes = _pendingVertices.size() * sizeof(VulkanVertex);
    if (!ensureVertexBuffer(vertexBytes)) {
        _pendingVertices.clear();
        _pendingDraws.clear();
        return;
    }

    void* mapped = nullptr;
    if (vkMapMemory(_device, _vertexBufferMemory, 0, vertexBytes, 0, &mapped) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkMapMemory for vertex buffer failed.\n");
        _pendingVertices.clear();
        _pendingDraws.clear();
        return;
    }
    mstd::memcpy(mapped, _pendingVertices.data(), vertexBytes);
    vkUnmapMemory(_device, _vertexBufferMemory);

    VkCommandBuffer commandBuffer = _commandBuffers[_currentFrame];

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(_swapchainExtent.height);
    viewport.width = static_cast<float>(_swapchainExtent.width);
    viewport.height = -static_cast<float>(_swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapchainExtent;

    VkDeviceSize offsets[] = {0};
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_vertexBuffer, offsets);

    for (const PendingDraw& draw : _pendingDraws) {
        if (draw.vertexCount == 0) {
            continue;
        }

        const auto textureIt = _textures.find(draw.textureHandle);
        if (textureIt == _textures.end() || textureIt->second.descriptorSet == VK_NULL_HANDLE) {
            continue;
        }

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0,
                                1, &textureIt->second.descriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(Mat4), draw.mvp.m);
        vkCmdDraw(commandBuffer, draw.vertexCount, 1, draw.firstVertex, 0);
    }

    _pendingVertices.clear();
    _pendingDraws.clear();
}

bool RenderDeviceVulkan::ensureVertexBuffer(mstd::size_t requiredSizeBytes) {
    if (requiredSizeBytes == 0) {
        return true;
    }

    const VkDeviceSize requiredSize = static_cast<VkDeviceSize>(requiredSizeBytes);
    if (_vertexBuffer != VK_NULL_HANDLE && _vertexBufferSize >= requiredSize) {
        return true;
    }

    VkDeviceSize newSize = mstd::max(requiredSize, kInitialVertexBufferSize);
    if (_vertexBufferSize > 0) {
        newSize = mstd::max(newSize, _vertexBufferSize * 2);
    }

    if (_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_device, _vertexBuffer, nullptr);
        _vertexBuffer = VK_NULL_HANDLE;
    }
    if (_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_device, _vertexBufferMemory, nullptr);
        _vertexBufferMemory = VK_NULL_HANDLE;
    }
    _vertexBufferSize = 0;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = newSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_vertexBuffer) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateBuffer (vertex) failed.\n");
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(_device, _vertexBuffer, &requirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(requirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        mstd::fprintf(stderr, "[Vulkan] No suitable host-visible memory type for vertex buffer.\n");
        vkDestroyBuffer(_device, _vertexBuffer, nullptr);
        _vertexBuffer = VK_NULL_HANDLE;
        return false;
    }

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_vertexBufferMemory) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkAllocateMemory for vertex buffer failed.\n");
        vkDestroyBuffer(_device, _vertexBuffer, nullptr);
        _vertexBuffer = VK_NULL_HANDLE;
        return false;
    }

    if (vkBindBufferMemory(_device, _vertexBuffer, _vertexBufferMemory, 0) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkBindBufferMemory for vertex buffer failed.\n");
        vkFreeMemory(_device, _vertexBufferMemory, nullptr);
        _vertexBufferMemory = VK_NULL_HANDLE;
        vkDestroyBuffer(_device, _vertexBuffer, nullptr);
        _vertexBuffer = VK_NULL_HANDLE;
        return false;
    }

    _vertexBufferSize = newSize;
    return true;
}

uint32_t RenderDeviceVulkan::findMemoryType(uint32_t typeFilter,
                                            VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        const bool typeMatches = (typeFilter & (1u << i)) != 0;
        const bool propertiesMatch =
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (typeMatches && propertiesMatch) {
            return i;
        }
    }

    return UINT32_MAX;
}

bool RenderDeviceVulkan::initVulkan() {
    if (!_window) {
        mstd::fprintf(stderr, "[Vulkan] GLFW window is null.\n");
        return false;
    }

    if (!createInstance()) {
        return false;
    }
    if (!createSurface()) {
        return false;
    }
    if (!pickPhysicalDevice()) {
        return false;
    }
    if (!createLogicalDevice()) {
        return false;
    }
    if (!createSwapchain()) {
        return false;
    }
    if (!createRenderPass()) {
        return false;
    }
    if (!createDescriptorResources()) {
        return false;
    }
    if (!createGraphicsPipeline()) {
        return false;
    }
    if (!createFramebuffers()) {
        return false;
    }
    if (!createCommandPool()) {
        return false;
    }
    if (!createCommandBuffers()) {
        return false;
    }
    if (!createSyncObjects()) {
        return false;
    }

    return true;
}

bool RenderDeviceVulkan::createInstance() {
    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    if (!extensions || extensionCount == 0) {
        mstd::fprintf(stderr, "[Vulkan] glfwGetRequiredInstanceExtensions failed.\n");
        return false;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "zocos";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "zocos";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = extensionCount;
    instanceInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&instanceInfo, nullptr, &_instance) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateInstance failed.\n");
        return false;
    }

    return true;
}

bool RenderDeviceVulkan::createSurface() {
    if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] glfwCreateWindowSurface failed.\n");
        return false;
    }

    return true;
}

bool RenderDeviceVulkan::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        mstd::fprintf(stderr, "[Vulkan] No Vulkan physical device found.\n");
        return false;
    }

    mstd::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    for (VkPhysicalDevice device : devices) {
        uint32_t graphicsQueueFamily = UINT32_MAX;
        uint32_t presentQueueFamily = UINT32_MAX;
        if (isDeviceSuitable(device, graphicsQueueFamily, presentQueueFamily)) {
            _physicalDevice = device;
            _graphicsQueueFamily = graphicsQueueFamily;
            _presentQueueFamily = presentQueueFamily;
            return true;
        }
    }

    mstd::fprintf(stderr, "[Vulkan] No suitable Vulkan physical device found.\n");
    return false;
}

bool RenderDeviceVulkan::createLogicalDevice() {
    mstd::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    mstd::array<uint32_t, 2> queueFamilies = {_graphicsQueueFamily, _presentQueueFamily};

    const float queuePriority = 1.0f;
    for (uint32_t queueFamily : queueFamilies) {
        bool exists = false;
        for (const auto& queueCreateInfo : queueCreateInfos) {
            if (queueCreateInfo.queueFamilyIndex == queueFamily) {
                exists = true;
                break;
            }
        }
        if (exists) {
            continue;
        }

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.pEnabledFeatures = &features;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(kRequiredDeviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();

    if (vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateDevice failed.\n");
        return false;
    }

    vkGetDeviceQueue(_device, _graphicsQueueFamily, 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, _presentQueueFamily, 0, &_presentQueue);
    return true;
}

bool RenderDeviceVulkan::createSwapchain() {
    const SwapchainSupport support = querySwapchainSupport(_physicalDevice);
    if (support.formats.empty() || support.presentModes.empty()) {
        mstd::fprintf(stderr, "[Vulkan] Swapchain support is incomplete.\n");
        return false;
    }

    const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats);
    const VkPresentModeKHR presentMode = choosePresentMode(support.presentModes);
    const VkExtent2D extent = chooseExtent(support.capabilities);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {_graphicsQueueFamily, _presentQueueFamily};
    if (_graphicsQueueFamily != _presentQueueFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapchain) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateSwapchainKHR failed.\n");
        return false;
    }

    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, nullptr);
    _swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, _swapchainImages.data());

    _swapchainFormat = surfaceFormat.format;
    _swapchainExtent = extent;

    _swapchainImageViews.resize(_swapchainImages.size(), VK_NULL_HANDLE);
    for (size_t i = 0; i < _swapchainImages.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _swapchainFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(_device, &viewInfo, nullptr, &_swapchainImageViews[i]) !=
            VK_SUCCESS) {
            mstd::fprintf(stderr, "[Vulkan] vkCreateImageView failed.\n");
            return false;
        }
    }

    return true;
}

bool RenderDeviceVulkan::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateRenderPass failed.\n");
        return false;
    }

    return true;
}

bool RenderDeviceVulkan::createDescriptorResources() {
    if (_textureDescriptorSetLayout != VK_NULL_HANDLE && _textureDescriptorPool != VK_NULL_HANDLE &&
        _textureSampler != VK_NULL_HANDLE) {
        return true;
    }

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_textureDescriptorSetLayout) !=
        VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateDescriptorSetLayout failed.\n");
        return false;
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 2048;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 2048;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_textureDescriptorPool) !=
        VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateDescriptorPool failed.\n");
        vkDestroyDescriptorSetLayout(_device, _textureDescriptorSetLayout, nullptr);
        _textureDescriptorSetLayout = VK_NULL_HANDLE;
        return false;
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(_device, &samplerInfo, nullptr, &_textureSampler) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateSampler failed.\n");
        vkDestroyDescriptorPool(_device, _textureDescriptorPool, nullptr);
        _textureDescriptorPool = VK_NULL_HANDLE;
        vkDestroyDescriptorSetLayout(_device, _textureDescriptorSetLayout, nullptr);
        _textureDescriptorSetLayout = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

bool RenderDeviceVulkan::createGraphicsPipeline() {
    destroyGraphicsPipeline();

    if (_textureDescriptorSetLayout == VK_NULL_HANDLE) {
        mstd::fprintf(stderr, "[Vulkan] Texture descriptor set layout is not initialized.\n");
        return false;
    }

    VkShaderModule vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragShaderModule = VK_NULL_HANDLE;

    VkShaderModuleCreateInfo vertShaderInfo{};
    vertShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertShaderInfo.codeSize = sizeof(kVulkanMinimalVertSpv);
    vertShaderInfo.pCode = kVulkanMinimalVertSpv;
    if (vkCreateShaderModule(_device, &vertShaderInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateShaderModule(vertex) failed.\n");
        return false;
    }

    VkShaderModuleCreateInfo fragShaderInfo{};
    fragShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragShaderInfo.codeSize = sizeof(kVulkanMinimalFragSpv);
    fragShaderInfo.pCode = kVulkanMinimalFragSpv;
    if (vkCreateShaderModule(_device, &fragShaderInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateShaderModule(fragment) failed.\n");
        vkDestroyShaderModule(_device, vertShaderModule, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(VulkanVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    mstd::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(VulkanVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(VulkanVertex, uv);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(VulkanVertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    constexpr mstd::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    VkDescriptorSetLayout setLayouts[] = {_textureDescriptorSetLayout};
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(Mat4);

    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) !=
        VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreatePipelineLayout failed.\n");
        vkDestroyShaderModule(_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(_device, vertShaderModule, nullptr);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;

    const VkResult createPipelineResult = vkCreateGraphicsPipelines(
        _device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline);

    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(_device, vertShaderModule, nullptr);

    if (createPipelineResult != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateGraphicsPipelines failed.\n");
        destroyGraphicsPipeline();
        return false;
    }

    return true;
}

void RenderDeviceVulkan::destroyGraphicsPipeline() {
    if (_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
        _graphicsPipeline = VK_NULL_HANDLE;
    }
    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
}

void RenderDeviceVulkan::destroyDescriptorResources() {
    if (_textureSampler != VK_NULL_HANDLE) {
        vkDestroySampler(_device, _textureSampler, nullptr);
        _textureSampler = VK_NULL_HANDLE;
    }
    if (_textureDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(_device, _textureDescriptorPool, nullptr);
        _textureDescriptorPool = VK_NULL_HANDLE;
    }
    if (_textureDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_device, _textureDescriptorSetLayout, nullptr);
        _textureDescriptorSetLayout = VK_NULL_HANDLE;
    }
}

bool RenderDeviceVulkan::createFramebuffers() {
    _framebuffers.resize(_swapchainImageViews.size(), VK_NULL_HANDLE);
    for (size_t i = 0; i < _swapchainImageViews.size(); ++i) {
        VkImageView attachments[] = {_swapchainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _swapchainExtent.width;
        framebufferInfo.height = _swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_framebuffers[i]) !=
            VK_SUCCESS) {
            mstd::fprintf(stderr, "[Vulkan] vkCreateFramebuffer failed.\n");
            return false;
        }
    }

    return true;
}

bool RenderDeviceVulkan::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = _graphicsQueueFamily;

    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkCreateCommandPool failed.\n");
        return false;
    }

    return true;
}

bool RenderDeviceVulkan::createCommandBuffers() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = kMaxFramesInFlight;

    if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
        mstd::fprintf(stderr, "[Vulkan] vkAllocateCommandBuffers failed.\n");
        return false;
    }

    return true;
}

bool RenderDeviceVulkan::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) !=
            VK_SUCCESS) {
            mstd::fprintf(stderr, "[Vulkan] vkCreateSemaphore(imageAvailable) failed.\n");
            return false;
        }
        if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) !=
            VK_SUCCESS) {
            mstd::fprintf(stderr, "[Vulkan] vkCreateSemaphore(renderFinished) failed.\n");
            return false;
        }
        if (vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            mstd::fprintf(stderr, "[Vulkan] vkCreateFence failed.\n");
            return false;
        }
    }

    return true;
}

bool RenderDeviceVulkan::recreateSwapchain() {
    if (_device == VK_NULL_HANDLE) {
        return false;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    if (width <= 0 || height <= 0) {
        // Minimized window: skip this frame and try again later.
        return false;
    }

    vkDeviceWaitIdle(_device);

    cleanupSwapchain();

    if (!createSwapchain()) {
        return false;
    }
    if (!createRenderPass()) {
        return false;
    }
    if (!createGraphicsPipeline()) {
        return false;
    }
    if (!createFramebuffers()) {
        return false;
    }

    return true;
}

void RenderDeviceVulkan::cleanupSwapchain() {
    destroyGraphicsPipeline();

    for (VkFramebuffer framebuffer : _framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(_device, framebuffer, nullptr);
        }
    }
    _framebuffers.clear();

    if (_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(_device, _renderPass, nullptr);
        _renderPass = VK_NULL_HANDLE;
    }

    for (VkImageView imageView : _swapchainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(_device, imageView, nullptr);
        }
    }
    _swapchainImageViews.clear();
    _swapchainImages.clear();

    if (_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        _swapchain = VK_NULL_HANDLE;
    }
}

void RenderDeviceVulkan::cleanup() {
    if (_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_device);
    }

    for (uint32_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (_inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(_device, _inFlightFences[i], nullptr);
            _inFlightFences[i] = VK_NULL_HANDLE;
        }
        if (_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
            _renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }
        if (_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
            _imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    if (_device != VK_NULL_HANDLE) {
        mstd::vector<mstd::uint32_t> textureIds;
        textureIds.reserve(_textures.size());
        for (const auto& pair : _textures) {
            textureIds.push_back(pair.first);
        }
        for (mstd::uint32_t id : textureIds) {
            TextureHandle handle{};
            handle.value = id;
            destroyTexture(handle);
        }

        if (_vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(_device, _vertexBuffer, nullptr);
            _vertexBuffer = VK_NULL_HANDLE;
        }
        if (_vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(_device, _vertexBufferMemory, nullptr);
            _vertexBufferMemory = VK_NULL_HANDLE;
        }
        _vertexBufferSize = 0;

        if (_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(_device, _commandPool, nullptr);
            _commandPool = VK_NULL_HANDLE;
        }

        cleanupSwapchain();
        destroyDescriptorResources();

        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
    }

    if (_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
    if (_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }

    _ready = false;
    _frameActive = false;
    _graphicsQueue = VK_NULL_HANDLE;
    _presentQueue = VK_NULL_HANDLE;
    _physicalDevice = VK_NULL_HANDLE;
    _graphicsQueueFamily = UINT32_MAX;
    _presentQueueFamily = UINT32_MAX;
    _swapchainFormat = VK_FORMAT_UNDEFINED;
    _swapchainExtent = {};
    _projection = Mat4::identity();
    _pendingVertices.clear();
    _pendingDraws.clear();
    _textures.clear();
    _nextTextureHandle = 1;
}

bool RenderDeviceVulkan::isDeviceSuitable(VkPhysicalDevice device, uint32_t& outGraphicsQueueFamily,
                                          uint32_t& outPresentQueueFamily) const {
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0) {
        return false;
    }

    mstd::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    outGraphicsQueueFamily = UINT32_MAX;
    outPresentQueueFamily = UINT32_MAX;

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueCount > 0 &&
            (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            outGraphicsQueueFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
        if (queueFamilies[i].queueCount > 0 && presentSupport == VK_TRUE) {
            outPresentQueueFamily = i;
        }

        if (outGraphicsQueueFamily != UINT32_MAX && outPresentQueueFamily != UINT32_MAX) {
            break;
        }
    }

    if (outGraphicsQueueFamily == UINT32_MAX || outPresentQueueFamily == UINT32_MAX) {
        return false;
    }

    if (!supportsDeviceExtensions(device)) {
        return false;
    }

    return supportsSwapchain(device);
}

bool RenderDeviceVulkan::supportsSwapchain(VkPhysicalDevice device) const {
    const SwapchainSupport support = querySwapchainSupport(device);
    return !support.formats.empty() && !support.presentModes.empty();
}

bool RenderDeviceVulkan::supportsDeviceExtensions(VkPhysicalDevice device) const {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    mstd::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         availableExtensions.data());

    mstd::set<mstd::string> requiredExtensions(kRequiredDeviceExtensions.begin(),
                                             kRequiredDeviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

RenderDeviceVulkan::SwapchainSupport
RenderDeviceVulkan::querySwapchainSupport(VkPhysicalDevice device) const {
    SwapchainSupport support;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &support.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);
    if (formatCount > 0) {
        support.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount,
                                             support.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);
    if (presentModeCount > 0) {
        support.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount,
                                                  support.presentModes.data());
    }

    return support;
}

VkSurfaceFormatKHR
RenderDeviceVulkan::chooseSurfaceFormat(const mstd::vector<VkSurfaceFormatKHR>& formats) const {
    for (const VkSurfaceFormatKHR& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formats[0];
}

VkPresentModeKHR
RenderDeviceVulkan::choosePresentMode(const mstd::vector<VkPresentModeKHR>& presentModes) const {
    for (VkPresentModeKHR mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D RenderDeviceVulkan::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_window, &width, &height);

    VkExtent2D extent{};
    extent.width = static_cast<uint32_t>(mstd::max(width, 1));
    extent.height = static_cast<uint32_t>(mstd::max(height, 1));

    extent.width = mstd::max(capabilities.minImageExtent.width,
                            mstd::min(capabilities.maxImageExtent.width, extent.width));
    extent.height = mstd::max(capabilities.minImageExtent.height,
                             mstd::min(capabilities.maxImageExtent.height, extent.height));

    return extent;
}

} // namespace zocos
