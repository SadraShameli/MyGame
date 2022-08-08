#pragma once

namespace MyGame
{
	class Renderer
	{
	public:
		static void Init();
		static void InitImGui();
		static void RenderImGui();

		static void OnWindowResize(const int, const int);
	};
}