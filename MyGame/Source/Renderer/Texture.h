#pragma once

#include "../DirectX/BufferHelper.h"

namespace MyGame
{
	class Texture : public GpuResource
	{
	public:
		enum DefaultTexture
		{
			kMagenta2D,
			kBlackOpaque2D,
			kBlackTransparent2D,
			kWhiteOpaque2D,
			kWhiteTransparent2D,
			kDefaultNormalMap,
			kBlackCubeMap,
			kNumDefaultTextures
		};

		Texture()
		{
			m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			m_Width = 0;
			m_Height = 0;
			m_Depth = 0;
		}
		Texture(D3D12_CPU_DESCRIPTOR_HANDLE Handle) : m_hCpuDescriptorHandle(Handle)
		{
			m_Width = 0;
			m_Height = 0;
			m_Depth = 0;
		}

		void Create2D(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData);
		void CreateCube(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData);

		void CreateTGAFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);
		bool CreateDDSFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);

		virtual void Destroy() override
		{
			GpuResource::Destroy();
			m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_hCpuDescriptorHandle; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		uint32_t GetDepth() const { return m_Depth; }

	private:
		friend class CommandContext;

	protected:
		uint32_t m_Width;
		uint32_t m_Height;
		uint32_t m_Depth;

		D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
	};

	inline static Texture DefaultTextures[Texture::kNumDefaultTextures];
	inline static D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultTexture(Texture::DefaultTexture texID) { return DefaultTextures[texID].GetSRV(); }
}