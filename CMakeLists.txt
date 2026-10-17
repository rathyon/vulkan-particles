cmake_minimum_required(VERSION 4.3)
project(vulkan_particles)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_COMPILE_WARNING_AS_ERROR 1)

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
        src/utils.hpp
        src/globals.hpp
        src/glm-usage.hpp
        src/vma-usage.cpp
        src/vma-usage.hpp
        src/renderer-context.cpp
        src/renderer-context.hpp
)

target_link_libraries(vulkan_particles PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(vulkan_particles PRIVATE ${SDL_LIBRARY})

# In case I want to use PCH
#target_precompile_headers(vulkan_particles PRIVATE src/pch.hpp)