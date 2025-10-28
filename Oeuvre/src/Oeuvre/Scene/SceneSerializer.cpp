#include "ovpch.h"
#include "SceneSerializer.h"
#include "cassert"

#include <yaml-cpp/yaml.h>
#include "Components.h"
#include "Entity.h"
#include <fstream>

#include "Oeuvre/Core/Log.h"

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<Oeuvre::uuid::UUID>
	{
		static Node encode(const Oeuvre::uuid::UUID& uuid)
		{
			Node node;
			node.push_back((uint64_t)uuid);
			return node;
		}

		static bool decode(const Node& node, Oeuvre::uuid::UUID& uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};

}

namespace Oeuvre
{
#define WRITE_SCRIPT_FIELD(FieldType, Type)           \
			case ScriptFieldType::FieldType:          \
				out << scriptField.GetValue<Type>();  \
				break

#define READ_SCRIPT_FIELD(FieldType, Type)             \
	case ScriptFieldType::FieldType:                   \
	{                                                  \
		Type data = scriptField["Data"].as<Type>();    \
		fieldInstance.SetValue(data);                  \
		break;                                         \
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	SceneSerializer::SceneSerializer(const std::shared_ptr<Scene>& scene)
		: m_Scene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<RelationshipComponent>())
		{
			out << YAML::Key << "RelationshipComponent";
			out << YAML::BeginMap; // RelationshipComponent

			auto& rc = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Curr" << YAML::Value << (uint32_t)rc.Curr;
			out << YAML::Key << "First" << YAML::Value << (uint32_t)rc.First;
			out << YAML::Key << "Prev" << YAML::Value << (uint32_t)rc.Prev;
			out << YAML::Key << "Next" << YAML::Value << (uint32_t)rc.Next;
			out << YAML::Key << "Parent" << YAML::Value << (uint32_t)rc.Parent;

			out << YAML::EndMap; // RelationshipComponent
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap; // MeshComponent

			auto& mc = entity.GetComponent<MeshComponent>();
			out << YAML::Key << "FilePath" << YAML::Value << mc.FilePath;
			out << YAML::Key << "AlbedoTexturePath" << YAML::Value << mc.AlbedoTexturePath;
			out << YAML::Key << "NormalTexturePath" << YAML::Value << mc.NormalTexturePath;
			out << YAML::Key << "RoughnessTexturePath" << YAML::Value << mc.RoughnessTexturePath;
			out << YAML::Key << "MetallicTexturePath" << YAML::Value << mc.MetallicTexturePath;

			out << YAML::Key << "UseNormalMap" << YAML::Value << mc.UseNormalMap;
			out << YAML::Key << "InvertNormalG" << YAML::Value << mc.InvertNormalG;
			out << YAML::Key << "Color" << YAML::Value << mc.Color;
			out << YAML::Key << "Roughness" << YAML::Value << mc.Roughness;
			out << YAML::Key << "Metallic" << YAML::Value << mc.Metallic;
			out << YAML::Key << "Emission" << YAML::Value << mc.Emission;

			out << YAML::Key << "CombinedTextures" << YAML::Value << mc.CombinedTextures;

			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cc = entity.GetComponent<CameraComponent>();
			out << YAML::Key << "MovementSpeed" << YAML::Value << cc.MovementSpeed;
			out << YAML::Key << "Primary" << YAML::Value << cc.Primary;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent

			auto& sc = entity.GetComponent<ScriptComponent>();
			out << YAML::Key << "ClassName" << YAML::Value << sc.ClassName;

			out << YAML::EndMap; // ScriptComponent
		}

		if (entity.HasComponent<PointLightComponent>())
		{
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap; // PointLightComponent

			auto& pc = entity.GetComponent<PointLightComponent>();
			out << YAML::Key << "Range" << YAML::Value << pc.Range;
			out << YAML::Key << "Intensity" << YAML::Value << pc.Intensity;
			out << YAML::Key << "Color" << YAML::Value << pc.Color;

			out << YAML::EndMap; // PointLightComponent
		}

		out << YAML::EndMap; // Entity	
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled Scene";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (const auto [ent, ref] : m_Scene->m_Registry.storage<TagComponent>().each())
		{
			Entity entity = { ent, m_Scene.get() };
			if (!entity)
				return;

			SerializeEntity(out, entity);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			OV_CORE_ERROR("Failed to load .ovscene file '{0}'\n     {1}", filepath, e.what());
			return false;
		}

		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		OV_CORE_TRACE("Deserializing scene '{0}'", sceneName);

		m_Scene->m_Registry.clear();

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				OV_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				uint32_t handle = entity["RelationshipComponent"]["Curr"].as<uint32_t>();
				Entity deserializedEntity = m_Scene->CreateEntityWithUUIDAndHandle(uuid, handle == 4294967295 ? entt::null : (entt::entity)handle, name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale = transformComponent["Scale"].as<glm::vec3>(); 
				}

				auto relationshipComponent = entity["RelationshipComponent"];
				if (relationshipComponent)
				{
					// Entities always have relationship components
					auto& rc = deserializedEntity.GetComponent<RelationshipComponent>();

					uint32_t curr = relationshipComponent["Curr"].as<uint32_t>();
					rc.Curr = curr == 4294967295 ? entt::null : (entt::entity)curr;

					uint32_t first = relationshipComponent["First"].as<uint32_t>();
					rc.First = first == 4294967295 ? entt::null : (entt::entity)first;

					uint32_t prev = relationshipComponent["Prev"].as<uint32_t>();
					rc.Prev = prev == 4294967295 ? entt::null : (entt::entity)prev;

					uint32_t next = relationshipComponent["Next"].as<uint32_t>();
					rc.Next = next == 4294967295 ? entt::null : (entt::entity)next;

					uint32_t parent = relationshipComponent["Parent"].as<uint32_t>();
					rc.Parent = parent == 4294967295 ? entt::null : (entt::entity)parent;
				}

				auto meshComponent = entity["MeshComponent"];
				if (meshComponent)
				{
					auto& mc = deserializedEntity.AddComponent<MeshComponent>();

					mc.FilePath = meshComponent["FilePath"].as<std::string>();
					mc.AlbedoTexturePath = meshComponent["AlbedoTexturePath"].as<std::string>();
					mc.NormalTexturePath = meshComponent["NormalTexturePath"].as<std::string>();
					mc.RoughnessTexturePath = meshComponent["RoughnessTexturePath"].as<std::string>();
					mc.MetallicTexturePath = meshComponent["MetallicTexturePath"].as<std::string>();

					mc.UseNormalMap = meshComponent["UseNormalMap"].as<bool>();
					mc.InvertNormalG = meshComponent["InvertNormalG"].as<bool>();
					mc.Color = meshComponent["Color"].as<glm::vec3>();
					mc.Roughness = meshComponent["Roughness"].as<float>();
					mc.Metallic = meshComponent["Metallic"].as<float>();
					mc.Emission = meshComponent["Emission"].as<float>();

					mc.CombinedTextures = meshComponent["CombinedTextures"].as<bool>();

					mc.Load();
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();

					cc.MovementSpeed = cameraComponent["MovementSpeed"].as<float>();
					cc.Primary = cameraComponent["Primary"].as<bool>();
				}

				auto scriptComponent = entity["ScriptComponent"];
				if (scriptComponent)
				{
					auto& sc = deserializedEntity.AddComponent<ScriptComponent>();

					sc.ClassName = scriptComponent["ClassName"].as<std::string>();
				}

				auto pointLightComponent = entity["PointLightComponent"];
				if (pointLightComponent)
				{
					auto& pc = deserializedEntity.AddComponent<PointLightComponent>();

					pc.Range = pointLightComponent["Range"].as<float>();
					pc.Intensity = pointLightComponent["Intensity"].as<float>();
					pc.Color = pointLightComponent["Color"].as<glm::vec3>();
				}
			}
		}

		return true;
	}
	
	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		assert(false);
		return false;
	}
}


