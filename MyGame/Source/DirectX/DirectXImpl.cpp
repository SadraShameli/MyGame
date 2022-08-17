#include "CommonHeaders.h"

#include "DirectXImpl.h"
#include "DirectXHelpers.h"
#include "../Core/Application.h"
#include "../Debugs/DebugHelpers.h"

#include <imgui_impl_dx12.h>

// TOOO remove
#include "../Renderer/Shader.h"

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

		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(D3D12_CmdQueue, Application::Get().GetNativeWindow(), &swapChainDesc, nullptr, nullptr, &swapChain));
		ThrowIfFailed(swapChain->QueryInterface(&m_swapChain));

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(D3D12_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&D3D12_RtvHeap)));
		NAME_D3D12_OBJ(D3D12_RtvHeap);
		m_rtvDescriptorSize = D3D12_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.NumDescriptors = 1 + FrameCount;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(D3D12_Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&D3D12_DsvHeap)));
		NAME_D3D12_OBJ(D3D12_DsvHeap);
		m_dsvDescriptorSize = D3D12_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(D3D12_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&D3D12_SrvHeap)));
		NAME_D3D12_OBJ(D3D12_SrvHeap);

		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.NumDescriptors = 2;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(D3D12_Device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&D3D12_SamplerHeap)));
		NAME_D3D12_OBJ(D3D12_SamplerHeap);

		CreateRenderTargets();
	}

	void DirectXImpl::LoadAssets()
	{
		{
			CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc = {};
			rootSigDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error));
			ThrowIfFailed(D3D12_Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&D312_RootSig)));
			NAME_D3D12_OBJ(D312_RootSig);
		}

#ifdef MYGAME_USE_DXCOMPILER	
		ComPtr<IDxcBlob> vertexShader;
		ComPtr<IDxcBlob> pixelShader;

		MYGAME_HRESULT_VALIDATE(Shader::CompileVertexShader(&vertexShader, L"Assets/Shaders/ShaderVertex.hlsl"));
		MYGAME_HRESULT_VALIDATE(Shader::CompilePixelShader(&pixelShader, L"Assets/Shaders/ShaderPixel.hlsl"));
#else
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

		MYGAME_HRESULT_VALIDATE(Shader::D3CompileVertexShader(&vertexShader, L"Assets/Shaders/ShaderVertex.hlsl"));
		MYGAME_HRESULT_VALIDATE(Shader::D3CompilePixelShader(&pixelShader, L"Assets/Shaders/ShaderPixel.hlsl"));
#endif

		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputLayoutDesc, _countof(inputLayoutDesc) };
		psoDesc.pRootSignature = D312_RootSig;
		psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
		psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(D3D12_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&D312_PSO)));
		NAME_D3D12_OBJ(D312_PSO);

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