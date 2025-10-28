#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

#include "../Include/PostprocessingConstantbuffer.hlsli"

Texture2D tex : register(t0);
SamplerState samplerTex : register(s0);

half4 UpsampleTent(float2 uv, float2 texelSize, float4 sampleScale)
{
    float4 d = texelSize.xyxy * float4(1.0, 1.0, -1.0, 0.0) * sampleScale;
    half4 s;
    s =  tex.Sample(samplerTex, uv - d.xy);
    s += tex.Sample(samplerTex, uv - d.wy) * 2.0;
    s += tex.Sample(samplerTex, uv - d.zy);

    s += tex.Sample(samplerTex, uv + d.zw) * 2.0;
    s += tex.Sample(samplerTex, uv) * 4.0;
    s += tex.Sample(samplerTex, uv + d.xw) * 2.0;

    s += tex.Sample(samplerTex, uv + d.zy);
    s += tex.Sample(samplerTex, uv + d.wy) * 2.0;
    s += tex.Sample(samplerTex, uv + d.xy);

    return s * (1.0 / 16.0);
}


float4 PSMain(Input input) : SV_TARGET
{
    float width;
    float height;
    tex.GetDimensions(width, height);
    float2 texelSize = float2(1.f / width, 1.f / height);
     
    float4 color = UpsampleTent(input.uv, texelSize, sampleScale);
    
    return color;
}