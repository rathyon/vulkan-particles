#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "glm-usage.hpp"
#include "resource/buffer.hpp"
#include "util.hpp"

namespace vkdemo {
    struct FrameContext;
    class Camera {
    public:
        enum class Type : uint8_t { LookAt, FirstPerson };

        Camera(const DeviceContext &device_context, Type type);
        virtual ~Camera() = default;

        Camera(const Camera &)             = delete;
        Camera(const Camera &&)            = delete;
        Camera &operator=(const Camera &)  = delete;
        Camera &operator=(const Camera &&) = delete;

        [[nodiscard]] glm::vec3 GetEulerRotation();
        [[nodiscard]] glm::vec3 GetLookAt();
        [[nodiscard]] glm::vec3 GetTranslation();

        void SetEulerRotation(const glm::vec3 &rotation);
        void SetLookAt(const glm::vec3 &look_at);
        void SetPerspective(const float fovy, const float near, const float far);
        void SetTranslation(const glm::vec3 &translation);

        void Update(const FrameContext *frame_context);

        struct UniformData {
            glm::mat4 PerspectiveMatrix{};
            glm::mat4 ViewMatrix{};
            glm::vec4 ViewPosition{};
        } UniformBufferData{};

        Buffered<std::unique_ptr<Buffer>> UniformBuffers{};

    private:
        const Type m_type; // This could just as easily be done using polymorphism later on
        glm::vec3  m_rotation{};
        glm::vec3  m_translation{0, 0, -5};
        glm::vec3  m_look_at{};

        float m_fovy = 60.0f; // Vertical field of view, in degrees.
        float m_near = 0.1f;
        float m_far  = 1000.f;
    };

} // namespace vkdemo

#endif // CAMERA_HPP
