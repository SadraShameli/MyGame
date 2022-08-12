#pragma once

#include "DirectXImpl.h"

#include "../Debugs/DebugHelpers.h"

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
		void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
		void SetRenderTargetFormats(UINT NumRTVs, const D3D12_RT_FORMAT_ARRAY* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);

		void SetVertexShader(ID3DBlob* Binary) { m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(Binary); }
		void SetPixelShader(ID3DBlob* Binary) { m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(Binary); }
		void SetGeometryShader(ID3DBlob* Binary) { m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(Binary); }
		void SetHullShader(ID3DBlob* Binary) { m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(Binary); }
		void SetDomainShader(ID3DBlob* Binary) { m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(Binary); }

		void SetVertexShader(IDxcBlob* Binary) { m_PSODesc.VS = { reinterpret_cast<UINT8*>(Binary->GetBufferPointer()), Binary->GetBufferSize() }; }
		void SetPixelShader(IDxcBlob* Binary) { m_PSODesc.PS = { reinterpret_cast<UINT8*>(Binary->GetBufferPointer()), Binary->GetBufferSize() }; }
		void SetGeometryShader(IDxcBlob* Binary) { m_PSODesc.GS = { reinterpret_cast<UINT8*>(Binary->GetBufferPointer()), Binary->GetBufferSize() }; }
		void SetHullShader(IDxcBlob* Binary) { m_PSODesc.HS = { reinterpret_cast<UINT8*>(Binary->GetBufferPointer()), Binary->GetBufferSize() }; }
		void SetDomainShader(IDxcBlob* Binary) { m_PSODesc.DS = { reinterpret_cast<UINT8*>(Binary->GetBufferPointer()), Binary->GetBufferSize() }; }

		virtual void Finalize();

	private:
		friend class CommandContext;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc = {};
	};

	class ComputePipelineState : public PipelineState
	{
	public:
		ComputePipelineState(const std::wstring& Name = L"Unnamed Compute PipelineState") : PipelineState(Name) {}

		void SetComputeShader(ID3DBlob* Binary) { m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(Binary); }

	private:
		friend class CommandContext;
		D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc = {};
	};
}