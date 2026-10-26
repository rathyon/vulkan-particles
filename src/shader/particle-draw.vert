#version 450

layout (binding = 0) uniform Camera {
    mat4 Projection;
    mat4 View;
    vec4 ViewPos;
} CameraUBO;

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec4 inVelocity;
layout (location = 2) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main() {
    gl_PointSize = 2.0;
    gl_Position = CameraUBO.Projection * CameraUBO.View * vec4(inPosition.xyz, 1);
    outColor = inColor;
}