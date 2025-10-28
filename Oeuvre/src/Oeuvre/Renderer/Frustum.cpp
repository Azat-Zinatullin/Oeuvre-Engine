#include "ovpch.h"
#include "Frustum.h"

namespace Oeuvre
{
	Frustum::Frustum()
	{
	}

	Frustum::~Frustum()
	{
	}

	void Frustum::ConstructFrustum(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, float screenDepth)
	{
		float zMinimum, r, t;

		// Load the projection matrix into a XMFLOAT4X4 structure.
		glm::mat4 projMatrix = projectionMatrix;

		// Calculate the minimum Z distance in the frustum.
		zMinimum = -projMatrix[3][2] / projMatrix[2][2];
		r = screenDepth / (screenDepth - zMinimum);
		projMatrix[2][2] = r;
		projMatrix[3][2] = -r * zMinimum;

		// Create the frustum matrix from the view matrix and updated projection matrix.
		const glm::mat4 &matrix = projMatrix * viewMatrix;

		// Get the near plane of the frustum.
		m_planes[0].x = matrix[0][2];
		m_planes[0].y = matrix[1][2];
		m_planes[0].z = matrix[2][2];
		m_planes[0].w = matrix[3][2];

		// Normalize it.
		t = (float)sqrt((m_planes[0].x * m_planes[0].x) + (m_planes[0].y * m_planes[0].y) + (m_planes[0].z * m_planes[0].z));
		m_planes[0].x /= t;
		m_planes[0].y /= t;
		m_planes[0].z /= t;
		m_planes[0].w /= t;

		// Calculate the far plane of frustum.
		m_planes[1].x = matrix[0][3] - matrix[0][2];
		m_planes[1].y = matrix[1][3] - matrix[1][2];
		m_planes[1].z = matrix[2][3] - matrix[2][2];
		m_planes[1].w = matrix[3][3] - matrix[3][2];

		// Normalize it.
		t = (float)sqrt((m_planes[1].x * m_planes[1].x) + (m_planes[1].y * m_planes[1].y) + (m_planes[1].z * m_planes[1].z));
		m_planes[1].x /= t;
		m_planes[1].y /= t;
		m_planes[1].z /= t;
		m_planes[1].w /= t;

		// Calculate the left plane of frustum.
		m_planes[2].x = matrix[0][3] + matrix[0][0];
		m_planes[2].y = matrix[1][3] + matrix[1][0];
		m_planes[2].z = matrix[2][3] + matrix[2][0];
		m_planes[2].w = matrix[3][3] + matrix[3][0];

		// Normalize it.
		t = (float)sqrt((m_planes[2].x * m_planes[2].x) + (m_planes[2].y * m_planes[2].y) + (m_planes[2].z * m_planes[2].z));
		m_planes[2].x /= t;
		m_planes[2].y /= t;
		m_planes[2].z /= t;
		m_planes[2].w /= t;

		// Calculate the right plane of frustum.
		m_planes[3].x = matrix[0][3] - matrix[0][0];
		m_planes[3].y = matrix[1][3] - matrix[1][0];
		m_planes[3].z = matrix[2][3] - matrix[2][0];
		m_planes[3].w = matrix[3][3] - matrix[3][0];

		// Normalize it.
		t = (float)sqrt((m_planes[3].x * m_planes[3].x) + (m_planes[3].y * m_planes[3].y) + (m_planes[3].z * m_planes[3].z));
		m_planes[3].x /= t;
		m_planes[3].y /= t;
		m_planes[3].z /= t;
		m_planes[3].w /= t;

		// Calculate the top plane of frustum.
		m_planes[4].x = matrix[0][3] - matrix[0][1];
		m_planes[4].y = matrix[1][3] - matrix[1][1];
		m_planes[4].z = matrix[2][3] - matrix[2][1];
		m_planes[4].w = matrix[3][3] - matrix[3][1];

		// Normalize it.
		t = (float)sqrt((m_planes[4].x * m_planes[4].x) + (m_planes[4].y * m_planes[4].y) + (m_planes[4].z * m_planes[4].z));
		m_planes[4].x /= t;
		m_planes[4].y /= t;
		m_planes[4].z /= t;
		m_planes[4].w /= t;

		// Calculate the bottom plane of frustum.
		m_planes[5].x = matrix[0][3] + matrix[0][1];
		m_planes[5].y = matrix[1][3] + matrix[1][1];
		m_planes[5].z = matrix[2][3] + matrix[2][1];
		m_planes[5].w = matrix[3][3] + matrix[3][1];

		// Normalize it.
		t = (float)sqrt((m_planes[5].x * m_planes[5].x) + (m_planes[5].y * m_planes[5].y) + (m_planes[5].z * m_planes[5].z));
		m_planes[5].x /= t;
		m_planes[5].y /= t;
		m_planes[5].z /= t;
		m_planes[5].w /= t;
	}

	bool Frustum::CheckAABB(glm::vec3 min, glm::vec3 max) const
	{
		bool inside = true;
		for (int i = 0; i < 6; i++)
		{
			float d = glm::max(min.x * m_planes[i].x, max.x * m_planes[i].x)
				+ glm::max(min.y * m_planes[i].y, max.y * m_planes[i].y)
				+ glm::max(min.z * m_planes[i].z, max.z * m_planes[i].z)
				+ m_planes[i].w;
			inside &= d > 0;
		}
		return inside;
	}
	bool Frustum::CheckSphere(glm::vec3 pos, float radius) const
	{
		bool res = true;
		for (int i = 0; i < 6; i++)
		{
			if (m_planes[i].x * pos.x + m_planes[i].y * pos.y +
				m_planes[i].z * pos.z + m_planes[i].w <= -radius)
				res = false;
		}
		return res;
	}
}


