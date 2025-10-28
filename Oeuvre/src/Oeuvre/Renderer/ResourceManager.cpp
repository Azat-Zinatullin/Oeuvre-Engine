#include "ovpch.h"
#include "ResourceManager.h"

#include "Oeuvre/Scene/Scene.h"
#include <Platform/Vulkan/VulkanRendererAPI.h>

namespace Oeuvre
{
	std::shared_ptr<ITexture> ResourceManager::GetOrCreateTexture(const std::string& path)
	{
		auto textureIterator = std::find_if(m_Textures.begin(), m_Textures.end(), [&path](const std::shared_ptr<ITexture>& tex) {
			return tex->GetPath() == path;
			});

		std::shared_ptr<ITexture> texture;
		if (textureIterator == m_Textures.end())
		{
			m_Textures.emplace_back(ITexture::Create(path));
			texture = m_Textures.back();
			if (!texture || !texture->IsLoaded())
			{
				m_Textures.pop_back();
				texture = nullptr;
			}
		}
		else
		{
			texture = m_Textures[textureIterator - m_Textures.begin()];
			OV_CORE_INFO("Texture found in pool, reusing: {}", path); 
		}

		return texture;
	}

	void ResourceManager::AddPendingModelEntity(uuid::UUID uuid)
	{
		m_PendingModelEntities.push_back(uuid);
	}

	void ResourceManager::LoadPendingEntityModels(Scene* scene)
	{
		if (m_PendingModelEntities.empty())
			return;
		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();
			vkDeviceWaitIdle(vkApi->_device);
		}		

		for (const auto& uuid : m_PendingModelEntities)
		{
			Entity entity = scene->TryGetEntityByUUID(uuid);
			if (entity && entity.HasComponent<MeshComponent>())
			{
				entity.GetComponent<MeshComponent>().Load();
			}
		}
		m_PendingModelEntities.clear();
	}

	void ResourceManager::InitErrorTextures()
	{
		m_BlackTexture = GetOrCreateTexture("../resources/textures/black.png");
		m_WhiteTexture = GetOrCreateTexture("../resources/textures/white.png");
	}

	std::shared_ptr<ITexture> ResourceManager::GetBlackTexture()
	{
		return m_BlackTexture;
	}

	std::shared_ptr<ITexture> ResourceManager::GetWhiteTexture()
	{
		return m_WhiteTexture;
	}

	std::vector<std::shared_ptr<ITexture>> ResourceManager::m_Textures = std::vector<std::shared_ptr<ITexture>>();
	std::vector<uuid::UUID> ResourceManager::m_PendingModelEntities = std::vector<uuid::UUID>();

	std::shared_ptr<ITexture> ResourceManager::m_BlackTexture;
	std::shared_ptr<ITexture> ResourceManager::m_WhiteTexture;
}


