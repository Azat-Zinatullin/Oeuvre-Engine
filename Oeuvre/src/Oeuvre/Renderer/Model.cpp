#include "ovpch.h"
#include "Model.h"

#include "DirectXMath.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "Platform/DirectX11/DX11RendererAPI.h"
#include <WICTextureLoader.h>

#include "Oeuvre/Utils/AssimpGLMHelpers.h"
#include "Oeuvre/Utils/FileSystem.h"

#include "Platform/DirectX12/DX12RendererAPI.h"
#include "Platform/DirectX12/DX12Texture2D.h"
#include <fstream>
#include <Platform/Vulkan/VulkanRendererAPI.h>
#include "ResourceManager.h"

#include "glad/glad.h"
#include <Platform/Vulkan/VulkanTexture2D.h>


#define BUILD_COMMON_VERTEX_ARRAY 0

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
	std::string base_name(std::string const& path)
	{
		return path.substr(path.find_last_of("/\\") + 1);
	}

	Model::Model(const std::string& filePath, const std::string& albedoTexPath, const std::string& normalTexPath, const std::string& roughnessTexPath, const std::string& metallicTexPath)
		: m_FilePath(filePath)
	{
		Assimp::Importer importer;

		unsigned int assimpFlags = aiProcessPreset_TargetRealtime_Quality;
		assimpFlags |= aiProcess_FlipUVs;

		const aiScene* scene = importer.ReadFile(filePath, assimpFlags);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ASSIMP::ERROR::Can't load model! " << importer.GetErrorString() << '\n';
			return;
		}

		const float maxFloat = 3.402823466e+38F;
		glm::vec3 _minBoundary(maxFloat, maxFloat, maxFloat);
		glm::vec3 _maxBoundary(-maxFloat, -maxFloat, -maxFloat);

		m_bounds.min = _minBoundary;
		m_bounds.max = _maxBoundary;

		//if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		//{
		//	auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
		//	dx12Api->FlushCommandQueue();
		//}

		FindTexturesDirectory();

		processNode(scene->mRootNode, scene);

#if BUILD_COMMON_VERTEX_ARRAY

		m_VertexArray = VertexArray::Create();
		auto vertexBuffer = VertexBuffer::Create((float*)m_Vertices.data(), sizeof(Vertex) * m_Vertices.size(), sizeof(Vertex));
		auto indexBuffer = IndexBuffer::Create(m_Indices.data(), m_Indices.size());
		m_VertexArray->AddVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);

#endif // BUILD_COMMON_VERTEX_ARRAY

#if 0
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

		if (scene->HasAnimations())
		{
			for (int i = 0; i < scene->mNumAnimations; i++)
			{
				printf("Animation %s:\n {", scene->mAnimations[i]->mName.C_Str());
				for (int j = 0; j < scene->mAnimations[i]->mNumChannels; j++)
				{
					auto channel = scene->mAnimations[i]->mChannels[j];

					printf("\tChannel %s\n", channel->mNodeName.C_Str());
				}
				printf("}\n");
			}
		}
#endif

		m_pTextureAlbedo = ResourceManager::GetOrCreateTexture(albedoTexPath);
		m_pTextureNormal = ResourceManager::GetOrCreateTexture(normalTexPath);
		m_pTextureRoughness = ResourceManager::GetOrCreateTexture(roughnessTexPath);
		m_pTextureMetalness = ResourceManager::GetOrCreateTexture(metallicTexPath);

		auto blackErrorTexture = ResourceManager::GetBlackTexture();
		auto whiteErrorTexture = ResourceManager::GetWhiteTexture();

		if (!m_pTextureAlbedo || !m_pTextureAlbedo->IsLoaded())
			m_pTextureAlbedo = whiteErrorTexture;
		if (!m_pTextureNormal || !m_pTextureNormal->IsLoaded())
			m_pTextureNormal = blackErrorTexture;
		if (!m_pTextureRoughness || !m_pTextureRoughness->IsLoaded())
			m_pTextureRoughness = whiteErrorTexture;
		if (!m_pTextureMetalness || !m_pTextureMetalness->IsLoaded())
			m_pTextureMetalness = whiteErrorTexture;

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();
			m_CombinedMaterialDS = vkApi->_globalDescriptorAllocator.Allocate(vkApi->_device, vkApi->_texturesLayout);

			auto albedoTex = (VulkanTexture2D*)m_pTextureAlbedo.get();
			auto normalTex = (VulkanTexture2D*)m_pTextureNormal.get();
			auto roughnessTex = (VulkanTexture2D*)m_pTextureRoughness.get();
			auto metallicTex = (VulkanTexture2D*)m_pTextureMetalness.get();

			DescriptorWriter writer;
			if (albedoTex && albedoTex->IsLoaded())
				writer.WriteImage(0, albedoTex->GetImage().imageView, vkApi->_defaultSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			if (normalTex && normalTex->IsLoaded())
				writer.WriteImage(1, normalTex->GetImage().imageView, vkApi->_defaultSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			if (roughnessTex && roughnessTex->IsLoaded())
				writer.WriteImage(2, roughnessTex->GetImage().imageView, vkApi->_defaultSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			if (metallicTex && metallicTex->IsLoaded())
				writer.WriteImage(3, metallicTex->GetImage().imageView, vkApi->_defaultSamplerLinear, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			writer.UpdateSet(vkApi->_device, m_CombinedMaterialDS);
		}
		m_Loaded = true;
	}

	Model::~Model()
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		{
			auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
			dx12Api->FlushCommandQueue();
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto vulkanApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

			vkDeviceWaitIdle(vulkanApi->_device);
			for (auto& mesh : m_meshes)
			{
				vulkanApi->DestroyBuffer(mesh.m_MeshAsset->meshBuffers.indexBuffer);
				vulkanApi->DestroyBuffer(mesh.m_MeshAsset->meshBuffers.vertexBuffer);
				vkFreeDescriptorSets(vulkanApi->_device, vulkanApi->_globalDescriptorAllocator.pool, 1, &mesh.m_MaterialDS);
			}
			vkFreeDescriptorSets(vulkanApi->_device, vulkanApi->_globalDescriptorAllocator.pool, 1, &m_CombinedMaterialDS);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			for (auto& mesh : m_meshes)
			{
				glDeleteVertexArrays(1, &mesh.VAO);
				glDeleteBuffers(1, &mesh.VBO);
				glDeleteBuffers(1, &mesh.EBO);
			}
		}
	}

	static bool OBBInFrustum(const glm::mat4& MVP, const AABB& aabb)
	{
		const glm::vec3& Min = aabb.min;
		const glm::vec3& Max = aabb.max;

		glm::vec4 obb_points[8];
		obb_points[0] = MVP * glm::vec4(Min[0], Max[1], Min[2], 1.f);
		obb_points[1] = MVP * glm::vec4(Min[0], Max[1], Max[2], 1.f);
		obb_points[2] = MVP * glm::vec4(Max[0], Max[1], Max[2], 1.f);
		obb_points[3] = MVP * glm::vec4(Max[0], Max[1], Min[2], 1.f);
		obb_points[4] = MVP * glm::vec4(Max[0], Min[1], Min[2], 1.f);
		obb_points[5] = MVP * glm::vec4(Max[0], Min[1], Max[2], 1.f);
		obb_points[6] = MVP * glm::vec4(Min[0], Min[1], Max[2], 1.f);
		obb_points[7] = MVP * glm::vec4(Min[0], Min[1], Min[2], 1.f);

		bool outside = false, outside_positive_plane, outside_negative_plane;
		for (int i = 0; i < 3; i++)
		{
			outside_positive_plane =
				obb_points[0][i] > obb_points[0].w &&
				obb_points[1][i] > obb_points[1].w &&
				obb_points[2][i] > obb_points[2].w &&
				obb_points[3][i] > obb_points[3].w &&
				obb_points[4][i] > obb_points[4].w &&
				obb_points[5][i] > obb_points[5].w &&
				obb_points[6][i] > obb_points[6].w &&
				obb_points[7][i] > obb_points[7].w;

			if (i == 2)
			{
				outside_negative_plane =
					obb_points[0][i] < 0.0f &&
					obb_points[1][i] < 0.0f &&
					obb_points[2][i] < 0.0f &&
					obb_points[3][i] < 0.0f &&
					obb_points[4][i] < 0.0f &&
					obb_points[5][i] < 0.0f &&
					obb_points[6][i] < 0.0f &&
					obb_points[7][i] < 0.0f;
			}
			else
			{
				outside_negative_plane =
					obb_points[0][i] < -obb_points[0].w &&
					obb_points[1][i] < -obb_points[1].w &&
					obb_points[2][i] < -obb_points[2].w &&
					obb_points[3][i] < -obb_points[3].w &&
					obb_points[4][i] < -obb_points[4].w &&
					obb_points[5][i] < -obb_points[5].w &&
					obb_points[6][i] < -obb_points[6].w &&
					obb_points[7][i] < -obb_points[7].w;
			}


			outside = outside || outside_positive_plane || outside_negative_plane;
		}
		return !outside;
	}

	static bool obbIntersectsSphere(const AABB& aabb, const glm::mat4& obbMatrix, const glm::vec3& sphereCenter, float sphereRadius)
	{
		glm::vec3 localSphereCenter = glm::vec3(glm::inverse(obbMatrix) * glm::vec4(sphereCenter, 1.f));
		float closestX = glm::max(aabb.min.x, glm::min(localSphereCenter.x, aabb.max.x));
		float closestY = glm::max(aabb.min.y, glm::min(localSphereCenter.y, aabb.max.y));
		float closestZ = glm::max(aabb.min.z, glm::min(localSphereCenter.z, aabb.max.z));

		glm::vec3 closestPoint = glm::vec3(obbMatrix * glm::vec4(closestX, closestY, closestZ, 1.f));

		return glm::length(closestPoint - sphereCenter) < sphereRadius;
	}

	DrawInfo Oeuvre::Model::Draw(const glm::mat4& MVP, bool frustumCulling, const glm::mat4& modelMat, bool pointLightCulling, const PointLightCullingData& pointLightCullingData)
	{
		if (m_bUseCombinedTextures)
		{
			if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
			{
				if (m_pTextureAlbedo.get() && m_pTextureAlbedo->IsLoaded())
					m_pTextureAlbedo->Bind(0);
				if (m_pTextureNormal.get() && m_pTextureNormal->IsLoaded())
					m_pTextureNormal->Bind(1);
				if (m_pTextureRoughness.get() && m_pTextureRoughness->IsLoaded())
					m_pTextureRoughness->Bind(2);
				if (m_pTextureMetalness.get() && m_pTextureMetalness->IsLoaded())
					m_pTextureMetalness->Bind(3);
			}
			else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
			{
				auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
				auto& commandList = dx12Api->GetCommandList();

				if (m_pTextureAlbedo.get() && m_pTextureAlbedo->IsLoaded())
				{
					auto gpuHandle = ((DX12Texture2D*)m_pTextureAlbedo.get())->GetGPUHandle();
					commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);
				}
				if (m_pTextureNormal.get() && m_pTextureNormal->IsLoaded())
				{
					auto gpuHandle = ((DX12Texture2D*)m_pTextureNormal.get())->GetGPUHandle();
					commandList->SetGraphicsRootDescriptorTable(3, gpuHandle);
				}
				if (m_pTextureRoughness.get() && m_pTextureRoughness->IsLoaded())
				{
					auto gpuHandle = ((DX12Texture2D*)m_pTextureRoughness.get())->GetGPUHandle();
					commandList->SetGraphicsRootDescriptorTable(4, gpuHandle);
				}
				if (m_pTextureMetalness.get() && m_pTextureMetalness->IsLoaded())
				{
					auto gpuHandle = ((DX12Texture2D*)m_pTextureMetalness.get())->GetGPUHandle();
					commandList->SetGraphicsRootDescriptorTable(5, gpuHandle);
				}
			}
			else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
			{
				if (m_pTextureAlbedo.get() && m_pTextureAlbedo->IsLoaded())
					m_pTextureAlbedo->Bind(3);
				if (m_pTextureNormal.get() && m_pTextureNormal->IsLoaded())
					m_pTextureNormal->Bind(4);
				if (m_pTextureRoughness.get() && m_pTextureRoughness->IsLoaded())
					m_pTextureRoughness->Bind(6);
				if (m_pTextureMetalness.get() && m_pTextureMetalness->IsLoaded())
					m_pTextureMetalness->Bind(5);
			}
			else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
			{
				if (m_pTextureAlbedo.get() && m_pTextureAlbedo->IsLoaded())
				{
					auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

					vkCmdBindDescriptorSets(vkApi->GetCurrentFrame()._mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkApi->_meshPipelineLayout, 1, 1, &m_CombinedMaterialDS, 0, nullptr);
				}
			}
		}


		int meshesDrawn = 0;
		int totalMeshes = 0;

		for (int i = 0; i < m_meshes.size(); i++)
		{
			const AABB& meshBounds = m_meshes[i].GetBounds();

			if (frustumCulling)
			{
				if (OBBInFrustum(MVP, meshBounds))
				{
					m_meshes[i].Draw(!m_bUseCombinedTextures, MVP, modelMat);
					meshesDrawn++;
				}
			}
			else
			{
				if (pointLightCulling)
				{
					if (obbIntersectsSphere(meshBounds, modelMat, pointLightCullingData.position, pointLightCullingData.range))
					{
						m_meshes[i].Draw(!m_bUseCombinedTextures, MVP, modelMat);
						meshesDrawn++;
					}
				}
				else
				{
					m_meshes[i].Draw(!m_bUseCombinedTextures, MVP, modelMat);
					meshesDrawn++;
				}				
			}
			totalMeshes++;
		}

		return DrawInfo{ meshesDrawn, totalMeshes };
	}

	void Model::ChangeTexture(const std::string& newTexPath, TextureType type)
	{
		switch (type)
		{
		case TextureType::ALBEDO:
			m_pTextureAlbedo.reset();
			m_pTextureAlbedo = ITexture::Create(newTexPath);
			break;
		case TextureType::NORMAL:
			m_pTextureNormal.reset();
			m_pTextureNormal = ITexture::Create(newTexPath);
			break;
		case TextureType::ROUGHNESS:
			m_pTextureRoughness.reset();
			m_pTextureRoughness = ITexture::Create(newTexPath);
			break;
		case TextureType::METALLIC:
			m_pTextureMetalness.reset();
			m_pTextureMetalness = ITexture::Create(newTexPath);
			break;
		}
	}

	void Model::processNode(aiNode* node, const aiScene* scene)
	{
		//printf("Node name: %s {\n", node->mName.C_Str());
		// process all the node's meshes (if any)

		glm::mat4 transform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation);

		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			m_meshes.push_back(processMesh(mesh, scene, transform));
		}
		// then do the same for each of its children
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
		//printf("}\n");
	}

	Mesh Oeuvre::Model::processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform)
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

			SetVertexBoneDataToDefault(vertex);

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

			//vertex.Pos = transform * glm::vec4(vertex.Pos, 1.f);
			//vertex.Normal = transform * glm::vec4(vertex.Normal, 0.f);			

			minBoundary.x = __min(minBoundary.x, vertex.Pos.x);
			minBoundary.y = __min(minBoundary.y, vertex.Pos.y);
			minBoundary.z = __min(minBoundary.z, vertex.Pos.z);

			maxBoundary.x = __max(maxBoundary.x, vertex.Pos.x);
			maxBoundary.y = __max(maxBoundary.y, vertex.Pos.y);
			maxBoundary.z = __max(maxBoundary.z, vertex.Pos.z);

			vertices.emplace_back(std::move(vertex));
		}

		meshBounds.min = glm::vec3(minBoundary.x, minBoundary.y, minBoundary.z);
		meshBounds.max = glm::vec3(maxBoundary.x, maxBoundary.y, maxBoundary.z);

		m_bounds.min.x = __min(m_bounds.min.x, minBoundary.x);
		m_bounds.min.y = __min(m_bounds.min.y, minBoundary.y);
		m_bounds.min.z = __min(m_bounds.min.z, minBoundary.z);

		m_bounds.max.x = __max(m_bounds.max.x, maxBoundary.x);
		m_bounds.max.y = __max(m_bounds.max.y, maxBoundary.y);
		m_bounds.max.z = __max(m_bounds.max.z, maxBoundary.z);

		// process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.emplace_back(face.mIndices[j]);
		}
	
		std::string materialName = "";
		std::vector<std::pair<TextureType, std::shared_ptr<ITexture>>> textures;

		RetrieveMeshTextures(scene, mesh->mMaterialIndex, materialName, textures);

		ExtractBoneWeightForVertices(vertices, mesh, scene);

#if BUILD_COMMON_VERTEX_ARRAY

		m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
		m_Indices.insert(m_Indices.end(), indices.begin(), indices.end());

#endif // BUILD_COMMON_VERTEX_ARRAY

		return Mesh(vertices, indices, materialName, textures, meshBounds);
	}

	static std::vector<std::filesystem::path> FindTexturesInDirectory(std::filesystem::path path)
	{
		std::vector<std::filesystem::path> textureFiles = FileSystem::FindFilesRecursiveByExtension(path, ".png");
		if (textureFiles.empty())
		{
			textureFiles = FileSystem::FindFilesRecursiveByExtension(path, ".jpg");
			if (textureFiles.empty())
			{
				textureFiles = FileSystem::FindFilesRecursiveByExtension(path, ".dds");
			}
		}
		return textureFiles;
	}

	void Model::FindTexturesDirectory()
	{
		std::filesystem::path modelDirectory = std::filesystem::path(m_FilePath).parent_path();

		std::string modelDirectoryStr = modelDirectory.string();
		std::string modelDirectoryStrName = "";

		size_t slashPos;
		if ((slashPos = modelDirectoryStr.find_last_of('/')) != std::string::npos)
		{
			modelDirectoryStrName = modelDirectoryStr.substr(slashPos + 1);
		}
		else if ((slashPos = modelDirectoryStr.find_last_of('\\')) != std::string::npos)
		{
			modelDirectoryStrName = modelDirectoryStr.substr(slashPos + 1);
		}

		//std::cout << "MDPSN: " << modelDirectoryStrName << "\n";

		std::vector<std::filesystem::path> textureFiles = FindTexturesInDirectory(modelDirectory);
		if (!textureFiles.empty())
		{
			m_TexturesDirectoryPath = textureFiles.at(0).parent_path().string();
			OV_CORE_INFO("Found model textures path: {}", m_TexturesDirectoryPath);
		}	
		else if (modelDirectoryStrName == "source")
		{
			textureFiles = FindTexturesInDirectory(modelDirectory.parent_path());
			if (!textureFiles.empty())
			{
				m_TexturesDirectoryPath = textureFiles.at(0).parent_path().string();
				OV_CORE_INFO("Found model textures path: {}", m_TexturesDirectoryPath);
			}
			else
			{
				OV_CORE_ERROR("Textures were not found for model: \"{}\"", m_FilePath);
			}
		}
	}

	void Model::RetrieveMeshTextures(const aiScene* scene, unsigned int materialIndex, std::string& outMaterialName, std::vector<std::pair<TextureType, std::shared_ptr<ITexture>>>& outTextures)
	{
		if (scene->HasMaterials())
		{
			aiMaterial* material = scene->mMaterials[materialIndex];

			outMaterialName = material->GetName().C_Str();

			for (int i = 0; i < SIZE_OF_ARRAY(textureTypes); i++)
			{
				if (material->GetTextureCount(textureTypes[i]) > 0)
				{
					if (textureTypes[i] != aiTextureType_DIFFUSE &&
						textureTypes[i] != aiTextureType_BASE_COLOR &&
						textureTypes[i] != aiTextureType_NORMALS &&
						textureTypes[i] != aiTextureType_DIFFUSE_ROUGHNESS &&
						textureTypes[i] != aiTextureType_METALNESS)
						continue;

					aiString path;
					material->GetTexture(textureTypes[i], 0, &path);

					std::string texPath = path.C_Str();

					std::filesystem::path texturePath = texPath;

					if (!m_TexturesDirectoryPath.empty() && texturePath.parent_path() != std::filesystem::path(m_TexturesDirectoryPath))
					{
						texPath = (std::filesystem::path(m_TexturesDirectoryPath) / texturePath.filename()).string();
					}

					//std::cout << texPath << '\n';

					auto texture = ResourceManager::GetOrCreateTexture(texPath);

					switch (textureTypes[i])
					{
					case aiTextureType_DIFFUSE:
					case aiTextureType_BASE_COLOR:
						outTextures.emplace_back(std::pair<TextureType, std::shared_ptr<ITexture>>(TextureType::ALBEDO, texture));
						break;
					case aiTextureType_NORMALS:
						outTextures.emplace_back(std::pair<TextureType, std::shared_ptr<ITexture>>(TextureType::NORMAL, texture));
						break;
					case aiTextureType_DIFFUSE_ROUGHNESS:
						outTextures.emplace_back(std::pair<TextureType, std::shared_ptr<ITexture>>(TextureType::ROUGHNESS, texture));
						break;
					case aiTextureType_METALNESS:
						outTextures.emplace_back(std::pair<TextureType, std::shared_ptr<ITexture>>(TextureType::METALLIC, texture));
						break;
					default:
						break;
					}
				}
			}
		}
	}

	void Model::SetVertexBoneDataToDefault(Vertex& vertex)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			vertex.BoneIDs[i] = -1;
			vertex.Weights[i] = 0.0f;
		}
	}

	void Model::SetVertexBoneData(Vertex& vertex, int boneID, float weight)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
		{
			if (vertex.BoneIDs[i] < 0)// || vertex.m_BoneIDs[i] > MAX_BONES)
			{
				vertex.Weights[i] = weight;
				vertex.BoneIDs[i] = boneID;
				break;
			}
		}
	}

	void Model::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
	{
		for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
			if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = m_BoneCounter;
				newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(
					mesh->mBones[boneIndex]->mOffsetMatrix);
				m_BoneInfoMap[boneName] = newBoneInfo;
				boneID = m_BoneCounter;
				m_BoneCounter++;
			}
			else
			{
				boneID = m_BoneInfoMap[boneName].id;
			}
			assert(boneID != -1);
			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				assert(vertexId <= vertices.size());
				SetVertexBoneData(vertices[vertexId], boneID, weight);
			}
		}
	}

	void Model::RecalculateMeshBounds(int meshId, const glm::mat4& transform)
	{
		const Mesh& mesh = m_meshes[meshId];

		const float maxFloat = 3.402823466e+38F;
		glm::vec3 _minBoundary(maxFloat, maxFloat, maxFloat);
		glm::vec3 _maxBoundary(-maxFloat, -maxFloat, -maxFloat);

		glm::vec3 minBoundary = _minBoundary;
		glm::vec3 maxBoundary = _maxBoundary;

		AABB meshBounds{};

		for (unsigned int i = 0; i < mesh.m_Vertices.size(); i++)
		{
			glm::vec4 newPos = transform * glm::vec4(mesh.m_Vertices[i].Pos, 1.f);

			minBoundary.x = __min(minBoundary.x, newPos.x);
			minBoundary.y = __min(minBoundary.y, newPos.y);
			minBoundary.z = __min(minBoundary.z, newPos.z);

			maxBoundary.x = __max(maxBoundary.x, newPos.x);
			maxBoundary.y = __max(maxBoundary.y, newPos.y);
			maxBoundary.z = __max(maxBoundary.z, newPos.z);
		}

		meshBounds.min = glm::vec3(minBoundary.x, minBoundary.y, minBoundary.z);
		meshBounds.max = glm::vec3(maxBoundary.x, maxBoundary.y, maxBoundary.z);

		m_meshes[meshId].m_Bounds = meshBounds;
	}
}