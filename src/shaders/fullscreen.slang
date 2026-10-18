// Renders a full screen quad.
// Uses a "fake fullscreen quad" technique described by Sascha Willems, described here:
// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/

struct VSInput
{
    uint VertexIndex : SV_VertexID;
}

struct VSOut
{
    float4 Position : SV_Position;
    float2 TexCoord;
}

[shader("vertex")]
VSOut vs_main(VSInput input)
{
    VSOut output;
    output.TexCoord = float2((input.VertexIndex << 1) & 2, input.VertexIndex & 2);
    output.Position = float4(output.TexCoord * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 fs_main(VSOut input) : SV_Target
{
    return float4(input.TexCoord, 0, 1);
}