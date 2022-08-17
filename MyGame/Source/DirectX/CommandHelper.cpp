#include "CommonHeaders.h"

#include "CommandHelper.h"
#include "RootSignature.h"
#include "DirectXImpl.h"

using namespace DirectX;

namespace MyGame
{
	// Command Allocator Pool

	CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) : m_cCommandListType(Type) { m_AllocatorPool.reserve(24); }
	CommandAllocatorPool::~CommandAllocatorPool() { Shutdown(); }

	void CommandAllocatorPool::Create(ID3D12Device* pDevice) { m_Device = pDevice; }
	void CommandAllocatorPool::Shutdown()
	{
		MYGAME_INFO("Releasing and Cleaning all Command Allocator");

		for (auto& pool : m_AllocatorPool)
			pool->Release();
		m_AllocatorPool.clear();
	}

	ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
	{
		MYGAME_INFO("Requesting Command Allocator");

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
			MYGAME_INFO("Creating Command Allocator");

			ThrowIfFailed(m_Device->CreateCommandAllocator(m_cCommandListType, IID_PPV_ARGS(&pAllocator)));
			m_AllocatorPool.emplace_back(pAllocator);
			NAME_D3D12_OBJ_INDEXED_STR(pAllocator, L"CommandAllocatorPool", m_AllocatorPool.size());
		}

		return pAllocator;
	}

	void CommandAllocatorPool::DiscardAllocator(ID3D12CommandAllocator* Allocator, uint64_t FenceValue) { m_ReadyAllocators.emplace(Allocator, FenceValue); }

	// Command Queue

	CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE Type) :
		m_Type(Type),
		D3D12_CmdQueue(nullptr),
		m_pFence(nullptr),
		m_FenceEventHandle(nullptr),
		m_NextFenceValue((uint64_t)Type << 56 | 1),
		m_LastCompletedFenceValue((uint64_t)Type << 56),
		m_AllocatorPool(Type) {}

	CommandQueue::~CommandQueue() { Shutdown(); }

	void CommandQueue::Create(ID3D12Device* pDevice)
	{
		MYGAME_ASSERT(pDevice != nullptr);
		MYGAME_ASSERT(!IsReady());
		MYGAME_ASSERT(m_AllocatorPool.Size() == 0);

		D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
		QueueDesc.Type = m_Type;
		pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&D3D12_CmdQueue));
		NAME_D3D12_OBJ_STR(D3D12_CmdQueue, L"CommandQueue::Create");

		ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
		m_pFence->Signal((uint64_t)m_Type << 56);

		m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
		MYGAME_ASSERT(m_FenceEventHandle != NULL);

		m_AllocatorPool.Create(pDevice);
		MYGAME_ASSERT(IsReady());
	}

	void CommandQueue::Shutdown()
	{
		if (D3D12_CmdQueue == nullptr)
			return;

		m_AllocatorPool.Shutdown();
		CloseHandle(m_FenceEventHandle);

		m_pFence->Release();
		m_pFence = nullptr;
		D3D12_CmdQueue->Release();
		D3D12_CmdQueue = nullptr;
	}

	ID3D12CommandAllocator* CommandQueue::RequestAllocator()
	{
		uint64_t CompletedFence = m_pFence->GetCompletedValue();
		return m_AllocatorPool.RequestAllocator(CompletedFence);
	}

	void CommandQueue::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator) { m_AllocatorPool.DiscardAllocator(FenceValue, Allocator); }

	uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* List)
	{
		ThrowIfFailed(((ID3D12GraphicsCommandList*)List)->Close());
		D3D12_CmdQueue->ExecuteCommandLists(1, &List);
		D3D12_CmdQueue->Signal(m_pFence, m_NextFenceValue);
		return m_NextFenceValue++;
	}

	uint64_t CommandQueue::IncrementFence()
	{
		D3D12_CmdQueue->Signal(m_pFence, m_NextFenceValue);
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
		D3D12_CmdQueue->Wait(Producer.m_pFence, FenceValue);
	}

	void CommandQueue::StallForProducer(CommandQueue& Producer)
	{
		MYGAME_ASSERT(Producer.m_NextFenceValue > 0);
		D3D12_CmdQueue->Wait(Producer.m_pFence, Producer.m_NextFenceValue - 1);
	}

	void CommandQueue::WaitForFence(uint64_t FenceValue)
	{
		if (IsFenceComplete(FenceValue))
			return;

		m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
		WaitForSingleObject(m_FenceEventHandle, INFINITE);
		m_LastCompletedFenceValue = FenceValue;
	}

	// Command List Manager

	void CommandListManager::Create(ID3D12Device* pDevice)
	{
		MYGAME_INFO("Creating Command Queues");

		MYGAME_ASSERT(pDevice != nullptr);
		m_Device = pDevice;
		m_GraphicsQueue.Create(pDevice);
		m_ComputeQueue.Create(pDevice);
		m_CopyQueue.Create(pDevice);
	}

	void CommandListManager::Shutdown()
	{
		MYGAME_INFO("Destroying Command Queues");

		m_GraphicsQueue.Shutdown();
		m_ComputeQueue.Shutdown();
		m_CopyQueue.Shutdown();
	}

	void CommandListManager::CreateNewCommandList(ID3D12GraphicsCommandList** List, ID3D12CommandAllocator** Allocator, D3D12_COMMAND_LIST_TYPE Type)
	{
		MYGAME_ASSERT(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
		switch (Type)
		{
		case D3D12_COMMAND_LIST_TYPE_DIRECT: *Allocator = m_GraphicsQueue.RequestAllocator(); break;
		case D3D12_COMMAND_LIST_TYPE_COMPUTE: *Allocator = m_ComputeQueue.RequestAllocator(); break;
		case D3D12_COMMAND_LIST_TYPE_COPY: *Allocator = m_CopyQueue.RequestAllocator(); break;
		}
		ThrowIfFailed(m_Device->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)));
		NAME_D3D12_OBJ_STR((*List), L"CommandListManager::CreateNewCommandList");
	}

	void CommandListManager::WaitForFence(uint64_t FenceValue)
	{
		CommandQueue& Producer = CommandListManager::GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
		Producer.WaitForFence(FenceValue);
	}

	// Command Signature

	void CommandSignature::Finalize(const RootSignature* RootSignature)
	{
		if (m_Finalized)
			return;

		MYGAME_INFO("Creating Command Signature");

		UINT ByteStride = 0;
		bool RequiresRootSignature = false;

		for (UINT i = 0; i < m_NumParameters; ++i)
		{
			switch (m_ParamArray[i].GetDesc().Type)
			{
			case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
				ByteStride += sizeof(D3D12_DRAW_ARGUMENTS);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
				ByteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
				ByteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
				ByteStride += m_ParamArray[i].GetDesc().Constant.Num32BitValuesToSet * 4;
				RequiresRootSignature = true;
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
				ByteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
				ByteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
			case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
			case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
				ByteStride += 8;
				RequiresRootSignature = true;
				break;
			}
		}

		D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc = {};
		CommandSignatureDesc.ByteStride = ByteStride;
		CommandSignatureDesc.NumArgumentDescs = m_NumParameters;
		CommandSignatureDesc.pArgumentDescs = (const D3D12_INDIRECT_ARGUMENT_DESC*)m_ParamArray.get();

		ID3D12RootSignature* pRootSig = RootSignature ? RootSignature->GetSignature() : nullptr;
		if (RequiresRootSignature)
			assert(pRootSig != nullptr);
		else
			pRootSig = nullptr;

		ThrowIfFailed(DirectXImpl::D3D12_Device->CreateCommandSignature(&CommandSignatureDesc, pRootSig, IID_PPV_ARGS(&m_Signature)));
		NAME_D3D12_OBJ(m_Signature.Get());
		m_Finalized = TRUE;
	}
}