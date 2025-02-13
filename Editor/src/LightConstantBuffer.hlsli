
#define MAX_POINT_LIGHTS 16

cbuffer LightBuffer : register(b1)
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
    int enableGI;
}