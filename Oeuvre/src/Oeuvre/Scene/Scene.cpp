#include "ovpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Components.h"

#include <glm/glm.hpp>

#include "ScriptableEntity.h"
#include <Oeuvre/Scripting/ScriptEngine.h>
#include <Platform/DirectX12/DX12RendererAPI.h>

namespace Oeuvre
{
	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = CreateEntityWithUUID(uuid::UUID(), name);
		if (name != "Scene")
		{
			Entity baseEntity = TryGetEntityByTag("Scene");
			entity.AddParent(baseEntity);
		}
		return entity;
	}

	Entity Scene::CreateEntityWithUUID(uuid::UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		entity.AddComponent<RelationshipComponent>(entity, entt::null, entt::null, entt::null, entt::null);

		m_EntityMap[uuid] = entity;

		return entity;
	}

	Entity Scene::CreateEntityWithUUIDAndHandle(uuid::UUID uuid, entt::entity handle, const std::string& name)
	{
		Entity entity = { m_Registry.create(handle), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		entity.AddComponent<RelationshipComponent>(handle, entt::null, entt::null, entt::null, entt::null);

		m_EntityMap[uuid] = entity;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		entity.RemoveParent();

		// Destroy all childs and subchilds
		auto& relComp = entity.GetComponent<RelationshipComponent>();
		auto curr = relComp.First; 
		while (curr != entt::null) {
			Entity ent = { curr, this };
			curr = ent.GetComponent<RelationshipComponent>().Next;
			DestroyEntity(ent);
		}

		m_Registry.destroy(entity);
	}

	void Scene::OnRuntimeStart()
	{
		std::cout << "Scene::OnRuntimeStart()\n";

		//Scriptig
		{
			ScriptEngine::OnRuntimeStart(this);
			// Instantiate all script entities
			auto view = m_Registry.view<ScriptComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				ScriptEngine::OnCreateEntity(entity);
			}
		}
	}

	void Scene::OnRuntimeStop()
	{
		std::cout << "Scene::OnRuntimeStop()\n";

		ScriptEngine::OnRuntimeStop();
	}

	void Scene::OnRuntimeUpdate(float dt)
	{
		// Update scripts
		{
			// C# Entity OnUpdate
			auto view = m_Registry.view<ScriptComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				ScriptEngine::OnUpdateEntity(entity, dt);
			}

			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
				{
					if (!nsc.Instance)
					{
						nsc.Instance = nsc.InstantiateScript();
						nsc.Instance->m_Entity = { entity, this };
						nsc.Instance->OnCreate();
					}

					nsc.Instance->OnUpdate(dt);
				});
		}
	}

	void Scene::Shutdown()
	{
		m_Registry.clear();
	}

	Entity Scene::TryGetEntityByTag(std::string_view name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const TagComponent& tc = view.get<TagComponent>(entity);
			if (tc.Tag == name)
				return Entity{ entity, this };
		}
		return Entity{};
	}

	Entity Scene::TryGetEntityByUUID(uuid::UUID uuid)
	{
		// TODO(Yan): Maybe should be assert
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return { m_EntityMap.at(uuid), this };

		return Entity{};
	}

	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
	{
		glm::mat4 transform(1.0f);

		Entity parent = { entity.GetComponent<RelationshipComponent>().Parent, this };
		if (parent)
			transform = GetWorldSpaceTransformMatrix(parent);

		return transform * entity.Transform().GetTransform();
	}

	TransformComponent Scene::GetWorldSpaceTransform(Entity entity)
	{
		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		TransformComponent transformComponent;
		transformComponent.SetTransform(transform);
		return transformComponent;
	}

	void Scene::ConvertToLocalSpace(Entity entity)
	{
		Entity parent = { entity.GetComponent<RelationshipComponent>().Parent, this };

		if (!parent)
			return;

		auto& transform = entity.Transform();
		glm::mat4 parentTransform = GetWorldSpaceTransformMatrix(parent);
		glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();
		transform.SetTransform(localTransform);
	}

	void Scene::ConvertToWorldSpace(Entity entity)
	{
		Entity parent = { entity.GetComponent<RelationshipComponent>().Parent, this };

		if (!parent)
			return;

		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		auto& entityTransform = entity.Transform();
		entityTransform.SetTransform(transform);
	}

	template<typename T>
	inline void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(sizeof(T) == 0);
	}

	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<RelationshipComponent>(Entity entity, RelationshipComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<MeshComponent>(Entity entity, MeshComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<PointLightComponent>(Entity entity, PointLightComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
	}
}


