#include "CommonHeaders.h"

#include "Renderer.h"

#include "../DirectX/DirectXImpl.h"

#include <SimpleMath.h>

namespace MyGame
{
	void Renderer::Init()
	{
		// Initialize DirectX 12
		DirectX.OnInit();
	}

	void Renderer::OnWindowResize(const int width, const int height) { DirectX.OnWindowResize(width, height); }
}