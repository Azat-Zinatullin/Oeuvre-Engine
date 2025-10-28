#pragma pack_matrix( row_major )

#include "Include/PostprocessingConstantBuffer.hlsli"

#define FXAA_PRESET 5

#include "FXAA.hlsl"

#include "ACES.hlsl"

struct Input
{
    float4 Pos : SV_Position;
    float2 tex : TEXCOORD0;
};

Texture2D t_Processed : register(t0);

SamplerState pointClampSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

static const float gamma = 2.2f;

float Luminance(float3 color)
{
    return 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;
}

float3 ConvertToLDR(float3 color)
{
    float srcLuminance = Luminance(color);

    float sqrWhiteLuminance = 50;
    float scaledLuminance = srcLuminance * 8;
    float mappedLuminance = (scaledLuminance * (1 + scaledLuminance / sqrWhiteLuminance)) / (1 + scaledLuminance);

    return color * (mappedLuminance / srcLuminance);
}

float3 LinearTosRGB(in float3 color)
{
    float3 x = color * 12.92f;
    float3 y = 1.055f * pow(saturate(color), 1.0f / 2.4f) - 0.055f;

    float3 clr = color;
    clr.r = color.r < 0.0031308f ? x.r : y.r;
    clr.g = color.g < 0.0031308f ? x.g : y.g;
    clr.b = color.b < 0.0031308f ? x.b : y.b;

    return clr;
}

float3 SRGBToLinear(in float3 color)
{
    float3 x = color / 12.92f;
    float3 y = pow(max((color + 0.055f) / 1.055f, 0.0f), 2.4f);

    float3 clr = color;
    clr.r = color.r <= 0.04045f ? x.r : y.r;
    clr.g = color.g <= 0.04045f ? x.g : y.g;
    clr.b = color.b <= 0.04045f ? x.b : y.b;

    return clr;
}

// Applies the filmic curve from John Hable's presentation
float3 ToneMapFilmicALU(in float3 color)
{
    color = max(0, color - 0.004f);
    color = (color * (6.2f * color + 0.5f)) / (color * (6.2f * color + 1.7f) + 0.06f);
    return color;
}

float3 _tonemap_uncharted2(const float3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 lumaBasedReinhardToneMapping(float3 color)
{
    float luma = dot(color, float3(0.2126f, 0.7152f, 0.0722f));
    float toneMappedLuma = luma / (1.f + luma);
    color *= toneMappedLuma / luma;
    color = pow(color, float3(1.f / gamma, 1.f / gamma, 1.f / gamma));
    return color;
}

// Uncharted 2
float3 uncharted2Tonemap(float3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 uncharted2(float3 color)
{
    const float W = 11.2;
    float exposureBias = 2.0;
    float3 curr = uncharted2Tonemap(exposureBias * color);
    float3 whiteScale = 1.0 / uncharted2Tonemap(float3(W, W, W));
    return curr * whiteScale;
}

float4 PSMain(Input input) : SV_Target
{
    FxaaTex fxaaTex;
    fxaaTex.smpl = pointClampSampler;
    fxaaTex.tex = t_Processed;
    
    uint dx, dy;
    t_Processed.GetDimensions(dx, dy);
    float2 rcpro = rcp(float2(dx, dy));
    
    float3 color = float3(1.f, 1.f, 1.f);
      
    if (enableFXAA)
    {
        color = FxaaPixelShader(input.tex, fxaaTex, rcpro);
    }
    else
    {
        color = t_Processed.Sample(pointClampSampler, input.tex).rgb;
    }
    
    switch (tonemapPreset)
    {
        case 0:         
            color = uncharted2(color);
            color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
            break;
        case 1:
            color = ACESFitted(color);
            color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));            
            break;
        case 2:
            //color = color / (color + float3(1.0, 1.0, 1.0));
            color = float3(1.0, 1.0, 1.0) - exp(-color * exposure);     
            color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
            break;
        case 3:
            color = color.rgb;
            break;
    }  
    
    return float4(color, 1.f);
}
