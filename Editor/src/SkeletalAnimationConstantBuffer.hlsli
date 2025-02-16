static const int MAX_BONES = 100;
static const int MAX_BONE_INFLUENCE = 4;

cbuffer SkeletalAnimationBuffer : register(b3)
{
    matrix finalBonesMatrices[MAX_BONES];
};