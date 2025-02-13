#pragma pack_matrix( row_major )

struct Input
{
    float4 Pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

SamplerState textureSampler : register(s0);
Texture2D albedoMap : register(t0);

float4 PSMain(Input input) : SV_TARGET
{
    float4 albedo = albedoMap.Sample(textureSampler, input.uv);
    if (albedo.a < 0.1)
        discard;
    
    return albedo;
}