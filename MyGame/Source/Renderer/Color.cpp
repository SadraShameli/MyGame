#include "CommonHeaders.h"

#include "Color.h"

using namespace DirectX;

namespace MyGame
{
	uint32_t Color::R11G11B10F(bool RoundToEven) const
	{

#ifdef MYGAME_USE_SSE
		static XMVECTORU32 Scale = { 0x07800000, 0x07800000, 0x07800000, 0 };
		static XMVECTORU32 Round1 = { 0x00010000, 0x00010000, 0x00020000, 0 };
		static XMVECTORU32 Round2 = { 0x0000FFFF, 0x0000FFFF, 0x0001FFFF, 0 };
		static XMVECTORU32 Mask = { 0x0FFE0000, 0x0FFE0000, 0x0FFC0000, 0 };

		__m128i ti = _mm_max_epi32(_mm_castps_si128(m_value), _mm_setzero_si128());
		ti = _mm_min_epi32(ti, _mm_set1_epi32(0x47800000));
		ti = _mm_castps_si128(_mm_mul_ps(_mm_castsi128_ps(ti), Scale));

		if (RoundToEven)
			ti = _mm_add_epi32(ti, _mm_max_epi32(_mm_and_si128(_mm_srli_epi32(ti, 1), Round1), Round2));
		else
			ti = _mm_add_epi32(ti, Round1);

		XMVECTORU32 ret;
		ret.v = _mm_castsi128_ps(_mm_and_si128(ti, Mask));
		return ret.u[0] >> 17 | ret.u[1] >> 6 | ret.u[2] << 4;
#else
		static const float kMaxVal = float(1 << 16);
		static const float kF32toF16 = (1.0 / (1ull << 56)) * (1.0 / (1ull << 56));
		union { float f; uint32_t u; } R, G, B;

		R.f = std::clamp(m_value.f[0], 0.0f, kMaxVal) * kF32toF16;
		G.f = std::clamp(m_value.f[1], 0.0f, kMaxVal) * kF32toF16;
		B.f = std::clamp(m_value.f[2], 0.0f, kMaxVal) * kF32toF16;

		if (RoundToEven)
		{
			R.u += 0x0FFFF + ((R.u >> 16) & 1);
			G.u += 0x0FFFF + ((G.u >> 16) & 1);
			B.u += 0x1FFFF + ((B.u >> 17) & 1);
		}
		else
		{
			R.u += 0x00010000;
			G.u += 0x00010000;
			B.u += 0x00020000;
		}

		R.u &= 0x0FFE0000;
		G.u &= 0x0FFE0000;
		B.u &= 0x0FFC0000;

		return R.u >> 17 | G.u >> 6 | B.u << 4;
#endif
	}

	uint32_t Color::R9G9B9E5() const
	{

#ifdef MYGAME_USE_SSE
		__m128 kMaxVal = _mm_castsi128_ps(_mm_set1_epi32(0x477F8000));
		__m128 rgb = _mm_min_ps(_mm_max_ps(m_value, _mm_setzero_ps()), kMaxVal);

		__m128 kMinVal = _mm_castsi128_ps(_mm_set1_epi32(0x37800000));
		__m128 MaxChannel = _mm_max_ps(rgb, kMinVal);
		MaxChannel = _mm_max_ps(_mm_permute_ps(MaxChannel, _MM_SHUFFLE(3, 1, 0, 2)), _mm_max_ps(_mm_permute_ps(MaxChannel, _MM_SHUFFLE(3, 0, 2, 1)), MaxChannel));

		__m128i kBias15 = _mm_set1_epi32(0x07804000);
		__m128i kExpMask = _mm_set1_epi32(0x7F800000);
		__m128i Bias = _mm_and_si128(_mm_add_epi32(_mm_castps_si128(MaxChannel), kBias15), kExpMask);
		rgb = _mm_add_ps(rgb, _mm_castsi128_ps(Bias));
		__m128i Exp = _mm_add_epi32(_mm_slli_epi32(Bias, 4), _mm_set1_epi32(0x10000000));

		XMVECTORU32 ret;
		ret.v = _mm_insert_ps(rgb, _mm_castsi128_ps(Exp), 0x30);
		return ret.u[3] | ret.u[2] << 18 | ret.u[1] << 9 | ret.u[0] & 511;
#else 
		static const float kMaxVal = float(0x1FF << 7);
		static const float kMinVal = float(1.f / (1 << 16));

		float r = std::clamp(m_value.f[0], 0.0f, kMaxVal);
		float g = std::clamp(m_value.f[1], 0.0f, kMaxVal);
		float b = std::clamp(m_value.f[2], 0.0f, kMaxVal);

		float MaxChannel = (std::max)((std::max)(r, g), (std::max)(b, kMinVal));

		union { float f; int32_t i; } R, G, B, E;
		E.f = MaxChannel;
		E.i += 0x07804000;
		E.i &= 0x7F800000;

		R.f = r + E.f;
		G.f = g + E.f;
		B.f = b + E.f;

		E.i <<= 4;
		E.i += 0x10000000;

		return E.i | B.i << 18 | G.i << 9 | R.i & 511;
#endif
	}
}