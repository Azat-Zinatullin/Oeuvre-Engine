#include "ovpch.h"
#include "Camera.h"
#include "RendererAPI.h"

namespace Oeuvre
{
	glm::mat4 Camera::GetViewMatrix()
	{
		if (RendererAPI::GetAPI() != RendererAPI::API::OpenGL)
			return glm::lookAtLH(m_Pos, m_Pos + m_Front, m_Up);
		else
			return glm::lookAtRH(m_Pos, m_Pos + m_Front, m_Up);
	}

	void Camera::FollowCamera(glm::vec3 pos, glm::vec3 lookAt)
	{
		auto frontVector = glm::normalize(lookAt - pos);
		auto worldUp = glm::vec3(0.f, 1.f, 0.f);
		auto rightVector = glm::normalize(glm::cross(worldUp, frontVector));
		auto upVector = glm::normalize(glm::cross(frontVector, rightVector));
		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			rightVector = glm::normalize(glm::cross(frontVector, worldUp));
			upVector = glm::normalize(glm::cross(rightVector, frontVector));
		}

		m_Pos = pos;
		m_Front = frontVector;
		m_Right = rightVector;
		m_Up = upVector;

		m_CameraWorld = glm::inverse(GetViewMatrix());
	}

	void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
	{
		float velocity = m_MovementSpeed * deltaTime;

		if (direction == CameraMovement::FORWARD)
			m_Pos += m_Front * velocity;
		if (direction == CameraMovement::BACKWARD)
			m_Pos -= m_Front * velocity;
		if (direction == CameraMovement::LEFT)
			m_Pos -= m_Right * velocity;
		if (direction == CameraMovement::RIGHT)
			m_Pos += m_Right * velocity;
	}

	void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
	{
		xoffset *= m_MouseSensitivity;
		yoffset *= m_MouseSensitivity;

		m_Yaw += xoffset;
		m_Pitch += yoffset;

		// make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (m_Pitch > 89.0f)
				m_Pitch = 89.0f;
			if (m_Pitch < -89.0f)
				m_Pitch = -89.0f;

			m_Yaw = fmodf(m_Yaw, 360.f);
		}

		// update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}

	void Camera::updateCameraVectors()
	{
		float yaw = m_Yaw;
		auto rollPitchYaw = glm::vec3(glm::radians(m_Pitch), glm::radians(yaw), 0.f);
		auto quat = glm::quat(rollPitchYaw);
		auto front = glm::vec3(0.f, 0.f, -1.f);
		auto frontVector = glm::normalize(quat * front);
		auto worldUp = glm::vec3(0.f, 1.f, 0.f);		
		auto rightVector = glm::normalize(glm::cross(worldUp, frontVector));
		auto upVector = glm::normalize(glm::cross(frontVector, rightVector));
		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			rightVector = glm::normalize(glm::cross(frontVector, worldUp));
			upVector = glm::normalize(glm::cross(rightVector, frontVector));
		}
		m_Front = frontVector;
		m_Right = rightVector;
		m_Up = upVector;

		//glm::vec3 front;
		//front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		//front.y = sin(glm::radians(m_Pitch));
		//front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		//m_Front = glm::normalize(front);
		//m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
		//m_Up = glm::normalize(glm::cross(m_Right, m_Front));

		m_CameraWorld = glm::inverse(GetViewMatrix());
	}
}