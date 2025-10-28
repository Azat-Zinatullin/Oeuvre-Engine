#pragma pack_matrix( row_major )

struct Input
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
};

struct Output
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : TEXCOORD1;
};

#include "Include/MatricesConstantBuffer.hlsli"

#include "Include/LightConstantBuffer.hlsli"

Output VSMain(Input input)
{
    Output output;
    
    float3 vertexPosition_worldspace = modelMat[3].xyz;
    
    float3 distToCamera = camPos.xyz - vertexPosition_worldspace;
    
    output.position = mul(float4(vertexPosition_worldspace, 1.0f), viewMat);
    float3 dist = float3(modelMat[0][0], modelMat[1][1], modelMat[2][2]);
    
    if (length(distToCamera) <= 2.5)
    {
        dist = lerp(0.0, float3(modelMat[0][0], modelMat[1][1], modelMat[2][2]), length(distToCamera) / 2.5);
    }   
    
    output.position = mul(float4(output.position + float4(input.position.xy * dist.xy, 0.0, 0.0)), projMat);  
    
    output.normal = normalize(mul(float4(input.normal, 1.f), normalMat));
    output.uv = input.uv;
    
    return output;
}