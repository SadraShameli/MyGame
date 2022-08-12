#include "CommonHeaders.h"

#include "ReadbackBuffer.h"
#include "DirectXImpl.h"

using namespace DirectX;

namespace MyGame
{
	void ReadbackBuffer::Create(std::wstring&& name, uint32_t NumElements, uint32_t ElementSize)
	{
		Destroy();

		m_ElementCount = NumElements;
		m_ElementSize = ElementSize;
		m_BufferSize = NumElements * ElementSize;
		m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ResourceDesc.Width = m_BufferSize;
		ResourceDesc.Height = 1;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(DirectXImpl::D12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource)));

		m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
		NAME_D3D12_OBJ_STR(m_pResource, name);
	}

	void* ReadbackBuffer::Map(void)
	{
		void* Memory;
		auto range = CD3DX12_RANGE(0, m_BufferSize);
		m_pResource->Map(0, &range, &Memory);
		return Memory;
	}

	void ReadbackBuffer::Unmap(void) { auto range = CD3DX12_RANGE(0, 0); m_pResource->Unmap(0, &range); }
}