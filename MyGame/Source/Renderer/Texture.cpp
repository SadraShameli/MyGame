#include "CommonHeaders.h"
#include "TextureManager.h"

#include "../DirectX/CommandContext.h"
#include "../DirectX/DirectXImpl.h"
#include "../Utilities/FileManager.h"

#include <D3D12MemAlloc.h>
#include <ResourceUploadBatch.h>
#include <DDSTextureLoader.h>
#include <DirectXHelpers.h>

using namespace DirectX;

namespace MyGame
{
	void Texture::Create2D(uint32_t rowPitchBytes, uint32_t width, uint32_t height, DXGI_FORMAT format, const void* data)
	{
		Destroy();

		m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;
		m_Width = width;
		m_Height = height;
		m_Depth = 1;

		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.DepthOrArraySize = 1;
		texDesc.MipLevels = 1;
		texDesc.Format = format;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_SUBRESOURCE_DATA texResource = {};
		texResource.pData = data;
		texResource.RowPitch = rowPitchBytes;
		texResource.SlicePitch = static_cast<LONG_PTR>(rowPitchBytes) * height;

		CommandContext::InitializeTexture(*this, &texResource, 1);
	}

	void Texture::CreateCube(uint32_t rowPitchBytes, uint32_t width, uint32_t height, DXGI_FORMAT Format, const void* data)
	{
		Destroy();

		m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;
		m_Width = width;
		m_Height = height;
		m_Depth = 6;

		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.DepthOrArraySize = 6;
		texDesc.MipLevels = 1;
		texDesc.Format = Format;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES HeapProps = {};
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		ThrowIfFailed(DirectXImpl::Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
			m_UsageState, nullptr, IID_PPV_ARGS(&m_Resource)));

		D3D12_SUBRESOURCE_DATA texResource = {};
		texResource.pData = data;
		texResource.RowPitch = rowPitchBytes;
		texResource.SlicePitch = texResource.RowPitch * height;
		CommandContext::InitializeTexture(*this, &texResource, 1);

		if (m_CpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_CpuDescriptorHandle = DescriptorHeap::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		DirectXImpl::Device->CreateShaderResourceView(m_Resource, &srvDesc, m_CpuDescriptorHandle);
	}

	bool Texture::CreateDDSFromMemory(const uint8_t* data, size_t size, bool sRGB)
	{
		DirectX::ResourceUploadBatch upload(DirectXImpl::Device);
		upload.Begin();

		MYGAME_HRESULT_TOSTR(CreateDDSTextureFromMemory(DirectXImpl::Device, upload, data, size, &m_Resource));

		auto uploadResourcesFinished = upload.End(CommandListManager::GetCommandQueue());
		uploadResourcesFinished.wait();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = m_Resource->GetDesc().Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = m_Resource->GetDesc().MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		if (m_CpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_CpuDescriptorHandle = DescriptorHeap::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DirectXImpl::Device->CreateShaderResourceView(m_Resource, &srvDesc, m_CpuDescriptorHandle);

		return 0;
	}

	void Texture::CreateTGAFromMemory(const uint8_t* data, size_t size, bool sRGB)
	{
		const uint8_t* memPtr = data;

		memPtr += 2;
		*(memPtr)++;
		memPtr += 9;

		uint16_t imagewidth = *(uint16_t*)memPtr;
		memPtr += sizeof(uint16_t);
		uint16_t imageheight = *(uint16_t*)memPtr;
		memPtr += sizeof(uint16_t);
		uint8_t bitCount = *memPtr++;

		++memPtr;

		uint32_t* formattedData = new uint32_t[imagewidth * imageheight];
		uint32_t* iter = formattedData;

		uint8_t numChannels = bitCount / 8;
		uint32_t numBytes = imagewidth * imageheight * numChannels;

		switch (numChannels)
		{
		default:
			break;
		case 3:
			for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 3)
			{
				*iter++ = 0xff000000 | memPtr[0] << 16 | memPtr[1] << 8 | memPtr[2];
				memPtr += 3;
			}
			break;
		case 4:
			for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 4)
			{
				*iter++ = memPtr[3] << 24 | memPtr[0] << 16 | memPtr[1] << 8 | memPtr[2];
				memPtr += 4;
			}
			break;
		}

		Create2D((size_t)4 * imagewidth, imagewidth, imageheight, sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, formattedData);

		delete[] formattedData;
	}
}