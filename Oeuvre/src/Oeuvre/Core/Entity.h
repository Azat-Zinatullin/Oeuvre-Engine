#pragma once
#include <unordered_set>
#include <vector>
#include <string>

#include <DirectXMath.h>

using namespace DirectX;

class EntityComponent
{
public:
	EntityComponent(unsigned long entityID, const std::string name)
		: m_EntityID(entityID), m_Name(name)
	{
	}

	virtual void OnUpdate() = 0;

	std::string GetName() { return m_Name; }
private:
	unsigned long m_EntityID;
	std::string m_Name;
};

class TransformComponent : public EntityComponent
{
public:
	TransformComponent(unsigned long entityID, const std::string name)
		: EntityComponent(entityID, name)
	{}
	virtual void OnUpdate() override
	{
	}
private:
	XMMATRIX m_Transform = {};
};

class EntityComponentManager
{
public:
	template <typename T>
	static T* CreateComponent(unsigned long entityID, const std::string name)
	{
		auto typeName = typeid(T).name();
		std::cout << typeName << '\n';
		if (!std::strcmp(typeName, "class TransformComponent"))
		{
			m_TransformComponents.push_back(TransformComponent(entityID, name));
			return &m_TransformComponents.back();
		}
	}
private:
	static std::vector<TransformComponent> m_TransformComponents;
};

std::vector<TransformComponent> EntityComponentManager::m_TransformComponents;

class Entity
{
public:
	Entity()
	{
		m_ID = Entity::nextID++;
	}

	template<typename T>
	T* CreateComponent(std::string name)
	{
		return EntityComponentManager::CreateComponent<T>(m_ID, name);
	}
private:
	static unsigned long nextID;
	unsigned long m_ID;
};

unsigned long Entity::nextID = 0;