#ifndef LIGHT_CONSTANT_BUFFER_HLSLI
#define LIGHT_CONSTANT_BUFFER_HLSLI

#define MAX_POINT_LIGHTS 16

cbuffer LightBuffer : register(b3)
{
    matrix lightSpaceMatrices[16];     
    matrix lightViewProjMat;
    float4 lightPos[MAX_POINT_LIGHTS];
    float4 lightColor[MAX_POINT_LIGHTS];
    float4 rangeRcpAndIntesity[MAX_POINT_LIGHTS]; // rangeRcp - x, intensity - y
    float4 rcpFrame;
    float4 billboardCenterWS;
    float4 sunLightDir;
    float4 cascadePlaneDistances[16];
    float4 ambientDown;
    float4 ambientRange;
    float4 sunLightColor;
    float4 camPos;
    int hemisphericAmbient; 
    float bias;
    float skyboxIntensity;
    int showDiffuseTexture;
    int numLights;
    int showCascades;
    float sunLightPower;
    int cascadeCount;
    float farPlane;
    float3 padding;
}

#endif // LIGHT_CONSTANT_BUFFER_HLSLI