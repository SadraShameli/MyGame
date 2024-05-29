#pragma once

#include "../DirectX/BufferHelper.h"

namespace MyGame
{
	class Texture : public GpuResource
	{
		friend class CommandContext;

	public:
		Texture()
		{
			m_Width = 0;
			m_Height = 0;
			m_Depth = 0;
			m_CpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		Texture(D3D12_CPU_DESCRIPTOR_HANDLE Handle) : m_CpuDescriptorHandle(Handle) {}

		virtual void Destroy() override
		{
			GpuResource::Destroy();
			m_CpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		uint32_t GetWidth() { return m_Width; }
		uint32_t GetHeight() { return m_Height; }
		uint32_t GetDepth() { return m_Depth; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() { return m_CpuDescriptorHandle; }

		void Create2D(uint32_t rowPitchBytes, uint32_t width, uint32_t height, DXGI_FORMAT format, const void* data);
		void CreateCube(uint32_t rowPitchBytes, uint32_t width, uint32_t height, DXGI_FORMAT format, const void* data);

		bool CreateDDSFromMemory(const uint8_t* data, size_t size, bool sRGB);
		void CreateTGAFromMemory(const uint8_t* data, size_t size, bool sRGB);
		void CreatePIXImageFromMemory(const void* data, size_t size);

	protected:
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Depth;
		D3D12_CPU_DESCRIPTOR_HANDLE m_CpuDescriptorHandle;
	};
}