#pragma once

namespace MyGame
{
	class Renderer
	{
	public:
		static void Init();
		static void OnWindowResize(const int, const int);
	};
}