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

        void Update(const FrameContext &frame_context);

        glm::vec3 Rotation{};
        glm::vec3 Position{};
        glm::vec3 LookAtPosition{};

        float FovY       = 60.0f;
        float Near       = 0.1f;
        float Far        = 1000.f;
        float OrbitSpeed = 0.0f;

        struct UniformData {
            glm::mat4 PerspectiveMatrix{};
            glm::mat4 ViewMatrix{};
            glm::vec4 ViewPos{};
        } UniformBufferData{};

        Buffered<std::unique_ptr<Buffer>> UniformBuffers{};
    };

} // namespace vkdemo

#endif // CAMERA_HPP
