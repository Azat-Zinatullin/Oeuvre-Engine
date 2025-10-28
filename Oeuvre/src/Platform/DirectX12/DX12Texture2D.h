#pragma once
#include "Oeuvre/Renderer/RHI/ITexture.h"
#include <d3d12.h>
#include <wrl.h>
#include "Platform/DirectX12/ExampleDescriptorHeapAllocator.h"

using namespace Microsoft::WRL;

namespace Oeuvre
{
	class DX12Texture2D : public ITexture
	{
	public:
		virtual ~DX12Texture2D();

		DX12Texture2D(const TextureDesc& specification);
		DX12Texture2D(const std::string& path);

		const TextureDesc& GetDesc() const override;
		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		uint32_t GetRendererID() const override;

		const std::string& GetPath() const override;
		void SetData(void* data, uint32_t size) override;
		void Bind(uint32_t slot) override;

		bool IsLoaded() const override;
		bool operator==(const ITexture& other) const override;

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() { return m_CPUHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() { return m_GPUHandle; }

	private:
		bool Load();
		TextureDesc m_Specification;
		uint32_t m_RendererID;
		std::string m_Path;
		bool m_IsLoaded = false;
		uint32_t m_Width, m_Height;
		
		ExampleDescriptorHeapAllocator* m_HeapAllocator = nullptr;

		ComPtr<ID3D12Resource> m_Resource;
		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;

		DXGI_FORMAT m_DataFormat;
	};
}



