#pragma once

#include "CommonIncludes.h"

namespace MyGame
{
	class DirectXImpl
	{
	public:
		static void OnInit();
		static void OnDestroy();

	protected:
		static void LoadPipeline();
		static void LoadAssets();

		static void CreateRenderTargets();
		static void CleanupRenderTarget();
		static void GetHardwareAdapter(IDXGIFactory4*, IDXGIAdapter1**);

		static void Present();
		static void WaitForGpu();

	protected:
		inline static constexpr UINT FrameCount = 3;

		inline static CD3DX12_VIEWPORT m_viewport;
		inline static CD3DX12_RECT m_scissorRect;
		inline static IDXGISwapChain3* m_swapChain;
		inline static ID3D12Device* D3D12_Device;
		inline static ID3D12Resource* D3D12_RenderTargets[FrameCount];
		inline static ID3D12Resource* m_depthStencil;
		inline static ID3D12CommandAllocator* D3D12_CmdAlloc;
		inline static ID3D12GraphicsCommandList* D3D12_CmdList;
		inline static ID3D12CommandQueue* D3D12_CmdQueue;
		inline static ID3D12RootSignature* D312_RootSig;
		inline static ID3D12DescriptorHeap* D3D12_RtvHeap;
		inline static ID3D12DescriptorHeap* D3D12_SrvHeap;
		inline static ID3D12DescriptorHeap* D3D12_DsvHeap;
		inline static ID3D12DescriptorHeap* m_cbvSrvHeap;
		inline static ID3D12DescriptorHeap* D3D12_SamplerHeap;
		inline static ID3D12PipelineState* D312_PSO;
		inline static ID3D12Fence* D3D12_Fence;

		inline static UINT m_frameIndex = 0;
		inline static HANDLE D312_FenceEvent = 0;
		inline static UINT64 D3D12_FenceValue = 1;
		inline static UINT64 m_LastfenceValue = 0;

		inline static UINT m_rtvDescriptorSize = 0;
		inline static UINT m_dsvDescriptorSize = 0;
	};
}
