#include "device-context.hpp"
#include "globals.hpp"
#include "renderer.hpp"
#include "window.hpp"

using namespace vkdemo;

int main() {

    Window window{globals::kWindowSize.x, globals::kWindowSize.y};
    DeviceContext context{};

    context.Initialize(window);

    Renderer renderer{context};
    renderer.Initialize();

    // todo enter render loop here

    return 0;
}
