#include "CommonHeaders.h"

#include "DirectXImpl.h"
#include "DirectXHelpers.h"
#include "../Core/Application.h"
#include "../Debugs/DebugHelpers.h"

#include <imgui_impl_dx12.h>

// TOOO remove
#include "../Renderer/Shader.h"

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
		const UINT64 fence = m_fenceValue;
		const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

		ThrowIfFailed(m_commandQueue->Signal(m_fence, m_fenceValue));
		++m_fenceValue;

		if (lastCompletedFence < fence)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
		CloseHandle(m_fenceEvent);
	}

	void DirectXImpl::LoadPipeline()
	{

#ifdef MYGAME_DEBUG 
		{
			ID3D12Debug* debugController;
			ID3D12Debug1* debugController1;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				debugController->QueryInterface(IID_PPV_ARGS(&debugController1));
				debugController1->SetEnableGPUBasedValidation(TRUE);
				debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
			}
		}
#endif	

		// TODO temporary
		UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };
		MYGAME_HRESULT_TOSTR(D3D12EnableExperimentalFeatures(1, experimentalFeatures, nullptr, nullptr));

		IDXGIFactory4* factory;
		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
		if (m_useWarpDevice)
		{
			IDXGIAdapter* warpAdapter;
			ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
			ThrowIfFailed(D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&D12Device)));
		}
		else
		{
			IDXGIAdapter1* hardwareAdapter;
			GetHardwareAdapter(factory, &hardwareAdapter);
			ThrowIfFailed(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&D12Device)));
		}

#ifdef MYGAME_DEBUG
		ID3D12InfoQueue* pInfoQueue;
		D12Device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
#endif

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
		NAME_D3D12_OBJ_STR(m_commandQueue, L"D3D12_CmdQueue");

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;

		IDXGISwapChain1* swapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(m_commandQueue, Application::Get().GetNativeWindow(), &swapChainDesc, nullptr, nullptr, &swapChain));
		ThrowIfFailed(swapChain->QueryInterface(&m_swapChain));
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(D12Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));
		NAME_D3D12_OBJ_STR(m_srvHeap, L"D3D12_SrvHeap");

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(D12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
		NAME_D3D12_OBJ_STR(m_rtvHeap, L"D3D12_RtvHeap");
		m_rtvDescriptorSize = D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.NumDescriptors = 1 + FrameCount;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(D12Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
		NAME_D3D12_OBJ_STR(m_dsvHeap, L"D3D12_DsvHeap");
		m_dsvDescriptorSize = D12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.NumDescriptors = 2;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(D12Device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap)));
		NAME_D3D12_OBJ_STR(m_samplerHeap, L"D3D12_SamplerHeap");

		ThrowIfFailed(D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
		NAME_D3D12_OBJ_STR(m_commandAllocator, L"D3D12_CmdAlloc");

		ThrowIfFailed(D12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, m_pipelineState, IID_PPV_ARGS(&m_commandList)));
		ThrowIfFailed(m_commandList->Close());
		NAME_D3D12_OBJ_STR(m_commandList, L"D3D12_CmdList");

		CreateRenderTargets();

		ThrowIfFailed(D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		NAME_D3D12_OBJ_STR(m_fence, L"D3D12_Fence");
		++m_fenceValue;

		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr) { ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError())); }
	}

	void DirectXImpl::LoadAssets()
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(D12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

		D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 ranges[4] = {}; // TODO Perfomance TIP: Order from most frequent to least frequent.
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);

		CD3DX12_ROOT_PARAMETER1 rootParams[4] = {};
		rootParams[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParams[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParams[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParams[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
		rootSigDesc.Init_1_1(_countof(rootParams), rootParams, 0, nullptr, rootSigFlags);

		ID3DBlob* signature;
		ID3DBlob* error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSigDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(D12Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		NAME_D3D12_OBJ_STR(m_rootSignature, L"D312_RootSig");

#ifdef MYGAME_USE_DXCOMPILER		
		IDxcBlob* vertexShader = Shader::CompileVertexShader(L"Assets/Shaders/ShaderVertex.hlsl");
		IDxcBlob* pixelShader = Shader::CompilePixelShader(L"Assets/Shaders/ShaderPixel.hlsl");
#else
		ID3DBlob* vertexShader = Shader::D3CompileVertexShader(L"Assets/Shaders/ShaderVertex.hlsl");
		ID3DBlob* pixelShader = Shader::D3CompilePixelShader(L"Assets/Shaders/ShaderPixel.hlsl");
#endif

		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = FALSE;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputLayoutDesc, _countof(inputLayoutDesc) };
		psoDesc.pRootSignature = m_rootSignature;

#ifdef MYGAME_USE_DXCOMPILER
		psoDesc.VS = { reinterpret_cast<UINT8 * *(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
		psoDesc.PS = { reinterpret_cast<UINT8 * *(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
#else
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
#endif

		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(D12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		NAME_D3D12_OBJ_STR(m_pipelineState, L"D312_PSO");

		psoDesc.PS = CD3DX12_SHADER_BYTECODE(0, 0);
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;
	}

	void DirectXImpl::CreateRenderTargets()
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < FrameCount; ++i)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
			D12Device->CreateRenderTargetView(m_renderTargets[i], nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
			NAME_D3D12_OBJ_INDEXED_STR(m_renderTargets[i], L"D3D12_Rtv", i);
		}
	}

	void DirectXImpl::CleanupRenderTarget()
	{
		WaitForGpu();
		for (auto& renderTarget : m_renderTargets) renderTarget->Release();
	}

	void DirectXImpl::WaitForGpu()
	{
		ThrowIfFailed(m_commandQueue->Signal(m_fence, m_fenceValue));
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	void DirectXImpl::GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
	{
		*ppAdapter = nullptr;
		for (UINT adapterIndex = 0; ; ++adapterIndex)
		{
			IDXGIAdapter1* pAdapter;
			if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter)) break;
			if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				*ppAdapter = pAdapter;
				return;
			}
			pAdapter->Release();
		}
	}
}