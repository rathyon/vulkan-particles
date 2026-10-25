cmake_minimum_required(VERSION 4.3)
project(vulkan_particles)

set(CMAKE_CXX_STANDARD 23)

# Using MSVC, set warnings as errors
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -WX")

# Using clang-cl, set warnings as errors
#set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-c++98-compat -Wno-switch-enum -Wno-lifetime-safety-cross-tu-constructor-suggestions -Wno-padded -Wno-lifetime-safety-intra-tu-constructor-suggestions")

include_directories(src)

# Note: for some strange reason, it seems SPIRV-Tools is dependent on shaderc in the Vulkan SDK for Windows
find_package(Vulkan REQUIRED glslang SPIRV-Tools shaderc_combined)

find_library(SDL_LIBRARY NAMES SDL3 HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)
find_library(Slang_LIBRARY NAMES slang slang-compiler HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)
find_library(glslang-default-resource-limits_LIBRARY NAMES glslang-default-resource-limits HINTS "$ENV{VULKAN_SDK}/lib" REQUIRED)

include_directories(${Vulkan_INCLUDE_DIRS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

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
        src/resource/render-attachment.cpp
        src/resource/render-attachment.hpp
        src/resource/swapchain.cpp
        src/resource/swapchain.hpp
        src/resource/sampled-image.cpp
        src/resource/sampled-image.hpp
        src/world/camera.cpp
        src/world/camera.hpp
)

target_link_libraries(vulkan_particles PRIVATE Vulkan::Vulkan ${SDL_LIBRARY} ${Slang_LIBRARY} Vulkan::glslang Vulkan::SPIRV-Tools Vulkan::shaderc_combined)
target_link_libraries(vulkan_particles PRIVATE
        debug "$ENV{VULKAN_SDK}/lib/glslang-default-resource-limitsd.lib"
        optimized "$ENV{VULKAN_SDK}/lib/glslang-default-resource-limits.lib")


# Copy shaders into directory of executable
add_custom_target(copy_shaders ALL
COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/shader/
        $<TARGET_FILE_DIR:vulkan_particles>/shader/
        COMMENT "Copying shaders files into bin/shaders"
)

add_dependencies(vulkan_particles  copy_shaders)