#include "CommonHeaders.h"

#include "DirectXImpl.h"
#include "../Core/Application.h"
#include "../Debugs/DebugHelpers.h"

// ImGui
#include <backends/imgui_impl_dx12.h>

// Math
#include <glm/glm.hpp>

// Temp
#include "Shader.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	// TODO remove these
	void DirectXImpl::WaitForLastSubmittedFrame()
	{
		FrameContext* frameCtx = &m_frameContext[m_frameIndex % FrameCount];

		UINT64 fenceValue = frameCtx->FenceValue;
		if (fenceValue == 0)
			return; // No fence was signaled

		frameCtx->FenceValue = 0;
		if (m_fence->GetCompletedValue() >= fenceValue)
			return;

		m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	DirectXImpl::FrameContext* DirectXImpl::WaitForNextFrameResources()
	{
		UINT nextFrameIndex = m_frameIndex + 1;
		m_frameIndex = nextFrameIndex;

		HANDLE waitableObjects[] = { m_swapChainWaitableObject, nullptr };
		DWORD numWaitableObjects = 1;

		FrameContext* frameCtx = &m_frameContext[nextFrameIndex % FrameCount];
		UINT64 fenceValue = frameCtx->FenceValue;
		if (fenceValue != 0) // means no fence was signaled
		{
			frameCtx->FenceValue = 0;
			m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
			waitableObjects[1] = m_fenceEvent;
			numWaitableObjects = 2;
		}

		WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

		return frameCtx;
	}
	///////////////////////////////////

	// Universal functions
	void DirectXImpl::InitImGui()
	{
		MYGAME_ASSERT(ImGui_ImplDX12_Init(m_device.Get(), FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM, m_srvHeap.Get(), m_srvHeap->GetCPUDescriptorHandleForHeapStart(), m_srvHeap->GetGPUDescriptorHandleForHeapStart()));
	}

	void DirectXImpl::RenderImGui()
	{
		ImGui::Render();

		UINT backBufferIdx = m_swapChain->GetCurrentBackBufferIndex();
		FrameContext* frameCtx = WaitForNextFrameResources();
		frameCtx->CommandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = m_renderTargets[backBufferIdx].Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		m_commandList->Reset(frameCtx->CommandAllocator.Get(), nullptr);
		m_commandList->ResourceBarrier(1, &barrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), backBufferIdx, m_rtvDescriptorSize);

		constexpr glm::vec4 clear_color = { 0.2f, 0.2f, 0.2f, 1.00f };
		constexpr float clear_color_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		m_commandList->ClearRenderTargetView(rtvHandle, clear_color_alpha, 0, nullptr);
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		m_commandList->SetDescriptorHeaps(1, m_srvHeap.GetAddressOf());

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		m_commandList->ResourceBarrier(1, &barrier);
		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Setting VSync
		if (application.GetWindow().IsVSync())
			m_swapChain->Present(1, 0); // VSync On
		else
			m_swapChain->Present(0, 0); // VSync Off

		UINT64 fenceValue = m_fenceValue + 1;
		m_commandQueue->Signal(m_fence.Get(), fenceValue);
		m_fenceValue = fenceValue;
		frameCtx->FenceValue = fenceValue;
	}

	void DirectXImpl::OnWindowResize(const int width, const int height)
	{
		CleanupRenderTarget();
		MYGAME_HRESULT_TOSTR(m_swapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
		CreateRenderTargets();
	}

	// DirectX 12 functions
	void DirectXImpl::OnInit()
	{
		LoadPipeline();
		LoadAssets();
	}

	void DirectXImpl::OnDestroy()
	{
		// Ensure that the GPU is no longer referencing resources that are about to be cleaned up by the destructor.
		const UINT64 fence = m_fenceValue;
		const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

		// Signal and increment the fence value.
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
		m_fenceValue++;

		// Wait until the previous frame is finished.
		if (lastCompletedFence < fence)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
		CloseHandle(m_fenceEvent);

		for (int i = 0; i < NumContexts; ++i)
		{
			CloseHandle(m_workerBeginRenderFrame[i]);
			CloseHandle(m_workerFinishShadowPass[i]);
			CloseHandle(m_workerFinishedRenderFrame[i]);
			CloseHandle(m_threadHandles[i]);
		}
	}

	void DirectXImpl::LoadPipeline()
	{

#ifdef MYGAME_DEBUG 
		{
			// Enable the DirectX 12 debug layer.
			ComPtr<ID3D12Debug> debugController;
			ComPtr<ID3D12Debug1> debugController1;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
			{
				debugController->EnableDebugLayer();
				debugController->QueryInterface(IID_PPV_ARGS(debugController1.GetAddressOf()));
				debugController1->SetEnableGPUBasedValidation(TRUE);
				debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE);
			}
		}
#endif	

		// Temporary workaround
		UUID experimentalFeatures[] = { D3D12ExperimentalShaderModels };
		MYGAME_HRESULT_TOSTR(D3D12EnableExperimentalFeatures(1, experimentalFeatures, nullptr, nullptr));

		// Create DXGI Factory
		ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
		if (m_useWarpDevice)
		{
			ComPtr<IDXGIAdapter> warpAdapter;
			ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
			ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));
		}
		else
		{
			ComPtr<IDXGIAdapter1> hardwareAdapter;
			GetHardwareAdapter(factory.Get(), &hardwareAdapter);
			ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)));
		}

#ifdef MYGAME_DEBUG
		ComPtr<ID3D12InfoQueue> pInfoQueue;
		m_device->QueryInterface(IID_PPV_ARGS(pInfoQueue.GetAddressOf()));
		//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		//pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
#endif

		// Describe and create the command queue.
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
		NAME_D3D12_OBJECT(m_commandQueue);

		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.Width = 0;
		swapChainDesc.Height = 0;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.Stereo = false;

		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(m_commandQueue.Get(), application.GetWin32Window(), &swapChainDesc, nullptr, nullptr, &swapChain));
		ThrowIfFailed(swapChain.As(&m_swapChain));
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
		m_swapChain->SetMaximumFrameLatency(FrameCount);

		{
			// Create descriptor heaps.
			// Describe and create a render target view (RTV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		{
			// Describe and create a render target view (SRV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.NumDescriptors = 1;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));
		}

		{
			// Describe and create a depth stencil view (DSV) descriptor heap.
			// Each frame has its own depth stencils (to write shadows onto) 
			// and then there is one for the scene itself.
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.NumDescriptors = 1 + FrameCount;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
		}

		{
			// Describe and create a sampler descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
			samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			samplerHeapDesc.NumDescriptors = 2; // One clamp and one wrap sampler.
			samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap)));
			NAME_D3D12_OBJECT(m_samplerHeap);
		}

		// Create render target views (RTVs).
		CreateRenderTargets();

		for (auto& frame : m_frameContext)
			ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.CommandAllocator)));

		// Create the command list.
		// Command lists are created in the recording state, but there is nothing to record yet. 
		//The main loop expects it to be closed, so close it now.
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frameContext[0].CommandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
		ThrowIfFailed(m_commandList->Close());

		// Create synchronization objects
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		++m_fenceValue;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr) { ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError())); }
	}

	void DirectXImpl::LoadAssets()
	{
		// Create root signature
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 ranges[4]; // Perfomance TIP: Order from most frequent to least frequent.
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 2 frequently changed diffuse + normal textures - using registers t1 and t2.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);    // 1 frequently changed constant buffer.
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);                                                // 1 infrequently changed shadow texture - starting in register t0.
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);

		CD3DX12_ROOT_PARAMETER1 rootParams[4];
		rootParams[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParams[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParams[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParams[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
		rootSigDesc.Init_1_1(_countof(rootParams), rootParams, 0, nullptr, rootSigFlags);

		// Serialize the root signature.
		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSigDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		NAME_D3D12_OBJECT(m_rootSignature);


#ifdef MYGAME_USE_DXCOMPILER		
		//Compile shaders with Dx Compiler
		ComPtr<IDxcBlob> vertexShader;
		ComPtr<IDxcBlob> pixelShader;
		if (!Shader::CompileVertexShader(vertexShader, "Assets/Shaders/ShaderVertex.hlsl") ||
			!Shader::CompilePixelShader(pixelShader, "Assets/Shaders/ShaderPixel.hlsl")) return;
#else
		//Compile shaders with D3D Compiler
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		if (!Shader::D3CompileVertexShader(vertexShader, "Assets/Shaders/ShaderVertex.hlsl") ||
			!Shader::D3CompilePixelShader(pixelShader, "Assets/Shaders/ShaderPixel.hlsl")) return;
#endif

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = FALSE;

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputLayoutDesc, _countof(inputLayoutDesc) };
		psoDesc.pRootSignature = m_rootSignature.Get();

#ifdef MYGAME_USE_DXCOMPILER
		psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
		psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
#else
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
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

		// Create the pipeline state.
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
		NAME_D3D12_OBJECT(m_pipelineState);

		// Alter the description and create the PSO for rendering
		// the shadow map.  The shadow map does not use a pixel
		// shader or render targets.
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(0, 0);
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;
	}

	void DirectXImpl::CreateRenderTargets()
	{
		// Create render target views (RTVs).
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < FrameCount; ++i)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
			m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			NAME_D3D12_OBJECT_INDEXED(m_renderTargets, i);
		}
	}

	void DirectXImpl::CleanupRenderTarget()
	{
		WaitForLastSubmittedFrame();

		for (auto& renderTarget : m_renderTargets) renderTarget.Reset();
	}

	void DirectXImpl::WaitForGpu()
	{
		// Wait for pending GPU work to complete.
		// Schedule a Signal command in the queue.
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

		// Wait until the fence has been processed.
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	void DirectXImpl::GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
	{
		*ppAdapter = nullptr;
		for (UINT adapterIndex = 0; ; ++adapterIndex)
		{
			IDXGIAdapter1* pAdapter = nullptr;
			if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter)) break;

			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				*ppAdapter = pAdapter;
				return;
			}
			pAdapter->Release();
		}
	}

	DirectXImpl DirectX;
}
