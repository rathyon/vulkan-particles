public RWTexture2D<float4> ResultImage;
// public RWTexture2D<uint8_t4> resultImage; //todo Which one is correct?

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    int3 group_size = WorkgroupSize();
    int3 group_count = WorkgroupCount();
    int3 group_total = group_size * group_count;

    // Emulates UV gradient color shader
    ResultImage[GlobalInvocationID.xy] = float4((float)GlobalInvocationID.x / group_total.x, (float)GlobalInvocationID.y / group_total.y, 0, 1);
}