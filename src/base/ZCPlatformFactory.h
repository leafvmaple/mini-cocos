#pragma once

#include "base/ZCStd.h"

namespace zocos {

class View;
class RenderDevice;

mstd::unique_ptr<View> createDefaultView();
mstd::unique_ptr<RenderDevice> createDefaultRenderDevice(View& view);

} // namespace zocos
