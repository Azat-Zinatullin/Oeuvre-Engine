#include "ovpch.h"
#include "VulkanTexture2D.h"
#include <vulkan/vulkan_core.h>
#include "VulkanRendererAPI.h"
#include <stb_image.h>

namespace Oeuvre
{
    VulkanTexture2D::~VulkanTexture2D()
    {
        // if (m_IsLoaded)
        //{
        //    auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();
        //    vkApi->DestroyImage(m_Image);
        //}   
    }

    VulkanTexture2D::VulkanTexture2D(const TextureDesc& specification)
    {
        auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

        m_Image = vkApi->CreateImage(VkExtent3D{ specification.Width, specification.Height, 1 }, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
    }

    VulkanTexture2D::VulkanTexture2D(const std::string& path)
        : m_Path(path)
    {
        Load();
    }

    bool VulkanTexture2D::Load()
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(0);
        stbi_uc* data = nullptr;
        {
            data = stbi_load(m_Path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        }

        if (data)
        {
            m_IsLoaded = true;

            m_Width = width;
            m_Height = height;

            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            //if (channels == 4)
            //{
            //    format = VK_FORMAT_R8G8B8A8_UNORM;
            //}
            //else if (channels == 3)
            //{
            //    format = VK_FORMAT_R8G8B8A8_UNORM;
            //}

            auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

            m_Image = vkApi->CreateImage(data, VkExtent3D{ m_Width, m_Height, 1 }, format, VK_IMAGE_USAGE_SAMPLED_BIT);

            vkApi->_mainDeletionQueue.push_function([=]() {
                vkApi->DestroyImage(m_Image);
            });

            stbi_image_free(data);

            return true;
        }
        return false;
    }

    void VulkanTexture2D::SetData(void* data, uint32_t size)
    {
    }

    void VulkanTexture2D::Bind(uint32_t slot)
    {
    }

    const TextureDesc& VulkanTexture2D::GetDesc() const
    {
        return m_Specification;
    }

    uint32_t VulkanTexture2D::GetWidth() const
    {
        return m_Width;
    }

    uint32_t VulkanTexture2D::GetHeight() const
    {
        return m_Height;
    }

    uint32_t VulkanTexture2D::GetRendererID() const
    {
        return m_RendererID;
    }

    const std::string& VulkanTexture2D::GetPath() const
    {
        return m_Path;
    }

    bool VulkanTexture2D::IsLoaded() const
    {
        return m_IsLoaded;
    }

    bool VulkanTexture2D::operator==(const ITexture& other) const
    {
        return m_RendererID == other.GetRendererID();
    }
}
