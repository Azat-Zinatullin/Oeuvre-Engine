#pragma once

#include "entt.hpp"
#include <unordered_map>
#include "Oeuvre/Core/UUID.h"

#include "Entity.h"

namespace Oeuvre
{
	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(uuid::UUID uuid, const std::string& name = std::string());
		Entity CreateEntityWithUUIDAndHandle(uuid::UUID uuid, entt::entity handle, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnRuntimeStart();
		void OnRuntimeStop();
		void OnRuntimeUpdate(float dt);

		void Shutdown();

		Entity TryGetEntityByTag(std::string_view name);
		Entity TryGetEntityByUUID(uuid::UUID uuid);

		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		TransformComponent GetWorldSpaceTransform(Entity entity);

		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);


		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

		entt::registry& GetRegistry() { return m_Registry; }

	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		entt::registry m_Registry;

		std::unordered_map<uuid::UUID, entt::entity> m_EntityMap;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class SceneSerializer;
	};
}

#include "EntityTemplates.h"