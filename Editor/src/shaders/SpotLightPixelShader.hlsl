#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : POSITION;
};

cbuffer ObjectBuffer : register(b0)
{
    matrix modelMat;
    matrix viewMat;
    matrix projMat;
    matrix normalMat;
    matrix viewProjMatInv;
}

#include "LightConstantBuffer.hlsli"

SamplerState textureSampler : register(s0);
SamplerState clampSampler : register(s1);
SamplerComparisonState comparisonSampler : register(s2);

Texture2D albedoMap : register(t0);
Texture2D depthMap : register(t1);

static float2 shadowMapSize = float2(1024.f, 1024.f);

float2 texOffset(int u, int v)
{
    return float2(u * 1.0f / shadowMapSize.x, v * 1.0f / shadowMapSize.y);
}

float SpotShadowPCF(float4 lightSpacePosition)
{
    float3 UVD = lightSpacePosition.xyz / lightSpacePosition.w;
    
    UVD.xy = 0.5 * UVD.xy + 0.5;
    UVD.y = 1.0 - UVD.y;
    
    //apply shadow map bias
    UVD.z -= 0.0005;

	//PCF sampling for shadow map
    float sum = 0;
    float x, y;

    float range = 2;
	//perform PCF filtering on a 4 x 4 texel neighborhood
    [unroll]
    for (y = -range; y <= range; y += 1.0)
    {
        [unroll]
        for (x = -range; x <= range; x += 1.0)
        {
            //sum += depthMap.SampleCmpLevelZero(comparisonSampler, UVD.xy + texOffset(x, y), UVD.z));
            sum += depthMap.SampleCmpLevelZero(comparisonSampler, UVD.xy, UVD.z, int2(x, y));
        }
    }
    float shadowFactor = sum / ((range * 2 + 1) * (range * 2 + 1));
    
    return shadowFactor;
}

float3 CalcLight(float3 position, float4 lightSpacePosition, float3 normal, float4 diffuseColor)
{
    float3 ToLight = normalize(lightPos[0].xyz - position.xyz);

    float NDotL = saturate(dot(ToLight, normal));
    float3 finalColor = diffuseColor.rgb * NDotL;
    
    float shadowAtt = SpotShadowPCF(lightSpacePosition);
    
    finalColor *= shadowAtt;
    
    return finalColor;
}

float4 PSMain(Input input) : SV_Target
{
    float4 diffuseColor = albedoMap.Sample(textureSampler, input.uv);
    
    float3 ambient = diffuseColor.xyz * 0.1;
    
    float lightSpacePos = mul(float4(input.fragPos, 1.f), lightViewProjMat);
    float3 color = CalcLight(input.fragPos, lightSpacePos, input.normal, diffuseColor);
    return float4(ambient + color, 1.f);
}
