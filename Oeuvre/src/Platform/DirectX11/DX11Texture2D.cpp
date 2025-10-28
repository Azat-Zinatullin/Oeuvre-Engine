#include "ovpch.h"
#include "DX11Texture2D.h"

#include "Platform/DirectX11/DX11RendererAPI.h"
#include "D:\Development\Projects\C_C++\Oeuvre-Engine\packages\directxtk_desktop_win10.2024.10.29.1\include\DDSTextureLoader.h"
#include "D:\Development\Projects\C_C++\Oeuvre-Engine\packages\directxtk_desktop_win10.2024.10.29.1\include\WICTextureLoader.h"

namespace Oeuvre
{
    namespace Utils {

        static DXGI_FORMAT OeuvreImageFormatToDX11ImageFormat(TextureFormat format)
        {
            switch (format)
            {
            case TextureFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
            }

            assert(false);
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    DX11Texture2D::~DX11Texture2D()
    {
        if (m_IsLoaded)
        {
            if (m_ShaderResourceView) m_ShaderResourceView->Release();
            if (m_Texture2D) m_Texture2D->Release();
            //std::cout << "Texture2D destructor worked\n";
        }
    }

    DX11Texture2D::DX11Texture2D(const TextureDesc& specification)
    {
        m_Width = specification.Width;
        m_Height = specification.Height;
        m_DataFormat = Utils::OeuvreImageFormatToDX11ImageFormat(specification.Format);

        D3D11_TEXTURE2D_DESC textureDesc;
        D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

        ZeroMemory(&textureDesc, sizeof(textureDesc));

        textureDesc.Width = m_Width;
        textureDesc.Height = m_Height;
        textureDesc.MipLevels = specification.GenerateMips ? 5 : 1;
        textureDesc.ArraySize = 1;
        textureDesc.Format = m_DataFormat; // DXGI_FORMAT_R16G16B16A16_FLOAT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        textureDesc.CPUAccessFlags = 0;
        textureDesc.MiscFlags = 0;

        auto device = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDevice();

        HRESULT result = device->CreateTexture2D(&textureDesc, nullptr, &m_Texture2D);
        if (FAILED(result))
        {
            OV_CORE_ERROR("Can't create texture!");
            return;
        }
  
        shaderResourceViewDesc.Format = textureDesc.Format;
        shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
        shaderResourceViewDesc.Texture2D.MipLevels = 1;

        result = device->CreateShaderResourceView(m_Texture2D, &shaderResourceViewDesc, &m_ShaderResourceView);
        if (FAILED(result)) OV_CORE_ERROR("Can't create shader resource view for texture!");
    }

    DX11Texture2D::DX11Texture2D(const std::string& path) :
        m_Path(path)
    {
        Load();
    }

    bool DX11Texture2D::Load()
    {
        std::wstring wpath(m_Path.begin(), m_Path.end());
        HRESULT hr;

        auto device = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDevice();
        auto deviceContext = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDeviceContext();

        if (m_Path.length() && m_Path.substr(m_Path.length() - 4, 4) == ".dds")
            hr = DirectX::CreateDDSTextureFromFile(device, deviceContext, wpath.c_str(), (ID3D11Resource**)&m_Texture2D, &m_ShaderResourceView);
        else
            hr = DirectX::CreateWICTextureFromFile(device, deviceContext, wpath.c_str(), (ID3D11Resource**)&m_Texture2D, &m_ShaderResourceView);

       
        if (FAILED(hr))
        {
            //OV_CORE_ERROR("Can't load texture from path: {}", m_Path.c_str());
            return false;
        }
        else
        {
            //OV_CORE_INFO("Loaded texture from path: {}", m_Path.c_str());
            //D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            //m_ShaderResourceView->GetDesc(&desc);
            //std::cout << "Dx11 texture loaded. Path: " << m_Path << ", Format: " << desc.Format << ".\n";
            //auto deviceContext = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDeviceContext();
            //deviceContext->GenerateMips(m_ShaderResourceView);
        }
           

        m_IsLoaded = true;
        return true;
    }

    const TextureDesc& DX11Texture2D::GetDesc() const
    {
        return m_Specification;
    }

    uint32_t DX11Texture2D::GetWidth() const
    {
        return m_Width;
    }

    uint32_t DX11Texture2D::GetHeight() const
    {
        return m_Height;
    }

    const std::string& DX11Texture2D::GetPath() const
    {
        return m_Path;
    }

    void DX11Texture2D::SetData(void* data, uint32_t size)
    {
    }

    void DX11Texture2D::Bind(uint32_t slot)
    {
        auto deviceContext = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDeviceContext();
        deviceContext->PSSetShaderResources(slot, 1, &m_ShaderResourceView);
    }

    bool DX11Texture2D::IsLoaded() const
    {
        return m_IsLoaded;
    }

    bool DX11Texture2D::operator==(const ITexture& other) const
    {
        return m_RendererID == other.GetRendererID();
    }

    uint32_t DX11Texture2D::GetRendererID() const
    {
        return m_RendererID;
    }
}


