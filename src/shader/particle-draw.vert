#version 450

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec4 inVelocity;
layout (location = 2) in vec4 inColor;

layout (location = 0) out vec4 outColor;

void main() {
    gl_PointSize = 2.0;
    gl_Position = vec4(inPosition.xy, 0, 1);
    outColor = inColor;
}