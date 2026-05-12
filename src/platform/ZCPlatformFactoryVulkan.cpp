#include "base/ZCPlatformFactory.h"

#include "platform/ZCRenderDeviceVulkan.h"
#include "platform/ZCVulkanViewImpl.h"

#include <cstdio>
#include <memory>

namespace zocos {

std::unique_ptr<View> createDefaultView() { return std::make_unique<VulkanViewImpl>(); }

std::unique_ptr<RenderDevice> createDefaultRenderDevice(View& view) {
    auto* vkView = dynamic_cast<VulkanViewImpl*>(&view);
    if (!vkView) {
        std::fprintf(stderr, "Vulkan backend requires VulkanViewImpl.\n");
        return {};
    }

    auto renderDevice = std::make_unique<RenderDeviceVulkan>(vkView->window());
    if (!renderDevice->isReady()) {
        return {};
    }

    return renderDevice;
}

} // namespace zocos
