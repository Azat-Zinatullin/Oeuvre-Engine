#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : TEXCOORD1;
};

struct Output
{
    float4 albedoMetallic : SV_Target0;
    float4 normalRoughness : SV_Target1;
    float4 albedoOnlyMode : SV_Target2;
};

Texture2D muzzleFlashSpriteSheet : register(t0);
SamplerState textureSampler : register(s0);

Output PSMain(Input input)
{
    float4 tex = muzzleFlashSpriteSheet.Sample(textureSampler, input.uv / float2(4.f, 5.f));
    if (tex.a < 0.1f)
        discard;
    
    tex *= 2.f;
    
    Output output;
    output.albedoMetallic = float4(tex.rgb, 0.f);
    output.normalRoughness = float4(0.f, 0.f, 0.f, 0.f);
    output.albedoOnlyMode = float4(0.5f, 0.f, (float)0xFFFFFFFF, 0.f);
    
    return output;
}