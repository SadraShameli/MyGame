#pragma once

#include "GpuResource.h"
#include "../Debugs/DebugHelpers.h"

#include <vector>
#include <queue>
#include <mutex>

#define DEFAULT_ALIGN 256

namespace MyGame
{
	class DynAlloc
	{
	public:
		DynAlloc(GpuResource& BaseResource, size_t ThisOffset, size_t ThisSize) : Buffer(BaseResource), Offset(ThisOffset), Size(ThisSize) {}
		GpuResource& Buffer;
		size_t Offset;
		size_t Size;
		void* DataPtr;
		D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
	};

	class LinearAllocationPage : public GpuResource
	{
	public:
		LinearAllocationPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES Usage) : GpuResource()
		{
			m_pResource = pResource;
			m_UsageState = Usage;
			m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
			m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
		}

		~LinearAllocationPage() { Unmap(); }

		void Map()
		{
			if (m_CpuVirtualAddress == nullptr)
				m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
		}

		void Unmap()
		{
			if (m_CpuVirtualAddress != nullptr)
			{
				m_pResource->Unmap(0, nullptr);
				m_CpuVirtualAddress = nullptr;
			}
		}

		void* m_CpuVirtualAddress;
		D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
	};

	enum class LinearAllocatorType
	{
		kInvalidAllocator = -1,
		kGpuExclusive = 0,
		kCpuWritable = 1,
		kNumAllocatorTypes = 2
	};

	enum class LinearAllocatorPageSize
	{
		kGpuAllocatorPageSize = 0x10000,
		kCpuAllocatorPageSize = 0x200000
	};

	class LinearAllocatorPageManager
	{
	public:
		LinearAllocatorPageManager();
		LinearAllocationPage* RequestPage();
		LinearAllocationPage* CreateNewPage(size_t PageSize = 0);

		void DiscardPages(uint64_t FenceID, const std::vector<LinearAllocationPage*>& Pages);
		void FreeLargePages(uint64_t FenceID, const std::vector<LinearAllocationPage*>& Pages);
		void Destroy() { m_PagePool.clear(); }

	private:
		static LinearAllocatorType sm_AutoType;
		LinearAllocatorType m_AllocationType;
		std::vector<std::unique_ptr<LinearAllocationPage> > m_PagePool;
		std::queue<std::pair<uint64_t, LinearAllocationPage*> > m_RetiredPages;
		std::queue<std::pair<uint64_t, LinearAllocationPage*> > m_DeletionQueue;
		std::queue<LinearAllocationPage*> m_AvailablePages;
		std::mutex m_Mutex;
	};

	class LinearAllocator
	{
	public:
		LinearAllocator(LinearAllocatorType Type) : m_AllocationType(Type), m_PageSize(0), m_CurOffset(~(size_t)0), m_CurPage(nullptr)
		{
			MYGAME_ASSERT(Type > LinearAllocatorType::kInvalidAllocator && Type < LinearAllocatorType::kNumAllocatorTypes);
			m_PageSize = static_cast<size_t>(Type == LinearAllocatorType::kGpuExclusive ? LinearAllocatorPageSize::kGpuAllocatorPageSize : LinearAllocatorPageSize::kCpuAllocatorPageSize);
		}

		DynAlloc Allocate(size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN);
		void CleanupUsedPages(uint64_t FenceID);
		static void DestroyAll()
		{
			sm_PageManager[0].Destroy();
			sm_PageManager[1].Destroy();
		}

	private:
		DynAlloc AllocateLargePage(size_t SizeInBytes);

		static LinearAllocatorPageManager sm_PageManager[2];
		LinearAllocatorType m_AllocationType;
		size_t m_PageSize;
		size_t m_CurOffset;
		LinearAllocationPage* m_CurPage;
		std::vector<LinearAllocationPage*> m_RetiredPages;
		std::vector<LinearAllocationPage*> m_LargePageList;
	};
}