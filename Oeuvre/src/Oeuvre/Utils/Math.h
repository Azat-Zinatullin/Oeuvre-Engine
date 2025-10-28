#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <DirectXMath.h>

namespace Oeuvre::Math
{
	constexpr float pi = 3.1415926535897932384626433832795f;
	constexpr float epsilon = 1e-5f;

	glm::mat4 XMMatrixToGLM(DirectX::XMMATRIX xmMat)
	{
		glm::mat4 glmMat;

		glmMat[0][0] = xmMat.r[0].m128_f32[0];
		glmMat[0][1] = xmMat.r[0].m128_f32[1];
		glmMat[0][2] = xmMat.r[0].m128_f32[2];
		glmMat[0][3] = xmMat.r[0].m128_f32[3];

		glmMat[1][0] = xmMat.r[1].m128_f32[0];
		glmMat[1][1] = xmMat.r[1].m128_f32[1];
		glmMat[1][2] = xmMat.r[1].m128_f32[2];
		glmMat[1][3] = xmMat.r[1].m128_f32[3];

		glmMat[2][0] = xmMat.r[2].m128_f32[0];
		glmMat[2][1] = xmMat.r[2].m128_f32[1];
		glmMat[2][2] = xmMat.r[2].m128_f32[2];
		glmMat[2][3] = xmMat.r[2].m128_f32[3];

		glmMat[3][0] = xmMat.r[3].m128_f32[0];
		glmMat[3][1] = xmMat.r[3].m128_f32[1];
		glmMat[3][2] = xmMat.r[3].m128_f32[2];
		glmMat[3][3] = xmMat.r[3].m128_f32[3];

		return glmMat;
	}

	bool IsEqualUsingDot(float dot)
	{
		const float kEpsilon = 0.000001f;
		// Returns false in the presence of NaN values.
		return dot > 1.0f - kEpsilon;
	}

	float QuaternionAngle(const glm::quat& a, const glm::quat& b)
	{
		float dot = glm::min(glm::abs(glm::dot(a, b)), 1.0f);
		return IsEqualUsingDot(dot) ? 0.0f : glm::acos(dot) * 2.0f;
	}

	glm::quat QuaternionRotateTowards(const glm::quat& from, const glm::quat& to, float maxAngleDegrees)
	{
		float angleRadians = Math::QuaternionAngle(from, to);

		if (glm::degrees(angleRadians) == 0.f)
			return to;

		if (glm::degrees(angleRadians) <= 90.f)
		{
			maxAngleDegrees *= glm::clamp(glm::degrees(angleRadians) / 180.f);
		}

		return glm::slerp(from, to, glm::min(1.0f, maxAngleDegrees / glm::degrees(angleRadians)));
	}
}