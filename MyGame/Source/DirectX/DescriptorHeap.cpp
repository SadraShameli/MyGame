#include "CommonHeaders.h"

#include "DescriptorHeap.h"
#include "CommandContext.h"
#include "DirectXImpl.h"

#include "../Debugs/DebugHelpers.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	std::mutex DescriptorAllocator::sm_AllocationMutex;
	std::vector<ComPtr<ID3D12DescriptorHeap>> DescriptorAllocator::sm_DescriptorHeapPool;

	void DescriptorAllocator::DestroyAll(void) { sm_DescriptorHeapPool.clear(); }

	ID3D12DescriptorHeap* DescriptorAllocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
	{
		std::lock_guard<std::mutex> LockGuard(sm_AllocationMutex);

		D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
		Desc.Type = Type;
		Desc.NumDescriptors = sm_NumDescriptorsPerHeap;
		Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ComPtr<ID3D12DescriptorHeap> pHeap;
		ThrowIfFailed(DirectXImpl::D3D12_Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap)));
		sm_DescriptorHeapPool.emplace_back(pHeap);
		return pHeap.Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate(uint32_t Count)
	{
		if (m_CurrentHeap == nullptr || m_RemainingFreeHandles < Count)
		{
			m_CurrentHeap = RequestNewHeap(m_Type);
			m_CurrentHandle = m_CurrentHeap->GetCPUDescriptorHandleForHeapStart();
			m_RemainingFreeHandles = sm_NumDescriptorsPerHeap;

			if (m_DescriptorSize == 0)
				m_DescriptorSize = DirectXImpl::D3D12_Device->GetDescriptorHandleIncrementSize(m_Type);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE ret = m_CurrentHandle;
		m_CurrentHandle.ptr += Count * m_DescriptorSize;
		m_RemainingFreeHandles -= Count;
		return ret;
	}

	void DescriptorHeap::Create(const std::wstring& Name, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount)
	{
		m_HeapDesc.Type = Type;
		m_HeapDesc.NumDescriptors = MaxCount;
		m_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(DirectXImpl::D3D12_Device->CreateDescriptorHeap(&m_HeapDesc, IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf())));
		NAME_D3D12_OBJ_STR(m_Heap, Name);

		m_DescriptorSize = DirectXImpl::D3D12_Device->GetDescriptorHandleIncrementSize(m_HeapDesc.Type);
		m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
		m_FirstHandle = DescriptorHandle(m_Heap->GetCPUDescriptorHandleForHeapStart(), m_Heap->GetGPUDescriptorHandleForHeapStart());
		m_NextFreeHandle = m_FirstHandle;
	}

	// Descriptor Handle

	DescriptorHandle DescriptorHeap::Alloc(uint32_t Count)
	{
		MYGAME_ASSERT(HasAvailableSpace(Count), "Descriptor Heap out of space.  Increase heap size.");
		DescriptorHandle ret = m_NextFreeHandle;
		m_NextFreeHandle += Count * m_DescriptorSize;
		m_NumFreeDescriptors -= Count;
		return ret;
	}

	bool DescriptorHeap::ValidateHandle(DescriptorHandle& DHandle)
	{
		if (DHandle.GetCpuPtr() < m_FirstHandle.GetCpuPtr() || DHandle.GetCpuPtr() >= m_FirstHandle.GetCpuPtr() + m_HeapDesc.NumDescriptors * m_DescriptorSize)
			return false;

		if (DHandle.GetGpuPtr() - m_FirstHandle.GetGpuPtr() != DHandle.GetCpuPtr() - m_FirstHandle.GetCpuPtr())
			return false;
		return true;
	}

	// Dynamic Descriptor Heap

	ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
	{
		std::lock_guard<std::mutex> LockGuard(sm_Mutex);
		uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

		while (!sm_RetiredDescriptorHeaps[idx].empty() && CommandListManager::IsFenceComplete(sm_RetiredDescriptorHeaps[idx].front().first))
		{
			sm_AvailableDescriptorHeaps[idx].push(sm_RetiredDescriptorHeaps[idx].front().second);
			sm_RetiredDescriptorHeaps[idx].pop();
		}

		if (!sm_AvailableDescriptorHeaps[idx].empty())
		{
			ID3D12DescriptorHeap* HeapPtr = sm_AvailableDescriptorHeaps[idx].front();
			sm_AvailableDescriptorHeaps[idx].pop();
			return HeapPtr;
		}
		else
		{
			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type = HeapType;
			HeapDesc.NumDescriptors = kNumDescriptorsPerHeap;
			HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> HeapPtr;

			ThrowIfFailed(DirectXImpl::D3D12_Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&HeapPtr)));
			sm_DescriptorHeapPool[idx].emplace_back(HeapPtr);
			return HeapPtr.Get();
		}
	}

	void DynamicDescriptorHeap::DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValue, const std::vector<ID3D12DescriptorHeap*>& UsedHeaps)
	{
		uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;
		std::lock_guard<std::mutex> LockGuard(sm_Mutex);
		for (auto iter = UsedHeaps.begin(); iter != UsedHeaps.end(); ++iter)
			sm_RetiredDescriptorHeaps[idx].push(std::make_pair(FenceValue, *iter));
	}

	void DynamicDescriptorHeap::RetireCurrentHeap(void)
	{
		if (m_CurrentOffset == 0)
		{
			MYGAME_ASSERT(!m_CurrentHeapPtr);
			return;
		}

		m_RetiredHeaps.push_back(m_CurrentHeapPtr);
		m_CurrentHeapPtr = nullptr;
		m_CurrentOffset = 0;
	}

	void DynamicDescriptorHeap::RetireUsedHeaps(uint64_t fenceValue)
	{
		DiscardDescriptorHeaps(m_DescriptorType, fenceValue, m_RetiredHeaps);
		m_RetiredHeaps.clear();
	}

	DynamicDescriptorHeap::DynamicDescriptorHeap(CommandContext& OwningContext, D3D12_DESCRIPTOR_HEAP_TYPE HeapType) : m_OwningContext(OwningContext), m_DescriptorType(HeapType)
	{
		m_CurrentHeapPtr = nullptr;
		m_CurrentOffset = 0;
		m_DescriptorSize = DirectXImpl::D3D12_Device->GetDescriptorHandleIncrementSize(HeapType);
	}

	void DynamicDescriptorHeap::CleanupUsedHeaps(uint64_t fenceValue)
	{
		RetireCurrentHeap();
		RetireUsedHeaps(fenceValue);
		m_GraphicsHandleCache.ClearCache();
		m_ComputeHandleCache.ClearCache();
	}

	inline ID3D12DescriptorHeap* DynamicDescriptorHeap::GetHeapPointer()
	{
		if (m_CurrentHeapPtr == nullptr)
		{
			MYGAME_ASSERT(m_CurrentOffset == 0);
			m_CurrentHeapPtr = RequestDescriptorHeap(m_DescriptorType);
			m_FirstDescriptor = DescriptorHandle(
				m_CurrentHeapPtr->GetCPUDescriptorHandleForHeapStart(),
				m_CurrentHeapPtr->GetGPUDescriptorHandleForHeapStart());
		}

		return m_CurrentHeapPtr;
	}

	uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize()
	{
		uint32_t NeededSpace = 0;
		uint32_t RootIndex = 0;
		uint32_t StaleParams = m_StaleRootParamsBitMap;

		while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
		{
			StaleParams ^= (1 << RootIndex);

			uint32_t MaxSetHandle = 0;
			MYGAME_ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
				"Root entry marked as stale but has no stale descriptors");
			NeededSpace += MaxSetHandle + 1;
		}
		return NeededSpace;
	}

	void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(
		D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize,
		DescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
	{
		uint32_t StaleParamCount = 0;
		uint32_t TableSize[DescriptorHandleCache::kMaxNumDescriptorTables] = {};
		uint32_t RootIndices[DescriptorHandleCache::kMaxNumDescriptorTables] = {};
		uint32_t NeededSpace = 0;
		uint32_t RootIndex = 0;

		uint32_t StaleParams = m_StaleRootParamsBitMap;
		while (_BitScanForward((unsigned long*)&RootIndex, StaleParams))
		{
			RootIndices[StaleParamCount] = RootIndex;
			StaleParams ^= (1 << RootIndex);

			uint32_t MaxSetHandle = 0;
			MYGAME_ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
				"Root entry marked as stale but has no stale descriptors");

			NeededSpace += MaxSetHandle + 1;
			TableSize[StaleParamCount] = MaxSetHandle + 1;

			++StaleParamCount;
		}

		MYGAME_ASSERT(StaleParamCount <= DescriptorHandleCache::kMaxNumDescriptorTables,
			"We're only equipped to handle so many descriptor tables");

		m_StaleRootParamsBitMap = 0;

		static const uint32_t kMaxDescriptorsPerCopy = 16;
		UINT NumDestDescriptorRanges = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy] = {};
		UINT pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy] = {};

		UINT NumSrcDescriptorRanges = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy] = {};
		UINT pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy] = {};

		for (uint32_t i = 0; i < StaleParamCount; ++i)
		{
			RootIndex = RootIndices[i];
			(CmdList->*SetFunc)(RootIndex, DestHandleStart);

			DescriptorTableCache& RootDescTable = m_RootDescriptorTable[RootIndex];

			D3D12_CPU_DESCRIPTOR_HANDLE* SrcHandles = RootDescTable.TableStart;
			uint64_t SetHandles = (uint64_t)RootDescTable.AssignedHandlesBitMap;
			D3D12_CPU_DESCRIPTOR_HANDLE CurDest = DestHandleStart;
			DestHandleStart += TableSize[i] * DescriptorSize;

			unsigned long SkipCount;
			while (_BitScanForward64(&SkipCount, SetHandles))
			{
				SetHandles >>= SkipCount;
				SrcHandles += SkipCount;
				CurDest.ptr += SkipCount * DescriptorSize;

				unsigned long DescriptorCount;
				_BitScanForward64(&DescriptorCount, ~SetHandles);
				SetHandles >>= DescriptorCount;

				if (NumSrcDescriptorRanges + DescriptorCount > kMaxDescriptorsPerCopy)
				{
					DirectXImpl::D3D12_Device->CopyDescriptors(
						NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
						NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, Type);

					NumSrcDescriptorRanges = 0;
					NumDestDescriptorRanges = 0;
				}

				pDestDescriptorRangeStarts[NumDestDescriptorRanges] = CurDest;
				pDestDescriptorRangeSizes[NumDestDescriptorRanges] = DescriptorCount;
				++NumDestDescriptorRanges;

				for (uint32_t j = 0; j < DescriptorCount; ++j)
				{
					pSrcDescriptorRangeStarts[NumSrcDescriptorRanges] = SrcHandles[j];
					pSrcDescriptorRangeSizes[NumSrcDescriptorRanges] = 1;
					++NumSrcDescriptorRanges;
				}

				SrcHandles += DescriptorCount;
				CurDest.ptr += DescriptorCount * DescriptorSize;
			}
		}

		DirectXImpl::D3D12_Device->CopyDescriptors(
			NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
			NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, Type);
	}

	void DynamicDescriptorHeap::CopyAndBindStagedTables(DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
	{
		uint32_t NeededSize = HandleCache.ComputeStagedSize();
		if (!HasSpace(NeededSize))
		{
			RetireCurrentHeap();
			UnbindAllValid();
			NeededSize = HandleCache.ComputeStagedSize();
		}

		m_OwningContext.SetDescriptorHeap(m_DescriptorType, GetHeapPointer());
		HandleCache.CopyAndBindStaleTables(m_DescriptorType, m_DescriptorSize, Allocate(NeededSize), CmdList, SetFunc);
	}

	void DynamicDescriptorHeap::UnbindAllValid(void)
	{
		m_GraphicsHandleCache.UnbindAllValid();
		m_ComputeHandleCache.UnbindAllValid();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
	{
		if (!HasSpace(1))
		{
			RetireCurrentHeap();
			UnbindAllValid();
		}

		m_OwningContext.SetDescriptorHeap(m_DescriptorType, GetHeapPointer());
		DescriptorHandle DestHandle = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
		m_CurrentOffset += 1;
		DirectXImpl::D3D12_Device->CopyDescriptorsSimple(1, DestHandle, Handle, m_DescriptorType);
		return DestHandle;
	}

	void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
	{
		m_StaleRootParamsBitMap = 0;

		unsigned long TableParams = m_RootDescriptorTablesBitMap;
		unsigned long RootIndex = 0;
		while (_BitScanForward(&RootIndex, TableParams))
		{
			TableParams ^= (1 << RootIndex);
			if (m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap != 0)
				m_StaleRootParamsBitMap |= (1 << RootIndex);
		}
	}

	void DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
	{
		MYGAME_ASSERT(((1 << RootIndex) & m_RootDescriptorTablesBitMap) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table");
		MYGAME_ASSERT(Offset + NumHandles <= m_RootDescriptorTable[RootIndex].TableSize);

		DescriptorTableCache& TableCache = m_RootDescriptorTable[RootIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE* CopyDest = TableCache.TableStart + Offset;
		for (UINT i = 0; i < NumHandles; ++i)
			CopyDest[i] = Handles[i];
		TableCache.AssignedHandlesBitMap |= ((1 << NumHandles) - 1) << Offset;
		m_StaleRootParamsBitMap |= (1 << RootIndex);
	}

	void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const RootSignature& RootSig)
	{
		UINT CurrentOffset = 0;
		m_StaleRootParamsBitMap = 0;

		unsigned long TableParams = m_RootDescriptorTablesBitMap;
		unsigned long RootIndex = 0;
		while (_BitScanForward(&RootIndex, TableParams))
		{
			TableParams ^= (1 << RootIndex);

			DescriptorTableCache& RootDescriptorTable = m_RootDescriptorTable[RootIndex];
			RootDescriptorTable.AssignedHandlesBitMap = 0;
			RootDescriptorTable.TableStart = m_HandleCache + CurrentOffset;
		}

		m_MaxCachedDescriptors = CurrentOffset;
		MYGAME_ASSERT(m_MaxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
	}
}