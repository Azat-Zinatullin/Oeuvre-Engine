#pragma once

#include <vector>
#include <string>
#include "Mesh.h"
#include "Oeuvre/Renderer/Frustum.h"

#include "Platform/Nvidia/GFSDK_VXGI_MathTypes.h"

#include<unordered_set>
#include <map>

#include <d3d11.h>
#include "AnimData.h"

struct aiNode;
struct aiMesh;
struct aiScene;

namespace Oeuvre
{
	class Model
	{
		std::string m_filePath;
		std::vector<Mesh> m_meshes;
		AABB m_bounds;
		std::vector<AABB> m_meshBounds;

		std::vector<std::shared_ptr<Texture>> m_textures;

		std::shared_ptr<Texture2D> m_pTextureAlbedo = nullptr;
		std::shared_ptr<Texture2D> m_pTextureMetalness = nullptr;
		std::shared_ptr<Texture2D> m_pTextureRoughness = nullptr;
		std::shared_ptr<Texture2D> m_pTextureNormal = nullptr;

		std::map<std::string, BoneInfo> m_BoneInfoMap;
		int m_BoneCounter = 0;

		bool m_bUseCombinedTextures = true;

		void processNode(aiNode* node, const aiScene* scene);
		Mesh processMesh(aiMesh* mesh, const aiScene* scene);

		void SetVertexBoneDataToDefault(Vertex& vertex);
		void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
		void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);


	public:
		Model(const std::string& filePath, const std::string& albedoTexPath, const std::string& normalTexPath, const std::string& roughnessTexPath, const std::string& metallicTexPath);
		Model(const Model& other) = delete;
		~Model();

		int Draw(const VXGI::Box3f* clippingBoxes, uint32_t numBoxes, const glm::mat4 modelMatrix, const Frustum* frustum);

		void ChangeTexture(const std::string& newTexPath, TextureType type);

		std::vector<Mesh>& GetMeshes() { return m_meshes; }

		bool& GetUseCombinedTextures() { return m_bUseCombinedTextures; }
		std::string GetFilePath() { return m_filePath; }

		std::vector<std::shared_ptr<Texture>> GetTextures() { return m_textures; }

		AABB GetBounds() { return m_bounds; }
		std::vector<AABB> GetMeshBounds() { return m_meshBounds; }

		auto& GetBoneInfoMap() { return m_BoneInfoMap; }
		int& GetBoneCount() { return m_BoneCounter; }
	};
}