static const float2 arrBasePos[4] =
{
    float2(-1.0, 1.0),
	float2(1.0, 1.0),
	float2(-1.0, -1.0),
	float2(1.0, -1.0),
};

static const float2 arrUV[4] =
{
    float2(0.0, 0.0),
	float2(1.0, 0.0),
	float2(0.0, 1.0),
	float2(1.0, 1.0),
};

struct Output
{
    float4 position : SV_Position; 
    float2 uv : TEXCOORD0;
};

Output VSMain(uint VertexID : SV_VertexID)
{
    Output output;

    output.position = float4(arrBasePos[VertexID].xy, 0.0, 1.0);        
    output.uv = arrUV[VertexID].xy;
    
    return output;
}