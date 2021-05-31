#pragma once

// Fast log2 function that trades precision for speed
inline float log2_lowp(float val)
{
	int* const exp_ptr = reinterpret_cast<int* const>(&val);
	int x = *exp_ptr;
	const int log_2 = ((x >> 23) & 0xFF) - 128;
	x &= ~(255 << 23);
	x += 127 << 23;
	*exp_ptr = x;

	val = ( (-1.0f / 3.0f) * val + 2.0f) * val - 2.0f / 3.0f;

	return val + log_2;
}

// Absolute-value: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/abs---vs
static INTRINSIC_INLINE void abs4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src)[4])
{
	dst[0].x = fabsf(src[0].x);
	dst[1].x = fabsf(src[1].x);
	dst[2].x = fabsf(src[2].x);
	dst[3].x = fabsf(src[3].x);

	dst[0].y = fabsf(src[0].y);
	dst[1].y = fabsf(src[1].y);
	dst[2].y = fabsf(src[2].y);
	dst[3].y = fabsf(src[3].y);

	dst[0].z = fabsf(src[0].z);
	dst[1].z = fabsf(src[1].z);
	dst[2].z = fabsf(src[2].z);
	dst[3].z = fabsf(src[3].z);

	dst[0].w = fabsf(src[0].w);
	dst[1].w = fabsf(src[1].w);
	dst[2].w = fabsf(src[2].w);
	dst[3].w = fabsf(src[3].w);
}

// Addition: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/add---vs
static INTRINSIC_INLINE void add4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dst[0].x = src0[0].x + src1[0].x;
	dst[1].x = src0[1].x + src1[1].x;
	dst[2].x = src0[2].x + src1[2].x;
	dst[3].x = src0[3].x + src1[3].x;

	dst[0].y = src0[0].y + src1[0].y;
	dst[1].y = src0[1].y + src1[1].y;
	dst[2].y = src0[2].y + src1[2].y;
	dst[3].y = src0[3].y + src1[3].y;

	dst[0].z = src0[0].z + src1[0].z;
	dst[1].z = src0[1].z + src1[1].z;
	dst[2].z = src0[2].z + src1[2].z;
	dst[3].z = src0[3].z + src1[3].z;

	dst[0].w = src0[0].w + src1[0].w;
	dst[1].w = src0[1].w + src1[1].w;
	dst[2].w = src0[2].w + src1[2].w;
	dst[3].w = src0[3].w + src1[3].w;
}

// Compare: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/cmp---ps
static INTRINSIC_INLINE void cmp4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4])
{
	dst[0].x = src0[0].x >= 0.0f ? src1[0].x : src2[0].x;
	dst[1].x = src0[1].x >= 0.0f ? src1[1].x : src2[1].x;
	dst[2].x = src0[2].x >= 0.0f ? src1[2].x : src2[2].x;
	dst[3].x = src0[3].x >= 0.0f ? src1[3].x : src2[3].x;

	dst[0].y = src0[0].y >= 0.0f ? src1[0].y : src2[0].y;
	dst[1].y = src0[1].y >= 0.0f ? src1[1].y : src2[1].y;
	dst[2].y = src0[2].y >= 0.0f ? src1[2].y : src2[2].y;
	dst[3].y = src0[3].y >= 0.0f ? src1[3].y : src2[3].y;

	dst[0].z = src0[0].z >= 0.0f ? src1[0].z : src2[0].z;
	dst[1].z = src0[1].z >= 0.0f ? src1[1].z : src2[1].z;
	dst[2].z = src0[2].z >= 0.0f ? src1[2].z : src2[2].z;
	dst[3].z = src0[3].z >= 0.0f ? src1[3].z : src2[3].z;

	dst[0].w = src0[0].w >= 0.0f ? src1[0].w : src2[0].w;
	dst[1].w = src0[1].w >= 0.0f ? src1[1].w : src2[1].w;
	dst[2].w = src0[2].w >= 0.0f ? src1[2].w : src2[2].w;
	dst[3].w = src0[3].w >= 0.0f ? src1[3].w : src2[3].w;
}

// Conditional select: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/cnd---ps
static INTRINSIC_INLINE void cnd4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4])
{
	dst[0].x = src0[0].x > 0.5f ? src1[0].x : src2[0].x;
	dst[1].x = src0[1].x > 0.5f ? src1[1].x : src2[1].x;
	dst[2].x = src0[2].x > 0.5f ? src1[2].x : src2[2].x;
	dst[3].x = src0[3].x > 0.5f ? src1[3].x : src2[3].x;

	dst[0].y = src0[0].y > 0.5f ? src1[0].y : src2[0].y;
	dst[1].y = src0[1].y > 0.5f ? src1[1].y : src2[1].y;
	dst[2].y = src0[2].y > 0.5f ? src1[2].y : src2[2].y;
	dst[3].y = src0[3].y > 0.5f ? src1[3].y : src2[3].y;

	dst[0].z = src0[0].z > 0.5f ? src1[0].z : src2[0].z;
	dst[1].z = src0[1].z > 0.5f ? src1[1].z : src2[1].z;
	dst[2].z = src0[2].z > 0.5f ? src1[2].z : src2[2].z;
	dst[3].z = src0[3].z > 0.5f ? src1[3].z : src2[3].z;

	dst[0].w = src0[0].w > 0.5f ? src1[0].w : src2[0].w;
	dst[1].w = src0[1].w > 0.5f ? src1[1].w : src2[1].w;
	dst[2].w = src0[2].w > 0.5f ? src1[2].w : src2[2].w;
	dst[3].w = src0[3].w > 0.5f ? src1[3].w : src2[3].w;
}

// Cross-product: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/crs---vs
static INTRINSIC_INLINE void crs4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dst[0].x = src0[0].y * src1[0].z - src0[0].z * src1[0].y;
	dst[1].x = src0[1].y * src1[1].z - src0[1].z * src1[1].y;
	dst[2].x = src0[2].y * src1[2].z - src0[2].z * src1[2].y;
	dst[3].x = src0[3].y * src1[3].z - src0[3].z * src1[3].y;

	dst[0].y = src0[0].z * src1[0].x - src0[0].x * src1[0].z;
	dst[1].y = src0[1].z * src1[1].x - src0[1].x * src1[1].z;
	dst[2].y = src0[2].z * src1[2].x - src0[2].x * src1[2].z;
	dst[3].y = src0[3].z * src1[3].x - src0[3].x * src1[3].z;

	dst[0].z = src0[0].x * src1[0].y - src0[0].y * src1[0].x;
	dst[1].z = src0[1].x * src1[1].y - src0[1].y * src1[1].x;
	dst[2].z = src0[2].x * src1[2].y - src0[2].y * src1[2].x;
	dst[3].z = src0[3].x * src1[3].y - src0[3].y * src1[3].x;
}

// 2-D dot product and add scalar: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp2add---ps
static INTRINSIC_INLINE void dp2add4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4])
{
	const float dp2result[4] =
	{
		(src0[0].x * src1[0].x) + (src0[0].y * src1[0].y),
		(src0[1].x * src1[1].x) + (src0[1].y * src1[1].y),
		(src0[2].x * src1[2].x) + (src0[2].y * src1[2].y),
		(src0[3].x * src1[3].x) + (src0[3].y * src1[3].y)
	};

	dst[0].x = dp2result[0] + src2[0].x;
	dst[1].x = dp2result[1] + src2[1].x;
	dst[2].x = dp2result[2] + src2[2].x;
	dst[3].x = dp2result[3] + src2[3].x;

	dst[0].y = dp2result[0] + src2[0].y;
	dst[1].y = dp2result[1] + src2[1].y;
	dst[2].y = dp2result[2] + src2[2].y;
	dst[3].y = dp2result[3] + src2[3].y;

	dst[0].z = dp2result[0] + src2[0].z;
	dst[1].z = dp2result[1] + src2[1].z;
	dst[2].z = dp2result[2] + src2[2].z;
	dst[3].z = dp2result[3] + src2[3].z;

	dst[0].w = dp2result[0] + src2[0].w;
	dst[1].w = dp2result[1] + src2[1].w;
	dst[2].w = dp2result[2] + src2[2].w;
	dst[3].w = dp2result[3] + src2[3].w;
}

// 2-D dot product and add scalar: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp2add---ps
static INTRINSIC_INLINE void dp2add1_4(float& dst, const D3DXVECTOR4& src0, const D3DXVECTOR4& src1, const float src2)
{
	dst = (src0.x * src1.x) + (src0.y * src1.y);
	dst += src2;
}

// dot3: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp3---vs
static INTRINSIC_INLINE void dp3_4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	const float f4[4] =
	{
		(src0[0].x * src1[0].x) + (src0[0].y * src1[0].y) + (src0[0].z * src1[0].z),
		(src0[1].x * src1[1].x) + (src0[1].y * src1[1].y) + (src0[1].z * src1[1].z),
		(src0[2].x * src1[2].x) + (src0[2].y * src1[2].y) + (src0[2].z * src1[2].z),
		(src0[3].x * src1[3].x) + (src0[3].y * src1[3].y) + (src0[3].z * src1[3].z)
	};

	dst[0].x = dst[0].y = dst[0].z = dst[0].w = f4[0];
	dst[1].x = dst[1].y = dst[1].z = dst[1].w = f4[1];
	dst[2].x = dst[2].y = dst[2].z = dst[2].w = f4[2];
	dst[3].x = dst[3].y = dst[3].z = dst[3].w = f4[3];
}

// dot4: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp4---ps
static INTRINSIC_INLINE void dp4_4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	const float f4[4] =
	{
		(src0[0].x * src1[0].x) + (src0[0].y * src1[0].y) + (src0[0].z * src1[0].z) + (src0[0].w * src1[0].w),
		(src0[1].x * src1[1].x) + (src0[1].y * src1[1].y) + (src0[1].z * src1[1].z) + (src0[1].w * src1[1].w),
		(src0[2].x * src1[2].x) + (src0[2].y * src1[2].y) + (src0[2].z * src1[2].z) + (src0[2].w * src1[2].w),
		(src0[3].x * src1[3].x) + (src0[3].y * src1[3].y) + (src0[3].z * src1[3].z) + (src0[3].w * src1[3].w)
	};

	dst[0].x = dst[0].y = dst[0].z = dst[0].w = f4[0];
	dst[1].x = dst[1].y = dst[1].z = dst[1].w = f4[1];
	dst[2].x = dst[2].y = dst[2].z = dst[2].w = f4[2];
	dst[3].x = dst[3].y = dst[3].z = dst[3].w = f4[3];
}

// dot4 quad against constant: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dp4---ps
static INTRINSIC_INLINE void dp4_4(float (&dst)[4], const D3DXVECTOR4* (&src0)[4], const D3DXVECTOR4& src1)
{
	dst[0] = (src0[0]->x * src1.x);
	dst[1] = (src0[1]->x * src1.x);
	dst[2] = (src0[2]->x * src1.x);
	dst[3] = (src0[3]->x * src1.x);

	dst[0] += (src0[0]->y * src1.y);
	dst[1] += (src0[1]->y * src1.y);
	dst[2] += (src0[2]->y * src1.y);
	dst[3] += (src0[3]->y * src1.y);

	dst[0] += (src0[0]->z * src1.z);
	dst[1] += (src0[1]->z * src1.z);
	dst[2] += (src0[2]->z * src1.z);
	dst[3] += (src0[3]->z * src1.z);

	dst[0] += (src0[0]->w * src1.w);
	dst[1] += (src0[1]->w * src1.w);
	dst[2] += (src0[2]->w * src1.w);
	dst[3] += (src0[3]->w * src1.w);
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
static INTRINSIC_INLINE void dst4(D3DXVECTOR4 (&dest)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dest[0].x = 1.0f;
	dest[1].x = 1.0f;
	dest[2].x = 1.0f;
	dest[3].x = 1.0f;

	dest[0].y = sqrtf(src0[0].y);
	dest[1].y = sqrtf(src0[1].y);
	dest[2].y = sqrtf(src0[2].y);
	dest[3].y = sqrtf(src0[3].y);

	dest[0].z = src0[0].z;
	dest[1].z = src0[1].z;
	dest[2].z = src0[2].z;
	dest[3].z = src0[3].z;

	dest[0].w = src1[0].w;
	dest[1].w = src1[1].w;
	dest[2].w = src1[2].w;
	dest[3].w = src1[3].w;
}

// ddx/dsx: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dsx---ps
static INTRINSIC_INLINE void dsx(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4])
{
	const D3DXVECTOR4 ddxResultTop(src0[GRADIENT_QUAD_TOPRIGHT_INDEX].x - src0[GRADIENT_QUAD_TOPLEFT_INDEX].x,
		src0[GRADIENT_QUAD_TOPRIGHT_INDEX].y - src0[GRADIENT_QUAD_TOPLEFT_INDEX].y,
		src0[GRADIENT_QUAD_TOPRIGHT_INDEX].z - src0[GRADIENT_QUAD_TOPLEFT_INDEX].z,
		src0[GRADIENT_QUAD_TOPRIGHT_INDEX].z - src0[GRADIENT_QUAD_TOPLEFT_INDEX].w);

	dst[GRADIENT_QUAD_TOPLEFT_INDEX].x = dst[GRADIENT_QUAD_TOPRIGHT_INDEX].x = ddxResultTop.x;
	dst[GRADIENT_QUAD_TOPLEFT_INDEX].y = dst[GRADIENT_QUAD_TOPRIGHT_INDEX].y = ddxResultTop.y;
	dst[GRADIENT_QUAD_TOPLEFT_INDEX].z = dst[GRADIENT_QUAD_TOPRIGHT_INDEX].z = ddxResultTop.z;
	dst[GRADIENT_QUAD_TOPLEFT_INDEX].w = dst[GRADIENT_QUAD_TOPRIGHT_INDEX].w = ddxResultTop.w;

	const D3DXVECTOR4 ddxResultBot(src0[GRADIENT_QUAD_BOTRIGHT_INDEX].x - src0[GRADIENT_QUAD_BOTLEFT_INDEX].x,
		src0[GRADIENT_QUAD_BOTRIGHT_INDEX].y - src0[GRADIENT_QUAD_BOTLEFT_INDEX].y,
		src0[GRADIENT_QUAD_BOTRIGHT_INDEX].z - src0[GRADIENT_QUAD_BOTLEFT_INDEX].z,
		src0[GRADIENT_QUAD_BOTRIGHT_INDEX].z - src0[GRADIENT_QUAD_BOTLEFT_INDEX].w);

	dst[GRADIENT_QUAD_BOTLEFT_INDEX].x = dst[GRADIENT_QUAD_BOTRIGHT_INDEX].x = ddxResultBot.x;
	dst[GRADIENT_QUAD_BOTLEFT_INDEX].y = dst[GRADIENT_QUAD_BOTRIGHT_INDEX].y = ddxResultBot.y;
	dst[GRADIENT_QUAD_BOTLEFT_INDEX].z = dst[GRADIENT_QUAD_BOTRIGHT_INDEX].z = ddxResultBot.z;
	dst[GRADIENT_QUAD_BOTLEFT_INDEX].w = dst[GRADIENT_QUAD_BOTRIGHT_INDEX].w = ddxResultBot.w;
}

// ddy/dsy: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dsy---ps
static INTRINSIC_INLINE void dsy(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4])
{
	const D3DXVECTOR4 ddyResultLeft(src0[GRADIENT_QUAD_BOTLEFT_INDEX].x - src0[GRADIENT_QUAD_TOPLEFT_INDEX].x,
		src0[GRADIENT_QUAD_BOTLEFT_INDEX].y - src0[GRADIENT_QUAD_TOPLEFT_INDEX].y,
		src0[GRADIENT_QUAD_BOTLEFT_INDEX].z - src0[GRADIENT_QUAD_TOPLEFT_INDEX].z,
		src0[GRADIENT_QUAD_BOTLEFT_INDEX].z - src0[GRADIENT_QUAD_TOPLEFT_INDEX].w);

	dst[GRADIENT_QUAD_TOPLEFT_INDEX].x = dst[GRADIENT_QUAD_BOTLEFT_INDEX].x = ddyResultLeft.x;
	dst[GRADIENT_QUAD_TOPLEFT_INDEX].y = dst[GRADIENT_QUAD_BOTLEFT_INDEX].y = ddyResultLeft.y;
	dst[GRADIENT_QUAD_TOPLEFT_INDEX].z = dst[GRADIENT_QUAD_BOTLEFT_INDEX].z = ddyResultLeft.z;
	dst[GRADIENT_QUAD_TOPLEFT_INDEX].w = dst[GRADIENT_QUAD_BOTLEFT_INDEX].w = ddyResultLeft.w;

	const D3DXVECTOR4 ddyResultRight(src0[GRADIENT_QUAD_BOTRIGHT_INDEX].x - src0[GRADIENT_QUAD_TOPRIGHT_INDEX].x,
		src0[GRADIENT_QUAD_BOTRIGHT_INDEX].y - src0[GRADIENT_QUAD_TOPRIGHT_INDEX].y,
		src0[GRADIENT_QUAD_BOTRIGHT_INDEX].z - src0[GRADIENT_QUAD_TOPRIGHT_INDEX].z,
		src0[GRADIENT_QUAD_BOTRIGHT_INDEX].z - src0[GRADIENT_QUAD_TOPRIGHT_INDEX].w);

	dst[GRADIENT_QUAD_TOPRIGHT_INDEX].x = dst[GRADIENT_QUAD_BOTRIGHT_INDEX].x = ddyResultRight.x;
	dst[GRADIENT_QUAD_TOPRIGHT_INDEX].y = dst[GRADIENT_QUAD_BOTRIGHT_INDEX].y = ddyResultRight.y;
	dst[GRADIENT_QUAD_TOPRIGHT_INDEX].z = dst[GRADIENT_QUAD_BOTRIGHT_INDEX].z = ddyResultRight.z;
	dst[GRADIENT_QUAD_TOPRIGHT_INDEX].w = dst[GRADIENT_QUAD_BOTRIGHT_INDEX].w = ddyResultRight.w;
}

// exp (minimum 21 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/exp---vs
// expp (minimum 10 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/expp---vs
static INTRINSIC_INLINE void exp4(D3DXVECTOR4 (&dst)[4], const float (&src_replicateSwizzleComponent)[4])
{
	const float val4[4] =
	{
		powf(2.0f, src_replicateSwizzleComponent[0]),
		powf(2.0f, src_replicateSwizzleComponent[1]),
		powf(2.0f, src_replicateSwizzleComponent[2]),
		powf(2.0f, src_replicateSwizzleComponent[3])
	};
	dst[0].x = dst[0].y = dst[0].z = dst[0].w = val4[0];
	dst[1].x = dst[1].y = dst[1].z = dst[1].w = val4[1];
	dst[2].x = dst[2].y = dst[2].z = dst[2].w = val4[2];
	dst[3].x = dst[3].y = dst[3].z = dst[3].w = val4[3];
}

// Frac: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/frc---vs
static INTRINSIC_INLINE void frc4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src)[4])
{
	dst[0].x = src[0].x - floorf(src[0].x);
	dst[1].x = src[1].x - floorf(src[1].x);
	dst[2].x = src[2].x - floorf(src[2].x);
	dst[3].x = src[3].x - floorf(src[3].x);

	dst[0].y = src[0].y - floorf(src[0].y);
	dst[1].y = src[1].y - floorf(src[1].y);
	dst[2].y = src[2].y - floorf(src[2].y);
	dst[3].y = src[3].y - floorf(src[3].y);

	dst[0].z = src[0].z - floorf(src[0].z);
	dst[1].z = src[1].z - floorf(src[1].z);
	dst[2].z = src[2].z - floorf(src[2].z);
	dst[3].z = src[3].z - floorf(src[3].z);

	dst[0].w = src[0].w - floorf(src[0].w);
	dst[1].w = src[1].w - floorf(src[1].w);
	dst[2].w = src[2].w - floorf(src[2].w);
	dst[3].w = src[3].w - floorf(src[3].w);
}

// lighting: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/lit---vs
/*
// The source vector is assumed to contain the values shown in the following pseudocode.
src.x = N dot L    ; The dot product between normal and direction to light (for diffuse lighting)
src.y = N dot H    ; The dot product between normal and half vector (for specular lighting)
src.z = ignored    ; This value is ignored
src.w = exponent   ; The value must be between -128.0 and 128.0 (for specular lighting)
*/
static const D3DXVECTOR4 initLitVector4(1.0f, 0.0f, 0.0f, 1.0f);
static INTRINSIC_INLINE void lit4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4])
{
	dst[0] = dst[1] = dst[2] = dst[3] = initLitVector4;

	if (src0[0].x > 0.0f)
	{
		dst[0].y = src0[0].x; // This is N dot L (for diffuse lighting)

		// Do specular lighting:
		if (src0[0].y > 0.0f)
		{
			const float MAXPOWER = 127.9961f; // An implementation must support at least eight fraction bits in the power argument (8.8 fixed point)
			float power = src0[0].w;
			if (power < -MAXPOWER)
				power = -MAXPOWER;
			else if (power > MAXPOWER)
				power = MAXPOWER;
			dst[0].z = powf(src0[0].y, power);
		}
	}

	if (src0[1].x > 0.0f)
	{
		dst[1].y = src0[1].x; // This is N dot L (for diffuse lighting)

		// Do specular lighting:
		if (src0[1].y > 0.0f)
		{
			const float MAXPOWER = 127.9961f; // An implementation must support at least eight fraction bits in the power argument (8.8 fixed point)
			float power = src0[1].w;
			if (power < -MAXPOWER)
				power = -MAXPOWER;
			else if (power > MAXPOWER)
				power = MAXPOWER;
			dst[1].z = powf(src0[1].y, power);
		}
	}

	if (src0[2].x > 0.0f)
	{
		dst[2].y = src0[2].x; // This is N dot L (for diffuse lighting)

		// Do specular lighting:
		if (src0[2].y > 0.0f)
		{
			const float MAXPOWER = 127.9961f; // An implementation must support at least eight fraction bits in the power argument (8.8 fixed point)
			float power = src0[2].w;
			if (power < -MAXPOWER)
				power = -MAXPOWER;
			else if (power > MAXPOWER)
				power = MAXPOWER;
			dst[2].z = powf(src0[2].y, power);
		}
	}

	if (src0[3].x > 0.0f)
	{
		dst[3].y = src0[3].x; // This is N dot L (for diffuse lighting)

		// Do specular lighting:
		if (src0[3].y > 0.0f)
		{
			const float MAXPOWER = 127.9961f; // An implementation must support at least eight fraction bits in the power argument (8.8 fixed point)
			float power = src0[3].w;
			if (power < -MAXPOWER)
				power = -MAXPOWER;
			else if (power > MAXPOWER)
				power = MAXPOWER;
			dst[3].z = powf(src0[3].y, power);
		}
	}
}

// Log (minimum 21 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/log---vs
// Logp (minimum 10 bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/logp---vs
static INTRINSIC_INLINE void log4(D3DXVECTOR4 (&dst)[4], const float (&src_replicateSwizzleComponent)[4])
{
	const float v4[4] = 
	{
		fabsf(src_replicateSwizzleComponent[0]),
		fabsf(src_replicateSwizzleComponent[1]),
		fabsf(src_replicateSwizzleComponent[2]),
		fabsf(src_replicateSwizzleComponent[3])
	};

	if (v4[0] != 0.0f)
		dst[0].x = dst[0].y = dst[0].z = dst[0].w = log2_lowp(v4[0]);
	else
		dst[0].x = dst[0].y = dst[0].z = dst[0].w = -FLT_MAX;

	if (v4[1] != 0.0f)
		dst[1].x = dst[1].y = dst[1].z = dst[1].w = log2_lowp(v4[1]);
	else
		dst[1].x = dst[1].y = dst[1].z = dst[1].w = -FLT_MAX;

	if (v4[2] != 0.0f)
		dst[2].x = dst[2].y = dst[2].z = dst[2].w = log2_lowp(v4[2]);
	else
		dst[2].x = dst[2].y = dst[2].z = dst[2].w = -FLT_MAX;

	if (v4[3] != 0.0f)
		dst[3].x = dst[3].y = dst[3].z = dst[3].w = log2_lowp(v4[3]);
	else
		dst[3].x = dst[3].y = dst[3].z = dst[3].w = -FLT_MAX;
}

// lerp: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/lrp---vs
template <const unsigned char channelWriteMask = 0xF, const unsigned char pixelWriteMask = 0xF>
static INTRINSIC_INLINE void lrp4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4])
{
	__m128 amount4[4];
	if (pixelWriteMask & 0x1) amount4[0] = *(const __m128* const)&(src0[0]);
	if (pixelWriteMask & 0x2) amount4[1] = *(const __m128* const)&(src0[1]);
	if (pixelWriteMask & 0x4) amount4[2] = *(const __m128* const)&(src0[2]);
	if (pixelWriteMask & 0x8) amount4[3] = *(const __m128* const)&(src0[3]);

	__m128 src1vec[4];
	if (pixelWriteMask & 0x1) src1vec[0] = *(const __m128* const)&(src1[0]);
	if (pixelWriteMask & 0x2) src1vec[1] = *(const __m128* const)&(src1[1]);
	if (pixelWriteMask & 0x4) src1vec[2] = *(const __m128* const)&(src1[2]);
	if (pixelWriteMask & 0x8) src1vec[3] = *(const __m128* const)&(src1[3]);

	__m128 src2vec[4];
	if (pixelWriteMask & 0x1) src2vec[0] = *(const __m128* const)&(src2[0]);
	if (pixelWriteMask & 0x2) src2vec[1] = *(const __m128* const)&(src2[1]);
	if (pixelWriteMask & 0x4) src2vec[2] = *(const __m128* const)&(src2[2]);
	if (pixelWriteMask & 0x8) src2vec[3] = *(const __m128* const)&(src2[3]);

	if (channelWriteMask == 0xF)
	{
		if (pixelWriteMask & 0x1) *(__m128* const)&(dst[0]) = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[0], src1vec[0]), amount4[0]), src1vec[0]);
		if (pixelWriteMask & 0x2) *(__m128* const)&(dst[1]) = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[1], src1vec[1]), amount4[1]), src1vec[1]);
		if (pixelWriteMask & 0x4) *(__m128* const)&(dst[2]) = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[2], src1vec[2]), amount4[2]), src1vec[2]);
		if (pixelWriteMask & 0x8) *(__m128* const)&(dst[3]) = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[3], src1vec[3]), amount4[3]), src1vec[3]);
		return;
	}

	__m128 result[4];
	if (pixelWriteMask & 0x1) result[0] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[0], src1vec[0]), amount4[0]), src1vec[0]);
	if (pixelWriteMask & 0x2) result[1] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[1], src1vec[1]), amount4[1]), src1vec[1]);
	if (pixelWriteMask & 0x4) result[2] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[2], src1vec[2]), amount4[2]), src1vec[2]);
	if (pixelWriteMask & 0x8) result[3] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[3], src1vec[3]), amount4[3]), src1vec[3]);

	if (channelWriteMask & 0x1)
	{
		if (pixelWriteMask & 0x1) dst[0].x = result[0].m128_f32[0];
		if (pixelWriteMask & 0x2) dst[1].x = result[1].m128_f32[0];
		if (pixelWriteMask & 0x4) dst[2].x = result[2].m128_f32[0];
		if (pixelWriteMask & 0x8) dst[3].x = result[3].m128_f32[0];
	}
	if (channelWriteMask & 0x2)	
	{
		if (pixelWriteMask & 0x1) dst[0].y = result[0].m128_f32[1];
		if (pixelWriteMask & 0x2) dst[1].y = result[1].m128_f32[1];
		if (pixelWriteMask & 0x4) dst[2].y = result[2].m128_f32[1];
		if (pixelWriteMask & 0x8) dst[3].y = result[3].m128_f32[1];
	}
	if (channelWriteMask & 0x4)
	{
		if (pixelWriteMask & 0x1) dst[0].z = result[0].m128_f32[2];
		if (pixelWriteMask & 0x2) dst[1].z = result[1].m128_f32[2];
		if (pixelWriteMask & 0x4) dst[2].z = result[2].m128_f32[2];
		if (pixelWriteMask & 0x8) dst[3].z = result[3].m128_f32[2];
	}
	if (channelWriteMask & 0x8)
	{
		if (pixelWriteMask & 0x1) dst[0].w = result[0].m128_f32[3];
		if (pixelWriteMask & 0x2) dst[1].w = result[1].m128_f32[3];
		if (pixelWriteMask & 0x4) dst[2].w = result[2].m128_f32[3];
		if (pixelWriteMask & 0x8) dst[3].w = result[3].m128_f32[3];
	}
}

// lerp: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/lrp---vs
template <const unsigned char channelWriteMask = 0xF, const unsigned char pixelWriteMask = 0xF>
static inline void lrp4(D3DXVECTOR4 (&dst)[4], const __m128 src0, const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4])
{
	__m128 amount4[4];
	if (pixelWriteMask & 0x1) amount4[0] = _mm_shuffle_ps(src0, src0, _MM_SHUFFLE(0, 0, 0, 0) );
	if (pixelWriteMask & 0x2) amount4[1] = _mm_shuffle_ps(src0, src0, _MM_SHUFFLE(1, 1, 1, 1) );
	if (pixelWriteMask & 0x4) amount4[2] = _mm_shuffle_ps(src0, src0, _MM_SHUFFLE(2, 2, 2, 2) );
	if (pixelWriteMask & 0x8) amount4[3] = _mm_shuffle_ps(src0, src0, _MM_SHUFFLE(3, 3, 3, 3) );

	__m128 src1vec[4];
	if (pixelWriteMask & 0x1) src1vec[0] = *(const __m128* const)&(src1[0]);
	if (pixelWriteMask & 0x2) src1vec[1] = *(const __m128* const)&(src1[1]);
	if (pixelWriteMask & 0x4) src1vec[2] = *(const __m128* const)&(src1[2]);
	if (pixelWriteMask & 0x8) src1vec[3] = *(const __m128* const)&(src1[3]);

	__m128 src2vec[4];
	if (pixelWriteMask & 0x1) src2vec[0] = *(const __m128* const)&(src2[0]);
	if (pixelWriteMask & 0x2) src2vec[1] = *(const __m128* const)&(src2[1]);
	if (pixelWriteMask & 0x4) src2vec[2] = *(const __m128* const)&(src2[2]);
	if (pixelWriteMask & 0x8) src2vec[3] = *(const __m128* const)&(src2[3]);

	if (channelWriteMask == 0xF)
	{
		if (pixelWriteMask & 0x1) *(__m128* const)&(dst[0]) = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[0], src1vec[0]), amount4[0]), src1vec[0]);
		if (pixelWriteMask & 0x2) *(__m128* const)&(dst[1]) = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[1], src1vec[1]), amount4[1]), src1vec[1]);
		if (pixelWriteMask & 0x4) *(__m128* const)&(dst[2]) = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[2], src1vec[2]), amount4[2]), src1vec[2]);
		if (pixelWriteMask & 0x8) *(__m128* const)&(dst[3]) = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[3], src1vec[3]), amount4[3]), src1vec[3]);
		return;
	}

	__m128 result[4];
	if (pixelWriteMask & 0x1) result[0] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[0], src1vec[0]), amount4[0]), src1vec[0]);
	if (pixelWriteMask & 0x2) result[1] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[1], src1vec[1]), amount4[1]), src1vec[1]);
	if (pixelWriteMask & 0x4) result[2] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[2], src1vec[2]), amount4[2]), src1vec[2]);
	if (pixelWriteMask & 0x8) result[3] = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(src2vec[3], src1vec[3]), amount4[3]), src1vec[3]);

	if (channelWriteMask & 0x1)
	{
		if (pixelWriteMask & 0x1) dst[0].x = result[0].m128_f32[0];
		if (pixelWriteMask & 0x2) dst[1].x = result[1].m128_f32[0];
		if (pixelWriteMask & 0x4) dst[2].x = result[2].m128_f32[0];
		if (pixelWriteMask & 0x8) dst[3].x = result[3].m128_f32[0];
	}
	if (channelWriteMask & 0x2)	
	{
		if (pixelWriteMask & 0x1) dst[0].y = result[0].m128_f32[1];
		if (pixelWriteMask & 0x2) dst[1].y = result[1].m128_f32[1];
		if (pixelWriteMask & 0x4) dst[2].y = result[2].m128_f32[1];
		if (pixelWriteMask & 0x8) dst[3].y = result[3].m128_f32[1];
	}
	if (channelWriteMask & 0x4)
	{
		if (pixelWriteMask & 0x1) dst[0].z = result[0].m128_f32[2];
		if (pixelWriteMask & 0x2) dst[1].z = result[1].m128_f32[2];
		if (pixelWriteMask & 0x4) dst[2].z = result[2].m128_f32[2];
		if (pixelWriteMask & 0x8) dst[3].z = result[3].m128_f32[2];
	}
	if (channelWriteMask & 0x8)
	{
		if (pixelWriteMask & 0x1) dst[0].w = result[0].m128_f32[3];
		if (pixelWriteMask & 0x2) dst[1].w = result[1].m128_f32[3];
		if (pixelWriteMask & 0x4) dst[2].w = result[2].m128_f32[3];
		if (pixelWriteMask & 0x8) dst[3].w = result[3].m128_f32[3];
	}
}

// 3x2 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m3x2---vs
static INTRINSIC_INLINE void m3x2_4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4])
{
	dst[0].x = (src0[0].x * src1[0].x) + (src0[0].x * src1[0].y) + (src0[0].x * src1[0].z);
	dst[1].x = (src0[1].x * src1[1].x) + (src0[1].x * src1[1].y) + (src0[1].x * src1[1].z);
	dst[2].x = (src0[2].x * src1[2].x) + (src0[2].x * src1[2].y) + (src0[2].x * src1[2].z);
	dst[3].x = (src0[3].x * src1[3].x) + (src0[3].x * src1[3].y) + (src0[3].x * src1[3].z);

	dst[0].y = (src0[0].x * src2[0].x) + (src0[0].y * src2[0].y) + (src0[0].z * src2[0].z);
	dst[1].y = (src0[1].x * src2[1].x) + (src0[1].y * src2[1].y) + (src0[1].z * src2[1].z);
	dst[2].y = (src0[2].x * src2[2].x) + (src0[2].y * src2[2].y) + (src0[2].z * src2[2].z);
	dst[3].y = (src0[3].x * src2[3].x) + (src0[3].y * src2[3].y) + (src0[3].z * src2[3].z);
}

// 3x3 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m3x3---vs
static INTRINSIC_INLINE void m3x3_4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4], const D3DXVECTOR4 (&src3)[4])
{
	dst[0].x = (src0[0].x * src1[0].x) + (src0[0].y * src1[0].y) + (src0[0].z * src1[0].z);
	dst[1].x = (src0[1].x * src1[1].x) + (src0[1].y * src1[1].y) + (src0[1].z * src1[1].z);
	dst[2].x = (src0[2].x * src1[2].x) + (src0[2].y * src1[2].y) + (src0[2].z * src1[2].z);
	dst[3].x = (src0[3].x * src1[3].x) + (src0[3].y * src1[3].y) + (src0[3].z * src1[3].z);

	dst[0].y = (src0[0].x * src2[0].x) + (src0[0].y * src2[0].y) + (src0[0].z * src2[0].z);
	dst[1].y = (src0[1].x * src2[1].x) + (src0[1].y * src2[1].y) + (src0[1].z * src2[1].z);
	dst[2].y = (src0[2].x * src2[2].x) + (src0[2].y * src2[2].y) + (src0[2].z * src2[2].z);
	dst[3].y = (src0[3].x * src2[3].x) + (src0[3].y * src2[3].y) + (src0[3].z * src2[3].z);

	dst[0].z = (src0[0].x * src3[0].x) + (src0[0].y * src3[0].y) + (src0[0].z * src3[0].z);
	dst[1].z = (src0[1].x * src3[1].x) + (src0[1].y * src3[1].y) + (src0[1].z * src3[1].z);
	dst[2].z = (src0[2].x * src3[2].x) + (src0[2].y * src3[2].y) + (src0[2].z * src3[2].z);
	dst[3].z = (src0[3].x * src3[3].x) + (src0[3].y * src3[3].y) + (src0[3].z * src3[3].z);
}

// 3x4 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m3x4---vs
static INTRINSIC_INLINE void m3x4_4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4], const D3DXVECTOR4 (&src3)[4], const D3DXVECTOR4 (&src4)[4])
{
	dst[0].x = (src0[0].x * src1[0].x) + (src0[0].y * src1[0].y) + (src0[0].z * src1[0].z);
	dst[1].x = (src0[1].x * src1[1].x) + (src0[1].y * src1[1].y) + (src0[1].z * src1[1].z);
	dst[2].x = (src0[2].x * src1[2].x) + (src0[2].y * src1[2].y) + (src0[2].z * src1[2].z);
	dst[3].x = (src0[3].x * src1[3].x) + (src0[3].y * src1[3].y) + (src0[3].z * src1[3].z);

	dst[0].y = (src0[0].x * src2[0].x) + (src0[0].y * src2[0].y) + (src0[0].z * src2[0].z);
	dst[0].y = (src0[1].x * src2[1].x) + (src0[1].y * src2[1].y) + (src0[1].z * src2[1].z);
	dst[0].y = (src0[2].x * src2[2].x) + (src0[2].y * src2[2].y) + (src0[2].z * src2[2].z);
	dst[0].y = (src0[3].x * src2[3].x) + (src0[3].y * src2[3].y) + (src0[3].z * src2[3].z);

	dst[0].z = (src0[0].x * src3[0].x) + (src0[0].y * src3[0].y) + (src0[0].z * src3[0].z);
	dst[1].z = (src0[1].x * src3[1].x) + (src0[1].y * src3[1].y) + (src0[1].z * src3[1].z);
	dst[2].z = (src0[2].x * src3[2].x) + (src0[2].y * src3[2].y) + (src0[2].z * src3[2].z);
	dst[3].z = (src0[3].x * src3[3].x) + (src0[3].y * src3[3].y) + (src0[3].z * src3[3].z);

	dst[0].w = (src0[0].x * src4[0].x) + (src0[0].y * src4[0].y) + (src0[0].z * src4[0].z);
	dst[1].w = (src0[1].x * src4[1].x) + (src0[1].y * src4[1].y) + (src0[1].z * src4[1].z);
	dst[2].w = (src0[2].x * src4[2].x) + (src0[2].y * src4[2].y) + (src0[2].z * src4[2].z);
	dst[3].w = (src0[3].x * src4[3].x) + (src0[3].y * src4[3].y) + (src0[3].z * src4[3].z);
}

// 4x3 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m4x3---vs
static INTRINSIC_INLINE void m4x3_4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4], const D3DXVECTOR4 (&src3)[4])
{
	dst[0].x = (src0[0].x * src1[0].x) + (src0[0].y * src1[0].y) + (src0[0].z * src1[0].z) + (src0[0].w * src1[0].w);
	dst[1].x = (src0[1].x * src1[1].x) + (src0[1].y * src1[1].y) + (src0[1].z * src1[1].z) + (src0[1].w * src1[1].w);
	dst[2].x = (src0[2].x * src1[2].x) + (src0[2].y * src1[2].y) + (src0[2].z * src1[2].z) + (src0[2].w * src1[2].w);
	dst[3].x = (src0[3].x * src1[3].x) + (src0[3].y * src1[3].y) + (src0[3].z * src1[3].z) + (src0[3].w * src1[3].w);

	dst[0].y = (src0[0].x * src2[0].x) + (src0[0].y * src2[0].y) + (src0[0].z * src2[0].z) + (src0[0].w * src2[0].w);
	dst[1].y = (src0[1].x * src2[1].x) + (src0[1].y * src2[1].y) + (src0[1].z * src2[1].z) + (src0[1].w * src2[1].w);
	dst[2].y = (src0[2].x * src2[2].x) + (src0[2].y * src2[2].y) + (src0[2].z * src2[2].z) + (src0[2].w * src2[2].w);
	dst[3].y = (src0[3].x * src2[3].x) + (src0[3].y * src2[3].y) + (src0[3].z * src2[3].z) + (src0[3].w * src2[3].w);

	dst[0].z = (src0[0].x * src3[0].x) + (src0[0].y * src3[0].y) + (src0[0].z * src3[0].z) + (src0[0].w * src3[0].w);
	dst[1].z = (src0[1].x * src3[1].x) + (src0[1].y * src3[1].y) + (src0[1].z * src3[1].z) + (src0[1].w * src3[1].w);
	dst[2].z = (src0[2].x * src3[2].x) + (src0[2].y * src3[2].y) + (src0[2].z * src3[2].z) + (src0[2].w * src3[2].w);
	dst[3].z = (src0[3].x * src3[3].x) + (src0[3].y * src3[3].y) + (src0[3].z * src3[3].z) + (src0[3].w * src3[3].w);
}

// 4x4 matrix mult: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/m4x4---vs
static INTRINSIC_INLINE void m4x4_4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4], const D3DXVECTOR4 (&src3)[4], const D3DXVECTOR4 (&src4)[4])
{
	dst[0].x = (src0[0].x * src1[0].x) + (src0[0].y * src1[0].y) + (src0[0].z * src1[0].z) + (src0[0].w * src1[0].w);
	dst[1].x = (src0[1].x * src1[1].x) + (src0[1].y * src1[1].y) + (src0[1].z * src1[1].z) + (src0[1].w * src1[1].w);
	dst[2].x = (src0[2].x * src1[2].x) + (src0[2].y * src1[2].y) + (src0[2].z * src1[2].z) + (src0[2].w * src1[2].w);
	dst[3].x = (src0[3].x * src1[3].x) + (src0[3].y * src1[3].y) + (src0[3].z * src1[3].z) + (src0[3].w * src1[3].w);

	dst[0].y = (src0[0].x * src2[0].x) + (src0[0].y * src2[0].y) + (src0[0].z * src2[0].z) + (src0[0].w * src2[0].w);
	dst[1].y = (src0[1].x * src2[1].x) + (src0[1].y * src2[1].y) + (src0[1].z * src2[1].z) + (src0[1].w * src2[1].w);
	dst[2].y = (src0[2].x * src2[2].x) + (src0[2].y * src2[2].y) + (src0[2].z * src2[2].z) + (src0[2].w * src2[2].w);
	dst[3].y = (src0[3].x * src2[3].x) + (src0[3].y * src2[3].y) + (src0[3].z * src2[3].z) + (src0[3].w * src2[3].w);

	dst[0].z = (src0[0].x * src3[0].x) + (src0[0].y * src3[0].y) + (src0[0].z * src3[0].z) + (src0[0].w * src3[0].w);
	dst[1].z = (src0[1].x * src3[1].x) + (src0[1].y * src3[1].y) + (src0[1].z * src3[1].z) + (src0[1].w * src3[1].w);
	dst[2].z = (src0[2].x * src3[2].x) + (src0[2].y * src3[2].y) + (src0[2].z * src3[2].z) + (src0[2].w * src3[2].w);
	dst[3].z = (src0[3].x * src3[3].x) + (src0[3].y * src3[3].y) + (src0[3].z * src3[3].z) + (src0[3].w * src3[3].w);

	dst[0].w = (src0[0].x * src4[0].x) + (src0[0].y * src4[0].y) + (src0[0].z * src4[0].z) + (src0[0].w * src4[0].w);
	dst[1].w = (src0[1].x * src4[1].x) + (src0[1].y * src4[1].y) + (src0[1].z * src4[1].z) + (src0[1].w * src4[1].w);
	dst[2].w = (src0[2].x * src4[2].x) + (src0[2].y * src4[2].y) + (src0[2].z * src4[2].z) + (src0[2].w * src4[2].w);
	dst[3].w = (src0[3].x * src4[3].x) + (src0[3].y * src4[3].y) + (src0[3].z * src4[3].z) + (src0[3].w * src4[3].w);
}

// Multiply-add: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mad---ps
static INTRINSIC_INLINE void mad4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4], const D3DXVECTOR4 (&src2)[4])
{
	dst[0].x = src0[0].x * src1[0].x + src2[0].x;
	dst[1].x = src0[1].x * src1[1].x + src2[1].x;
	dst[2].x = src0[2].x * src1[2].x + src2[2].x;
	dst[3].x = src0[3].x * src1[3].x + src2[3].x;

	dst[0].y = src0[0].y * src1[0].y + src2[0].y;
	dst[1].y = src0[1].y * src1[1].y + src2[1].y;
	dst[2].y = src0[2].y * src1[2].y + src2[2].y;
	dst[3].y = src0[3].y * src1[3].y + src2[3].y;

	dst[0].z = src0[0].z * src1[0].z + src2[0].z;
	dst[1].z = src0[1].z * src1[1].z + src2[1].z;
	dst[2].z = src0[2].z * src1[2].z + src2[2].z;
	dst[3].z = src0[3].z * src1[3].z + src2[3].z;

	dst[0].w = src0[0].w * src1[0].w + src2[0].w;
	dst[1].w = src0[1].w * src1[1].w + src2[1].w;
	dst[2].w = src0[2].w * src1[2].w + src2[2].w;
	dst[3].w = src0[3].w * src1[3].w + src2[3].w;
}

// Maximum: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/max---vs
#undef max
static INTRINSIC_INLINE void max4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dst[0].x = (src0[0].x >= src1[0].x) ? src0[0].x : src1[0].x;
	dst[1].x = (src0[1].x >= src1[1].x) ? src0[1].x : src1[1].x;
	dst[2].x = (src0[2].x >= src1[2].x) ? src0[2].x : src1[2].x;
	dst[3].x = (src0[3].x >= src1[3].x) ? src0[3].x : src1[3].x;

	dst[0].y = (src0[0].y >= src1[0].y) ? src0[0].y : src1[0].y;
	dst[1].y = (src0[1].y >= src1[1].y) ? src0[1].y : src1[1].y;
	dst[2].y = (src0[2].y >= src1[2].y) ? src0[2].y : src1[2].y;
	dst[3].y = (src0[3].y >= src1[3].y) ? src0[3].y : src1[3].y;

	dst[0].z = (src0[0].z >= src1[0].z) ? src0[0].z : src1[0].z;
	dst[1].z = (src0[1].z >= src1[1].z) ? src0[1].z : src1[1].z;
	dst[2].z = (src0[2].z >= src1[2].z) ? src0[2].z : src1[2].z;
	dst[3].z = (src0[3].z >= src1[3].z) ? src0[3].z : src1[3].z;

	dst[0].w = (src0[0].w >= src1[0].w) ? src0[0].w : src1[0].w;
	dst[1].w = (src0[1].w >= src1[1].w) ? src0[1].w : src1[1].w;
	dst[2].w = (src0[2].w >= src1[2].w) ? src0[2].w : src1[2].w;
	dst[3].w = (src0[3].w >= src1[3].w) ? src0[3].w : src1[3].w;
}

// Minimum: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/min---vs
#undef min
static INTRINSIC_INLINE void min4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dst[0].x = (src0[0].x < src1[0].x) ? src0[0].x : src1[0].x;
	dst[1].x = (src0[1].x < src1[1].x) ? src0[1].x : src1[1].x;
	dst[2].x = (src0[2].x < src1[2].x) ? src0[2].x : src1[2].x;
	dst[3].x = (src0[3].x < src1[3].x) ? src0[3].x : src1[3].x;

	dst[0].y = (src0[0].y < src1[0].y) ? src0[0].y : src1[0].y;
	dst[1].y = (src0[1].y < src1[1].y) ? src0[1].y : src1[1].y;
	dst[2].y = (src0[2].y < src1[2].y) ? src0[2].y : src1[2].y;
	dst[3].y = (src0[3].y < src1[3].y) ? src0[3].y : src1[3].y;

	dst[0].z = (src0[0].z < src1[0].z) ? src0[0].z : src1[0].z;
	dst[1].z = (src0[1].z < src1[1].z) ? src0[1].z : src1[1].z;
	dst[2].z = (src0[2].z < src1[2].z) ? src0[2].z : src1[2].z;
	dst[3].z = (src0[3].z < src1[3].z) ? src0[3].z : src1[3].z;

	dst[0].w = (src0[0].w < src1[0].w) ? src0[0].w : src1[0].w;
	dst[1].w = (src0[1].w < src1[1].w) ? src0[1].w : src1[1].w;
	dst[2].w = (src0[2].w < src1[2].w) ? src0[2].w : src1[2].w;
	dst[3].w = (src0[3].w < src1[3].w) ? src0[3].w : src1[3].w;
}

// Move (float to float): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov4(D3DXVECTOR4& dst, const D3DXVECTOR4& src)
{
	dst = src;
}

// Move (float to float): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov4(int4& dst, const D3DXVECTOR4& src)
{
	dst.x = RoundToNearest(src.x);
	dst.y = RoundToNearest(src.y);
	dst.z = RoundToNearest(src.z);
	dst.w = RoundToNearest(src.w);
}

// Move (float to float): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mova4(int4& dst, const D3DXVECTOR4& src)
{
	dst.x = RoundToNearest(src.x);
	dst.y = RoundToNearest(src.y);
	dst.z = RoundToNearest(src.z);
	dst.w = RoundToNearest(src.w);
}

// Move (float to float): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mov---vs
static INTRINSIC_INLINE void mov4(D3DXVECTOR4& dst, const int4& src)
{
	dst.x = (const float)src.x;
	dst.y = (const float)src.y;
	dst.z = (const float)src.z;
	dst.w = (const float)src.w;
}

// Multiply: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/mul---vs
static INTRINSIC_INLINE void mul4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dst[0].x = src0[0].x * src1[0].x;
	dst[1].x = src0[1].x * src1[1].x;
	dst[2].x = src0[2].x * src1[2].x;
	dst[3].x = src0[3].x * src1[3].x;

	dst[0].y = src0[0].y * src1[0].y;
	dst[1].y = src0[1].y * src1[1].y;
	dst[2].y = src0[2].y * src1[2].y;
	dst[3].y = src0[3].y * src1[3].y;

	dst[0].z = src0[0].z * src1[0].z;
	dst[1].z = src0[1].z * src1[1].z;
	dst[2].z = src0[2].z * src1[2].z;
	dst[3].z = src0[3].z * src1[3].z;

	dst[0].w = src0[0].w * src1[0].w;
	dst[1].w = src0[1].w * src1[1].w;
	dst[2].w = src0[2].w * src1[2].w;
	dst[3].w = src0[3].w * src1[3].w;
}

// No-op: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nop---vs
static INTRINSIC_INLINE void nop4()
{
	;
}

// No-op: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nop---vs
static INTRINSIC_INLINE void nop4(class ShaderEngineBase&)
{
	;
}

// Normalize: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nrm---vs
static INTRINSIC_INLINE void nrm4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4])
{
	float f4[4] =
	{
		src0[0].x * src0[0].x + src0[0].y * src0[0].y + src0[0].z * src0[0].z,
		src0[1].x * src0[1].x + src0[1].y * src0[1].y + src0[1].z * src0[1].z,
		src0[2].x * src0[2].x + src0[2].y * src0[2].y + src0[2].z * src0[2].z,
		src0[3].x * src0[3].x + src0[3].y * src0[3].y + src0[3].z * src0[3].z
	};

	if (f4[0] != 0.0f)
		f4[0] = 1.0f / sqrtf(f4[0]);
	else
		f4[0] = FLT_MAX;

	if (f4[1] != 0.0f)
		f4[1] = 1.0f / sqrtf(f4[1]);
	else
		f4[1] = FLT_MAX;

	if (f4[2] != 0.0f)
		f4[2] = 1.0f / sqrtf(f4[2]);
	else
		f4[2] = FLT_MAX;

	if (f4[3] != 0.0f)
		f4[3] = 1.0f / sqrtf(f4[3]);
	else
		f4[3] = FLT_MAX;

	dst[0].x = src0[0].x * f4[0];
	dst[1].x = src0[1].x * f4[1];
	dst[2].x = src0[2].x * f4[2];
	dst[3].x = src0[3].x * f4[3];

	dst[0].y = src0[0].y * f4[0];
	dst[1].y = src0[1].y * f4[1];
	dst[2].y = src0[2].y * f4[2];
	dst[3].y = src0[3].y * f4[3];

	dst[0].z = src0[0].z * f4[0];
	dst[1].z = src0[1].z * f4[1];
	dst[2].z = src0[2].z * f4[2];
	dst[3].z = src0[3].z * f4[3];

	dst[0].w = src0[0].w * f4[0];
	dst[1].w = src0[1].w * f4[1];
	dst[2].w = src0[2].w * f4[2];
	dst[3].w = src0[3].w * f4[3];
}

// Normalize (xyz only): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/nrm---vs
static INTRINSIC_INLINE void nrm_xyz4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4])
{
	float f4[4] =
	{
		src0[0].x * src0[0].x + src0[0].y * src0[0].y + src0[0].z * src0[0].z,
		src0[1].x * src0[1].x + src0[1].y * src0[1].y + src0[1].z * src0[1].z,
		src0[2].x * src0[2].x + src0[2].y * src0[2].y + src0[2].z * src0[2].z,
		src0[3].x * src0[3].x + src0[3].y * src0[3].y + src0[3].z * src0[3].z
	};

	if (f4[0] != 0.0f)
		f4[0] = 1.0f / sqrtf(f4[0]);
	else
		f4[0] = FLT_MAX;

	if (f4[1] != 0.0f)
		f4[1] = 1.0f / sqrtf(f4[1]);
	else
		f4[1] = FLT_MAX;

	if (f4[2] != 0.0f)
		f4[2] = 1.0f / sqrtf(f4[2]);
	else
		f4[2] = FLT_MAX;

	if (f4[3] != 0.0f)
		f4[3] = 1.0f / sqrtf(f4[3]);
	else
		f4[3] = FLT_MAX;

	dst[0].x = src0[0].x * f4[0];
	dst[1].x = src0[1].x * f4[1];
	dst[2].x = src0[2].x * f4[2];
	dst[3].x = src0[3].x * f4[3];

	dst[0].y = src0[0].y * f4[0];
	dst[1].y = src0[1].y * f4[1];
	dst[2].y = src0[2].y * f4[2];
	dst[3].y = src0[3].y * f4[3];

	dst[0].z = src0[0].z * f4[0];
	dst[1].z = src0[1].z * f4[1];
	dst[2].z = src0[2].z * f4[2];
	dst[3].z = src0[3].z * f4[3];

	// Don't modify the W component in this version of the function
}

// Power (15 minimum bits of precision): https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/pow---vs
static INTRINSIC_INLINE void pow4(D3DXVECTOR4 (&dst)[4], const float (&src0)[4], const float (&src1)[4])
{
	const float absVec[4] = 
	{
		fabsf(src0[0]),
		fabsf(src0[1]),
		fabsf(src0[2]),
		fabsf(src0[3])
	};

	dst[0].x = dst[0].y = dst[0].z = dst[0].w = powf(absVec[0], src1[0]);
	dst[1].x = dst[1].y = dst[1].z = dst[1].w = powf(absVec[1], src1[1]);
	dst[2].x = dst[2].y = dst[2].z = dst[2].w = powf(absVec[2], src1[2]);
	dst[3].x = dst[3].y = dst[3].z = dst[3].w = powf(absVec[3], src1[3]);
}

// Return from function or return from shader program: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/ret---ps
static INTRINSIC_INLINE void ret4(void)
{
	return;
}

// Reciprocal: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/rcp---vs
static INTRINSIC_INLINE void rcp4(D3DXVECTOR4 (&dst)[4], const float (&src)[4])
{
	float f4[4] = 
	{
		src[0],
		src[1],
		src[2],
		src[3]
	};

	f4[0] = f4[0] == 0.0f ? FLT_MAX : 1.0f / f4[0];
	f4[1] = f4[1] == 0.0f ? FLT_MAX : 1.0f / f4[1];
	f4[2] = f4[2] == 0.0f ? FLT_MAX : 1.0f / f4[2];
	f4[3] = f4[3] == 0.0f ? FLT_MAX : 1.0f / f4[3];

	dst[0].x = dst[0].y = dst[0].z = dst[0].w = f4[0];
	dst[1].x = dst[1].y = dst[1].z = dst[1].w = f4[1];
	dst[2].x = dst[2].y = dst[2].z = dst[2].w = f4[2];
	dst[3].x = dst[3].y = dst[3].z = dst[3].w = f4[3];
}

// Reciprocal square-root: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/rsq---vs
static INTRINSIC_INLINE void rsq4(D3DXVECTOR4 (&dst)[4], const float (&src)[4])
{
	float f4[4] = 
	{
		src[0],
		src[1],
		src[2],
		src[3]
	};

	if (f4[0] == 0.0f)
		f4[0] = FLT_MAX;
	else
	{
		f4[0] = fabsf(f4[0]);
		if (f4[0] != 1.0f)
			f4[0] = 1.0f / (float)sqrtf(f4[0]);
	}

	if (f4[1] == 0.0f)
		f4[1] = FLT_MAX;
	else
	{
		f4[1] = fabsf(f4[1]);
		if (f4[1] != 1.0f)
			f4[1] = 1.0f / (float)sqrtf(f4[1]);
	}

	if (f4[2] == 0.0f)
		f4[2] = FLT_MAX;
	else
	{
		f4[2] = fabsf(f4[2]);
		if (f4[2] != 1.0f)
			f4[2] = 1.0f / (float)sqrtf(f4[2]);
	}

	if (f4[3] == 0.0f)
		f4[3] = FLT_MAX;
	else
	{
		f4[3] = fabsf(f4[3]);
		if (f4[3] != 1.0f)
			f4[3] = 1.0f / (float)sqrtf(f4[3]);
	}

	dst[0].x = dst[0].y = dst[0].z = dst[0].w = f4[0];
	dst[1].x = dst[1].y = dst[1].z = dst[1].w = f4[1];
	dst[2].x = dst[2].y = dst[2].z = dst[2].w = f4[2];
	dst[3].x = dst[3].y = dst[3].z = dst[3].w = f4[3];
}

// Sine/cosine: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sincos---vs
static INTRINSIC_INLINE void sincos_c4(D3DXVECTOR4 (&dst)[4], const float (&src)[4])
{
	dst[0].x = cosf(src[0]);
	dst[1].x = cosf(src[1]);
	dst[2].x = cosf(src[2]);
	dst[3].x = cosf(src[3]);
}

// Sine/cosine: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sincos---vs
static INTRINSIC_INLINE void sincos_s4(D3DXVECTOR4 (&dst)[4], const float (&src)[4])
{
	dst[0].y = sinf(src[0]);
	dst[1].y = sinf(src[1]);
	dst[2].y = sinf(src[2]);
	dst[3].y = sinf(src[3]);
}

// Sine/cosine: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sincos---vs
static INTRINSIC_INLINE void sincos_sc4(D3DXVECTOR4 (&dst)[4], const float (&src)[4])
{
	dst[0].x = cosf(src[0]);
	dst[1].x = cosf(src[1]);
	dst[2].x = cosf(src[2]);
	dst[3].x = cosf(src[3]);

	dst[0].y = sinf(src[0]);
	dst[1].y = sinf(src[1]);
	dst[2].y = sinf(src[2]);
	dst[3].y = sinf(src[3]);
}

// Signed Greater-than or Equal to: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sge---vs
static INTRINSIC_INLINE void sge4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dst[0].x = (src0[0].x >= src1[0].x) ? 1.0f : 0.0f;
	dst[1].x = (src0[1].x >= src1[1].x) ? 1.0f : 0.0f;
	dst[2].x = (src0[2].x >= src1[2].x) ? 1.0f : 0.0f;
	dst[3].x = (src0[3].x >= src1[3].x) ? 1.0f : 0.0f;

	dst[0].y = (src0[0].y >= src1[0].y) ? 1.0f : 0.0f;
	dst[1].y = (src0[1].y >= src1[1].y) ? 1.0f : 0.0f;
	dst[2].y = (src0[2].y >= src1[2].y) ? 1.0f : 0.0f;
	dst[3].y = (src0[3].y >= src1[3].y) ? 1.0f : 0.0f;

	dst[0].z = (src0[0].z >= src1[0].z) ? 1.0f : 0.0f;
	dst[1].z = (src0[1].z >= src1[1].z) ? 1.0f : 0.0f;
	dst[2].z = (src0[2].z >= src1[2].z) ? 1.0f : 0.0f;
	dst[3].z = (src0[3].z >= src1[3].z) ? 1.0f : 0.0f;

	dst[0].w = (src0[0].w >= src1[0].w) ? 1.0f : 0.0f;
	dst[1].w = (src0[1].w >= src1[1].w) ? 1.0f : 0.0f;
	dst[2].w = (src0[2].w >= src1[2].w) ? 1.0f : 0.0f;
	dst[3].w = (src0[3].w >= src1[3].w) ? 1.0f : 0.0f;
}

// Sign determination function: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sgn---vs
static INTRINSIC_INLINE void sgn4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1_unused)[4], const D3DXVECTOR4 (&src2_unused)[4])
{
	UNREFERENCED_PARAMETER(src1_unused);
	UNREFERENCED_PARAMETER(src2_unused);

	dst[0].x = src0[0].x < 0 ? -1.0f : src0[0].x == 0.0f ? 0.0f : 1.0f;
	dst[1].x = src0[1].x < 0 ? -1.0f : src0[1].x == 0.0f ? 0.0f : 1.0f;
	dst[2].x = src0[2].x < 0 ? -1.0f : src0[2].x == 0.0f ? 0.0f : 1.0f;
	dst[3].x = src0[3].x < 0 ? -1.0f : src0[3].x == 0.0f ? 0.0f : 1.0f;

	dst[0].y = src0[0].y < 0 ? -1.0f : src0[0].y == 0.0f ? 0.0f : 1.0f;
	dst[1].y = src0[1].y < 0 ? -1.0f : src0[1].y == 0.0f ? 0.0f : 1.0f;
	dst[2].y = src0[2].y < 0 ? -1.0f : src0[2].y == 0.0f ? 0.0f : 1.0f;
	dst[3].y = src0[3].y < 0 ? -1.0f : src0[3].y == 0.0f ? 0.0f : 1.0f;

	dst[0].z = src0[0].z < 0 ? -1.0f : src0[0].z == 0.0f ? 0.0f : 1.0f;
	dst[1].z = src0[1].z < 0 ? -1.0f : src0[1].z == 0.0f ? 0.0f : 1.0f;
	dst[2].z = src0[2].z < 0 ? -1.0f : src0[2].z == 0.0f ? 0.0f : 1.0f;
	dst[3].z = src0[3].z < 0 ? -1.0f : src0[3].z == 0.0f ? 0.0f : 1.0f;

	dst[0].w = src0[0].w < 0 ? -1.0f : src0[0].w == 0.0f ? 0.0f : 1.0f;
	dst[1].w = src0[1].w < 0 ? -1.0f : src0[1].w == 0.0f ? 0.0f : 1.0f;
	dst[2].w = src0[2].w < 0 ? -1.0f : src0[2].w == 0.0f ? 0.0f : 1.0f;
	dst[3].w = src0[3].w < 0 ? -1.0f : src0[3].w == 0.0f ? 0.0f : 1.0f;
}

// Signed Less-than: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/slt---vs
static INTRINSIC_INLINE void slt4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dst[0].x = (src0[0].x < src1[0].x) ? 1.0f : 0.0f;
	dst[1].x = (src0[1].x < src1[1].x) ? 1.0f : 0.0f;
	dst[2].x = (src0[2].x < src1[2].x) ? 1.0f : 0.0f;
	dst[3].x = (src0[3].x < src1[3].x) ? 1.0f : 0.0f;

	dst[0].y = (src0[0].y < src1[0].y) ? 1.0f : 0.0f;
	dst[1].y = (src0[1].y < src1[1].y) ? 1.0f : 0.0f;
	dst[2].y = (src0[2].y < src1[2].y) ? 1.0f : 0.0f;
	dst[3].y = (src0[3].y < src1[3].y) ? 1.0f : 0.0f;

	dst[0].z = (src0[0].z < src1[0].z) ? 1.0f : 0.0f;
	dst[1].z = (src0[1].z < src1[1].z) ? 1.0f : 0.0f;
	dst[2].z = (src0[2].z < src1[2].z) ? 1.0f : 0.0f;
	dst[3].z = (src0[3].z < src1[3].z) ? 1.0f : 0.0f;

	dst[0].w = (src0[0].w < src1[0].w) ? 1.0f : 0.0f;
	dst[1].w = (src0[1].w < src1[1].w) ? 1.0f : 0.0f;
	dst[2].w = (src0[2].w < src1[2].w) ? 1.0f : 0.0f;
	dst[3].w = (src0[3].w < src1[3].w) ? 1.0f : 0.0f;
}

// Subtract: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/sub---vs
static INTRINSIC_INLINE void sub4(D3DXVECTOR4 (&dst)[4], const D3DXVECTOR4 (&src0)[4], const D3DXVECTOR4 (&src1)[4])
{
	dst[0].x = src0[0].x - src1[0].x;
	dst[1].x = src0[1].x - src1[1].x;
	dst[2].x = src0[2].x - src1[2].x;
	dst[3].x = src0[3].x - src1[3].x;

	dst[0].y = src0[0].y - src1[0].y;
	dst[1].y = src0[1].y - src1[1].y;
	dst[2].y = src0[2].y - src1[2].y;
	dst[3].y = src0[3].y - src1[3].y;

	dst[0].z = src0[0].z - src1[0].z;
	dst[1].z = src0[1].z - src1[1].z;
	dst[2].z = src0[2].z - src1[2].z;
	dst[3].z = src0[3].z - src1[3].z;

	dst[0].w = src0[0].w - src1[0].w;
	dst[1].w = src0[1].w - src1[1].w;
	dst[2].w = src0[2].w - src1[2].w;
	dst[3].w = src0[3].w - src1[3].w;
}

// Texture sample functions (tex2D, tex2Dgrad, tex2Dlod):
template <const unsigned char writeMask>
void __cdecl tex2Dmip0_4(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);

template <const unsigned char writeMask>
void __cdecl tex2Dlod4(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);

template <const unsigned char writeMask, const bool useTexCoordMipBias>
void __cdecl tex2Dgrad4(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
