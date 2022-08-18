#pragma once

#include "DirectXImpl.h"
#include "DirectXHelpers.h"

#include <mutex>
#include <vector>
#include <queue>
#include <string>

namespace MyGame
{
	// Descriptor Heap

	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type) : m_Type(Type), m_CurrentHeap(nullptr), m_DescriptorSize(0) { m_CurrentHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);
		static ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);
		static void DestroyAll(void);

	protected:
		static const uint32_t sm_NumDescriptorsPerHeap = 256;
		static std::mutex sm_AllocationMutex;
		static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool;

		D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
		ID3D12DescriptorHeap* m_CurrentHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentHandle;
		uint32_t m_DescriptorSize;
		uint32_t m_RemainingFreeHandles;
	};

	// Descriptor Handle

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

	public:
		static inline DescriptorAllocator DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
		{
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV
		};
		static inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1) { return DescriptorAllocators[Type].Allocate(Count); }

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
		D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
		uint32_t m_DescriptorSize;
		uint32_t m_NumFreeDescriptors;
		DescriptorHandle m_FirstHandle;
		DescriptorHandle m_NextFreeHandle;
	};

	// Dynamic Descriptor Heap

	class CommandContext;
	class RootSignature;
	class DynamicDescriptorHeap
	{
	public:
		DynamicDescriptorHeap(CommandContext& OwningContext, D3D12_DESCRIPTOR_HEAP_TYPE HeapType);

		static void DestroyAll()
		{
			sm_DescriptorHeapPool[0].clear();
			sm_DescriptorHeapPool[1].clear();
		}
		void CleanupUsedHeaps(uint64_t fenceValue);

		void SetGraphicsDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]) { m_GraphicsHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles); }
		void SetComputeDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]) { m_ComputeHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles); }

		D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handles);

		void ParseGraphicsRootSignature(const RootSignature& RootSig) { m_GraphicsHandleCache.ParseRootSignature(m_DescriptorType, RootSig); }
		void ParseComputeRootSignature(const RootSignature& RootSig) { m_ComputeHandleCache.ParseRootSignature(m_DescriptorType, RootSig); }

		inline void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
		{
			if (m_GraphicsHandleCache.m_StaleRootParamsBitMap != 0)
				CopyAndBindStagedTables(m_GraphicsHandleCache, CmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
		}
		inline void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* CmdList)
		{
			if (m_ComputeHandleCache.m_StaleRootParamsBitMap != 0)
				CopyAndBindStagedTables(m_ComputeHandleCache, CmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
		}

	private:
		static inline const uint32_t kNumDescriptorsPerHeap = 1024;
		static inline std::mutex sm_Mutex;
		static inline std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool[2];
		static inline std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> sm_RetiredDescriptorHeaps[2];
		static inline std::queue<ID3D12DescriptorHeap*> sm_AvailableDescriptorHeaps[2];

		static ID3D12DescriptorHeap* RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
		static void DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps);

		CommandContext& m_OwningContext;
		ID3D12DescriptorHeap* m_CurrentHeapPtr;
		const D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
		uint32_t m_DescriptorSize;
		uint32_t m_CurrentOffset;
		DescriptorHandle m_FirstDescriptor;
		std::vector<ID3D12DescriptorHeap*> m_RetiredHeaps;

		struct DescriptorTableCache
		{
			DescriptorTableCache() : AssignedHandlesBitMap(0) {}
			uint32_t AssignedHandlesBitMap;
			D3D12_CPU_DESCRIPTOR_HANDLE* TableStart;
			uint32_t TableSize;
		};

		struct DescriptorHandleCache
		{
			DescriptorHandleCache() { ClearCache(); }

			void ClearCache()
			{
				m_RootDescriptorTablesBitMap = 0;
				m_StaleRootParamsBitMap = 0;
				m_MaxCachedDescriptors = 0;
			}

			uint32_t m_RootDescriptorTablesBitMap;
			uint32_t m_StaleRootParamsBitMap;
			uint32_t m_MaxCachedDescriptors;

			static const uint32_t kMaxNumDescriptors = 256;
			static const uint32_t kMaxNumDescriptorTables = 16;

			uint32_t ComputeStagedSize();
			void CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, DescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
				void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

			DescriptorTableCache m_RootDescriptorTable[kMaxNumDescriptorTables];
			D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCache[kMaxNumDescriptors];

			void UnbindAllValid();
			void StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
			void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const RootSignature& RootSig);
		};

		DescriptorHandleCache m_GraphicsHandleCache;
		DescriptorHandleCache m_ComputeHandleCache;

		bool HasSpace(uint32_t Count) { return (m_CurrentHeapPtr != nullptr && m_CurrentOffset + Count <= kNumDescriptorsPerHeap); }
		void RetireCurrentHeap(void);
		void RetireUsedHeaps(uint64_t fenceValue);
		ID3D12DescriptorHeap* GetHeapPointer();

		DescriptorHandle Allocate(UINT Count)
		{
			DescriptorHandle ret = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
			m_CurrentOffset += Count;
			return ret;
		}

		void CopyAndBindStagedTables(DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
			void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

		void UnbindAllValid(void);
	};
}