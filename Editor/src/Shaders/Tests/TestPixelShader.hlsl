struct Input
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

SamplerState linearSampler : register(s0);
Texture2D<float4> tex : register(t0);

float4 PSMain(Input input) : SV_TARGET
{
    return tex.Sample(linearSampler, input.uv);
}