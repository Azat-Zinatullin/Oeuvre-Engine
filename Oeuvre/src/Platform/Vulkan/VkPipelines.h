#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>

namespace VkUtil
{
    enum class ShaderKind
    {
        VERTEX_SHADER,
        FRAGMENT_SHADER,
        GEOMETRY_SHADER,
        COMPUTE_SHADER
    };

    bool LoadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
    bool ConvertToSPIRVAndLoadShaderModule(const char* filePath, const char* sourceName, ShaderKind kind, bool optimize, VkDevice device, VkShaderModule* outShaderModule);
}

class PipelineBuilder 
{
public:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineLayout _pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;

    PipelineBuilder() { Clear(); }

    void Clear();

    VkPipeline BuildPipeline(VkDevice device);
    void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode mode);
    void SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void SetMultisamplingNone();
    void DisableBlending();
    void SetColorAttachmentFormat(VkFormat format);
    void SetDepthFormat(VkFormat format);
    void DisableDepthtest();
    void EnableDepthtest(bool depthWriteEnable, VkCompareOp op);
};