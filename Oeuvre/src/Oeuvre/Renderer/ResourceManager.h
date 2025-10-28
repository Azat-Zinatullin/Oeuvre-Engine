#pragma once
#include <vector>
#include <memory>
#include "RHI/ITexture.h"
#include "Oeuvre/Core/UUID.h"

namespace Oeuvre
{
	class Scene;

	class ResourceManager
	{
	public:
		static std::vector<std::shared_ptr<ITexture>>& GetTextures() { return m_Textures; }

		static std::shared_ptr<ITexture> GetOrCreateTexture(const std::string& path);

		static void AddPendingModelEntity(uuid::UUID uuid);
		static void LoadPendingEntityModels(Scene* scene);

		static void InitErrorTextures();
		static std::shared_ptr<ITexture> GetBlackTexture();
		static std::shared_ptr<ITexture> GetWhiteTexture();
	private:
		static std::vector<std::shared_ptr<ITexture>> m_Textures;
		static std::shared_ptr<ITexture> m_BlackTexture;
		static std::shared_ptr<ITexture> m_WhiteTexture;

		static std::vector<uuid::UUID> m_PendingModelEntities;
	};
}

