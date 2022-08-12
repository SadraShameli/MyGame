#include "CommonHeaders.h"

#include "ColorBuffer.h"
#include "CommandContext.h"

namespace MyGame
{
	void ColorBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
	{
		MYGAME_ASSERT(ArraySize == 1 || NumMips == 1, "We don't support auto-mips on texture arrays");
		m_NumMipMaps = NumMips - 1;

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

		RTVDesc.Format = Format;
		UAVDesc.Format = GetUAVFormat(Format);
		SRVDesc.Format = Format;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (ArraySize > 1)
		{
			RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			RTVDesc.Texture2DArray.MipSlice = 0;
			RTVDesc.Texture2DArray.FirstArraySlice = 0;
			RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

			UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			UAVDesc.Texture2DArray.MipSlice = 0;
			UAVDesc.Texture2DArray.FirstArraySlice = 0;
			UAVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

			SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			SRVDesc.Texture2DArray.MipLevels = NumMips;
			SRVDesc.Texture2DArray.MostDetailedMip = 0;
			SRVDesc.Texture2DArray.FirstArraySlice = 0;
			SRVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;
		}
		else if (m_FragmentCount > 1)
		{
			RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			RTVDesc.Texture2D.MipSlice = 0;

			UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			UAVDesc.Texture2D.MipSlice = 0;

			SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = NumMips;
			SRVDesc.Texture2D.MostDetailedMip = 0;
		}

		if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_RTVHandle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			m_SRVHandle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		ID3D12Resource* Resource = m_pResource;
		Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle);
		Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);

		if (m_FragmentCount > 1)
			return;

		for (auto& handle : m_UAVHandle)
		{
			if (handle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
				handle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, handle);
			UAVDesc.Texture2D.MipSlice++;
		}
	}

	void ColorBuffer::CreateFromSwapChain(const std::wstring& Name, ID3D12Resource* BaseResource)
	{
		AssociateWithResource(DirectXImpl::D12Device, Name, BaseResource, D3D12_RESOURCE_STATE_PRESENT);

		m_RTVHandle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		DirectXImpl::D12Device->CreateRenderTargetView(m_pResource, nullptr, m_RTVHandle);
	}

	void ColorBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMem)
	{
		NumMips = (NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);
		D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
		D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, NumMips, Format, Flags);

		ResourceDesc.SampleDesc.Count = m_FragmentCount;
		ResourceDesc.SampleDesc.Quality = 0;

		D3D12_CLEAR_VALUE ClearValue = {};
		ClearValue.Format = Format;
		ClearValue.Color[0] = m_ClearColor.R();
		ClearValue.Color[1] = m_ClearColor.G();
		ClearValue.Color[2] = m_ClearColor.B();
		ClearValue.Color[3] = m_ClearColor.A();

		CreateTextureResource(DirectXImpl::D12Device, Name, ResourceDesc, ClearValue, VidMem);
		CreateDerivedViews(DirectXImpl::D12Device, Format, 1, NumMips);
	}

	void ColorBuffer::CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMem)
	{
		D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
		D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, ArrayCount, 1, Format, Flags);

		D3D12_CLEAR_VALUE ClearValue = {};
		ClearValue.Format = Format;
		ClearValue.Color[0] = m_ClearColor.R();
		ClearValue.Color[1] = m_ClearColor.G();
		ClearValue.Color[2] = m_ClearColor.B();
		ClearValue.Color[3] = m_ClearColor.A();

		CreateTextureResource(DirectXImpl::D12Device, Name, ResourceDesc, ClearValue, VidMem);
		CreateDerivedViews(DirectXImpl::D12Device, Format, ArrayCount, 1);
	}

	void ColorBuffer::GenerateMipMaps(CommandContext& BaseContext)
	{
		if (m_NumMipMaps == 0)
			return;

		ComputeContext& Context = BaseContext.GetComputeContext();
		Context.TransitionResource(*this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		Context.SetDynamicDescriptor(1, 0, m_SRVHandle);

		for (uint32_t TopMip = 0; TopMip < m_NumMipMaps; )
		{
			uint32_t SrcWidth = m_Width >> TopMip;
			uint32_t SrcHeight = m_Height >> TopMip;
			uint32_t DstWidth = SrcWidth >> 1;
			uint32_t DstHeight = SrcHeight >> 1;
			uint32_t NonPowerOfTwo = (SrcWidth & 1) | (SrcHeight & 1) << 1;

			uint32_t AdditionalMips = 0;
			_BitScanForward((unsigned long*)&AdditionalMips, (DstWidth == 1 ? DstHeight : DstWidth) | (DstHeight == 1 ? DstWidth : DstHeight));
			uint32_t NumMips = 1 + (AdditionalMips > 3 ? 3 : AdditionalMips);

			if (TopMip + NumMips > m_NumMipMaps)
				NumMips = m_NumMipMaps - TopMip;
			if (DstWidth == 0)
				DstWidth = 1;
			if (DstHeight == 0)
				DstHeight = 1;

			Context.SetConstants(0, TopMip, NumMips, 1.0f / DstWidth, 1.0f / DstHeight);
			Context.SetDynamicDescriptors(2, 0, NumMips, m_UAVHandle + TopMip + 1);
			Context.Dispatch2D(DstWidth, DstHeight);
			Context.InsertUAVBarrier(*this);
			TopMip += NumMips;
		}

		Context.TransitionResource(*this, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
}