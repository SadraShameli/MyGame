#pragma once

#include "DirectXImpl.h"

#include "../Debugs/DebugHelpers.h"

namespace MyGame
{
	class PipelineState
	{
	public:
		PipelineState(const wchar_t* Name) : m_Name(Name), m_RootSignature(nullptr), m_PSO(nullptr) {}

		static void DestroyAll();

		void SetRootSignature(const RootSignature& BindMappings) { m_RootSignature = &BindMappings; }
		const RootSignature& GetRootSignature() const
		{
			assert(m_RootSignature != nullptr);
			return *m_RootSignature;
		}

		ID3D12PipelineState* GetPipelineStateObject(void) const { return m_PSO; }

	protected:
		const wchar_t* m_Name;
		const RootSignature* m_RootSignature;
		ID3D12PipelineState* m_PSO;
	};

	class GraphicsPSO : public PipelineState
	{
	public:
		GraphicsPSO(const wchar_t* Name = L"Unnamed Graphics PipelineState");

		void SetBlendState(const D3D12_BLEND_DESC& BlendDesc);
		void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc);
		void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc);
		void SetSampleMask(UINT SampleMask);
		void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
		void SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
		void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
		void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
		void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);
		void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

		void SetVertexShader(const void* Binary, size_t Size) { m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
		void SetPixelShader(const void* Binary, size_t Size) { m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
		void SetGeometryShader(const void* Binary, size_t Size) { m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
		void SetHullShader(const void* Binary, size_t Size) { m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
		void SetDomainShader(const void* Binary, size_t Size) { m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }

		void SetVertexShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.VS = Binary; }
		void SetPixelShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.PS = Binary; }
		void SetGeometryShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.GS = Binary; }
		void SetHullShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.HS = Binary; }
		void SetDomainShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.DS = Binary; }

		void Finalize();

	private:
		friend class CommandContext;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
		std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;
	};

	class ComputePipelineState : public PipelineState
	{
	public:
		ComputePipelineState(const wchar_t* Name = L"Unnamed Compute PipelineState");

		void SetComputeShader(const void* Binary, size_t Size) { m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(Binary), Size); }
		void SetComputeShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.CS = Binary; }

		void Finalize();

	private:
		friend class CommandContext;
		D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;
	};
}