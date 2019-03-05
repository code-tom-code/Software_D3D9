#pragma once

#include "GlobalToggles.h"
#include "IDirect3DSurface9Hook.h"

static inline const bool IsCompressedFormat(const D3DFORMAT format)
{
	switch (format)
	{
	case D3DFMT_DXT1:
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
		return true;
	default:
		return false;
	}
}

#pragma pack(push)
#pragma pack(1)
struct DXT1Chunk
{
	unsigned short c0;
	unsigned short c1;
	unsigned char interpLookup4x4[4];
};
static_assert(sizeof(DXT1Chunk) == 8, "Error: Unexpected DXT1 chunk size!");

struct DXT3Chunk
{
	unsigned char alphaValues[8];
	DXT1Chunk Colors;
};
static_assert(sizeof(DXT3Chunk) == 16, "Error: Unexpected DXT3 chunk size!");

struct DXT5Chunk
{
	unsigned char alpha_c0;
	unsigned char alpha_c1;
	unsigned char alphaInterpLookup4x4[6];
	DXT1Chunk Colors;
};
static_assert(sizeof(DXT5Chunk) == 16, "Error: Unexpected DXT5 chunk size!");

#pragma pack(pop)

static inline const D3DCOLOR DXT565To888(const unsigned short color565)
{
	const unsigned short b5 = (color565 & ( (0x20 - 1) << 11) ) >> 11;
	const unsigned short g6 = (color565 & ( (0x40 - 1) << 5) ) >> 5;
	const unsigned short r5 = (color565 & (0x20 - 1) );

	const unsigned short b8 = r5 << 3;
	const unsigned short g8 = g6 << 2;
	const unsigned short r8 = b5 << 3;

	return D3DCOLOR_XRGB(r8, g8, b8);
}


/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	HRESULT ret = realObject->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DSurface9Hook::AddRef(THIS)
{
	ULONG ret = realObject->AddRef();
	++refCount;
#ifdef _DEBUG
	char buffer[256] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
	sprintf(buffer, "AddRef on surface: 0x%08X; RefCount is now: %u\n", (const unsigned)this, (const unsigned)refCount);
#pragma warning(pop)
	OutputDebugStringA(buffer);
#endif
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DSurface9Hook::Release(THIS)
{
	ULONG ret = realObject->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked Surface %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

/*** IDirect3DResource9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
{
	LPDIRECT3DDEVICE9 realD3D9dev = NULL;
	HRESULT ret = realObject->GetDevice(&realD3D9dev);
	if (FAILED(ret) )
	{
		*ppDevice = NULL;
		return ret;
	}

	// Check that the parentHook's underlying IDirect3DDevice9* matches the realD3D9dev pointer
	if (parentDevice->GetUnderlyingDevice() != realD3D9dev)
	{
		DbgBreakPrint("Error: Unknown d3d9 device hook detected!");
	}
	parentDevice->AddRef(); // Super important to increment the ref-count here, otherwise our parent object will get destroyed when Release() is called on it!

	*ppDevice = parentDevice;
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::SetPrivateData(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
{
	HRESULT ret = realObject->SetPrivateData(refguid, pData, SizeOfData, Flags);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::GetPrivateData(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)
{
	HRESULT ret = realObject->GetPrivateData(refguid, pData, pSizeOfData);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::FreePrivateData(THIS_ REFGUID refguid)
{
	HRESULT ret = realObject->FreePrivateData(refguid);
	return ret;
}

COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE IDirect3DSurface9Hook::SetPriority(THIS_ DWORD PriorityNew)
{
	DWORD ret = realObject->SetPriority(PriorityNew);
	return ret;
}

COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE IDirect3DSurface9Hook::GetPriority(THIS)
{
	DWORD ret = realObject->GetPriority();
	return ret;
}

COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE IDirect3DSurface9Hook::PreLoad(THIS)
{
	realObject->PreLoad();
}

COM_DECLSPEC_NOTHROW D3DRESOURCETYPE STDMETHODCALLTYPE IDirect3DSurface9Hook::GetType(THIS)
{
	D3DRESOURCETYPE ret = realObject->GetType();
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::GetContainer(THIS_ REFIID riid,void** ppContainer)
{
	HRESULT ret = realObject->GetContainer(riid, ppContainer);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	// TODO: Validate that the surface's container pointer matches the inner object pointer of the HookParentTexture
#endif

	*ppContainer = HookParentTexturePtr;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::GetDesc(THIS_ D3DSURFACE_DESC *pDesc)
{
	if (!pDesc)
		return D3DERR_INVALIDCALL;

#ifdef _DEBUG
	HRESULT ret = realObject->GetDesc(pDesc);
	if (FAILED(ret) )
	{
		// We should have caught this error, but didn't for some reason
		__debugbreak();
	}

	if (pDesc != NULL)
	{
		if (pDesc->Width != InternalWidth)
		{
			__debugbreak();
		}
		if (pDesc->Height != InternalHeight)
		{
			__debugbreak();
		}
		if (pDesc->Format != InternalFormat)
		{
			__debugbreak();
		}
		if (pDesc->Pool != InternalPool)
		{
			__debugbreak();
		}
		if (pDesc->Usage != InternalUsage)
		{
			__debugbreak();
		}
		if (pDesc->Type != D3DRTYPE_SURFACE)
		{
			__debugbreak();
		}
		if (pDesc->MultiSampleType != InternalMultiSampleType)
		{
			__debugbreak();
		}
		if (pDesc->MultiSampleQuality != InternalMultiSampleQuality)
		{
			__debugbreak();
		}
	}
#endif

	pDesc->Width = InternalWidth;
	pDesc->Height = InternalHeight;
	pDesc->Format = InternalFormat;
	pDesc->Pool = InternalPool;
	pDesc->Usage = InternalUsage;
	pDesc->Type = D3DRTYPE_SURFACE;
	pDesc->MultiSampleType = InternalMultiSampleType;
	pDesc->MultiSampleQuality = InternalMultiSampleQuality;

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::LockRect(THIS_ D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags)
{
#ifdef _DEBUG
	if (!realObject)
	{
		// Should never have an invalid internal surface object
		__debugbreak();
	}

	//if (LockableRT && pRect)
	//{
		// Only for debugging UT2004, remove later!
		//__debugbreak();
	//}

	{
		char buffer[256] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(buffer, "0x%08X::LockRect(pLockedRect = 0x%08X, pRect = 0x%08X, Flags = 0x%08X)\n", (const unsigned)this, (const unsigned)pLockedRect, (const unsigned)pRect, Flags);
#pragma warning(pop)
		OutputDebugStringA(buffer);
	}

	D3DLOCKED_RECT realRect = {0};
	HRESULT ret = realObject->LockRect(&realRect, pRect, Flags);
	if (FAILED(ret) )
	{
		realObject->UnlockRect();
		return ret;
	}
	realObject->UnlockRect();
#endif

#ifdef SURFACE_MAGIC_COOKIE
	ValidateSurfaceMagicCookie(surfaceBytes);
#endif

	if (!pLockedRect)
		return D3DERR_INVALIDCALL;

	static const DWORD validLockFlags = D3DLOCK_DISCARD | D3DLOCK_DONOTWAIT | D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY;
	if (Flags & (~validLockFlags) )
		return D3DERR_INVALIDCALL; // These are the only D3DLOCK flags valid for this function call

	if (creationMethod == deviceImplicitDepthStencil || creationMethod == createDepthStencil)
	{
		switch (InternalFormat)
		{
		default:
			return D3DERR_INVALIDCALL;
		case D3DFMT_D16_LOCKABLE:
		case D3DFMT_D32F_LOCKABLE:
		case D3DFMT_D32_LOCKABLE:
		case D3DFMT_S8_LOCKABLE:
			break;
		}
	}

	if (InternalPool == D3DPOOL_DEFAULT)
	{
		if (Flags & D3DLOCK_READONLY)
		{
		}
		else
		{
			if (!(InternalUsage & D3DUSAGE_DYNAMIC) )
				return D3DERR_INVALIDCALL;
		}
	}

	if (InternalMultiSampleType != D3DMULTISAMPLE_NONE)
		return D3DERR_INVALIDCALL;

	if (pRect)
	{
		// It is illegal to use the D3DLOCK_DISCARD flag with a subrect: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205896(v=vs.85).aspx
		if (Flags & D3DLOCK_DISCARD)
			return D3DERR_INVALIDCALL;

		// RECT validation:
		if (pRect->left < 0 || pRect->right < 0 || pRect->top < 0 || pRect->bottom < 0)
			return D3DERR_INVALIDCALL;
		if (pRect->left >= pRect->right)
			return D3DERR_INVALIDCALL;
		if (pRect->top >= pRect->bottom)
			return D3DERR_INVALIDCALL;
		if (pRect->left >= (const long)InternalWidth)
			return D3DERR_INVALIDCALL;
		if (pRect->right > (const long)InternalWidth)
			return D3DERR_INVALIDCALL;
		if (pRect->top >= (const long)InternalHeight)
			return D3DERR_INVALIDCALL;
		if (pRect->bottom > (const long)InternalHeight)
			return D3DERR_INVALIDCALL;

		D3DCOLOR* const pixels = (D3DCOLOR* const)surfaceBytesRaw;
		pLockedRect->pBits = pixels + pRect->top * InternalWidth + pRect->left;
	}
	else
	{
#ifdef SURFACE_ENFORCE_DISCARD_ON_LOCK
		if (Flags & D3DLOCK_DISCARD)
		{
			memset(surfaceBytesRaw, 0, surfaceBytesRawSize);
			if (auxSurfaceBytesRaw)
				memset(auxSurfaceBytesRaw, 0, auxSurfaceBytesRawSize);
		}
#endif

		pLockedRect->pBits = surfaceBytesRaw;
	}

	pLockedRect->Pitch = GetSurfacePitchBytes(InternalWidth, InternalFormat);

#ifdef _DEBUG
	if (pLockedRect->Pitch != realRect.Pitch)
	{
		char buffer[256] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(buffer, "Lock rect failed due to pitch misalignment. Pitch = %u, expected pitch = %u. Format = %u. Dims = %ux%u\n", realRect.Pitch, pLockedRect->Pitch, InternalFormat, InternalWidth, InternalHeight);
#pragma warning(pop)
		OutputDebugStringA(buffer);
	}

	if ( (pLockedRect->Pitch & 0x1) != 0)
	{
		// Should never get an odd pitch back
		__debugbreak();
	}

	if (!pLockedRect->pBits)
	{
		// Should never get a NULL pointer back
		__debugbreak();
	}
#endif // _DEBUG

#ifdef SURFACE_MAGIC_COOKIE
	DWORD* const magicDword = (DWORD* const)surfaceBytesRaw + (surfaceBytes.size() / sizeof(DWORD) - 1);
	*magicDword = surfaceMagicBytes;
#endif // SURFACE_MAGIC_COOKIE

	return S_OK;
}

void IDirect3DSurface9Hook::DecompressSurfaceDXT1()
{
	const DXT1Chunk* dxt1Chunks = (const DXT1Chunk* const)surfaceBytesRaw;

	D3DCOLOR* const rawPixels = (D3DCOLOR* const)auxSurfaceBytesRaw;

	for (unsigned y = 0; y < InternalHeight; y += 4)
	{
		for (unsigned x = 0; x < InternalWidth; x += 4)
		{
			const DXT1Chunk& chunk = *dxt1Chunks++;

			const D3DCOLOR c0 = DXT565To888(chunk.c0);
			const D3DCOLOR c1 = DXT565To888(chunk.c1);
			D3DCOLOR c2;
			D3DCOLOR c3;

			const unsigned r0 = (c0 & 0x00FF0000) >> 16;
			const unsigned g0 = (c0 & 0x0000FF00) >> 8;
			const unsigned b0 = (c0 & 0x000000FF);

#ifdef _DEBUG
			if (r0 > 255)
			{
				__debugbreak();
			}
			if (g0 > 255)
			{
				__debugbreak();
			}
			if (b0 > 255)
			{
				__debugbreak();
			}
#endif

			const unsigned r1 = (c1 & 0x00FF0000) >> 16;
			const unsigned g1 = (c1 & 0x0000FF00) >> 8;
			const unsigned b1 = (c1 & 0x000000FF);

#ifdef _DEBUG
			if (r1 > 255)
			{
				__debugbreak();
			}
			if (g1 > 255)
			{
				__debugbreak();
			}
			if (b1 > 255)
			{
				__debugbreak();
			}
#endif

			if (chunk.c0 > chunk.c1)
			{
				// c2 = (2/3)c0 + (1/3)c1
				const unsigned r2 = (r0 + r0 + r1 + 1) / 3;
				const unsigned g2 = (g0 + g0 + g1 + 1) / 3;
				const unsigned b2 = (b0 + b0 + b1 + 1) / 3;
#ifdef _DEBUG
				if (r2 > 255)
				{
					__debugbreak();
				}
				if (g2 > 255)
				{
					__debugbreak();
				}
				if (b2 > 255)
				{
					__debugbreak();
				}
#endif
				c2 = D3DCOLOR_XRGB(r2, g2, b2);

				// c2 = (1/3)c0 + (2/3)c1
				const unsigned r3 = (r0 + r1 + r1 + 1) / 3;
				const unsigned g3 = (g0 + g1 + g1 + 1) / 3;
				const unsigned b3 = (b0 + b1 + b1 + 1) / 3;
#ifdef _DEBUG
				if (r3 > 255)
				{
					__debugbreak();
				}
				if (g3 > 255)
				{
					__debugbreak();
				}
				if (b3 > 255)
				{
					__debugbreak();
				}
#endif
				c3 = D3DCOLOR_XRGB(r3, g3, b3);
			}
			else
			{
				// c2 = (1/2)c0 + (1/2)c1
				const unsigned r2 = (r0 + r1) / 2;
				const unsigned g2 = (g0 + g1) / 2;
				const unsigned b2 = (b0 + b1) / 2;

#ifdef _DEBUG
				if (r2 > 255)
				{
					__debugbreak();
				}
				if (g2 > 255)
				{
					__debugbreak();
				}
				if (b2 > 255)
				{
					__debugbreak();
				}
#endif

				c2 = D3DCOLOR_XRGB(r2, g2, b2);

				c3 = 0x00000000;
			}

			for (unsigned char dy = 0; dy < 4; ++dy)
			{
				for (unsigned char dx = 0; dx < 4; ++dx)
				{
					const unsigned char pixelByte = chunk.interpLookup4x4[dy];
					unsigned char pixelBits;
					switch (dx)
					{
					case 0:
						pixelBits = pixelByte & 0x3;
						break;
					case 1:
						pixelBits = (pixelByte & (0x3 << 2) ) >> 2;
						break;
					case 2:
						pixelBits = (pixelByte & (0x3 << 4) ) >> 4;
						break;
					case 3:
						pixelBits = (pixelByte & (0x3 << 6) ) >> 6;
						break;
					default:
#ifdef _DEBUG
						__debugbreak();
#else
						__assume(0);
#endif
					}

#ifdef _DEBUG
					if (pixelBits > 3)
					{
						__debugbreak();
					}
#endif
					D3DCOLOR pixelColor;
					switch (pixelBits)
					{
					case 0:
						pixelColor = c0;
						break;
					case 1:
						pixelColor = c1;
						break;
					case 2:
						pixelColor = c2;
						break;
					case 3:
						pixelColor = c3;
						break;
					default:
#ifdef _DEBUG
						__debugbreak();
#else
						__assume(0);
#endif
					}

					rawPixels[(y + dy) * InternalWidth + (x + dx)] = pixelColor;
				}
			}
		}
	}
}

static inline const DWORD DecompressDXT3Alpha(const DXT3Chunk& chunk, const unsigned dx, const unsigned dy)
{
	const unsigned char byteIndex = (dy * 4 + dx) / 2;

	unsigned char byteMask;
	if (dx & 0x1)
		byteMask = 0xF0;
	else
		byteMask = 0x0F;

	unsigned char byteShift;
	if (dx & 0x1)
		byteShift = 4;
	else
		byteShift = 0;

	const unsigned char alpha4Bit = (chunk.alphaValues[byteIndex] & byteMask) >> byteShift;

#ifdef _DEBUG
	if (alpha4Bit > 15)
	{
		__debugbreak();
	}
#endif

	const unsigned alpha = alpha4Bit << 4;
	return alpha;
}

void IDirect3DSurface9Hook::DecompressSurfaceDXT3()
{
	const DXT3Chunk* dxt3Chunks = (const DXT3Chunk* const)surfaceBytesRaw;

	D3DCOLOR* const rawPixels = (D3DCOLOR* const)auxSurfaceBytesRaw;

	for (unsigned y = 0; y < InternalHeight; y += 4)
	{
		for (unsigned x = 0; x < InternalWidth; x += 4)
		{
			const DXT3Chunk& chunk = *dxt3Chunks++;

			const D3DCOLOR c0 = DXT565To888(chunk.Colors.c0);
			const D3DCOLOR c1 = DXT565To888(chunk.Colors.c1);
			D3DCOLOR c2;
			D3DCOLOR c3;

			const unsigned r0 = (c0 & 0x00FF0000) >> 16;
			const unsigned g0 = (c0 & 0x0000FF00) >> 8;
			const unsigned b0 = (c0 & 0x000000FF);

#ifdef _DEBUG
			if (r0 > 255)
			{
				__debugbreak();
			}
			if (g0 > 255)
			{
				__debugbreak();
			}
			if (b0 > 255)
			{
				__debugbreak();
			}
#endif

			const unsigned r1 = (c1 & 0x00FF0000) >> 16;
			const unsigned g1 = (c1 & 0x0000FF00) >> 8;
			const unsigned b1 = (c1 & 0x000000FF);

#ifdef _DEBUG
			if (r1 > 255)
			{
				__debugbreak();
			}
			if (g1 > 255)
			{
				__debugbreak();
			}
			if (b1 > 255)
			{
				__debugbreak();
			}
#endif

			// c2 = (2/3)c0 + (1/3)c1
			const unsigned r2 = (r0 + r0 + r1 + 1) / 3;
			const unsigned g2 = (g0 + g0 + g1 + 1) / 3;
			const unsigned b2 = (b0 + b0 + b1 + 1) / 3;
#ifdef _DEBUG
			if (r2 > 255)
			{
				__debugbreak();
			}
			if (g2 > 255)
			{
				__debugbreak();
			}
			if (b2 > 255)
			{
				__debugbreak();
			}
#endif
			c2 = D3DCOLOR_XRGB(r2, g2, b2);

			// c3 = (1/3)c0 + (2/3)c1
			const unsigned r3 = (r0 + r1 + r1 + 1) / 3;
			const unsigned g3 = (g0 + g1 + g1 + 1) / 3;
			const unsigned b3 = (b0 + b1 + b1 + 1) / 3;
#ifdef _DEBUG
			if (r3 > 255)
			{
				__debugbreak();
			}
			if (g3 > 255)
			{
				__debugbreak();
			}
			if (b3 > 255)
			{
				__debugbreak();
			}
#endif
			c3 = D3DCOLOR_XRGB(r3, g3, b3);

			for (unsigned char dy = 0; dy < 4; ++dy)
			{
				const unsigned char pixelByte = chunk.Colors.interpLookup4x4[dy];
				for (unsigned char dx = 0; dx < 4; ++dx)
				{
					unsigned char pixelBits;
					switch (dx)
					{
					case 0:
						pixelBits = pixelByte & 0x3;
						break;
					case 1:
						pixelBits = (pixelByte >> 2) & 0x3;
						break;
					case 2:
						pixelBits = (pixelByte >> 4) & 0x3;
						break;
					case 3:
						pixelBits = (pixelByte >> 6) & 0x3;
						break;
					default:
#ifdef _DEBUG
						__debugbreak();
#else
						__assume(0);
#endif
					}

#ifdef _DEBUG
					if (pixelBits > 3)
					{
						__debugbreak();
					}
#endif
					D3DCOLOR pixelColor;
					switch (pixelBits)
					{
					case 0:
						pixelColor = c0;
						break;
					case 1:
						pixelColor = c1;
						break;
					case 2:
						pixelColor = c2;
						break;
					case 3:
						pixelColor = c3;
						break;
					default:
#ifdef _DEBUG
						__debugbreak();
#else
						__assume(0);
#endif
					}

					const DWORD alpha = DecompressDXT3Alpha(chunk, dx, dy);

					const D3DCOLOR alphaColor = D3DCOLOR_ARGB(alpha, 0, 0, 0);

					rawPixels[(y + dy) * InternalWidth + (x + dx)] = ( (pixelColor & 0x00FFFFFF) | alphaColor);
				}
			}
		}
	}
}

static inline const DWORD DecompressDXT5PixelAlpha(const DXT5Chunk& chunk, const unsigned char pixelIndex)
{
 	// The end of this read will read 16 bits into the colors section of the buffer, this is fine (it's not like it'll cause a seg-fault, and we're not using that data anyway).
	const unsigned char bitIndex = pixelIndex * 3; // Range [0...47]
#ifdef _DEBUG
	if (bitIndex > 47)
	{
		__debugbreak();
	}
#endif

	unsigned __int64 interpLookupNum = *(const unsigned __int64* const)&(chunk.alphaInterpLookup4x4);
	interpLookupNum &= ( ( (const unsigned __int64)0x7) << bitIndex);
	interpLookupNum >>= bitIndex;
	const unsigned char shortBitIndex = (const unsigned char)interpLookupNum; // Range [0...7]
#ifdef _DEBUG
	if (shortBitIndex > 7)
	{
		__debugbreak();
	}
#endif

	unsigned alpha;
	const unsigned a0 = chunk.alpha_c0;
	const unsigned a1 = chunk.alpha_c1;

	if (a0 > a1)
	{
		switch (shortBitIndex)
		{
		case 0:
			alpha = a0;
			break;
		case 1:
			alpha = a1;
			break;
		case 2:
			alpha = (6 * a0 +     a1) / 7;
			break;
		case 3:
			alpha = (5 * a0 + 2 * a1) / 7;
			break;
		case 4:
			alpha = (4 * a0 + 3 * a1) / 7;
			break;
		case 5:
			alpha = (3 * a0 + 4 * a1) / 7;
			break;
		case 6:
			alpha = (2 * a0 + 5 * a1) / 7;
			break;
		case 7:
			alpha = (    a0 + 6 * a1) / 7;
			break;
		default:
#ifdef _DEBUG
			__debugbreak();
#else
			__assume(0);
#endif
		}
	}
	else
	{
		switch (shortBitIndex)
		{
		case 0:
			alpha = a0;
			break;
		case 1:
			alpha = a1;
			break;
		case 2:
			alpha = (4 * a0 +     a1) / 5;
			break;
		case 3:
			alpha = (3 * a0 + 2 * a1) / 5;
			break;
		case 4:
			alpha = (2 * a0 + 3 * a1) / 5;
			break;
		case 5:
			alpha = (    a0 + 4 * a1) / 5;
			break;
		case 6:
			alpha = 0;
			break;
		case 7:
			alpha = 255;
			break;
		default:
#ifdef _DEBUG
			__debugbreak();
#else
			__assume(0);
#endif
		}
	}

#ifdef _DEBUG
	if (alpha > 255)
	{
		__debugbreak();
	}
#endif

	return alpha;
}

void IDirect3DSurface9Hook::DecompressSurfaceDXT5()
{
	const DXT5Chunk* dxt5Chunks = (const DXT5Chunk* const)surfaceBytesRaw;

	D3DCOLOR* const rawPixels = (D3DCOLOR* const)auxSurfaceBytesRaw;

	for (unsigned y = 0; y < InternalHeight; y += 4)
	{
		for (unsigned x = 0; x < InternalWidth; x += 4)
		{
			const DXT5Chunk& chunk = *dxt5Chunks++;

			const D3DCOLOR c0 = DXT565To888(chunk.Colors.c0);
			const D3DCOLOR c1 = DXT565To888(chunk.Colors.c1);
			D3DCOLOR c2;
			D3DCOLOR c3;

			const unsigned r0 = (c0 & 0x00FF0000) >> 16;
			const unsigned g0 = (c0 & 0x0000FF00) >> 8;
			const unsigned b0 = (c0 & 0x000000FF);

#ifdef _DEBUG
			if (r0 > 255)
			{
				__debugbreak();
			}
			if (g0 > 255)
			{
				__debugbreak();
			}
			if (b0 > 255)
			{
				__debugbreak();
			}
#endif

			const unsigned r1 = (c1 & 0x00FF0000) >> 16;
			const unsigned g1 = (c1 & 0x0000FF00) >> 8;
			const unsigned b1 = (c1 & 0x000000FF);

#ifdef _DEBUG
			if (r1 > 255)
			{
				__debugbreak();
			}
			if (g1 > 255)
			{
				__debugbreak();
			}
			if (b1 > 255)
			{
				__debugbreak();
			}
#endif

			// c2 = (2/3)c0 + (1/3)c1
			const unsigned r2 = (r0 + r0 + r1 + 1) / 3;
			const unsigned g2 = (g0 + g0 + g1 + 1) / 3;
			const unsigned b2 = (b0 + b0 + b1 + 1) / 3;
#ifdef _DEBUG
			if (r2 > 255)
			{
				__debugbreak();
			}
			if (g2 > 255)
			{
				__debugbreak();
			}
			if (b2 > 255)
			{
				__debugbreak();
			}
#endif
			c2 = D3DCOLOR_XRGB(r2, g2, b2);

			// c3 = (1/3)c0 + (2/3)c1
			const unsigned r3 = (r0 + r1 + r1 + 1) / 3;
			const unsigned g3 = (g0 + g1 + g1 + 1) / 3;
			const unsigned b3 = (b0 + b1 + b1 + 1) / 3;
#ifdef _DEBUG
			if (r3 > 255)
			{
				__debugbreak();
			}
			if (g3 > 255)
			{
				__debugbreak();
			}
			if (b3 > 255)
			{
				__debugbreak();
			}
#endif
			c3 = D3DCOLOR_XRGB(r3, g3, b3);

			for (unsigned char dy = 0; dy < 4; ++dy)
			{
				const unsigned char pixelByte = chunk.Colors.interpLookup4x4[dy];
				for (unsigned char dx = 0; dx < 4; ++dx)
				{
					unsigned char pixelBits;
					switch (dx)
					{
					case 0:
						pixelBits = pixelByte & 0x3;
						break;
					case 1:
						pixelBits = (pixelByte >> 2) & 0x3;
						break;
					case 2:
						pixelBits = (pixelByte >> 4) & 0x3;
						break;
					case 3:
						pixelBits = (pixelByte >> 6) & 0x3;
						break;
					default:
#ifdef _DEBUG
						__debugbreak();
#else
						__assume(0);
#endif
					}

#ifdef _DEBUG
					if (pixelBits > 3)
					{
						__debugbreak();
					}
#endif
					D3DCOLOR pixelColor;
					switch (pixelBits)
					{
					case 0:
						pixelColor = c0;
						break;
					case 1:
						pixelColor = c1;
						break;
					case 2:
						pixelColor = c2;
						break;
					case 3:
						pixelColor = c3;
						break;
					default:
#ifdef _DEBUG
						__debugbreak();
#else
						__assume(0);
#endif
					}

					const DWORD alpha = DecompressDXT5PixelAlpha(chunk, dy * 4 + dx);

					const D3DCOLOR alphaColor = D3DCOLOR_ARGB(alpha, 0, 0, 0);
					rawPixels[(y + dy) * InternalWidth + (x + dx)] = ( (pixelColor & 0x00FFFFFF) | alphaColor);
				}
			}
		}
	}
}

void IDirect3DSurface9Hook::DecompressSurfaceToAuxBuffer()
{
	switch (InternalFormat)
	{
	case D3DFMT_DXT1:
		DecompressSurfaceDXT1();
		break;
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
		DecompressSurfaceDXT3();
		break;
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
		DecompressSurfaceDXT5();
		break;
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
		break;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::UnlockRect(THIS)
{
#ifdef _DEBUG
	{
		char buffer[256] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(buffer, "0x%08X::UnlockRect()\n", (const unsigned)this);
#pragma warning(pop)
		OutputDebugStringA(buffer);
	}
#endif
	if (IsCompressedFormat(InternalFormat) )
	{
#ifdef SURFACE_MAGIC_COOKIE
		DWORD* const magicDword = (DWORD* const)auxSurfaceBytesRaw + (auxSurfaceBytes.size() / sizeof(DWORD) - 1);
		*magicDword = surfaceMagicBytes;
#endif
		DecompressSurfaceToAuxBuffer();
#ifdef SURFACE_MAGIC_COOKIE
		if (*magicDword != surfaceMagicBytes)
		{
			__debugbreak();
		}
#endif
	}

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif

#ifdef SURFACE_MAGIC_COOKIE
	ValidateSurfaceMagicCookie(surfaceBytes);
#endif

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::GetDC(THIS_ HDC *phdc)
{
	HRESULT ret = realObject->GetDC(phdc);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::ReleaseDC(THIS_ HDC hdc)
{
	HRESULT ret = realObject->ReleaseDC(hdc);
	return ret;
}

const unsigned IDirect3DSurface9Hook::GetSurfacePitchBytes(const unsigned Width, const D3DFORMAT Format)
{
	switch (Format)
	{
	case D3DFMT_UNKNOWN:
		return 0;
	case D3DFMT_R8G8B8				 :
		return RoundUpTo4(Width * 3);
	case D3DFMT_A8R8G8B8             :
	case D3DFMT_X8R8G8B8             :
	case D3DFMT_A2B10G10R10          :
	case D3DFMT_A8B8G8R8             :
	case D3DFMT_X8B8G8R8             :
	case D3DFMT_G16R16               :
	case D3DFMT_A2R10G10B10          :
	case D3DFMT_X8L8V8U8             :
	case D3DFMT_Q8W8V8U8             :
	case D3DFMT_V16U16               :
	case D3DFMT_A2W10V10U10          :
	case D3DFMT_D32                  :
    case D3DFMT_D24S8                :
    case D3DFMT_D24X8                :
    case D3DFMT_D24X4S4              :
    case D3DFMT_D32F_LOCKABLE        :
    case D3DFMT_D24FS8               :
	case D3DFMT_D32_LOCKABLE         :
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	case D3DFMT_INDEX32              :
	case D3DFMT_G16R16F              :
	case D3DFMT_R32F                 :
	case D3DFMT_A2B10G10R10_XR_BIAS  :
		return Width * 4;
	case D3DFMT_R5G6B5               :
	case D3DFMT_X1R5G5B5             :
	case D3DFMT_A1R5G5B5             :
	case D3DFMT_A4R4G4B4             :
	case D3DFMT_A8R3G3B2             :
	case D3DFMT_X4R4G4B4             :
	case D3DFMT_A8P8                 :
	case D3DFMT_A8L8                 :
	case D3DFMT_V8U8                 :
	case D3DFMT_L6V5U5               :
	case D3DFMT_D16_LOCKABLE         :
    case D3DFMT_D15S1                :
    case D3DFMT_D16                  :
	case D3DFMT_L16                  :
    case D3DFMT_INDEX16              :
	case D3DFMT_R16F                 :
	case D3DFMT_CxV8U8               :
		return RoundUpTo4(Width * 2);
	case D3DFMT_R3G3B2               :
	case D3DFMT_A8                   :
	case D3DFMT_P8                   :
	case D3DFMT_L8                   :
	case D3DFMT_A4L4                 :
	case D3DFMT_S8_LOCKABLE          :
		return RoundUpTo4(Width);
	case D3DFMT_A16B16G16R16         :
	case D3DFMT_Q16W16V16U16         :
	case D3DFMT_A16B16G16R16F        :
	case D3DFMT_G32R32F              :
		return Width * 8;
	case D3DFMT_A32B32G32R32F        :
		return Width * 16;
    case D3DFMT_UYVY                 :
    case D3DFMT_R8G8_B8G8            :
    case D3DFMT_YUY2                 :
    case D3DFMT_G8R8_G8B8            :
		return RoundUpTo4(Width);
    case D3DFMT_DXT1                 :
	{
		const unsigned numBytes = Width * 2;
		if (numBytes < sizeof(DXT1Chunk) )
			return sizeof(DXT1Chunk) ;
		return RoundUpTo8(numBytes);
	}
    case D3DFMT_DXT2                 :
    case D3DFMT_DXT3                 :
	{
		const unsigned numBytes = Width * 4;
		if (numBytes < sizeof(DXT3Chunk) )
			return sizeof(DXT3Chunk) ;
		return RoundUpTo16(numBytes);
	}
    case D3DFMT_DXT4                 :
    case D3DFMT_DXT5                 :
	{
		const unsigned numBytes = Width * 4;
		if (numBytes < sizeof(DXT5Chunk) )
			return sizeof(DXT5Chunk) ;
		return RoundUpTo16(numBytes);
	}
	case D3DFMT_A1                   :
	{
		if (Width == 0)
			return 0;
		const unsigned numBytes = Width / 8; // One bit per pixel
		if (numBytes < 4)
			return 4;
		else
			return RoundUpTo4(numBytes);
	}
    case D3DFMT_VERTEXDATA           :
	case D3DFMT_MULTI2_ARGB8         :
	case D3DFMT_BINARYBUFFER         :
	default:
		return RoundUpTo4(Width); // Uhhhhhh...
	}
}

const unsigned IDirect3DSurface9Hook::GetSurfaceSizeBytes(const unsigned Width, const unsigned Height, const D3DFORMAT Format)
{
	switch (Format)
	{
	case D3DFMT_UNKNOWN:
		return 0;
	case D3DFMT_R8G8B8				 :
	case D3DFMT_A8R8G8B8             :
	case D3DFMT_X8R8G8B8             :
	case D3DFMT_A2B10G10R10          :
	case D3DFMT_A8B8G8R8             :
	case D3DFMT_X8B8G8R8             :
	case D3DFMT_G16R16               :
	case D3DFMT_A2R10G10B10          :
	case D3DFMT_X8L8V8U8             :
	case D3DFMT_Q8W8V8U8             :
	case D3DFMT_V16U16               :
	case D3DFMT_A2W10V10U10          :
	case D3DFMT_D32                  :
    case D3DFMT_D24S8                :
    case D3DFMT_D24X8                :
    case D3DFMT_D24X4S4              :
    case D3DFMT_D32F_LOCKABLE        :
    case D3DFMT_D24FS8               :
	case D3DFMT_D32_LOCKABLE         :
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	case D3DFMT_INDEX32              :
	case D3DFMT_G16R16F              :
	case D3DFMT_R32F                 :
	case D3DFMT_A2B10G10R10_XR_BIAS  :
	case D3DFMT_R5G6B5               :
	case D3DFMT_X1R5G5B5             :
	case D3DFMT_A1R5G5B5             :
	case D3DFMT_A4R4G4B4             :
	case D3DFMT_A8R3G3B2             :
	case D3DFMT_X4R4G4B4             :
	case D3DFMT_A8P8                 :
	case D3DFMT_A8L8                 :
	case D3DFMT_V8U8                 :
	case D3DFMT_L6V5U5               :
	case D3DFMT_D16_LOCKABLE         :
    case D3DFMT_D15S1                :
    case D3DFMT_D16                  :
	case D3DFMT_L16                  :
    case D3DFMT_INDEX16              :
	case D3DFMT_R16F                 :
	case D3DFMT_CxV8U8               :
	case D3DFMT_R3G3B2               :
	case D3DFMT_A8                   :
	case D3DFMT_P8                   :
	case D3DFMT_L8                   :
	case D3DFMT_A4L4                 :
	case D3DFMT_S8_LOCKABLE          :
	case D3DFMT_A16B16G16R16         :
	case D3DFMT_Q16W16V16U16         :
	case D3DFMT_A16B16G16R16F        :
	case D3DFMT_G32R32F              :
	case D3DFMT_A32B32G32R32F        :
    case D3DFMT_UYVY                 :
    case D3DFMT_R8G8_B8G8            :
    case D3DFMT_YUY2                 :
    case D3DFMT_G8R8_G8B8            :
	case D3DFMT_A1                   :
	case D3DFMT_VERTEXDATA           :
	case D3DFMT_MULTI2_ARGB8         :
	case D3DFMT_BINARYBUFFER         :
	default:
		return GetSurfacePitchBytes(Width, Format) * Height;
    case D3DFMT_DXT1                 :
    case D3DFMT_DXT2                 :
    case D3DFMT_DXT3                 :
    case D3DFMT_DXT4                 :
    case D3DFMT_DXT5                 :
		return GetSurfacePitchBytes(Width, Format) * RoundUpTo4(Height);
	}
}

static inline void AllocSurfaceBytes(const unsigned surfaceBytesSize, BYTE*& outAllocBytes, unsigned& outSurfaceBytesSize)
{
	void* voidBytes = NULL;
#ifdef SURFACE_ALLOC_PAGE_NOACCESS
	voidBytes = PageAllocWithNoAccessPage(surfaceBytesSize);
#else // #ifdef SURFACE_ALLOC_PAGE_NOACCESS
	voidBytes = malloc(surfaceBytesSize);
#endif // #ifdef SURFACE_ALLOC_PAGE_NOACCESS

	if (!voidBytes)
	{
		__debugbreak(); // Can't alloc our new surface!
	}

	outAllocBytes = (BYTE* const)voidBytes;
	outSurfaceBytesSize = surfaceBytesSize;
}

void IDirect3DSurface9Hook::CreateOffscreenPlainSurface(UINT _Width, UINT _Height, D3DFORMAT _Format, D3DPOOL _Pool)
{
	creationMethod = createOffscreenPlain;
	InternalWidth = _Width;
	InternalHeight = _Height;
	InternalFormat = _Format;
	InternalPool = _Pool;
	LockableRT = TRUE;
	DiscardRT = FALSE;
	is1x1surface = (InternalWidth == 1 && InternalHeight == 1);
	InternalWidthM1 = InternalWidth - 1;
	InternalHeightM1 = InternalHeight - 1;
	InternalWidthM1F = (const float)InternalWidthM1;
	InternalHeightM1F = (const float)InternalHeightM1;

	AllocSurfaceBytes(GetSurfaceSizeBytes(InternalWidth, InternalHeight, InternalFormat)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
		, surfaceBytesRaw, surfaceBytesRawSize);

	if (IsCompressedFormat(InternalFormat) )
	{
		AllocSurfaceBytes(GetSurfaceSizeBytes( (InternalWidth + 3) & ~3, (InternalHeight + 3) & ~3, D3DFMT_A8R8G8B8)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
			, auxSurfaceBytesRaw, auxSurfaceBytesRawSize);
	}

#ifdef SURFACE_MAGIC_COOKIE
	DWORD* const magicDword = (DWORD* const)surfaceBytesRaw + (surfaceBytes.size() / sizeof(DWORD) - 1);
	*magicDword = surfaceMagicBytes;
#endif // SURFACE_MAGIC_COOKIE

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

void IDirect3DSurface9Hook::CreateDepthStencilSurface(UINT _Width, UINT _Height, D3DFORMAT _Format, D3DMULTISAMPLE_TYPE _MultiSample, DWORD _MultisampleQuality, BOOL _Discard)
{
	creationMethod = createDepthStencil;
	InternalWidth = _Width;
	InternalHeight = _Height;
	InternalFormat = _Format;
	DiscardRT = _Discard;
	LockableRT = FALSE;
	InternalPool = D3DPOOL_DEFAULT;
	InternalUsage = (const DebuggableUsage)(D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL);
	is1x1surface = (InternalWidth == 1 && InternalHeight == 1);
	InternalWidthM1 = InternalWidth - 1;
	InternalHeightM1 = InternalHeight - 1;
	InternalWidthM1F = (const float)InternalWidthM1;
	InternalHeightM1F = (const float)InternalHeightM1;

	AllocSurfaceBytes(GetSurfaceSizeBytes(InternalWidth, InternalHeight, InternalFormat)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
		, surfaceBytesRaw, surfaceBytesRawSize);

	if (HasStencil(InternalFormat) )
	{
		AllocSurfaceBytes(GetSurfaceSizeBytes(InternalWidth, InternalHeight, D3DFMT_S8_LOCKABLE)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
			, auxSurfaceBytesRaw, auxSurfaceBytesRawSize);
	}

#ifdef SURFACE_MAGIC_COOKIE
	DWORD* const magicDword = (DWORD* const)surfaceBytesRaw + (surfaceBytes.size() / sizeof(DWORD) - 1);
	*magicDword = surfaceMagicBytes;
#endif // SURFACE_MAGIC_COOKIE

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

void IDirect3DSurface9Hook::CreateRenderTarget(UINT _Width, UINT _Height, D3DFORMAT _Format, D3DMULTISAMPLE_TYPE _MultiSample, DWORD _MultisampleQuality, BOOL _Lockable)
{
	creationMethod = createRenderTarget;
	InternalWidth = _Width;
	InternalHeight = _Height;
	InternalFormat = _Format;
	LockableRT = _Lockable;
	DiscardRT = FALSE;
	InternalPool = D3DPOOL_DEFAULT;
	InternalUsage = (const DebuggableUsage)(D3DUSAGE_RENDERTARGET);
	is1x1surface = (InternalWidth == 1 && InternalHeight == 1);
	InternalWidthM1 = InternalWidth - 1;
	InternalHeightM1 = InternalHeight - 1;
	InternalWidthM1F = (const float)InternalWidthM1;
	InternalHeightM1F = (const float)InternalHeightM1;

	AllocSurfaceBytes(GetSurfaceSizeBytes(InternalWidth, InternalHeight, InternalFormat)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
		, surfaceBytesRaw, surfaceBytesRawSize);

	if (IsCompressedFormat(InternalFormat) )
	{
		AllocSurfaceBytes(GetSurfaceSizeBytes( (InternalWidth + 3) & ~3, (InternalHeight + 3) & ~3, D3DFMT_A8R8G8B8)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
			, auxSurfaceBytesRaw, auxSurfaceBytesRawSize);
	}

#ifdef SURFACE_MAGIC_COOKIE
	DWORD* const magicDword = (DWORD* const)surfaceBytesRaw + (surfaceBytes.size() / sizeof(DWORD) - 1);
	*magicDword = surfaceMagicBytes;
#endif // SURFACE_MAGIC_COOKIE

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

void IDirect3DSurface9Hook::CreateTextureImplicitSurface(UINT _Width, UINT _Height, D3DFORMAT _Format, D3DPOOL _Pool, const DebuggableUsage _Usage, UINT _Level, IDirect3DTexture9Hook* _HookParentTexturePtr)
{
	creationMethod = createTexture;
	InternalWidth = _Width;
	InternalHeight = _Height;
	InternalFormat = _Format;
	InternalPool = _Pool;
	InternalUsage = _Usage;
	TextureSurfaceLevel = _Level;
	HookParentTexturePtr = _HookParentTexturePtr;
	LockableRT = TRUE;
	if (InternalPool == D3DPOOL_DEFAULT && !(InternalUsage & D3DUSAGE_DYNAMIC) ) // Default, non-dynamic texture surfaces are non-lockable!
	{
		LockableRT = FALSE;
	}
	DiscardRT = FALSE;
	is1x1surface = (InternalWidth == 1 && InternalHeight == 1);
	InternalWidthM1 = InternalWidth - 1;
	InternalHeightM1 = InternalHeight - 1;
	InternalWidthM1F = (const float)InternalWidthM1;
	InternalHeightM1F = (const float)InternalHeightM1;

	AllocSurfaceBytes(GetSurfaceSizeBytes(InternalWidth, InternalHeight, InternalFormat)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
		, surfaceBytesRaw, surfaceBytesRawSize);

	if (IsCompressedFormat(InternalFormat) )
	{
		AllocSurfaceBytes(GetSurfaceSizeBytes( (InternalWidth + 3) & ~3, (InternalHeight + 3) & ~3, D3DFMT_A8R8G8B8)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
			, auxSurfaceBytesRaw, auxSurfaceBytesRawSize);
	}

#ifdef SURFACE_MAGIC_COOKIE
	DWORD* const magicDword = (DWORD* const)surfaceBytesRaw + (surfaceBytes.size() / sizeof(DWORD) - 1);
	*magicDword = surfaceMagicBytes;
#endif // SURFACE_MAGIC_COOKIE

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

void IDirect3DSurface9Hook::CreateDeviceImplicitSurface(const D3DPRESENT_PARAMETERS& d3dpp)
{
	creationMethod = deviceImplicitBackbuffer;
	InternalWidth = d3dpp.BackBufferWidth;
	InternalHeight = d3dpp.BackBufferHeight;
	InternalFormat = d3dpp.BackBufferFormat;
	InternalPool = D3DPOOL_DEFAULT;
	InternalUsage = (const DebuggableUsage)(D3DUSAGE_RENDERTARGET); // The implicit backbuffer is a rendertarget
	is1x1surface = (InternalWidth == 1 && InternalHeight == 1);
	InternalWidthM1 = InternalWidth - 1;
	InternalHeightM1 = InternalHeight - 1;
	InternalWidthM1F = (const float)InternalWidthM1;
	InternalHeightM1F = (const float)InternalHeightM1;

	if (d3dpp.Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
		LockableRT = TRUE;
	else
		LockableRT = FALSE;

	DiscardRT = FALSE;

	AllocSurfaceBytes(GetSurfaceSizeBytes(InternalWidth, InternalHeight, InternalFormat)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
		, surfaceBytesRaw, surfaceBytesRawSize);

	if (IsCompressedFormat(InternalFormat) )
	{
		AllocSurfaceBytes(GetSurfaceSizeBytes( (InternalWidth + 3) & ~3, (InternalHeight + 3) & ~3, D3DFMT_A8R8G8B8)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
			, auxSurfaceBytesRaw, auxSurfaceBytesRawSize);
	}

#ifdef SURFACE_MAGIC_COOKIE
	DWORD* const magicDword = (DWORD* const)surfaceBytesRaw + (surfaceBytes.size() / sizeof(DWORD) - 1);
	*magicDword = surfaceMagicBytes;
#endif // SURFACE_MAGIC_COOKIE

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

void IDirect3DSurface9Hook::CreateDeviceImplicitDepthStencil(const D3DPRESENT_PARAMETERS& d3dpp)
{
	creationMethod = deviceImplicitDepthStencil;
	InternalWidth = d3dpp.BackBufferWidth;
	InternalHeight = d3dpp.BackBufferHeight;
	InternalFormat = d3dpp.AutoDepthStencilFormat;
	InternalPool = D3DPOOL_DEFAULT;
	InternalUsage = (const DebuggableUsage)(D3DUSAGE_DEPTHSTENCIL); // Important not to mark this with the D3DUSAGE_RENDERTARGET Usage because that will get a Usage-mismatch with real D3D9!
	LockableRT = FALSE;
	is1x1surface = (InternalWidth == 1 && InternalHeight == 1);
	InternalWidthM1 = InternalWidth - 1;
	InternalHeightM1 = InternalHeight - 1;
	InternalWidthM1F = (const float)InternalWidthM1;
	InternalHeightM1F = (const float)InternalHeightM1;

	if (d3dpp.Flags & D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL)
		DiscardRT = TRUE;
	else
		DiscardRT = FALSE;

	AllocSurfaceBytes(GetSurfaceSizeBytes(InternalWidth, InternalHeight, InternalFormat)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
		, surfaceBytesRaw, surfaceBytesRawSize);

	if (HasStencil(InternalFormat) )
	{
		AllocSurfaceBytes(GetSurfaceSizeBytes(InternalWidth, InternalHeight, D3DFMT_S8_LOCKABLE)
#ifdef SURFACE_MAGIC_COOKIE
		+ sizeof(DWORD)
#endif
			, auxSurfaceBytesRaw, auxSurfaceBytesRawSize);
	}

#ifdef SURFACE_MAGIC_COOKIE
	DWORD* const magicDword = (DWORD* const)surfaceBytesRaw + (surfaceBytes.size() / sizeof(DWORD) - 1);
	*magicDword = surfaceMagicBytes;
#endif // SURFACE_MAGIC_COOKIE

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

// Fills a rect on this surface with a color (.rgba). A NULL rectangle means to fill the entire surface.
void IDirect3DSurface9Hook::InternalColor4Fill(const D3DXVECTOR4& color, const D3DRECT* const pRect/* = NULL*/)
{
	if (!pRect)
	{
		switch (InternalFormat)
		{
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
		{
			const D3DCOLOR ldrColor = Float4ToD3DCOLORClamp(color);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(D3DCOLOR) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			D3DCOLOR* const originPtr = (D3DCOLOR* const)surfaceBytesRaw;
			const unsigned numPixels = InternalWidth * InternalHeight;
			for (unsigned x = 0; x < numPixels; ++x)
				originPtr[x] = ldrColor;
		}
			break;
		case D3DFMT_A16B16G16R16:
		{
			A16B16G16R16 writeColor;
			Float4ToA16B16G16R16(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(A16B16G16R16) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			A16B16G16R16* const originPtr = (A16B16G16R16* const)surfaceBytesRaw;
			const unsigned numPixels = InternalWidth * InternalHeight;
			for (unsigned x = 0; x < numPixels; ++x)
				originPtr[x] = writeColor;
		}
			break;
		case D3DFMT_A16B16G16R16F:
		{
			A16B16G16R16F writeColor;
			Float4ToA16B16G16R16F(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(A16B16G16R16F) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			A16B16G16R16F* const originPtr = (A16B16G16R16F* const)surfaceBytesRaw;
			const unsigned numPixels = InternalWidth * InternalHeight;
			for (unsigned x = 0; x < numPixels; ++x)
				originPtr[x] = writeColor;
		}
			break;
		case D3DFMT_A32B32G32R32F:
		{
			A32B32G32R32F writeColor;
			Float4ToA32B32G32R32F(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(A32B32G32R32F) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			A32B32G32R32F* const originPtr = (A32B32G32R32F* const)surfaceBytesRaw;
			const unsigned numPixels = InternalWidth * InternalHeight;
			for (unsigned x = 0; x < numPixels; ++x)
				originPtr[x] = writeColor;
		}
			break;
		case D3DFMT_R16F:
		{
			D3DXFLOAT16 writeColor;
			Float4ToR16F(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(D3DXFLOAT16) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			D3DXFLOAT16* const originPtr = (D3DXFLOAT16* const)surfaceBytesRaw;
			const unsigned numPixels = InternalWidth * InternalHeight;
			for (unsigned x = 0; x < numPixels; ++x)
				originPtr[x] = writeColor;
		}
			break;
		case D3DFMT_L8:
		{
			unsigned char writeColor;
			Float4ToL8Clamp(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(unsigned char) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			unsigned char* const originPtr = (unsigned char* const)surfaceBytesRaw;
			const unsigned numPixels = InternalWidth * InternalHeight;
			for (unsigned x = 0; x < numPixels; ++x)
				originPtr[x] = writeColor;
		}
			break;
		default: // TODO: Add more formats. This function is used in the D3D9 Clear() call.
			// Not yet supported!
			__debugbreak();
			break;
		}
	}
	else
	{
		switch (InternalFormat)
		{
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
		{
			const D3DCOLOR ldrColor = Float4ToD3DCOLORClamp(color);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(D3DCOLOR) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			D3DCOLOR* const originPtr = (D3DCOLOR* const)surfaceBytesRaw + pRect->x1;
			const unsigned rowFillSizePixels = (pRect->x2 - pRect->x1);
			const unsigned rowFillSizeBytes = sizeof(D3DCOLOR) * rowFillSizePixels;
			for (int row = pRect->y1; row < pRect->y2; ++row)
			{
				D3DCOLOR* const topLeftPtr = originPtr + (row * InternalWidth);
				for (unsigned column = 0; column < rowFillSizePixels; ++column)
					topLeftPtr[column] = ldrColor;

				// Memset sucks, it only splats one byte, not the whole DWORD!
				//memset(topLeftPtr, ldrColor, rowFillSizeBytes);
			}
		}
			break;
		case D3DFMT_A16B16G16R16:
		{
			A16B16G16R16 writeColor;
			Float4ToA16B16G16R16(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(A16B16G16R16) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			A16B16G16R16* const originPtr = (A16B16G16R16* const)surfaceBytesRaw + pRect->x1;
			const unsigned rowFillSizePixels = (pRect->x2 - pRect->x1);
			const unsigned rowFillSizeBytes = sizeof(A16B16G16R16) * rowFillSizePixels;
			for (int row = pRect->y1; row < pRect->y2; ++row)
			{
				A16B16G16R16* const topLeftPtr = originPtr + (row * InternalWidth);
				for (unsigned column = 0; column < rowFillSizePixels; ++column)
					topLeftPtr[column] = writeColor;
			}
		}
			break;
		case D3DFMT_A16B16G16R16F:
		{
			A16B16G16R16F writeColor;
			Float4ToA16B16G16R16F(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(A16B16G16R16F) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			A16B16G16R16F* const originPtr = (A16B16G16R16F* const)surfaceBytesRaw + pRect->x1;
			const unsigned rowFillSizePixels = (pRect->x2 - pRect->x1);
			const unsigned rowFillSizeBytes = sizeof(A16B16G16R16F) * rowFillSizePixels;
			for (int row = pRect->y1; row < pRect->y2; ++row)
			{
				A16B16G16R16F* const topLeftPtr = originPtr + (row * InternalWidth);
				for (unsigned column = 0; column < rowFillSizePixels; ++column)
					topLeftPtr[column] = writeColor;
			}
		}
			break;
		case D3DFMT_A32B32G32R32F:
		{
			A32B32G32R32F writeColor;
			Float4ToA32B32G32R32F(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(A32B32G32R32F) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			A32B32G32R32F* const originPtr = (A32B32G32R32F* const)surfaceBytesRaw + pRect->x1;
			const unsigned rowFillSizePixels = (pRect->x2 - pRect->x1);
			const unsigned rowFillSizeBytes = sizeof(A32B32G32R32F) * rowFillSizePixels;
			for (int row = pRect->y1; row < pRect->y2; ++row)
			{
				A32B32G32R32F* const topLeftPtr = originPtr + (row * InternalWidth);
				for (unsigned column = 0; column < rowFillSizePixels; ++column)
					topLeftPtr[column] = writeColor;
			}
		}
			break;
		case D3DFMT_R16F:
		{
			D3DXFLOAT16 writeColor;
			Float4ToR16F(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(D3DXFLOAT16) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			D3DXFLOAT16* const originPtr = (D3DXFLOAT16* const)surfaceBytesRaw + pRect->x1;
			const unsigned rowFillSizePixels = (pRect->x2 - pRect->x1);
			const unsigned rowFillSizeBytes = sizeof(D3DXFLOAT16) * rowFillSizePixels;
			for (int row = pRect->y1; row < pRect->y2; ++row)
			{
				D3DXFLOAT16* const topLeftPtr = originPtr + (row * InternalWidth);
				for (unsigned column = 0; column < rowFillSizePixels; ++column)
					topLeftPtr[column] = writeColor;
			}
		}
			break;
		case D3DFMT_L8:
		{
			unsigned char writeColor;
			Float4ToL8Clamp(color, writeColor);
#ifdef _DEBUG
			if (surfaceBytesRawSize != sizeof(unsigned char) * InternalWidth * InternalHeight
#ifdef SURFACE_MAGIC_COOKIE
				+ sizeof(DWORD)
#endif
				)
			{
				__debugbreak();
			}
#endif
			unsigned char* const originPtr = (unsigned char* const)surfaceBytesRaw + pRect->x1;
			const unsigned rowFillSizePixels = (pRect->x2 - pRect->x1);
			const unsigned rowFillSizeBytes = sizeof(unsigned char) * rowFillSizePixels;
			for (int row = pRect->y1; row < pRect->y2; ++row)
			{
				unsigned char* const topLeftPtr = originPtr + (row * InternalWidth);
				for (unsigned column = 0; column < rowFillSizePixels; ++column)
					topLeftPtr[column] = writeColor;
			}
		}
			break;
		default: // TODO: Add more formats. This function is used in the D3D9 Clear() call.
			// Not yet supported!
			__debugbreak();
			break;
		}
	}

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

// Fills a rect on this surface with a color (.rgba). A NULL rectangle means to fill the entire surface.
void IDirect3DSurface9Hook::InternalColorFill(const D3DCOLOR color, const D3DRECT* const pRect/* = NULL*/)
{
	D3DXVECTOR4 colorFloat4;
	ColorDWORDToFloat4(color, colorFloat4);

	if (pRect)
	{
		const LONG WidthM1 = InternalWidthM1;
		const LONG HeightM1 = InternalHeightM1;

		D3DRECT clippedRect;
		if (pRect->x1 < 0)
			clippedRect.x1 = 0;
		else if (pRect->x1 > WidthM1)
			clippedRect.x1 = WidthM1;
		else
			clippedRect.x1 = pRect->x1;
		if (pRect->x2 < 0)
			clippedRect.x2 = 0;
		else if (pRect->x2 > WidthM1)
			clippedRect.x2 = WidthM1;
		else
			clippedRect.x2 = pRect->x2;
		if (pRect->y1 < 0)
			clippedRect.y1 = 0;
		else if (pRect->y1 > HeightM1)
			clippedRect.y1 = HeightM1;
		else
			clippedRect.y1 = pRect->y1;
		if (pRect->y2 < 0)
			clippedRect.y2 = 0;
		else if (pRect->y2 > HeightM1)
			clippedRect.y2 = HeightM1;
		else
			clippedRect.y2 = pRect->y2;
		if (clippedRect.x2 < clippedRect.x1)
			clippedRect.x2 = clippedRect.x1;
		if (clippedRect.y2 < clippedRect.y1)
			clippedRect.y2 = clippedRect.y1;

		InternalColor4Fill(colorFloat4, &clippedRect);
	}
	else
	{
		InternalColor4Fill(colorFloat4, NULL);
	}

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

// Fills a rect on this surface with a depth value. A NULL rectangle means to fill the entire surface.
void IDirect3DSurface9Hook::InternalDepthFill(const float depth, const D3DRECT* const pRect/* = NULL*/)
{
	if (pRect)
	{
		if (pRect->x1 != 0 || pRect->y1 != 0 || pRect->x2 != InternalWidth || pRect->y2 != InternalHeight)
		{
			// TODO: Fill this in. This function is used in the D3D9 Clear() call.
			__debugbreak();
			return;
		}
	}

	switch (InternalFormat)
	{
	case D3DFMT_D15S1:
	{
		unsigned short* const pixels = (unsigned short* const)surfaceBytesRaw;
		const unsigned short formatPixel = (const unsigned short)(0x7FFF * depth);
		const unsigned numPixels = InternalWidth * InternalHeight;
		for (unsigned x = 0; x < numPixels; ++x)
			pixels[x] = formatPixel;
	}
	break;
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
	{
		unsigned short* const pixels = (unsigned short* const)surfaceBytesRaw;
		const unsigned short formatPixel = (const unsigned short)(0xFFFF * depth);
		const unsigned numPixels = InternalWidth * InternalHeight;
		for (unsigned x = 0; x < numPixels; ++x)
			pixels[x] = formatPixel;
	}
		break;
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
	{
		unsigned* const pixels = (unsigned* const)surfaceBytesRaw;
		const unsigned formatPixel = (const unsigned)(0x00FFFFFF * depth);
		const unsigned numPixels = InternalWidth * InternalHeight;
		for (unsigned x = 0; x < numPixels; ++x)
			pixels[x] = formatPixel;
	}
		break;
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	{
		unsigned* const pixels = (unsigned * const)surfaceBytesRaw;
		const unsigned formatPixel = (const unsigned)(0xFFFFFFFF * depth);
		const unsigned numPixels = InternalWidth * InternalHeight;
		for (unsigned x = 0; x < numPixels; ++x)
			pixels[x] = formatPixel;
	}
		break;
	case D3DFMT_D32F_LOCKABLE:
	{
		float* const pixels = (float* const)surfaceBytesRaw;
		const unsigned numPixels = InternalWidth * InternalHeight;
		for (unsigned x = 0; x < numPixels; ++x)
			pixels[x] = depth;
	}
		break;
	default:
#ifdef _DEBUG
		__debugbreak(); // Error: Can't call SetDepth() on non-depth formats!
#endif
	break;
	}

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

// Fills a rect on this surface with a stencil value. A NULL rectangle means to fill the entire surface.
void IDirect3DSurface9Hook::InternalStencilFill(const DWORD stencil, const D3DRECT* const pRect/* = NULL*/)
{
	if (!HasStencil(InternalFormat) )
		return;

	const DWORD stencilFormatMask = GetStencilFormatMask(InternalFormat);
	const DWORD maskedStencil = stencilFormatMask & stencil;
	const BYTE maskedStencilByte = (const BYTE)maskedStencil;

	if (!pRect)
	{
		BYTE* const stencilBuffer = auxSurfaceBytesRaw;
		memset(stencilBuffer, maskedStencilByte, InternalWidth * InternalHeight * sizeof(BYTE) );
	}
	else
	{
		BYTE* const originPtr = auxSurfaceBytesRaw + pRect->x1;
		const unsigned rowFillSizePixels = (pRect->x2 - pRect->x1);
		for (int row = pRect->y1; row < pRect->y2; ++row)
		{
			BYTE* const topLeftPtr = originPtr + (row * InternalWidth);
			for (unsigned column = 0; column < rowFillSizePixels; ++column)
				topLeftPtr[column] = maskedStencilByte;
		}
	}

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

void IDirect3DSurface9Hook::SetPixel(const unsigned x, const unsigned y, const D3DCOLOR color)
{
	if (InternalFormat == D3DFMT_X8R8G8B8 || InternalFormat == D3DFMT_A8R8G8B8)
	{
		D3DCOLOR* const pixels = (D3DCOLOR* const)surfaceBytesRaw;
		D3DCOLOR& outPixel = pixels[y * InternalWidth + x];
#ifdef WITH_SURFACE_HASHING
		RecomputePartialSurfaceHash(outPixel, color);
#endif
		outPixel = color;
	}
	else
	{
		// TODO: Add support for more surface types!
		__debugbreak();
	}
}

const D3DCOLOR IDirect3DSurface9Hook::GetPixel(const unsigned x, const unsigned y) const
{
#ifdef _DEBUG
	if (InternalFormat == D3DFMT_X8R8G8B8 || InternalFormat == D3DFMT_A8R8G8B8)
	{
#endif
		const D3DCOLOR* const pixels = (const D3DCOLOR* const)surfaceBytesRaw;
		return pixels[y * InternalWidth + x];
#ifdef _DEBUG
	}
	else
	{
		__debugbreak();
	}
	return 0x00000000;
#endif
}

template void IDirect3DSurface9Hook::GetPixelVec<0, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<1, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<2, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<3, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<4, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<5, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<6, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<7, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<8, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<9, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<10, false>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<11, false>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<12, false>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<13, false>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<14, false>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<15, false>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<0, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<1, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<2, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<3, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<4, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<5, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<6, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<7, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<8, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<9, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<10, true>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<11, true>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<12, true>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<13, true>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<14, true>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<15, true>(const unsigned x, const unsigned y,D3DXVECTOR4& outColor) const;

#ifdef USE_CHEAP_GAMMA_APPROXIMATION

static const float gamma2_2 = 2.2f;
template <const unsigned char writeMask>
static inline void GammaCorrectSample(D3DXVECTOR4& outColor)
{
	if (writeMask & 0x1)
		outColor.x = powf(outColor.x, gamma2_2);
	if (writeMask & 0x2)
		outColor.y = powf(outColor.y, gamma2_2);
	if (writeMask & 0x4)
		outColor.z = powf(outColor.z, gamma2_2);
}

template <const unsigned char writeMask>
static inline void GammaCorrectSample4(D3DXVECTOR4 (&outColor4)[4])
{
	if (writeMask & 0x1)
	{
		outColor4[0].x = powf(outColor4[0].x, gamma2_2);
		outColor4[1].x = powf(outColor4[1].x, gamma2_2);
		outColor4[2].x = powf(outColor4[2].x, gamma2_2);
		outColor4[3].x = powf(outColor4[3].x, gamma2_2);
	}
	if (writeMask & 0x2)
	{
		outColor4[0].y = powf(outColor4[0].y, gamma2_2);
		outColor4[1].y = powf(outColor4[1].y, gamma2_2);
		outColor4[2].y = powf(outColor4[2].y, gamma2_2);
		outColor4[3].y = powf(outColor4[3].y, gamma2_2);
	}
	if (writeMask & 0x4)
	{
		outColor4[0].z = powf(outColor4[0].z, gamma2_2);
		outColor4[1].z = powf(outColor4[1].z, gamma2_2);
		outColor4[2].z = powf(outColor4[2].z, gamma2_2);
		outColor4[3].z = powf(outColor4[3].z, gamma2_2);
	}
}

#else

static const float gamma2_4 = 2.4f;
static const float inv1055 = 1.0f / 1.055f;
static const float inv1292 = 1.0f / 12.92f;
template <const unsigned char writeMask>
static inline void GammaCorrectSample(D3DXVECTOR4& outColor)
{
	if (writeMask & 0x1)
	{
		if (outColor.x > 0.04045)
			outColor.x = powf( (outColor.x + 0.055f) * inv1055, gamma2_4);
		else
			outColor.x = outColor.x * inv1292;
	}
	if (writeMask & 0x2)
	{
		if (outColor.y > 0.04045)
			outColor.y = powf( (outColor.y + 0.055f) * inv1055, gamma2_4);
		else
			outColor.y = outColor.y * inv1292;
	}
	if (writeMask & 0x4)
	{
		if (outColor.z > 0.04045)
			outColor.z = powf( (outColor.z + 0.055f) * inv1055, gamma2_4);
		else
			outColor.z = outColor.z * inv1292;
	}
}

template <const unsigned char writeMask>
static inline void GammaCorrectSample4(D3DXVECTOR4 (&outColor4)[4])
{
	if (writeMask & 0x1)
	{
		if (outColor4[0].x > 0.04045) outColor4[0].x = powf( (outColor4[0].x + 0.055f) * inv1055, gamma2_4);
		else outColor4[0].x = outColor4[0].x * inv1292;
		if (outColor4[1].x > 0.04045) outColor4[1].x = powf( (outColor4[1].x + 0.055f) * inv1055, gamma2_4);
		else outColor4[1].x = outColor4[1].x * inv1292;
		if (outColor4[2].x > 0.04045) outColor4[2].x = powf( (outColor4[2].x + 0.055f) * inv1055, gamma2_4);
		else outColor4[2].x = outColor4[2].x * inv1292;
		if (outColor4[3].x > 0.04045) outColor4[3].x = powf( (outColor4[3].x + 0.055f) * inv1055, gamma2_4);
		else outColor4[3].x = outColor4[3].x * inv1292;
	}
	if (writeMask & 0x2)
	{
		if (outColor4[0].y > 0.04045) outColor4[0].y = powf( (outColor4[0].y + 0.055f) * inv1055, gamma2_4);
		else outColor4[0].y = outColor4[0].y * inv1292;
		if (outColor4[1].y > 0.04045) outColor4[1].y = powf( (outColor4[1].y + 0.055f) * inv1055, gamma2_4);
		else outColor4[1].y = outColor4[1].y * inv1292;
		if (outColor4[2].y > 0.04045) outColor4[2].y = powf( (outColor4[2].y + 0.055f) * inv1055, gamma2_4);
		else outColor4[2].y = outColor4[2].y * inv1292;
		if (outColor4[3].y > 0.04045) outColor4[3].y = powf( (outColor4[3].y + 0.055f) * inv1055, gamma2_4);
		else outColor4[3].y = outColor4[3].y * inv1292;
	}
	if (writeMask & 0x4)
	{
		if (outColor4[0].z > 0.04045) outColor4[0].z = powf( (outColor4[0].z + 0.055f) * inv1055, gamma2_4);
		else outColor4[0].z = outColor4[0].z * inv1292;
		if (outColor4[1].z > 0.04045) outColor4[1].z = powf( (outColor4[1].z + 0.055f) * inv1055, gamma2_4);
		else outColor4[1].z = outColor4[1].z * inv1292;
		if (outColor4[2].z > 0.04045) outColor4[2].z = powf( (outColor4[2].z + 0.055f) * inv1055, gamma2_4);
		else outColor4[2].z = outColor4[2].z * inv1292;
		if (outColor4[3].z > 0.04045) outColor4[3].z = powf( (outColor4[3].z + 0.055f) * inv1055, gamma2_4);
		else outColor4[3].z = outColor4[3].z * inv1292;
	}
}

#endif

template <const unsigned char writeMask, const bool sRGBSurface>
void IDirect3DSurface9Hook::GetPixelVec(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const
{
	switch (InternalFormat)
	{
	default:
#ifdef _DEBUG
		__debugbreak(); // TODO: Add support for more surface types!
#else
		__assume(0);
#endif
	case D3DFMT_X8R8G8B8:
	{
		const D3DCOLOR ldrColor = GetPixel(x, y);
		ColorDWORDToFloat4<writeMask & 0x7>(ldrColor, outColor);
		outColor.w = 1.0f;
		if (sRGBSurface)
			GammaCorrectSample<writeMask & 0x7>(outColor);
	}
		break;
	case D3DFMT_A8R8G8B8:
	{
		const D3DCOLOR ldrColor = GetPixel(x, y);
		ColorDWORDToFloat4<writeMask>(ldrColor, outColor);
		if (sRGBSurface)
			GammaCorrectSample<writeMask & 0x7>(outColor);
	}
		break;
	case D3DFMT_A16B16G16R16:
	{
		const A16B16G16R16* const pixels = (const A16B16G16R16* const)surfaceBytesRaw;
		const A16B16G16R16& pixel = pixels[y * InternalWidth + x];
		ColorA16B16G16R16ToFloat4<writeMask>(pixel, outColor);
		if (sRGBSurface)
			GammaCorrectSample<writeMask & 0x7>(outColor);
	}
		break;
	case D3DFMT_A16B16G16R16F:
	{
		const A16B16G16R16F* const pixels = (const A16B16G16R16F* const)surfaceBytesRaw;
		const A16B16G16R16F& pixel = pixels[y * InternalWidth + x];
		ColorA16B16G16R16FToFloat4<writeMask>(pixel, outColor);
	}
	break;
	case D3DFMT_A32B32G32R32F:
	{
		const A32B32G32R32F* const pixels = (const A32B32G32R32F* const)surfaceBytesRaw;
		const A32B32G32R32F& pixel = pixels[y * InternalWidth + x];
		ColorA32B32G32R32FToFloat4<writeMask>(pixel, outColor);
	}
		break;
	case D3DFMT_R16F:
	{
		const D3DXFLOAT16* const pixels = (const D3DXFLOAT16* const)surfaceBytesRaw;
		const D3DXFLOAT16& pixel = pixels[y * InternalWidth + x];
		ColorR16FToFloat4<writeMask>(pixel, outColor);
	}
		break;
	case D3DFMT_L8:
	{
		const unsigned char* const pixels = (const unsigned char* const)surfaceBytesRaw;
		const unsigned char pixel = pixels[y * InternalWidth + x];
		L8ToFloat4<writeMask>(pixel, outColor);
		if (sRGBSurface)
			GammaCorrectSample<writeMask & 0x7>(outColor);
	}
	break;
	case D3DFMT_DXT1:
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
	{
		const D3DCOLOR* const rawPixels = (const D3DCOLOR* const)auxSurfaceBytesRaw;
		const D3DCOLOR ldrColor = rawPixels[y * InternalWidth + x];
		ColorDWORDToFloat4<writeMask>(ldrColor, outColor);
		if (sRGBSurface)
			GammaCorrectSample<writeMask & 0x7>(outColor);
	}
		break;

	}
}

template <const unsigned char writeMask, const bool sRGBSurface>
void IDirect3DSurface9Hook::GetPixelVec4(const unsigned (&x4)[4], const unsigned (&y4)[4], D3DXVECTOR4 (&outColor4)[4]) const
{
	switch (InternalFormat)
	{
	default:
#ifdef _DEBUG
		__debugbreak(); // TODO: Add support for more surface types!
#else
		__assume(0);
#endif
	case D3DFMT_X8R8G8B8:
	{
		const D3DCOLOR* const pixels = (const D3DCOLOR* const)surfaceBytesRaw;
		const D3DCOLOR ldrColor4[4] =
		{
			pixels[y4[0] * InternalWidth + x4[0] ],
			pixels[y4[1] * InternalWidth + x4[1] ],
			pixels[y4[2] * InternalWidth + x4[2] ],
			pixels[y4[3] * InternalWidth + x4[3] ]
		};
		ColorDWORDToFloat4_4<writeMask & 0x7>(ldrColor4, outColor4);
		outColor4[0].w = outColor4[1].w = outColor4[2].w = outColor4[3].w = 1.0f;
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7>(outColor4);
	}
		break;
	case D3DFMT_A8R8G8B8:
	{
		const D3DCOLOR* const pixels = (const D3DCOLOR* const)surfaceBytesRaw;
		const D3DCOLOR ldrColor4[4] =
		{
			pixels[y4[0] * InternalWidth + x4[0] ],
			pixels[y4[1] * InternalWidth + x4[1] ],
			pixels[y4[2] * InternalWidth + x4[2] ],
			pixels[y4[3] * InternalWidth + x4[3] ]
		};
		ColorDWORDToFloat4_4<writeMask>(ldrColor4, outColor4);
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7>(outColor4);
	}
		break;
	case D3DFMT_A16B16G16R16:
	{
		const A16B16G16R16* const pixels = (const A16B16G16R16* const)surfaceBytesRaw;
		const A16B16G16R16 pixel4[4] = 
		{
			pixels[y4[0] * InternalWidth + x4[0] ],
			pixels[y4[1] * InternalWidth + x4[1] ],
			pixels[y4[2] * InternalWidth + x4[2] ],
			pixels[y4[3] * InternalWidth + x4[3] ]
		};

		ColorA16B16G16R16ToFloat4_4<writeMask>(pixel4, outColor4);
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7>(outColor4);
	}
		break;
	case D3DFMT_A16B16G16R16F:
	{
		const A16B16G16R16F* const pixels = (const A16B16G16R16F* const)surfaceBytesRaw;
		const A16B16G16R16F* const pixel4[4] = 
		{
			&pixels[y4[0] * InternalWidth + x4[0] ],
			&pixels[y4[1] * InternalWidth + x4[1] ],
			&pixels[y4[2] * InternalWidth + x4[2] ],
			&pixels[y4[3] * InternalWidth + x4[3] ]
		};

		ColorA16B16G16R16FToFloat4_4<writeMask>(pixel4, outColor4);
	}
	break;
	case D3DFMT_A32B32G32R32F:
	{
		const A32B32G32R32F* const pixels = (const A32B32G32R32F* const)surfaceBytesRaw;
		const A32B32G32R32F* const pixel4[4] = 
		{
			&pixels[y4[0] * InternalWidth + x4[0] ],
			&pixels[y4[1] * InternalWidth + x4[1] ],
			&pixels[y4[2] * InternalWidth + x4[2] ],
			&pixels[y4[3] * InternalWidth + x4[3] ]
		};

		ColorA32B32G32R32FToFloat4_4<writeMask>(pixel4, outColor4);
	}
		break;
	case D3DFMT_R16F:
	{
		const D3DXFLOAT16* const pixels = (const D3DXFLOAT16* const)surfaceBytesRaw;
		const D3DXFLOAT16 pixel4[4] = 
		{
			pixels[y4[0] * InternalWidth + x4[0] ],
			pixels[y4[1] * InternalWidth + x4[1] ],
			pixels[y4[2] * InternalWidth + x4[2] ],
			pixels[y4[3] * InternalWidth + x4[3] ]
		};

		ColorR16FToFloat4_4<writeMask>(pixel4, outColor4);
	}
		break;
	case D3DFMT_L8:
	{
		const unsigned char* const pixels = (const unsigned char* const)surfaceBytesRaw;
		const unsigned char pixel4[4] = 
		{
			pixels[y4[0] * InternalWidth + x4[0] ],
			pixels[y4[1] * InternalWidth + x4[1] ],
			pixels[y4[2] * InternalWidth + x4[2] ],
			pixels[y4[3] * InternalWidth + x4[3] ]
		};

		L8ToFloat4_4<writeMask>(pixel4, outColor4);
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7>(outColor4);
	}
	break;
	case D3DFMT_DXT1:
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
	{
		const D3DCOLOR* const rawPixels = (const D3DCOLOR* const)auxSurfaceBytesRaw;
		const D3DCOLOR ldrColor4[4] = 
		{
			rawPixels[y4[0] * InternalWidth + x4[0] ],
			rawPixels[y4[1] * InternalWidth + x4[1] ],
			rawPixels[y4[2] * InternalWidth + x4[2] ],
			rawPixels[y4[3] * InternalWidth + x4[3] ]
		};
		ColorDWORDToFloat4_4<writeMask>(ldrColor4, outColor4);
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7>(outColor4);
	}
		break;

	}
}

const float IDirect3DSurface9Hook::GetDepth(const unsigned x, const unsigned y) const
{
	switch (InternalFormat)
	{
	case D3DFMT_D15S1:
	{
		const unsigned short* const pixels = (const unsigned short* const)surfaceBytesRaw;
		const unsigned short formatPixel = pixels[y * InternalWidth + x];
		return formatPixel / 32767.0f;
	}
		break;
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
	{
		const unsigned short* const pixels = (const unsigned short* const)surfaceBytesRaw;
		const unsigned short formatPixel = pixels[y * InternalWidth + x];
		return formatPixel / 65535.0f;
	}
		break;
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
	{
		const unsigned* const pixels = (const unsigned* const)surfaceBytesRaw;
		const unsigned formatPixel = pixels[y * InternalWidth + x];
		return formatPixel / 16777215.0f;
	}
		break;
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	{
		const unsigned* const pixels = (const unsigned* const)surfaceBytesRaw;
		const unsigned formatPixel = pixels[y * InternalWidth + x];
		return formatPixel / 4294967295.0f;
	}
		break;
	case D3DFMT_D32F_LOCKABLE:
	{
		const float* const pixels = (const float* const)surfaceBytesRaw;
		return pixels[y * InternalWidth + x];
	}
		break;
	default:
#ifdef _DEBUG
		__debugbreak(); // Error: Can't call GetDepth() on non-depth formats!
#endif
		break;
	}
	return 0.0f;
}

const unsigned IDirect3DSurface9Hook::GetRawDepthValueFromFloatDepth(const float floatDepth) const
{
	switch (InternalFormat)
	{
	case D3DFMT_D15S1:
		return (const unsigned)(floatDepth * 32767.0f);
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
		return (const unsigned)(floatDepth * 65535.0f);
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
		return (const unsigned)(floatDepth * 16777215.0f);
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
		return (const unsigned)(floatDepth * 4294967295.0f);
	case D3DFMT_D32F_LOCKABLE:
		return *(const unsigned* const)&floatDepth;
	default:
#ifdef _DEBUG
		__debugbreak(); // Error: Can't call GetDepth() on non-depth formats!
#endif
		return 0u;
	}
}

const unsigned IDirect3DSurface9Hook::GetRawDepth(const unsigned x, const unsigned y) const
{
	switch (InternalFormat)
	{
	case D3DFMT_D15S1:
	{
		const unsigned short* const pixels = (const unsigned short* const)surfaceBytesRaw;
		const unsigned short formatPixel = pixels[y * InternalWidth + x];
		return formatPixel;
	}
		break;
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
	{
		const unsigned short* const pixels = (const unsigned short* const)surfaceBytesRaw;
		const unsigned short formatPixel = pixels[y * InternalWidth + x];
		return formatPixel;
	}
		break;
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
	{
		const unsigned* const pixels = (const unsigned* const)surfaceBytesRaw;
		const unsigned formatPixel = pixels[y * InternalWidth + x];
		return formatPixel;
	}
		break;
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	{
		const unsigned* const pixels = (const unsigned* const)surfaceBytesRaw;
		const unsigned formatPixel = pixels[y * InternalWidth + x];
		return formatPixel;
	}
		break;
	case D3DFMT_D32F_LOCKABLE:
	{
		const float* const pixels = (const float* const)surfaceBytesRaw;
		const unsigned* const rawPixels = (const unsigned* const)pixels;
		return rawPixels[y * InternalWidth + x];
	}
		break;
	default:
#ifdef _DEBUG
		__debugbreak(); // Error: Can't call GetDepth() on non-depth formats!
#endif
		break;
	}
	return 0x00000000;
}

void IDirect3DSurface9Hook::SetDepth(const unsigned x, const unsigned y, const float depth)
{
#ifdef _DEBUG
	if (x >= InternalWidth)
	{
		__debugbreak(); // Out of bounds depth write
	}
	if (y >= InternalHeight)
	{
		__debugbreak(); // Out of bounds depth write
	}
#endif
	switch (InternalFormat)
	{
	case D3DFMT_D15S1:
	{
		unsigned short* const pixels = (unsigned short* const)surfaceBytesRaw;
		const unsigned short formatDepth = (const unsigned short)(0x7FFF * depth);
		pixels[y * InternalWidth + x] = formatDepth;
	}
		break;
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
	{
		unsigned short* const pixels = (unsigned short* const)surfaceBytesRaw;
		const unsigned short formatDepth = (const unsigned short)(0xFFFF * depth);
		pixels[y * InternalWidth + x] = formatDepth;
	}
		break;
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
	{
		unsigned* const pixels = (unsigned* const)surfaceBytesRaw;
		const unsigned formatDepth = (const unsigned)(0x00FFFFFF * depth);
		unsigned& outPixel = pixels[y * InternalWidth + x];
		outPixel = formatDepth;
	}
		break;
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	{
		unsigned* const pixels = (unsigned* const)surfaceBytesRaw;
		const unsigned uDepth = (const unsigned)(depth * 0xFFFFFFFF);
		unsigned& outPixel = pixels[y * InternalWidth + x];
		outPixel = uDepth;
	}
		break;
	case D3DFMT_D32F_LOCKABLE:
	{
		float* const pixels = (float* const)surfaceBytesRaw;
		float& outPixel = pixels[y * InternalWidth + x];
		outPixel = depth;
	}
		break;
	default:
#ifdef _DEBUG
		__debugbreak(); // Error: Can't call SetDepth() on non-depth formats!
#endif
		break;
	}
}

void IDirect3DSurface9Hook::SetStencil(const unsigned x, const unsigned y, const DWORD stencil)
{
#ifdef _DEBUG
	if (x >= InternalWidth)
	{
		__debugbreak(); // Out of bounds stencil write
	}
	if (y >= InternalHeight)
	{
		__debugbreak(); // Out of bounds stencil write
	}
#endif
	const DWORD stencilMask = GetStencilFormatMask(InternalFormat);
	const DWORD maskedStencil = stencil & stencilMask;
	const BYTE maskedStencilByte = (const BYTE)maskedStencil;

	BYTE* const stencilBuffer = auxSurfaceBytesRaw;
	stencilBuffer[y * InternalWidth + x] = maskedStencilByte;

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

const DWORD IDirect3DSurface9Hook::GetStencil(const unsigned x, const unsigned y) const
{
	const BYTE* const stencilBuffer = auxSurfaceBytesRaw;
	return stencilBuffer[y * InternalWidth + x];
}

// Don't define the <0> version of this function. Anybody calling it is a dummy because it doesn't write anything anyway.
// template void IDirect3DSurface9Hook::SetPixelVec<0>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<1>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<2>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<3>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<4>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<5>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<6>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<7>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<8>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<9>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<10>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<11>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<12>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<13>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<14>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);
template void IDirect3DSurface9Hook::SetPixelVec<15>(const unsigned x, const unsigned y, const D3DXVECTOR4& color);

template <const unsigned char writeMask>
void IDirect3DSurface9Hook::SetPixelVec(const unsigned x, const unsigned y, const D3DXVECTOR4& color)
{
#ifdef _DEBUG
	if (x >= InternalWidth)
	{
		__debugbreak(); // Out of bounds pixel write
	}
	if (y >= InternalHeight)
	{
		__debugbreak(); // Out of bounds pixel write
	}
#endif

	switch (InternalFormat)
	{
	default:
#ifdef _DEBUG
		__debugbreak(); // TODO: Add more surface formats
#else
		__assume(0);
#endif
	case D3DFMT_X8R8G8B8:
	{
		const D3DCOLOR ldrColor = Float4ToX8R8G8B8Clamp<writeMask & 0x7>(color);
		SetPixel(x, y, ldrColor);
	}
		break;
	case D3DFMT_A8R8G8B8:
	{
		const D3DCOLOR ldrColor = Float4ToD3DCOLORClamp<writeMask>(color);
		SetPixel(x, y, ldrColor);
	}
		break;
	case D3DFMT_A16B16G16R16:
	{
		A16B16G16R16* const pixels = (A16B16G16R16* const)surfaceBytesRaw;
		A16B16G16R16& writePixel = pixels[y * InternalWidth + x];
#ifdef WITH_SURFACE_HASHING
		RecomputePartialSurfaceHashXor(writePixel);
#endif
		Float4ToA16B16G16R16<writeMask>(color, writePixel);
#ifdef WITH_SURFACE_HASHING
		RecomputePartialSurfaceHashXor(writePixel);
#endif
	}
		break;
	case D3DFMT_A16B16G16R16F:
	{
		A16B16G16R16F* const pixels = (A16B16G16R16F* const)surfaceBytesRaw;
		A16B16G16R16F& writePixel = pixels[y * InternalWidth + x];
#ifdef WITH_SURFACE_HASHING
		RecomputePartialSurfaceHashXor(writePixel);
#endif
		Float4ToA16B16G16R16F<writeMask>(color, writePixel);
#ifdef WITH_SURFACE_HASHING
		RecomputePartialSurfaceHashXor(writePixel);
#endif
	}
		break;
	case D3DFMT_A32B32G32R32F:
	{
		A32B32G32R32F* const pixels = (A32B32G32R32F* const)surfaceBytesRaw;
		A32B32G32R32F& writePixel = pixels[y * InternalWidth + x];
#ifdef WITH_SURFACE_HASHING
		RecomputePartialSurfaceHashXor(writePixel);
#endif
		Float4ToA32B32G32R32F<writeMask>(color, writePixel);
#ifdef WITH_SURFACE_HASHING
		RecomputePartialSurfaceHashXor(writePixel);
#endif
	}
		break;
	case D3DFMT_R16F:
	{
		D3DXFLOAT16* const pixels = (D3DXFLOAT16* const)surfaceBytesRaw;
		D3DXFLOAT16& writePixel = pixels[y * InternalWidth + x];
		Float4ToR16F<writeMask>(color, writePixel);

		// Can't update the surface hash in this case...
	}
		break;
	case D3DFMT_L8:
	{
		unsigned char* const pixels = (unsigned char* const)surfaceBytesRaw;
		unsigned char& writePixel = pixels[y * InternalWidth + x];
		Float4ToL8Clamp<writeMask>(color, writePixel);

		// Can't update the surface hash in this case...
	}
		break;
	}
}

void IDirect3DSurface9Hook::UpdateSurfaceInternal(const IDirect3DSurface9Hook* const sourceSurface, const RECT* const sourceRect, const POINT* const destPoint)
{
#ifdef _DEBUG
		if (InternalFormat != sourceSurface->InternalFormat)
		{
			__debugbreak();
		}
#endif

	// Copy entire surface:
	if (!sourceRect || !destPoint)
	{
		D3DCOLOR* const pixels = (D3DCOLOR* const)surfaceBytesRaw;

#ifdef _DEBUG
		if (surfaceBytesRawSize != sourceSurface->surfaceBytesRawSize)
		{
			__debugbreak();
		}
		if (InternalWidth != sourceSurface->InternalWidth)
		{
			__debugbreak();
		}
		if (InternalHeight != sourceSurface->InternalHeight)
		{
			__debugbreak();
		}
#endif

		const D3DCOLOR* const sourcePixels = (const D3DCOLOR* const)sourceSurface->GetSurfaceBytesRaw();
		memcpy(pixels, sourcePixels, sourceSurface->GetSurfaceBytesSize() );

		if (auxSurfaceBytesRaw != NULL)
		{
			const BYTE* const auxSourceBytes = (const BYTE* const)sourceSurface->auxSurfaceBytesRaw;
			memcpy(auxSurfaceBytesRaw, auxSourceBytes, auxSurfaceBytesRawSize);
		}
	}
	else
	{
		D3DCOLOR* pixels = (D3DCOLOR* const)surfaceBytesRaw + destPoint->y * InternalWidth + destPoint->x;
		const D3DCOLOR* const sourcePixels = (const D3DCOLOR* const)sourceSurface->GetSurfaceBytesRaw();
		const unsigned rowWidth = sourceRect->right - sourceRect->left;
		for (int y = sourceRect->top; y < sourceRect->bottom; ++y)
		{
			const D3DCOLOR* const rowPixels = sourcePixels + y * sourceSurface->InternalWidth;
			for (int x = sourceRect->left; x < sourceRect->right; ++x)
			{
				const D3DCOLOR sourceColor = *(rowPixels + x);
				*pixels++ = sourceColor;
			}
			pixels -= rowWidth;
			pixels += InternalWidth;
		}

		if (auxSurfaceBytesRaw != NULL)
		{
			// TODO: Handle this case!
			__debugbreak();
		}
	}

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

template <const unsigned char writeMask, const bool sRGBSurface>
void IDirect3DSurface9Hook::SampleSurfaceInternal(const float x, const float y, const D3DTEXTUREFILTERTYPE texf, D3DXVECTOR4& outColor) const
{
	// Keep the simple case simple
	if (is1x1surface)
	{
		GetPixelVec<writeMask, sRGBSurface>(0, 0, outColor);
		return;
	}

#ifdef _DEBUG
	if (x > 1.0f || x < 0.0f)
		__debugbreak();
	if (y > 1.0f || y < 0.0f)
		__debugbreak();
#endif
	float u = x * InternalWidth;
	float v = y * InternalHeight;

	const unsigned WidthM1 = InternalWidthM1;
	const unsigned HeightM1 = InternalHeightM1;

	const float maxU = InternalWidthM1F;
	const float maxV = InternalHeightM1F;

	if (u > maxU)
		u = maxU;
	if (v > maxV)
		v = maxV;

	switch (texf)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case D3DTEXF_ANISOTROPIC    : // It is legal for anisotropic filtering to fall back to using linear filtering if the device doesn't support anisotropic
	case D3DTEXF_PYRAMIDALQUAD  :
	case D3DTEXF_GAUSSIANQUAD   :
	case D3DTEXF_CONVOLUTIONMONO:
	case D3DTEXF_LINEAR         :
	{
		const int cuTopleft = (const int)u;
		const int cvTopleft = (const int)v;
		unsigned cuTopright = cuTopleft + 1;
		if (cuTopright > WidthM1)
			cuTopright = WidthM1;
		const unsigned cvTopright = cvTopleft;
		const unsigned cuBotleft = cuTopleft;
		unsigned cvBotleft = cvTopleft + 1;
		if (cvBotleft > HeightM1)
			cvBotleft = HeightM1;
		const unsigned cuBotright = cuTopright;
		const unsigned cvBotright = cvBotleft;

		const unsigned cu4[4] =
		{
			(const unsigned)cuTopleft,
			cuTopright,
			cuBotleft,
			cuBotright
		};

		const unsigned cv4[4] =
		{
			(const unsigned)cvTopleft,
			cvTopright,
			cvBotleft,
			cvBotright
		};

		D3DXVECTOR4 bilinearSamples[4];
		GetPixelVec4<writeMask, sRGBSurface>(cu4, cv4, bilinearSamples);

		D3DXVECTOR4 topHorizLerp, botHorizLerp;
		const float xLerp = u - cuTopleft;
		lrp<writeMask>(topHorizLerp, bilinearSamples[0], bilinearSamples[1], xLerp);
		lrp<writeMask>(botHorizLerp, bilinearSamples[2], bilinearSamples[3], xLerp);
		const float yLerp = v - cvTopleft;
		lrp<writeMask>(outColor, topHorizLerp, botHorizLerp, yLerp);

#ifdef _DEBUG
		if (xLerp > 1.0f || xLerp < 0.0f)
			__debugbreak();
		if (yLerp > 1.0f || yLerp < 0.0f)
			__debugbreak();
#endif
	}
		break;
	case D3DTEXF_NONE           :
	case D3DTEXF_POINT          :
		GetPixelVec<writeMask, sRGBSurface>( (const int)u, (const int)v, outColor);
		break;
	}
}

template <const unsigned char writeMask, const bool sRGBSurface>
void IDirect3DSurface9Hook::SampleSurfaceInternal4(const float (&x4)[4], const float (&y4)[4], const D3DTEXTUREFILTERTYPE texf, D3DXVECTOR4 (&outColor4)[4]) const
{
	// Keep the simple case simple
	if (is1x1surface)
	{
		D3DXVECTOR4 broadcastPixel;
		GetPixelVec<writeMask, sRGBSurface>(0, 0, broadcastPixel);

		outColor4[0] = broadcastPixel;
		outColor4[1] = broadcastPixel;
		outColor4[2] = broadcastPixel;
		outColor4[3] = broadcastPixel;
		return;
	}

#ifdef _DEBUG
	if (x4[0] > 1.0f || x4[0] < 0.0f)
		__debugbreak();
	if (x4[1] > 1.0f || x4[1] < 0.0f)
		__debugbreak();
	if (x4[2] > 1.0f || x4[2] < 0.0f)
		__debugbreak();
	if (x4[3] > 1.0f || x4[3] < 0.0f)
		__debugbreak();
	if (y4[0] > 1.0f || y4[0] < 0.0f)
		__debugbreak();
	if (y4[1] > 1.0f || y4[1] < 0.0f)
		__debugbreak();
	if (y4[2] > 1.0f || y4[2] < 0.0f)
		__debugbreak();
	if (y4[3] > 1.0f || y4[3] < 0.0f)
		__debugbreak();
#endif
	float u4[4] =
	{
		x4[0] * InternalWidth,
		x4[1] * InternalWidth,
		x4[2] * InternalWidth,
		x4[3] * InternalWidth
	};

	float v4[4] =
	{
		y4[0] * InternalHeight,
		y4[1] * InternalHeight,
		y4[2] * InternalHeight,
		y4[3] * InternalHeight
	};

	const unsigned WidthM1 = InternalWidthM1;
	const unsigned HeightM1 = InternalHeightM1;

	const float maxU = InternalWidthM1F;
	const float maxV = InternalHeightM1F;

	if (u4[0] > maxU) u4[0] = maxU;
	if (u4[1] > maxU) u4[1] = maxU;
	if (u4[2] > maxU) u4[2] = maxU;
	if (u4[3] > maxU) u4[3] = maxU;

	if (v4[0] > maxV) v4[0] = maxV;
	if (v4[1] > maxV) v4[1] = maxV;
	if (v4[2] > maxV) v4[2] = maxV;
	if (v4[3] > maxV) v4[3] = maxV;

	switch (texf)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case D3DTEXF_ANISOTROPIC    : // It is legal for anisotropic filtering to fall back to using linear filtering if the device doesn't support anisotropic
	case D3DTEXF_PYRAMIDALQUAD  :
	case D3DTEXF_GAUSSIANQUAD   :
	case D3DTEXF_CONVOLUTIONMONO:
	case D3DTEXF_LINEAR         :
	{
		// D3DXVECTOR4 topleft, topright, botleft, botright;
		D3DXVECTOR4 topleft4[4];

		const unsigned cuTopleft4[4] =
		{
			(const unsigned)u4[0],
			(const unsigned)u4[1],
			(const unsigned)u4[2],
			(const unsigned)u4[3]
		};

		const unsigned cvTopleft4[4] =
		{
			(const unsigned)v4[0],
			(const unsigned)v4[1],
			(const unsigned)v4[2],
			(const unsigned)v4[3]
		};

		GetPixelVec4<writeMask, sRGBSurface>(cuTopleft4, cvTopleft4, topleft4);

		unsigned cuTopright4[4] =
		{
			(const unsigned)(cuTopleft4[0] + 1),
			(const unsigned)(cuTopleft4[1] + 1),
			(const unsigned)(cuTopleft4[2] + 1),
			(const unsigned)(cuTopleft4[3] + 1)
		};

		if (cuTopright4[0] > WidthM1) cuTopright4[0] = WidthM1;
		if (cuTopright4[1] > WidthM1) cuTopright4[1] = WidthM1;
		if (cuTopright4[2] > WidthM1) cuTopright4[2] = WidthM1;
		if (cuTopright4[3] > WidthM1) cuTopright4[3] = WidthM1;

		D3DXVECTOR4 topright4[4];
		GetPixelVec4<writeMask, sRGBSurface>(cuTopright4, cvTopleft4, topright4);

		unsigned cvBotleft4[4] =
		{
			(const unsigned)(cvTopleft4[0] + 1),
			(const unsigned)(cvTopleft4[1] + 1),
			(const unsigned)(cvTopleft4[2] + 1),
			(const unsigned)(cvTopleft4[3] + 1)
		};

		if (cvBotleft4[0] > HeightM1) cvBotleft4[0] = HeightM1;
		if (cvBotleft4[1] > HeightM1) cvBotleft4[1] = HeightM1;
		if (cvBotleft4[2] > HeightM1) cvBotleft4[2] = HeightM1;
		if (cvBotleft4[3] > HeightM1) cvBotleft4[3] = HeightM1;

		D3DXVECTOR4 botleft4[4];
		GetPixelVec4<writeMask, sRGBSurface>(cuTopleft4, cvBotleft4, botleft4);

		D3DXVECTOR4 botright4[4];
		GetPixelVec4<writeMask, sRGBSurface>(cuTopright4, cvBotleft4, botright4);

		D3DXVECTOR4 topHorizLerp4[4];
		const float xLerp4[4] =
		{
			u4[0] - cuTopleft4[0],
			u4[1] - cuTopleft4[1],
			u4[2] - cuTopleft4[2],
			u4[3] - cuTopleft4[3]
		};

		lrp<writeMask>(topHorizLerp4[0], topleft4[0], topright4[0], xLerp4[0]);
		lrp<writeMask>(topHorizLerp4[1], topleft4[1], topright4[1], xLerp4[1]);
		lrp<writeMask>(topHorizLerp4[2], topleft4[2], topright4[2], xLerp4[2]);
		lrp<writeMask>(topHorizLerp4[3], topleft4[3], topright4[3], xLerp4[3]);

		D3DXVECTOR4 botHorizLerp4[4];
		lrp<writeMask>(botHorizLerp4[0], botleft4[0], botright4[0], xLerp4[0]);
		lrp<writeMask>(botHorizLerp4[1], botleft4[1], botright4[1], xLerp4[1]);
		lrp<writeMask>(botHorizLerp4[2], botleft4[2], botright4[2], xLerp4[2]);
		lrp<writeMask>(botHorizLerp4[3], botleft4[3], botright4[3], xLerp4[3]);

		const float yLerp4[4] =
		{
			v4[0] - cvTopleft4[0],
			v4[1] - cvTopleft4[1],
			v4[2] - cvTopleft4[2],
			v4[3] - cvTopleft4[3]
		};
		lrp<writeMask>(outColor4[0], topHorizLerp4[0], botHorizLerp4[0], yLerp4[0]);
		lrp<writeMask>(outColor4[1], topHorizLerp4[1], botHorizLerp4[1], yLerp4[1]);
		lrp<writeMask>(outColor4[2], topHorizLerp4[2], botHorizLerp4[2], yLerp4[2]);
		lrp<writeMask>(outColor4[3], topHorizLerp4[3], botHorizLerp4[3], yLerp4[3]);

#ifdef _DEBUG
		if (xLerp4[0] > 1.0f || xLerp4[0] < 0.0f) __debugbreak();
		if (xLerp4[1] > 1.0f || xLerp4[1] < 0.0f) __debugbreak();
		if (xLerp4[2] > 1.0f || xLerp4[2] < 0.0f) __debugbreak();
		if (xLerp4[3] > 1.0f || xLerp4[3] < 0.0f) __debugbreak();
		if (yLerp4[0] > 1.0f || yLerp4[0] < 0.0f) __debugbreak();
		if (yLerp4[1] > 1.0f || yLerp4[1] < 0.0f) __debugbreak();
		if (yLerp4[2] > 1.0f || yLerp4[2] < 0.0f) __debugbreak();
		if (yLerp4[3] > 1.0f || yLerp4[3] < 0.0f) __debugbreak();
#endif
	}
		break;
	case D3DTEXF_NONE           :
	case D3DTEXF_POINT          :
	{
		const unsigned u4i[4] =
		{
			(const unsigned)u4[0],
			(const unsigned)u4[1],
			(const unsigned)u4[2],
			(const unsigned)u4[3]
		};
		const unsigned v4i[4] =
		{
			(const unsigned)v4[0],
			(const unsigned)v4[1],
			(const unsigned)v4[2],
			(const unsigned)v4[3]
		};
		GetPixelVec4<writeMask, sRGBSurface>(u4i, v4i, outColor4);
	}
		break;
	}
}

template void IDirect3DSurface9Hook::SampleSurface<0>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<1>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<2>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<3>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<4>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<5>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<6>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<7>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<8>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<9>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<10>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<11>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<12>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<13>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<14>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::SampleSurface<15>(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;

template <const unsigned char writeMask>
void IDirect3DSurface9Hook::SampleSurface(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const
{
	// TODO: Appropriately use magfilter and minfilter here, instead of always using the magfilter
	if (samplerState.stateUnion.namedStates.sRGBTexture)
		SampleSurfaceInternal<writeMask, true>(x, y, samplerState.stateUnion.namedStates.magFilter, outColor);
	else
		SampleSurfaceInternal<writeMask, false>(x, y, samplerState.stateUnion.namedStates.magFilter, outColor);
}

template void IDirect3DSurface9Hook::SampleSurface4<0>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<1>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<2>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<3>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<4>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<5>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<6>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<7>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<8>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<9>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<10>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<11>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<12>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<13>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<14>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::SampleSurface4<15>(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;
template <const unsigned char writeMask>
void IDirect3DSurface9Hook::SampleSurface4(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const
{
	// TODO: Appropriately use magfilter and minfilter here, instead of always using the magfilter
	if (samplerState.stateUnion.namedStates.sRGBTexture)
		SampleSurfaceInternal4<writeMask, true>(x4, y4, samplerState.stateUnion.namedStates.magFilter, outColor4);
	else
		SampleSurfaceInternal4<writeMask, false>(x4, y4, samplerState.stateUnion.namedStates.magFilter, outColor4);
}

#define DUMP_SURF_DIR "TexDump"

void IDirect3DSurface9Hook::InitDumpSurfaces(void)
{
	CreateDirectoryA(DUMP_SURF_DIR, NULL);
}

void IDirect3DSurface9Hook::DumpSurfaceToDisk(void) const
{
	//if (InternalFormat == D3DFMT_X8R8G8B8 || InternalFormat == D3DFMT_A8R8G8B8)
	{
		char surfaceFilename[MAX_PATH] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
#ifdef WITH_SURFACE_HASHING
		if (hash.format >= 0 && hash.format <= D3DFMT_BINARYBUFFER)
			sprintf(surfaceFilename, DUMP_SURF_DIR "\\Surf0x%08X_len0x%X_fmt%u_hash0x%08X.dds", (const unsigned)this, hash.sizeBytes, hash.format, hash.hashVal);
		else
			sprintf(surfaceFilename, DUMP_SURF_DIR "\\Surf0x%08X_len0x%X_fmt%c%c%c%c_hash0x%08X.dds", (const unsigned)this, hash.sizeBytes, 
				( (const char* const)&hash.format)[0], ( (const char* const)&hash.format)[1], ( (const char* const)&hash.format)[2], ( (const char* const)&hash.format)[3],
				hash.hashVal);
#else
		sprintf(surfaceFilename, DUMP_SURF_DIR "\\Surf0x%08X.dds", (const unsigned)this);
#endif
#pragma warning(pop)
		D3DXSaveSurfaceToFileA(surfaceFilename, D3DXIFF_DDS, (LPDIRECT3DSURFACE9)this, NULL, NULL);
	}
}

#ifdef WITH_SURFACE_HASHING
void IDirect3DSurface9Hook::RecomputeSurfaceHash(void)
{
	hash.format = InternalFormat;
	hash.sizeBytes = surfaceBytes.size()
#ifdef SURFACE_MAGIC_COOKIE
		- sizeof(DWORD) // Don't count the magic sentinel value DWORD as part of the hash (this enables hash-parity with Release builds)
#endif
		;

	unsigned long hashTemp = 0x00000000;
	const unsigned long* const ulSurfaceBytesRaw = (const unsigned long* const)surfaceBytesRaw;
	for (unsigned x = 0; x < hash.sizeBytes / sizeof(unsigned long); ++x)
		hashTemp ^= ulSurfaceBytesRaw[x];

	hash.hashVal = hashTemp;
}

void IDirect3DSurface9Hook::RecomputePartialSurfaceHash(const DWORD oldValue, const DWORD newValue)
{
	// This can more cleanly be thought of as xor'ing the oldValue against the hashVal and then xor'ing the newValue against the hashVal.
	// Doing it all in one step is just faster:
	hash.hashVal ^= (oldValue ^ newValue);
}
#endif
