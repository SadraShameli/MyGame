#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#ifdef MYGAME_DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#include <PlatformHelpers.h>

#include "DirectXHelpers.h"