#include "CommonHeaders.h"

#include "CommandList.h"
#include "DirectXImpl.h"
#include "DirectXHelpers.h"

#include "../Debugs/DebugHelpers.h"

using namespace DirectX;

namespace MyGame
{
	CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE Type) :
		m_Type(Type),
		m_CommandQueue(nullptr),
		m_pFence(nullptr),
		m_NextFenceValue((uint64_t)Type << 56 | 1),
		m_LastCompletedFenceValue((uint64_t)Type << 56),
		m_AllocatorPool(Type) {}

	CommandQueue::~CommandQueue() { Shutdown(); }
	void CommandQueue::Shutdown()
	{
		if (m_CommandQueue == nullptr)
			return;

		m_AllocatorPool.Shutdown();
		CloseHandle(m_FenceEventHandle);

		m_pFence->Release();
		m_pFence = nullptr;
		m_CommandQueue->Release();
		m_CommandQueue = nullptr;
	}

	void CommandListManager::Shutdown()
	{
		m_GraphicsQueue.Shutdown();
		m_ComputeQueue.Shutdown();
		m_CopyQueue.Shutdown();
	}

	void CommandQueue::Create(ID3D12Device* pDevice)
	{
		MYGAME_ASSERT(pDevice != nullptr);
		MYGAME_ASSERT(!IsReady());
		MYGAME_ASSERT(m_AllocatorPool.Size() == 0);

		D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
		QueueDesc.Type = m_Type;
		pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_CommandQueue));
		m_CommandQueue->SetName(L"CommandListManager::m_CommandQueue");

		ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
		NAME_D3D12_OBJ(pDevice);
		m_pFence->Signal((uint64_t)m_Type << 56);

		m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
		MYGAME_ASSERT(m_FenceEventHandle != NULL);

		m_AllocatorPool.Create(pDevice);
		MYGAME_ASSERT(IsReady());
	}

	void CommandListManager::Create(ID3D12Device* pDevice)
	{
		MYGAME_ASSERT(pDevice != nullptr);
		D12Device = pDevice;
		m_GraphicsQueue.Create(pDevice);
		m_ComputeQueue.Create(pDevice);
		m_CopyQueue.Create(pDevice);
	}

	void CommandListManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList** List, ID3D12CommandAllocator** Allocator)
	{
		MYGAME_ASSERT(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
		switch (Type)
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT: *Allocator = m_GraphicsQueue.RequestAllocator(); break;
		case D3D12_COMMAND_LIST_TYPE_BUNDLE: break;
		case D3D12_COMMAND_LIST_TYPE_COMPUTE: *Allocator = m_ComputeQueue.RequestAllocator(); break;
		case D3D12_COMMAND_LIST_TYPE_COPY: *Allocator = m_CopyQueue.RequestAllocator(); break;
		}

		ThrowIfFailed(D12Device->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)));
		NAME_D3D12_OBJ_STR((*List), L"CommandList");
	}

	uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* List)
	{
		std::lock_guard<std::mutex> LockGuard(m_FenceMutex);

		ThrowIfFailed(((ID3D12GraphicsCommandList*)List)->Close());
		m_CommandQueue->ExecuteCommandLists(1, &List);
		m_CommandQueue->Signal(m_pFence, m_NextFenceValue);
		return m_NextFenceValue++;
	}

	uint64_t CommandQueue::IncrementFence(void)
	{
		std::lock_guard<std::mutex> LockGuard(m_FenceMutex);

		m_CommandQueue->Signal(m_pFence, m_NextFenceValue);
		return m_NextFenceValue++;
	}

	bool CommandQueue::IsFenceComplete(uint64_t FenceValue)
	{
		if (FenceValue > m_LastCompletedFenceValue)
			m_LastCompletedFenceValue = max(m_LastCompletedFenceValue, m_pFence->GetCompletedValue());
		return FenceValue <= m_LastCompletedFenceValue;
	}

	void CommandQueue::StallForFence(uint64_t FenceValue)
	{
		CommandQueue& Producer = CommandListManager::GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
		m_CommandQueue->Wait(Producer.m_pFence, FenceValue);
	}

	void CommandQueue::StallForProducer(CommandQueue& Producer)
	{
		MYGAME_ASSERT(Producer.m_NextFenceValue > 0);
		m_CommandQueue->Wait(Producer.m_pFence, Producer.m_NextFenceValue - 1);
	}

	void CommandQueue::WaitForFence(uint64_t FenceValue)
	{
		if (IsFenceComplete(FenceValue))
			return;

		std::lock_guard<std::mutex> LockGuard(m_EventMutex);

		m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
		WaitForSingleObject(m_FenceEventHandle, INFINITE);
		m_LastCompletedFenceValue = FenceValue;
	}

	void CommandListManager::WaitForFence(uint64_t FenceValue)
	{
		CommandQueue& Producer = CommandListManager::GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
		Producer.WaitForFence(FenceValue);
	}

	ID3D12CommandAllocator* CommandQueue::RequestAllocator()
	{
		uint64_t CompletedFence = m_pFence->GetCompletedValue();
		return m_AllocatorPool.RequestAllocator(CompletedFence);
	}

	void CommandQueue::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator) { m_AllocatorPool.DiscardAllocator(FenceValue, Allocator); }
}