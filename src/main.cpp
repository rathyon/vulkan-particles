#include "device-context.hpp"
#include "globals.hpp"
#include "renderer-context.hpp"
#include "shader-library.hpp"
#include "window.hpp"

using namespace vkdemo;

int main() {

    Window window{globals::kWindowSize.x, globals::kWindowSize.y};
    DeviceContext device_context{};

    device_context.Initialize(window);

    RendererContext renderer_context{device_context};
    renderer_context.Initialize();

    ShaderLibrary shader_library{device_context};
    shader_library.Initialize();
    shader_library.LoadShaders();

    // todo enter render loop here

    return 0;
}
