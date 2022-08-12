#include "CommonHeaders.h"

#include "Texture.h"

#include "../DirectX/CommandContext.h"
#include "../DirectX/DirectXImpl.h"
#include "../Utilities/FileManager.h"

#include <D3D12MemAlloc.h>
#include <ResourceUploadBatch.h>
#include <DDSTextureLoader.h>

using namespace DirectX;

namespace MyGame
{
	void Texture::Create2D(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData)
	{
		Destroy();

		m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

		m_Width = (uint32_t)Width;
		m_Height = (uint32_t)Height;
		m_Depth = 1;

		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = Width;
		texDesc.Height = (UINT)Height;
		texDesc.DepthOrArraySize = 1;
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

		ThrowIfFailed(DirectXImpl::D12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
			m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));
		NAME_D3D12_OBJ_STR(m_pResource, L"Texture");

		D3D12_SUBRESOURCE_DATA texResource = {};
		texResource.pData = InitialData;
		texResource.RowPitch = RowPitchBytes;
		texResource.SlicePitch = RowPitchBytes * Height;
		CommandContext::InitializeTexture(*this, 1, &texResource);

		if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_hCpuDescriptorHandle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		DirectXImpl::D12Device->CreateShaderResourceView(m_pResource, nullptr, m_hCpuDescriptorHandle);
	}

	void Texture::CreateCube(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData)
	{
		Destroy();

		m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;
		m_Width = (uint32_t)Width;
		m_Height = (uint32_t)Height;
		m_Depth = 6;

		D3D12_RESOURCE_DESC texDesc = {};
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		texDesc.Width = Width;
		texDesc.Height = (UINT)Height;
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

		ThrowIfFailed(DirectXImpl::D12Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
			m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));
		NAME_D3D12_OBJ_STR(m_pResource, L"Texture");

		D3D12_SUBRESOURCE_DATA texResource = {};
		texResource.pData = InitialData;
		texResource.RowPitch = RowPitchBytes;
		texResource.SlicePitch = texResource.RowPitch * Height;
		CommandContext::InitializeTexture(*this, 1, &texResource);

		if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_hCpuDescriptorHandle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		DirectXImpl::D12Device->CreateShaderResourceView(m_pResource, &srvDesc, m_hCpuDescriptorHandle);
	}


	void Texture::CreateTGAFromMemory(const void* _filePtr, size_t, bool sRGB)
	{
		const uint8_t* filePtr = (const uint8_t*)_filePtr;

		filePtr += 2;
		*(filePtr)++;
		filePtr += 9;

		uint16_t imageWidth = *(uint16_t*)filePtr;
		filePtr += sizeof(uint16_t);
		uint16_t imageHeight = *(uint16_t*)filePtr;
		filePtr += sizeof(uint16_t);
		uint8_t bitCount = *filePtr++;

		filePtr++;

		uint32_t* formattedData = new uint32_t[imageWidth * imageHeight];
		uint32_t* iter = formattedData;

		uint8_t numChannels = bitCount / 8;
		uint32_t numBytes = imageWidth * imageHeight * numChannels;

		switch (numChannels)
		{
		default:
			break;
		case 3:
			for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 3)
			{
				*iter++ = 0xff000000 | filePtr[0] << 16 | filePtr[1] << 8 | filePtr[2];
				filePtr += 3;
			}
			break;
		case 4:
			for (uint32_t byteIdx = 0; byteIdx < numBytes; byteIdx += 4)
			{
				*iter++ = filePtr[3] << 24 | filePtr[0] << 16 | filePtr[1] << 8 | filePtr[2];
				filePtr += 4;
			}
			break;
		}

		Create2D(4 * imageWidth, imageWidth, imageHeight, sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, formattedData);

		delete[] formattedData;
	}

	bool Texture::CreateDDSFromMemory(const void* filePtr, size_t fileSize, bool sRGB)
	{
		if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_hCpuDescriptorHandle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		ResourceUploadBatch resourceUpload(DirectXImpl::D12Device);
		resourceUpload.Begin();

		return SUCCEEDED(CreateDDSTextureFromMemory(DirectXImpl::D12Device, resourceUpload, (const uint8_t*)filePtr, fileSize, &m_pResource, 0));
	}
}