#pragma pack_matrix( row_major )

cbuffer ObjectBuffer : register(b0)
{
    matrix modelMat;
    matrix viewMat;
    matrix projMat;
    matrix normalMat;
    matrix viewProjMatInv;
};

struct VertexInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    int4 boneIds : BONEIDS;
    float4 boneWeights : BONEWEIGHTS;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float3 localPos : TEXCOORD0;
};

SamplerState textureSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};
Texture2D<float4> HDRMap : register(t0);

PixelInput VSMain(VertexInput input)
{
    PixelInput output;

    output.localPos = input.position.xyz;

    float4x4 newView = viewMat;
    newView[3][0] = 0.0;
    newView[3][1] = 0.0;
    newView[3][2] = 0.0;
	
    output.position.w = 1.f;
    output.position = mul(float4(input.position, 1.f), newView);
    output.position = mul(output.position, projMat);

    output.position = output.position.xyzw;
    output.position.z = output.position.w * 0.9999;

    return output;
}

static const float2 invAtan = float2(0.1591, 0.3183);
float2 SampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

float4 PSMain(PixelInput input) : SV_TARGET
{
    float2 uv = SampleSphericalMap(normalize(input.localPos)); // make sure to normalize localPos
    float3 color = HDRMap.Sample(textureSampler, uv).rgb;
    
    //color = pow(color, float3(2.2, 2.2, 2.2));
    //color = color / (color + float3(1.0, 1.0, 1.0));
    //color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    color = min(color, float3(500.f, 500.f, 500.f));
    
    return float4(color, 1.0);
}