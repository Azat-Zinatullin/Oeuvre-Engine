#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

#include "../Include/PostprocessingConstantBuffer.hlsli"

Texture2D image : register(t0);
SamplerState textureSampler : register(s0);

static const float weight[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

float4 PSMain(Input input) : SV_TARGET
{
    float texWidth;
    float texHeight;
    image.GetDimensions(texWidth, texHeight);
    float2 tex_offset = 1.f / float2(texWidth, texHeight);
    
    //float2 tex_offset = 1.f / textureSize; // gets size of single texel
    
    float3 result = image.Sample(textureSampler, input.uv).rgb * weight[0]; // current fragment's contribution


    if (horizontalBlur == 1)
    {
        for (int i = 1; i < 5; ++i)
        {
            result += image.Sample(textureSampler, input.uv + float2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += image.Sample(textureSampler, input.uv - float2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for (int j = 1; j < 5; ++j)
        {
            result += image.Sample(textureSampler, input.uv + float2(0.0, tex_offset.y * j)).rgb * weight[j];
            result += image.Sample(textureSampler, input.uv - float2(0.0, tex_offset.y * j)).rgb * weight[j];
        }
    }
 
    return float4(result, 1.f);
}