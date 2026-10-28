#version 450

#extension GL_EXT_buffer_reference: require
#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_nonuniform_qualifier: require

layout(set = 0, binding = 0) uniform sampler Sampler;
layout(set = 1, binding = 0) uniform texture2D InputImages[];

struct BlurData
{
    vec2 DestinationImageSize;
    uint SourceImageIndex;
    uint DestinationImageIndex;
    float SampleDistance;
};

layout (buffer_reference, std430, buffer_reference_align = 4) buffer BlurDataUBORef
{
    BlurData BlurDataUBO;
};

layout (push_constant) uniform PushConstants
{
    BlurDataUBORef BlurDataRef;
} PushConstant;

layout (location = 0) in vec2 InFragTexCoord;
layout (location = 0) out vec4 OutFragColor;

void main() {
    const BlurData blur_data = PushConstant.BlurDataRef.BlurDataUBO;
    ivec2 size = ivec2(blur_data.DestinationImageSize);

//    // Limit write to image bounds
//    if (thread_id.x >= size.x || thread_id.y >= size.y) {
//        return;
//    }
//
//    // debug color
//    vec4 color = vec4(thread_id / size, 0, 1);
//
//    imageStore(DestinationImages[blur_data.DestinationImageIndex], thread_id, color);

    //OutFragColor = vec4(0,1,0,1);

    OutFragColor = texture(sampler2D(InputImages[blur_data.SourceImageIndex], Sampler), InFragTexCoord);
}