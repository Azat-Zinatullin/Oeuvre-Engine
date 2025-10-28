#pragma once
#include "Oeuvre/Renderer/RendererAPI.h"
#include "VkDescriptors.h"
#include "VkTypes.h"
#include <deque>

namespace Oeuvre
{
	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void push_function(std::function<void()>&& function) {
			deletors.push_back(function);
		}

		void flush() {
			// reverse iterate the deletion queue to execute all the functions
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
				(*it)(); //call functors
			}

			deletors.clear();
		}
	};

	struct FrameData
	{
		VkCommandPool _commandPool;
		VkCommandBuffer _mainCommandBuffer;
		VkSemaphore _swapchainSemaphore;
		VkFence _renderFence;
		DeletionQueue _deletionQueue;
	};

	constexpr unsigned int FRAME_OVERLAP = 3;

	class VulkanRendererAPI : public RendererAPI
	{
	public:
		VulkanRendererAPI();
		virtual ~VulkanRendererAPI() = default;

		void Init() override;
		void Shutdown() override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		void SetClearColor(const glm::vec4& color) override;
		void Clear() override;
		void OnWindowResize(int width, int height) override;

		void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount) override;

		void BeginFrame() override;
		void EndFrame(bool vSyncEnabled) override;

		GPUMeshBuffers UploadMesh(std::span<uint32_t> indices, std::span<VulkanVertex> vertices);

		void InitVulkan();
		void InitSwapchain();
		void InitCommands();
		void InitSyncStructures();
		void InitDescriptors();
		void InitPipelines();
		void InitBackgroundPipelines();

		void InitMeshPipeline();
		void InitDefaultData();
		void InitImgui();
		void InitUniformBuffers();

		void CreateSwapchain(uint32_t width, uint32_t height);
		void DestroySwapchain();
		void ResizeSwapchain();

		void ResizeDrawImage(uint32_t width, uint32_t height);

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

		void DrawBackground(VkCommandBuffer cmd);
		void DrawGeometry(VkCommandBuffer cmd);

		AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, const std::string& debugName = "");
		void DestroyBuffer(const AllocatedBuffer& buffer);

		AllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		AllocatedImage CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
		void DestroyImage(const AllocatedImage& img);

		FrameData& GetCurrentFrame() { return _frames[_frameNumber % FRAME_OVERLAP]; }

		bool _isInitialized = false;

		VkInstance _instance;
		VkDebugUtilsMessengerEXT _debug_messenger;
		VkSurfaceKHR _surface;
		VkDevice _device;
		VkPhysicalDevice _chosenGPU;
		VkQueue _graphicsQueue;
		uint32_t _graphicsQueueFamily;

		VmaAllocator _allocator;

		DeletionQueue _mainDeletionQueue;

		AllocatedImage _drawImage;
		AllocatedImage _depthImage;
		VkExtent2D _drawExtent;

		VkFormat _swapchainImageFormat;
		VkExtent2D _swapchainExtent;
		VkSwapchainKHR _swapchain;
		std::vector<VkImage> _swapchainImages;
		std::vector<VkImageView> _swapchainImageViews;
		int _swapchainImageCount;
		uint32_t _swapchainImageIndex;

		VkExtent2D _windowExtent = { 1600, 900 };
		bool _resizeRequested = false;

		FrameData _frames[FRAME_OVERLAP];
		int _frameNumber = 0;

		std::vector<VkSemaphore> _submitSemaphores;

		VkFence _immFence;
		VkCommandPool _immCommandPool;
		VkCommandBuffer _immCommandBuffer;		

		DescriptorAllocator _globalDescriptorAllocator;

		VkDescriptorSetLayout _drawImageDescriptorLayout;
		VkDescriptorSet _drawImageDescriptors;

		VkPipelineLayout _gradientPipelineLayout;
		VkPipeline _gradientPipeline;

		AllocatedBuffer _saBuffer;
		VkDescriptorSetLayout _saAndMatBufferLayout;
		VkDescriptorSet _saBufferDescriptorSet;

		AllocatedBuffer _lightBuffer;
		VkDescriptorSetLayout _lightBufferLayout;
		VkDescriptorSet _lightBufferDescriptorSet;

		VkPipelineLayout _meshPipelineLayout;
		VkPipeline _meshPipeline;

		AllocatedImage _imguiImage;
		VkDescriptorPool _imguiPool;

		bool _editorDisabled = false;

		VkSampler _defaultSamplerLinear;
		VkSampler _defaultSamplerNearest;

		VkDescriptorSetLayout _texturesLayout;
	};
}



