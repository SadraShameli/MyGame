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
		MYGAME_ASSERT(ImGui_ImplDX12_Init(DirectXImpl::D3D12_Device, DirectXImpl::FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM,
			DirectXImpl::D3D12_SrvHeap, DirectXImpl::D3D12_SrvHeap->GetCPUDescriptorHandleForHeapStart(),
			DirectXImpl::D3D12_SrvHeap->GetGPUDescriptorHandleForHeapStart()));
	}

	void Renderer::RenderImGui()
	{
		UINT backBufferIdx = DirectXImpl::m_swapChain->GetCurrentBackBufferIndex();
		DirectXImpl::D3D12_CmdAlloc->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = DirectXImpl::D3D12_RenderTargets[backBufferIdx];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		D3D12_CmdList->Reset(DirectXImpl::D3D12_CmdAlloc, nullptr);
		D3D12_CmdList->ResourceBarrier(1, &barrier);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(DirectXImpl::D3D12_RtvHeap->GetCPUDescriptorHandleForHeapStart(), backBufferIdx, DirectXImpl::m_rtvDescriptorSize);

		constexpr XMFLOAT4 clear_color = { 0.2f, 0.2f, 0.2f, 1.00f };
		constexpr float clear_color_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		D3D12_CmdList->ClearRenderTargetView(rtvHandle, clear_color_alpha, 0, nullptr);
		D3D12_CmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		D3D12_CmdList->SetDescriptorHeaps(1, &DirectXImpl::D3D12_SrvHeap);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), D3D12_CmdList);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		D3D12_CmdList->ResourceBarrier(1, &barrier);
		ThrowIfFailed(D3D12_CmdList->Close());
		ID3D12CommandList* ppCommandLists[] = { D3D12_CmdList };
		DirectXImpl::D3D12_CmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		DirectXImpl::m_swapChain->Present(1, 0);

		const UINT64 fence = DirectXImpl::D3D12_FenceValue;
		ThrowIfFailed(DirectXImpl::D3D12_CmdQueue->Signal(DirectXImpl::D3D12_Fence, fence));
		++DirectXImpl::D3D12_FenceValue;

		//TODO remove
		if (DirectXImpl::D3D12_Fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(DirectXImpl::D3D12_Fence->SetEventOnCompletion(fence, DirectXImpl::D312_FenceEvent));
			WaitForSingleObject(DirectXImpl::D312_FenceEvent, INFINITE);
		}
	}

	void Renderer::OnWindowResize(const int width, const int height)
	{
		DirectXImpl::CleanupRenderTarget();
		MYGAME_HRESULT_TOSTR(DirectXImpl::m_swapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
		DirectXImpl::CreateRenderTargets();
	}
}