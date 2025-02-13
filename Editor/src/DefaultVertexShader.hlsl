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
    float3 fragPos : WSPOSITION;
};

cbuffer ObjectBuffer : register(b0)
{
    matrix modelMat;
    matrix viewMat;
    matrix projMat;
    matrix normalMat;
    matrix viewProjMatInv;
}

cbuffer LightBuffer : register(b1)
{
    matrix lightViewProjMat;
    float4x4 CubeView[6];
    float4x4 CubeProj;
    float4 lightPos[4];
    float4 conLinQuad;
    float3 lightColor;
    float bias;
    float3 camPos;
    int showDiffuseTexture;
    int numLights;
    int enableGI;
}

Output VSMain(Input input)
{
    Output output;
    output.position = mul(float4(input.position, 1.f), modelMat);
    output.fragPos = output.position.xyz;
    output.position = mul(output.position, viewMat);
    output.position = mul(output.position, projMat);
      
    output.normal = normalize(mul(float4(input.normal, 0.f), normalMat));
    output.uv = input.uv;
    
    return output;
}

