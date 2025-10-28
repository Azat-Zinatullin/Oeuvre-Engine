#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

#include "Include/MatricesConstantBuffer.hlsli"

#include "Include/PostprocessingConstantBuffer.hlsli"

#include "Include/Lighting.hlsli"

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

struct MpData
{
    uint hoveringObjId;
    float3 debug;
};

RWStructuredBuffer<MpData> mpBuffer : register(u1);

static const float3 Fdielectric = float3(0.04, 0.04, 0.04);

//static const float4x4 biasMat = float4x4(
//	0.5, 0.0, 0.0, 0.5,
//	0.0, 0.5, 0.0, 0.5,
//	0.0, 0.0, 1.0, 0.0,
//	0.0, 0.0, 0.0, 1.0
//);

static const float4x4 biasMat = float4x4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0
);

#define CSM_FIRST 0

#if CSM_FIRST
#define SHADOW_MAP_CASCADE_COUNT 5
#else
#define SHADOW_MAP_CASCADE_COUNT 4
#endif




#define BLOCKER_SEARCH_NUM_SAMPLES 64 
#define PCF_NUM_SAMPLES 64 
#define NEAR_PLANE 0.1 
#define LIGHT_WORLD_SIZE .5 
#define LIGHT_FRUSTUM_WIDTH 3.75 
// Assuming that LIGHT_FRUSTUM_WIDTH == LIGHT_FRUSTUM_HEIGHT 
#define LIGHT_SIZE_UV (LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH) 

static const float2 PoissonDistribution[64] =
{
    float2(-0.94201624, -0.39906216),
	float2(0.94558609, -0.76890725),
	float2(-0.094184101, -0.92938870),
	float2(0.34495938, 0.29387760),
	float2(-0.91588581, 0.45771432),
	float2(-0.81544232, -0.87912464),
	float2(-0.38277543, 0.27676845),
	float2(0.97484398, 0.75648379),
	float2(0.44323325, -0.97511554),
	float2(0.53742981, -0.47373420),
	float2(-0.26496911, -0.41893023),
	float2(0.79197514, 0.19090188),
	float2(-0.24188840, 0.99706507),
	float2(-0.81409955, 0.91437590),
	float2(0.19984126, 0.78641367),
	float2(0.14383161, -0.14100790),
	float2(-0.413923, -0.439757),
	float2(-0.979153, -0.201245),
	float2(-0.865579, -0.288695),
	float2(-0.243704, -0.186378),
	float2(-0.294920, -0.055748),
	float2(-0.604452, -0.544251),
	float2(-0.418056, -0.587679),
	float2(-0.549156, -0.415877),
	float2(-0.238080, -0.611761),
	float2(-0.267004, -0.459702),
	float2(-0.100006, -0.229116),
	float2(-0.101928, -0.380382),
	float2(-0.681467, -0.700773),
	float2(-0.763488, -0.543386),
	float2(-0.549030, -0.750749),
	float2(-0.809045, -0.408738),
	float2(-0.388134, -0.773448),
	float2(-0.429392, -0.894892),
	float2(-0.131597, 0.065058),
	float2(-0.275002, 0.102922),
	float2(-0.106117, -0.068327),
	float2(-0.294586, -0.891515),
	float2(-0.629418, 0.379387),
	float2(-0.407257, 0.339748),
	float2(0.071650, -0.384284),
	float2(0.022018, -0.263793),
	float2(0.003879, -0.136073),
	float2(-0.137533, -0.767844),
	float2(-0.050874, -0.906068),
	float2(0.114133, -0.070053),
	float2(0.163314, -0.217231),
	float2(-0.100262, -0.587992),
	float2(-0.004942, 0.125368),
	float2(0.035302, -0.619310),
	float2(0.195646, -0.459022),
	float2(0.303969, -0.346362),
	float2(-0.678118, 0.685099),
	float2(-0.628418, 0.507978),
	float2(-0.508473, 0.458753),
	float2(0.032134, -0.782030),
	float2(0.122595, 0.280353),
	float2(-0.043643, 0.312119),
	float2(0.132993, 0.085170),
	float2(-0.192106, 0.285848),
	float2(0.183621, -0.713242),
	float2(0.265220, -0.596716),
	float2(-0.009628, -0.483058),
	float2(-0.018516, 0.435703)
};

float2 SamplePoisson(int index)
{
    return PoissonDistribution[index % 64];
}

float PenumbraSize(float zReceiver, float zBlocker) //Parallel plane estimation 
{
    return (zReceiver - zBlocker) / zBlocker;
}

float FindBlocker(float3 coords, uint cascade, float Bias)
{
    //This uses similar triangles to compute what  
    //area of the shadow map we should search 
    float lightSizeUV = cascadePlaneDistances[9].x;
     
    const float light_zNear = NEAR_PLANE; // 0.01 gives artifacts? maybe because of ortho proj?
    const float lightRadiusUV = 0.05;
    float searchWidth = lightSizeUV * (coords.z - light_zNear) / coords.z;
    
    int numBlockers = 0;
    float blockerSum = 0;
    
    for (int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; i++)
    {
        float shadowMapDepth = sunLightShadowMap.SampleLevel(pointTextureSampler, float3(coords.xy + SamplePoisson(i) * searchWidth, cascade), 0);
        if (shadowMapDepth < (coords.z - Bias))
        {
            blockerSum += shadowMapDepth;
            numBlockers++;
        }
    }
    
    if (numBlockers > 0)
        return blockerSum / float(numBlockers);
    
    return -1.0;
}

float PCF_Filter(float3 coords, float filterRadiusUV, uint cascade, float Bias)
{
    float sum = 0.0;
    for (int i = 0; i < PCF_NUM_SAMPLES; i++)
    {
        float2 offset = SamplePoisson(i) * filterRadiusUV;
        sum += sunLightShadowMap.SampleCmpLevelZero(comparisonSampler, float3(coords.xy + offset, cascade), coords.z - Bias);
    }
    return sum / PCF_NUM_SAMPLES;
}

float PCSS(float3 coords, uint cascade, float Bias)
{
    // STEP 1: blocker search 
    float avgBlockerDepth = FindBlocker(coords, cascade, Bias);
    if (avgBlockerDepth <= -1.0)   
        return 1.0;
    // STEP 2: penumbra size 
    float penumbraRatio = PenumbraSize(coords.z, avgBlockerDepth);
    
    float lightSizeUV = cascadePlaneDistances[9].x;
    
    float filterRadiusUV = penumbraRatio * lightSizeUV * NEAR_PLANE / coords.z;
    filterRadiusUV = min(filterRadiusUV, 0.002);
    // STEP 3: filtering 
    return PCF_Filter(coords, filterRadiusUV, cascade, Bias);
}
















float textureProj(float4 shadowCoord, float2 offset, uint cascadeIndex, float Bias)
{
    float shadow = 1.0;
    //float bias = 0.005;
    
    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
    {
        float dist = sunLightShadowMap.SampleCmpLevelZero(comparisonSampler, float3(shadowCoord.xy + offset, cascadeIndex), shadowCoord.z - Bias).r;
        if (shadowCoord.w)
        {
            return dist;
        }
    }
    return shadow;
}

float filterPCF(float4 sc, uint cascadeIndex, float Bias)
{
    int3 texDim;
    sunLightShadowMap.GetDimensions(texDim.x, texDim.y, texDim.z);
    float scale = 0.75;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1.5;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadowFactor += textureProj(sc, float2(dx * x, dy * y), cascadeIndex, Bias);
            count++;
        }
    }
    return shadowFactor / count;
}

float ShadowCalc(float3 WorldPos, float3 ViewPos, float3 Normal, float3 LightDir)
{
    // Get cascade index for the current fragment's view position
    uint cascadeIndex = 0;
    for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i)
    {
        if (abs(ViewPos.z) > cascadePlaneDistances[i].x)
        {
            cascadeIndex = i + 1;
        }
    }

	// Depth compare for shadowing
    float4 shadowCoord = mul(float4(WorldPos, 1.0), lightSpaceMatrices[cascadeIndex]);
 
    shadowCoord = shadowCoord / shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    shadowCoord.y = 1.0 - shadowCoord.y;
    
    
    // calculate bias (based on depth map resolution and slope)
    const float MINIMUM_SHADOW_BIAS = 0.002;
    //float Bias = max(MINIMUM_SHADOW_BIAS * (1.0 - dot(Normal, LightDir)), MINIMUM_SHADOW_BIAS);
    float Bias = max(10.0 * MINIMUM_SHADOW_BIAS * (1.0 - dot(Normal, LightDir)), MINIMUM_SHADOW_BIAS);
    //float biasModifier = cascadePlaneDistances[9].x;
    float biasModifier = lerp(0.016, 0.75, 1.0 / (float) (cascadeIndex + 1) - 0.25);
    if (cascadeIndex == SHADOW_MAP_CASCADE_COUNT - 1)
    {
        Bias *= 1.0 / (farPlane * biasModifier);
    }
    else
    {
        Bias *= 1.0 / (cascadePlaneDistances[cascadeIndex].x * biasModifier);
    }
      
    float shadow = 1.0;
    shadow = filterPCF(shadowCoord, cascadeIndex, Bias);
    //shadow = PCSS(shadowCoord.xyz, cascadeIndex, Bias);
    
    return shadow;
}

float ShadowCalculation(float3 fragPosWorldSpace, float3 Normal, float3 lightDir)
{
    // select cascade layer
    float4 fragPosViewSpace = mul(float4(fragPosWorldSpace, 1.0), viewMat);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < cascadePlaneDistances[i].x)
        {
            layer = i;
            break;
        }
    }
    if (layer == -1)
    {
        layer = cascadeCount;
    }
    
    float4 fragPosLightSpace = mul(float4(fragPosWorldSpace, 1.0), lightSpaceMatrices[layer]);
    // perform perspective divide
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
     
    // transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;
    
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 1.0;
    }
     
    // calculate bias (based on depth map resolution and slope)
    float3 normal = normalize(Normal);
    float3 LightDir = normalize(lightDir);
    float Bias = max(0.05 * (1.0 - dot(normal, LightDir)), 0.005);
    const float biasModifier = 0.5f;
    if (layer == cascadeCount)
    {
        Bias *= 1.0 / (farPlane * biasModifier);
    }
    else
    {
        Bias *= 1.0 / (cascadePlaneDistances[layer].x * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    
    uint texWidth, texHeight, texElements;
    sunLightShadowMap.GetDimensions(texWidth, texHeight, texElements);
    float2 texelSize = 1.0 / float2(texWidth, texHeight);
    
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
        [unroll]
        for (int y = -1; y <= 1; ++y)
        {
            //float pcfDepth = texture(shadowMap, float3(projCoords.xy + float2(x, y) * texelSize, layer)).r; 
            //float pcfDepth = sunLightShadowMap.SampleCmpLevelZero(comparisonSampler, float3(UVD.xy, bestCascade), UVD.z, int2(x, y));
            //float pcfDepth = sunLightShadowMap.Sample(linearClampSampler, float3(projCoords.xy + float2(x, y) * texelSize, layer));
            //shadow += (currentDepth - Bias) > pcfDepth ? 1.0 : 0.0;           
            //shadow += sunLightShadowMap.SampleCmpLevelZero(comparisonSampler, float3(projCoords.xy + float2(x, y) * texelSize, layer), currentDepth - Bias).r;
            shadow += sunLightShadowMap.SampleCmpLevelZero(comparisonSampler, float3(projCoords.xy, layer), currentDepth - Bias, int2(x, y)).r;
        }
    }
    shadow /= 9.0;
        
    return shadow;
}

float3 prefilteredReflection(float3 R, float roughness)
{
    const float MAX_REFLECTION_LOD = 4.0; // todo: param/const
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    float3 a = preFilterMap.SampleLevel(minMagLinearMipPointClampSampler, R, lodf).rgb;
    float3 b = preFilterMap.SampleLevel(minMagLinearMipPointClampSampler, R, lodc).rgb;
    return lerp(a, b, lod - lodf);
}

float3 IBL(float3 F0, float3 Lr, float3 Albedo, float Roughness, float Metalness, float3 Normal, float3 View, float NdotV)
{
    float3 irradiance = irradianceMap.Sample(linearClampSampler, Normal).rgb;
    float3 F = FresnelSchlickRoughness(F0, NdotV, Roughness);
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - Metalness;
    
    float3 diffuseIBL = Albedo * irradiance;
    
    float3 R = reflect(-View, Normal);
       
    const float MAX_REFLECTION_LOD = 4.0;
  
    float3 prefilteredColor = preFilterMap.SampleLevel(minMagLinearMipPointClampSampler, Lr, Roughness * MAX_REFLECTION_LOD).rgb;
    float2 specularBRDF = brdfLUT.Sample(pointWrapSampler, float2(NdotV, 1.0 - Roughness)).rg;
    float3 specularIBL = prefilteredColor * (F * specularBRDF.x + specularBRDF.y);

    return kD * diffuseIBL + specularIBL;
}

// this is supposed to get the world position from the depth buffer
float3 WorldPosFromDepth(float depth, float2 uv)
{
    float z = depth;

    float4 clipSpacePosition = float4(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0, z, 1.0);
    float4 viewSpacePosition = mul(clipSpacePosition, projMatInv);

    float4 worldSpacePosition = mul(viewSpacePosition, viewMatInv);

    worldSpacePosition /= worldSpacePosition.w;
    
    return worldSpacePosition.xyz;
}

float3 CalcLight(float2 uv, float4 position)
{
    float4 albedoMetallic = gBufferAlbedoMetallic.Sample(pointTextureSampler, uv);
    float4 normalAndRough = gBufferNormalRoughness.Sample(pointTextureSampler, uv);
    float depth = gBufferDepth.Sample(pointTextureSampler, uv).r;
    
    float3 Albedo = pow(albedoMetallic.rgb, float3(2.2, 2.2, 2.2));
    float3 Normal = normalize(normalAndRough.rgb);
    float Roughness = normalAndRough.a;
    Roughness = max(Roughness, 0.01); // As roughness and NdotL/NdotV approaches zero, G1 will approach infinity due to the divide, and you will get these super bright spots.
    
    float Metalness = albedoMetallic.a;
      
    float3 worldPos = WorldPosFromDepth(depth, uv);
   
    float3 posViewSpace = mul(float4(worldPos, 1.0), viewMat).xyz;
    
    float4 albedoOnlyMode = gBufferAlbedoOnlyMode.Sample(pointTextureSampler, uv);
    
    // Mouse picking 
    uint selectedObjectId = (uint) padding.z;
    float hoveringObjId = gBufferAlbedoOnlyMode.Sample(pointTextureSampler, padding.xy).b;
    if (padding.x != -1.f && hoveringObjId != -1.0 && (uint) hoveringObjId != 0xFFFFFFFF)
    {
        mpBuffer[0].hoveringObjId = (uint) hoveringObjId;
        mpBuffer[0].debug = posViewSpace.xyz;
    }
    else
    {
        mpBuffer[0].hoveringObjId = 0xFFFFFFFF;
        mpBuffer[0].debug = posViewSpace.xyz;
    }
    
    float3 View = normalize(camPos.xyz - worldPos.xyz);
    float NdotV = max(dot(Normal, View), 0.0);
    //NdotV = max(NdotV, 0.00005); // As roughness and NdotL/NdotV approaches zero, G1 will approach infinity due to the divide, and you will get these super bright spots.
    
    // Specular reflection vector
    float3 Lr = 2.0 * NdotV * Normal - View;
    
	// Fresnel reflectance, metals use albedo
    float3 F0 = lerp(Fdielectric, Albedo, Metalness);
    
    float3 directLighting = float3(0.0, 0.0, 0.0);
    directLighting += CalculatePointLights(F0, worldPos, Albedo, Roughness, Metalness, Normal, View, NdotV);

    // Sunlight ////////////////////////////////////////////////////////////////////////////////////////////////
    float3 L = normalize(sunLightDir.xyz);
    
#if CSM_FIRST  
    float shadowAtt = ShadowCalculation(worldPos, Normal, L);
#else
    float shadowAtt = ShadowCalc(worldPos, posViewSpace, Normal, L);
#endif
    
    directLighting += CalculateDirLight(F0, Albedo, Roughness, Metalness, Normal, View, NdotV) * shadowAtt;
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
     
    float hbao = t_hbao.Sample(linearClampSampler, uv).r;
    
    float3 color = float3(1.f, 1.f, 1.f);
    if (!showDiffuseTexture)
    {
        float3 ambient = IBL(F0, Lr, Albedo, Roughness, Metalness, Normal, View, NdotV) * skyboxIntensity;
        //float3 ambient = Albedo * 0.05;
            
        if (enableHBAO)
            ambient = ambient * hbao;
            
        float emission = albedoOnlyMode.y;
            
        color = directLighting + ambient + emission * Albedo;
        
        if (albedoOnlyMode.r == 0.5f)
            color = Albedo;
    }
    else
    {
        color = sunLightShadowMap.Sample(linearClampSampler, float3(uv, 3.f));
    }
       
    const int thickness = 3;
    
    const float3 outlineColor = SRGBToLinear(float3(1.0, 0.64705, 0.0));
    
    // outline effect
    if (thickness > 0 && gBufferSelectedObjectShape.Sample(pointTextureSampler, uv).r == 1.0)
    {
        float xs, ys;
        gBufferSelectedObjectShape.GetDimensions(xs, ys);
        float2 FragCoord = uv * float2(xs, ys);
       
        int i, j, r = thickness;
        
        float xx, yy, rr, x, y, dx, dy;
        dx = 1.0 / xs; // texel size
        dy = 1.0 / ys;
        //x = FragCoord.x * dx;
        //y = FragCoord.y * dy;
        x = position.x * dx;
        y = position.y * dy;
        rr = (float) thickness * thickness;
        [unroll]
        for (yy = y - (float(thickness) * dy), i = -r; i <= r; i++, yy += dy)  
            [unroll]
            for (xx = x - (float(thickness) * dx), j = -r; j <= r; j++, xx += dx)   
                if ((i * i) + (j * j) <= rr && gBufferSelectedObjectShape.Sample(pointTextureSampler, float2(xx, yy)).r < 0.01)
                {
                    color = outlineColor; // outline color
                    i = r + r + 1;
                    j = r + r + 1;
                    break;
                }
    }
    
    
    // if the pixel is black (we are on the silhouette)
    //if (gBufferSelectedObjectShape.Sample(pointTextureSampler, uv).r == 1.0)
    //{
    //    float tx, ty;
    //    gBufferSelectedObjectShape.GetDimensions(tx, ty);
        
    //    float2 size = 1.0f / float2(tx, ty);

    //    for (int i = -1; i <= +1; i++)
    //    {
    //        for (int j = -1; j <= +1; j++)
    //        {
    //            if (i == 0 && j == 0)
    //            {
    //                continue;
    //            }

    //            float2 offset = float2(i, j) * size;

    //            // and if one of the neighboring pixels is white (we are on the border)
    //            if (gBufferSelectedObjectShape.Sample(pointTextureSampler, uv + offset).r == 0.0)
    //            {
    //                color = outlineColor;
    //                i = 2;
    //                break;
    //            }
    //        }
    //    }
    //}
    
    
    if (showCascades)
    {
        // select cascade layer
#if CSM_FIRST
        int layer = -1;
        for (int i = 0; i < cascadeCount; ++i)
        {
            if (abs(posViewSpace.z) < cascadePlaneDistances[i].x)
            {
                layer = i;
                break;
            }
        }
        if (layer == -1)
        {
            layer = cascadeCount;
        }
#else
        uint layer = 0;
        for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i)
        {
            if (abs(posViewSpace.z) > cascadePlaneDistances[i].x)
            {
                layer = i + 1;
            }
        }
#endif      
    
        switch (layer)
        {
            case 0:
                color *= float3(1.0, 0.25, 0.25);
                break;
            case 1:
                color *= float3(0.25, 1.0, 0.25);
                break;
            case 2:
                color *= float3(0.25, 0.25, 1.0);
                break;
            case 3:
                color *= float3(1.0, 1.0, 0.25);
                break;
            case 4:
                color *= float3(0.25, 1.0, 1.0);
                break;
        }
        
        color = gBufferSelectedObjectShape.Sample(pointTextureSampler, uv).rgb;
    }
        
    //color = ConvertToLDR(color); 
    //color = color / (color + float3(1.0, 1.0, 1.0));
    //color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    
    //color = float3(brdfLUT.Sample(pointWrapSampler, uv).rg, 0.0);
    
    //color = worldPos;
    
    return color;
}

float4 PSMain(Input input) : SV_Target
{
    float3 color = CalcLight(input.uv, input.position);
    
    return float4(color, 1.f);
}
