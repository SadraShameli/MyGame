#pragma once

#include "BufferHelper.h"
#include "DirectXHelpers.h"

namespace MyGame
{
	class GpuBuffer : public GpuResource
	{
	public:
		virtual ~GpuBuffer() { Destroy(); }

		void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const void* initialData = nullptr);
		void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const UploadBuffer& srcData, uint32_t srcOffset = 0);
		void CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize, const void* initialData = nullptr);

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const { return m_UAV; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_SRV; }

		D3D12_GPU_VIRTUAL_ADDRESS RootConstantBufferView() const { return m_GpuVirtualAddress; }

		D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView(uint32_t Offset, uint32_t Size) const;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const;
		D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t BaseVertexIndex = 0) const
		{
			size_t Offset = BaseVertexIndex * m_ElementSize;
			return VertexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize);
		}

		D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const;
		D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t StartIndex = 0) const
		{
			size_t Offset = StartIndex * m_ElementSize;
			return IndexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize == 4);
		}

		size_t GetBufferSize() const { return m_BufferSize; }
		uint32_t GetElementCount() const { return m_ElementCount; }
		uint32_t GetElementSize() const { return m_ElementSize; }

	protected:
		GpuBuffer() : m_BufferSize(0), m_ElementCount(0), m_ElementSize(0)
		{
			m_ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		D3D12_RESOURCE_DESC DescribeBuffer();
		virtual void CreateDerivedViews() = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE m_UAV;
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRV;

		size_t m_BufferSize;
		uint32_t m_ElementCount;
		uint32_t m_ElementSize;
		D3D12_RESOURCE_FLAGS m_ResourceFlags;
	};

	inline D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
	{
		D3D12_VERTEX_BUFFER_VIEW VBView = {};
		VBView.BufferLocation = m_GpuVirtualAddress + Offset;
		VBView.SizeInBytes = Size;
		VBView.StrideInBytes = Stride;
		return VBView;
	}

	inline D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit) const
	{
		D3D12_INDEX_BUFFER_VIEW IBView;
		IBView.BufferLocation = m_GpuVirtualAddress + Offset;
		IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		IBView.SizeInBytes = Size;
		return IBView;
	}

	class ByteAddressBuffer : public GpuBuffer
	{
	public:
		virtual void CreateDerivedViews() override;
	};

	class IndirectArgsBuffer : public ByteAddressBuffer
	{
	public:
		IndirectArgsBuffer() {}
	};

	class StructuredBuffer : public GpuBuffer
	{
	public:
		virtual void Destroy() override
		{
			m_CounterBuffer.Destroy();
			GpuBuffer::Destroy();
		}

		virtual void CreateDerivedViews() override;
		ByteAddressBuffer& GetCounterBuffer() { return m_CounterBuffer; }

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterSRV(CommandContext& Context);
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterUAV(CommandContext& Context);

	private:
		ByteAddressBuffer m_CounterBuffer;
	};

	class TypedBuffer : public GpuBuffer
	{
	public:
		TypedBuffer(DXGI_FORMAT Format) : m_DataFormat(Format) {}
		virtual void CreateDerivedViews() override;

	protected:
		DXGI_FORMAT m_DataFormat;
	};

	class ReadbackBuffer : public GpuBuffer
	{
	public:
		virtual ~ReadbackBuffer() { Destroy(); }

		void Create(std::wstring&& name, uint32_t NumElements, uint32_t ElementSize);

		void* Map();
		void Unmap();

	protected:
		void CreateDerivedViews() {}
	};
}