#include "base/ZCPlatformFactory.h"

#include "platform/opengl/ZCGLViewImpl.h"
#include "platform/opengl/ZCRenderDeviceGL.h"

#include <memory>

namespace zocos {

std::unique_ptr<View> createDefaultView() { return std::make_unique<GLViewImpl>(); }

std::unique_ptr<RenderDevice> createDefaultRenderDevice(View& view) {
    (void)view;
    return std::make_unique<RenderDeviceGL>();
}

} // namespace zocos
