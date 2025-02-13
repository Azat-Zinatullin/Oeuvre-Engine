#include "Mesh.h"

#include "Oeuvre/Renderer/Buffer.h"
#include "Oeuvre/Renderer/VertexArray.h"
#include "Oeuvre/Renderer/Texture.h"
#include "Oeuvre/Renderer/RendererAPI.h"

#include <iostream>
#include <d3d11.h>

namespace Oeuvre
{
	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::string& materialName, const std::vector<std::pair<TextureType, std::shared_ptr<Texture>>>& textures, const AABB& bounds)
		: m_Vertices(vertices), m_Indices(indices), m_MaterialName(materialName), m_Bounds(bounds)
	{
		//std::cout << "Inside nesh constructor bounds lower: (" << m_Bounds.lower.x << ", " << m_Bounds.lower.y << ", " << m_Bounds.lower.z << ")\n";


		for (auto& texture : textures)
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

		Init();
	}

	//Mesh::Mesh(const Mesh& other)
	//{
	//	m_Vertices = other.m_Vertices;
	//	m_Indices = other.m_Indices;
	//	m_MaterialName = other.m_MaterialName;
	//	m_AlbedoTextureName = other.m_AlbedoTextureName;
	//	m_NormalTextureName = other.m_NormalTextureName;
	//	m_RoughnessTextureName = other.m_RoughnessTextureName;
	//	m_MetallicTextureName = other.m_MetallicTextureName;
	//	Init();
	//}

	/*Mesh::Mesh(Mesh&& other) noexcept
	{
		m_Vertices = std::move(other.m_Vertices);
		m_Indices = std::move(other.m_Indices);
		m_VertexArray = std::move(other.m_VertexArray);
		m_MaterialName = std::move(other.m_MaterialName);
		m_AlbedoTextureName = std::move(other.m_AlbedoTextureName);
		m_NormalTextureName = std::move(other.m_NormalTextureName);
		m_RoughnessTextureName = std::move(other.m_RoughnessTextureName);
		m_MetallicTextureName = std::move(other.m_MetallicTextureName);
		m_pTextureAlbedo = std::move(other.m_pTextureAlbedo);
		m_pTextureNormal = std::move(other.m_pTextureNormal);
		m_pTextureMetalness = std::move(other.m_pTextureMetalness);
		m_pTextureRoughness = std::move(other.m_pTextureRoughness);
		m_pTextureSpecular = std::move(other.m_pTextureSpecular);
		m_Bounds = std::move(other.m_Bounds);
		other.m_VertexArray.reset();
		other.m_pTextureAlbedo.reset();
		other.m_pTextureNormal.reset();
		other.m_pTextureMetalness.reset();
		other.m_pTextureRoughness.reset();
		other.m_pTextureSpecular.reset();
		other.m_VertexArray = nullptr;
		other.m_pTextureAlbedo = nullptr;
		other.m_pTextureNormal = nullptr;
		other.m_pTextureMetalness = nullptr;
		other.m_pTextureRoughness = nullptr;
		other.m_pTextureSpecular = nullptr;
	}

	Mesh& Mesh::operator=(Mesh&& other) noexcept
	{
		if (&other != this)
		{
			m_Vertices = std::move(other.m_Vertices);
			m_Indices = std::move(other.m_Indices);
			m_VertexArray = std::move(other.m_VertexArray);
			m_MaterialName = std::move(other.m_MaterialName);
			m_AlbedoTextureName = std::move(other.m_AlbedoTextureName);
			m_NormalTextureName = std::move(other.m_NormalTextureName);
			m_RoughnessTextureName = std::move(other.m_RoughnessTextureName);
			m_MetallicTextureName = std::move(other.m_MetallicTextureName);
			m_pTextureAlbedo = std::move(other.m_pTextureAlbedo);
			m_pTextureNormal = std::move(other.m_pTextureNormal);
			m_pTextureMetalness = std::move(other.m_pTextureMetalness);
			m_pTextureRoughness = std::move(other.m_pTextureRoughness);
			m_pTextureSpecular = std::move(other.m_pTextureSpecular);
			m_Bounds = std::move(other.m_Bounds);
			other.m_VertexArray.reset();
			other.m_pTextureAlbedo.reset();
			other.m_pTextureNormal.reset();
			other.m_pTextureMetalness.reset();
			other.m_pTextureRoughness.reset();
			other.m_pTextureSpecular.reset();
			other.m_VertexArray = nullptr;
			other.m_pTextureAlbedo = nullptr;
			other.m_pTextureNormal = nullptr;
			other.m_pTextureMetalness = nullptr;
			other.m_pTextureRoughness = nullptr;
			other.m_pTextureSpecular = nullptr;
		}
		return *this;
	}*/

	Mesh::~Mesh()
	{
		std::cout << "mesh destructor worked\n";
	}

	void Mesh::Draw(const std::shared_ptr<Shader>& vertexShader, const std::shared_ptr<Shader>& pixelShader, bool applyTextures)
	{
		if (vertexShader.get())
			vertexShader->Bind();
		if (pixelShader.get())
			pixelShader->Bind();
		if (applyTextures)
		{
			if (m_pTextureAlbedo.get())
				m_pTextureAlbedo->Bind(0);
			if (m_pTextureNormal.get())
				m_pTextureNormal->Bind(1);
			if (m_pTextureRoughness.get())
				m_pTextureRoughness->Bind(2);
			if (m_pTextureMetalness.get())
				m_pTextureMetalness->Bind(3);
		}
		RendererAPI::GetInstance()->DrawIndexed(m_VertexArray, m_Indices.size());
	}

	void Mesh::ChangeTexture(const std::string& newTexPath, TextureType type)
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

	void Mesh::Init()
	{
		m_VertexArray = VertexArray::Create();
		auto vertexBuffer = VertexBuffer::Create((float*)m_Vertices.data(), sizeof(Vertex) * m_Vertices.size(), sizeof(Vertex));
		auto indexBuffer = IndexBuffer::Create(m_Indices.data(), m_Indices.size());
		m_VertexArray->AddVertexBuffer(vertexBuffer);
		m_VertexArray->SetIndexBuffer(indexBuffer);		
	}
}


