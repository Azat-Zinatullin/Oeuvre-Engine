#include "ovpch.h"
#include "Input.h"

#include <GLFW/glfw3.h>
#include <Platform/Windows/WindowsWindow.h>

namespace Oeuvre
{
	bool Input::IsKeyPressed(KeyCode key)
	{
		return glfwGetKey(WindowsWindow::GetInstance()->GetGLFWwindow(), key) == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		return glfwGetMouseButton(WindowsWindow::GetInstance()->GetGLFWwindow(), button) == GLFW_PRESS;
	}

	glm::vec2 Input::GetMousePosition()
	{
		double xpos, ypos;
		glfwGetCursorPos(WindowsWindow::GetInstance()->GetGLFWwindow(), &xpos, &ypos);
		return glm::vec2((float)xpos, (float)ypos);
	}

	float Input::GetMouseX()
	{
		glm::vec2 mousePos = GetMousePosition();
		return mousePos.x;
	}

	float Input::GetMouseY()
	{
		glm::vec2 mousePos = GetMousePosition();
		return mousePos.y;
	}
}


