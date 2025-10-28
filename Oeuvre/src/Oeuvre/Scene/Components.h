#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <string>

#include "Oeuvre/Renderer/Model.h"
#include "Oeuvre/Renderer/Camera.h"
#include "entt.hpp"

#include <glm/gtx/matrix_decompose.hpp>

#include "Oeuvre/Core/UUID.h"

namespace Oeuvre
{
	class Entity;

	struct IDComponent
	{
		uuid::UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
		IDComponent(uuid::UUID id) : ID(id)
		{ }
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {
		}
	};

	struct RelationshipComponent
	{
		entt::entity Curr{ entt::null };
		entt::entity First{ entt::null };
		entt::entity Prev{ entt::null };
		entt::entity Next{ entt::null };
		entt::entity Parent{ entt::null };

		RelationshipComponent() = default;
		RelationshipComponent(const RelationshipComponent&) = default;
		RelationshipComponent(entt::entity curr, entt::entity first, entt::entity prev, entt::entity next, entt::entity parent)
			: Curr(curr), First(first), Prev(prev), Next(next), Parent(parent)
		{
		}
	};

	 struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {
		}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(glm::vec3(glm::radians(Rotation.x), glm::radians(Rotation.y), glm::radians(Rotation.z))));

			return glm::translate(glm::mat4(1.f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}

		void SetTransform(const glm::mat4& transform)
		{
			glm::quat rotation;
			glm::vec3 skew;
			glm::vec4 perspective;

			glm::decompose(transform, Scale, rotation, Translation, skew, perspective);
			glm::vec3 eulerAngles = glm::eulerAngles(rotation);
			Rotation = glm::vec3(glm::degrees(eulerAngles.x), glm::degrees(eulerAngles.y), glm::degrees(eulerAngles.z));
		}
	};

	struct MeshComponent
	{
		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(const std::string& filePath, const std::string& albedoTexPath, const std::string& normalTexPath, const std::string& roughnessTexPath, const std::string& metallicTexPath, bool load)
			: FilePath(filePath), AlbedoTexturePath(albedoTexPath), NormalTexturePath(normalTexPath),
			RoughnessTexturePath(roughnessTexPath), MetallicTexturePath(metallicTexPath)
		{
			if (load)
				Load();
		}

		void Load()
		{
			m_Model.reset();
			m_Model = std::make_unique<Model>(FilePath, AlbedoTexturePath, NormalTexturePath, RoughnessTexturePath, MetallicTexturePath);
		}

		std::string FilePath = "";
		std::string AlbedoTexturePath = "";
		std::string NormalTexturePath = "";
		std::string RoughnessTexturePath = "";
		std::string MetallicTexturePath = "";
		bool UseNormalMap = true;
		bool InvertNormalG = false;
		glm::vec3 Color = glm::vec3(1.f);
		float Roughness = 1.f;
		float Metallic = 0.f;
		float Emission = 0.f;
		bool CombinedTextures = true;
		std::unique_ptr<Model> m_Model = nullptr;
	};

	struct CameraComponent
	{
		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
		CameraComponent(const Camera& camera) : m_Camera(camera)
		{
		}
		Camera m_Camera;
		float MovementSpeed = 10.f;
		bool Primary = true;
	};

	struct PointLightComponent
	{
		PointLightComponent() = default;
		PointLightComponent(const PointLightComponent&) = default;
		float Range = 50.f;
		float Intensity = 1.f;
		glm::vec3 Color = { 1.f, 1.f, 1.f };
	};

	struct ScriptComponent
	{
		std::string ClassName = "";

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;
	};

	class ScriptableEntity;

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity* (*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

}