#pragma once

#ifdef _DEBUG
	#define INTRINSIC_INLINE inline
#else
	#define INTRINSIC_INLINE __forceinline
#endif

// Round-to-nearest is used when converting from floats to ints instead of C/C++'s default "round towards zero": https://msdn.microsoft.com/en-us/library/windows/desktop/bb147214(v=vs.85).aspx
static INTRINSIC_INLINE const int RoundToNearest(const float f)
{
	if (f >= 0.0f)
		return (const int)(f + 0.5f);
	else
		return (const int)(f - 0.5f);
}

#include "ShaderIntrinsicFunctions4.h"

// Absolute-value: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/abs---vs
static INTRINSIC_INLINE void abs(D3DXVECTOR4& dst, const D3DXVECTOR4& src)
{
	dst.x = fabsf(src.x);
	dst.y = fabsf(src.y);
	dst.z = fabsf(src.z);
	dst.w = fabsf(src.w);
}

// Absolute-value: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/abs---vs
static INTRINSIC_INLINE void abs(float& dst, const float src)
{
	dst = fabsf(src);
}

// Addition: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/add---vs
static INTRINSIC_INLINE void add(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.x = src0.x + src1.x;
	dst.y = src0.y + src1.y;
	dst.z = src0.z + src1.z;
	dst.w = src0.w + src1.w;
}

// Addition: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/add---vs
static INTRINSIC_INLINE void add(float& dst, const float src0, const float src1)
{
	dst = src0 + src1;
}

// Compare: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/cmp---ps
static INTRINSIC_INLINE void cmp(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2)
{
	dst.x = src0.x >= 0.0f ? src1.x : src2.x;
	dst.y = src0.y >= 0.0f ? src1.y : src2.y;
	dst.z = src0.z >= 0.0f ? src1.z : src2.z;
	dst.w = src0.w >= 0.0f ? src1.w : src2.w;
}

// Compare: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/cmp---ps
static INTRINSIC_INLINE void cmp(float& dst, const float src0, const float src1, const float src2)
{
	dst = src0 >= 0.0f ? src1 : src2;
}

// Conditional select: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/cnd---ps
static INTRINSIC_INLINE void cnd(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2)
{
	dst.x = src0.x > 0.5f ? src1.x : src2.x;
	dst.y = src0.y > 0.5f ? src1.y : src2.y;
	dst.z = src0.z > 0.5f ? src1.z : src2.z;
	dst.w = src0.w > 0.5f ? src1.w : src2.w;
}

// Cross-product: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/crs---vs
static INTRINSIC_INLINE void crs(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.x = src0.y * src1.z - src0.z * src1.y;
	dst.y = src0.z * src1.x - src0.x * src1.z;
	dst.z = src0.x * src1.y - src0.y * src1.x;
}

// 2-D dot product and add scalar: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp2add---ps
static INTRINSIC_INLINE void dp2add(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y);
	dst.x = dst.y = dst.z = dst.w;
	dst.x += src2.x;
	dst.y += src2.y;
	dst.z += src2.z;
	dst.w += src2.w;
}

// 2-D dot product and add scalar: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp2add---ps
static INTRINSIC_INLINE void dp2add(float4& dst, const float4& src0, const float4& src1, const float4& src2)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y);
	dst.x = dst.y = dst.z = dst.w;
	dst.x += src2.x;
	dst.y += src2.y;
	dst.z += src2.z;
	dst.w += src2.w;
}

// 2-D dot product and add scalar: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp2add---ps
static INTRINSIC_INLINE void dp2add1(float& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const float src2)
{
	dst = (src0.x * src1.x) + (src0.y * src1.y);
	dst += src2;
}

// dot3: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp3---vs
static INTRINSIC_INLINE void dp3(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z);
	dst.x = dst.y = dst.z = dst.w;
}

// dot3: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp3---vs
static INTRINSIC_INLINE void dp3(float4& dst, const float4& src0, const float4& src1)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z);
	dst.x = dst.y = dst.z = dst.w;
}

// dot3: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp3---vs
static INTRINSIC_INLINE void dp3(float4& dst, const D3DXVECTOR4& src0, const float4& src1)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z);
	dst.x = dst.y = dst.z = dst.w;
}

// dot3: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp3---vs
static INTRINSIC_INLINE void dp3(float4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z);
	dst.x = dst.y = dst.z = dst.w;
}

// dot3: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp3---vs
static INTRINSIC_INLINE void dp3(float& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z);
}

// dot3: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp3---vs
static INTRINSIC_INLINE void dp3(float& dst, const float x0, const float y0, const float z0, const float x1, const float y1, const float z1)
{
	dst = (x0 * x1) + (y0 * y1) + (z0 * z1);
}

// dot4: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp4---ps
static INTRINSIC_INLINE void dp4(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y) + 
         (src0.z * src1.z) + (src0.w * src1.w);
	dst.x = dst.y = dst.z = dst.w;
}

// dot4: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp4---ps
static INTRINSIC_INLINE void dp4(float4& dst, const float4& src0, const float4& src1)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y) + 
         (src0.z * src1.z) + (src0.w * src1.w);
	dst.x = dst.y = dst.z = dst.w;
}

// dot4: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp4---ps
static INTRINSIC_INLINE void dp4(float& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst = (src0.x * src1.x) + (src0.y * src1.y) + 
         (src0.z * src1.z) + (src0.w * src1.w);
}

// dot4: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp4---ps
static INTRINSIC_INLINE void dp4(float4& dst, const D3DXVECTOR4& src0, const float4& src1)
{
	dst.w = (src0.x * src1.x) + (src0.y * src1.y) + 
         (src0.z * src1.z) + (src0.w * src1.w);
	dst.x = dst.y = dst.z = dst.w;
}

// distance: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dst---vs
/*
// The source vectors are assumed to contain the values shown in the following pseudocode:
src0.x = undefined
src0.y = d * d
src0.z = d * d
src0.w = undefined
src1.x = undefined
src1.y = 1 / d
src1.z = undefined
src1.w = 1 / d

// The destination vector is assumed to look like this after the dst() function is executed:
dest.x = 1
dest.y = d
dest.z = d * d
dest.w = 1 / d
*/
static INTRINSIC_INLINE void dst(D3DXVECTOR4& dest, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dest.x = 1.0f;
	dest.y = sqrtf(src0.y);
	dest.z = src0.z;
	dest.w = src1.w;
}

// ddx/dsx: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dsx---ps
static INTRINSIC_INLINE void dsx(D3DXVECTOR4& dst, const D3DXVECTOR4& src0)
{
	UNREFERENCED_PARAMETER(src0);

	// We can't compute derivatives for just 1-pixel warps, so return zero for everything (need at least a 2x2 quad warp to compute this)
	dst.x = dst.y = dst.z = dst.w = 0.0f;
}

// ddy/dsy: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dsy---ps
static INTRINSIC_INLINE void dsy(D3DXVECTOR4& dst, const D3DXVECTOR4& src0)
{
	UNREFERENCED_PARAMETER(src0);

	// We can't compute derivatives for just 1-pixel warps, so return zero for everything (need at least a 2x2 quad warp to compute this)
	dst.x = dst.y = dst.z = dst.w = 0.0f;
}

// exp (minimum 21 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/exp---vs
// expp (minimum 10 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/expp---vs
static INTRINSIC_INLINE void exp(D3DXVECTOR4& dst, const float src_replicateSwizzleComponent)
{
	dst.x = dst.y = dst.z = dst.w = (const float)powf(2.0f, src_replicateSwizzleComponent);
}

// exp (minimum 21 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/exp---vs
// expp (minimum 10 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/expp---vs
static INTRINSIC_INLINE void exp(float& dst, const float src_replicateSwizzleComponent)
{
	dst = (const float)powf(2.0f, src_replicateSwizzleComponent);
}

// Frac: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/frc---vs
static INTRINSIC_INLINE void frc(D3DXVECTOR4& dst, const D3DXVECTOR4& src)
{
	dst.x = src.x - floorf(src.x);
	dst.y = src.y - floorf(src.y);
	dst.z = src.z - floorf(src.z);
	dst.w = src.w - floorf(src.w);
}

// Frac: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/frc---vs
static INTRINSIC_INLINE void frc(float& dst, const float src)
{
	dst = src - (const float)floor(src);
}

// lighting: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/lit---vs
/*
// The source vector is assumed to contain the values shown in the following pseudocode.
src.x = N dot L    ; The dot product between normal and direction to light (for diffuse lighting)
src.y = N dot H    ; The dot product between normal and half vector (for specular lighting)
src.z = ignored    ; This value is ignored
src.w = exponent   ; The value must be between -128.0 and 128.0 (for specular lighting)
*/
static INTRINSIC_INLINE void lit(D3DXVECTOR4& dst, const D3DXVECTOR4& src0)
{
	dst.x = 1.0f;
	dst.y = 0.0f;
	dst.z = 0.0f;
	dst.w = 1.0f;

	if (src0.x > 0.0f)
	{
		dst.y = src0.x; // This is N dot L (for diffuse lighting)

		// Do specular lighting:
		if (src0.y > 0.0f)
		{
			const float MAXPOWER = 127.9961f; // An implementation must support at least eight fraction bits in the power argument (8.8 fixed point)
			float power = src0.w;
			if (power < -MAXPOWER)
				power = -MAXPOWER;
			else if (power > MAXPOWER)
				power = MAXPOWER;
			dst.z = powf(src0.y, power);
		}
	}
}

// Log (minimum 21 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/log---vs
// Logp (minimum 10 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/logp---vs
static INTRINSIC_INLINE void log(D3DXVECTOR4& dst, const float src_replicateSwizzleComponent)
{
	const float v = fabsf(src_replicateSwizzleComponent);
	if (v != 0.0f)
	{
		dst.x = dst.y = dst.z = dst.w = log2_lowp(v);
	}
	else
	{
		dst.x = dst.y = dst.z = dst.w = -FLT_MAX;
	}
}

// Log (minimum 21 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/log---vs
// Logp (minimum 10 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/logp---vs
static INTRINSIC_INLINE void log(float& dst, const float src_replicateSwizzleComponent)
{
	const float v = fabsf(src_replicateSwizzleComponent);
	if (v != 0.0f)
	{
		dst = log2_lowp(v);
	}
	else
	{
		dst = -FLT_MAX;
	}
}

// lerp: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/lrp---vs
template <const unsigned char writeMask = 0xF>
static INTRINSIC_INLINE void lrp(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2)
{
	const __m128 amount4 = *(const __m128* const)&src0;
	const __m128 src1vec = *(const __m128* const)&src1;
	const __m128 src2vec = *(const __m128* const)&src2;
	__m128& outVec = *(__m128* const)&dst;

	if ( (writeMask & 0x7) == 0x7)
		outVec = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec, src1vec), amount4), src1vec);
	else
	{
		const __m128 result = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec, src1vec), amount4), src1vec);
		if (writeMask & 0x1)
			dst.x = result.m128_f32[0];
		if (writeMask & 0x2)
			dst.y = result.m128_f32[1];
		if (writeMask & 0x4)
			dst.z = result.m128_f32[2];
		if (writeMask & 0x8)
			dst.w = result.m128_f32[3];
	}
}

// lerp: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/lrp---vs
template <const unsigned char writeMask = 0xF>
static INTRINSIC_INLINE void lrp(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const float amount)
{
	const __m128 amount4 = _mm_set1_ps(amount);
	const __m128 src0vec = *(const __m128* const)&src0;
	const __m128 src1vec = *(const __m128* const)&src1;
	__m128& outVec = *(__m128* const)&dst;

	if (writeMask == 0xF)
		outVec = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src1vec, src0vec), amount4), src0vec);
	else
	{
		const __m128 result = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src1vec, src0vec), amount4), src0vec);
		if (writeMask & 0x1)
			dst.x = result.m128_f32[0];
		if (writeMask & 0x2)
			dst.y = result.m128_f32[1];
		if (writeMask & 0x4)
			dst.z = result.m128_f32[2];
		if (writeMask & 0x8)
			dst.w = result.m128_f32[3];
	}
}

// lerp: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/lrp---vs
static INTRINSIC_INLINE void lrp(float& dst, const float src0, const float src1, const float src2)
{	
	dst = src0 * (src1 - src2) + src2;
}

// 3x2 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m3x2---vs
static INTRINSIC_INLINE void m3x2(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2)
{
	dst.x = (src0.x * src1.x) + (src0.x * src1.y) + (src0.x * src1.z);
	dst.y = (src0.x * src2.x) + (src0.y * src2.y) + (src0.z * src2.z);
}

// 3x3 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m3x3---vs
static INTRINSIC_INLINE void m3x3(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2, const D3DXVECTOR4& src3)
{
	dst.x = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z);
	dst.y = (src0.x * src2.x) + (src0.y * src2.y) + (src0.z * src2.z);
	dst.z = (src0.x * src3.x) + (src0.y * src3.y) + (src0.z * src3.z);
}

// 3x4 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m3x4---vs
static INTRINSIC_INLINE void m3x4(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2, const D3DXVECTOR4& src3, const D3DXVECTOR4& src4)
{
	dst.x = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z);
	dst.y = (src0.x * src2.x) + (src0.y * src2.y) + (src0.z * src2.z);
	dst.z = (src0.x * src3.x) + (src0.y * src3.y) + (src0.z * src3.z);
	dst.w = (src0.x * src4.x) + (src0.y * src4.y) + (src0.z * src4.z);
}

// 4x3 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m4x3---vs
static INTRINSIC_INLINE void m4x3(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2, const D3DXVECTOR4& src3)
{
	dst.x = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z) + (src0.w * src1.w);
	dst.y = (src0.x * src2.x) + (src0.y * src2.y) + (src0.z * src2.z) + (src0.w * src2.w);
	dst.z = (src0.x * src3.x) + (src0.y * src3.y) + (src0.z * src3.z) + (src0.w * src3.w);
}

// 4x4 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m4x4---vs
static INTRINSIC_INLINE void m4x4(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2, const D3DXVECTOR4& src3, const D3DXVECTOR4& src4)
{
	dst.x = (src0.x * src1.x) + (src0.y * src1.y) + (src0.z * src1.z) + (src0.w * src1.w);
	dst.y = (src0.x * src2.x) + (src0.y * src2.y) + (src0.z * src2.z) + (src0.w * src2.w);
	dst.z = (src0.x * src3.x) + (src0.y * src3.y) + (src0.z * src3.z) + (src0.w * src3.w);
	dst.w = (src0.x * src4.x) + (src0.y * src4.y) + (src0.z * src4.z) + (src0.w * src4.w);
}

// Multiply-add: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mad---ps
static INTRINSIC_INLINE void mad(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const D3DXVECTOR4& src2)
{
	dst.x = src0.x * src1.x + src2.x;
	dst.y = src0.y * src1.y + src2.y;
	dst.z = src0.z * src1.z + src2.z;
	dst.w = src0.w * src1.w + src2.w;
}

// Multiply-add: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mad---ps
static INTRINSIC_INLINE void mad(float& dst, const float src0, const float src1, const float src2)
{
	dst = src0 * src1 + src2;
}

// Maximum: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/max---vs
#undef max
static INTRINSIC_INLINE void max(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.x = (src0.x >= src1.x) ? src0.x : src1.x;
	dst.y = (src0.y >= src1.y) ? src0.y : src1.y;
	dst.z = (src0.z >= src1.z) ? src0.z : src1.z;
	dst.w = (src0.w >= src1.w) ? src0.w : src1.w;
}

// Maximum: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/max---vs
static INTRINSIC_INLINE void max(float& dst, const float src0, const float src1)
{
	dst = (src0 >= src1) ? src0 : src1;
}

// Minimum: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/min---vs
#undef min
static INTRINSIC_INLINE void min(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.x = (src0.x < src1.x) ? src0.x : src1.x;
	dst.y = (src0.y < src1.y) ? src0.y : src1.y;
	dst.z = (src0.z < src1.z) ? src0.z : src1.z;
	dst.w = (src0.w < src1.w) ? src0.w : src1.w;
}

// Minimum: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/min---vs
static INTRINSIC_INLINE void min(float& dst, const float src0, const float src1)
{
	dst = (src0 < src1) ? src0 : src1;
}

// Move (float to float): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov(D3DXVECTOR4& dst, const D3DXVECTOR4& src)
{
	dst = src;
}

// Move (float to float): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov(float& dst, const float src)
{
	dst = src;
}

// Move (float to int): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov(int& dst, const float src)
{
	dst = RoundToNearest(src);
}

// Move (float to int): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mova(int& dst, const float src)
{
	dst = RoundToNearest(src);
}

// Move (int to float): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov(float& dst, const int src)
{
	dst = (const float)src;
}

// Move (float to int): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov(int4& dst, const D3DXVECTOR4& src)
{
	dst.x = RoundToNearest(src.x);
	dst.y = RoundToNearest(src.y);
	dst.z = RoundToNearest(src.z);
	dst.w = RoundToNearest(src.w);
}

// Move (float to int): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mova(int4& dst, const D3DXVECTOR4& src)
{
	dst.x = RoundToNearest(src.x);
	dst.y = RoundToNearest(src.y);
	dst.z = RoundToNearest(src.z);
	dst.w = RoundToNearest(src.w);
}

// Move (int to float): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov(D3DXVECTOR4& dst, const int4& src)
{
	dst.x = (const float)src.x;
	dst.y = (const float)src.y;
	dst.z = (const float)src.z;
	dst.w = (const float)src.w;
}

// Multiply: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mul---vs
static INTRINSIC_INLINE void mul(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.x = src0.x * src1.x;
	dst.y = src0.y * src1.y;
	dst.z = src0.z * src1.z;
	dst.w = src0.w * src1.w;
}

// Multiply: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mul---vs
static INTRINSIC_INLINE void mul(float& dst, const float src0, const float src1)
{
	dst = src0 * src1;
}

// No-op: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nop---vs
static INTRINSIC_INLINE void nop()
{
	;
}

// No-op: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nop---vs
static INTRINSIC_INLINE void nop(class ShaderEngineBase&)
{
	;
}

// Normalize: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nrm---vs
static INTRINSIC_INLINE void nrm(D3DXVECTOR4& dst, const D3DXVECTOR4& src0)
{
	float f = src0.x * src0.x + src0.y * src0.y + src0.z * src0.z;
	if (f != 0.0f)
		f = (1.0f / sqrtf(f) );
	else
		f = FLT_MAX;

	dst.x = src0.x * f;
	dst.y = src0.y * f;
	dst.z = src0.z * f;
	dst.w = src0.w * f;
}

// Normalize: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nrm---vs
static INTRINSIC_INLINE void nrm(float4& dst, const float4& src0)
{
	float f = src0.x * src0.x + src0.y * src0.y + src0.z * src0.z;
	if (f != 0.0f)
		f = (const float)(1.0f / sqrtf(f) );
	else
		f = FLT_MAX;

	dst.x = src0.x * f;
	dst.y = src0.y * f;
	dst.z = src0.z * f;
	dst.w = src0.w * f;
}

// Normalize (xyz only): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nrm---vs
static INTRINSIC_INLINE void nrm_xyz(D3DXVECTOR4& dst, const D3DXVECTOR4& src0)
{
	float f = src0.x * src0.x + src0.y * src0.y + src0.z * src0.z;
	if (f != 0.0f)
		f = (const float)(1.0f / sqrtf(f) );
	else
		f = FLT_MAX;

	dst.x = src0.x * f;
	dst.y = src0.y * f;
	dst.z = src0.z * f;
}

// Power (15 minimum bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/pow---vs
static INTRINSIC_INLINE void pow(D3DXVECTOR4& dst, const float src0, const float src1)
{
	dst.x = dst.y = dst.z = dst.w = powf(fabsf(src0), src1);
}

// Power (15 minimum bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/pow---vs
static INTRINSIC_INLINE void pow(float& dst, const float src0, const float src1)
{
	dst = powf(fabsf(src0), src1);
}

// Return from function or return from shader program: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/ret---ps
static INTRINSIC_INLINE void ret(void)
{
	return;
}

// Reciprocal: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/rcp---vs
static INTRINSIC_INLINE void rcp(D3DXVECTOR4& dst, const float src)
{
	float f = src;
	if(f == 0.0f)
	{
		f = FLT_MAX;
	}
	else 
	{
		f = 1.0f / f;
	}

	dst.x = dst.y = dst.z = dst.w = f;
}

// Reciprocal: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/rcp---vs
static INTRINSIC_INLINE void rcp(float& dst, const float src)
{
	float f = src;
	if(f == 0.0f)
	{
		f = FLT_MAX;
	}
	else 
	{
		f = 1.0f / f;
	}

	dst = f;
}

// Reciprocal square-root: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/rsq---vs
static INTRINSIC_INLINE void rsq(D3DXVECTOR4& dst, const float src)
{
	float f = src;
	if (f == 0.0f)
		f = FLT_MAX;
	else
	{
		f = fabsf(f);
		if (f != 1.0f)
			f = 1.0f / (float)sqrtf(f);
	}

	dst.x = dst.y = dst.z = dst.w = f;
}

// Reciprocal square-root: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/rsq---vs
static INTRINSIC_INLINE void rsq(float& dst, const float src)
{
	float f = src;
	if (f == 0.0f)
		f = FLT_MAX;
	else
	{
		f = fabsf(f);
		if (f != 1.0f)
			f = 1.0f / (float)sqrtf(f);
	}

	dst = f;
}

// Sine/cosine: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sincos---vs
static INTRINSIC_INLINE void sincos_c(D3DXVECTOR4& dst, const float src)
{
	dst.x = cosf(src);
}

// Sine/cosine: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sincos---vs
static INTRINSIC_INLINE void sincos_s(D3DXVECTOR4& dst, const float src)
{
	dst.y = sinf(src);
}

// Sine/cosine: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sincos---vs
static INTRINSIC_INLINE void sincos_sc(D3DXVECTOR4& dst, const float src)
{
	dst.x = cosf(src);
	dst.y = sinf(src);
}

// Signed Greater-than or Equal to: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sge---vs
static INTRINSIC_INLINE void sge(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.x = (src0.x >= src1.x) ? 1.0f : 0.0f;
	dst.y = (src0.y >= src1.y) ? 1.0f : 0.0f;
	dst.z = (src0.z >= src1.z) ? 1.0f : 0.0f;
	dst.w = (src0.w >= src1.w) ? 1.0f : 0.0f;
}

// Signed Greater-than or Equal to: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sge---vs
static INTRINSIC_INLINE void sge(float& dst, const float src0, const float src1)
{
	dst = (src0 >= src1) ? 1.0f : 0.0f;
}

// Sign determination function: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sgn---vs
static INTRINSIC_INLINE void sgn(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1_unused, const D3DXVECTOR4& src2_unused)
{
	UNREFERENCED_PARAMETER(src1_unused);
	UNREFERENCED_PARAMETER(src2_unused);

	dst.x = src0.x < 0 ? -1.0f : src0.x == 0.0f ? 0.0f : 1.0f;
	dst.y = src0.y < 0 ? -1.0f : src0.y == 0.0f ? 0.0f : 1.0f;
	dst.z = src0.z < 0 ? -1.0f : src0.z == 0.0f ? 0.0f : 1.0f;
	dst.w = src0.w < 0 ? -1.0f : src0.w == 0.0f ? 0.0f : 1.0f;
}

// Signed Less-than: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/slt---vs
static INTRINSIC_INLINE void slt(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.x = (src0.x < src1.x) ? 1.0f : 0.0f;
	dst.y = (src0.y < src1.y) ? 1.0f : 0.0f;
	dst.z = (src0.z < src1.z) ? 1.0f : 0.0f;
	dst.w = (src0.w < src1.w) ? 1.0f : 0.0f;
}

// Signed Less-than: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/slt---vs
static INTRINSIC_INLINE void slt(float& dst, const float src0, const float src1)
{
	dst = (src0 < src1) ? 1.0f : 0.0f;
}

// Subtract: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sub---vs
static INTRINSIC_INLINE void sub(D3DXVECTOR4& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1)
{
	dst.x = src0.x - src1.x;
	dst.y = src0.y - src1.y;
	dst.z = src0.z - src1.z;
	dst.w = src0.w - src1.w;
}

static __forceinline const float saturate(const float inputFloat)
{
	if (inputFloat > 1.0f)
		return 1.0f;
	else if (inputFloat < 0.0f)
		return 0.0f;
	return inputFloat;
}

// Texture sample functions (tex2D, tex2Dgrad, tex2Dlod):
template <const unsigned char writeMask>
void __fastcall tex2Dmip0(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);

template <const unsigned char writeMask>
void __fastcall tex2Dlod(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);

template <const unsigned char writeMask, const bool useTexCoordMipBias>
void __fastcall tex2Dgrad(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
