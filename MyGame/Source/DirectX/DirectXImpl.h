#pragma once

#include "CommonIncludes.h"
#include "BufferHelper.h"

namespace MyGame
{
	namespace DirectXImpl
	{
		extern void OnInit();
		extern void OnDestroy();
		extern void OnResize(int Width, int Height);

		extern void LoadPipeline();

		extern void Present();
		extern void CreateRenderTargetViews();

		extern void GetHardwareAdapter(IDXGIFactory4*, IDXGIAdapter1**);

		inline constexpr UINT FrameCount = 3;
		inline UINT FrameIndex = 0;

		inline IDXGISwapChain3* SwapChain = nullptr;
		inline ID3D12Device* Device = nullptr;

		inline ColorBuffer RenderTargets[FrameCount];
	};
}
