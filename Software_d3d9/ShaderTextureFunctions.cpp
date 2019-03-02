#pragma once

#include "d3d9include.h"
#include "ShaderEngineBase.h"
#include "IDirect3DTexture9Hook.h"

template void __fastcall tex2Dmip0<0>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<1>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<2>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<3>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<4>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<5>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<6>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<7>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<8>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<9>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<10>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<11>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<12>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<13>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<14>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);
template void __fastcall tex2Dmip0<15>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr);

template <const unsigned char writeMask>
void __fastcall tex2Dmip0(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const sampler* const samplerPtr)
{
	if (samplerPtr->texture)
		samplerPtr->texture->SampleTextureLoD<writeMask>(texCoord.x, texCoord.y, 0.0f, samplerPtr->samplerState, outVal);
	else
	{
		// D3D9 seems to treat sampling from a NULL sampler as opaque white (this is the case in the OpenGL standard), even though
		// in D3D10 they changed this to instead return transparent black!
		outVal = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

template void __fastcall tex2Dlod<0>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<1>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<2>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<3>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<4>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<5>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<6>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<7>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<8>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<9>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<10>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<11>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<12>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<13>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<14>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);
template void __fastcall tex2Dlod<15>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr);

template <const unsigned char writeMask>
void __fastcall tex2Dlod(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoordAndLoD, const sampler* const samplerPtr)
{
	if (samplerPtr->texture)
		samplerPtr->texture->SampleTextureLoD<writeMask>(texCoordAndLoD.x, texCoordAndLoD.y, texCoordAndLoD.w, samplerPtr->samplerState, outVal);
	else
	{
		// D3D9 seems to treat sampling from a NULL sampler as opaque white (this is the case in the OpenGL standard), even though
		// in D3D10 they changed this to instead return transparent black!
		outVal = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

template void __fastcall tex2Dgrad<0, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<1, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<2, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<3, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<4, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<5, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<6, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<7, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<8, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<9, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<10, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<11, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<12, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<13, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<14, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<15, false>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<0, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<1, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<2, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<3, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<4, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<5, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<6, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<7, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<8, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<9, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<10, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<11, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<12, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<13, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<14, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);
template void __fastcall tex2Dgrad<15, true>(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr);

template <const unsigned char writeMask, const bool useTexCoordMipBias>
void __fastcall tex2Dgrad(D3DXVECTOR4& outVal, const D3DXVECTOR4& texCoord, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const sampler* const samplerPtr)
{
	if (samplerPtr->texture)
	{
		if (useTexCoordMipBias)
			samplerPtr->texture->SampleTextureGradBias<writeMask>(texCoord.x, texCoord.y, texCoord.w, texDdx, texDdy, samplerPtr->samplerState, outVal);
		else
			samplerPtr->texture->SampleTextureGrad<writeMask>(texCoord.x, texCoord.y, texDdx, texDdy, samplerPtr->samplerState, outVal);
	}
	else
	{
		// D3D9 seems to treat sampling from a NULL sampler as opaque white (this is the case in the OpenGL standard), even though
		// in D3D10 they changed this to instead return transparent black!
		outVal = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}
