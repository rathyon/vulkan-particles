#include "device-context.hpp"
#include "globals.hpp"
#include "renderer-context.hpp"
#include "window.hpp"

using namespace vkdemo;

int main() {

    Window window{globals::kWindowSize.x, globals::kWindowSize.y};
    DeviceContext device_context{};

    device_context.Initialize(window);

    RendererContext renderer_context{device_context};
    renderer_context.Initialize();

    // todo enter render loop here

    return 0;
}
