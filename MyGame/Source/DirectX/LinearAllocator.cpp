#include "CommonHeaders.h"

#include "LinearAllocator.h"
#include "CommandList.h"
#include "DirectXImpl.h"

#include "../Utilities/CommonMath.h"

using namespace DirectX;

namespace MyGame
{
	LinearAllocatorType LinearAllocatorPageManager::sm_AutoType = LinearAllocatorType::kGpuExclusive;
	LinearAllocatorPageManager::LinearAllocatorPageManager()
	{
		m_AllocationType = sm_AutoType;
		sm_AutoType = (LinearAllocatorType)(static_cast<int>(sm_AutoType) + 1);
		MYGAME_ASSERT(sm_AutoType <= LinearAllocatorType::kNumAllocatorTypes);
	}

	LinearAllocatorPageManager LinearAllocator::sm_PageManager[2];

	LinearAllocationPage* LinearAllocatorPageManager::RequestPage()
	{
		std::lock_guard<std::mutex> LockGuard(m_Mutex);

		while (!m_RetiredPages.empty() && CommandListManager::IsFenceComplete(m_RetiredPages.front().first))
		{
			m_AvailablePages.push(m_RetiredPages.front().second);
			m_RetiredPages.pop();
		}

		LinearAllocationPage* PagePtr = nullptr;

		if (!m_AvailablePages.empty())
		{
			PagePtr = m_AvailablePages.front();
			m_AvailablePages.pop();
		}
		else
		{
			PagePtr = CreateNewPage();
			m_PagePool.emplace_back(PagePtr);
		}

		return PagePtr;
	}

	void LinearAllocatorPageManager::DiscardPages(uint64_t FenceValue, const std::vector<LinearAllocationPage*>& UsedPages)
	{
		std::lock_guard<std::mutex> LockGuard(m_Mutex);

		for (auto iter = UsedPages.begin(); iter != UsedPages.end(); ++iter)
			m_RetiredPages.push(std::make_pair(FenceValue, *iter));
	}

	void LinearAllocatorPageManager::FreeLargePages(uint64_t FenceValue, const std::vector<LinearAllocationPage*>& LargePages)
	{
		std::lock_guard<std::mutex> LockGuard(m_Mutex);

		while (!m_DeletionQueue.empty() && CommandListManager::IsFenceComplete(m_DeletionQueue.front().first))
		{
			delete m_DeletionQueue.front().second;
			m_DeletionQueue.pop();
		}

		for (auto iter = LargePages.begin(); iter != LargePages.end(); ++iter)
		{
			(*iter)->Unmap();
			m_DeletionQueue.push(std::make_pair(FenceValue, *iter));
		}
	}

	LinearAllocationPage* LinearAllocatorPageManager::CreateNewPage(size_t PageSize)
	{
		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC ResourceDesc = {};
		ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		ResourceDesc.Alignment = 0;
		ResourceDesc.Height = 1;
		ResourceDesc.DepthOrArraySize = 1;
		ResourceDesc.MipLevels = 1;
		ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		ResourceDesc.SampleDesc.Count = 1;
		ResourceDesc.SampleDesc.Quality = 0;
		ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		D3D12_RESOURCE_STATES DefaultUsage = D3D12_RESOURCE_STATE_COMMON;

		if (m_AllocationType == LinearAllocatorType::kGpuExclusive)
		{
			HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
			ResourceDesc.Width = PageSize == 0 ? static_cast<int>(LinearAllocatorPageSize::kGpuAllocatorPageSize) : PageSize;
			ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			DefaultUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		else
		{
			HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			ResourceDesc.Width = PageSize == 0 ? static_cast<int>(LinearAllocatorPageSize::kCpuAllocatorPageSize) : PageSize;
			ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			DefaultUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
		}

		ID3D12Resource* pBuffer = nullptr;
		ThrowIfFailed(DirectXImpl::D12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc, DefaultUsage, nullptr, IID_PPV_ARGS(&pBuffer)));
		NAME_D3D12_OBJ_STR(pBuffer, L"LinearAllocatorPage");

		return new LinearAllocationPage(pBuffer, DefaultUsage);
	}

	void LinearAllocator::CleanupUsedPages(uint64_t FenceID)
	{
		if (m_CurPage != nullptr)
		{
			m_RetiredPages.push_back(m_CurPage);
			m_CurPage = nullptr;
			m_CurOffset = 0;
		}

		sm_PageManager[static_cast<int>(m_AllocationType)].DiscardPages(FenceID, m_RetiredPages);
		m_RetiredPages.clear();

		sm_PageManager[static_cast<int>(m_AllocationType)].FreeLargePages(FenceID, m_LargePageList);
		m_LargePageList.clear();
	}

	DynAlloc LinearAllocator::AllocateLargePage(size_t SizeInBytes)
	{
		LinearAllocationPage* OneOff = sm_PageManager[static_cast<int>(m_AllocationType)].CreateNewPage(SizeInBytes);
		m_LargePageList.push_back(OneOff);

		DynAlloc ret(*OneOff, 0, SizeInBytes);
		ret.DataPtr = OneOff->m_CpuVirtualAddress;
		ret.GpuAddress = OneOff->m_GpuVirtualAddress;

		return ret;
	}

	DynAlloc LinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment)
	{
		const size_t AlignmentMask = Alignment - 1;
		MYGAME_ASSERT((AlignmentMask & Alignment) == 0);
		const size_t AlignedSize = Math::AlignUpWithMask(SizeInBytes, AlignmentMask);

		if (AlignedSize > m_PageSize)
			return AllocateLargePage(AlignedSize);

		m_CurOffset = Math::AlignUp(m_CurOffset, Alignment);

		if (m_CurOffset + AlignedSize > m_PageSize)
		{
			MYGAME_ASSERT(m_CurPage != nullptr);
			m_RetiredPages.push_back(m_CurPage);
			m_CurPage = nullptr;
		}

		if (m_CurPage == nullptr)
		{
			m_CurPage = sm_PageManager[static_cast<int>(m_AllocationType)].RequestPage();
			m_CurOffset = 0;
		}

		DynAlloc ret(*m_CurPage, m_CurOffset, AlignedSize);
		ret.DataPtr = (uint8_t*)m_CurPage->m_CpuVirtualAddress + m_CurOffset;
		ret.GpuAddress = m_CurPage->m_GpuVirtualAddress + m_CurOffset;
		m_CurOffset += AlignedSize;

		return ret;
	}
}