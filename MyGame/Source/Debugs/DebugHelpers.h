#pragma once

#include "../Core/Log.h"

#include <filesystem>
#include <string>
#include <comdef.h>

#ifdef MYGAME_DEBUG
#define MYGAME_DEBUGBREAK() __debugbreak()
#define MYGAME_INTERNAL_ASSERT_IMPL(type, check, msg, ...) if(!(check)) { MYGAME_ERROR(msg, __VA_ARGS__); __debugbreak(); } 
#define MYGAME_INTERNAL_ASSERT_WITH_MSG(type, check, ...) MYGAME_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define MYGAME_INTERNAL_ASSERT_NO_MSG(type, check) MYGAME_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", #check, std::filesystem::path(__FILE__).filename().string(), __LINE__)
#define MYGAME_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define MYGAME_INTERNAL_ASSERT_GET_MACRO(...)  MYGAME_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, MYGAME_INTERNAL_ASSERT_WITH_MSG, MYGAME_INTERNAL_ASSERT_NO_MSG) 
#define MYGAME_ASSERT(...) MYGAME_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__)

#ifdef MYGAME_ENABLE_INFO_EVENTS 
#define MYGAME_INFO_EVENTS(x) MYGAME_INFO("{0}", x.ToString());
#else
#define MYGAME_INFO_EVENTS(x)
#endif 

#define MYGAME_HRESULT_TOSTR(x) if (FAILED(x)) MYGAME_ERROR(_com_error(x).ErrorMessage());

#else
#define HZ_DEBUGBREAK()
#define MYGAME_ASSERT(x, ...) (x)
#define MYGAME_INFO_EVENTS(x)

#define MYGAME_HRESULT_TOSTR(x) 

#endif

#define MYGAME_HRESULT_VERIFY(x) if (FAILED(x)) { MYGAME_HRESULT_TOSTR(x); return x; }
#define MYGAME_HRESULT_VALIDATE(x) if (FAILED(x)) { MYGAME_HRESULT_TOSTR(x); return ; }