#include "CommonHeaders.h"

#include "DynamicDescriptorHeap.h"
#include "CommandList.h"
#include "DirectXImpl.h"
#include "CommandContext.h"

using namespace DirectX;

namespace MyGame
{
	std::mutex DynamicDescriptorHeap::sm_Mutex;
	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DynamicDescriptorHeap::sm_DescriptorHeapPool[2];
	std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> DynamicDescriptorHeap::sm_RetiredDescriptorHeaps[2];
	std::queue<ID3D12DescriptorHeap*> DynamicDescriptorHeap::sm_AvailableDescriptorHeaps[2];

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

			ThrowIfFailed(DirectXImpl::m_device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&HeapPtr)));
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
			MYGAME_ASSERT(m_CurrentHeapPtr == nullptr);
			return;
		}

		MYGAME_ASSERT(m_CurrentHeapPtr != nullptr);
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
		m_DescriptorSize = DirectXImpl::m_device->GetDescriptorHandleIncrementSize(HeapType);
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
					DirectXImpl::m_device->CopyDescriptors(
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

		DirectXImpl::m_device->CopyDescriptors(
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
		DirectXImpl::m_device->CopyDescriptorsSimple(1, DestHandle, Handle, m_DescriptorType);
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
		MYGAME_ASSERT(RootSig.m_NumParameters <= 16, "Maybe we need to support something greater");

		m_StaleRootParamsBitMap = 0;
		m_RootDescriptorTablesBitMap = (Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
			RootSig.m_SamplerTableBitMap : RootSig.m_DescriptorTableBitMap);

		unsigned long TableParams = m_RootDescriptorTablesBitMap;
		unsigned long RootIndex = 0;
		while (_BitScanForward(&RootIndex, TableParams))
		{
			TableParams ^= (1 << RootIndex);

			UINT TableSize = RootSig.m_DescriptorTableSize[RootIndex];
			MYGAME_ASSERT(TableSize > 0);

			DescriptorTableCache& RootDescriptorTable = m_RootDescriptorTable[RootIndex];
			RootDescriptorTable.AssignedHandlesBitMap = 0;
			RootDescriptorTable.TableStart = m_HandleCache + CurrentOffset;
			RootDescriptorTable.TableSize = TableSize;

			CurrentOffset += TableSize;
		}

		m_MaxCachedDescriptors = CurrentOffset;
		MYGAME_ASSERT(m_MaxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
	}
}