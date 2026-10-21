struct Particle {
	float4 Position;
	float4 Velocity;
	float4 Color;
};

struct WorldData {
    float DeltaTime;
    uint ParticleCount;
}

[[vk::binding(0, 0)]] RWStructuredBuffer<Particle> ParticlesIn;
[[vk::binding(1, 0)]] RWStructuredBuffer<Particle> ParticlesOut;
[[vk::binding(2, 0)]] ConstantBuffer<WorldData> WorldDataUBO;

[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    // Current SSBO index
    uint index = GlobalInvocationID.x;

    // Don't try to write beyond particle count
    if (index >= WorldDataUBO.ParticleCount) {
        return;
    }

    //int3 group_size = WorkgroupSize();
    //int3 group_count = WorkgroupCount();
    //int3 group_total = group_size * group_count;

    // Emulates UV gradient color shader
    // ResultImage[GlobalInvocationID.xy] = float4((float)GlobalInvocationID.x / group_total.x, (float)GlobalInvocationID.y / group_total.y, 0, 1);

    ParticlesOut[index].Position = ParticlesIn[index].Position;
    ParticlesOut[index].Velocity = ParticlesIn[index].Velocity;
    ParticlesOut[index].Color = ParticlesIn[index].Color;
}