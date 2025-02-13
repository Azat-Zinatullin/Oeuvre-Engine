struct PSInput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL;
    float3 positionWS : WSPOSITION;
    VxgiVoxelizationPSInputData vxgiData;
};

#define MAX_POINT_LIGHTS 16

cbuffer LightBuffer : register(b0)
{
    matrix lightViewProjMat;
    float4x4 CubeView[6];
    float4x4 CubeProj;
    float4 lightPos[4];
    float4 conLinQuad;
    float3 lightColor;
    float bias;
    float3 camPos;
    int showDiffuseTexture;
    int numLights;
}

Texture2D t_DiffuseColor : register(t0);

Texture2D normalMap : register(t1);
Texture2D roughnessMap : register(t2);
Texture2D metallicMap : register(t3);

Texture2D<float> spotLightDepthMap : register(t4);
TextureCube<float> depthMaps[MAX_POINT_LIGHTS] : register(t6);

SamplerState g_SamplerLinearWrap : register(s0);
SamplerComparisonState comparisonSampler : register(s1);

static const float PI = 3.14159265;

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

float PointShadowPCF(int depthmapIndex, float3 ToPixel)
{
    float3 ToPixelAbs = abs(ToPixel);
    float Z = max(ToPixelAbs.x, max(ToPixelAbs.y, ToPixelAbs.z));
    float2 lpv = float2(CubeProj[2][2], CubeProj[3][2]);
    float Depth = (lpv.x * Z + lpv.y) / Z - bias;
    return depthMaps[depthmapIndex].SampleCmpLevelZero(comparisonSampler, ToPixel, Depth);
}

void PSMain(PSInput IN)
{
    float3 worldPos = IN.positionWS.xyz;
    float3 normal = normalize(IN.normal.xyz);

    //float3 albedo = g_DiffuseColor.rgb;

    //if (g_DiffuseColor.a > 0)
    float3 albedo = t_DiffuseColor.Sample(g_SamplerLinearWrap, IN.texCoord.xy).rgb;

    float3 radiosity = 0;
  
    for (int i = 0; i < numLights; i++)
    {
        float3 LightDirection = normalize(lightPos[i].xyz - worldPos);
        //float NdotL = saturate(-dot(normal, LightDirection.xyz));
        float NdotL = max(dot(normal, LightDirection), 0.0);
    
        if (NdotL > 0)
        {
            float3 vL = worldPos.xyz - lightPos[i].xyz;
            float3 L = normalize(vL);
    
            float shadowAtt = PointShadowPCF(i, vL);
        
            float distance = length(vL);
            float attenuation = 1.0 / (conLinQuad[0] + conLinQuad[1] * distance + conLinQuad[2] * (distance * distance));
            float3 radiance = attenuation;

            radiosity += albedo.rgb * lightColor.rgb * (NdotL * shadowAtt * radiance);
        }
    }  
    
    // spotLight
    //float3 ToLight = normalize(lightPos[3].xyz - worldPos);

    //float NDotL = saturate(dot(normal, ToLight));
    
    //float4 lightSpacePos = mul(float4(IN.positionWS, 1.f), lightViewProjMat);
    //float spotShadowAtt = SpotShadowPCF(lightSpacePos);
    
    //radiosity += albedo.rgb * lightColor.rgb * (NDotL * spotShadowAtt);
    
   

    radiosity += albedo.rgb * VxgiGetIndirectIrradiance(worldPos, normal) / PI;

    VxgiStoreVoxelizationData(IN.vxgiData, normal, 1, radiosity, 0);
};