RWTexture2D<float4> tex : register(u0);

[numthreads(16, 16, 1)]
void CSMain(uint3 groupId : SV_GroupID,
    uint3 groupThreadId : SV_GroupThreadID,
    uint3 dispatchThreadId : SV_DispatchThreadID,
    uint groupIndex : SV_GroupIndex)
{ 
    float4 color = float4(0.f, 0.f, 0.f, 1.f);
    
    if (groupThreadId.x != 0 && groupThreadId.y != 0)
        color = float4(dispatchThreadId.xy / float2(1024.f, 1024.f), 1.f, 1.f);
    
    tex[dispatchThreadId.xy] = color;
}