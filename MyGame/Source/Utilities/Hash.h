#pragma once

#include "CommonMath.h"

#define ENABLE_SSE_CRC32 

#ifdef ENABLE_SSE_CRC32
#pragma intrinsic(_mm_crc32_u32)
#pragma intrinsic(_mm_crc32_u64)
#endif

namespace MyGame
{
	namespace Utility
	{
		inline size_t HashRange(const uint32_t* const Begin, const uint32_t* const End, size_t Hash)
		{

#ifdef ENABLE_SSE_CRC32
			const uint64_t* Iter64 = (const uint64_t*)Math::AlignUp(Begin, 8);
			const uint64_t* const End64 = (const uint64_t* const)Math::AlignDown(End, 8);

			if ((uint32_t*)Iter64 > Begin)
				Hash = _mm_crc32_u32((uint32_t)Hash, *Begin);

			while (Iter64 < End64)
				Hash = _mm_crc32_u64((uint64_t)Hash, *Iter64++);

			if ((uint32_t*)Iter64 < End)
				Hash = _mm_crc32_u32((uint32_t)Hash, *(uint32_t*)Iter64);
#else
			for (const uint32_t* Iter = Begin; Iter < End; ++Iter)
				Hash = 16777619U * Hash ^ *Iter;
#endif
			return Hash;
		}

		template <typename T> inline size_t HashState(const T* StateDesc, size_t Count = 1, size_t Hash = 2166136261U)
		{
			static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "State object is not word-aligned");
			return HashRange((uint32_t*)StateDesc, (uint32_t*)(StateDesc + Count), Hash);
		}
	}
}