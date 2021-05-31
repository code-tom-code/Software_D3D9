#pragma once

#include "IDirect3DTexture9Hook.h"
#include "IDirect3DSurface9Hook.h"
#include "IDirect3DDevice9Hook.h"

/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	HRESULT ret = realObject->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG IDirect3DTexture9Hook::AddRef(THIS)
{
	ULONG ret = realObject->AddRef();
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG IDirect3DTexture9Hook::Release(THIS)
{
	ULONG ret = realObject->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked Texture %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

/*** IDirect3DBaseTexture9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
{
	LPDIRECT3DDEVICE9 realD3D9dev = NULL;
	HRESULT ret = realObject->GetDevice(&realD3D9dev);
	if (FAILED(ret) )
	{
		*ppDevice = NULL;
		return ret;
	}

#ifdef _DEBUG
	// Check that the parentHook's underlying IDirect3DDevice9* matches the realD3D9dev pointer
	if (parentDevice->GetUnderlyingDevice() != realD3D9dev)
	{
		DbgBreakPrint("Error: Unknown d3d9 device hook detected!");
	}
#endif
	parentDevice->AddRef(); // Super important to increment the ref-count here, otherwise our parent object will get destroyed when Release() is called on it!

	*ppDevice = parentDevice;
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::SetPrivateData(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
{
	HRESULT ret = realObject->SetPrivateData(refguid, pData, SizeOfData, Flags);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::GetPrivateData(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)
{
	HRESULT ret = realObject->GetPrivateData(refguid, pData, pSizeOfData);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::FreePrivateData(THIS_ REFGUID refguid)
{
	HRESULT ret = realObject->FreePrivateData(refguid);
	return ret;
}

COM_DECLSPEC_NOTHROW DWORD IDirect3DTexture9Hook::SetPriority(THIS_ DWORD PriorityNew)
{
	DWORD ret = realObject->SetPriority(PriorityNew);
	return ret;
}

COM_DECLSPEC_NOTHROW DWORD IDirect3DTexture9Hook::GetPriority(THIS)
{
	DWORD ret = realObject->GetPriority();
	return ret;
}

COM_DECLSPEC_NOTHROW void IDirect3DTexture9Hook::PreLoad(THIS)
{
	realObject->PreLoad();
}

COM_DECLSPEC_NOTHROW D3DRESOURCETYPE IDirect3DTexture9Hook::GetType(THIS)
{
#ifdef _DEBUG
	D3DRESOURCETYPE realRet = realObject->GetType();
	if (realRet != D3DRTYPE_TEXTURE)
	{
		__debugbreak();
	}
#endif
	return D3DRTYPE_TEXTURE;
}

COM_DECLSPEC_NOTHROW DWORD IDirect3DTexture9Hook::SetLOD(THIS_ DWORD LODNew)
{
	DWORD ret = realObject->SetLOD(LODNew);
	return ret;
}

COM_DECLSPEC_NOTHROW DWORD IDirect3DTexture9Hook::GetLOD(THIS)
{
	DWORD ret = realObject->GetLOD();
	return ret;
}

COM_DECLSPEC_NOTHROW DWORD IDirect3DTexture9Hook::GetLevelCount(THIS)
{
	DWORD ret = realObject->GetLevelCount();
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::SetAutoGenFilterType(THIS_ D3DTEXTUREFILTERTYPE FilterType)
{
	HRESULT ret = realObject->SetAutoGenFilterType(FilterType);
	if (FAILED(ret) )
		return ret;

	AutoGenFilter = FilterType;

	return ret;
}

COM_DECLSPEC_NOTHROW D3DTEXTUREFILTERTYPE IDirect3DTexture9Hook::GetAutoGenFilterType(THIS)
{
#ifdef _DEBUG
	D3DTEXTUREFILTERTYPE realRet = realObject->GetAutoGenFilterType();
	if (realRet != AutoGenFilter)
	{
		__debugbreak();
	}
#endif
	return AutoGenFilter;
}

COM_DECLSPEC_NOTHROW void IDirect3DTexture9Hook::GenerateMipSubLevels(THIS)
{
	realObject->GenerateMipSubLevels();

	if (!(InternalUsage & D3DUSAGE_AUTOGENMIPMAP) )
		return;

	// TODO: Implement this
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::GetLevelDesc(THIS_ UINT Level,D3DSURFACE_DESC *pDesc)
{
	//HRESULT ret = realObject->GetLevelDesc(Level, pDesc);

	if (Level >= surfaces.size() )
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return D3DERR_INVALIDCALL;
	}

	HRESULT ret = surfaces[Level]->GetDesc(pDesc);
	if (FAILED(ret) )
		return ret;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::GetSurfaceLevel(THIS_ UINT Level,IDirect3DSurface9** ppSurfaceLevel)
{
	LPDIRECT3DSURFACE9 realSurface = NULL;
	HRESULT ret = realObject->GetSurfaceLevel(Level, &realSurface);
	if (FAILED(ret) )
		return ret;

	if (InternalUsage & D3DUSAGE_AUTOGENMIPMAP)
	{
		if (Level > 0)
		{
#ifdef _DEBUG
			__debugbreak();
#endif
			return D3DERR_INVALIDCALL;
		}
	}

	if (ppSurfaceLevel)
	{
		IDirect3DSurface9Hook* const retSurface = surfaces[Level];

#ifdef _DEBUG
		if (retSurface->GetUnderlyingSurface() != realSurface)
		{
			__debugbreak();
		}
#endif

		*ppSurfaceLevel = retSurface;
		retSurface->AddRef(); // Super important to increment the ref-count here, otherwise our surfaces will get deleted out from under us!
		return ret;
	}
	else
	{
		return D3DERR_INVALIDCALL;
	}
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::LockRect(THIS_ UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags)
{
	/*HRESULT ret = realObject->LockRect(Level, pLockedRect, pRect, Flags);
	if (FAILED(ret) )
		return ret;*/

	if (Level >= surfaces.size() )
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return D3DERR_INVALIDCALL;
	}

	HRESULT ret = surfaces[Level]->LockRect(pLockedRect, pRect, Flags);
	if (FAILED(ret) )
		return ret;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::UnlockRect(THIS_ UINT Level)
{
	//HRESULT ret = realObject->UnlockRect(Level);

	if (Level >= surfaces.size() )
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return D3DERR_INVALIDCALL;
	}

	HRESULT ret = surfaces[Level]->UnlockRect();
	if (FAILED(ret) )
		return ret;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT IDirect3DTexture9Hook::AddDirtyRect(THIS_ CONST RECT* pDirtyRect)
{
	HRESULT ret = realObject->AddDirtyRect(pDirtyRect);
	return ret;
}

static inline const unsigned GetMipLevels(unsigned Dimension)
{
	unsigned ret = 1;

	while (Dimension > 1)
	{
		Dimension >>= 1;
		++ret;
	}

	return ret;
}

static inline const unsigned GetMipLevels(const unsigned Width, const unsigned Height)
{
	if (Width > Height)
		return GetMipLevels(Width);
	else
		return GetMipLevels(Height);
}

static inline const unsigned GetMipLevels(const unsigned Width, const unsigned Height, const unsigned Depth)
{
	if (Width > Height && Width > Depth)
		return GetMipLevels(Width);
	else if (Height > Width && Height > Depth)
		return GetMipLevels(Height);
	else
		return GetMipLevels(Depth);
}

void IDirect3DTexture9Hook::CreateTexture(const UINT _Width, const UINT _Height, const UINT _Levels, const DebuggableUsage _Usage, const D3DFORMAT _Format, const D3DPOOL _Pool)
{
	InternalWidth = _Width;
	InternalHeight = _Height;
	InternalLevels = _Levels;
	InternalUsage = _Usage;
	InternalFormat = _Format;
	InternalPool = _Pool;

	const unsigned numMips = InternalLevels ? InternalLevels : GetMipLevels(InternalWidth, InternalHeight);
	UINT surfaceWidth = InternalWidth;
	UINT surfaceHeight = InternalHeight;
	for (unsigned x = 0; x < numMips; ++x)
	{
		LPDIRECT3DSURFACE9 realSurface = NULL;
		if (!(InternalUsage & D3DUSAGE_AUTOGENMIPMAP) || x == 0)
		{
			if (FAILED(realObject->GetSurfaceLevel(x, &realSurface) ) || !realSurface)
			{
				__debugbreak();
			}
		}

		IDirect3DSurface9Hook* newSurface = new IDirect3DSurface9Hook(realSurface, parentDevice);
		newSurface->CreateTextureImplicitSurface(surfaceWidth, surfaceHeight, InternalFormat, InternalPool, InternalUsage, x, this);

		surfaceWidth >>= 1;
		if (surfaceWidth == 0)
			surfaceWidth = 1;
		surfaceHeight >>= 1;
		if (surfaceHeight == 0)
			surfaceHeight = 1;

		surfaces.push_back(newSurface);
	}

	surfaceCountMinusOne = surfaces.size() - 1;
	surfaceCountMinusTwo = ( (const int)surfaceCountMinusOne) - 1; // This can be negative
	surfaceCountMinusOneF = (const float)surfaceCountMinusOne;

	surfaceLevel0 = surfaces[0];
}

const bool IDirect3DTexture9Hook::UpdateTextureInternal(const IDirect3DTexture9Hook* const sourceTexture)
{
	// In addition, this method will fail if the textures are of different formats. Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205858(v=vs.85).aspx
	if (sourceTexture->InternalFormat != InternalFormat)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return false;
	}

	// If the source texture has fewer levels than the destination, the method will fail. Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205858(v=vs.85).aspx
	if (sourceTexture->surfaces.size() < surfaces.size() )
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return false;
	}

	if (InternalUsage & D3DUSAGE_AUTOGENMIPMAP)
	{
		surfaceLevel0->UpdateSurfaceInternal(sourceTexture->surfaceLevel0, NULL, NULL);
		return true;
	}
	// If pSourceTexture is an autogenerated mipmap and pDestinationTexture a non-autogenerated mipmap, UpdateTexture will fail. Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205858(v=vs.85).aspx
	else if (sourceTexture->InternalUsage & D3DUSAGE_AUTOGENMIPMAP)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return false;
	}

	// For now, only support copying between equal textures:
	if (sourceTexture->surfaces.size() != surfaces.size() )
	{
		__debugbreak();
	}
	if (sourceTexture->InternalWidth != InternalWidth)
	{
		__debugbreak();
	}
	if (sourceTexture->InternalHeight != InternalHeight)
	{
		__debugbreak();
	}

	const unsigned numSurfacesToCopy = surfaces.size();
	for (unsigned x = 0; x < numSurfacesToCopy; ++x)
	{
		surfaces[x]->UpdateSurfaceInternal(sourceTexture->surfaces[x], NULL, NULL);
	}

	return true;
}

template void IDirect3DTexture9Hook::SampleTextureLoD<0>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<1>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<2>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<3>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<4>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<5>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<6>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<7>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<8>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<9>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<10>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<11>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<12>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<13>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<14>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureLoD<15>(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;

template <const unsigned char writeMask>
void IDirect3DTexture9Hook::SampleTextureLoD(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const
{
	if (writeMask == 0x0)
	{
		outColor.x = outColor.y = outColor.z = outColor.w = 0.0f;
		return;
	}

	switch (samplerState.stateUnion.namedStates.addressU)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#endif
	case D3DTADDRESS_WRAP      :
		if (x > 1.0f)
			x = fmodf(x, 1.0f);
		else if (x < 0.0f)
			x = 1.0f - fmodf(fabsf(x), 1.0f);
		break;
	case D3DTADDRESS_MIRROR    :
	{
		const int ix = (const int)x;
		x = fmodf(x, 1.0f);
		if (ix & 0x1)
			x = 1.0f - x;
	}
		break;
	case D3DTADDRESS_CLAMP     :
		if (x > 1.0f)
			x = 1.0f;
		else if (x < 0.0f)
			x = 0.0f;
		break;
	case D3DTADDRESS_BORDER    :
		if (x < 0.0f || x > 1.0f)
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor);
			return;
		}
		break;
	case D3DTADDRESS_MIRRORONCE:
	{
		const float f = fabsf(x);
		x = f > 1.0f ? 1.0f : f;
	}
		break;
	}

	switch (samplerState.stateUnion.namedStates.addressV)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#endif
	case D3DTADDRESS_WRAP      :
		if (y > 1.0f)
			y = fmodf(y, 1.0f);
		else if (y < 0.0f)
			y = 1.0f - fmodf(fabsf(y), 1.0f);
		break;
	case D3DTADDRESS_MIRROR    :
	{
		const int iy = (const int)y;
		y = fmodf(y, 1.0f);
		if (iy & 0x1)
			y = 1.0f - y;
	}
		break;
	case D3DTADDRESS_CLAMP     :
		if (y > 1.0f)
			y = 1.0f;
		else if (y < 0.0f)
			y = 0.0f;
		break;
	case D3DTADDRESS_BORDER    :
		if (y < 0.0f || y > 1.0f)
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor);
			return;
		}
		break;
	case D3DTADDRESS_MIRRORONCE:
	{
		const float f = fabsf(y);
		y = f > 1.0f ? 1.0f : f;
	}
		break;
	}

	mip += samplerState.stateUnion.namedStates.mipMapLoDBias;

	const float maxMipLevel = samplerState.cachedFloatMaxMipLevel;

	if (mip < maxMipLevel)
		mip = maxMipLevel;

	const float minMipLevel = surfaceCountMinusOneF;

	if (mip > minMipLevel)
		mip = minMipLevel;

	if (mip < 0.0f)
		mip = 0.0f;

	if (mip == 0.0f)
	{
		surfaceLevel0->SampleSurface<writeMask>(x, y, samplerState, outColor);
	}
	else
	{
		switch (samplerState.stateUnion.namedStates.mipFilter)
		{
		default:
#ifdef _DEBUG
			__debugbreak(); // Invalid mip filter!
#endif
		case D3DTEXF_NONE           : // Mip-mapping is disabled, always read level 0 of the surface
			surfaceLevel0->SampleSurface<writeMask>(x, y, samplerState, outColor);
			break;
		case D3DTEXF_POINT          :
		{
			int mipLevel = (const int)(mip + 0.5f);
			const int maxMip = surfaceCountMinusOne;
			if (mipLevel > maxMip)
				mipLevel = maxMip;
			surfaces[mipLevel]->SampleSurface<writeMask>(x, y, samplerState, outColor);
		}
			break;
		case D3DTEXF_ANISOTROPIC    :
		case D3DTEXF_PYRAMIDALQUAD  :
		case D3DTEXF_GAUSSIANQUAD   :
		case D3DTEXF_CONVOLUTIONMONO:
#ifdef _DEBUG
			__debugbreak();
			// Use of these types with D3DSAMP_MIPFILTER is undefined!
#endif
		case D3DTEXF_LINEAR         :
		{
			const int mipLevelLow = (const int)mip;
			const float mipLerp = mip - mipLevelLow;

			if (mipLerp == 0.0f)
			{
				surfaces[mipLevelLow]->SampleSurface<writeMask>(x, y, samplerState, outColor);
			}
			else if (mipLevelLow >= surfaceCountMinusTwo)
			{
				surfaces[surfaceCountMinusOne]->SampleSurface<writeMask>(x, y, samplerState, outColor);
			}
			else
			{
				const int mipLevelHigh = mipLevelLow + 1;

				D3DXVECTOR4 lowColor, highColor;
				surfaces[mipLevelLow]->SampleSurface<writeMask>(x, y, samplerState, lowColor);
				surfaces[mipLevelHigh]->SampleSurface<writeMask>(x, y, samplerState, highColor);

				lrp<writeMask>(outColor, lowColor, highColor, mipLerp);
			}
		}
			break;
		}
	}
}

template void IDirect3DTexture9Hook::SampleTextureLoD4<0>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<1>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<2>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<3>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<4>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<5>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<6>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<7>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<8>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<9>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<10>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<11>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<12>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<13>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<14>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureLoD4<15>(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;

union borderColorUsageUnion
{
	unsigned char usingBorderColorChannel[4];
	unsigned anyUsingBorderColor;
};
static_assert(sizeof(borderColorUsageUnion) == sizeof(unsigned), "Error! Unexpected union size!");

template <const unsigned char writeMask>
void IDirect3DTexture9Hook::SampleTextureLoD4(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const
{
	if (writeMask == 0x0)
	{
		outColor4[0].x = outColor4[0].y = outColor4[0].z = outColor4[0].w = 0.0f;
		outColor4[1].x = outColor4[1].y = outColor4[1].z = outColor4[1].w = 0.0f;
		outColor4[2].x = outColor4[2].y = outColor4[2].z = outColor4[2].w = 0.0f;
		outColor4[3].x = outColor4[3].y = outColor4[3].z = outColor4[3].w = 0.0f;
		return;
	}

	borderColorUsageUnion usingBorderColor4;
	usingBorderColor4.anyUsingBorderColor = 0x00000000;

	switch (samplerState.stateUnion.namedStates.addressU)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#endif
	case D3DTADDRESS_WRAP      :
		if (x4[0] > 1.0f)
			x4[0] = fmodf(x4[0], 1.0f);
		else if (x4[0] < 0.0f)
			x4[0] = 1.0f - fmodf(fabsf(x4[0]), 1.0f);

		if (x4[1] > 1.0f)
			x4[1] = fmodf(x4[1], 1.0f);
		else if (x4[1] < 0.0f)
			x4[1] = 1.0f - fmodf(fabsf(x4[1]), 1.0f);

		if (x4[2] > 1.0f)
			x4[2] = fmodf(x4[2], 1.0f);
		else if (x4[2] < 0.0f)
			x4[2] = 1.0f - fmodf(fabsf(x4[2]), 1.0f);

		if (x4[3] > 1.0f)
			x4[3] = fmodf(x4[3], 1.0f);
		else if (x4[3] < 0.0f)
			x4[3] = 1.0f - fmodf(fabsf(x4[3]), 1.0f);
		break;
	case D3DTADDRESS_MIRROR    :
	{
		const int ix4[4] = 
		{
			(const int)x4[0],
			(const int)x4[1],
			(const int)x4[2],
			(const int)x4[3]
		};
		x4[0] = fmodf(x4[0], 1.0f);
		if (ix4[0] & 0x1)
			x4[0] = 1.0f - x4[0];

		x4[1] = fmodf(x4[1], 1.0f);
		if (ix4[1] & 0x1)
			x4[1] = 1.0f - x4[1];

		x4[2] = fmodf(x4[2], 1.0f);
		if (ix4[2] & 0x1)
			x4[2] = 1.0f - x4[2];

		x4[3] = fmodf(x4[3], 1.0f);
		if (ix4[3] & 0x1)
			x4[3] = 1.0f - x4[3];
	}
		break;
	case D3DTADDRESS_CLAMP     :
		if (x4[0] > 1.0f)
			x4[0] = 1.0f;
		else if (x4[0] < 0.0f)
			x4[0] = 0.0f;

		if (x4[1] > 1.0f)
			x4[1] = 1.0f;
		else if (x4[1] < 0.0f)
			x4[1] = 0.0f;

		if (x4[2] > 1.0f)
			x4[2] = 1.0f;
		else if (x4[2] < 0.0f)
			x4[2] = 0.0f;

		if (x4[3] > 1.0f)
			x4[3] = 1.0f;
		else if (x4[3] < 0.0f)
			x4[3] = 0.0f;
		break;
	case D3DTADDRESS_BORDER    :
		if (x4[0] < 0.0f || x4[0] > 1.0f)
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor4[0]);
			usingBorderColor4.usingBorderColorChannel[0] = 0xFF;
		}
		if (x4[1] < 0.0f || x4[1] > 1.0f)
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor4[1]);
			usingBorderColor4.usingBorderColorChannel[1] = 0xFF;
		}
		if (x4[2] < 0.0f || x4[2] > 1.0f)
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor4[2]);
			usingBorderColor4.usingBorderColorChannel[2] = 0xFF;
		}
		if (x4[3] < 0.0f || x4[3] > 1.0f)
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor4[3]);
			usingBorderColor4.usingBorderColorChannel[3] = 0xFF;
		}

		// Early out in cases where the whole pixel-quad has texcoords outside the border region:
		if (usingBorderColor4.anyUsingBorderColor == 0xFFFFFFFF)
			return;
		break;
	case D3DTADDRESS_MIRRORONCE:
	{
		const float posF[4] = 
		{
			fabsf(x4[0]),
			fabsf(x4[1]),
			fabsf(x4[2]),
			fabsf(x4[3])
		};
		x4[0] = posF[0] > 1.0f ? 1.0f : posF[0];
		x4[1] = posF[1] > 1.0f ? 1.0f : posF[1];
		x4[2] = posF[2] > 1.0f ? 1.0f : posF[2];
		x4[3] = posF[3] > 1.0f ? 1.0f : posF[3];
	}
		break;
	}

	switch (samplerState.stateUnion.namedStates.addressV)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#endif
	case D3DTADDRESS_WRAP      :
		if (y4[0] > 1.0f)
			y4[0] = fmodf(y4[0], 1.0f);
		else if (y4[0] < 0.0f)
			y4[0] = 1.0f - fmodf(fabsf(y4[0]), 1.0f);

		if (y4[1] > 1.0f)
			y4[1] = fmodf(y4[1], 1.0f);
		else if (y4[1] < 0.0f)
			y4[1] = 1.0f - fmodf(fabsf(y4[1]), 1.0f);

		if (y4[2] > 1.0f)
			y4[2] = fmodf(y4[2], 1.0f);
		else if (y4[2] < 0.0f)
			y4[2] = 1.0f - fmodf(fabsf(y4[2]), 1.0f);

		if (y4[3] > 1.0f)
			y4[3] = fmodf(y4[3], 1.0f);
		else if (y4[3] < 0.0f)
			y4[3] = 1.0f - fmodf(fabsf(y4[3]), 1.0f);
		break;
	case D3DTADDRESS_MIRROR    :
	{
		const int iy4[4] = 
		{
			(const int)y4[0],
			(const int)y4[1],
			(const int)y4[2],
			(const int)y4[3]
		};
		y4[0] = fmodf(y4[0], 1.0f);
		if (iy4[0] & 0x1)
			y4[0] = 1.0f - y4[0];

		y4[1] = fmodf(y4[1], 1.0f);
		if (iy4[1] & 0x1)
			y4[1] = 1.0f - y4[1];

		y4[2] = fmodf(y4[2], 1.0f);
		if (iy4[2] & 0x1)
			y4[2] = 1.0f - y4[2];

		y4[3] = fmodf(y4[3], 1.0f);
		if (iy4[3] & 0x1)
			y4[3] = 1.0f - y4[3];
	}
		break;
	case D3DTADDRESS_CLAMP     :
		if (y4[0] > 1.0f)
			y4[0] = 1.0f;
		else if (y4[0] < 0.0f)
			y4[0] = 0.0f;

		if (y4[1] > 1.0f)
			y4[1] = 1.0f;
		else if (y4[1] < 0.0f)
			y4[1] = 0.0f;

		if (y4[2] > 1.0f)
			y4[2] = 1.0f;
		else if (y4[2] < 0.0f)
			y4[2] = 0.0f;

		if (y4[3] > 1.0f)
			y4[3] = 1.0f;
		else if (y4[3] < 0.0f)
			y4[3] = 0.0f;
		break;
	case D3DTADDRESS_BORDER    :
		if (!usingBorderColor4.usingBorderColorChannel[0] && (y4[0] < 0.0f || y4[0] > 1.0f) )
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor4[0]);
			usingBorderColor4.usingBorderColorChannel[0] = 0xFF;
		}
		if (!usingBorderColor4.usingBorderColorChannel[1] && (y4[1] < 0.0f || y4[1] > 1.0f) )
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor4[1]);
			usingBorderColor4.usingBorderColorChannel[1] = 0xFF;
		}
		if (!usingBorderColor4.usingBorderColorChannel[2] && (y4[2] < 0.0f || y4[2] > 1.0f) )
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor4[2]);
			usingBorderColor4.usingBorderColorChannel[2] = 0xFF;
		}
		if (!usingBorderColor4.usingBorderColorChannel[3] && (y4[3] < 0.0f || y4[3] > 1.0f) )
		{
			ColorDWORDToFloat4(samplerState.stateUnion.namedStates.borderColor, outColor4[3]);
			usingBorderColor4.usingBorderColorChannel[3] = 0xFF;
		}

		// Early out in cases where the whole pixel-quad has texcoords outside the border region:
		if (usingBorderColor4.anyUsingBorderColor == 0xFFFFFFFF)
			return;
		break;
	case D3DTADDRESS_MIRRORONCE:
	{
		const float posF[4] = 
		{
			fabsf(y4[0]),
			fabsf(y4[1]),
			fabsf(y4[2]),
			fabsf(y4[3])
		};
		y4[0] = posF[0] > 1.0f ? 1.0f : posF[0];
		y4[1] = posF[1] > 1.0f ? 1.0f : posF[1];
		y4[2] = posF[2] > 1.0f ? 1.0f : posF[2];
		y4[3] = posF[3] > 1.0f ? 1.0f : posF[3];
	}
		break;
	}

	mip4[0] += samplerState.stateUnion.namedStates.mipMapLoDBias;
	mip4[1] += samplerState.stateUnion.namedStates.mipMapLoDBias;
	mip4[2] += samplerState.stateUnion.namedStates.mipMapLoDBias;
	mip4[3] += samplerState.stateUnion.namedStates.mipMapLoDBias;

	const float maxMipLevel = samplerState.cachedFloatMaxMipLevel;

	if (mip4[0] < maxMipLevel)
		mip4[0] = maxMipLevel;
	if (mip4[1] < maxMipLevel)
		mip4[1] = maxMipLevel;
	if (mip4[2] < maxMipLevel)
		mip4[2] = maxMipLevel;
	if (mip4[3] < maxMipLevel)
		mip4[3] = maxMipLevel;

	const float minMipLevel = surfaceCountMinusOneF;

	if (mip4[0] > minMipLevel)
		mip4[0] = minMipLevel;
	if (mip4[1] > minMipLevel)
		mip4[1] = minMipLevel;
	if (mip4[2] > minMipLevel)
		mip4[2] = minMipLevel;
	if (mip4[3] > minMipLevel)
		mip4[3] = minMipLevel;

	if (mip4[0] < 0.0f)
		mip4[0] = 0.0f;
	if (mip4[1] < 0.0f)
		mip4[1] = 0.0f;
	if (mip4[2] < 0.0f)
		mip4[2] = 0.0f;
	if (mip4[3] < 0.0f)
		mip4[3] = 0.0f;

	switch (samplerState.stateUnion.namedStates.mipFilter)
	{
	default:
#ifdef _DEBUG
			__debugbreak(); // Invalid mip filter!
#endif
	case D3DTEXF_NONE           : // Mip-mapping is disabled, always read level 0 of the surface
	{
		if (usingBorderColor4.anyUsingBorderColor == 0x00000000)
		{
			surfaceLevel0->SampleSurface4<writeMask>(x4, y4, samplerState, outColor4);
		}
		else
		{
			if (!usingBorderColor4.usingBorderColorChannel[0])
				surfaceLevel0->SampleSurface<writeMask>(x4[0], y4[0], samplerState, outColor4[0]);
			if (!usingBorderColor4.usingBorderColorChannel[1])
				surfaceLevel0->SampleSurface<writeMask>(x4[1], y4[1], samplerState, outColor4[1]);
			if (!usingBorderColor4.usingBorderColorChannel[2])
				surfaceLevel0->SampleSurface<writeMask>(x4[2], y4[2], samplerState, outColor4[2]);
			if (!usingBorderColor4.usingBorderColorChannel[3])
				surfaceLevel0->SampleSurface<writeMask>(x4[3], y4[3], samplerState, outColor4[3]);
		}
	}
		break;
	case D3DTEXF_POINT          :
	{
		const int maxMip = surfaceCountMinusOne;
		int mipLevel4[4] =
		{
			(const int)(mip4[0] + 0.5f),
			(const int)(mip4[1] + 0.5f),
			(const int)(mip4[2] + 0.5f),
			(const int)(mip4[3] + 0.5f)
		};
		if (mipLevel4[0] > maxMip)
			mipLevel4[0] = maxMip;
		if (mipLevel4[1] > maxMip)
			mipLevel4[1] = maxMip;
		if (mipLevel4[2] > maxMip)
			mipLevel4[2] = maxMip;
		if (mipLevel4[3] > maxMip)
			mipLevel4[3] = maxMip;

		if ( (usingBorderColor4.anyUsingBorderColor == 0x00000000) &&
			(mipLevel4[0] == mipLevel4[1]) &&
			(mipLevel4[2] == mipLevel4[3]) &&
			(mipLevel4[0] == mipLevel4[2]) )
		{
			surfaces[mipLevel4[0] ]->SampleSurface4<writeMask>(x4, y4, samplerState, outColor4);
		}
		else
		{
			if (!usingBorderColor4.usingBorderColorChannel[0])
				surfaces[mipLevel4[0] ]->SampleSurface<writeMask>(x4[0], y4[0], samplerState, outColor4[0]);
			if (!usingBorderColor4.usingBorderColorChannel[1])
				surfaces[mipLevel4[1] ]->SampleSurface<writeMask>(x4[1], y4[1], samplerState, outColor4[1]);
			if (!usingBorderColor4.usingBorderColorChannel[2])
				surfaces[mipLevel4[2] ]->SampleSurface<writeMask>(x4[2], y4[2], samplerState, outColor4[2]);
			if (!usingBorderColor4.usingBorderColorChannel[3])
				surfaces[mipLevel4[3] ]->SampleSurface<writeMask>(x4[3], y4[3], samplerState, outColor4[3]);
		}
	}
		break;
	case D3DTEXF_ANISOTROPIC    :
	case D3DTEXF_PYRAMIDALQUAD  :
	case D3DTEXF_GAUSSIANQUAD   :
	case D3DTEXF_CONVOLUTIONMONO:
#ifdef _DEBUG
			__debugbreak();
			// Use of these types with D3DSAMP_MIPFILTER is undefined!
#endif
	case D3DTEXF_LINEAR         :
	{
		const int mipLevelLow4[4] = 
		{
			(const int)mip4[0],
			(const int)mip4[1],
			(const int)mip4[2],
			(const int)mip4[3]
		};
		const float mipLerp4[4] = 
		{
			mip4[0] - mipLevelLow4[0],
			mip4[1] - mipLevelLow4[1],
			mip4[2] - mipLevelLow4[2],
			mip4[3] - mipLevelLow4[3]
		};

		const int maxMip = (const int)surfaceCountMinusOne;
		const int maxMipLevelLow = maxMip - 1;

		// See if we can use a unified miplevel for our sample4, or if we have to do four individual samples to different miplevel surfaces:
		if (usingBorderColor4.anyUsingBorderColor == 0x00000000)
		{
			// Well this is going to be tough to vectorize...
			if ( (mipLerp4[0] == mipLerp4[1]) &&
				(mipLerp4[2] == mipLerp4[3]) &&
				(mipLerp4[0] == mipLerp4[2]) &&
				(mipLerp4[0] == 0.0f) &&
				(mipLevelLow4[0] == mipLevelLow4[1]) &&
				(mipLevelLow4[2] == mipLevelLow4[3]) &&
				(mipLevelLow4[0] == mipLevelLow4[2]) )
			{
				surfaces[mipLevelLow4[0] ]->SampleSurface4<writeMask>(x4, y4, samplerState, outColor4);
				return;
			}
			else if (mipLevelLow4[0] >= maxMipLevelLow &&
				mipLevelLow4[1] >= maxMipLevelLow && 
				mipLevelLow4[2] >= maxMipLevelLow && 
				mipLevelLow4[3] >= maxMipLevelLow)
			{
				surfaces[maxMip]->SampleSurface4<writeMask>(x4, y4, samplerState, outColor4);
				return;
			}
			else if (mipLevelLow4[0] == mipLevelLow4[1] &&
					 mipLevelLow4[2] == mipLevelLow4[3] &&
					 mipLevelLow4[0] == mipLevelLow4[2])
			{
				D3DXVECTOR4 lowColor4[4];
				surfaces[mipLevelLow4[0] ]->SampleSurface4<writeMask>(x4, y4, samplerState, lowColor4);
				D3DXVECTOR4 highColor4[4];
				surfaces[mipLevelLow4[0] + 1]->SampleSurface4<writeMask>(x4, y4, samplerState, highColor4);

				lrp<writeMask>(outColor4[0], lowColor4[0], highColor4[0], mipLerp4[0]);
				lrp<writeMask>(outColor4[1], lowColor4[1], highColor4[1], mipLerp4[1]);
				lrp<writeMask>(outColor4[2], lowColor4[2], highColor4[2], mipLerp4[2]);
				lrp<writeMask>(outColor4[3], lowColor4[3], highColor4[3], mipLerp4[3]);
				return;
			}
		}

		// Sad times, we have no choice but to do separate samples from different mip-level surfaces:
		if (!usingBorderColor4.usingBorderColorChannel[0])
		{
			if (mipLerp4[0] == 0.0f)
				surfaces[mipLevelLow4[0] ]->SampleSurface<writeMask>(x4[0], y4[0], samplerState, outColor4[0]);
			else if (mipLevelLow4[0] >= maxMipLevelLow)
				surfaces[maxMip]->SampleSurface<writeMask>(x4[0], y4[0], samplerState, outColor4[0]);
			else
			{
				D3DXVECTOR4 lowColor, highColor;
				surfaces[mipLevelLow4[0] ]->SampleSurface<writeMask>(x4[0], y4[0], samplerState, lowColor);
				surfaces[mipLevelLow4[0] + 1]->SampleSurface<writeMask>(x4[0], y4[0], samplerState, highColor);
				lrp<writeMask>(outColor4[0], lowColor, highColor, mipLerp4[0]);
			}
		}

		if (!usingBorderColor4.usingBorderColorChannel[1])
		{
			if (mipLerp4[1] == 0.0f)
				surfaces[mipLevelLow4[1] ]->SampleSurface<writeMask>(x4[1], y4[1], samplerState, outColor4[1]);
			else if (mipLevelLow4[1] >= maxMipLevelLow)
				surfaces[maxMip]->SampleSurface<writeMask>(x4[1], y4[1], samplerState, outColor4[1]);
			else
			{
				D3DXVECTOR4 lowColor, highColor;
				surfaces[mipLevelLow4[1] ]->SampleSurface<writeMask>(x4[1], y4[1], samplerState, lowColor);
				surfaces[mipLevelLow4[1] + 1]->SampleSurface<writeMask>(x4[1], y4[1], samplerState, highColor);
				lrp<writeMask>(outColor4[1], lowColor, highColor, mipLerp4[1]);
			}
		}

		if (!usingBorderColor4.usingBorderColorChannel[2])
		{
			if (mipLerp4[2] == 0.0f)
				surfaces[mipLevelLow4[2] ]->SampleSurface<writeMask>(x4[2], y4[2], samplerState, outColor4[2]);
			else if (mipLevelLow4[2] >= maxMipLevelLow)
				surfaces[maxMip]->SampleSurface<writeMask>(x4[2], y4[2], samplerState, outColor4[2]);
			else
			{
				D3DXVECTOR4 lowColor, highColor;
				surfaces[mipLevelLow4[2] ]->SampleSurface<writeMask>(x4[2], y4[2], samplerState, lowColor);
				surfaces[mipLevelLow4[2] + 1]->SampleSurface<writeMask>(x4[2], y4[2], samplerState, highColor);
				lrp<writeMask>(outColor4[2], lowColor, highColor, mipLerp4[2]);
			}
		}

		if (!usingBorderColor4.usingBorderColorChannel[3])
		{
			if (mipLerp4[3] == 0.0f)
				surfaces[mipLevelLow4[3] ]->SampleSurface<writeMask>(x4[3], y4[3], samplerState, outColor4[3]);
			else if (mipLevelLow4[3] >= maxMipLevelLow)
				surfaces[maxMip]->SampleSurface<writeMask>(x4[3], y4[3], samplerState, outColor4[3]);
			else
			{
				D3DXVECTOR4 lowColor, highColor;
				surfaces[mipLevelLow4[3] ]->SampleSurface<writeMask>(x4[3], y4[3], samplerState, lowColor);
				surfaces[mipLevelLow4[3] + 1]->SampleSurface<writeMask>(x4[3], y4[3], samplerState, highColor);
				lrp<writeMask>(outColor4[3], lowColor, highColor, mipLerp4[3]);
			}
		}
	}
		break;
	}
}

template void IDirect3DTexture9Hook::SampleTextureGrad<0>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<1>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<2>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<3>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<4>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<5>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<6>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<7>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<8>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<9>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<10>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<11>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<12>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<13>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<14>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGrad<15>(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;

static inline const float dot2(const D3DXVECTOR4& a, const D3DXVECTOR4& b)
{
	return (a.x * b.x) + (a.y * b.y);
}

template <const unsigned char writeMask>
void IDirect3DTexture9Hook::SampleTextureGrad(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const
{
	const unsigned numMipLevels = surfaces.size();
	if (numMipLevels <= 1)
		return SampleTextureLoD<writeMask>(x, y, 0.0f, samplerState, outColor);
	else
	{
		// Compute the mip-level here:
		const float deltaXSquared = dot2(texDdx, texDdx);
		const float deltaYSquared = dot2(texDdy, texDdy);
		const float maxDeltaSquared = deltaXSquared >= deltaYSquared ? deltaXSquared : deltaYSquared;
		const float mip = 0.5f * log2_lowp(maxDeltaSquared) * numMipLevels;
		return SampleTextureLoD<writeMask>(x, y, mip, samplerState, outColor);
	}
}

template void IDirect3DTexture9Hook::SampleTextureGrad4<0>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<1>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<2>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<3>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<4>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<5>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<6>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<7>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<8>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<9>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<10>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<11>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<12>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<13>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<14>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGrad4<15>(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;

static inline void dot2_4(const D3DXVECTOR4 (&a4)[4], const D3DXVECTOR4 (&b4)[4], float (&outDot4)[4])
{
	outDot4[0] = (a4[0].x * b4[0].x) + (a4[0].y * b4[0].y);
	outDot4[1] = (a4[1].x * b4[1].x) + (a4[1].y * b4[1].y);
	outDot4[2] = (a4[2].x * b4[2].x) + (a4[2].y * b4[2].y);
	outDot4[3] = (a4[3].x * b4[3].x) + (a4[3].y * b4[3].y);
}

template <const unsigned char writeMask>
void IDirect3DTexture9Hook::SampleTextureGrad4(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const
{
	const unsigned numMipLevels = surfaces.size();
	if (numMipLevels <= 1)
	{
		float zeroMip4[4] =
		{
			0.0f,
			0.0f,
			0.0f,
			0.0f
		};
		return SampleTextureLoD4<writeMask>(x4, y4, zeroMip4, samplerState, outColor4);
	}
	else
	{
		// Compute the mip-level here:
		float deltaXSquared4[4];
		float deltaYSquared4[4];
		dot2_4(texDdx4, texDdx4, deltaXSquared4);
		dot2_4(texDdy4, texDdy4, deltaYSquared4);
		float maxDeltaSquared4[4];
		maxDeltaSquared4[0] = deltaXSquared4[0] >= deltaYSquared4[0] ? deltaXSquared4[0] : deltaYSquared4[0];
		maxDeltaSquared4[1] = deltaXSquared4[1] >= deltaYSquared4[1] ? deltaXSquared4[1] : deltaYSquared4[1];
		maxDeltaSquared4[2] = deltaXSquared4[2] >= deltaYSquared4[2] ? deltaXSquared4[2] : deltaYSquared4[2];
		maxDeltaSquared4[3] = deltaXSquared4[3] >= deltaYSquared4[3] ? deltaXSquared4[3] : deltaYSquared4[3];
		const float halfNumMips = 0.5f * numMipLevels;
		float mip4[4] =
		{
			log2_lowp(maxDeltaSquared4[0]) * halfNumMips,
			log2_lowp(maxDeltaSquared4[1]) * halfNumMips,
			log2_lowp(maxDeltaSquared4[2]) * halfNumMips,
			log2_lowp(maxDeltaSquared4[3]) * halfNumMips
		};
		return SampleTextureLoD4<writeMask>(x4, y4, mip4, samplerState, outColor4);
	}
}

template void IDirect3DTexture9Hook::SampleTextureGradBias<0>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<1>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<2>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<3>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<4>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<5>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<6>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<7>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<8>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<9>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<10>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<11>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<12>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<13>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<14>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias<15>(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;

template <const unsigned char writeMask>
void IDirect3DTexture9Hook::SampleTextureGradBias(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const
{
	const unsigned numMipLevels = surfaces.size();
	if (numMipLevels <= 1)
		return SampleTextureLoD<writeMask>(x, y, mipBias, samplerState, outColor);
	else
	{
		// Compute the mip-level here:
		const float deltaXSquared = dot2(texDdx, texDdx);
		const float deltaYSquared = dot2(texDdy, texDdy);
		const float maxDeltaSquared = deltaXSquared >= deltaYSquared ? deltaXSquared : deltaYSquared;
		const float mip = 0.5f * log2_lowp(maxDeltaSquared) * numMipLevels;
		return SampleTextureLoD<writeMask>(x, y, mip + mipBias, samplerState, outColor);
	}
}

template void IDirect3DTexture9Hook::SampleTextureGradBias4<0>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<1>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<2>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<3>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<4>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<5>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<6>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<7>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<8>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<9>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<10>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<11>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<12>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<13>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<14>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DTexture9Hook::SampleTextureGradBias4<15>(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;

template <const unsigned char writeMask>
void IDirect3DTexture9Hook::SampleTextureGradBias4(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const
{
	const unsigned numMipLevels = surfaces.size();
	if (numMipLevels <= 1)
	{
		float biasMip4[4] =
		{
			mipBias4[0],
			mipBias4[1],
			mipBias4[2],
			mipBias4[3]
		};
		return SampleTextureLoD4<writeMask>(x4, y4, biasMip4, samplerState, outColor4);
	}
	else
	{
		// Compute the mip-level here:
		float deltaXSquared4[4];
		float deltaYSquared4[4];
		dot2_4(texDdx4, texDdx4, deltaXSquared4);
		dot2_4(texDdy4, texDdy4, deltaYSquared4);
		float maxDeltaSquared4[4];
		maxDeltaSquared4[0] = deltaXSquared4[0] >= deltaYSquared4[0] ? deltaXSquared4[0] : deltaYSquared4[0];
		maxDeltaSquared4[1] = deltaXSquared4[1] >= deltaYSquared4[1] ? deltaXSquared4[1] : deltaYSquared4[1];
		maxDeltaSquared4[2] = deltaXSquared4[2] >= deltaYSquared4[2] ? deltaXSquared4[2] : deltaYSquared4[2];
		maxDeltaSquared4[3] = deltaXSquared4[3] >= deltaYSquared4[3] ? deltaXSquared4[3] : deltaYSquared4[3];
		const float halfNumMips = 0.5f * numMipLevels;
		float mip4biased[4] =
		{
			log2_lowp(maxDeltaSquared4[0]) * halfNumMips + mipBias4[0],
			log2_lowp(maxDeltaSquared4[1]) * halfNumMips + mipBias4[1],
			log2_lowp(maxDeltaSquared4[2]) * halfNumMips + mipBias4[2],
			log2_lowp(maxDeltaSquared4[3]) * halfNumMips + mipBias4[3]
		};
		return SampleTextureLoD4<writeMask>(x4, y4, mip4biased, samplerState, outColor4);
	}
}
