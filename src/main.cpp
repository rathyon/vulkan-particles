#include "device-context.hpp"
#include "window.hpp"

using namespace vkdemo;

int main() {

    Window window{1280, 720};
    DeviceContext context{};

    context.Initialize(window);

    return 0;
}
