#ifndef MATERIAL_CONSTANT_BUFFER_HLSLI
#define MATERIAL_CONSTANT_BUFFER_HLSLI

cbuffer MaterialBuffer : register(b1)
{
    float3 materialColor : packoffset(c0);
    int sponza : packoffset(c0.w);
    int objectId : packoffset(c1);
    int notTextured : packoffset(c1.y);
    int albedoOnly : packoffset(c1.z);
    float materialRoughness : packoffset(c1.w);
    float materialMetallic : packoffset(c2);
    int terrain : packoffset(c2.y);
    float terrainUVScale : packoffset(c2.z);
    int objectOutlinePass : packoffset(c2.w);
    float cubemapLod : packoffset(c3);
    int useNormalMap : packoffset(c3.y);
    int invertNormalG : packoffset(c3.z);
    float emission : packoffset(c3.w);
    int texturesBindlessIndexStart : packoffset(c4);
    float3 mbPadding : packoffset(c4.y);
}

#endif // MATERIAL_CONSTANT_BUFFER_HLSLI