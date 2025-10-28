#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float2 camPos : TEXCOORD1;
};

struct Output
{
    float4 albedo : SV_Target0;
    float4 normalRoughness : SV_Target1;
    float4 positionMetallic : SV_Target2;
    float4 albedoOnly : SV_Target3;
};

#include "Include/GridCalculation.hlsli"

Output PSMain(Input input)
{
    Output output;
    
    output.albedo = gridColor(input.uv, input.camPos);
    output.normalRoughness = float4(0.f, 0.f, 0.f, 0.f);
    output.positionMetallic = float4(0.f, 0.f, 0.f, 0.f);
    output.albedoOnly = float4(0.5f, 0.f, (float) 0xFFFFFFFF, 0.f);
    
    return output;
}