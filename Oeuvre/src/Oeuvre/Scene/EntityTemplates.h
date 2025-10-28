#pragma once

namespace Oeuvre
{
	template<typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		assert(!HasComponent<T>() && "Entity already has component!");
		T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		m_Scene->OnComponentAdded<T>(*this, component);
		return component;
	}

	template<typename T, typename... Args>
	T& Entity::AddOrReplaceComponent(Args&&... args)
	{
		T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
		m_Scene->OnComponentAdded<T>(*this, component);
		return component;
	}

	template<typename T>
	T& Entity::GetComponent()
	{
		assert(HasComponent<T>() && "Entity does not have component!");
		return m_Scene->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	bool Entity::HasComponent()
	{
		return m_Scene->m_Registry.any_of<T>(m_EntityHandle);
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		assert(HasComponent<T>() && "Entity does not have component!");
		m_Scene->m_Registry.remove<T>(m_EntityHandle);
	}
}

