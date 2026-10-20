#include "camera.hpp"

#include "globals.hpp"
#include "renderer-context.hpp"

namespace vkdemo {
    Camera::Camera(const DeviceContext &device_context, Type type) {
        for (auto &buffer: UniformBuffers) {
            buffer = std::make_unique<Buffer>(device_context, vk::BufferUsageFlagBits::eUniformBuffer, true, sizeof(UniformData));
        }
    }

    void Camera::Update(const FrameContext &frame_context) {
        // Update transforms
        // glm::mat4 rot_mat = glm::mat4{1};
        // rot_mat = glm::rotate(rot_mat, glm::radians(Rotation.x * 1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        // rot_mat = glm::rotate(rot_mat, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        // rot_mat = glm::rotate(rot_mat, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        UniformBufferData.ViewPos = glm::vec4(Position, 0.0f);

        // Orbit type camera transformation
        UniformBufferData.ViewMatrix = glm::lookAtLH(Position, LookAtPosition, globals::kUpAxis);

        // Update uniform data
        UniformBufferData.PerspectiveMatrix = glm::perspective(FovY, globals::kAspectRatio, Near, Far);

        memcpy(UniformBuffers[frame_context.Index]->AllocationInfo.pMappedData, &UniformBufferData, sizeof(UniformData));
    }
} // namespace vkdemo
