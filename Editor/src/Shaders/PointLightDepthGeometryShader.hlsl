#pragma pack_matrix( row_major )

#include "Include/MatricesConstantBuffer.hlsli"

cbuffer PointLightShadowGen : register(b4)
{
    matrix CubeProj;
    matrix CubeView[6];
}

struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct Output
{
    float4 Pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    uint RTIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void GSMain(triangle Input input[3], inout TriangleStream<Output> OutStream)
{
    for (int iFace = 0; iFace < 6; iFace++)
    {
        Output output;

        output.RTIndex = iFace;

        for (int v = 0; v < 3; v++)
        {
            output.Pos = mul(input[v].position, CubeView[iFace]);
            output.Pos = mul(output.Pos, CubeProj);
            output.uv = input[v].uv;
            OutStream.Append(output);
        }
        OutStream.RestartStrip();
    }        
}
