#include "base/ZCPlatformFactory.h"

#include "platform/vulkan/ZCRenderDeviceVulkan.h"
#include "platform/vulkan/ZCVulkanViewImpl.h"

#include <cstdio>
#include "base/ZCStd.h"

namespace zocos {

mstd::unique_ptr<View> createDefaultView() { return mstd::make_unique<VulkanViewImpl>(); }

mstd::unique_ptr<RenderDevice> createDefaultRenderDevice(View& view) {
    auto* vkView = dynamic_cast<VulkanViewImpl*>(&view);
    if (!vkView) {
        std::fprintf(stderr, "Vulkan backend requires VulkanViewImpl.\n");
        return {};
    }

    auto renderDevice = mstd::make_unique<RenderDeviceVulkan>(vkView->window());
    if (!renderDevice->isReady()) {
        return {};
    }

    return renderDevice;
}

} // namespace zocos
