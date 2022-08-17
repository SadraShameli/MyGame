#pragma once

#include "DirectXImpl.h"
#include "../Debugs/DebugHelpers.h"

#include <queue>
#include <d3d12.h>

namespace MyGame
{
	// Command Allocator Pool

	class CommandAllocatorPool
	{
	public:
		CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);
		~CommandAllocatorPool();

		void Create(ID3D12Device* pDevice);
		void Shutdown();

		ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
		void DiscardAllocator(ID3D12CommandAllocator* Allocator, uint64_t FenceValue);

		inline size_t Size() { return m_AllocatorPool.size(); }

	private:
		ID3D12Device* m_Device = nullptr;
		D3D12_COMMAND_LIST_TYPE m_cCommandListType;
		std::vector<ID3D12CommandAllocator*> m_AllocatorPool;

		struct AllocatorPool
		{
			ID3D12CommandAllocator* CommandAllocator;
			uint64_t Fence;
		};
		std::queue<AllocatorPool> m_ReadyAllocators;
	};

	// Command Queue

	class CommandQueue
	{
		friend class CommandListManager;
		friend class CommandContext;

	public:
		CommandQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);
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

		inline bool IsReady() { return D3D12_CmdQueue != nullptr; }
		ID3D12CommandQueue* GetCommandQueue() { return D3D12_CmdQueue; }

		ID3D12CommandAllocator* RequestAllocator();
		void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator* Allocator);
		uint64_t ExecuteCommandList(ID3D12CommandList* List);

	private:
		ID3D12CommandQueue* D3D12_CmdQueue;
		D3D12_COMMAND_LIST_TYPE m_Type;
		ID3D12Fence* m_pFence;
		uint64_t m_NextFenceValue;
		uint64_t m_LastCompletedFenceValue;
		HANDLE m_FenceEventHandle;

		CommandAllocatorPool m_AllocatorPool;
	};

	// Command List Manager

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
		static void CreateNewCommandList(ID3D12GraphicsCommandList** List, ID3D12CommandAllocator** Allocator, D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);

		static bool IsFenceComplete(uint64_t FenceValue) { return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue); }
		static void WaitForFence(uint64_t FenceValue);
		static void IdleGPU()
		{
			m_GraphicsQueue.WaitForIdle();
			m_ComputeQueue.WaitForIdle();
			m_CopyQueue.WaitForIdle();
		}

	private:
		static inline ID3D12Device* m_Device = nullptr;

		static inline CommandQueue m_GraphicsQueue = D3D12_COMMAND_LIST_TYPE_DIRECT;
		static inline CommandQueue m_ComputeQueue = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		static inline CommandQueue m_CopyQueue = D3D12_COMMAND_LIST_TYPE_COPY;
	};

	// IndirectParameter

	class IndirectParameter
	{
	public:
		IndirectParameter() { m_IndirectParam.Type = (D3D12_INDIRECT_ARGUMENT_TYPE)0xFFFFFFFF; }

		void Draw() { m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW; }
		void DrawIndexed() { m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED; }
		void Dispatch() { m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH; }

		void IndexBufferView() { m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW; }
		void VertexBufferView(UINT Slot)
		{
			m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
			m_IndirectParam.VertexBuffer.Slot = Slot;
		}

		void Constant(UINT RootParameterIndex, UINT DestOffsetIn32BitValues, UINT Num32BitValuesToSet)
		{
			m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			m_IndirectParam.Constant.RootParameterIndex = RootParameterIndex;
			m_IndirectParam.Constant.DestOffsetIn32BitValues = DestOffsetIn32BitValues;
			m_IndirectParam.Constant.Num32BitValuesToSet = Num32BitValuesToSet;
		}

		void ConstantBufferView(UINT RootParameterIndex)
		{
			m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
			m_IndirectParam.ConstantBufferView.RootParameterIndex = RootParameterIndex;
		}

		void ShaderResourceView(UINT RootParameterIndex)
		{
			m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
			m_IndirectParam.ShaderResourceView.RootParameterIndex = RootParameterIndex;
		}

		void UnorderedAccessView(UINT RootParameterIndex)
		{
			m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
			m_IndirectParam.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
		}

		const D3D12_INDIRECT_ARGUMENT_DESC& GetDesc() const { return m_IndirectParam; }

	private:
		friend class CommandSignature;

	protected:
		D3D12_INDIRECT_ARGUMENT_DESC m_IndirectParam = {};
	};

	// Command Signature

	class RootSignature;
	class CommandSignature : public DirectXImpl
	{
	public:
		CommandSignature(UINT NumParams = 0) : m_Finalized(FALSE), m_NumParameters(NumParams) { Reset(NumParams); }

		void Destroy()
		{
			m_Signature = nullptr;
			m_ParamArray = nullptr;
		}

		void Reset(UINT NumParams)
		{
			if (NumParams > 0)
				m_ParamArray.reset(new IndirectParameter[NumParams]);
			else
				m_ParamArray = nullptr;
			m_NumParameters = NumParams;
		}

		IndirectParameter& operator[] (size_t EntryIndex)
		{
			assert(EntryIndex < m_NumParameters);
			return m_ParamArray.get()[EntryIndex];
		}

		const IndirectParameter& operator[] (size_t EntryIndex) const
		{
			assert(EntryIndex < m_NumParameters);
			return m_ParamArray.get()[EntryIndex];
		}

		void Finalize(const RootSignature* RootSignature = nullptr);

		ID3D12CommandSignature* GetSignature() const { return m_Signature.Get(); }

	protected:
		BOOL m_Finalized;
		UINT m_NumParameters;
		std::unique_ptr<IndirectParameter[]> m_ParamArray;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_Signature;
	};

	inline static CommandSignature DispatchIndirectCommandSignature(1);
	inline static CommandSignature DrawIndirectCommandSignature(1);
}