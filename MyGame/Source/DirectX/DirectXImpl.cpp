#include "CommonHeaders.h"

#include "DirectXImpl.h"
#include "DirectXHelpers.h"

#include "../Core/Application.h"
#include "../Core/Timer.h"
#include "../Debugs/DebugHelpers.h"

#include "CommandHelper.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	namespace DirectXImpl
	{
		void OnInit()
		{
			MYGAME_INFO("DirectX: Initializing DirectX 12 API");
			Timer initTime;

			LoadPipeline();
			LoadAssets();

			MYGAME_INFO("DirectX: Initialization done in {0} milliseconds", initTime.ElapsedMillis());
		}

		void OnDestroy()
		{
			SwapChain->SetFullscreenState(FALSE, nullptr);
			SwapChain->Release();

			for (auto& rtv : RenderTargets)
				rtv.Destroy();
		}

		void OnResize(int Width, int Height)
		{
			MYGAME_INFO("DirectX: Changing Display resolution to {0} * {1}", Width, Height);

			CommandListManager::IdleGPU();

			for (uint32_t i = 0; i < FrameCount; ++i)
				RenderTargets[i].Destroy();

			MYGAME_HRESULT_TOSTR(DirectXImpl::SwapChain->ResizeBuffers(0, (UINT)Width, (UINT)Height, DXGI_FORMAT_UNKNOWN, 0));

			FrameIndex = 0;
			CommandListManager::IdleGPU();

			CreateRenderTargetViews();
		}

		void LoadPipeline()
		{
			UINT dxgiFactoryFlags = 0;
#ifdef MYGAME_DEBUG 
			{
				ComPtr<ID3D12Debug> debugController;
				ComPtr<ID3D12Debug1> debugController1;

				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
				{
					MYGAME_INFO("DirectX: Enabling debug Layer");

					debugController->EnableDebugLayer();
					debugController->QueryInterface(IID_PPV_ARGS(&debugController1));
					debugController1->SetEnableGPUBasedValidation(TRUE);
					debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
					dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
				}

				UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };
				MYGAME_HRESULT_TOSTR(D3D12EnableExperimentalFeatures(1, experimentalFeatures, nullptr, nullptr));
			}
#endif	
			MYGAME_INFO("DirectX: Creating Dxgi Factory");
			ComPtr<IDXGIFactory4> factory;
			ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

			ComPtr<IDXGIAdapter1> adapter;
			GetHardwareAdapter(factory.Get(), &adapter);

			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			MYGAME_INFO(L"DirectX: Selected GPU: {0} || Dedicated Video Memory: {1} MB", desc.Description, desc.DedicatedVideoMemory >> 20);
			MYGAME_INFO("DirectX: Creating DirectX12 Graphics Device");
			ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&Device)));

#ifdef MYGAME_DEBUG
			{
				ComPtr<ID3D12InfoQueue> pInfoQueue;
				Device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
				//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			}
#endif

			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = FrameCount;
			swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;

			CommandListManager::Create(Device);
			ID3D12CommandQueue* commandQueue = CommandListManager::GetCommandQueue();

			MYGAME_INFO("DirectX: Creating Swap Chain");
			ComPtr<IDXGISwapChain1> swapChain;
			ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue, Application::Get().GetNativeWindow(), &swapChainDesc, nullptr, nullptr, &swapChain));
			ThrowIfFailed(swapChain->QueryInterface(&SwapChain));
		}

		void LoadAssets()
		{
			MYGAME_INFO("DirectX: Creating SRV Heap");
			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.NumDescriptors = 1;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&SrvHeap)));
			NAME_D3D12_OBJ(SrvHeap);

			CreateRenderTargetViews();
		}

		void Present()
		{
			SwapChain->Present(1, 0);
			FrameIndex = (FrameIndex + 1) % FrameCount;
		}

		void CreateRenderTargetViews()
		{
			MYGAME_INFO("DirectX: Creating Render Target Views");

			for (UINT i = 0; i < FrameCount; ++i)
			{
				ComPtr<ID3D12Resource> DisplayPlane;
				ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&DisplayPlane)));
				RenderTargets[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
			}
		}

		void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
		{
			for (UINT adapterIndex = 0; ; ++adapterIndex)
			{
				IDXGIAdapter1* pAdapter;
				if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter)) break;
				if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_12_2, _uuidof(ID3D12Device), nullptr)))
				{
					*ppAdapter = pAdapter;
					return;
				}
				pAdapter->Release();
			}
		}
	}
}