#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
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

SamplerState linearTextureSampler : register(s0);
SamplerState pointTextureSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerComparisonState comparisonSampler : register(s2);

Texture2D gBufferAlbedo : register(t0);
Texture2D gBufferNormalRoughness : register(t1);
Texture2D gBufferPositionMetallic : register(t2);
Texture2D gBufferDepth : register(t3);
Texture2D gBufferLightViewPosition : register(t4);

Texture2D indirectDiffuse : register(t5);
Texture2D indirectConfidence : register(t6);
Texture2D indirectSpecular : register(t7);
Texture2D t_ssao : register(t8);

Texture2D<float> spotLightDepthMap : register(t12);

TextureCube<float> depthMaps[MAX_POINT_LIGHTS] : register(t15);

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

static const float2 DiscSamples16[] =
{ // 5 random points in disc with radius 2.500000
    float2(0.000000, 2.500000),
	float2(2.377641, 0.772542),
	float2(1.469463, -2.022543),
	float2(-1.469463, -2.022542),
	float2(-2.377641, 0.772543),
	float2(-1.459477, 0.118057),
	float2(-0.862243, -0.542883),
	float2(0.391576, 1.298885),
	float2(-0.915699, 0.026971),
	float2(-1.31299, 2.247145),
	float2(0.122017, 1.809441),
	float2(-2.361178, 1.408826),
	float2(2.314536, -1.321850),
	float2(-1.572230, 0.819497),
	float2(1.133181, -0.921345),
	float2(2.438403, -0.622744),
};

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
        float3 SamplePos = nlV + SideVector * DiscSamples16[i].x + UpVector * DiscSamples16[i].y;
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

    float tapoffset = (1.0f / 512.0f);

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
            sum += spotLightDepthMap.SampleCmpLevelZero(comparisonSampler, UVD.xy, UVD.z, int2(x, y));
        }
    }
    float shadowFactor = sum / ((range * 2 + 1) * (range * 2 + 1));
    
    return shadowFactor;
}

static const float PI = 3.14159265359;

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

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

float3 CalcLight(float2 uv)
{
    //float4 albedo4 = gBufferAlbedo.Sample(textureSampler, uv);
    //if (albedo4.a < 0.1)
    //    discard;

    //float3 albedo = pow(gBufferAlbedo.Sample(textureSampler, uv).rgb, 2.2);
    float4 albedo = gBufferAlbedo.Sample(pointTextureSampler, uv);
    //albedo.rgb = pow(gBufferAlbedo.Sample(textureSampler, uv).rgb, 2.2);
    float4 normalAndRough = gBufferNormalRoughness.Sample(pointTextureSampler, uv);
    float3 normal = normalAndRough.rgb;
    float roughness = normalAndRough.a;
    float metallic = gBufferPositionMetallic.Sample(pointTextureSampler, uv).w;
    
    float3 WorldPos = gBufferPositionMetallic.Sample(pointTextureSampler, uv).xyz;
    
    float4 lightViewPosition = gBufferLightViewPosition.Sample(pointTextureSampler, uv);
    
    float ao = 1.0;

    float3 N = normalize(normal);
    float3 V = normalize(camPos - WorldPos.xyz);

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo.rgb, metallic);
	           
    // reflectance equation
    float3 Lo = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < numLights; ++i)
    {
        // calculate per-light radiance
        float3 L = normalize(lightPos[i].xyz - WorldPos.xyz);
        float3 H = normalize(V + L);
        float distance = length(lightPos[i].xyz - WorldPos.xyz);
        //float attenuation = 1.0 / (distance * distance);
        //float attenuation = 10.0 / (distance);
        //float attenuation = 1.0;
        float attenuation = 1.0 / (conLinQuad[0] + conLinQuad[1] * distance + conLinQuad[2] * (distance * distance));
    
        float3 radiance = lightColor * attenuation;
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        float3 kS = F;
        float3 kD = float3(1.f, 1.f, 1.f) - kS;
        kD *= 1.0 - metallic;
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL;
    }

    // Spotlight ////////////////////////////////////////////////////////
    //float3 L = normalize(lightPos[numLights].xyz - WorldPos.xyz);
    //float3 H = normalize(V + L);
        
    //// cook-torrance brdf
    //float NDF = DistributionGGX(N, H, roughness);
    //float G = GeometrySmith(N, V, L, roughness);
    //float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
    //float3 kS = F;
    //float3 kD = float3(1.f, 1.f, 1.f) - kS;
    //kD *= 1.0 - metallic;
        
    //float3 numerator = NDF * G * F;
    //float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    //float3 specular = numerator / max(denominator, 0.001);
            
    //// add to outgoing radiance Lo
    //float NdotL = max(dot(N, L), 0.0);
    //Lo += (kD * albedo.rgb / PI + specular) * NdotL;
    ////////////////////////////////////////////////////////////////////

    float4 indirectDiff = indirectDiffuse.Sample(pointTextureSampler, uv);
    float indirectConf = indirectConfidence.Sample(pointTextureSampler, uv).r;
    float3 indirectSpec = indirectSpecular.Sample(pointTextureSampler, uv).rgb;
    
    float ambient = lerp(1, indirectDiff.a, indirectConf);
    indirectDiff.rgb = pow(max(0, indirectDiff.rgb), 0.7);
    
    float diffuse = 0.f;
    float shadow = 0.f;
    for (int j = 0; j < numLights; j++)
    {
        float3 lightDir = normalize(lightPos[j].xyz - WorldPos.xyz);
        float NdotL = saturate(dot(normal.xyz, lightDir));
    
        if (NdotL > 0.f)
        {
            float3 vL = WorldPos.xyz - lightPos[j].xyz;
            float3 L = normalize(vL);
    
            //float shadowAtt = PointShadowPCF(vL);
            float shadowAtt = _sampleCubeShadowPCFSwizzle3x3(j, L, vL);
            //float shadowAtt = _sampleCubeShadowPCFDisc16(L, vL);
        
            float distance = length(lightPos[j].xyz - WorldPos.xyz);
            float attenuation = 1.0 / (conLinQuad[0] + conLinQuad[1] * distance + conLinQuad[2] * (distance * distance));
            float3 radiance = lightColor * attenuation;
        
            diffuse += shadowAtt * NdotL * radiance;
            shadow += shadowAtt;
        }
    }
    
     // Spotlight shadow //////////////////////////////////////////////////////  
    //float spotShadowAtt = SpotShadowPCF(lightViewPosition);
    ///////////////////////////////////////////////////////////////////

    float3 color = float3(1.f, 1.f, 1.f);
    if (!showDiffuseTexture)
    {     
        float3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;
        
        if (enableGI)
            color = Lo * shadow  + albedo.rgb * (lightColor * ambient + indirectDiff.rgb) + albedo.a * indirectSpec.rgb * (1 - kS) * metallic;
        else
            color = Lo * shadow  + albedo.rgb * 0.05f;
    }
    else
    {
        color = indirectDiff.rgb;
    }
    
    color = ConvertToLDR(color);
    //color = color / (color + float3(1.0, 1.0, 1.0));
    //color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));  
    
    return color;
}

float4 PSMain(Input input) : SV_Target
{
    float3 color = CalcLight(input.uv);
    
    return float4(color, 1.f);
}
