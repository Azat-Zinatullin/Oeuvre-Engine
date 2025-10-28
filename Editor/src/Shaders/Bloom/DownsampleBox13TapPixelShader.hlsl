#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

#include "../Include/PostprocessingConstantBuffer.hlsli"

Texture2D tex : register(t0);
SamplerState samplerTex : register(s0);

half4 DownsampleBox13Tap(float2 uv, float2 texelSize)
{
    half4 A = tex.Sample(samplerTex, uv + texelSize * float2(-1.0, -1.0));
    half4 B = tex.Sample(samplerTex, uv + texelSize * float2(0.0, -1.0));
    half4 C = tex.Sample(samplerTex, uv + texelSize * float2(1.0, -1.0));
    half4 D = tex.Sample(samplerTex, uv + texelSize * float2(-0.5, -0.5));
    half4 E = tex.Sample(samplerTex, uv + texelSize * float2(0.5, -0.5));
    half4 F = tex.Sample(samplerTex, uv + texelSize * float2(-1.0, 0.0));
    half4 G = tex.Sample(samplerTex, uv);
    half4 H = tex.Sample(samplerTex, uv + texelSize * float2(1.0, 0.0));
    half4 I = tex.Sample(samplerTex, uv + texelSize * float2(-0.5, 0.5));
    half4 J = tex.Sample(samplerTex, uv + texelSize * float2(0.5, 0.5));
    half4 K = tex.Sample(samplerTex, uv + texelSize * float2(-1.0, 1.0));
    half4 L = tex.Sample(samplerTex, uv + texelSize * float2(0.0, 1.0));
    half4 M = tex.Sample(samplerTex, uv + texelSize * float2(1.0, 1.0));

    half2 div = (1.0 / 4.0) * half2(0.5, 0.125);

    half4 o = (D + E + I + J) * div.x;
    o += (A + B + G + F) * div.y;
    o += (B + C + H + G) * div.y;
    o += (F + G + L + K) * div.y;
    o += (G + H + M + L) * div.y;

    return o;
}

float4 PSMain(Input input) : SV_TARGET
{ 
    float width;
    float height;
    tex.GetDimensions(width, height);
    float2 texelSize = float2(1.f / width, 1.f / height);
    
    float4 color = DownsampleBox13Tap(input.uv, texelSize);
    
    return color;
}