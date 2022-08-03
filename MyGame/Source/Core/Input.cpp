#include "CommonHeaders.h"

#include "Input.h"
#include "Application.h"

#include <GLFW/glfw3.h>

namespace MyGame
{
	bool Input::IsKeyPressed(const int key)
	{
		auto* window = application.GetWindow().GetWindow();
		auto state = glfwGetKey(window, key);
		return state == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(const int button)
	{
		auto* window = application.GetWindow().GetWindow();
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}

	DirectX::XMFLOAT2 Input::GetMousePosition()
	{
		auto* window = application.GetWindow().GetWindow();
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	float Input::GetMouseX() { return GetMousePosition().x; }
	float Input::GetMouseY() { return GetMousePosition().y; }
}