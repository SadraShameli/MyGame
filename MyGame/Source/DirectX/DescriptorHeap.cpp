#include "CommonHeaders.h"

#include "DescriptorHeap.h"
#include "CommandContext.h"
#include "DirectXImpl.h"

#include "../Debugs/DebugHelpers.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	// DescriptorAllocator

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate(uint32_t count)
	{
		if (!m_CurrentHeap || m_RemainingFreeHandles < count)
		{
			m_CurrentHeap = RequestNewHeap(m_Type);
			m_CurrentHandle = m_CurrentHeap->GetCPUDescriptorHandleForHeapStart();
			m_RemainingFreeHandles = s_NumDescriptorsPerHeap;

			if (!m_DescriptorSize)
			{
				m_DescriptorSize = DirectXImpl::Device->GetDescriptorHandleIncrementSize(m_Type);
			}
		}

		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_CurrentHandle;
		m_CurrentHandle.ptr += static_cast<size_t>(count) * m_DescriptorSize;
		m_RemainingFreeHandles -= count;
		return handle;
	}

	ID3D12DescriptorHeap* DescriptorAllocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
	{
		std::lock_guard<std::mutex> LockGuard(s_Mutex);

		D3D12_DESCRIPTOR_HEAP_DESC desc = {
			.Type = type,
			.NumDescriptors = s_NumDescriptorsPerHeap,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		};

		ComPtr<ID3D12DescriptorHeap> heap;
		ThrowIfFailed(DirectXImpl::Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
		s_HeapPool.emplace_back(heap);
		return heap.Get();
	}

	void DescriptorAllocator::DestroyAll()
	{
		s_HeapPool.clear();
	}

	// DescriptorHeap

	DescriptorHandle DescriptorHeap::Allocate(uint32_t count)
	{
		MYGAME_ASSERT(HasAvailableSpace(count), "Descriptor Heap out of space. Increase heap size.");
		DescriptorHandle handle = m_NextFreeHandle;
		m_NextFreeHandle += static_cast<size_t>(count) * m_DescriptorSize;
		m_NumFreeDescriptors -= count;
		return handle;
	}

	void DescriptorHeap::Create(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxCount)
	{
		m_HeapDesc.Type = type;
		m_HeapDesc.NumDescriptors = maxCount;
		m_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		m_HeapDesc.NodeMask = 1;

		ThrowIfFailed(DirectXImpl::Device->CreateDescriptorHeap(&m_HeapDesc, IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf())));
		NAME_D3D12_OBJ_STR(m_Heap, name);

		m_DescriptorSize = DirectXImpl::Device->GetDescriptorHandleIncrementSize(m_HeapDesc.Type);
		m_FirstHandle = DescriptorHandle(m_Heap->GetCPUDescriptorHandleForHeapStart(), m_Heap->GetGPUDescriptorHandleForHeapStart());
		m_NextFreeHandle = m_FirstHandle;
		m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
	}

	bool DescriptorHeap::ValidateHandle(const DescriptorHandle& handle)
	{
		if (handle.GetCpuPtr() < m_FirstHandle.GetCpuPtr() || handle.GetCpuPtr() >= m_FirstHandle.GetCpuPtr() + static_cast<uint64_t>(m_HeapDesc.NumDescriptors) * m_DescriptorSize)
			return false;
		if (handle.GetGpuPtr() - m_FirstHandle.GetGpuPtr() != handle.GetCpuPtr() - m_FirstHandle.GetCpuPtr())
			return false;
		return true;
	}

	// Dynamic Descriptor Heap

	DynamicDescriptorHeap::DynamicDescriptorHeap(CommandContext& ctx, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
		: m_OwningContext(ctx), m_DescriptorType(heapType)
	{
		m_CurrentHeapPtr = nullptr;
		m_CurrentOffset = 0;
		m_DescriptorSize = DirectXImpl::Device->GetDescriptorHandleIncrementSize(heapType);
	}

	void DynamicDescriptorHeap::DestroyAll()
	{
		for (auto& heap : s_HeapPool)
			heap.clear();
	}

	ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	{
		std::lock_guard<std::mutex> LockGuard(m_Mutex);
		uint32_t idx = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

		while (!s_RetiredHeaps[idx].empty() && CommandListManager::IsFenceComplete(s_RetiredHeaps[idx].front().first))
		{
			s_AvailableHeaps[idx].push(s_RetiredHeaps[idx].front().second);
			s_RetiredHeaps[idx].pop();
		}

		if (!s_AvailableHeaps[idx].empty())
		{
			ID3D12DescriptorHeap* ptr = s_AvailableHeaps[idx].front();
			s_AvailableHeaps[idx].pop();
			return ptr;
		}
		else
		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {
				.Type = heapType,
				.NumDescriptors = s_NumDescriptorsPerHeap,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				.NodeMask = 1
			};

			ComPtr<ID3D12DescriptorHeap> ptr;
			ThrowIfFailed(DirectXImpl::Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&ptr)));
			s_HeapPool[idx].emplace_back(ptr);
			return ptr.Get();
		}
	}

	void DynamicDescriptorHeap::DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint64_t fenceValue, const std::vector<ID3D12DescriptorHeap*>& usedHeaps)
	{
		std::lock_guard<std::mutex> LockGuard(m_Mutex);
		uint32_t idx = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

		for (auto iter = usedHeaps.begin(); iter != usedHeaps.end(); ++iter)
			s_RetiredHeaps[idx].push(std::make_pair(fenceValue, *iter));
	}

	void DynamicDescriptorHeap::RetireCurrentHeap(void)
	{
		if (!m_CurrentOffset)
		{
			MYGAME_ASSERT(!m_CurrentHeapPtr);
			return;
		}

		MYGAME_ASSERT(m_CurrentHeapPtr);
		m_RetiredHeaps.emplace_back(m_CurrentHeapPtr);
		m_CurrentHeapPtr = nullptr;
		m_CurrentOffset = 0;
	}

	void DynamicDescriptorHeap::RetireUsedHeaps(uint64_t fenceValue)
	{
		DiscardDescriptorHeaps(m_DescriptorType, fenceValue, m_RetiredHeaps);
		m_RetiredHeaps.clear();
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
		uint32_t rootIndex = 0;
		uint32_t StaleParams = m_StaleRootParamsBitMap;

		while (_BitScanForward((unsigned long*)&rootIndex, StaleParams))
		{
			StaleParams ^= (1 << rootIndex);

			uint32_t MaxSetHandle = 0;
			MYGAME_ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_RootDescriptorTable[rootIndex].AssignedHandlesBitMap),
				"Root entry marked as stale but has no stale descriptors");
			NeededSpace += MaxSetHandle + 1;
		}
		return NeededSpace;
	}

	void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(
		D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t DescriptorSize,
		DescriptorHandle DestHandleStart, ID3D12GraphicsCommandList* CmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE))
	{
		uint32_t StaleParamCount = 0;
		uint32_t TableSize[DescriptorHandleCache::s_MaxNumDescriptorTables] = {};
		uint32_t RootIndices[DescriptorHandleCache::s_MaxNumDescriptorTables] = {};
		uint32_t NeededSpace = 0;
		uint32_t rootIndex = 0;

		uint32_t StaleParams = m_StaleRootParamsBitMap;
		while (_BitScanForward((unsigned long*)&rootIndex, StaleParams))
		{
			RootIndices[StaleParamCount] = rootIndex;
			StaleParams ^= (1 << rootIndex);

			uint32_t MaxSetHandle = 0;
			MYGAME_ASSERT(TRUE == _BitScanReverse((unsigned long*)&MaxSetHandle, m_RootDescriptorTable[rootIndex].AssignedHandlesBitMap),
				"Root entry marked as stale but has no stale descriptors");

			NeededSpace += MaxSetHandle + 1;
			TableSize[StaleParamCount] = MaxSetHandle + 1;

			++StaleParamCount;
		}

		MYGAME_ASSERT(StaleParamCount <= DescriptorHandleCache::s_MaxNumDescriptorTables,
			"We're only equipped to handle so many descriptor tables");

		m_StaleRootParamsBitMap = 0;

		static const uint32_t kMaxDescriptorsPerCopy = 16;
		uint32_t NumDestDescriptorRanges = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy] = {};
		uint32_t pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy] = {};

		uint32_t NumSrcDescriptorRanges = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy] = {};
		uint32_t pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy] = {};

		for (uint32_t i = 0; i < StaleParamCount; ++i)
		{
			rootIndex = RootIndices[i];
			(CmdList->*SetFunc)(rootIndex, DestHandleStart);

			DescriptorTableCache& RootDescTable = m_RootDescriptorTable[rootIndex];

			D3D12_CPU_DESCRIPTOR_HANDLE* SrcHandles = RootDescTable.TableStart;
			uint64_t SetHandles = (uint64_t)RootDescTable.AssignedHandlesBitMap;
			D3D12_CPU_DESCRIPTOR_HANDLE CurDest = DestHandleStart;
			DestHandleStart += TableSize[i] * DescriptorSize;

			DWORD SkipCount;
			while (_BitScanForward64(&SkipCount, SetHandles))
			{
				SetHandles >>= SkipCount;
				SrcHandles += SkipCount;
				CurDest.ptr += SkipCount * DescriptorSize;

				DWORD DescriptorCount;
				_BitScanForward64(&DescriptorCount, ~SetHandles);
				SetHandles >>= DescriptorCount;

				if (NumSrcDescriptorRanges + DescriptorCount > kMaxDescriptorsPerCopy)
				{
					DirectXImpl::Device->CopyDescriptors(
						NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
						NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, type);

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

		DirectXImpl::Device->CopyDescriptors(
			NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
			NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, type);
	}

	void DynamicDescriptorHeap::CopyAndBindStagedTables(DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* CmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE))
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
		DirectXImpl::Device->CopyDescriptorsSimple(1, DestHandle, Handle, m_DescriptorType);
		return DestHandle;
	}

	void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
	{
		m_StaleRootParamsBitMap = 0;

		unsigned long TableParams = m_RootDescriptorTablesBitMap;
		unsigned long rootIndex = 0;
		while (_BitScanForward(&rootIndex, TableParams))
		{
			TableParams ^= (1 << rootIndex);
			if (m_RootDescriptorTable[rootIndex].AssignedHandlesBitMap != 0)
				m_StaleRootParamsBitMap |= (1 << rootIndex);
		}
	}

	void DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* handles)
	{
		MYGAME_ASSERT(((1 << rootIndex) & m_RootDescriptorTablesBitMap) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table");
		MYGAME_ASSERT(offset + numHandles <= m_RootDescriptorTable[rootIndex].TableSize);

		DescriptorTableCache& TableCache = m_RootDescriptorTable[rootIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE* CopyDest = TableCache.TableStart + offset;

		for (uint32_t i = 0; i < numHandles; ++i)
		{
			CopyDest[i] = handles[i];
		}

		TableCache.AssignedHandlesBitMap |= ((1 << numHandles) - 1) << offset;
		m_StaleRootParamsBitMap |= (1 << rootIndex);
	}

	void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig)
	{
		uint32_t CurrentOffset = 0;
		m_StaleRootParamsBitMap = 0;
		m_RootDescriptorTablesBitMap = (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
			rootSig.m_SamplerTableBitMap : rootSig.m_DescriptorTableBitMap);

		uint32_t TableParams = m_RootDescriptorTablesBitMap;
		DWORD rootIndex;
		while (_BitScanForward(&rootIndex, TableParams))
		{
			TableParams ^= (1 << rootIndex);

			auto TableSize = rootSig.m_DescriptorTableSize[rootIndex];

			DescriptorTableCache& RootDescriptorTable = m_RootDescriptorTable[rootIndex];
			RootDescriptorTable.AssignedHandlesBitMap = 0;
			RootDescriptorTable.TableStart = m_HandleCache + CurrentOffset;
			RootDescriptorTable.TableSize = TableSize;
			CurrentOffset += TableSize;
		}

		m_MaxCachedDescriptors = CurrentOffset;
		MYGAME_ASSERT(m_MaxCachedDescriptors <= s_MaxNumDescriptors, "Exceeded user-supplied maximum cache size");
	}
}