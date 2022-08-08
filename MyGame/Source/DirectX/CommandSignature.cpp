#include "CommonHeaders.h"

#include "CommandSignature.h"
#include "RootSignature.h"
#include "DirectXImpl.h"

using namespace DirectX;

namespace MyGame
{
	void CommandSignature::Finalize(const RootSignature* RootSignature)
	{
		if (m_Finalized)
			return;

		UINT ByteStride = 0;
		bool RequiresRootSignature = false;

		for (UINT i = 0; i < m_NumParameters; ++i)
		{
			switch (m_ParamArray[i].GetDesc().Type)
			{
			case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
				ByteStride += sizeof(D3D12_DRAW_ARGUMENTS);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
				ByteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
				ByteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
				ByteStride += m_ParamArray[i].GetDesc().Constant.Num32BitValuesToSet * 4;
				RequiresRootSignature = true;
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
				ByteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
				ByteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
				break;
			case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
			case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
			case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
				ByteStride += 8;
				RequiresRootSignature = true;
				break;
			}
		}

		D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc = {};
		CommandSignatureDesc.ByteStride = ByteStride;
		CommandSignatureDesc.NumArgumentDescs = m_NumParameters;
		CommandSignatureDesc.pArgumentDescs = (const D3D12_INDIRECT_ARGUMENT_DESC*)m_ParamArray.get();

		ID3D12RootSignature* pRootSig = RootSignature ? RootSignature->GetSignature() : nullptr;
		if (RequiresRootSignature)
			assert(pRootSig != nullptr);
		else
			pRootSig = nullptr;

		ThrowIfFailed(DirectXImpl::m_device->CreateCommandSignature(&CommandSignatureDesc, pRootSig, IID_PPV_ARGS(&m_Signature)));
		NAME_D3D12_OBJECT(m_Signature.Get());
		m_Finalized = TRUE;
	}
}