#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

Texture2D tex : register(t0);
Texture2D tex1 : register(t1);
SamplerState samplerTex : register(s0);

float4 PSMain(Input input) : SV_TARGET
{
    float4 first = tex.Sample(samplerTex, input.uv);
    float4 second = tex1.Sample(samplerTex, input.uv);
    
    return first + second;
}