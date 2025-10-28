#pragma once

#include "Oeuvre/Core/Log.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "vk_mem_alloc.h"
#include <glm/glm.hpp>

#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            OV_CORE_ERROR("In file {} line {} detected Vulkan error: {}", __FILE__, __LINE__, string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

struct AllocatedImage 
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer 
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct VulkanVertex 
{
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
    int BoneIDs[4];
    float Weights[4];
};

struct SkeletalAnimationBuffer
{
    glm::mat4 finalBonesMatrices[100];
};

struct LightBuffer
{
    glm::vec4 lightPositionsAndIntensities[16];
    glm::vec4 lightColorsAndRanges[16];
    glm::vec3 camPos;
    int lightCount;
    glm::mat4 view;
    glm::vec3 lightDir;
    float sunlightPower;
    float skyboxIntensity;
    float farPlane;
    glm::vec2 padding;
};

struct MaterialBuffer
{
    glm::vec3 color;
    int sponza;
    glm::vec2 metalRough;
    int notTextured;
    int useNormalMap;
};

// holds the resources needed for a mesh
struct GPUMeshBuffers 
{
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

// push constants for our mesh object draws
struct GPUDrawPushConstants 
{
    glm::mat4 mvpMatrix;
    glm::mat4 modelMatrix;
    VkDeviceAddress vertexBuffer;
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection; // w for sun power
    glm::vec4 sunlightColor;
};