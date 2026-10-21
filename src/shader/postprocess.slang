[[vk::binding(0, 0)]]Texture2D InputTexture;
[[vk::binding(1, 0)]]SamplerState Sampler;

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
    // just passing through
    return InputTexture.Sample(Sampler, input.TexCoord);
}