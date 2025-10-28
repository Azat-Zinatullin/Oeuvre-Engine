#ifndef MATRICES_CONSTANT_BUFFER_HLSLI
#define MATRICES_CONSTANT_BUFFER_HLSLI

cbuffer MatricesBuffer : register(b0)
{
    matrix modelMat;
    matrix viewMat;
    matrix projMat;
    matrix normalMat;
    matrix viewMatInv;
    matrix projMatInv;  
}

#endif // MATRICES_CONSTANT_BUFFER_HLSLI