#pragma once

#include <vector>
#include <string>
#include "Mesh.h"
#include "Oeuvre/Renderer/Frustum.h"

#include<unordered_set>
#include <unordered_map>

#include <d3d11.h>
#include "AnimData.h"

#include "Oeuvre/Utils/LightCulling.h"

struct aiNode;
struct aiMesh;
struct aiScene;

namespace Oeuvre
{
	struct DrawInfo
	{
		int meshesDrawn = 0;
		int totalMeshes = 0;
	};

	class Model
	{
		bool m_Loaded = false;
		std::string m_FilePath;
		std::vector<Mesh> m_meshes;
		AABB m_bounds;

		std::vector<Vertex> m_Vertices;
		std::vector<unsigned int> m_Indices;
		
		std::shared_ptr<VertexArray> m_VertexArray;

		std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
		int m_BoneCounter = 0;

		bool m_bUseCombinedTextures = true;

		void processNode(aiNode* node, const aiScene* scene);
		Mesh processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);

		std::string m_TexturesDirectoryPath = "";
		void FindTexturesDirectory();
		void RetrieveMeshTextures(const aiScene* scene, unsigned int materialIndex, std::string& outMaterialName, std::vector<std::pair<TextureType, std::shared_ptr<ITexture>>>& outTextures);

		void SetVertexBoneDataToDefault(Vertex& vertex);
		void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
		void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);

		VkDescriptorSet m_CombinedMaterialDS;
	public:
		Model(const std::string& filePath, const std::string& albedoTexPath, const std::string& normalTexPath, const std::string& roughnessTexPath, const std::string& metallicTexPath);
		Model(const Model& other) = delete;
		~Model();

		DrawInfo Draw(const glm::mat4& MVP, bool frustumCulling, const glm::mat4& modelMat, bool pointLightCulling, const PointLightCullingData& pointLightCullingData);

		void ChangeTexture(const std::string& newTexPath, TextureType type);

		std::vector<Mesh>& GetMeshes() { return m_meshes; }

		bool& GetUseCombinedTextures() { return m_bUseCombinedTextures; }
		std::string GetFilePath() { return m_FilePath; }
		bool IsLoaded() { return m_Loaded; }

		AABB GetBounds() { return m_bounds; }

		auto& GetBoneInfoMap() { return m_BoneInfoMap; }
		int& GetBoneCount() { return m_BoneCounter; }

		void RecalculateMeshBounds(int meshId, const glm::mat4& transform);

		std::shared_ptr<ITexture> m_pTextureAlbedo = nullptr;
		std::shared_ptr<ITexture> m_pTextureMetalness = nullptr;
		std::shared_ptr<ITexture> m_pTextureRoughness = nullptr;
		std::shared_ptr<ITexture> m_pTextureNormal = nullptr;
	};
}