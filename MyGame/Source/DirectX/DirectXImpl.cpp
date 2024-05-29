#include "CommonHeaders.h"

#include "DirectXImpl.h"
#include "DirectXHelpers.h"
#include "CommandHelper.h"
#include "CommandContext.h"
#include "GraphicsCommon.h"

#include "../Core/Application.h"
#include "../Core/Timer.h"
#include "../Debugs/DebugHelpers.h"
#include "../Renderer/TextureManager.h"
#include "../Renderer/Shader.h"

#include "../Shaders/ScreenQuadPresentVS.h"
#include "../Shaders/PresentSDRPS.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	static constexpr uint32_t s_FrameCount = 3;
	static constexpr DXGI_FORMAT s_SwapchainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	static uint32_t s_CurrentFrame = 0;

	static ColorBuffer s_DisplayPlane[s_FrameCount];
	ColorBuffer s_SceneBuffer;
	static RootSignature s_PresentRS;
	static GraphicsPSO s_PresentSDRPS(L"PresentSDR GraphicsPSO");
	static GraphicsPSO s_PresentHDRPS(L"PresentHDR GraphicsPSO");

	void DirectXImpl::Init()
	{
		Timer initTime;
		MYGAME_INFO("DirectX: Initializing DirectX 12 API");

		LoadPipeline();

		GraphicsCommon::Init();
		InitCommonStates();

		MYGAME_INFO("DirectX: Initialization done in {0} milliseconds", initTime.ElapsedMillis());
	}

	void DirectXImpl::InitCommonStates()
	{
		s_PresentRS.Reset(4, 2);
		s_PresentRS[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 2);
		s_PresentRS[1].InitAsConstants(0, 6, D3D12_SHADER_VISIBILITY_ALL);
		s_PresentRS[2].InitAsBufferSRV(2, D3D12_SHADER_VISIBILITY_PIXEL);
		s_PresentRS[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 2);
		s_PresentRS.InitStaticSampler(0, GraphicsCommon::SamplerLinearClampDesc);
		s_PresentRS.InitStaticSampler(1, GraphicsCommon::SamplerPointClampDesc);
		s_PresentRS.Finalize(L"Present");

		s_PresentSDRPS.SetRootSignature(s_PresentRS);
		s_PresentSDRPS.SetRasterizerState(GraphicsCommon::RasterizerTwoSided);
		s_PresentSDRPS.SetBlendState(GraphicsCommon::BlendDisable);
		s_PresentSDRPS.SetDepthStencilState(GraphicsCommon::DepthStateDisabled);
		s_PresentSDRPS.SetSampleMask(0xFFFFFFFF);
		s_PresentSDRPS.SetInputLayout(nullptr, 0);
		s_PresentSDRPS.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		s_PresentSDRPS.SetVertexShader(ScreenQuadPresentVS, sizeof(ScreenQuadPresentVS));
		s_PresentSDRPS.SetPixelShader(PresentSDRPS, sizeof(PresentSDRPS));
		s_PresentSDRPS.SetRenderTargetFormat(s_SwapchainFormat, DXGI_FORMAT_UNKNOWN);
		s_PresentSDRPS.Finalize();

		s_SceneBuffer.Create(L"Scene ColorBuffer", s_DisplayPlane[0].GetWidth(), s_DisplayPlane[0].GetHeight(), 1, s_SwapchainFormat);
	}

	void DirectXImpl::OnDestroy()
	{
		SwapChain->SetFullscreenState(FALSE, nullptr);
		SwapChain->Release();

		for (auto& rtv : s_DisplayPlane)
			rtv.Destroy();
	}

	void DirectXImpl::OnResize(uint32_t width, uint32_t height)
	{
		MYGAME_INFO("DirectX: Changing Display resolution to {0} * {1}", width, height);

		CommandListManager::IdleGPU();

		for (uint32_t i = 0; i < s_FrameCount; ++i)
			s_DisplayPlane[i].Destroy();

		MYGAME_HRESULT_TOSTR(DirectXImpl::SwapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0));

		s_CurrentFrame = 0;
		CommandListManager::IdleGPU();

		CreateRenderTargetViews();
	}

	void DirectXImpl::LoadPipeline()
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

		DXGI_ADAPTER_DESC1 desc = {};
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
		swapChainDesc.BufferCount = s_FrameCount;
		swapChainDesc.Format = s_SwapchainFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;

		CommandListManager::Create(Device);
		ID3D12CommandQueue* commandQueue = CommandListManager::GetCommandQueue();

		MYGAME_INFO("DirectX: Creating Swap Chain");
		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue, Application::Get().GetNativeWindow(), &swapChainDesc, nullptr, nullptr, &swapChain));
		ThrowIfFailed(swapChain->QueryInterface(&SwapChain));

		CreateRenderTargetViews();
	}

	void DirectXImpl::PreparePresentSDR()
	{
		GraphicsContext& ctx = GraphicsContext::Begin(L"Present");

		ctx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx.TransitionResource(s_SceneBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		ctx.SetDynamicDescriptor(0, 0, s_SceneBuffer.GetSRV());

		ctx.SetRootSignature(s_PresentRS);
		ctx.SetPipelineState(s_PresentSDRPS);
		ctx.TransitionResource(s_DisplayPlane[s_CurrentFrame], D3D12_RESOURCE_STATE_RENDER_TARGET);
		ctx.SetRenderTarget(s_DisplayPlane[s_CurrentFrame].GetRTV());
		ctx.SetViewportAndScissor(0, 0, s_DisplayPlane[s_CurrentFrame].GetWidth(), s_DisplayPlane[s_CurrentFrame].GetHeight());

		ctx.TransitionResource(s_DisplayPlane[s_CurrentFrame], D3D12_RESOURCE_STATE_PRESENT);
		ctx.Finish();
	}


	void DirectXImpl::Present()
	{
		PreparePresentSDR();

		SwapChain->Present(1, 0);
		s_CurrentFrame = (s_CurrentFrame + 1) % s_FrameCount;
	}

	void DirectXImpl::CreateRenderTargetViews()
	{
		MYGAME_INFO("DirectX: Creating Render Target Views");

		for (UINT i = 0; i < s_FrameCount; ++i)
		{
			ComPtr<ID3D12Resource> resource;
			ThrowIfFailed(SwapChain->GetBuffer(i, IID_PPV_ARGS(&resource)));
			s_DisplayPlane[i].CreateFromSwapChain(L"DisplayPlane ColorBuffer", resource.Detach());
		}
	}

	void DirectXImpl::GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
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