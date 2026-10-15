cmake_minimum_required(VERSION 4.3)
project(vulkan_particles)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(vulkan_particles
        src/main.cpp
        src/device-context.hpp
        src/device-context.cpp
)

target_link_libraries(vulkan_particles ${Vulkan_LIBRARIES})

target_precompile_headers(vulkan_particles PUBLIC src/pch.hpp)