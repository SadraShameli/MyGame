#pragma once

#include "Source/Events/EventCodes/KeyCodes.h"
#include "Source/Events/EventCodes/MouseCodes.h"

#include <glm/glm.hpp>

namespace MyGame {

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
