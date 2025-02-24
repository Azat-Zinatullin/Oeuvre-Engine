#pragma pack_matrix( row_major )

#include "PostprocessingConstantBuffer.hlsli"


#define FXAA_PRESET 5

#include "FXAA.hlsl"

//#define FXAA_PC 1
//#define FXAA_HLSL_5 1
//#define FXAA_QUALITY__PRESET 12
//#define FXAA_GREEN_AS_LUMA 1

//#include "fxaa.hlsli"

struct Input
{
    float4 Pos : SV_Position;
    float2 tex : TEXCOORD0;
};

Texture2D t_Processed : register(t0);
SamplerState s_samLinear : register(s0);

float4 PSMain(Input input) : SV_Target
{   
    //float2 pos = (input.tex.xy * FxaaFloat2(0.5, 0.5)) + FxaaFloat2(0.5, 0.5);

    FxaaTex fxaaTex;
    fxaaTex.smpl = s_samLinear;
    fxaaTex.tex = t_Processed;
    
    uint dx, dy;
    t_Processed.GetDimensions(dx, dy);
    float2 rcpro = rcp(float2(dx, dy));
      
    return float4(FxaaPixelShader(input.tex, fxaaTex, rcpro), 1.f);  
}
