#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

#include "Include/MatricesConstantBuffer.hlsli"

#include "Include/PostprocessingConstantBuffer.hlsli"

SamplerState linearClampSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};
SamplerState minMagLinearMipPointClampSampler
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};
SamplerState pointWrapSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
};

static const float3 Fdielectric = float3(0.04, 0.04, 0.04);

#include "Include/LightConstantBuffer.hlsli"
#include "Include/PBR.hlsli"

cbuffer PointLightShadowGen : register(b4)
{
    matrix CubeProj;
    matrix CubeView[6];
}

SamplerState linearTextureSampler : register(s0);
SamplerState pointTextureSampler : register(s1);
SamplerComparisonState comparisonSampler : register(s2);

Texture2D gBufferAlbedo : register(t0);
Texture2D gBufferNormalRoughness : register(t1);
Texture2D gBufferPositionMetallic : register(t2);
Texture2D gBufferAlbedoOnlyMode : register(t3);

TextureCube<float> depthMaps[MAX_POINT_LIGHTS] : register(t4);

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

float3 CalculateDirLight(float3 F0, float3 Albedo, float Roughness, float Metalness, float3 Normal, float3 View, float NdotV)
{
    float3 result = float3(0.0, 0.0, 0.0);
  
    if (sunLightPower == 0.0)
        return result;

    float3 Li = normalize(sunLightDir.xyz);
    float3 Lradiance = sunLightPower * sunLightColor.rgb;
    float3 Lh = normalize(Li + View);

		// Calculate angles between surface normal and various light vectors.
    float cosLi = max(0.0, dot(Normal, Li));
    float cosLh = max(0.0, dot(Normal, Lh));

    float3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, View)), Roughness);
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


float3 CalculatePointLights(in float3 F0, float3 worldPos, float3 Albedo, float Roughness, float Metalness, float3 Normal, float3 View, float NdotV)
{
    float3 result = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < numLights; ++i)
    {
         // calculate per-light radiance
        float3 Li = normalize(lightPos[i].xyz - worldPos.xyz);
        float3 Lh = normalize(View + Li);
        
        float distance = length(lightPos[i].xyz - worldPos.xyz);
        float DistToLightNorm = 1.0 - saturate(distance * rangeRcpAndIntesity[i].x);
        float attenuation = DistToLightNorm * DistToLightNorm;
        float3 Lradiance = SRGBToLinear(lightColor[i].rgb) * attenuation * rangeRcpAndIntesity[i].y;
        
		// cook-torrance brdf
        // Calculate angles between surface normal and various light vectors.
        float cosLi = max(0.0, dot(Normal, Li));
        float cosLh = max(0.0, dot(Normal, Lh));

        float3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, View)), Roughness);
        float D = DistributionGGX(cosLh, Roughness);
        float G = GaSchlickGGX(cosLi, NdotV, Roughness);
        
        float3 kd = (1.0 - F) * (1.0 - Metalness);
        float3 diffuseBRDF = kd * Albedo;

		// Cook-Torrance
        float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * NdotV);
        specularBRDF = clamp(specularBRDF, float3(0.0, 0.0, 0.0), float3(10.0, 10.0, 10.0));
        
        float3 vL = worldPos.xyz - lightPos[i].xyz;
        float3 Ll = normalize(vL);
        
        float shadowAtt = _sampleCubeShadowPCFSwizzle3x3(i, Ll, vL);
        
		// Total contribution for this light.
        result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi * shadowAtt;
    }
    return result;
}

float linearizeDepth(float nonLinearDepth, float4x4 invProjMatrix)
{
    // Create NDC coordinates with the non-linear depth
    float4 ndcCoords = float4(0, 0, nonLinearDepth, 1.0f);

    // Unproject to view-space (homogenous)
    float4 viewCoords = mul(invProjMatrix, ndcCoords);

    // Divide by w to get actual view-space coordinates
    float linearDepth = viewCoords.z / viewCoords.w;

    return linearDepth;
}

float3 CalcLight(float2 uv, float4 position)
{
    //float4 albedo = pow(gBufferAlbedo.Sample(pointTextureSampler, uv).rgba, float4(2.2, 2.2, 2.2, 2.2));
    float4 albedo = gBufferAlbedo.Sample(pointTextureSampler, uv).rgba;
    
    float3 Albedo = albedo.rgb;
    
    float4 normalAndRough = gBufferNormalRoughness.Sample(pointTextureSampler, uv);
    float3 Normal = normalize(normalAndRough.rgb);
    float Roughness = normalAndRough.a;
    float Metalness = gBufferPositionMetallic.Sample(pointTextureSampler, uv).w;
    
    float3 worldPos = gBufferPositionMetallic.Sample(pointTextureSampler, uv).xyz;
    
    float4 albedoOnlyMode = gBufferAlbedoOnlyMode.Sample(pointTextureSampler, uv);
    
    float3 View = normalize(camPos.xyz - worldPos.xyz);
    float NdotV = max(dot(Normal, View), 0.0);
    
    // Specular reflection vector
    float3 Lr = 2.0 * NdotV * Normal - View;
    
	// Fresnel reflectance, metals use albedo
    float3 F0 = lerp(Fdielectric, Albedo, Metalness);
    
    float3 directLighting = float3(0.0, 0.0, 0.0);
    directLighting += CalculatePointLights(F0, worldPos, Albedo, Roughness, Metalness, Normal, View, NdotV);

    float3 color = float3(1.f, 1.f, 1.f);
     
    float3 ambient = Albedo * 0.05;
           
    color = directLighting + ambient; 
        
    if (albedoOnlyMode.r == 0.5f)
        color = albedo.rgb;

    color = ConvertToLDR(color);
    
    return color;
}

float4 PSMain(Input input) : SV_Target
{
    float3 color = CalcLight(input.uv, input.position);
    
    return float4(color, 1.f);
}
