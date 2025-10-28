#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

#include "../Include/PostprocessingConstantbuffer.hlsli"

Texture2D tex : register(t0);
SamplerState samplerTex : register(s0);

// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
float3 QuadraticThreshold(float3 color, float threshold, float3 curve)
{
    // Pixel brightness
    float br = max(color.r, max(color.g, color.b));

    // Under-threshold part
    float rq = clamp(br - curve.x, 0.0, curve.y);
    rq = curve.z * rq * rq;

    // Combine and apply the brightness response curve
    color *= max(rq, br - threshold) / max(br, 1e-4);

    return color;
}

float4 Prefilter(float4 color, float2 uv)
{
    color = min(params.x, color); // clamp to max
    color = float4(QuadraticThreshold(color.rgb, threshold.x, threshold.yzw), 1.f);
    return color;
}

float4 PSMain(Input input) : SV_TARGET
{
    float4 color = tex.Sample(samplerTex, input.uv);
    return Prefilter(color, input.uv);
    
    //float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    //if (brightness > 1.0)
    //    color = float4(color.rgb, 1.0);
    //else
    //    color = float4(0.0, 0.0, 0.0, 1.0);
    //return color;
}