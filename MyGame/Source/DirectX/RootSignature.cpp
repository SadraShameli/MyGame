#include "CommonHeaders.h"

#include "RootSignature.h"
#include "DirectXImpl.h"
#include "DirectXHelpers.h"

#include "../Core/Log.h"
#include "../Debugs/DebugHelpers.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	RootSignature::RootSignature(std::wstring&& Name)
	{
		m_Finalized = false;
		m_Name = std::move(Name);
		m_Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		m_Signature = nullptr;
	}

	void RootSignature::Finalize(D3D12_ROOT_SIGNATURE_FLAGS Flags)
	{
		if (m_Finalized)
			return;
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(DirectXImpl::D12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		// A single 32-bit constant root parameter that is used by the vertex shader.
		CD3DX12_ROOT_PARAMETER1 rootParameters[1] = {};
		rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		// Serialize the root signature.
		ComPtr<ID3DBlob> rootSignatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
			featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
		// Create the root signature.
		ThrowIfFailed(DirectXImpl::D12Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_Signature)));

		NAME_D3D12_OBJ_STR(m_Signature, m_Name);

		m_Finalized = true;
	}

	ID3D12RootSignature* RootSignature::GetSignature() const { return m_Signature.Get(); }
}