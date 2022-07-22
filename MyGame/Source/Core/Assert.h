#pragma once

#include "Source/Core/Base.h"
#include "Source/Core/Log.h"
#include <filesystem>

#ifdef MYGAME_DEBUG

#define MYGAME_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { MYGAME_ERROR(msg, __VA_ARGS__); __debugbreak(); } }
#define MYGAME_INTERNAL_ASSERT_WITH_MSG(type, check, ...) MYGAME_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define MYGAME_INTERNAL_ASSERT_NO_MSG(type, check) MYGAME_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", #check, std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define MYGAME_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define MYGAME_INTERNAL_ASSERT_GET_MACRO(...)  MYGAME_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, MYGAME_INTERNAL_ASSERT_WITH_MSG, MYGAME_INTERNAL_ASSERT_NO_MSG) 

#define MYGAME_ASSERT(...) MYGAME_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__)
#else
#define MYGAME_ASSERT(x, ...) x
#endif
