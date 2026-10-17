#include "device-context.hpp"
#include "window.hpp"

#include <iostream>

using namespace vkdemo;

int main() {

    std::cout << "Before start" << std::endl;
    Window window{1280, 720};
    DeviceContext context{};

    context.Initialize(window);

    return 0;
}
