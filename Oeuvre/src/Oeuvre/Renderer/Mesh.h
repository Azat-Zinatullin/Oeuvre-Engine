#pragma once

#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>

#include "Oeuvre/Renderer/VertexArray.h"
#include "Oeuvre/Renderer/Shader.h"
#include "Oeuvre/Renderer/Texture.h"

#include "Platform/Nvidia/GFSDK_VXGI_MathTypes.h"

namespace Oeuvre
{
	struct Vertex
	{
		glm::vec3 Pos;
		glm::vec2 Tex;
		glm::vec3 Normal;
	};

	enum class TextureType
	{
		ALBEDO,
		NORMAL,
		ROUGHNESS,
		METALLIC,
		SPECULAR
	};

	struct AABB
	{
		glm::vec3 lower;
		glm::vec3 upper;
	};

	class Mesh
	{
		void Init();
	public:
		Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::string& materialName, const std::vector<std::pair<TextureType, std::shared_ptr<Texture>>>& textures, const AABB& bounds);

		 //Mesh(const Mesh& other) = delete;
		//Mesh(Mesh&& other) noexcept;
		//Mesh& operator=(Mesh&& other) noexcept;

		~Mesh();

		void Draw(const std::shared_ptr<Shader>& vertexShader, const std::shared_ptr<Shader>& pixelShader, bool applyTextures);

		size_t GetNumVertices() { return m_Vertices.size(); }
		Vertex* GetVertices() { return m_Vertices.data(); }

		size_t GetNumIndices() { return m_Indices.size(); }
		unsigned int* GetIndices() { return m_Indices.data(); }

		std::string GetMaterialName() { return m_MaterialName; }

		void ChangeTexture(const std::string& newTexPath, TextureType type);

		std::shared_ptr<Texture> GetAlbedoTexture() { return m_pTextureAlbedo; }
		std::shared_ptr<Texture> GetNormalTexture() { if (m_pTextureNormal) return m_pTextureNormal; else return nullptr; }
		std::shared_ptr<Texture> GetRoughnessTexture() { if (m_pTextureRoughness) return m_pTextureRoughness; else return nullptr; }
		std::shared_ptr<Texture> GetMetallicTexture() { if (m_pTextureMetalness) return m_pTextureMetalness; else return nullptr; }

		AABB GetBounds() { return m_Bounds; }

		std::string m_AlbedoTextureName = "";
		std::string m_NormalTextureName = "";
		std::string m_RoughnessTextureName = "";
		std::string m_MetallicTextureName = "";
	protected:
		std::vector<Vertex> m_Vertices;
		std::vector<unsigned int> m_Indices;

		std::shared_ptr<VertexArray> m_VertexArray;

		std::string m_MaterialName = "";

		std::shared_ptr<Texture> m_pTextureAlbedo = nullptr;
		std::shared_ptr<Texture> m_pTextureNormal = nullptr;
		std::shared_ptr<Texture> m_pTextureRoughness = nullptr;
		std::shared_ptr<Texture> m_pTextureMetalness = nullptr;

		AABB m_Bounds;
	};
}