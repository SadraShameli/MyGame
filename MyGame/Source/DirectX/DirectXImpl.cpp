#include "CommonHeaders.h"

#include "DirectXImpl.h"
#include "DirectXHelpers.h"

#include "../Core/Application.h"
#include "../Core/Timer.h"
#include "../Debugs/DebugHelpers.h"

// TOOO remove
#include "../Renderer/Shader.h"
#include "RootSignature.h"
#include "PipelineState.h"

#define MYGAME_USE_DXCOMPILER

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	void DirectXImpl::OnInit()
	{
		LoadPipeline();
		LoadAssets();
	}

	void DirectXImpl::OnDestroy()
	{
		const UINT64 fence = D3D12_FenceValue;
		const UINT64 lastCompletedFence = D3D12_Fence->GetCompletedValue();

		ThrowIfFailed(D3D12_CmdQueue->Signal(D3D12_Fence, D3D12_FenceValue));
		++D3D12_FenceValue;

		if (lastCompletedFence < fence)
		{
			ThrowIfFailed(D3D12_Fence->SetEventOnCompletion(fence, D312_FenceEvent));
			WaitForSingleObject(D312_FenceEvent, INFINITE);
		}
		CloseHandle(D312_FenceEvent);
	}

	void DirectXImpl::LoadPipeline()
	{
		MYGAME_INFO("DirectX: Initializing DirectX 12 API");
		Timer initTime;

		UINT dxgiFactoryFlags = 0;
		m_viewport.Width = (float)Application::Get().GetWindow().GetWidth();
		m_viewport.Height = (float)Application::Get().GetWindow().GetHeight();
		m_scissorRect.right = (long)Application::Get().GetWindow().GetWidth();
		m_scissorRect.bottom = (long)Application::Get().GetWindow().GetHeight();

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
		ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&D3D12_Device)));

#ifdef MYGAME_DEBUG
		{
			ComPtr<ID3D12InfoQueue> pInfoQueue;
			D3D12_Device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
			//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		}
#endif

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(D3D12_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&D3D12_CmdQueue)));
		NAME_D3D12_OBJ(D3D12_CmdQueue);

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		MYGAME_INFO("DirectX: Creating Swap Chain");
		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(D3D12_CmdQueue, Application::Get().GetNativeWindow(), &swapChainDesc, nullptr, nullptr, &swapChain));
		ThrowIfFailed(swapChain->QueryInterface(&m_swapChain));

		MYGAME_INFO("DirectX: Creating RTV Heap");
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(D3D12_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&D3D12_RtvHeap)));
		NAME_D3D12_OBJ(D3D12_RtvHeap);
		m_rtvDescriptorSize = D3D12_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		MYGAME_INFO("DirectX: Creating DSV Heap");
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.NumDescriptors = 1 + FrameCount;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(D3D12_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&D3D12_DsvHeap)));
		NAME_D3D12_OBJ(D3D12_DsvHeap);
		m_dsvDescriptorSize = D3D12_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		MYGAME_INFO("DirectX: Creating SRV Heap");
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(D3D12_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&D3D12_SrvHeap)));
		NAME_D3D12_OBJ(D3D12_SrvHeap);

		MYGAME_INFO("DirectX: Creating Sampler Heap");
		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.NumDescriptors = 2;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(D3D12_Device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&D3D12_SamplerHeap)));
		NAME_D3D12_OBJ(D3D12_SamplerHeap);

		MYGAME_INFO("DirectX: Creating Render Targets");
		CreateRenderTargets();

		MYGAME_INFO("DirectX: Initialization done in {0} milliseconds", initTime.ElapsedMillis());
	}

	void DirectXImpl::LoadAssets()
	{
		D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		RootSignature sig;
		sig.Reset(4, 0);
		sig[0].InitAsConstants(0, 4);
		sig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10);
		sig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 10);
		sig[3].InitAsConstantBuffer(1);
		sig.Finalize(L"Triangle", rootSigFlags);

		ComPtr<IDxcBlob> vertexShader;
		ComPtr<IDxcBlob> pixelShader;

		MYGAME_HRESULT_VALIDATE(Shader::CompileVertexShader(&vertexShader, L"Assets/Shaders/ShaderVertex.hlsl"));
		MYGAME_HRESULT_VALIDATE(Shader::CompilePixelShader(&pixelShader, L"Assets/Shaders/ShaderPixel.hlsl"));

		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D312_RootSig = sig.GetSignature();

		GraphicsPSO pso(L"Triangle");
		pso.SetRootSignature(sig);
		pso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		pso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		pso.SetInputLayout(inputLayoutDesc, _countof(inputLayoutDesc));
		pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		pso.SetVertexShader(vertexShader.Get());
		pso.SetPixelShader(pixelShader.Get());
		pso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
		pso.Finalize();

		D312_PSO = pso.GetPipelineStateObject();

		ThrowIfFailed(D3D12_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&D3D12_CmdAlloc)));
		NAME_D3D12_OBJ(D3D12_CmdAlloc);

		ThrowIfFailed(D3D12_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_CmdAlloc, D312_PSO, IID_PPV_ARGS(&D3D12_CmdList)));
		ThrowIfFailed(D3D12_CmdList->Close());
		NAME_D3D12_OBJ(D3D12_CmdList);

		ThrowIfFailed(D3D12_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&D3D12_Fence)));
		NAME_D3D12_OBJ(D3D12_Fence);

		D312_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (D312_FenceEvent == nullptr) { ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError())); }
	}

	void DirectXImpl::CreateRenderTargets()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(D3D12_RtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < FrameCount; ++i)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&D3D12_RenderTargets[i])));
			D3D12_Device->CreateRenderTargetView(D3D12_RenderTargets[i], nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
			NAME_D3D12_OBJ_INDEXED(D3D12_RenderTargets[i], i);
		}
	}

	void DirectXImpl::CleanupRenderTarget()
	{
		WaitForGpu();
		for (auto& renderTarget : D3D12_RenderTargets) renderTarget->Release();
	}

	void DirectXImpl::Present()
	{
		m_swapChain->Present(1, 0);
		//m_frameIndex = (m_frameIndex + 1) % FrameCount;
	}

	void DirectXImpl::WaitForGpu()
	{
		//ThrowIfFailed(D3D12_CmdQueue->Signal(D3D12_Fence, D3D12_FenceValue));
		//ThrowIfFailed(D3D12_Fence->SetEventOnCompletion(D3D12_FenceValue, D312_FenceEvent));
		//WaitForSingleObjectEx(D312_FenceEvent, INFINITE, FALSE);

		const UINT64 fence = D3D12_FenceValue;
		ThrowIfFailed(D3D12_CmdQueue->Signal(D3D12_Fence, fence));
		D3D12_FenceValue++;

		// Wait until the previous frame is finished.
		if (D3D12_Fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(D3D12_Fence->SetEventOnCompletion(fence, D312_FenceEvent));
			WaitForSingleObject(D312_FenceEvent, INFINITE);
		}

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
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