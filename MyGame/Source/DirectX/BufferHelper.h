#pragma once

#include "DirectXImpl.h"
#include "DirectXHelpers.h"

#include "../Renderer/Color.h"

namespace MyGame
{
	class GpuResource : public DirectXImpl
	{
		friend class CommandContext;
		friend class GraphicsContext;
		friend class ComputeContext;

	public:
		GpuResource() :
			m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
			m_UsageState(D3D12_RESOURCE_STATE_COMMON),
			m_TransitioningState((D3D12_RESOURCE_STATES)-1) {}

		GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
			m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
			m_pResource(pResource),
			m_UsageState(CurrentState),
			m_TransitioningState((D3D12_RESOURCE_STATES)-1) {}

		~GpuResource() { Destroy(); }
		virtual void Destroy()
		{
			m_pResource = nullptr;
			m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
			++m_VersionID;
		}

		ID3D12Resource* operator->() { return m_pResource; }
		const ID3D12Resource* operator->() const { return m_pResource; }

		ID3D12Resource* GetResource() { return m_pResource; }
		const ID3D12Resource* GetResource() const { return m_pResource; }

		ID3D12Resource** GetAddressOf() { return &m_pResource; }
		D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }
		uint32_t GetVersionID() const { return m_VersionID; }

	protected:
		ID3D12Resource* m_pResource;
		D3D12_RESOURCE_STATES m_UsageState;
		D3D12_RESOURCE_STATES m_TransitioningState;
		D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;

		uint32_t m_VersionID = 0;
	};

	class UploadBuffer : public GpuResource
	{
	public:
		virtual ~UploadBuffer() { Destroy(); }

		void Create(const std::wstring& name, size_t BufferSize);

		void* Map(void);
		void Unmap(size_t begin = 0, size_t end = -1);

		size_t GetBufferSize() const { return m_BufferSize; }

	protected:
		size_t m_BufferSize;
	};

	class PixelBuffer : public GpuResource
	{
	public:
		PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN), m_BankRotation(0) {}

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		uint32_t GetDepth() const { return m_ArraySize; }
		const DXGI_FORMAT& GetFormat() const { return m_Format; }

		void ExportToFile(const std::string& FilePath);

	protected:
		D3D12_RESOURCE_DESC DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);

		void AssociateWithResource(ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState);
		void CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
			D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

		static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
		static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
		static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
		static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
		static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
		static size_t BytesPerPixel(DXGI_FORMAT Format);

		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_ArraySize;
		DXGI_FORMAT m_Format;
		uint32_t m_BankRotation;
	};

	class ColorBuffer : public PixelBuffer
	{
	public:
		ColorBuffer(Color ClearColor = Color(0.0f, 0.0f, 0.0f, 0.0f)) : m_ClearColor(ClearColor), m_NumMipMaps(0), m_FragmentCount(1), m_SampleCount(1)
		{
			m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			for (auto& handle : m_UAVHandle)
				handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		void CreateFromSwapChain(const std::wstring& Name, ID3D12Resource* BaseResource);
		void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
		void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_SRVHandle; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV() const { return m_RTVHandle; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const { return m_UAVHandle[0]; }

		void SetClearColor(Color ClearColor) { m_ClearColor = ClearColor; }
		void SetMsaaMode(uint32_t NumColorSamples, uint32_t NumCoverageSamples)
		{
			assert(NumCoverageSamples >= NumColorSamples);
			m_FragmentCount = NumColorSamples;
			m_SampleCount = NumCoverageSamples;
		}

		Color GetClearColor() const { return m_ClearColor; }
		void GenerateMipMaps(CommandContext& Context);

	protected:
		D3D12_RESOURCE_FLAGS CombineResourceFlags() const
		{
			D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;
			if (Flags == D3D12_RESOURCE_FLAG_NONE && m_FragmentCount == 1)
				Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

			return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
		}

		static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
		{
			uint32_t HighBit = 0;
			_BitScanReverse((unsigned long*)&HighBit, Width | Height);
			return HighBit + 1;
		}

		void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);

		Color m_ClearColor;
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
		D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
		uint32_t m_NumMipMaps;
		uint32_t m_FragmentCount;
		uint32_t m_SampleCount;
	};

	class DepthBuffer : public PixelBuffer
	{
	public:
		DepthBuffer(float ClearDepth = 0.0f, uint8_t ClearStencil = 0) : m_ClearDepth(ClearDepth), m_ClearStencil(ClearStencil)
		{
			m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
		void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() { return m_hDSV[0]; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_DepthReadOnly() { return m_hDSV[1]; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_StencilReadOnly() { return m_hDSV[2]; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_ReadOnly() { return m_hDSV[3]; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetDepthSRV() { return m_hDepthSRV; }
		const D3D12_CPU_DESCRIPTOR_HANDLE& GetStencilSRV() { return m_hStencilSRV; }

		float GetClearDepth() { return m_ClearDepth; }
		uint8_t GetClearStencil() { return m_ClearStencil; }

	protected:
		void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format);

		float m_ClearDepth;
		uint8_t m_ClearStencil;
		D3D12_CPU_DESCRIPTOR_HANDLE m_hDSV[4];
		D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;
	};
}
