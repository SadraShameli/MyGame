#pragma once

#include <d3dx12.h>

template <typename Object, typename Name>
inline void SetName(Object object, Name name) { object->SetName(reinterpret_cast<LPCWSTR>(name)); }
template <typename Object>
inline void SetNameIndexed(Object object, LPCWSTR name, UINT index)
{
	WCHAR fullName[50];
	if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
		object->SetName(fullName);
}

#ifdef MYGAME_DEBUG
#define NAME_D3D12_OBJECT(x) SetName((x), L#x)
#define NAME_D3D12_OBJECT_STR(x, s) SetName((x), s)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x), L#x, n)

#else

#define NAME_D3D12_OBJECT(x)
#define NAME_D3D12_OBJECT_STR(x, s)
#define NAME_D3D12_OBJECT_INDEXED(x, n)
#endif

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN ((D3D12_GPU_VIRTUAL_ADDRESS)-1)