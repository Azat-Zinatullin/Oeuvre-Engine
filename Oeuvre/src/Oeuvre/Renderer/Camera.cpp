#pragma once

#include "Camera.h"

namespace Oeuvre
{
	glm::mat4 Camera::GetViewMatrix()
	{
		glm::vec3 pos = glm::vec3(m_Pos.x, m_Pos.y, m_Pos.z);
		glm::vec3 front = glm::vec3(m_Front.x, m_Front.y, m_Front.z);
		glm::vec3 up = glm::vec3(m_Up.x, m_Up.y, m_Up.z);
		return glm::lookAtLH(pos, pos + front, up);
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

		//std::cout << "CamPos: " << m_Pos.x << ", " << m_Pos.y << ", " << m_Pos.z << '\n';
	}

	void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
	{
		xoffset *= m_MouseSensitivity;
		yoffset *= m_MouseSensitivity;

		m_Yaw += xoffset;
		m_Pitch += yoffset;

		//std::cout << "CamPitchYaw: " << m_Pitch << ", " << m_Yaw << '\n';

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
		auto rollPitchYaw = glm::vec3(glm::radians(m_Pitch), glm::radians(m_Yaw), 0.f);
		auto quat = glm::quat(rollPitchYaw);
		auto front = glm::vec3(0.f, 0.f, -1.f);
		auto frontVector = glm::normalize(quat * front);
		auto worldUp = glm::vec3(0.f, 1.f, 0.f);
		auto rightVector = glm::normalize(glm::cross(worldUp, frontVector));
		auto upVector = glm::normalize(glm::cross(frontVector, rightVector));
		m_Front = frontVector;
		m_Right = rightVector;
		m_Up = upVector;

		//glm::vec3 front;
		//front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		//front.y = sin(glm::radians(m_Pitch));
		//front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		//m_Front = front;
		//m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
		//m_Up = glm::normalize(glm::cross(m_Right, m_Front));
	}
}