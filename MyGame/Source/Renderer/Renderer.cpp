#include "CommonHeaders.h"

#include "Renderer.h"

#include "Source/DirectX/DirectXBuild.h"

#include <SimpleMath.h>

using namespace MyGame::DirectXImpl;

void Renderer::OnWindowResize(const int width, const int height)
{
	WaitForLastSubmittedFrame();
	CleanupRenderTarget();
	HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
	assert(SUCCEEDED(result) && "Failed to resize swapchain.");
	CreateRenderTarget();
}