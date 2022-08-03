#pragma once

// DirectX 12 API
#include <d3d12.h>
#include <D3dx12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <d3dcompiler.h>
#pragma comment(lib, "dxcompiler.lib")

// Debug
#ifdef MYGAME_DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

// Helpers
#include <PlatformHelpers.h>
#include "DirectXHelpers.h"