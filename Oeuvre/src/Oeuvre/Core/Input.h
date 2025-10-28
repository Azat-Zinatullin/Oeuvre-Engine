#pragma once

#include <Oeuvre/Core/KeyCodes.h>
#include <Oeuvre/Core/MouseCodes.h>

#include <glm/glm.hpp>

namespace Oeuvre {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}