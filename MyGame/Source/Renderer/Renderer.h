#pragma once

#include "../DirectX/DirectXImpl.h"

namespace MyGame
{
	class Renderer : public DirectXImpl
	{
	public:
		static void Init();
		static void InitImGui();
		static void RenderImGui();

		static void OnWindowResize(const int, const int);
	};
}