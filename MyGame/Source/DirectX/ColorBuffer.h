#pragma once

#include "PixelBuffer.h"
#include "GpuBuffer.h"

#include "../Renderer/Color.h"
#include "../Debugs/DebugHelpers.h"

#include <d3d12.h>

namespace MyGame
{
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
}