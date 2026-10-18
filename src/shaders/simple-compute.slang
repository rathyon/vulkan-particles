[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID, uint3 LocalInvocationID : SV_GroupThreadID)
{

}