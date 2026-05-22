#pragma once

#include "base/ZCRenderDevice.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace zocos {

class RenderDeviceVulkan : public RenderDevice {
public:
    explicit RenderDeviceVulkan(GLFWwindow* window);
    ~RenderDeviceVulkan() override;

    bool isReady() const { return _ready; }

    void beginFrame(const Mat4& projection, int framebufferWidth, int framebufferHeight) override;
    void submit(const RenderCommand& command) override;
    void endFrame() override;

    TextureHandle createTexture(const TextureCreateInfo& createInfo) override;
    void destroyTexture(TextureHandle texture) override;
    void updateTextureRegion(TextureHandle texture, int x, int y, int width, int height,
                             const TextureUploadData& data) override;

private:
    struct VulkanVertex {
        float position[2]{};
        float uv[2]{};
        float color[4]{};
    };

    struct PendingDraw {
        std::uint32_t textureHandle = 0;
        uint32_t firstVertex = 0;
        uint32_t vertexCount = 0;
        Mat4 mvp = Mat4::identity();
    };

    struct TextureResource {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        int width = 0;
        int height = 0;
        int bytesPerPixel = 4;
    };

    struct SwapchainSupport {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    bool initVulkan();
    bool createInstance();
    bool createSurface();
    bool pickPhysicalDevice();
    bool createLogicalDevice();
    bool createSwapchain();
    bool createRenderPass();
    bool createDescriptorResources();
    bool createGraphicsPipeline();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();
    bool recreateSwapchain();
    bool ensureVertexBuffer(std::size_t requiredSizeBytes);

    void cleanupSwapchain();
    void cleanup();
    void destroyDescriptorResources();
    void destroyGraphicsPipeline();
    void drawSprite(const DrawSpriteCommand& command);
    void drawQuads(const DrawQuadsCommand& command);
    uint32_t appendQuadVertices(const QuadVertex* vertices, std::size_t vertexCount, float opacity);
    void flushDrawCommands();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    bool isDeviceSuitable(VkPhysicalDevice device, uint32_t& outGraphicsQueueFamily,
                          uint32_t& outPresentQueueFamily) const;
    bool supportsSwapchain(VkPhysicalDevice device) const;
    bool supportsDeviceExtensions(VkPhysicalDevice device) const;
    SwapchainSupport querySwapchainSupport(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

    GLFWwindow* _window = nullptr;
    bool _ready = false;
    bool _frameActive = false;
    Mat4 _projection = Mat4::identity();

    VkInstance _instance = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _presentQueue = VK_NULL_HANDLE;
    uint32_t _graphicsQueueFamily = UINT32_MAX;
    uint32_t _presentQueueFamily = UINT32_MAX;

    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkFormat _swapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D _swapchainExtent{};
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout _textureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool _textureDescriptorPool = VK_NULL_HANDLE;
    VkSampler _textureSampler = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> _framebuffers;

    VkCommandPool _commandPool = VK_NULL_HANDLE;
    VkBuffer _vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory _vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceSize _vertexBufferSize = 0;
    static constexpr uint32_t kMaxFramesInFlight = 2;
    std::array<VkCommandBuffer, kMaxFramesInFlight> _commandBuffers{};
    std::array<VkSemaphore, kMaxFramesInFlight> _imageAvailableSemaphores{};
    std::array<VkSemaphore, kMaxFramesInFlight> _renderFinishedSemaphores{};
    std::array<VkFence, kMaxFramesInFlight> _inFlightFences{};
    uint32_t _currentFrame = 0;
    uint32_t _currentImageIndex = 0;
    std::vector<VulkanVertex> _pendingVertices;
    std::vector<PendingDraw> _pendingDraws;

    std::unordered_map<std::uint32_t, TextureResource> _textures;
    std::uint32_t _nextTextureHandle = 1;
};

} // namespace zocos
