cmake_minimum_required(VERSION 4.3)
project(vulkan_particles)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_COMPILE_WARNING_AS_ERROR 1)

include_directories(src)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

find_library(SDL_LIBRARY NAMES SDL3 HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)
find_library(Slang_LIBRARY NAMES slang slang-compiler HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

add_executable(vulkan_particles
        src/main.cpp
        src/device-context.hpp
        src/device-context.cpp
        src/window.cpp
        src/window.hpp
        src/util.hpp
        src/globals.hpp
        src/glm-usage.hpp
        src/vma-usage.cpp
        src/vma-usage.hpp
        src/renderer-context.cpp
        src/renderer-context.hpp
        src/shader-registry.cpp
        src/shader-registry.hpp
        src/renderer.cpp
        src/Renderer.hpp
        src/vk-util.hpp
        src/resource/image.cpp
        src/resource/image.hpp
        src/resource/buffer.cpp
        src/resource/buffer.hpp
        src/resource/texture.cpp
        src/resource/texture.hpp
        src/shader/fullscreen.slang
        src/shader/simple-compute.slang
        src/resource/render-attachment.cpp
        src/resource/render-attachment.hpp
        src/resource/swapchain.cpp
        src/resource/swapchain.hpp
        src/resource/sampled-image.cpp
        src/resource/sampled-image.hpp
        src/scene/camera.cpp
        src/scene/camera.hpp
)

target_link_libraries(vulkan_particles PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(vulkan_particles PRIVATE ${SDL_LIBRARY})
target_link_libraries(vulkan_particles PRIVATE ${Slang_LIBRARY})

# In case I want to use PCH
#target_precompile_headers(vulkan_particles PRIVATE src/pch.hpp)

# Copy shader into directory of executable
add_custom_command(
        TARGET vulkan_particles
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E
        copy_directory ${CMAKE_SOURCE_DIR}/src/shader/ $<TARGET_FILE_DIR:vulkan_particles>/shader/)