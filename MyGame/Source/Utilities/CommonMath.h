#pragma once

#include <DirectXMath.h>
#include <intrin.h>

namespace MyGame
{
	namespace Math
	{
		template <typename T> __forceinline T AlignUpWithMask(T value, size_t mask)
		{
			return (T)(((size_t)value + mask) & ~mask);
		}

		template <typename T> __forceinline T AlignDownWithMask(T value, size_t mask)
		{
			return (T)((size_t)value & ~mask);
		}

		template <typename T> __forceinline T AlignUp(T value, size_t alignment)
		{
			return AlignUpWithMask(value, alignment - 1);
		}

		template <typename T> __forceinline T AlignDown(T value, size_t alignment)
		{
			return AlignDownWithMask(value, alignment - 1);
		}

		template <typename T> __forceinline bool IsAligned(T value, size_t alignment)
		{
			return 0 == ((size_t)value & (alignment - 1));
		}

		template <typename T> __forceinline T DivideByMultiple(T value, size_t alignment)
		{
			return (T)((value + alignment - 1) / alignment);
		}

		template <typename T> __forceinline bool IsPowerOfTwo(T value)
		{
			return 0 == (value & (value - 1));
		}

		template <typename T> __forceinline bool IsDivisible(T value, T divisor)
		{
			return (value / divisor) * divisor == value;
		}

		__forceinline uint8_t Log2(uint64_t value)
		{
			unsigned long mssb;
			unsigned long lssb;

			if (_BitScanReverse64(&mssb, value) > 0 && _BitScanForward64(&lssb, value) > 0)
				return uint8_t(mssb + (mssb == lssb ? 0 : 1));
			else
				return 0;
		}

		template <typename T> __forceinline T AlignPowerOfTwo(T value)
		{
			return value == 0 ? 0 : 1 << Log2(value);
		}

		__forceinline DirectX::XMVECTOR SplatZero()
		{
			return DirectX::XMVectorZero();
		}

#ifdef _XM_SSE_INTRINSICS_

		__forceinline DirectX::XMVECTOR SplatOne(DirectX::XMVECTOR zero = SplatZero())
		{
			__m128i AllBits = _mm_castps_si128(_mm_cmpeq_ps(zero, zero));
			return _mm_castsi128_ps(_mm_slli_epi32(_mm_srli_epi32(AllBits, 25), 23));
			//return _mm_cvtepi32_ps(_mm_srli_epi32(SetAllBits(zero), 31));				
		}

#ifdef MYGAME_USE_SSE
		__forceinline DirectX::XMVECTOR CreateXUnitVector(DirectX::XMVECTOR one = SplatOne())
		{
			return _mm_insert_ps(one, one, 0x0E);
		}
		__forceinline DirectX::XMVECTOR CreateYUnitVector(DirectX::XMVECTOR one = SplatOne())
		{
			return _mm_insert_ps(one, one, 0x0D);
		}
		__forceinline DirectX::XMVECTOR CreateZUnitVector(DirectX::XMVECTOR one = SplatOne())
		{
			return _mm_insert_ps(one, one, 0x0B);
		}
		__forceinline DirectX::XMVECTOR CreateWUnitVector(DirectX::XMVECTOR one = SplatOne())
		{
			return _mm_insert_ps(one, one, 0x07);
		}
		__forceinline DirectX::XMVECTOR SetWToZero(DirectX::FXMVECTOR vec)
		{
			return _mm_insert_ps(vec, vec, 0x08);
		}
		__forceinline DirectX::XMVECTOR SetWToOne(DirectX::FXMVECTOR vec)
		{
			return _mm_blend_ps(vec, SplatOne(), 0x8);
		}
#else
		__forceinline DirectX::XMVECTOR CreateXUnitVector(DirectX::XMVECTOR one = SplatOne())
		{
			return _mm_castsi128_ps(_mm_srli_si128(_mm_castps_si128(one), 12));
		}
		__forceinline DirectX::XMVECTOR CreateYUnitVector(DirectX::XMVECTOR one = SplatOne())
		{
			DirectX::XMVECTOR unitx = CreateXUnitVector(one);
			return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(unitx), 4));
		}
		__forceinline DirectX::XMVECTOR CreateZUnitVector(DirectX::XMVECTOR one = SplatOne())
		{
			DirectX::XMVECTOR unitx = CreateXUnitVector(one);
			return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(unitx), 8));
		}
		__forceinline DirectX::XMVECTOR CreateWUnitVector(DirectX::XMVECTOR one = SplatOne())
		{
			return _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(one), 12));
		}
		__forceinline DirectX::XMVECTOR SetWToZero(DirectX::FXMVECTOR vec)
		{
			__m128i MaskOffW = _mm_srli_si128(_mm_castps_si128(_mm_cmpeq_ps(vec, vec)), 4);
			return _mm_and_ps(vec, _mm_castsi128_ps(MaskOffW));
		}
		__forceinline DirectX::XMVECTOR SetWToOne(DirectX::FXMVECTOR vec)
		{
			return _mm_movelh_ps(vec, _mm_unpackhi_ps(vec, SplatOne()));
		}
#endif

#else 
		__forceinline DirectX::XMVECTOR SplatOne() { return XMVectorSplatOne(); }
		__forceinline DirectX::XMVECTOR CreateXUnitVector() { return g_XMIdentityR0; }
		__forceinline DirectX::XMVECTOR CreateYUnitVector() { return g_XMIdentityR1; }
		__forceinline DirectX::XMVECTOR CreateZUnitVector() { return g_XMIdentityR2; }
		__forceinline DirectX::XMVECTOR CreateWUnitVector() { return g_XMIdentityR3; }
		__forceinline DirectX::XMVECTOR SetWToZero(DirectX::FXMVECTOR vec) { return XMVectorAndInt(vec, g_XMMask3); }
		__forceinline DirectX::XMVECTOR SetWToOne(DirectX::FXMVECTOR vec) { return XMVectorSelect(g_XMIdentityR3, vec, g_XMMask3); }

#endif
		enum EZeroTag { kZero, kOrigin };
		enum EIdentityTag { kOne, kIdentity };
		enum EXUnitVector { kXUnitVector };
		enum EYUnitVector { kYUnitVector };
		enum EZUnitVector { kZUnitVector };
		enum EWUnitVector { kWUnitVector };
	}
}