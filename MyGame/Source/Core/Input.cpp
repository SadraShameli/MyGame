#include "CommonHeaders.h"

#include "Source/Core/Input.h"
#include "Source/Core/Application.h"

#include <GLFW/glfw3.h>

namespace MyGame
{
	bool Input::IsKeyPressed(const int key)
	{
		auto* window = Application::Get().GetWindow().GetNativeWindow();
		auto state = glfwGetKey(window, key);
		return state == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(const int button)
	{
		auto* window = Application::Get().GetWindow().GetNativeWindow();
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}

	glm::vec2 Input::GetMousePosition()
	{
		auto* window = Application::Get().GetWindow().GetNativeWindow();
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	float Input::GetMouseX() { return GetMousePosition().x; }
	float Input::GetMouseY() { return GetMousePosition().y; }
}