#pragma once

#include "../Utilities/Utility.h"

namespace MyGame
{
	template <typename Object>
	inline void SetName(Object object, const wchar_t* name) { object->SetName(name); }
	template <typename Object>
	inline void SetName(Object object, const std::wstring& name) { SetName(object, name.c_str()); }
	template <typename Object>
	inline void SetNameIndexed(Object object, const wchar_t* name, UINT index)
	{
		WCHAR fullName[50];
		if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
			object->SetName(fullName);
	}
	template <typename Object>
	inline void SetNameIndexed(Object object, const std::wstring& name, UINT index) { SetNameIndexed(object, name.c_str(), index); }

	inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
	{
		if (path == nullptr) throw std::exception();
		DWORD size = GetModuleFileName(nullptr, path, pathSize);
		if (size == 0 || size == pathSize) throw std::exception();
		WCHAR* lastSlash = wcsrchr(path, L'\\');
		if (lastSlash) *(lastSlash + 1) = L'\0';
	}
}

#ifdef MYGAME_DEBUG
#define NAME_D3D12_OBJ(x) SetName(x, L#x)
#define NAME_D3D12_OBJ_STR(x, s) SetName(x, s)
#define NAME_D3D12_OBJ_INDEXED(x, n) SetNameIndexed(x, L#x, n)
#define NAME_D3D12_OBJ_INDEXED_STR(x, s, n) SetNameIndexed(x, s, n)

#define NAME_D3D12_WRL(x) SetName(x.Get(), L#x)
#define NAME_D3D12_WRL_STR(x, s) SetName(x.Get(), s)
#define NAME_D3D12_WRL_INDEXED(x, n) SetNameIndexed(x.Get(), L#x, n)
#define NAME_D3D12_WRL_INDEXED_STR(x, s, n) SetNameIndexed(x.Get(), s, n)

#else
#define NAME_D3D12_OBJ(x)
#define NAME_D3D12_OBJ_STR(x, s)
#define NAME_D3D12_OBJ_INDEXED(x, n)
#define NAME_D3D12_OBJ_INDEXED_STR(x, s, n)

#define NAME_D3D12_WRL(x)
#define NAME_D3D12_WRL_STR(x, s) 
#define NAME_D3D12_WRL_INDEXED(x, n) 
#define NAME_D3D12_WRL_INDEXED_STR(x, s, n)
#endif

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN ((D3D12_GPU_VIRTUAL_ADDRESS)-1)