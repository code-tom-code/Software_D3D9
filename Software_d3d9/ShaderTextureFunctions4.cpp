#pragma once

#include "d3d9include.h"
#include "ShaderEngineBase.h"
#include "IDirect3DTexture9Hook.h"

template void __cdecl tex2Dmip0_4<0>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<1>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<2>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<3>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<4>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<5>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<6>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<7>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<8>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<9>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<10>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<11>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<12>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<13>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<14>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dmip0_4<15>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr);

template <const unsigned char writeMask>
void __cdecl tex2Dmip0_4(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const sampler* const samplerPtr)
{
	if (samplerPtr->texture)
	{
		float x4[4] =
		{
			texCoord[0].x,
			texCoord[1].x,
			texCoord[2].x,
			texCoord[3].x
		};
		float y4[4] =
		{
			texCoord[0].y,
			texCoord[1].y,
			texCoord[2].y,
			texCoord[3].y
		};
		float zeroFloat4[4] = 
		{
			0.0f,
			0.0f,
			0.0f,
			0.0f
		};
		samplerPtr->texture->SampleTextureLoD4<writeMask>(x4, y4, zeroFloat4, samplerPtr->samplerState, outVal4);
	}
	else
	{
		// D3D9 seems to treat sampling from a NULL sampler as opaque white (this is the case in the OpenGL standard), even though
		// in D3D10 they changed this to instead return transparent black!
		outVal4[0] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[1] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[2] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[3] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

template void __cdecl tex2Dlod4<0>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<1>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<2>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<3>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<4>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<5>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<6>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<7>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<8>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<9>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<10>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<11>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<12>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<13>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<14>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dlod4<15>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr);

template <const unsigned char writeMask>
void __cdecl tex2Dlod4(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoordAndLoD)[4], const sampler* const samplerPtr)
{
	if (samplerPtr->texture)
	{
		float x4[4] =
		{
			texCoordAndLoD[0].x,
			texCoordAndLoD[1].x,
			texCoordAndLoD[2].x,
			texCoordAndLoD[3].x
		};
		float y4[4] =
		{
			texCoordAndLoD[0].y,
			texCoordAndLoD[1].y,
			texCoordAndLoD[2].y,
			texCoordAndLoD[3].y
		};
		float mipFloat4[4] =
		{
			texCoordAndLoD[0].w,
			texCoordAndLoD[1].w,
			texCoordAndLoD[2].w,
			texCoordAndLoD[3].w
		};
		samplerPtr->texture->SampleTextureLoD4<writeMask>(x4, y4, mipFloat4, samplerPtr->samplerState, outVal4);
	}
	else
	{
		// D3D9 seems to treat sampling from a NULL sampler as opaque white (this is the case in the OpenGL standard), even though
		// in D3D10 they changed this to instead return transparent black!
		outVal4[0] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[1] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[2] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[3] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

template void __cdecl tex2Dgrad4<0, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<1, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<2, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<3, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<4, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<5, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<6, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<7, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<8, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<9, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<10, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<11, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<12, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<13, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<14, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<15, false>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<0, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<1, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<2, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<3, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<4, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<5, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<6, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<7, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<8, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<9, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<10, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<11, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<12, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<13, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<14, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);
template void __cdecl tex2Dgrad4<15, true>(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr);

template <const unsigned char writeMask, const bool useTexCoordMipBias>
void __cdecl tex2Dgrad4(D3DXVECTOR4 (&outVal4)[4], const D3DXVECTOR4 (&texCoord)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const sampler* const samplerPtr)
{
	if (samplerPtr->texture)
	{
		float x4[4] =
		{
			texCoord[0].x,
			texCoord[1].x,
			texCoord[2].x,
			texCoord[3].x
		};
		float y4[4] =
		{
			texCoord[0].y,
			texCoord[1].y,
			texCoord[2].y,
			texCoord[3].y
		};

		if (useTexCoordMipBias)
		{
			const float mipBias4[4] =
			{
				texCoord[0].w,
				texCoord[1].w,
				texCoord[2].w,
				texCoord[3].w
			};
			samplerPtr->texture->SampleTextureGradBias4<writeMask>(x4, y4, mipBias4, texDdx4, texDdy4, samplerPtr->samplerState, outVal4);
		}
		else
			samplerPtr->texture->SampleTextureGrad4<writeMask>(x4, y4, texDdx4, texDdy4, samplerPtr->samplerState, outVal4);
	}
	else
	{
		// D3D9 seems to treat sampling from a NULL sampler as opaque white (this is the case in the OpenGL standard), even though
		// in D3D10 they changed this to instead return transparent black!
		outVal4[0] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[1] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[2] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
		outVal4[3] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}
