#include "camera.hpp"

#include "globals.hpp"
#include "renderer-context.hpp"

namespace vkdemo {
    Camera::Camera(const DeviceContext &device_context, Type type) : m_type(type) {
        for (auto &buffer: UniformBuffers) {
            buffer = std::make_unique<Buffer>(device_context, vk::BufferUsageFlagBits::eUniformBuffer, true, sizeof(UniformData));
        }
    }

    glm::vec3 Camera::GetEulerRotation() { return m_rotation; }
    glm::vec3 Camera::GetLookAt() { return m_look_at; }
    glm::vec3 Camera::GetTranslation() { return m_translation; }

    void Camera::SetEulerRotation(const glm::vec3 &rotation) { m_rotation = rotation; }
    void Camera::SetLookAt(const glm::vec3 &look_at) { m_look_at = look_at; }

    void Camera::SetPerspective(const float fovy, const float near, const float far) {
        m_fovy = fovy;
        m_near = near;
        m_far  = far;
    }
    void Camera::SetTranslation(const glm::vec3 &translation) { m_translation = translation; }

    void Camera::Update(const FrameContext *frame_context) {
        // Update transforms

        if (m_type == Type::LookAt) {
            UniformBufferData.ViewPosition = glm::vec4(m_translation, 1.0f);

            // "Orbit" type camera transformation. Handedness for camera's coordinate system has inverted Z, hence right handed
            UniformBufferData.ViewMatrix = glm::lookAtRH(m_translation, m_look_at, globals::kUpAxis);

            // Update uniform data
            UniformBufferData.PerspectiveMatrix = glm::perspective(glm::radians(m_fovy), globals::kAspectRatio, m_near, m_far);

            UniformBufferData.PerspectiveMatrix[1][1] *= -1; // Invert Y-Axis due to Vulkan <-> GLM/OpenGL coordinate system mismatch

        } else if (m_type == Type::FirstPerson) {
            throw std::logic_error{"Not implemented"};
            // glm::mat4 rot_mat = glm::mat4{1};
            // rot_mat = glm::rotate(rot_mat, glm::radians(Rotation.x * 1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            // rot_mat = glm::rotate(rot_mat, glm::radians(Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            // rot_mat = glm::rotate(rot_mat, glm::radians(Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        }

        memcpy(UniformBuffers[frame_context->Index]->AllocationInfo.pMappedData, &UniformBufferData, sizeof(UniformData));
    }
} // namespace vkdemo
