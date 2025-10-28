#pragma pack_matrix( row_major )

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

#include "Include/LightConstantBuffer.hlsli"

[maxvertexcount(12)]
void GSMain(triangle Input input[3], inout TriangleStream<Output> OutStream)
{
    for (int iFace = 0; iFace < 4; iFace++)
    {
        Output output;

        output.RTIndex = iFace;
    
        for (int v = 0; v < 3; v++)
        {
            output.Pos = mul(input[v].position, lightSpaceMatrices[iFace]);
            output.uv = input[v].uv;
        
            OutStream.Append(output);
        }
        OutStream.RestartStrip();
    }
}