#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "Include/PBR_Resources.hlsli"
#include "Include/LightConstantBuffer.hlsli"
#include "Include/PBR.hlsli"


cbuffer PointLightShadowGen : register(b4)
{
    matrix CubeProj;
    matrix CubeView[6];
}

static const float2 shadowMapSize = float2(1024.f, 1024.f);


float _vectorToDepth(float3 vec, float n, float f)
{
    float3 AbsVec = abs(vec);
    float LocalZcomp = max(AbsVec.x, max(AbsVec.y, AbsVec.z));

    float NormZComp = (f + n) / (f - n) - (2 * f * n) / (f - n) / LocalZcomp;
    return (NormZComp + 1.0) * 0.5;
}

float _vectorToDepth2(float3 ToPixel)
{
    float3 ToPixelAbs = abs(ToPixel);
    float Z = max(ToPixelAbs.x, max(ToPixelAbs.y, ToPixelAbs.z));
    float2 lpv = float2(CubeProj[2][2], CubeProj[3][2]);
    float Depth = (lpv.x * Z + lpv.y) / Z;
    return Depth;
}

float _sampleCubeShadowPCFDisc16(int cubemapIndex, float3 L, float3 vL)
{
    float3 SideVector = normalize(cross(L, float3(0, 0, 1)));
    float3 UpVector = cross(SideVector, L);

    SideVector *= 1.0 / 1024.0;
    UpVector *= 1.0 / 1024.0;
	
    //float sD = _vectorToDepth(vL, 0.1, 100);
    float sD = _vectorToDepth2(vL) - bias;
    
    float3 nlV = float3(L.xy, L.z);

    float totalShadow = 0;

	[UNROLL]
    for (int i = 0; i < 16; ++i)
    {
        float3 SamplePos = nlV + SideVector * poissonDisk[i].x + UpVector * poissonDisk[i].y;
        totalShadow += depthMaps[cubemapIndex].SampleCmpLevelZero(
				comparisonSampler,
				SamplePos,
				sD);
    }
    totalShadow /= 16;

    return totalShadow;
}

float _sampleCubeShadowPCFSwizzle3x3(int cubemapIndex, float3 L, float3 vL)
{
    //float sD = _vectorToDepth(vL, 1, 100);
    float sD = _vectorToDepth2(vL) - bias;

    float3 forward = float3(L.xy, L.z);
    float3 right = normalize(cross(forward, float3(0, 0, 1)));
    //float3 right = float3(forward.z, -forward.x, forward.y);
    right -= forward * dot(right, forward);
    right = normalize(right);
    float3 up = cross(right, forward);

    float tapoffset = 1.0f / 1024.0f;

    right *= tapoffset;
    up *= tapoffset;

    float3 v0;
    v0.x = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward - right - up, sD).r;
    v0.y = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward - up, sD).r;
    v0.z = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward + right - up, sD).r;
	
    float3 v1;
    v1.x = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward - right, sD).r;
    v1.y = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward, sD).r;
    v1.z = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward + right, sD).r;

    float3 v2;
    v2.x = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward - right + up, sD).r;
    v2.y = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward + up, sD).r;
    v2.z = depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, forward + right + up, sD).r;
	
	
    return dot(v0 + v1 + v2, .1111111f);
}

float PointShadowPCF(int cubemapIndex, float3 ToPixel)
{
    float3 ToPixelAbs = abs(ToPixel);
    float Z = max(ToPixelAbs.x, max(ToPixelAbs.y, ToPixelAbs.z));
    float2 lpv = float2(CubeProj[2][2], CubeProj[3][2]);
    float Depth = (lpv.x * Z + lpv.y) / Z - bias;
    return depthMaps[cubemapIndex].SampleCmpLevelZero(comparisonSampler, ToPixel, Depth);
}

float3 CalculateDirLight(float3 F0, float3 Albedo, float Roughness, float Metalness, float3 Normal, float3 V, float NdotV)
{
    float3 result = float3(0.0, 0.0, 0.0);
  
    if (sunLightPower == 0.0)
        return result;

    float3 L = normalize(sunLightDir.xyz);
    float3 Lradiance = sunLightPower * sunLightColor.rgb;
    float3 H = normalize(L + V);

	// Calculate angles between surface normal and various light vectors.
    float cosLi = max(0.0, dot(Normal, L));
    float cosLh = max(0.0, dot(Normal, H));

    float3 F = FresnelSchlickRoughness(F0, max(0.0, dot(H, V)), Roughness);
    float D = DistributionGGX(cosLh, Roughness);
    float G = GaSchlickGGX(cosLi, NdotV, Roughness);

    float3 kd = (1.0 - F) * (1.0 - Metalness);
    float3 diffuseBRDF = kd * Albedo;

	// Cook-Torrance
    float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * NdotV);
    specularBRDF = clamp(specularBRDF, float3(0.0, 0.0, 0.0), float3(10.0, 10.0, 10.0));
    result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
    
    return result;
}


float3 CalculatePointLights(float3 F0, float3 WorldPos, float3 Albedo, float Roughness, float Metalness, float3 Normal, float3 V, float NdotV)
{
    float3 result = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < numLights; ++i)
    {     
         // calculate per-light radiance
        float3 L = normalize(lightPos[i].xyz - WorldPos.xyz);
        float3 H = normalize(V + L);
        
        float distance = length(lightPos[i].xyz - WorldPos.xyz);
        float DistToLightNorm = 1.0 - saturate(distance * rangeRcpAndIntesity[i].x);
        float attenuation = DistToLightNorm * DistToLightNorm;
        float3 Lradiance = lightColor[i].rgb * attenuation * rangeRcpAndIntesity[i].y;
        
		// cook-torrance brdf
        float cosLi = max(0.0, dot(Normal, L));
        float cosLh = max(0.0, dot(Normal, H));

        float3 F = FresnelSchlickRoughness(F0, max(0.0, dot(H, V)), Roughness);
        float D = DistributionGGX(cosLh, Roughness);
        float G = GaSchlickGGX(cosLi, NdotV, Roughness);
        
        float3 kd = (1.0 - F) * (1.0 - Metalness);
        float3 diffuseBRDF = kd * Albedo;

		// Cook-Torrance
        float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * NdotV);
        specularBRDF = clamp(specularBRDF, float3(0.0, 0.0, 0.0), float3(10.0, 10.0, 10.0));
        
        float3 vL = WorldPos.xyz - lightPos[i].xyz;
        float3 Ll = normalize(vL);
        
        float shadowAtt = _sampleCubeShadowPCFSwizzle3x3(i, Ll, vL);
        
		// Total contribution for this light.
        result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi * shadowAtt;
    }
    return result;
}

#endif // LIGHTING_HLSLI