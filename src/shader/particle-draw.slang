struct Camera {
    matrix Perspective;
    matrix View;
    float4 ViewPosition;
}

[[vk::binding(0, 0)]] ConstantBuffer<Camera> CameraUBO;

struct VSInput
{
    float4 Position;
    float4 Velocity;
    float4 Color;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float4 Color;
    float PointSize : SV_PointSize;
};

[shader("vertex")]
VSOutput vs_main(VSInput input){
    VSOutput output;
    output.Position = input.Position;
    output.Color = input.Color;
    output.PointSize = 8.0;
    return output;
}

[shader("fragment")]
float4 fs_main(VSOutput input){
    return input.Color;
}