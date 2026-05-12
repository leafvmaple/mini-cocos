#pragma once

#include <memory>

namespace zocos {

class View;
class RenderDevice;

std::unique_ptr<View> createDefaultView();
std::unique_ptr<RenderDevice> createDefaultRenderDevice(View& view);

} // namespace zocos
