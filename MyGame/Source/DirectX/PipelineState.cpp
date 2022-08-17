#include "CommonHeaders.h"

#include "PipelineState.h"

#include "../Debugs/DebugHelpers.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	GraphicsPSO::GraphicsPSO(const std::wstring& Name) : PipelineState(Name) {}

	void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& BlendDesc) { m_PSODesc.BlendState = BlendDesc; }
	void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc) { m_PSODesc.RasterizerState = RasterizerDesc; }
	void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc) { m_PSODesc.DepthStencilState = DepthStencilDesc; }
	void GraphicsPSO::SetSampleMask(UINT SampleMask) { m_PSODesc.SampleMask = SampleMask; }
	void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType) { m_PSODesc.PrimitiveTopologyType = TopologyType; }
	void GraphicsPSO::SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements) { m_PSODesc.InputLayout = { pInputElementDescs, NumElements }; }
	void GraphicsPSO::SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality) { SetRenderTargetFormats(0, nullptr, DSVFormat, MsaaCount, MsaaQuality); }
	void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
	{
		D3D12_RT_FORMAT_ARRAY RTVArray = {};
		RTVArray.NumRenderTargets = 1;
		RTVArray.RTFormats[0] = RTVFormat;
		SetRenderTargetFormats(RTVArray.NumRenderTargets, &RTVArray, DSVFormat, MsaaCount, MsaaQuality);
	}

	void GraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const D3D12_RT_FORMAT_ARRAY* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
	{
		for (UINT i = 0; i < NumRTVs; ++i)
			m_PSODesc.RTVFormats[i] = RTVFormats->RTFormats[i];
		m_PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		m_PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		m_PSODesc.DepthStencilState.DepthEnable = FALSE;
		m_PSODesc.DepthStencilState.StencilEnable = FALSE;
		m_PSODesc.SampleMask = UINT_MAX;
		m_PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_PSODesc.NumRenderTargets = NumRTVs;
		m_PSODesc.DSVFormat = DSVFormat;
		m_PSODesc.SampleDesc.Count = MsaaCount;
		m_PSODesc.SampleDesc.Quality = MsaaQuality;
	}

	void GraphicsPSO::Finalize()
	{
		m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
		MYGAME_ASSERT(m_PSODesc.pRootSignature != nullptr);
		MYGAME_ASSERT(m_PSODesc.DepthStencilState.DepthEnable != (m_PSODesc.DSVFormat == DXGI_FORMAT_UNKNOWN));

		ThrowIfFailed(DirectXImpl::D3D12_Device->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
		NAME_D3D12_OBJ_STR(m_PSO, m_Name);

		while (!m_PSO)
			std::this_thread::yield();
	}
}