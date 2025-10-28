#include "ovpch.h"
#include "ScriptGlue.h"

#include "mono/metadata/object.h"

#include <string>
#include <iostream>

#include <glm/glm.hpp>
#include <Oeuvre/Core/Log.h>

#include <Oeuvre/Scripting/ScriptEngine.h>

#include <Oeuvre/Core/KeyCodes.h>
#include <Oeuvre/Core/Input.h>
#include <mono/metadata/reflection.h>

#include <Oeuvre/Scene/Scene.h>

namespace Oeuvre
{
	static std::unordered_map<MonoType*, std::function<bool(Entity)>> s_EntityHasComponentFuncs;

#define OV_ADD_INTERNAL_CALL(Name) mono_add_internal_call("Oeuvre.InternalCalls::" #Name, Name);

	static void NativeLog(MonoString* string, int value)
	{
		char* cStr = mono_string_to_utf8(string);
		std::string str(cStr);
		mono_free(cStr);
		std::cout << str << ", " << value << "\n";
	}

	static void NativeLog_Vector(glm::vec3* parameter, glm::vec3* outResult)
	{
		OV_CORE_WARN("Value: ({}, {}, {})", parameter->x, parameter->y, parameter->z);
		*outResult = glm::normalize(*parameter);
	}

	static float NativeLog_VectorDot(glm::vec3* parameter)
	{
		OV_CORE_WARN("Value: ({}, {}, {})", parameter->x, parameter->y, parameter->z);
		return glm::dot(*parameter, *parameter);
	}

	static bool Entity_HasComponent(uuid::UUID entityId, MonoReflectionType* componentType)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		assert(scene);
		Entity entity = scene->TryGetEntityByUUID(entityId);
		assert(entity);

		MonoType* managedType = mono_reflection_type_get_type(componentType);
		assert(s_EntityHasComponentFuncs.find(managedType) == s_EntityHasComponentFuncs.end());
		return s_EntityHasComponentFuncs.at(managedType)(entity);
	}

	static void TransformComponent_GetTranslation(uuid::UUID entityId, glm::vec3* outTranslation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();
		Entity entity = scene->TryGetEntityByUUID(entityId);
		auto& tc = entity.GetComponent<TransformComponent>();

		*outTranslation = tc.Translation;
	}

	static void TransformComponent_SetTranslation(uuid::UUID entityId, glm::vec3* translation)
	{
		Scene* scene = ScriptEngine::GetSceneContext();

		Entity entity = scene->TryGetEntityByUUID(entityId);
		auto& tc = entity.GetComponent<TransformComponent>();

		tc.Translation = *translation;
	}

	static bool Input_IsKeyDown(KeyCode keyCode)
	{
		return Input::IsKeyPressed(keyCode);
	}

	template<typename Component>
	static void RegisterComponent()
	{
		std::string typeName = typeid(Component).name();
		size_t pos = typeName.find_last_of(':');		
		std::string managedTypename = std::string("Oeuvre.") + typeName.substr(pos + 1);

		MonoType* managedType = mono_reflection_type_from_name(managedTypename.data(), ScriptEngine::GetCoreAssemblyImage());
		s_EntityHasComponentFuncs[managedType] = [](Entity entity) { return entity.HasComponent<Component>(); };
	}

	void ScriptGlue::RegisterComponents()
	{
		RegisterComponent<TransformComponent>();
	}

	void ScriptGlue::RegisterFunctions()
	{
		OV_ADD_INTERNAL_CALL(NativeLog);
		OV_ADD_INTERNAL_CALL(NativeLog_Vector);
		OV_ADD_INTERNAL_CALL(NativeLog_VectorDot);

		OV_ADD_INTERNAL_CALL(Entity_HasComponent);
		OV_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		OV_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
		OV_ADD_INTERNAL_CALL(Input_IsKeyDown);

	}
}