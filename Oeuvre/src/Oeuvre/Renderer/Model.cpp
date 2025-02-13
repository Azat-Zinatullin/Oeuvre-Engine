#include "Model.h"

#include <iostream>
#include <algorithm>
#include "Oeuvre/Core/Win.h"
#include "DirectXMath.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Platform/DirectX11/DX11RendererAPI.h"
#include <WICTextureLoader.h>

aiTextureType textureTypes[]{

	/** The texture is combined with the result of the diffuse
	*  lighting equation.
	*  OR
	*  PBR Specular/Glossiness
	*/
	aiTextureType_DIFFUSE,

	/** The texture is combined with the result of the specular
	 *  lighting equation.
	 *  OR
	 *  PBR Specular/Glossiness
	 */
	aiTextureType_SPECULAR,

	/** The texture is combined with the result of the ambient
	 *  lighting equation.
	 */
	aiTextureType_AMBIENT,

	/** The texture is added to the result of the lighting
	 *  calculation. It isn't influenced by incoming light.
	 */
	aiTextureType_EMISSIVE,

	/** The texture is a height map.
	 *
	 *  By convention, higher gray-scale values stand for
	 *  higher elevations from the base height.
	 */
	aiTextureType_HEIGHT,

	/** The texture is a (tangent space) normal-map.
	 *
	 *  Again, there are several conventions for tangent-space
	 *  normal maps. Assimp does (intentionally) not
	 *  distinguish here.
	 */
	aiTextureType_NORMALS,

	/** The texture defines the glossiness of the material.
	 *
	 *  The glossiness is in fact the exponent of the specular
	 *  (phong) lighting equation. Usually there is a conversion
	 *  function defined to map the linear color values in the
	 *  texture to a suitable exponent. Have fun.
	*/
	aiTextureType_SHININESS,

	/** The texture defines per-pixel opacity.
	 *
	 *  Usually 'white' means opaque and 'black' means
	 *  'transparency'. Or quite the opposite. Have fun.
	*/
	aiTextureType_OPACITY,

	/** Displacement texture
	 *
	 *  The exact purpose and format is application-dependent.
	 *  Higher color values stand for higher vertex displacements.
	*/
	aiTextureType_DISPLACEMENT,

	/** Lightmap texture (aka Ambient Occlusion)
	 *
	 *  Both 'Lightmaps' and dedicated 'ambient occlusion maps' are
	 *  covered by this material property. The texture contains a
	 *  scaling value for the final color value of a pixel. Its
	 *  intensity is not affected by incoming light.
	*/
	aiTextureType_LIGHTMAP,

	/** Reflection texture
	 *
	 * Contains the color of a perfect mirror reflection.
	 * Rarely used, almost never for real-time applications.
	*/
	aiTextureType_REFLECTION,

	/** PBR Materials
	 * PBR definitions from maya and other modelling packages now use this standard.
	 * This was originally introduced around 2012.
	 * Support for this is in game engines like Godot, Unreal or Unity3D.
	 * Modelling packages which use this are very common now.
	 */

	aiTextureType_BASE_COLOR,
	aiTextureType_NORMAL_CAMERA,
	aiTextureType_EMISSION_COLOR,
	aiTextureType_METALNESS,
	aiTextureType_DIFFUSE_ROUGHNESS,
	aiTextureType_AMBIENT_OCCLUSION,

	/** PBR Material Modifiers
	* Some modern renderers have further PBR modifiers that may be overlaid
	* on top of the 'base' PBR materials for additional realism.
	* These use multiple texture maps, so only the base type is directly defined
	*/

	/** Sheen
	* Generally used to simulate textiles that are covered in a layer of microfibers
	* eg velvet
	* https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_sheen
	*/
	aiTextureType_SHEEN,

	/** Clearcoat
	* Simulates a layer of 'polish' or 'lacquer' layered on top of a PBR substrate
	* https://autodesk.github.io/standard-surface/#closures/coating
	* https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_materials_clearcoat
	*/
	aiTextureType_CLEARCOAT,

	/** Transmission
	* Simulates transmission through the surface
	* May include further information such as wall thickness
	*/
	aiTextureType_TRANSMISSION
};

using namespace DirectX;

namespace Oeuvre
{
	Model::Model(const std::string& filePath, const std::string& albedoTexPath, const std::string& normalTexPath, const std::string& roughnessTexPath, const std::string& metallicTexPath)
		: m_filePath(filePath)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_ImproveCacheLocality | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph); //| aiProcess_GenSmoothNormals | );
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ASSIMP::ERROR::Can't load model! " << importer.GetErrorString() << '\n';
			return;
		}

		const float maxFloat = 3.402823466e+38F;
		glm::vec3 _minBoundary(maxFloat, maxFloat, maxFloat);
		glm::vec3 _maxBoundary(-maxFloat, -maxFloat, -maxFloat);

		m_bounds.lower = _minBoundary;
		m_bounds.upper = _maxBoundary;

		processNode(scene->mRootNode, scene);

		if (scene->HasMaterials())
		{
			for (int i = 0; i < scene->mNumMaterials; i++)
			{
				aiMaterial* material = scene->mMaterials[i];
				std::cout << "Material Name: " << '\"' << material->GetName().C_Str() << "\" {" << std::endl;

				for (int j = 0; j < ARRAYSIZE(textureTypes); j++)
				{
					int numTextures = material->GetTextureCount(textureTypes[j]);
					if (numTextures > 0)
					{
						aiString path;
						material->GetTexture(textureTypes[j], 0, &path);
						std::cout << '\t' << "Map Type: \"" << textureTypes[j] << "\"" << ", Map Path: " << '\"' << path.C_Str() << '\"' << std::endl;
					}
				}
				std::cout << "};" << std::endl;
			}
		}

		m_pTextureAlbedo = Texture2D::Create(albedoTexPath.c_str());
		m_pTextureNormal = Texture2D::Create(normalTexPath.c_str());
		m_pTextureRoughness = Texture2D::Create(roughnessTexPath.c_str());
		m_pTextureMetalness = Texture2D::Create(metallicTexPath.c_str());
	}

	Model::~Model()
	{
	}

	int Model::Draw(const std::shared_ptr<Shader>& vertexShader, const std::shared_ptr<Shader>& pixelShader, const VXGI::Box3f* clippingBoxes, uint32_t numBoxes, const glm::mat4 modelMatrix, const Frustum* frustum)
	{
		if (m_pTextureAlbedo.get())
			m_pTextureAlbedo->Bind(0);
		if (m_pTextureNormal.get())
			m_pTextureNormal->Bind(1);
		if (m_pTextureRoughness.get())
			m_pTextureRoughness->Bind(2);
		if (m_pTextureMetalness.get())
			m_pTextureMetalness->Bind(3);

		int meshesDrawn = 0;
		for (int i = 0; i < m_meshes.size(); i++)
		{
			AABB meshBounds = m_meshes[i].GetBounds();

			// converting mesh bounds to world space
			auto lower = meshBounds.lower;
			auto upper = meshBounds.upper;

			glm::vec4 lowerNew = modelMatrix * glm::vec4(lower.x, lower.y, lower.z, 1.f);
			glm::vec4 upperNew = modelMatrix * glm::vec4(upper.x, upper.y, upper.z, 1.f);

			meshBounds.lower = glm::vec3(lowerNew.x, lowerNew.y, lowerNew.z);
			meshBounds.upper = glm::vec3(upperNew.x, upperNew.y, upperNew.z);

			if (clippingBoxes && numBoxes)
			{
				VXGI::Box3f bounds;
				bounds.lower = VXGI::float3(meshBounds.lower.x, meshBounds.lower.y, meshBounds.lower.z);
				bounds.upper = VXGI::float3(meshBounds.upper.x, meshBounds.upper.y, meshBounds.upper.z);

				bool contained = false;
				for (UINT clipbox = 0; clipbox < numBoxes; ++clipbox)
				{
					if (clippingBoxes[clipbox].intersectsWith(bounds))
					{
						contained = true;
						auto lower = clippingBoxes[clipbox].lower;
						auto upper = clippingBoxes[clipbox].upper;
						break;
					}
				}

				if (!contained)
					continue;
			}

			if (frustum)
			{
				if (frustum->CheckCube(meshBounds.lower, meshBounds.upper))
				{
					m_meshes[i].Draw(vertexShader, pixelShader, !m_bUseCombinedTextures);
					meshesDrawn++;
				}
			}
			else
			{
				m_meshes[i].Draw(vertexShader, pixelShader, !m_bUseCombinedTextures);
				meshesDrawn++;
			}
		}
		return meshesDrawn;
	}

	void Model::ChangeTexture(const std::string& newTexPath, TextureType type)
	{
		switch (type)
		{
		case TextureType::ALBEDO:
			m_pTextureAlbedo.reset();
			m_pTextureAlbedo = Texture2D::Create(newTexPath);
			break;
		case TextureType::NORMAL:
			m_pTextureNormal.reset();
			m_pTextureNormal = Texture2D::Create(newTexPath);
			break;
		case TextureType::ROUGHNESS:
			m_pTextureRoughness.reset();
			m_pTextureRoughness = Texture2D::Create(newTexPath);
			break;
		case TextureType::METALLIC:
			m_pTextureMetalness.reset();
			m_pTextureMetalness = Texture2D::Create(newTexPath);
			break;
		}
	}

	void Model::processNode(aiNode* node, const aiScene* scene)
	{
		// process all the node's meshes (if any)
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			//std::cout << "Mesh Material Index: " << mesh->mMaterialIndex << std::endl;
			m_meshes.emplace_back(processMesh(mesh, scene));
		}
		// then do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;

		vertices.reserve(mesh->mNumVertices);

		const float maxFloat = 3.402823466e+38F;
		glm::vec3 _minBoundary(maxFloat, maxFloat, maxFloat);
		glm::vec3 _maxBoundary(-maxFloat, -maxFloat, -maxFloat);

		glm::vec3 minBoundary = _minBoundary;
		glm::vec3 maxBoundary = _maxBoundary;

		AABB meshBounds;

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			vertex.Pos.x = mesh->mVertices[i].x;
			vertex.Pos.y = mesh->mVertices[i].y;
			vertex.Pos.z = mesh->mVertices[i].z;

			if (mesh->mTextureCoords[0])
			{
				vertex.Tex.x = mesh->mTextureCoords[0][i].x;
				vertex.Tex.y = mesh->mTextureCoords[0][i].y;
			}
			else
				vertex.Tex = glm::vec2(0.f);
			vertex.Normal.x = mesh->mNormals[i].x;
			vertex.Normal.y = mesh->mNormals[i].y;
			vertex.Normal.z = mesh->mNormals[i].z;

			vertices.emplace_back(std::move(vertex));

			minBoundary.x = __min(minBoundary.x, mesh->mVertices[i].x);
			minBoundary.y = __min(minBoundary.y, mesh->mVertices[i].y);
			minBoundary.z = __min(minBoundary.z, mesh->mVertices[i].z);

			maxBoundary.x = __max(maxBoundary.x, mesh->mVertices[i].x);
			maxBoundary.y = __max(maxBoundary.y, mesh->mVertices[i].y);
			maxBoundary.z = __max(maxBoundary.z, mesh->mVertices[i].z);
		}

		meshBounds.lower = glm::vec3(minBoundary.x, minBoundary.y, minBoundary.z);
		meshBounds.upper = glm::vec3(maxBoundary.x, maxBoundary.y, maxBoundary.z);

		m_meshBounds.emplace_back(meshBounds);

		m_bounds.lower.x = __min(m_bounds.lower.x, minBoundary.x);
		m_bounds.lower.y = __min(m_bounds.lower.y, minBoundary.y);
		m_bounds.lower.z = __min(m_bounds.lower.z, minBoundary.z);

		m_bounds.upper.x = __max(m_bounds.upper.x, maxBoundary.x);
		m_bounds.upper.y = __max(m_bounds.upper.y, maxBoundary.y);
		m_bounds.upper.z = __max(m_bounds.upper.z, maxBoundary.z);

		// process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.emplace_back(face.mIndices[j]);
		}

		std::string materialName = "";
		std::vector<std::pair<TextureType, std::shared_ptr<Texture>>> textures;
		if (scene->HasMaterials())
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			materialName = material->GetName().C_Str();

			for (int i = 0; i < ARRAYSIZE(textureTypes); i++)
			{
				if (material->GetTextureCount(textureTypes[i]) > 0)
				{
					aiString path;
					material->GetTexture(textureTypes[i], 0, &path);

					aiString texFullPath{};
					if (m_filePath == "E:\\Development\\Projects\\C_C++\\DirectX11\\resources\\sponza\\glTF\\Sponza.gltf" ||
						m_filePath == "E:\\Development\\Projects\\C_C++\\DirectX11\\resources\\bistro\\bistro.fbx" || true)
					{
						std::string filePath = m_filePath;
						int lastSlashPos = filePath.find_last_of('\\');
						filePath.erase(lastSlashPos, filePath.size() - lastSlashPos);
						std::cout << "Model path: " << filePath << '\n';
						size_t offset = filePath.find("\\source");
						if (offset != std::string::npos)
						{
							filePath = filePath.substr(0, offset) + "\\textures";
							std::cout << "New model path:" << filePath << '\n';
						}

						char fullPath[256];
						if (m_filePath == "E:\\Development\\Projects\\C_C++\\DirectX11\\resources\\bistro\\bistro.fbx")
						{
							std::string pathStr = path.C_Str();
							snprintf(fullPath, 256, "%s%s", "E:\\Development\\Projects\\C_C++\\DirectX11\\resources\\bistro\\", pathStr.c_str());
							std::cout << "Bistro path: " << fullPath << '\n';
						}
						else
						{
							std::string pathStr = path.C_Str();
							//std::replace(pathStr.begin(), pathStr.end(), '/', '\\');
							//size_t offset = pathStr.find_last_of("\\");
							//if (offset != std::string::npos)
							//{
							//	pathStr = pathStr.substr(offset + 1, pathStr.size() - offset - 1);	
							//	/*size_t offset2 = pathStr.find("jpg");
							//	if (offset2 != std::string::npos)
							//	{
							//		pathStr = pathStr.replace(offset2, 3, "jpeg");
							//	}*/
							//	std::cout << "New texture path: " << pathStr;
							//}								
							snprintf(fullPath, 256, "%s%s%s", filePath.c_str(), "\\", pathStr.c_str());
						}

						texFullPath.Set(fullPath);
					}

					auto textureIterator = std::find_if(m_textures.begin(), m_textures.end(), [&texFullPath](const std::shared_ptr<Texture>& tex) {
						return tex.get()->GetPath() == texFullPath.C_Str();
						});

					std::shared_ptr<Texture> texture;
					if (textureIterator == m_textures.end())
					{
						m_textures.emplace_back(Texture2D::Create(texFullPath.C_Str()));
						texture = m_textures.back();
					}
					else
					{
						texture = m_textures[textureIterator - m_textures.begin()];
					}

					switch (textureTypes[i])
					{
					case aiTextureType_DIFFUSE:
					case aiTextureType_BASE_COLOR:
						textures.emplace_back(std::pair<TextureType, std::shared_ptr<Texture>>(TextureType::ALBEDO, texture));
						break;
					case aiTextureType_NORMALS:
						textures.emplace_back(std::pair<TextureType, std::shared_ptr<Texture>>(TextureType::NORMAL, texture));
						break;
					case aiTextureType_DIFFUSE_ROUGHNESS:
						textures.emplace_back(std::pair<TextureType, std::shared_ptr<Texture>>(TextureType::ROUGHNESS, texture));
						break;
					case aiTextureType_METALNESS:
						textures.emplace_back(std::pair<TextureType, std::shared_ptr<Texture>>(TextureType::METALLIC, texture));
						break;
					default:
						break;
					}
				}
			}
		}
		return Mesh(vertices, indices, materialName, textures, meshBounds);
	}
}