#pragma once

#include "DescriptorHeap.h"

namespace MyGame
{
	class DirectXImpl
	{
	public:
		static void OnInit();
		static void OnDestroy();

		inline static constexpr UINT FrameCount = 3;

		inline static CD3DX12_VIEWPORT m_viewport;
		inline static CD3DX12_RECT m_scissorRect;
		inline static IDXGISwapChain3* m_swapChain;
		inline static ID3D12Device* D12Device;
		inline static ID3D12Resource* m_renderTargets[FrameCount];
		inline static ID3D12Resource* m_depthStencil;
		inline static ID3D12CommandAllocator* m_commandAllocator;
		inline static ID3D12GraphicsCommandList* m_commandList;
		inline static ID3D12CommandQueue* m_commandQueue;
		inline static ID3D12RootSignature* m_rootSignature;
		inline static ID3D12DescriptorHeap* m_rtvHeap;
		inline static ID3D12DescriptorHeap* m_srvHeap;
		inline static ID3D12DescriptorHeap* m_dsvHeap;
		inline static ID3D12DescriptorHeap* m_cbvSrvHeap;
		inline static ID3D12DescriptorHeap* m_samplerHeap;
		inline static ID3D12PipelineState* m_pipelineState;
		inline static ID3D12Fence* m_fence;

		inline static UINT m_frameIndex;
		inline static HANDLE m_fenceEvent;
		inline static UINT64 m_fenceValue;
		inline static UINT64 m_LastfenceValue;
		inline static UINT m_rtvDescriptorSize;
		inline static UINT m_dsvDescriptorSize;
		inline static bool m_useWarpDevice = false;

		static void LoadPipeline();
		static void LoadAssets();
		static void CreateRenderTargets();
		static void CleanupRenderTarget();

		static void WaitForGpu();
		static void GetHardwareAdapter(IDXGIFactory4*, IDXGIAdapter1**);

	protected:
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
