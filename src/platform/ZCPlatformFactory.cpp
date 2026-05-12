#include "base/ZCPlatformFactory.h"

#include "platform/ZCGLViewImpl.h"
#include "platform/ZCRenderDeviceGL.h"

#include <memory>

namespace zocos {

std::unique_ptr<View> createDefaultView() { return std::make_unique<GLViewImpl>(); }

std::unique_ptr<RenderDevice> createDefaultRenderDevice() {
    return std::make_unique<RenderDeviceGL>();
}

} // namespace zocos
