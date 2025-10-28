#include "ovpch.h"
#include "DX12Texture2D.h"
#include <d3dx12.h>
#include "Platform/DirectX12/DX12RendererAPI.h"
#include <ResourceUploadBatch.h>
#include "D:/Development/Projects/C_C++/Oeuvre-Engine/packages/directxtk12_desktop_win10.2025.3.21.3/include/WICTextureLoader.h"
#include "D:/Development/Projects/C_C++/Oeuvre-Engine/packages/directxtk12_desktop_win10.2025.3.21.3/include/DDSTextureLoader.h"
#include "D:/Development/Projects/C_C++/Oeuvre-Engine/packages/directxtk12_desktop_win10.2025.3.21.3/include/DirectXHelpers.h"

using namespace DirectX;

namespace Oeuvre
{
	DX12Texture2D::~DX12Texture2D()
	{
	}

	DX12Texture2D::DX12Texture2D(const TextureDesc& specification)
	{
	}

	DX12Texture2D::DX12Texture2D(const std::string& path)
		: m_Path(path)
	{
		Load();
	}

	const TextureDesc& DX12Texture2D::GetDesc() const
	{
		return m_Specification;
	}

	uint32_t DX12Texture2D::GetWidth() const
	{
		return m_Width;
	}

	uint32_t DX12Texture2D::GetHeight() const
	{
		return m_Height;
	}

	uint32_t DX12Texture2D::GetRendererID() const
	{
		return m_RendererID;
	}

	const std::string& DX12Texture2D::GetPath() const
	{
		return m_Path;
	}

	void DX12Texture2D::SetData(void* data, uint32_t size)
	{
	}

	void DX12Texture2D::Bind(uint32_t slot)
	{
	}

	bool DX12Texture2D::IsLoaded() const
	{
		return m_IsLoaded;
	}

	bool DX12Texture2D::operator==(const ITexture& other) const
	{
		return m_RendererID == other.GetRendererID();
	}

	bool DX12Texture2D::Load()
	{
		auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		auto& device = dx12Api->GetDevice();
		auto& heapAllocator = DX12RendererAPI::g_pd3dSrvDescHeapAlloc;

		if (m_Path == "")
		{
			std::cout << "ERROR:: Can't load texture with empty path!\n";
			return false;
		}

		std::wstring path{ m_Path.begin(), m_Path.end() };

		ResourceUploadBatch resourceUpload(device.Get());

		resourceUpload.Begin();

		HRESULT hr;
		if (m_Path.substr(m_Path.length() - 4, 4) == ".dds")
		{
			hr = CreateDDSTextureFromFile(device.Get(), resourceUpload, path.c_str(),
				m_Resource.ReleaseAndGetAddressOf(), true);
		}
		else
		{
			hr = CreateWICTextureFromFile(device.Get(), resourceUpload, path.c_str(),
				m_Resource.ReleaseAndGetAddressOf(), true);
		}
		if (FAILED(hr))
		{
			std::cout << "ERROR:: Can't create WIC texture!\n";
			// Upload the resources to the GPU.
			auto uploadResourcesFinished = resourceUpload.End(dx12Api->GetCommandQueue().Get());

			// Wait for the upload thread to terminate
			uploadResourcesFinished.wait();
			return false;
		}

		// Upload the resources to the GPU.
		auto uploadResourcesFinished = resourceUpload.End(dx12Api->GetCommandQueue().Get());

		// Wait for the upload thread to terminate
		uploadResourcesFinished.wait();

		heapAllocator.Alloc(&m_CPUHandle, &m_GPUHandle);

		CreateShaderResourceView(device.Get(), m_Resource.Get(), m_CPUHandle);		

		m_IsLoaded = true;

		return true;
	}
}