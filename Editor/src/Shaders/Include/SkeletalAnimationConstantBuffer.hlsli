#ifndef SKELETAL_ANIMATION_CONSTANT_BUFFER_HLSLI
#define SKELETAL_ANIMATION_CONSTANT_BUFFER_HLSLI


static const int MAX_BONES = 100;
static const int MAX_BONE_INFLUENCE = 4;

cbuffer SkeletalAnimationBuffer : register(b2)
{
    matrix finalBonesMatrices[MAX_BONES];
};


#endif // SKELETAL_ANIMATION_CONSTANT_BUFFER_HLSLI