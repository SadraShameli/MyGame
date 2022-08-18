#pragma once

#include "../DirectX/DirectXImpl.h"

namespace MyGame
{
	class Renderer
	{
	public:
		static void OnInit();
		static void OnUpdate();
		static void OnWindowResize(int, int);

		static void InitImGui();
		static void RenderImGui();
	};
}