#include "ovpch.h"
#include "VulkanRendererAPI.h"
#include "VkBootstrap.h"
#include <GLFW/glfw3.h>
#include "Platform/Windows/WindowsWindow.h"
#include <Oeuvre/Core/Log.h>
#include "VkInitializers.h"
#include "VkImages.h"
#include "VkPipelines.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <iostream>


namespace Oeuvre
{
	VulkanRendererAPI::VulkanRendererAPI()
	{
		Init();
	}

	void VulkanRendererAPI::Init()
	{
		InitVulkan();
		InitSwapchain();
		InitCommands();
		InitSyncStructures();
		InitDescriptors();
		InitUniformBuffers();
		InitPipelines();
		InitImgui();
		InitDefaultData();

		_isInitialized = true;
	}

	void VulkanRendererAPI::InitVulkan()
	{
		vkb::InstanceBuilder builder;

		bool bUseValidationLayers;
#ifdef _DEBUG
		bUseValidationLayers = true;
#else
		bUseValidationLayers = false;
#endif

		//make the vulkan instance, with basic debug features
		auto inst_ret = builder.set_app_name("Oeuvre Vulkan Application")
			.request_validation_layers(bUseValidationLayers)
			.use_default_debug_messenger()
			.require_api_version(1, 3, 0)
			.build();

		vkb::Instance vkb_inst = inst_ret.value();

		//grab the instance 
		_instance = vkb_inst.instance;
		_debug_messenger = vkb_inst.debug_messenger;


		VkResult err = glfwCreateWindowSurface(_instance, WindowsWindow::GetInstance()->GetGLFWwindow(), nullptr, &_surface);
		if (err)
		{
			OV_CORE_ERROR("Failed to create vulkan surface!");
		}

		//vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features.dynamicRendering = true;
		features.synchronization2 = true;

		//vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;


		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features)
			.set_required_features_12(features12)
			.set_surface(_surface)
			.add_required_extension("VK_EXT_scalar_block_layout")
			.select()
			.value();


		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

		// Get the VkDevice handle used in the rest of a vulkan application
		_device = vkbDevice.device;
		_chosenGPU = physicalDevice.physical_device;

		// use vkbootstrap to get a Graphics queue
		_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		// initialize the memory allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = _chosenGPU;
		allocatorInfo.device = _device;
		allocatorInfo.instance = _instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorInfo.pAllocationCallbacks = nullptr;

		vmaCreateAllocator(&allocatorInfo, &_allocator);

		_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying _allocator\n";
			vmaDestroyAllocator(_allocator);
			});
	}

	void VulkanRendererAPI::InitSwapchain()
	{
		CreateSwapchain(_windowExtent.width, _windowExtent.height);

		//draw image size will match the window
		VkExtent3D drawImageExtent = {
			_windowExtent.width,
			_windowExtent.height,
			1
		};

		//hardcoding the draw format to 32 bit float
		_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		_drawImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rimg_info = VkInit::ImageCreateInfo(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = VkInit::ImageviewCreateInfo(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

		_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
		_depthImage.imageExtent = drawImageExtent;
		VkImageUsageFlags depthImageUsages{};
		depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageCreateInfo dimg_info = VkInit::ImageCreateInfo(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

		//allocate and create the image
		vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depthImage.image, &_depthImage.allocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo dview_info = VkInit::ImageviewCreateInfo(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

		VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage.imageView));

		_mainDeletionQueue.push_function([&]() {
			std::cout << "Destroying _drawImage.imageView, _drawImage.image, _depthImage.imageView and _depthImage.image\n";
			vkDestroyImageView(_device, _drawImage.imageView, nullptr);
			vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);

			vkDestroyImageView(_device, _depthImage.imageView, nullptr);
			vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);
			});
	}

	void VulkanRendererAPI::CreateSwapchain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

		_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			//.use_default_format_selection()
			.set_desired_format(VkSurfaceFormatKHR{ _swapchainImageFormat, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			//use vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)//VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		_swapchainExtent = vkbSwapchain.extent;
		//store swapchain and its related images
		_swapchain = vkbSwapchain.swapchain;
		_swapchainImages = vkbSwapchain.get_images().value();
		_swapchainImageViews = vkbSwapchain.get_image_views().value();

		_swapchainImageCount = _swapchainImages.size();
	}

	void VulkanRendererAPI::DestroySwapchain()
	{
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);

		// destroy swapchain resources
		for (int i = 0; i < _swapchainImageViews.size(); i++) {

			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		}
	}

	void VulkanRendererAPI::ResizeSwapchain()
	{
		vkDeviceWaitIdle(_device);

		DestroySwapchain();

		//int width, height;
		//glfwGetWindowSize(WindowsWindow::GetInstance()->GetGLFWwindow(), &width, &height);

		RECT clientRect;
		GetClientRect(WindowsWindow::GetInstance()->GetHWND(), &clientRect);

		_windowExtent.width = clientRect.right - clientRect.left;
		_windowExtent.height = clientRect.bottom - clientRect.top;

		CreateSwapchain(_windowExtent.width, _windowExtent.height);

		_resizeRequested = false;
	}

	void VulkanRendererAPI::ResizeDrawImage(uint32_t width, uint32_t height)
	{
		vkDeviceWaitIdle(_device);

		vkDestroyImageView(_device, _drawImage.imageView, nullptr);
		vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);

		vkDestroyImageView(_device, _depthImage.imageView, nullptr);
		vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);

		vkDestroyImageView(_device, _imguiImage.imageView, nullptr);
		vmaDestroyImage(_allocator, _imguiImage.image, _imguiImage.allocation);

		_drawExtent.width = width;
		_drawExtent.height = height;

		VkExtent3D drawImageExtent = {
			width,
			height,
			1
		};

		//hardcoding the draw format to 32 bit float
		_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		_drawImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rimg_info = VkInit::ImageCreateInfo(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = VkInit::ImageviewCreateInfo(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

		_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
		_depthImage.imageExtent = drawImageExtent;
		VkImageUsageFlags depthImageUsages{};
		depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageCreateInfo dimg_info = VkInit::ImageCreateInfo(_depthImage.imageFormat, depthImageUsages, drawImageExtent);

		//allocate and create the image
		vmaCreateImage(_allocator, &dimg_info, &rimg_allocinfo, &_depthImage.image, &_depthImage.allocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo dview_info = VkInit::ImageviewCreateInfo(_depthImage.imageFormat, _depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

		VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImage.imageView));

		DescriptorWriter writer;
		writer.WriteImage(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		writer.UpdateSet(_device, _drawImageDescriptors);



		//hardcoding the draw format to 32 bit float
		_imguiImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		_imguiImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages2{};
		drawImageUsages2 |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages2 |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages2 |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages2 |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageCreateInfo rimg_info2 = VkInit::ImageCreateInfo(_imguiImage.imageFormat, drawImageUsages2, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo2 = {};
		rimg_allocinfo2.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo2.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		VK_CHECK(vmaCreateImage(_allocator, &rimg_info2, &rimg_allocinfo2, &_imguiImage.image, &_imguiImage.allocation, nullptr));

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info2 = VkInit::ImageviewCreateInfo(_imguiImage.imageFormat, _imguiImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_device, &rview_info2, nullptr, &_imguiImage.imageView));

	}

	void VulkanRendererAPI::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VK_CHECK(vkResetFences(_device, 1, &_immFence));
		VK_CHECK(vkResetCommandBuffer(_immCommandBuffer, 0));

		VkCommandBuffer cmd = _immCommandBuffer;

		VkCommandBufferBeginInfo cmdBeginInfo = VkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		function(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdinfo = VkInit::CommandBufferSubmitInfo(cmd);
		VkSubmitInfo2 submit = VkInit::SubmitInfo(&cmdinfo, nullptr, nullptr);

		// submit command buffer to the queue and execute it.
		//  _renderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immFence));

		VK_CHECK(vkWaitForFences(_device, 1, &_immFence, true, 9999999999));
	}

	void VulkanRendererAPI::InitCommands()
	{
		//create a command pool for commands submitted to the graphics queue.
		//we also want the pool to allow for resetting of individual command buffers
		VkCommandPoolCreateInfo commandPoolInfo = VkInit::CommandPoolCreateInfo(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

			// allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = VkInit::CommandBufferAllocateInfo(_frames[i]._commandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
		}

		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_immCommandPool));

		// allocate the command buffer for immediate submits
		VkCommandBufferAllocateInfo cmdAllocInfo = VkInit::CommandBufferAllocateInfo(_immCommandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_immCommandBuffer));

		_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying _immCommandBuffer and _immCommandPool\n";
			//vkFreeCommandBuffers(_device, _immCommandPool, 1, &_immCommandBuffer);
			vkDestroyCommandPool(_device, _immCommandPool, nullptr);
			});
	}

	void VulkanRendererAPI::InitSyncStructures()
	{
		//create syncronization structures
		//one fence to control when the gpu has finished rendering the frame,
		//and 2 semaphores to syncronize rendering with swapchain
		//we want the fence to start signalled so we can wait on it on the first frame
		VkFenceCreateInfo fenceCreateInfo = VkInit::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = VkInit::SemaphoreCreateInfo();

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
			VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
		}

		_submitSemaphores.resize(_swapchainImageCount);
		for (int i = 0; i < _swapchainImageCount; i++) {
			VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_submitSemaphores[i]));
		}

		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_immFence));
		_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying _immFence\n";
			vkDestroyFence(_device, _immFence, nullptr);
			});
	}

	void VulkanRendererAPI::InitDescriptors()
	{
		//create a descriptor pool that will hold 10 sets with 1 image each
		std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
		{
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 5},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}
		};

		_globalDescriptorAllocator.InitPool(_device, 1000, sizes);

		//make the descriptor set layout for our compute draw
		{
			DescriptorLayoutBuilder builder;
			builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			_drawImageDescriptorLayout = builder.Build(_device, VK_SHADER_STAGE_COMPUTE_BIT);
		}

		//allocate a descriptor set for our draw image
		_drawImageDescriptors = _globalDescriptorAllocator.Allocate(_device, _drawImageDescriptorLayout);

		DescriptorWriter writer;
		writer.WriteImage(0, _drawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		writer.UpdateSet(_device, _drawImageDescriptors);

		//make sure both the descriptor allocator and the new layout get cleaned up properly
		_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying _globalDescriptorAllocator\n";
			_globalDescriptorAllocator.DestroyPool(_device);

			vkDestroyDescriptorSetLayout(_device, _drawImageDescriptorLayout, nullptr);
			});
	}

	void VulkanRendererAPI::InitPipelines()
	{
		InitBackgroundPipelines();
		InitMeshPipeline();
	}

	void VulkanRendererAPI::InitBackgroundPipelines()
	{
		VkPipelineLayoutCreateInfo computeLayout{};
		computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		computeLayout.pNext = nullptr;
		computeLayout.pSetLayouts = &_drawImageDescriptorLayout;
		computeLayout.setLayoutCount = 1;

		VK_CHECK(vkCreatePipelineLayout(_device, &computeLayout, nullptr, &_gradientPipelineLayout));

		VkShaderModule computeDrawShader;
		//if (!VkUtil::LoadShaderModule("src/Shaders/GLSL/Vulkan/gradient.comp.spv", _device, &computeDrawShader))
		if (!VkUtil::ConvertToSPIRVAndLoadShaderModule("src/Shaders/GLSL/Vulkan/gradient.comp", "gradient.comp", VkUtil::ShaderKind::COMPUTE_SHADER, true, _device, &computeDrawShader))
		{
			OV_CORE_ERROR("Error when building the compute shader");
		}

		VkPipelineShaderStageCreateInfo stageinfo{};
		stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageinfo.pNext = nullptr;
		stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageinfo.module = computeDrawShader;
		stageinfo.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = nullptr;
		computePipelineCreateInfo.layout = _gradientPipelineLayout;
		computePipelineCreateInfo.stage = stageinfo;

		VK_CHECK(vkCreateComputePipelines(_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_gradientPipeline));

		vkDestroyShaderModule(_device, computeDrawShader, nullptr);

		_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying _gradientPipelineLayout and _gradientPipeline\n";
			vkDestroyPipelineLayout(_device, _gradientPipelineLayout, nullptr);
			vkDestroyPipeline(_device, _gradientPipeline, nullptr);
			});
	}

	void VulkanRendererAPI::InitMeshPipeline()
	{
		VkShaderModule triangleFragShader;
		//if (!VkUtil::LoadShaderModule("src/Shaders/GLSL/Vulkan/textured_mesh.frag.spv", _device, &triangleFragShader)) {
		if (!VkUtil::ConvertToSPIRVAndLoadShaderModule("src/Shaders/GLSL/Vulkan/textured_mesh.frag", "textured_mesh.frag", VkUtil::ShaderKind::FRAGMENT_SHADER, true, _device, &triangleFragShader)) {
			OV_CORE_ERROR("Error when building the textured mesh fragment shader module");
		}
		else {
			OV_CORE_INFO("Textured mesh fragment shader succesfully loaded");
		}

		VkShaderModule triangleVertexShader;
		//if (!VkUtil::LoadShaderModule("src/Shaders/GLSL/Vulkan/skinned_mesh.vert.spv", _device, &triangleVertexShader)) {
		if (!VkUtil::ConvertToSPIRVAndLoadShaderModule("src/Shaders/GLSL/Vulkan/skinned_mesh.vert", "skinned_mesh.vert", VkUtil::ShaderKind::VERTEX_SHADER, true, _device, &triangleVertexShader)) {
			OV_CORE_ERROR("Error when building the skinned mesh vertex shader module");
		}
		else {
			OV_CORE_INFO("Skinned mesh vertex shader succesfully loaded");
		}

		VkPushConstantRange bufferRange{};
		bufferRange.offset = 0;
		bufferRange.size = sizeof(GPUDrawPushConstants);
		bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		DescriptorLayoutBuilder layoutBuilder;
		layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		layoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		_texturesLayout = layoutBuilder.Build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);

		_mainDeletionQueue.push_function([&]() {
			std::cout << "Destroying _texturesLayout\n";
			vkDestroyDescriptorSetLayout(_device, _texturesLayout, nullptr);
			});

		VkDescriptorSetLayout layouts[] = { _saAndMatBufferLayout,
			_texturesLayout, _lightBufferLayout };

		VkPipelineLayoutCreateInfo pipeline_layout_info = VkInit::PipelineLayoutCreateInfo();
		pipeline_layout_info.pPushConstantRanges = &bufferRange;
		pipeline_layout_info.pushConstantRangeCount = 1;
		pipeline_layout_info.setLayoutCount = 3;
		pipeline_layout_info.pSetLayouts = layouts;

		VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_meshPipelineLayout));

		PipelineBuilder pipelineBuilder;

		//use the triangle layout we created
		pipelineBuilder._pipelineLayout = _meshPipelineLayout;
		//connecting the vertex and pixel shaders to the pipeline
		pipelineBuilder.SetShaders(triangleVertexShader, triangleFragShader);
		//it will draw triangles
		pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		//filled triangles
		pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
		//no backface culling
		pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		//no multisampling
		pipelineBuilder.SetMultisamplingNone();
		//no blending
		pipelineBuilder.DisableBlending();

		pipelineBuilder.EnableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

		//connect the image format we will draw into, from draw image
		pipelineBuilder.SetColorAttachmentFormat(_drawImage.imageFormat);
		pipelineBuilder.SetDepthFormat(_depthImage.imageFormat);

		//finally build the pipeline
		_meshPipeline = pipelineBuilder.BuildPipeline(_device);

		//clean structures
		vkDestroyShaderModule(_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

		_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying _meshPipelineLayout and _meshPipeline\n";
			vkDestroyPipelineLayout(_device, _meshPipelineLayout, nullptr);
			vkDestroyPipeline(_device, _meshPipeline, nullptr);
			});
	}

	void VulkanRendererAPI::InitDefaultData()
	{
		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

		sampl.magFilter = VK_FILTER_NEAREST;
		sampl.minFilter = VK_FILTER_NEAREST;
		vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerNearest);

		sampl.magFilter = VK_FILTER_LINEAR;
		sampl.minFilter = VK_FILTER_LINEAR;
		vkCreateSampler(_device, &sampl, nullptr, &_defaultSamplerLinear);

		_mainDeletionQueue.push_function([&]() {
			vkDestroySampler(_device, _defaultSamplerNearest, nullptr);
			vkDestroySampler(_device, _defaultSamplerLinear, nullptr);
			});
	}

	void VulkanRendererAPI::InitImgui()
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &_imguiPool));

		_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying _imguiPool\n";
			vkDestroyDescriptorPool(_device, _imguiPool, nullptr);
			});

		//draw image size will match the window
		VkExtent3D drawImageExtent = {
			_windowExtent.width,
			_windowExtent.height,
			1
		};

		//hardcoding the draw format to 32 bit float
		_imguiImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		_imguiImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImageCreateInfo rimg_info = VkInit::ImageCreateInfo(_imguiImage.imageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		VK_CHECK(vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_imguiImage.image, &_imguiImage.allocation, nullptr));

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = VkInit::ImageviewCreateInfo(_imguiImage.imageFormat, _imguiImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_imguiImage.imageView));

		_mainDeletionQueue.push_function([=]() {
			std::cout << "Destroying _imguiImage.imageView and _imguiImage.image\n";
			vkDestroyImageView(_device, _imguiImage.imageView, nullptr);
			vmaDestroyImage(_allocator, _imguiImage.image, _imguiImage.allocation);
			});
	}

	void VulkanRendererAPI::InitUniformBuffers()
	{
		_saBuffer = CreateBuffer(sizeof(SkeletalAnimationBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, "SaBuffer");
		_mainDeletionQueue.push_function([this]() {
			std::cout << "Destroying _saBuffer\n";
			DestroyBuffer(_saBuffer);
			});

		DescriptorLayoutBuilder layoutBuilder;
		layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		_saAndMatBufferLayout = layoutBuilder.Build(_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

		//allocate a descriptor set for our uniform buffer
		_saBufferDescriptorSet = _globalDescriptorAllocator.Allocate(_device, _saAndMatBufferLayout);

		DescriptorWriter writer;
		writer.WriteBuffer(0, _saBuffer.buffer, sizeof(SkeletalAnimationBuffer), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.UpdateSet(_device, _saBufferDescriptorSet);

		//make sure both the descriptor allocator and the new layout get cleaned up properly
		_mainDeletionQueue.push_function([&]() {
			std::cout << "Destroying _saAndMatBufferLayout\n";
			vkDestroyDescriptorSetLayout(_device, _saAndMatBufferLayout, nullptr);
			});

		// light uniform buffer /////////////////
		_lightBuffer = CreateBuffer(sizeof(LightBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO, "LightBuffer");
		_mainDeletionQueue.push_function([this]() {
			std::cout << "Destroying _lightBuffer\n";
			DestroyBuffer(_lightBuffer);
			});

		DescriptorLayoutBuilder layoutBuilder2;
		layoutBuilder2.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		_lightBufferLayout = layoutBuilder2.Build(_device, VK_SHADER_STAGE_FRAGMENT_BIT);

		//allocate a descriptor set for our uniform buffer
		_lightBufferDescriptorSet = _globalDescriptorAllocator.Allocate(_device, _lightBufferLayout);

		DescriptorWriter writer2;
		writer2.WriteBuffer(0, _lightBuffer.buffer, sizeof(LightBuffer), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer2.UpdateSet(_device, _lightBufferDescriptorSet);

		//make sure both the descriptor allocator and the new layout get cleaned up properly
		_mainDeletionQueue.push_function([&]() {
			std::cout << "Destroying _lightBufferLayout\n";
			vkDestroyDescriptorSetLayout(_device, _lightBufferLayout, nullptr);
			});

	}

	void VulkanRendererAPI::Shutdown()
	{
		if (_isInitialized)
		{
			//make sure the gpu has stopped doing its things
			vkDeviceWaitIdle(_device);

			_mainDeletionQueue.flush();
			for (int i = 0; i < FRAME_OVERLAP; i++) {
				vkFreeCommandBuffers(_device, _frames[i]._commandPool, 1, &_frames[i]._mainCommandBuffer);
				//already written from before
				vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
				//destroy sync objects
				vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
				vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
				//_frames[i]._deletionQueue.flush();
			}

			for (int i = 0; i < _swapchainImageCount; i++) {
				vkDestroySemaphore(_device, _submitSemaphores[i], nullptr);
			}

			DestroySwapchain();

			vkDestroySurfaceKHR(_instance, _surface, nullptr);
			vkDestroyDevice(_device, nullptr);

			vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
			vkDestroyInstance(_instance, nullptr);
			//SDL_DestroyWindow(_window);
		}
	}

	void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
	}

	void VulkanRendererAPI::SetClearColor(const glm::vec4& color)
	{
	}

	void VulkanRendererAPI::Clear()
	{
	}

	void VulkanRendererAPI::OnWindowResize(int width, int height)
	{
		_resizeRequested = true;
	}

	void VulkanRendererAPI::DrawBackground(VkCommandBuffer cmd)
	{
		// bind the gradient drawing compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipeline);

		// bind the descriptor set containing the draw image for the compute pipeline
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _gradientPipelineLayout, 0, 1, &_drawImageDescriptors, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
	}

	void VulkanRendererAPI::DrawGeometry(VkCommandBuffer cmd)
	{
		//begin a render pass  connected to our draw image
		VkRenderingAttachmentInfo colorAttachment = VkInit::AttachmentInfo(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttachment = VkInit::DepthAttachmentInfo(_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = VkInit::RenderingInfo(_drawExtent, &colorAttachment, &depthAttachment);
		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipeline);

		//set dynamic viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = _drawExtent.width;
		viewport.height = _drawExtent.height;
		viewport.minDepth = 1.f;
		viewport.maxDepth = 0.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = _drawExtent.width;
		scissor.extent.height = _drawExtent.height;

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 0, 1, &_saBufferDescriptorSet, 0, nullptr);
		//vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _meshPipelineLayout, 2, 1, &_lightBufferDescriptorSet, 0, nullptr);
	}

	AllocatedBuffer VulkanRendererAPI::CreateBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, const std::string& debugName)
	{
		// allocate buffer
		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;
		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = memoryUsage;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		AllocatedBuffer newBuffer;

		// allocate the buffer
		VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation,
			&newBuffer.info));

		return newBuffer;
	}

	void VulkanRendererAPI::DestroyBuffer(const AllocatedBuffer& buffer)
	{
		vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
	}

	AllocatedImage VulkanRendererAPI::CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
	{
		//format = VK_FORMAT_R8G8B8A8_SRGB;

		AllocatedImage newImage;
		newImage.imageFormat = format;
		newImage.imageExtent = size;

		VkImageCreateInfo img_info = VkInit::ImageCreateInfo(format, usage, size);
		if (mipmapped) {
			img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
		}

		// always allocate images on dedicated GPU memory
		VmaAllocationCreateInfo allocinfo = {};
		allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// allocate and create the image
		VK_CHECK(vmaCreateImage(_allocator, &img_info, &allocinfo, &newImage.image, &newImage.allocation, nullptr));

		// if the format is a depth format, we will need to have it use the correct
		// aspect flag
		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if (format == VK_FORMAT_D32_SFLOAT) {
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		// build a image-view for the image
		VkImageViewCreateInfo view_info = VkInit::ImageviewCreateInfo(format, newImage.image, aspectFlag);
		view_info.subresourceRange.levelCount = img_info.mipLevels;

		VK_CHECK(vkCreateImageView(_device, &view_info, nullptr, &newImage.imageView));

		return newImage;
	}

	AllocatedImage VulkanRendererAPI::CreateImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
	{
		size_t data_size = size.depth * size.width * size.height * 4;
		AllocatedBuffer uploadbuffer = CreateBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		memcpy(uploadbuffer.info.pMappedData, data, data_size);

		AllocatedImage new_image = CreateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

		ImmediateSubmit([&](VkCommandBuffer cmd) {
			VkUtil::TransitionImage(cmd, new_image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = size;

			// copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				&copyRegion);

			VkUtil::TransitionImage(cmd, new_image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

		DestroyBuffer(uploadbuffer);

		return new_image;
	}

	void VulkanRendererAPI::DestroyImage(const AllocatedImage& img)
	{
		vkDestroyImageView(_device, img.imageView, nullptr);
		vmaDestroyImage(_allocator, img.image, img.allocation);
	}

	void VulkanRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
	{
	}

	GPUMeshBuffers VulkanRendererAPI::UploadMesh(std::span<uint32_t> indices, std::span<VulkanVertex> vertices)
	{
		const size_t vertexBufferSize = vertices.size() * sizeof(VulkanVertex);
		const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		GPUMeshBuffers newSurface;

		//create vertex buffer
		newSurface.vertexBuffer = CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY, "vertex buffer");

		//find the adress of the vertex buffer
		VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
		newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_device, &deviceAdressInfo);

		//create index buffer
		newSurface.indexBuffer = CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY, "index buffer");

		AllocatedBuffer staging = CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, "staging buffer");

		void* data = staging.allocation->GetMappedData();

		// copy vertex buffer
		memcpy(data, vertices.data(), vertexBufferSize);
		// copy index buffer
		memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

		ImmediateSubmit([&](VkCommandBuffer cmd) {
			VkBufferCopy vertexCopy{ 0 };
			vertexCopy.dstOffset = 0;
			vertexCopy.srcOffset = 0;
			vertexCopy.size = vertexBufferSize;

			vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

			VkBufferCopy indexCopy{ 0 };
			indexCopy.dstOffset = 0;
			indexCopy.srcOffset = vertexBufferSize;
			indexCopy.size = indexBufferSize;

			vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
			});

		DestroyBuffer(staging);

		return newSurface;
	}

	void VulkanRendererAPI::BeginFrame()
	{

		//request image from the swapchain
		VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, GetCurrentFrame()._swapchainSemaphore, nullptr, &_swapchainImageIndex);
		if (e == VK_ERROR_OUT_OF_DATE_KHR) {
			_resizeRequested = true;
			return;
		}

		//naming it cmd for shorter writing
		VkCommandBuffer cmd = GetCurrentFrame()._mainCommandBuffer;

		// now that we are sure that the commands finished executing, we can safely
		// reset the command buffer to begin recording again.
		VK_CHECK(vkResetCommandBuffer(cmd, 0));

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = VkInit::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		//_drawExtent.width = std::min(_swapchainExtent.width, _drawImage.imageExtent.width);
		//_drawExtent.height = std::min(_swapchainExtent.height, _drawImage.imageExtent.height);

		//start the command buffer recording
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// transition our main draw image into general layout so we can write into it
		// we will overwrite it all so we dont care about what was the older layout
		VkUtil::TransitionImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		DrawBackground(cmd);

		VkUtil::TransitionImage(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkUtil::TransitionImage(cmd, _depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		DrawGeometry(cmd);

		// here will be mesh rendering and VulkanBetween
	}

	void VulkanRendererAPI::EndFrame(bool vSyncEnabled)
	{
		VkCommandBuffer cmd = GetCurrentFrame()._mainCommandBuffer;

		if (!_editorDisabled)
		{
			vkCmdEndRendering(cmd);
			// set swapchain image layout to Present so we can show it on the screen
			VkUtil::TransitionImage(cmd, _swapchainImages[_swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}
		else
		{
			VkUtil::TransitionImage(cmd, _swapchainImages[_swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		VK_CHECK(vkEndCommandBuffer(cmd));

		//prepare the submission to the queue. 
		//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
		//we will signal the _renderSemaphore, to signal that rendering has finished

		VkCommandBufferSubmitInfo cmdinfo = VkInit::CommandBufferSubmitInfo(cmd);

		VkSemaphoreSubmitInfo waitInfo = VkInit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, GetCurrentFrame()._swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = VkInit::SemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, _submitSemaphores[_swapchainImageIndex]);

		VkSubmitInfo2 submit = VkInit::SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

		//submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, GetCurrentFrame()._renderFence));

		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &_swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &_submitSemaphores[_swapchainImageIndex];
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &_swapchainImageIndex;

		VkResult presentResult = vkQueuePresentKHR(_graphicsQueue, &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
			_resizeRequested = true;
		}

		//increase the number of frames drawn
		_frameNumber++;
	}
}


