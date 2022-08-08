#include "CommonHeaders.h"

#include "SamplerManager.h"
#include "DirectXImpl.h"

#include "../Utilities/Hash.h"
#include "../Debugs/DebugHelpers.h"

using namespace DirectX;

namespace MyGame
{
	std::map<size_t, D3D12_CPU_DESCRIPTOR_HANDLE> s_SamplerCache;

	D3D12_CPU_DESCRIPTOR_HANDLE SamplerDesc::CreateDescriptor()
	{
		size_t hashValue = Utility::HashState(this);
		auto iter = s_SamplerCache.find(hashValue);
		if (iter != s_SamplerCache.end())
		{
			return iter->second;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Handle = DirectXImpl::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		DirectXImpl::m_device->CreateSampler(this, Handle);
		return Handle;
	}

	void SamplerDesc::CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
	{
		MYGAME_ASSERT(Handle.ptr != 0 && Handle.ptr != -1);
		DirectXImpl::m_device->CreateSampler(this, Handle);
	}
}
