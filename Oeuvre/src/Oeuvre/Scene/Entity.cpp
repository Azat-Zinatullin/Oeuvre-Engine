#include "ovpch.h"
#include "Entity.h"
#include <glm/gtx/matrix_decompose.hpp>
#include "Scene.h"

namespace Oeuvre
{
	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	void Entity::AddParent(Entity parent)
	{	
		auto& parentRelComp = parent.GetComponent<RelationshipComponent>();
		auto& relationshipComponent = GetComponent<RelationshipComponent>();

		auto curr = parentRelComp.Parent;
		bool isOneOfTheChildren = false;
		while (curr != entt::null) {
			Entity ent = { curr, m_Scene };
			if (curr == *this)
			{
				isOneOfTheChildren = true;
				break;
			}
			curr = ent.GetComponent<RelationshipComponent>().Parent;
		}

		if (relationshipComponent.Parent == parent || isOneOfTheChildren)
			return;		
	
		RemoveParent();

		relationshipComponent.Parent = parent;

		// Add to the new parent
		if (parentRelComp.First == entt::null)
		{
			parentRelComp.First = *this;
		}
		else
		{
			auto curr = parentRelComp.First;
			auto last = parentRelComp.First;
			while (curr != entt::null) {
				Entity ent = { curr, m_Scene };
				curr = ent.GetComponent<RelationshipComponent>().Next;
				if (curr == *this)
					return;
				if (curr != entt::null)
					last = curr;
			}
			Entity lastChild = { last, m_Scene };
			lastChild.GetComponent<RelationshipComponent>().Next = *this;
			relationshipComponent.Prev = last;
		}

		m_Scene->ConvertToLocalSpace(*this);
	}

	void Entity::RemoveParent()
	{
		auto& relationshipComponent = GetComponent<RelationshipComponent>();

		// Remove from the last parent
		if (relationshipComponent.Parent != entt::null)
		{
			auto currParent = relationshipComponent.Parent;
			Entity currParentEnt = { currParent, m_Scene };
			auto& currParentRelComp = currParentEnt.GetComponent<RelationshipComponent>();

			auto curr = currParentRelComp.First;
			entt::entity prev = entt::null;
			while (curr != entt::null) {
				if (curr == *this)
				{
					if (curr == currParentRelComp.First)
					{
						if (relationshipComponent.Next != entt::null)
						{
							currParentRelComp.First = relationshipComponent.Next;
							Entity nextEnt = { relationshipComponent.Next, m_Scene };
							nextEnt.GetComponent<RelationshipComponent>().Prev = entt::null;
						}
						else
						{
							currParentRelComp.First = entt::null;
						}
					}
					else
					{
						if (prev != entt::null && relationshipComponent.Next != entt::null)
						{
							Entity prevEnt = { prev, m_Scene };
							prevEnt.GetComponent<RelationshipComponent>().Next = relationshipComponent.Next;
							Entity nextEnt = { relationshipComponent.Next, m_Scene };
							nextEnt.GetComponent<RelationshipComponent>().Prev = prev;
						}
						else if (prev != entt::null && relationshipComponent.Next == entt::null)
						{
							Entity prevEnt = { prev, m_Scene };
							prevEnt.GetComponent<RelationshipComponent>().Next = entt::null;
						}
					}
					break;
				}
				Entity ent = { curr, m_Scene };
				prev = curr;
				curr = ent.GetComponent<RelationshipComponent>().Next;
			}
		}
		m_Scene->ConvertToWorldSpace(*this);

		// Reset state
		relationshipComponent.Parent = entt::null;
		relationshipComponent.Prev = entt::null;
		relationshipComponent.Next = entt::null;
	}
}


