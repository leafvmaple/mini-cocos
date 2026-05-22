#include "base/ZCPlatformFactory.h"

#include "platform/opengl/ZCGLViewImpl.h"
#include "platform/opengl/ZCRenderDeviceGL.h"

#include "base/ZCStd.h"

namespace zocos {

mstd::unique_ptr<View> createDefaultView() { return mstd::make_unique<GLViewImpl>(); }

mstd::unique_ptr<RenderDevice> createDefaultRenderDevice(View& view) {
    (void)view;
    return mstd::make_unique<RenderDeviceGL>();
}

} // namespace zocos
