#pragma once

#include "../Events/EventCodes/KeyCodes.h"
#include "../Events/EventCodes/MouseCodes.h"

#include <DirectXMath.h>

namespace MyGame
{
	class Input
	{
	public:
		static bool IsKeyPressed(const int);
		static bool IsMouseButtonPressed(const int);

		static DirectX::XMFLOAT2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}
