#include "CommonHeaders.h"

#include "Triangle.h"

#include "../../Core/Log.h"
#include "../../Debugs/Instrumentor.h"
#include "../../Debugs/DebugHelpers.h"

#include "../../DirectX/DirectXImpl.h"
#include "../../DirectX/CommandContext.h"
#include "../../DirectX/RootSignature.h"
#include "../../DirectX/PipelineState.h"
#include "../../Renderer/Shader.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	TriangleLayer::TriangleLayer() : Layer("Triangle") {}

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	void TriangleLayer::OnAttach()
	{
		// Define the geometry for a triangle.
		Vertex triangleVertices[] =
		{
			{ { 0.0f, 0.25f * 1.7777778, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f * 1.7777778, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f * 1.7777778, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		auto d1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto d2 = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

		ThrowIfFailed(D3D12_Device->CreateCommittedResource(
			&d1,
			D3D12_HEAP_FLAG_NONE,
			&d2,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	void TriangleLayer::OnDetach()
	{


	}

	void TriangleLayer::OnEvent(Event& e)
	{


	}

	void TriangleLayer::OnUpdate()
	{
		PopulateCommandList();

		// Execute the command list.
		ID3D12CommandList* ppCommandLists[] = { D3D12_CmdList };
		D3D12_CmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		// Present the frame.
		Present();

		WaitForGpu();
	}

	void TriangleLayer::PopulateCommandList()
	{
		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(D3D12_CmdAlloc->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(D3D12_CmdList->Reset(D3D12_CmdAlloc, D312_PSO));

		// Set necessary state.
		D3D12_CmdList->SetGraphicsRootSignature(D312_RootSig);
		D3D12_CmdList->RSSetViewports(1, &m_viewport);
		D3D12_CmdList->RSSetScissorRects(1, &m_scissorRect);

		// Indicate that the back buffer will be used as a render target.
		auto d1 = CD3DX12_RESOURCE_BARRIER::Transition(D3D12_RenderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		D3D12_CmdList->ResourceBarrier(1, &d1);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(D3D12_RtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
		D3D12_CmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		// Record commands.
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		D3D12_CmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		D3D12_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		D3D12_CmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		D3D12_CmdList->DrawInstanced(3, 1, 0, 0);

		// Indicate that the back buffer will now be used to present.
		auto d2 = CD3DX12_RESOURCE_BARRIER::Transition(D3D12_RenderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		D3D12_CmdList->ResourceBarrier(1, &d2);

		ThrowIfFailed(D3D12_CmdList->Close());
	}
}