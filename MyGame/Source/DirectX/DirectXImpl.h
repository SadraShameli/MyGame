#pragma once

#include "CommonIncludes.h"
#include "BufferHelper.h"

namespace MyGame
{
	class DirectXImpl
	{
	public:
		static void Init();
		static void OnDestroy();
		static void OnResize(uint32_t width, uint32_t height);

		static void LoadPipeline();
		static void Present();

		static void CreateRenderTargetViews();
		static void GetHardwareAdapter(IDXGIFactory4*, IDXGIAdapter1**);

		inline static IDXGISwapChain3* SwapChain = nullptr;
		inline static ID3D12Device* Device = nullptr;

	private:
		inline static void InitCommonStates();
		inline static void PreparePresentSDR();
	};
}
