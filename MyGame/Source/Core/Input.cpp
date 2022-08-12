#include "CommonHeaders.h"

#include "Input.h"
#include "Application.h"

namespace MyGame
{
	bool Input::IsKeyPressed(int key) { return GetKeyState(key) < 0; }

	bool Input::IsMouseButtonPressed(int button) { return GetKeyState(button) < 0; }

	DirectX::XMFLOAT2 Input::GetMousePosition()
	{
		POINT pos = {};
		GetCursorPos(&pos);
		return { static_cast<float>(pos.x), static_cast<float>(pos.y) };
	}

	float Input::GetMouseX() { return GetMousePosition().x; }
	float Input::GetMouseY() { return GetMousePosition().y; }
}