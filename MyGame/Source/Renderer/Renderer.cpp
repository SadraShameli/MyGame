#include "CommonHeaders.h"

#include "Renderer.h"

#include "../Core/Application.h"
#include "../Core/Log.h"
#include "../Debugs/DebugHelpers.h"

using namespace DirectX;

namespace MyGame
{

	void Renderer::OnInit()
	{
		DirectXImpl::OnInit();
	}

	void Renderer::OnUpdate()
	{
		DirectXImpl::Present();
	}

	void Renderer::InitImGui()
	{

	}

	void Renderer::RenderImGui()
	{

	}

	void Renderer::OnWindowResize(int width, int height) { DirectXImpl::OnResize(width, height); }
}