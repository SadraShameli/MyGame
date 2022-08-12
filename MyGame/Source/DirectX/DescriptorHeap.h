#pragma once

#include "DirectXHelpers.h"

#include <mutex>
#include <vector>
#include <queue>
#include <string>

namespace MyGame
{
	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type) : m_Type(Type), m_CurrentHeap(nullptr), m_DescriptorSize(0) { m_CurrentHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);
		static void DestroyAll(void);

	protected:
		static const uint32_t sm_NumDescriptorsPerHeap = 256;
		static std::mutex sm_AllocationMutex;
		static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool;
		static ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

		D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
		ID3D12DescriptorHeap* m_CurrentHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentHandle;
		uint32_t m_DescriptorSize;
		uint32_t m_RemainingFreeHandles;
	};

	class DescriptorHandle
	{
	public:
		DescriptorHandle()
		{
			m_CpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle) : m_CpuHandle(CpuHandle) { m_GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle) : m_CpuHandle(CpuHandle), m_GpuHandle(GpuHandle) {}

		DescriptorHandle operator+ (INT OffsetScaledByDescriptorSize)
		{
			DescriptorHandle ret = *this;
			ret += OffsetScaledByDescriptorSize;
			return ret;
		}

		void operator += (INT OffsetScaledByDescriptorSize)
		{
			if (m_CpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
				m_CpuHandle.ptr += OffsetScaledByDescriptorSize;
			if (m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
				m_GpuHandle.ptr += OffsetScaledByDescriptorSize;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE* operator&() { return &m_CpuHandle; }
		operator D3D12_CPU_DESCRIPTOR_HANDLE() { return m_CpuHandle; }
		operator D3D12_GPU_DESCRIPTOR_HANDLE() { return m_GpuHandle; }

		size_t GetCpuPtr() { return m_CpuHandle.ptr; }
		uint64_t GetGpuPtr() { return m_GpuHandle.ptr; }
		bool IsNull() { return m_CpuHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
		bool IsShaderVisible() { return m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
	};

	class DescriptorHeap
	{
	public:
		DescriptorHeap() {}
		~DescriptorHeap() { Destroy(); }

		void Create(const std::wstring& DebugHeapName, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount);
		void Destroy() { m_Heap = nullptr; }

		bool HasAvailableSpace(uint32_t Count) { return Count <= m_NumFreeDescriptors; }
		DescriptorHandle Alloc(uint32_t Count = 1);

		DescriptorHandle operator[] (uint32_t arrayIdx) { return m_FirstHandle + arrayIdx * m_DescriptorSize; }

		uint32_t GetOffsetOfHandle(DescriptorHandle& DHandle) { return (uint32_t)(DHandle.GetCpuPtr() - m_FirstHandle.GetCpuPtr()) / m_DescriptorSize; }
		bool ValidateHandle(DescriptorHandle& DHandle);

		ID3D12DescriptorHeap* GetHeapPointer() { return m_Heap.Get(); }
		uint32_t GetDescriptorSize() { return m_DescriptorSize; }

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
		D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
		uint32_t m_DescriptorSize;
		uint32_t m_NumFreeDescriptors;
		DescriptorHandle m_FirstHandle;
		DescriptorHandle m_NextFreeHandle;
	};
}