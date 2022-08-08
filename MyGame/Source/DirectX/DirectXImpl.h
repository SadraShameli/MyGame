#pragma once

#include "DescriptorHeap.h"

namespace MyGame
{
	class DirectXImpl
	{
	public:
		static void OnInit();
		static void OnDestroy();

	private:
		inline static constexpr UINT FrameCount = 3;
		inline static constexpr UINT NumContexts = 3;

		struct FrameContext
		{
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList;
			UINT64 FenceValue;
		};
		inline static FrameContext m_frameContext[FrameCount] = {};

		inline static CD3DX12_VIEWPORT m_viewport;
		inline static CD3DX12_RECT m_scissorRect;
		inline static Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
		inline static Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		inline static Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
		inline static Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
		inline static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		inline static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		inline static Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
		inline static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
		inline static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		inline static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
		inline static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		inline static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
		inline static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplerHeap;
		inline static Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

		inline static UINT m_frameIndex;
		inline static HANDLE m_fenceEvent;
		inline static Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		inline static UINT64 m_fenceValue;
		inline static UINT64 m_LastfenceValue;
		inline static UINT m_rtvDescriptorSize;
		inline static bool m_useWarpDevice = false;

		static void LoadPipeline();
		static void LoadAssets();
		static void CreateRenderTargets();
		static void CleanupRenderTarget();

		static void WaitForGpu();
		static void GetHardwareAdapter(IDXGIFactory4*, IDXGIAdapter1**);

		// Remove
		static void WaitForLastSubmittedFrame();
		static FrameContext* WaitForNextFrameResources();
		inline static HANDLE m_swapChainWaitableObject = NULL;

	private:
		friend class Renderer;
		friend class DescriptorHeap;
		friend class DescriptorAllocator;
		friend class DynamicDescriptorHeap;
		friend class LinearAllocatorPageManager;
		friend class CommandAllocatorPool;
		friend class CommandQueue;
		friend class CommandListManager;
		friend class CommandContext;
		friend class CommandSignature;
		friend class ComputePipelineState;
		friend class RootSignature;
		friend class GraphicsPSO;
		friend class GpuBuffer;
		friend class ReadbackBuffer;
		friend class DepthBuffer;
		friend class UploadBuffer;
		friend class ByteAddressBuffer;
		friend class StructuredBuffer;
		friend class TypedBuffer;
		friend class ColorBuffer;
		friend class ManagedTexture;
		friend class SamplerDesc;
		friend class SampleManager;
		friend class ComputeContext;
		friend class Texture;

		inline static DescriptorAllocator g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
		{
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV
		};
		inline static D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1) { return g_DescriptorAllocator[Type].Allocate(Count); }
	};
}
