#include "DX11Texture2D.h"

#include <cassert>
#include <iostream>

#include "Platform/DirectX11/DX11RendererAPI.h"
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>
#include <Oeuvre/Core/Log.h>

namespace Oeuvre
{
    namespace Utils {

        static DXGI_FORMAT OeuvreImageFormatToDX11DataFormat(ImageFormat format)
        {
            switch (format)
            {
            //case ImageFormat::RGB8:  return DXGI_FORMAT_R8G8B8A8_SINT;
            case ImageFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
            }

            assert(false);
            return DXGI_FORMAT_UNKNOWN;
        }
    }

    DX11Texture2D::~DX11Texture2D()
    {
        if (m_ShaderResourceView) m_ShaderResourceView->Release();
        if (m_Texture2D) m_Texture2D->Release();
        //std::cout << "Texture2D destructor worked\n";
    }

    DX11Texture2D::DX11Texture2D(const TextureSpecification& specification)
    {
        m_Width = specification.Width;
        m_Height = specification.Height;
        m_DataFormat = Utils::OeuvreImageFormatToDX11DataFormat(specification.Format);

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
        std::wstring wpath(path.begin(), path.end());
        HRESULT hr;

        auto device = static_cast<DX11RendererAPI*>(DX11RendererAPI::GetInstance().get())->GetDevice();


        if (path.length() && path.substr(path.length() - 4, 4) == ".dds")
            hr = DirectX::CreateDDSTextureFromFile(device, wpath.c_str(), (ID3D11Resource**)&m_Texture2D, &m_ShaderResourceView);
        else
            hr = DirectX::CreateWICTextureFromFile(device, wpath.c_str(), (ID3D11Resource**) &m_Texture2D, &m_ShaderResourceView);

        if (FAILED(hr))
        {
            OV_CORE_ERROR("Can't load texture from path: {}", path.c_str());
            return;
        }

       /* ID3D11Resource* res = nullptr;
        m_ShaderResourceView->GetResource(&res);
        if (res)
        {
            ID3D11Texture2D* tex = nullptr;
            res->QueryInterface<ID3D11Texture2D>(&tex);
            if (tex)
                m_Texture2D = tex;
        }      */

        //D3D11_SHADER_RESOURCE_VIEW_DESC pDesc;
        //m_ShaderResourceView->GetDesc(&pDesc);
        //std::cout << "Texture format: " << pDesc.Format << '\n';
    }

    const TextureSpecification& DX11Texture2D::GetSpecification() const
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

    bool DX11Texture2D::operator==(const Texture& other) const
    {
        return m_RendererID == other.GetRendererID();
    }

    uint32_t DX11Texture2D::GetRendererID() const
    {
        return m_RendererID;
    }
}


