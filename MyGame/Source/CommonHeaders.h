#pragma once

#include <algorithm>
#include <functional>

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <optional>

#include <utility>
#include <filesystem>
#include <fstream>

#include <cstdint>
#include <string> 
#include <string_view>

#include <future>
#include <mutex>
#include <thread>

#include <memory>
#include <vector>
#include <array>

// DirectX 12 API
#include <d3d12.h>
#include <D3dx12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// Debug
#ifdef MYGAME_DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

// Helpers
#include <PlatformHelpers.h>
#include "DirectXHelpers.h"