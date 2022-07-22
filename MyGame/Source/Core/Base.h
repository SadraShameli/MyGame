#pragma once

#include "Source/Core/Log.h"
#include "Source/Core/Assert.h"

#ifdef MYGAME_DEBUG
#define MYGAME_DEBUGBREAK() __debugbreak()
#define MYGAME_ENABLE_ASSERTS
#else
#define HZ_DEBUGBREAK()
#endif

#define MYGAME_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }