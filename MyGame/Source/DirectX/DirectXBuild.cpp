#include "CommonHeaders.h"

#include "DirectXBuild.h"

#include "Source/Debugs/Assert.h"

// DirectX 12 API
#include "PlatformHelpers.h"

namespace MyGame
{
	namespace DirectXImpl
	{
		FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
		UINT g_frameIndex = 0;

		ID3D12Device* g_pd3dDevice = nullptr;
		ID3D12DescriptorHeap* g_pd3dRtvDescHeap = nullptr;
		ID3D12DescriptorHeap* g_pd3dSrvDescHeap = nullptr;
		ID3D12CommandQueue* g_pd3dCommandQueue = nullptr;
		ID3D12GraphicsCommandList* g_pd3dCommandList = nullptr;
		ID3D12Fence* g_fence = nullptr;
		HANDLE g_fenceEvent = nullptr;
		UINT64 g_fenceLastSignaledValue = 0;
		IDXGISwapChain3* g_pSwapChain = nullptr;
		HANDLE g_hSwapChainWaitableObject = nullptr;
		ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

		// Set true to use 4X MSAA. The default is false.
		bool m4xMsaaState = true; // 4X MSAA enabled
		UINT m4xMsaaQuality = 4;

		bool CreateDeviceD3D(GLFWwindow* window)
		{
			// Get GLFW's underlying win32 handle
			auto hWnd = glfwGetWin32Window(window);

			// Setup swap chain
			DXGI_SWAP_CHAIN_DESC1 sd;
			{
				ZeroMemory(&sd, sizeof(sd));
				sd.BufferCount = NUM_BACK_BUFFERS;
				sd.Width = 0;
				sd.Height = 0;
				sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
				sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				sd.SampleDesc.Count = 1;
				sd.SampleDesc.Quality = 0;
				sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
				sd.Scaling = DXGI_SCALING_NONE;
				sd.Stereo = false;
			}

#ifdef MYGAME_DIRECTX_DEBUG
			// [DEBUG] Enable debug interface
			ID3D12Debug* pdx12Debug = nullptr;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
				pdx12Debug->EnableDebugLayer();
#endif

			// Create device
			if (D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
				return false;

#ifdef MYGAME_DIRECTX_DEBUG
			// [DEBUG] Setup debug interface to break on any warnings/errors
			if (pdx12Debug != nullptr)
			{
				ID3D12InfoQueue* pInfoQueue = nullptr;
				g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
				pInfoQueue->Release();
				pdx12Debug->Release();
			}
#endif

			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				desc.NumDescriptors = NUM_BACK_BUFFERS;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				desc.NodeMask = 1;
				if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
					return false;

				SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
				for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
				{
					g_mainRenderTargetDescriptor[i] = rtvHandle;
					rtvHandle.ptr += rtvDescriptorSize;
				}
			}

			{
				D3D12_COMMAND_QUEUE_DESC desc = {};
				desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
				desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
				desc.NodeMask = 1;
				if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
					return false;
			}

			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				desc.NumDescriptors = 1;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
					return false;
			}

			for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
				if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
					return false;

			if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, nullptr, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
				g_pd3dCommandList->Close() != S_OK)
				return false;

			if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
				return false;

			D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};
			msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			msQualityLevels.SampleCount = 4;
			msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
			msQualityLevels.NumQualityLevels = 0;
			DirectX::ThrowIfFailed(g_pd3dDevice->CheckFeatureSupport(
				D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
				&msQualityLevels,
				sizeof(msQualityLevels)));
			m4xMsaaQuality = msQualityLevels.NumQualityLevels;
			MYGAME_ASSERT(m4xMsaaQuality, "Unexpected MSAA quality level.");

			g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (g_fenceEvent == nullptr)
				return false;

			{
				IDXGIFactory4* dxgiFactory = nullptr;
				IDXGISwapChain1* swapChain1 = nullptr;
				if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
					return false;
				if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1) != S_OK)
					return false;
				if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
					return false;
				swapChain1->Release();
				dxgiFactory->Release();
				g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
				g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
			}

			CreateRenderTarget();
			return true;
		}

		void CleanupDeviceD3D()
		{
			CleanupRenderTarget();
			if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, nullptr); g_pSwapChain->Release(); g_pSwapChain = nullptr; }
			if (g_hSwapChainWaitableObject != nullptr) { CloseHandle(g_hSwapChainWaitableObject); }
			for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
				if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = nullptr; }
			if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = nullptr; }
			if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = nullptr; }
			if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = nullptr; }
			if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = nullptr; }
			if (g_fence) { g_fence->Release(); g_fence = nullptr; }
			if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = nullptr; }
			if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

#ifdef MYGAME_DIRECTX_DEBUG
			IDXGIDebug1* pDebug = nullptr;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
			{
				pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
				pDebug->Release();
			}
#endif
		}

		void CreateRenderTarget()
		{
			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
			{
				ID3D12Resource* pBackBuffer = nullptr;
				g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
				g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, g_mainRenderTargetDescriptor[i]);
				g_mainRenderTargetResource[i] = pBackBuffer;
			}
		}

		void CleanupRenderTarget()
		{
			WaitForLastSubmittedFrame();

			for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
				if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = nullptr; }
		}

		void WaitForLastSubmittedFrame()
		{
			FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

			UINT64 fenceValue = frameCtx->FenceValue;
			if (fenceValue == 0)
				return; // No fence was signaled

			frameCtx->FenceValue = 0;
			if (g_fence->GetCompletedValue() >= fenceValue)
				return;

			g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
			WaitForSingleObject(g_fenceEvent, INFINITE);
		}

		FrameContext* WaitForNextFrameResources()
		{
			UINT nextFrameIndex = g_frameIndex + 1;
			g_frameIndex = nextFrameIndex;

			HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, nullptr };
			DWORD numWaitableObjects = 1;

			FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
			UINT64 fenceValue = frameCtx->FenceValue;
			if (fenceValue != 0) // means no fence was signaled
			{
				frameCtx->FenceValue = 0;
				g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
				waitableObjects[1] = g_fenceEvent;
				numWaitableObjects = 2;
			}

			WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

			return frameCtx;
		}
	}
}