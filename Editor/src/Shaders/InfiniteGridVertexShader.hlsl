#pragma pack_matrix( row_major )

struct Output
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float2 camPos : TEXCOORD1;
};

#include "Include/MatricesConstantBuffer.hlsli"

#include "Include/LightConstantBuffer.hlsli"
#include "Include/GridCalculation.hlsli"

Output VSMain(uint VertexID : SV_VertexID)
{
    Output output;
    
    int idx = indices[VertexID];
    float3 position = pos[idx] * gridSize;
	
    position.x += camPos.x;
    position.z += camPos.z;

    output.camPos = camPos.xz;

    output.position = mul(float4(position, 1.0), modelMat);
    output.position = mul(output.position, viewMat);
    output.position = mul(output.position, projMat);
    
    output.uv = position.xz;
    
    return output;
}
