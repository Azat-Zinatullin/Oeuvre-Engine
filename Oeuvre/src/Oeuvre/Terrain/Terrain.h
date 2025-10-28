#pragma once

#include <d3d11.h>
#include <glm/glm.hpp>
#include "Oeuvre/Renderer/Mesh.h"


namespace Oeuvre
{
	class Terrain
	{
	public:
		Terrain();
		Terrain(const Terrain&);
		~Terrain();

		struct HeightMapType
		{
			float x, y, z;
			float nx, ny, nz;
		};

		struct ModelType
		{
			float x, y, z;
			float tu, tv;
			float nx, ny, nz;
		};

		struct VectorType
		{
			float x, y, z;
		};

		bool Initialize(ID3D11Device*, char*);
		void Shutdown();
		bool Render(ID3D11DeviceContext*);

		int GetIndexCount();

		int m_terrainHeight, m_terrainWidth;
		HeightMapType* m_heightMap;

	private:
		bool LoadSetupFile(char*);
		bool LoadBitmapHeightMap();
		bool LoadRawHeightMap();
		void ShutdownHeightMap();
		void SetTerrainCoordinates();
		bool CalculateNormals();
		bool BuildTerrainModel();
		void ShutdownTerrainModel();

		bool InitializeBuffers(ID3D11Device*);
		void ShutdownBuffers();
		void RenderBuffers(ID3D11DeviceContext*);

	private:
		ID3D11Buffer* m_vertexBuffer, * m_indexBuffer;
		int m_vertexCount, m_indexCount;

		float m_heightScale;
		char m_terrainFilename[256];
		ModelType* m_terrainModel;
	};
}

