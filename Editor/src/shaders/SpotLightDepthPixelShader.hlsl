#pragma pack_matrix( row_major )

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 fragPos : POSITION;
};

cbuffer ObjectBuffer : register(b0)
{
    matrix modelMat;
    matrix viewMat;
    matrix projMat;
    matrix normalMat;
    matrix viewProjMatInv;
}

float4 PSMain(Input input) : SV_Target
{
    return float4(1.f, 1.f, 1.f, 1.f);
}
