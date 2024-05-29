#pragma once

#include "DirectXImpl.h"
#include "RootSignature.h"

namespace MyGame
{
	class PipelineState
	{
	public:
		PipelineState(const std::wstring& Name) : m_Name(Name) {}

		void SetRootSignature(const RootSignature& BindMappings) { m_RootSignature = &BindMappings; }
		const RootSignature& GetRootSignature() const { return *m_RootSignature; }
		ID3D12PipelineState* GetPipelineStateObject() const { return m_PSO; }

	protected:
		std::wstring m_Name;
		const RootSignature* m_RootSignature = nullptr;
		ID3D12PipelineState* m_PSO = nullptr;
	};

	class GraphicsPSO : public PipelineState
	{
	public:
		GraphicsPSO(const std::wstring& Name = L"Unnamed Graphics PipelineState");

		void SetBlendState(const D3D12_BLEND_DESC& BlendDesc);
		void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc);
		void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc);
		void SetSampleMask(UINT SampleMask);
		void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
		void SetInputLayout(const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements);
		void SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
		void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN, UINT MsaaCount = 1, UINT MsaaQuality = 0);
		void SetRenderTargetFormats(UINT NumRTVs, const D3D12_RT_FORMAT_ARRAY* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);

		template<typename Blob>
		void SetVertexShader(Blob* data) { m_PSODesc.VS = { reinterpret_cast<UINT8*>(data->GetBufferPointer()), data->GetBufferSize() }; }
		template<typename Blob>
		void SetPixelShader(Blob* data) { m_PSODesc.PS = { reinterpret_cast<UINT8*>(data->GetBufferPointer()), data->GetBufferSize() }; }
		template<typename Blob>
		void SetGeometryShader(Blob* data) { m_PSODesc.GS = { reinterpret_cast<UINT8*>(data->GetBufferPointer()), data->GetBufferSize() }; }
		template<typename Blob>
		void SetHullShader(Blob* data) { m_PSODesc.HS = { reinterpret_cast<UINT8*>(data->GetBufferPointer()), data->GetBufferSize() }; }
		template<typename Blob>
		void SetDomainShader(Blob* data) { m_PSODesc.DS = { reinterpret_cast<UINT8*>(data->GetBufferPointer()), data->GetBufferSize() }; }

		void SetVertexShader(const void* data, size_t size) { m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(data), size); }
		void SetPixelShader(const void* data, size_t size) { m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(data), size); }
		void SetGeometryShader(const void* data, size_t size) { m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(data), size); }
		void SetHullShader(const void* data, size_t size) { m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(data), size); }
		void SetDomainShader(const void* data, size_t size) { m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(data), size); }

		void Finalize();

	private:
		friend class CommandContext;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc = {};
	};

	class ComputePSO : public PipelineState
	{
	public:
		ComputePSO(const std::wstring& Name = L"Unnamed Compute PipelineState") : PipelineState(Name) {}

		void SetComputeShader(ID3DBlob* data) { m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(data); }

	private:
		friend class CommandContext;
		D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc = {};
	};
}