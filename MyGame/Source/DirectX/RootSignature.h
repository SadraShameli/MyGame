#pragma once

#include "../Debugs/DebugHelpers.h"

#include <d3dx12.h>

namespace MyGame
{
	class RootSignature
	{
	public:
		RootSignature(std::wstring&& Name = L"RootSignature");

		void Finalize(D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
		ID3D12RootSignature* GetSignature() const;

	private:
		BOOL m_Finalized;
		std::wstring m_Name;
		D3D12_ROOT_SIGNATURE_FLAGS m_Flags;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_Signature;
	};
}