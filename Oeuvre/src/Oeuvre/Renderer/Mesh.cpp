#include "ovpch.h"
#include "Mesh.h"

#include "Oeuvre/Renderer/Buffer.h"
#include "Oeuvre/Renderer/RendererAPI.h"
#include "Oeuvre/Renderer/RHI/ITexture.h"
#include "Oeuvre/Renderer/VertexArray.h"

#include "Oeuvre/Utils/HelperMacros.h"
#include "Platform/DirectX12/DX12RendererAPI.h"
#include "Platform/DirectX12/DX12Texture2D.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <iostream>

#include <Platform/Vulkan/VulkanRendererAPI.h>
#include <random>

#include "glad/glad.h"
#include <Platform/Vulkan/VulkanTexture2D.h>

#include "Oeuvre/Renderer/ResourceManager.h"

namespace Oeuvre
{
	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::string& materialName, const std::vector<std::pair<TextureType, std::shared_ptr<ITexture>>>& textures, const AABB& bounds)
		: m_Vertices(vertices), m_Indices(indices), m_MaterialName(materialName), m_Bounds(bounds)
	{
		for (auto& texture : textures)
		{
			if (texture.second)
			{
				switch (texture.first)
				{
				case TextureType::ALBEDO:
					m_AlbedoTextureName = texture.second.get()->GetPath();
					m_pTextureAlbedo = texture.second;
					break;
				case TextureType::NORMAL:
					m_NormalTextureName = texture.second.get()->GetPath();
					m_pTextureNormal = texture.second;
					break;
				case TextureType::ROUGHNESS:
					m_RoughnessTextureName = texture.second.get()->GetPath();
					m_pTextureRoughness = texture.second;
					break;
				case TextureType::METALLIC:
					m_MetallicTextureName = texture.second.get()->GetPath();
					m_pTextureMetalness = texture.second;
					break;
				default:
					break;
				}
			}
		}

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

		Init();
		//std::cout << "mesh constructor worked\n";
	}

	Mesh::~Mesh()
	{
		//std::cout << "mesh destructor worked\n";
	}

	Mesh::Mesh(Mesh&& other)
	{
		m_AlbedoTextureName = std::move(other.m_AlbedoTextureName);
		m_NormalTextureName = std::move(other.m_NormalTextureName);
		m_RoughnessTextureName = std::move(other.m_RoughnessTextureName);
		m_MetallicTextureName = std::move(other.m_MetallicTextureName);

		m_Vertices = std::move(other.m_Vertices);
		m_Indices = std::move(other.m_Indices);

		m_VertexArray = std::move(other.m_VertexArray);
		m_Geometry = std::move(other.m_Geometry);
		m_MeshAsset = std::move(other.m_MeshAsset);

		m_MaterialName = std::move(other.m_MaterialName);

		m_pTextureAlbedo = std::move(other.m_pTextureAlbedo);
		m_pTextureNormal = std::move(other.m_pTextureNormal);
		m_pTextureRoughness = std::move(other.m_pTextureRoughness);
		m_pTextureMetalness = std::move(other.m_pTextureMetalness);

		m_Bounds = std::move(other.m_Bounds);
		//std::cout << "mesh move constructor worked\n";
		VAO = other.VAO;
		VBO = other.VBO;
		EBO = other.EBO;

		m_MaterialDS = std::move(other.m_MaterialDS);
	}

	Mesh& Mesh::operator=(Mesh&& other)
	{
		m_AlbedoTextureName = std::move(other.m_AlbedoTextureName);
		m_NormalTextureName = std::move(other.m_NormalTextureName);
		m_RoughnessTextureName = std::move(other.m_RoughnessTextureName);
		m_MetallicTextureName = std::move(other.m_MetallicTextureName);

		m_Vertices = std::move(other.m_Vertices);
		m_Indices = std::move(other.m_Indices);

		m_VertexArray = std::move(other.m_VertexArray);
		m_Geometry = std::move(other.m_Geometry);
		m_MeshAsset = std::move(other.m_MeshAsset);

		m_MaterialName = std::move(other.m_MaterialName);

		m_pTextureAlbedo = std::move(other.m_pTextureAlbedo);
		m_pTextureNormal = std::move(other.m_pTextureNormal);
		m_pTextureRoughness = std::move(other.m_pTextureRoughness);
		m_pTextureMetalness = std::move(other.m_pTextureMetalness);

		m_Bounds = std::move(other.m_Bounds);

		VAO = other.VAO;
		VBO = other.VBO;
		EBO = other.EBO;

		m_MaterialDS = std::move(other.m_MaterialDS);
		//std::cout << "mesh move assignment operator worked\n";

		return *this;
	}

	void Mesh::Init()
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		{
			m_VertexArray = VertexArray::Create();
			auto vertexBuffer = VertexBuffer::Create((float*)m_Vertices.data(), sizeof(Vertex) * m_Vertices.size(), sizeof(Vertex));
			auto indexBuffer = IndexBuffer::Create(m_Indices.data(), m_Indices.size());
			m_VertexArray->AddVertexBuffer(vertexBuffer);
			m_VertexArray->SetIndexBuffer(indexBuffer);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		{
			auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
			auto& device = dx12Api->GetDevice();
			auto& commandList = dx12Api->GetCommandList();

			const UINT vbByteSize = (UINT)m_Vertices.size() * sizeof(Vertex);
			const UINT ibByteSize = (UINT)m_Indices.size() * sizeof(uint32_t);

			m_Geometry = std::make_shared<MeshGeometry>();
			m_Geometry->Name = "meshGeo";

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_Geometry->VertexBufferCPU));
			CopyMemory(m_Geometry->VertexBufferCPU->GetBufferPointer(), m_Vertices.data(), vbByteSize);

			ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_Geometry->IndexBufferCPU));
			CopyMemory(m_Geometry->IndexBufferCPU->GetBufferPointer(), m_Indices.data(), ibByteSize);

			m_Geometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
				commandList.Get(), m_Vertices.data(), vbByteSize, m_Geometry->VertexBufferUploader);

			m_Geometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
				commandList.Get(), m_Indices.data(), ibByteSize, m_Geometry->IndexBufferUploader);

			m_Geometry->VertexByteStride = sizeof(Vertex);
			m_Geometry->VertexBufferByteSize = vbByteSize;
			m_Geometry->IndexFormat = DXGI_FORMAT_R32_UINT;
			m_Geometry->IndexBufferByteSize = ibByteSize;

			SubmeshGeometry submesh;
			submesh.IndexCount = (UINT)m_Indices.size();
			submesh.StartIndexLocation = 0;
			submesh.BaseVertexLocation = 0;

			m_Geometry->DrawArgs["mesh"] = submesh;
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			m_MeshAsset = std::make_shared<MeshAsset>();

			m_MeshAsset->name = "";

			std::vector<VulkanVertex> vertices;
			vertices.resize(m_Vertices.size());

			std::random_device rd;
			std::mt19937 e{ rd() };
			std::uniform_real_distribution<float> dist{ 0.f, 1.f };

			for (int i = 0; i < vertices.size(); i++)
			{
				vertices[i].position = m_Vertices[i].Pos;
				vertices[i].normal = m_Vertices[i].Normal;
				vertices[i].uv_x = m_Vertices[i].Tex.x;
				vertices[i].uv_y = m_Vertices[i].Tex.y;
				vertices[i].color = glm::vec4(dist(e), dist(e), dist(e), 1.f);
				for (int j = 0; j < MAX_BONE_INFLUENCE; j++)
				{
					vertices[i].BoneIDs[j] = m_Vertices[i].BoneIDs[j];
					vertices[i].Weights[j] = m_Vertices[i].Weights[j];
				}
			}

			auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();
			m_MeshAsset->meshBuffers = vkApi->UploadMesh(m_Indices, vertices);

			m_MaterialDS = vkApi->_globalDescriptorAllocator.Allocate(vkApi->_device, vkApi->_texturesLayout);

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

			writer.UpdateSet(vkApi->_device, m_MaterialDS);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);

			glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), m_Vertices.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), m_Indices.data(), GL_STATIC_DRAW);

			// vertex positions
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
			// vertex texture coords
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
			// vertex normals
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tex));
			// vertex bone ids
			glEnableVertexAttribArray(3);
			glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, BoneIDs));
			// vertex bone weights
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Weights));

			glBindVertexArray(0);
		}
	}

	void Mesh::Draw(bool applyTextures, const glm::mat4& mvpMat, const glm::mat4& modelMat)
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::DirectX11)
		{
			if (applyTextures)
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
			RendererAPI::GetInstance()->DrawIndexed(m_VertexArray, m_Indices.size());
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::DirectX12)
		{
			auto dx12Api = (DX12RendererAPI*)RendererAPI::GetInstance().get();
			auto& commandList = dx12Api->GetCommandList();

			auto vertexBufferView = m_Geometry->VertexBufferView();
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

			auto indexBufferView = m_Geometry->IndexBufferView();
			commandList->IASetIndexBuffer(&indexBufferView);

			commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (applyTextures)
			{
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

			commandList->DrawIndexedInstanced(m_Geometry->DrawArgs["mesh"].IndexCount, 1, 0, 0, 0);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			auto vkApi = (VulkanRendererAPI*)RendererAPI::GetInstance().get();

			VkCommandBuffer cmd = vkApi->GetCurrentFrame()._mainCommandBuffer;

			if (applyTextures)
			{
				vkCmdBindDescriptorSets(vkApi->GetCurrentFrame()._mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkApi->_meshPipelineLayout, 1, 1, &m_MaterialDS, 0, nullptr);
			}

			GPUDrawPushConstants push_constants;
			push_constants.mvpMatrix = mvpMat;
			push_constants.modelMatrix = modelMat;
			push_constants.vertexBuffer = m_MeshAsset->meshBuffers.vertexBufferAddress;

			vkCmdPushConstants(cmd, vkApi->_meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);
			vkCmdBindIndexBuffer(cmd, m_MeshAsset->meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(cmd, m_Indices.size(), 1, 0, 0, 0);
		}
		else if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			if (applyTextures)
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
			
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	void Mesh::ChangeTexture(const std::string& newTexPath, TextureType type)
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
}


