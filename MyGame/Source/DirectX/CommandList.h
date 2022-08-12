#pragma once

#include "CommandAllocatorPool.h"

namespace MyGame
{
	class CommandQueue
	{
		friend class CommandListManager;
		friend class CommandContext;

	public:
		CommandQueue(D3D12_COMMAND_LIST_TYPE Type);
		~CommandQueue();

		void Create(ID3D12Device* pDevice);
		void Shutdown();

		uint64_t IncrementFence();
		uint64_t GetNextFenceValue() { return m_NextFenceValue; }
		void WaitForIdle() { WaitForFence(IncrementFence()); }
		bool IsFenceComplete(uint64_t FenceValue);
		void StallForFence(uint64_t FenceValue);
		void StallForProducer(CommandQueue& Producer);
		void WaitForFence(uint64_t FenceValue);

		inline bool IsReady() { return m_CommandQueue != nullptr; }
		ID3D12CommandQueue* GetCommandQueue() { return m_CommandQueue; }

	private:
		uint64_t ExecuteCommandList(ID3D12CommandList* List);
		ID3D12CommandAllocator* RequestAllocator();
		void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);

		ID3D12CommandQueue* m_CommandQueue;
		const D3D12_COMMAND_LIST_TYPE m_Type;

		CommandAllocatorPool m_AllocatorPool;
		std::mutex m_FenceMutex;
		std::mutex m_EventMutex;

		ID3D12Fence* m_pFence;
		uint64_t m_NextFenceValue;
		uint64_t m_LastCompletedFenceValue;
		HANDLE m_FenceEventHandle;
	};

	class CommandListManager
	{
		friend class CommandContext;

	public:
		static void Create(ID3D12Device* pDevice);
		static void Shutdown();

		static CommandQueue& GetGraphicsQueue() { return m_GraphicsQueue; }
		static CommandQueue& GetComputeQueue() { return m_ComputeQueue; }
		static CommandQueue& GetCopyQueue() { return m_CopyQueue; }
		static CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			switch (Type)
			{
			case D3D12_COMMAND_LIST_TYPE_COMPUTE: return m_ComputeQueue;
			case D3D12_COMMAND_LIST_TYPE_COPY: return m_CopyQueue;
			default: return m_GraphicsQueue;
			}
		}

		static ID3D12CommandQueue* GetCommandQueue() { return m_GraphicsQueue.GetCommandQueue(); }
		static void CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList** List, ID3D12CommandAllocator** Allocator);

		static bool IsFenceComplete(uint64_t FenceValue) { return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue); }
		static void WaitForFence(uint64_t FenceValue);
		static void IdleGPU()
		{
			m_GraphicsQueue.WaitForIdle();
			m_ComputeQueue.WaitForIdle();
			m_CopyQueue.WaitForIdle();
		}

	private:
		inline static ID3D12Device* D12Device = nullptr;

		inline static CommandQueue m_GraphicsQueue = D3D12_COMMAND_LIST_TYPE_DIRECT;
		inline static CommandQueue m_ComputeQueue = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		inline static CommandQueue m_CopyQueue = D3D12_COMMAND_LIST_TYPE_COPY;
	};
}