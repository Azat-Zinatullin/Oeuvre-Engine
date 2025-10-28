#ifndef POSTPROCESSING_CONSTANT_BUFFER_HLSLI
#define POSTPROCESSING_CONSTANT_BUFFER_HLSLI

cbuffer PostprocessingBuffer : register(b2)
{
    float4 kernelOffsets[16];
    int enableFXAA;
    int enableHBAO;
    int horizontalBlur;
    float sampleScale;
    float4 params;
    float4 threshold;
    float2 textureSize;
    int tonemapPreset;
    float exposure;
}

#endif // POSTPROCESSING_CONSTANT_BUFFER_HLSLI