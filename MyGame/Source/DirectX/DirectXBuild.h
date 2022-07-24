#pragma once

// Graphics Framework
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// DirectX 12 API
#include <d3d12.h>
#include <dxgi1_6.h>

#ifdef MYGAME_DEBUG
#define MYGAME_DIRECTX_DEBUG
#endif

#ifdef MYGAME_DIRECTX_DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

namespace MyGame
{
	namespace DirectXImpl
	{
		// DirectX data
		struct FrameContext
		{
			ID3D12CommandAllocator* CommandAllocator;
			UINT64                  FenceValue;
		};

		static int const NUM_FRAMES_IN_FLIGHT = 3;
		extern FrameContext g_frameContext[NUM_FRAMES_IN_FLIGHT];
		extern UINT g_frameIndex;

		static int const NUM_BACK_BUFFERS = 3;
		extern ID3D12Device* g_pd3dDevice;
		extern ID3D12DescriptorHeap* g_pd3dRtvDescHeap;
		extern ID3D12DescriptorHeap* g_pd3dSrvDescHeap;
		extern ID3D12CommandQueue* g_pd3dCommandQueue;
		extern ID3D12GraphicsCommandList* g_pd3dCommandList;
		extern ID3D12Fence* g_fence;
		extern HANDLE g_fenceEvent;
		extern UINT64 g_fenceLastSignaledValue;
		extern IDXGISwapChain3* g_pSwapChain;
		extern HANDLE g_hSwapChainWaitableObject;
		extern ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS];
		extern D3D12_CPU_DESCRIPTOR_HANDLE g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS];

		// Forward declarations of helper functions
		extern bool CreateDeviceD3D(GLFWwindow*);
		extern void CleanupDeviceD3D();
		extern void CreateRenderTarget();
		extern void CleanupRenderTarget();
		extern void WaitForLastSubmittedFrame();
		extern FrameContext* WaitForNextFrameResources();
	}
}