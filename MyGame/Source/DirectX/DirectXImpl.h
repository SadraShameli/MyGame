#pragma once

#include "DirectXIncludes.h"

namespace MyGame
{
	class DirectXImpl
	{
	public:
		void InitImGui();
		void RenderImGui();
		void OnWindowResize(const int, const int);

		void OnInit();
		void OnDestroy();

	private:
		static constexpr UINT FrameCount = 3;
		static constexpr UINT NumContexts = 3;

		struct FrameContext
		{
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
			UINT64 FenceValue;
		} m_frameContext[FrameCount];

		// Pipeline variables
		static const int CommandListCount = 3;
		static const int CommandListPre = 0;
		static const int CommandListMid = 1;
		static const int CommandListPost = 2;
		bool m_useWarpDevice = false;

		// Pipeline objects.
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
		Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplerHeap;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

		// App resources.
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_textures[128];
		Microsoft::WRL::ComPtr<ID3D12Resource> m_textureUploads[128];
		Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferUpload;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferUpload;
		UINT m_rtvDescriptorSize;

		// Synchronization objects.
		HANDLE m_workerBeginRenderFrame[NumContexts];
		HANDLE m_workerFinishShadowPass[NumContexts];
		HANDLE m_workerFinishedRenderFrame[NumContexts];
		HANDLE m_threadHandles[NumContexts];
		UINT m_frameIndex;
		HANDLE m_fenceEvent;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
		UINT64 m_fenceValue;

		void LoadPipeline();
		void LoadAssets();
		void CreateRenderTargets();
		void CleanupRenderTarget();

		void WaitForGpu();
		void GetHardwareAdapter(IDXGIFactory4*, IDXGIAdapter1**);

		// Remove
		void WaitForLastSubmittedFrame();
		FrameContext* WaitForNextFrameResources();
		HANDLE m_swapChainWaitableObject = NULL;
	};

	extern DirectXImpl DirectX;
}
