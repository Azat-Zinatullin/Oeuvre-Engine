#pragma once

#include "DirectXMath.h"
#include <iostream>

#include "glm/glm.hpp"
#include "glm/matrix.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace DirectX;

namespace Oeuvre
{
    enum class CameraMovement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    class Camera
    {
    public:
        Camera() = default;

        glm::vec3& GetPosition() { return m_Pos; }
        float& GetMovementSpeed() { return m_MovementSpeed; }
        glm::mat4 GetViewMatrix();

        glm::vec3 GetFrontVector() { return m_Front; }

        void ProcessKeyboard(CameraMovement direction, float deltaTime);
        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    private:
        void updateCameraVectors();

        glm::vec3 m_Pos = glm::vec3(0.f, 0.f, 0.f);
        glm::vec3 m_Front = glm::vec3(0.f, 0.f, 1.f);
        glm::vec3 m_Right = glm::vec3(1.f, 0.f, 0.f);
        glm::vec3 m_Up = glm::vec3(0.f, 1.f, 0.f);
        glm::vec3 m_WorldUp = glm::vec3(0.f, 1.f, 0.f);
        float m_Yaw = 180.f, m_Pitch = 0.f;
        float m_MovementSpeed = 2.5f;
        float m_MouseSensitivity = 0.1f;
    };
}

