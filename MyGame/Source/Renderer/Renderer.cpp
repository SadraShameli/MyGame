#include "CommonHeaders.h"

#include "Renderer.h"

#include "../Core/Application.h"
#include "../Core/Log.h"
#include "../Debugs/DebugHelpers.h"
#include "../DirectX/DirectXImpl.h"

#include <imgui_impl_dx12.h>

using namespace DirectX;

namespace MyGame
{
	void Renderer::Init()
	{
		DirectXImpl::OnInit();
	}

	void Renderer::InitImGui()
	{
		MYGAME_ASSERT(ImGui_ImplDX12_Init(DirectXImpl::D12Device, DirectXImpl::FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM,
			DirectXImpl::m_srvHeap, DirectXImpl::m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
			DirectXImpl::m_srvHeap->GetGPUDescriptorHandleForHeapStart()));
	}

	void Renderer::RenderImGui()
	{
		UINT backBufferIdx = DirectXImpl::m_swapChain->GetCurrentBackBufferIndex();
		DirectXImpl::m_commandAllocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = DirectXImpl::m_renderTargets[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		DirectXImpl::m_commandList->Reset(DirectXImpl::m_commandAllocator, nullptr);
		DirectXImpl::m_commandList->ResourceBarrier(1, &barrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(DirectXImpl::m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), backBufferIdx, DirectXImpl::m_rtvDescriptorSize);

		constexpr XMFLOAT4 clear_color = { 0.2f, 0.2f, 0.2f, 1.00f };
		constexpr float clear_color_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		DirectXImpl::m_commandList->ClearRenderTargetView(rtvHandle, clear_color_alpha, 0, nullptr);
		DirectXImpl::m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		DirectXImpl::m_commandList->SetDescriptorHeaps(1, &DirectXImpl::m_srvHeap);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), DirectXImpl::m_commandList);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		DirectXImpl::m_commandList->ResourceBarrier(1, &barrier);
		ThrowIfFailed(DirectXImpl::m_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { DirectXImpl::m_commandList };
		DirectXImpl::m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		DirectXImpl::m_swapChain->Present(1, 0);

		const UINT64 fence = DirectXImpl::m_fenceValue;
		ThrowIfFailed(DirectXImpl::m_commandQueue->Signal(DirectXImpl::m_fence, fence));
		++DirectXImpl::m_fenceValue;

		//TODO remove
		if (DirectXImpl::m_fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(DirectXImpl::m_fence->SetEventOnCompletion(fence, DirectXImpl::m_fenceEvent));
			WaitForSingleObject(DirectXImpl::m_fenceEvent, INFINITE);
		}
	}

	void Renderer::OnWindowResize(const int width, const int height)
	{
		DirectXImpl::CleanupRenderTarget();
		MYGAME_HRESULT_TOSTR(DirectXImpl::m_swapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
		DirectXImpl::CreateRenderTargets();
	}
}