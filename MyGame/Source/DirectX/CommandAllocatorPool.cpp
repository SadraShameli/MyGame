#include "CommonHeaders.h"

#include "DirectXHelpers.h"
#include "CommandAllocatorPool.h"

using namespace DirectX;

namespace MyGame
{
	CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) : m_cCommandListType(Type) { m_AllocatorPool.reserve(24); }
	CommandAllocatorPool::~CommandAllocatorPool() { Shutdown(); }

	void CommandAllocatorPool::Create(ID3D12Device* pDevice) { D12Device = pDevice; }
	void CommandAllocatorPool::Shutdown()
	{
		for (auto& pool : m_AllocatorPool)
			pool->Release();
		m_AllocatorPool.clear();
	}

	ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
	{
		std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);
		ID3D12CommandAllocator* pAllocator = nullptr;

		if (!m_ReadyAllocators.empty())
		{
			AllocatorPool& AllocatorPair = m_ReadyAllocators.front();
			if (AllocatorPair.Fence <= CompletedFenceValue)
			{
				pAllocator = AllocatorPair.CommandAllocator;
				ThrowIfFailed(pAllocator->Reset());
				m_ReadyAllocators.pop();
			}
		}

		if (pAllocator == nullptr)
		{
			ThrowIfFailed(D12Device->CreateCommandAllocator(m_cCommandListType, IID_PPV_ARGS(&pAllocator)));
			NAME_D3D12_OBJ(pAllocator);
			m_AllocatorPool.emplace_back(pAllocator);
		}

		return pAllocator;
	}

	void CommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
	{
		std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);
		m_ReadyAllocators.emplace(Allocator, FenceValue);
	}
}