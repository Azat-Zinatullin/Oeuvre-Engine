#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>

#include "Oeuvre/Renderer/VertexArray.h"
#include "Oeuvre/Renderer/Shader.h"
#include "Oeuvre/Renderer/RHI/ITexture.h"

#include "Platform/DirectX12/d3dUtil.h"

#include "Platform/Vulkan/VkLoader.h"
#include "AABB.h"

namespace Oeuvre
{
	constexpr int MAX_BONE_INFLUENCE = 4;
	constexpr int MAX_BONES = 100;

	struct Vertex
	{
		glm::vec3 Pos;
		glm::vec2 Tex;
		glm::vec3 Normal;
		int BoneIDs[MAX_BONE_INFLUENCE];
		float Weights[MAX_BONE_INFLUENCE];
	};

	enum class TextureType
	{
		ALBEDO,
		NORMAL,
		ROUGHNESS,
		METALLIC,
		SPECULAR
	};

	class Mesh
	{
		void Init();
	public:
		Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::string& materialName, const std::vector<std::pair<TextureType, std::shared_ptr<ITexture>>>& textures, const AABB& bounds);
		~Mesh();

		Mesh(Mesh&& other);
		Mesh& operator=(Mesh&& other);

		void Draw(bool applyTextures, const glm::mat4 & mvpMat, const glm::mat4& modelMat);

		size_t GetNumVertices() { return m_Vertices.size(); }
		size_t GetNumIndices() { return m_Indices.size(); }

		std::vector<Vertex> GetVertices() { return m_Vertices; }
		std::vector<unsigned int> GetIndices() { return m_Indices; }

		Vertex* GetVerticesData() { return m_Vertices.data(); }
		unsigned int* GetIndicesData() { return m_Indices.data(); }

		std::string GetMaterialName() { return m_MaterialName; }

		void ChangeTexture(const std::string& newTexPath, TextureType type);

		std::shared_ptr<ITexture> GetAlbedoTexture() { if (m_pTextureAlbedo) return m_pTextureAlbedo; else return nullptr; }
		std::shared_ptr<ITexture> GetNormalTexture() { if (m_pTextureNormal) return m_pTextureNormal; else return nullptr; }
		std::shared_ptr<ITexture> GetRoughnessTexture() { if (m_pTextureRoughness) return m_pTextureRoughness; else return nullptr; }
		std::shared_ptr<ITexture> GetMetallicTexture() { if (m_pTextureMetalness) return m_pTextureMetalness; else return nullptr; }

		AABB GetBounds() { return m_Bounds; }

		std::string m_AlbedoTextureName = "";
		std::string m_NormalTextureName = "";
		std::string m_RoughnessTextureName = "";
		std::string m_MetallicTextureName = "";

		std::shared_ptr<MeshAsset> m_MeshAsset = nullptr;
	protected:
		std::vector<Vertex> m_Vertices;
		std::vector<unsigned int> m_Indices;

		std::shared_ptr<VertexArray> m_VertexArray = nullptr;
		std::shared_ptr<MeshGeometry> m_Geometry = nullptr;

		std::string m_MaterialName = "";

		std::shared_ptr<ITexture> m_pTextureAlbedo = nullptr;
		std::shared_ptr<ITexture> m_pTextureNormal = nullptr;
		std::shared_ptr<ITexture> m_pTextureRoughness = nullptr;
		std::shared_ptr<ITexture> m_pTextureMetalness = nullptr;

		AABB m_Bounds;

		friend class Model;

		// OpenGL TODO: change this!
		unsigned int VAO, VBO, EBO;

		VkDescriptorSet m_MaterialDS;
	};
}