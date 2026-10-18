#include "globals.hpp"

#include "window.hpp"
#include "renderer.hpp"

using namespace vkdemo;

int main() {

    Window window{globals::kWindowSize.x, globals::kWindowSize.y};

    Renderer renderer{};
    renderer.Initialize(window);

    // todo enter render loop here

    return 0;
}
