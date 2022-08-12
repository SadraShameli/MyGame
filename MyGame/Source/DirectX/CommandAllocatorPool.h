#pragma once

#include <vector>
#include <queue>
#include <mutex>

#include <d3d12.h>

namespace MyGame
{
	struct AllocatorPool
	{
		ID3D12CommandAllocator* CommandAllocator;
		uint64_t Fence;
	};

	class CommandAllocatorPool
	{
	public:
		CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);
		~CommandAllocatorPool();

		void Create(ID3D12Device* pDevice);
		void Shutdown();

		ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
		void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

		inline size_t Size() { return m_AllocatorPool.size(); }

	private:
		D3D12_COMMAND_LIST_TYPE m_cCommandListType;

		ID3D12Device* D12Device = nullptr;
		std::vector<ID3D12CommandAllocator*> m_AllocatorPool;
		std::queue<AllocatorPool> m_ReadyAllocators;
		std::mutex m_AllocatorMutex;
	};
}