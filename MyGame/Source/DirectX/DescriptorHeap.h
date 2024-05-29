#pragma once

#include <queue>

namespace MyGame
{
	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type)
			: m_Type(type), m_CurrentHeap(nullptr), m_DescriptorSize(0), m_RemainingFreeHandles(0)
		{
			m_CurrentHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t count);
		static ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
		static void DestroyAll();

	protected:
		static constexpr uint32_t s_NumDescriptorsPerHeap = 256;
		static inline std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> s_HeapPool;
		static inline std::mutex s_Mutex;

		D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
		ID3D12DescriptorHeap* m_CurrentHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentHandle;
		uint32_t m_DescriptorSize;
		uint32_t m_RemainingFreeHandles;
	};

	class DescriptorHandle
	{
	public:
		DescriptorHandle()
		{
			m_CpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle)
			: m_CpuHandle(CpuHandle)
		{
			m_GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle)
			: m_CpuHandle(CpuHandle), m_GpuHandle(GpuHandle) {}

		DescriptorHandle operator+ (size_t offsetScaledByDescriptorSize)
		{
			DescriptorHandle ret = *this;
			ret += offsetScaledByDescriptorSize;
			return ret;
		}

		void operator += (size_t offsetScaledByDescriptorSize)
		{
			if (m_CpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
				m_CpuHandle.ptr += offsetScaledByDescriptorSize;
			if (m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
				m_GpuHandle.ptr += offsetScaledByDescriptorSize;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE* operator&() const { return &m_CpuHandle; }
		operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_CpuHandle; }
		operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return m_GpuHandle; }

		size_t GetCpuPtr() const { return m_CpuHandle.ptr; }
		uint64_t GetGpuPtr() const { return m_GpuHandle.ptr; }
		bool IsNull() const { return m_CpuHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
		bool IsShaderVisible() const { return m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
	};

	class DescriptorHeap
	{
	public:
		DescriptorHeap()
			: m_HeapDesc(), m_DescriptorSize(0), m_NumFreeDescriptors(0) {}

		~DescriptorHeap()
		{
			Destroy();
		}

		void Create(const std::wstring& debugHeapName, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxCount = 1);
		void Destroy() { m_Heap = nullptr; }

		DescriptorHandle Allocate(uint32_t count = 1);

		DescriptorHandle operator[] (uint32_t arrayIdx)
		{
			return m_FirstHandle + static_cast<size_t>(arrayIdx) * m_DescriptorSize;
		}

		uint32_t GetOffsetOfHandle(DescriptorHandle& handle)
		{
			return static_cast<uint32_t>(handle.GetCpuPtr() - m_FirstHandle.GetCpuPtr()) / m_DescriptorSize;
		}

		ID3D12DescriptorHeap* GetHeapPointer() { return m_Heap.Get(); }
		uint32_t GetDescriptorSize() { return m_DescriptorSize; }

		bool ValidateHandle(const DescriptorHandle& handle);

		bool HasAvailableSpace(uint32_t count)
		{
			return count <= m_NumFreeDescriptors;
		}

	public:
		static inline DescriptorAllocator DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
		{
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV
		};

		static D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count = 1)
		{
			return DescriptorAllocators[type].Allocate(count);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
		D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
		uint32_t m_DescriptorSize;
		uint32_t m_NumFreeDescriptors;
		DescriptorHandle m_FirstHandle;
		DescriptorHandle m_NextFreeHandle;
	};

	// Dynamic Descriptor Heap

	class CommandContext;
	class RootSignature;
	class DynamicDescriptorHeap
	{
	public:
		DynamicDescriptorHeap(CommandContext& ctx, D3D12_DESCRIPTOR_HEAP_TYPE heapType);

		static void DestroyAll();
		void CleanupUsedHeaps(uint64_t fenceValue);

		void SetGraphicsDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* handles)
		{
			m_GraphicsHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, handles);
		}

		void SetComputeDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* handles)
		{
			m_ComputeHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, handles);
		}

		D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handles);

		void ParseGraphicsRootSignature(const RootSignature& rootSig)
		{
			m_GraphicsHandleCache.ParseRootSignature(m_DescriptorType, rootSig);
		}

		void ParseComputeRootSignature(const RootSignature& rootSig)
		{
			m_ComputeHandleCache.ParseRootSignature(m_DescriptorType, rootSig);
		}

		void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* cmdList)
		{
			if (m_GraphicsHandleCache.m_StaleRootParamsBitMap != 0)
			{
				CopyAndBindStagedTables(m_GraphicsHandleCache, cmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
			}
		}

		void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* cmdList)
		{
			if (m_ComputeHandleCache.m_StaleRootParamsBitMap != 0)
			{
				CopyAndBindStagedTables(m_ComputeHandleCache, cmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
			}
		}

	private:
		static ID3D12DescriptorHeap* RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
		static void DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint64_t fenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& usedHeaps);

	private:
		static constexpr uint32_t s_NumHeaps = 2;
		static constexpr uint32_t s_NumDescriptorsPerHeap = 1024;
		static inline std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> s_HeapPool[s_NumHeaps];
		static inline std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> s_RetiredHeaps[s_NumHeaps];
		static inline std::queue<ID3D12DescriptorHeap*> s_AvailableHeaps[s_NumHeaps];
		static inline std::mutex m_Mutex;

		CommandContext& m_OwningContext;
		ID3D12DescriptorHeap* m_CurrentHeapPtr;
		const D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
		uint32_t m_DescriptorSize;
		uint32_t m_CurrentOffset;
		DescriptorHandle m_FirstDescriptor;
		std::vector<ID3D12DescriptorHeap*> m_RetiredHeaps;

		struct DescriptorTableCache
		{
			uint32_t AssignedHandlesBitMap;
			D3D12_CPU_DESCRIPTOR_HANDLE* TableStart;
			uint32_t TableSize;
		};

		struct DescriptorHandleCache
		{
			DescriptorHandleCache()
			{
				ClearCache();
			}

			void ClearCache()
			{
				m_RootDescriptorTablesBitMap = 0;
				m_StaleRootParamsBitMap = 0;
				m_MaxCachedDescriptors = 0;
			}

			uint32_t m_RootDescriptorTablesBitMap;
			uint32_t m_StaleRootParamsBitMap;
			uint32_t m_MaxCachedDescriptors;

			static constexpr uint32_t s_MaxNumDescriptors = 256;
			static constexpr uint32_t s_MaxNumDescriptorTables = 16;

			uint32_t ComputeStagedSize();
			void CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorSize, DescriptorHandle destHandleStart, ID3D12GraphicsCommandList* cmdList,
				void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* setFunc)(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE));

			DescriptorTableCache m_RootDescriptorTable[s_MaxNumDescriptorTables] = {};
			D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCache[s_MaxNumDescriptors] = {};

			void UnbindAllValid();
			void StageDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE* handles);
			void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig);
		};

		DescriptorHandleCache m_GraphicsHandleCache;
		DescriptorHandleCache m_ComputeHandleCache;

		bool HasSpace(uint32_t count)
		{
			return (m_CurrentHeapPtr != nullptr && m_CurrentOffset + count <= s_NumDescriptorsPerHeap);
		}

		void RetireCurrentHeap();
		void RetireUsedHeaps(uint64_t fenceValue);
		ID3D12DescriptorHeap* GetHeapPointer();

		DescriptorHandle Allocate(uint32_t count)
		{
			DescriptorHandle ret = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
			m_CurrentOffset += count;
			return ret;
		}

		void CopyAndBindStagedTables(DescriptorHandleCache& HandleCache, ID3D12GraphicsCommandList* cmdList,
			void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(uint32_t, D3D12_GPU_DESCRIPTOR_HANDLE));

		void UnbindAllValid();
	};
}