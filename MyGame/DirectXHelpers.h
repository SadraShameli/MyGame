#pragma once

#include "d3dx12.h"

inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
	pObject->SetName(name);
}

inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
	WCHAR fullName[50];
	if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
		pObject->SetName(fullName);
}

#ifdef MYGAME_DEBUG

#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

#else

#define NAME_D3D12_OBJECT(x) x
#define NAME_D3D12_OBJECT_INDEXED(x, n)
#endif