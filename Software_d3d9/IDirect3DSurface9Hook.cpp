#pragma once

#include "GlobalToggles.h"
#include "IDirect3DSurface9Hook.h"

static const __m128i zeroMaskVecI = { 0 };
static const __m128 zeroVecF = { 0.0f, 0.0f, 0.0f, 0.0f };
static const __m128 oneVecF = { 1.0f, 1.0f, 1.0f, 1.0f };
static const __m128 d15divisor = { 1.0f / 32767.0f, 1.0f / 32767.0f, 1.0f / 32767.0f, 1.0f / 32767.0f };
static const __m128 d16divisor = { 1.0f / 65535.0f, 1.0f / 65535.0f, 1.0f / 65535.0f, 1.0f / 65535.0f };
static const __m128 d24divisor = { 1.0f / 16777215.0f, 1.0f / 16777215.0f, 1.0f / 16777215.0f, 1.0f / 16777215.0f };
static const __m128 d32divisor = { 1.0f / 4294967295.0f, 1.0f / 4294967295.0f, 1.0f / 4294967295.0f, 1.0f / 4294967295.0f };

static const unsigned oneInt4_DWORD[4] = { 0x1, 0x1, 0x1, 0x1 };
static const __m128i oneInt4 = *(const __m128i* const)oneInt4_DWORD;

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

#ifdef _DEBUG
void IDirect3DSurface9Hook::ValidateRealObjectDesc() const
{
	if (!realObject)
		return;

	D3DSURFACE_DESC desc = {};
	HRESULT ret = realObject->GetDesc(&desc);
	if (FAILED(ret) )
	{
		// We should have caught this error, but didn't for some reason
		__debugbreak();
	}

	if (desc.Width != InternalWidth)
	{
		__debugbreak();
	}
	if (desc.Height != InternalHeight)
	{
		__debugbreak();
	}
	if (desc.Format != InternalFormat)
	{
		__debugbreak();
	}
	if (desc.Pool != InternalPool)
	{
		__debugbreak();
	}
	if (desc.Usage != InternalUsage)
	{
		__debugbreak();
	}
	if (desc.Type != D3DRTYPE_SURFACE)
	{
		__debugbreak();
	}
	if (desc.MultiSampleType != InternalMultiSampleType)
	{
		__debugbreak();
	}
	if (desc.MultiSampleQuality != InternalMultiSampleQuality)
	{
		__debugbreak();
	}
}
#endif

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DSurface9Hook::GetDesc(THIS_ D3DSURFACE_DESC *pDesc)
{
	if (!pDesc)
		return D3DERR_INVALIDCALL;

#ifdef _DEBUG
	ValidateRealObjectDesc();
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

void IDirect3DSurface9Hook::UpdateCachedValuesOnCreate()
{
	is1x1surface = (InternalWidth == 1 && InternalHeight == 1);
	InternalWidthM1 = InternalWidth - 1;
	InternalHeightM1 = InternalHeight - 1;
	InternalWidthM1F = (const float)InternalWidthM1;
	InternalHeightM1F = (const float)InternalHeightM1;

	InternalWidthSplatted = _mm_set1_epi32(InternalWidth);
	InternalWidthSplattedF = _mm_cvtepi32_ps(InternalWidthSplatted);
	InternalHeightSplattedF = _mm_cvtepi32_ps(_mm_set1_epi32(InternalHeight) );
	InternalWidthM1SplattedF = _mm_set1_ps(InternalWidthM1F);
	InternalHeightM1SplattedF = _mm_set1_ps(InternalHeightM1F);
	InternalWidthM1Splatted = _mm_set1_epi32(InternalWidthM1);
	InternalHeightM1Splatted = _mm_set1_epi32(InternalHeightM1);
	InternalWidthHeightM1F = _mm_set_ps(0.0f, 0.0f, InternalHeightM1F, InternalWidthM1F);
	InternalWidthHeightM2F = _mm_set_ps(0.0f, 0.0f, (const float)(InternalHeightM1 - 1), (const float)(InternalWidthM1 - 1) );
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
	UpdateCachedValuesOnCreate();

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

#ifdef _DEBUG
	ValidateRealObjectDesc();
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
	InternalUsage = (const DebuggableUsage)(D3DUSAGE_DEPTHSTENCIL); // Important not to mark this with the D3DUSAGE_RENDERTARGET Usage because that will get a Usage-mismatch with real D3D9!
	UpdateCachedValuesOnCreate();

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

#ifdef _DEBUG
	ValidateRealObjectDesc();
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
	UpdateCachedValuesOnCreate();

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

#ifdef _DEBUG
	ValidateRealObjectDesc();
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
	UpdateCachedValuesOnCreate();

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

#ifdef _DEBUG
	ValidateRealObjectDesc();
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
	UpdateCachedValuesOnCreate();

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

#ifdef _DEBUG
	ValidateRealObjectDesc();
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
	UpdateCachedValuesOnCreate();

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

#ifdef _DEBUG
	ValidateRealObjectDesc();
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
template void IDirect3DSurface9Hook::GetPixelVec<10, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<11, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<12, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<13, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<14, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<15, false>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
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
template void IDirect3DSurface9Hook::GetPixelVec<10, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<11, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<12, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<13, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<14, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;
template void IDirect3DSurface9Hook::GetPixelVec<15, true>(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;

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

template <const unsigned char writeMask, const unsigned char pixelWriteMask>
static inline void GammaCorrectSample4(D3DXVECTOR4 (&outColor4)[4])
{
	if (writeMask & 0x1)
	{
		if (pixelWriteMask & 0x1) outColor4[0].x = powf(outColor4[0].x, gamma2_2);
		if (pixelWriteMask & 0x2) outColor4[1].x = powf(outColor4[1].x, gamma2_2);
		if (pixelWriteMask & 0x4) outColor4[2].x = powf(outColor4[2].x, gamma2_2);
		if (pixelWriteMask & 0x8) outColor4[3].x = powf(outColor4[3].x, gamma2_2);
	}
	if (writeMask & 0x2)
	{
		if (pixelWriteMask & 0x1) outColor4[0].y = powf(outColor4[0].y, gamma2_2);
		if (pixelWriteMask & 0x2) outColor4[1].y = powf(outColor4[1].y, gamma2_2);
		if (pixelWriteMask & 0x4) outColor4[2].y = powf(outColor4[2].y, gamma2_2);
		if (pixelWriteMask & 0x8) outColor4[3].y = powf(outColor4[3].y, gamma2_2);
	}
	if (writeMask & 0x4)
	{
		if (pixelWriteMask & 0x1) outColor4[0].z = powf(outColor4[0].z, gamma2_2);
		if (pixelWriteMask & 0x2) outColor4[1].z = powf(outColor4[1].z, gamma2_2);
		if (pixelWriteMask & 0x4) outColor4[2].z = powf(outColor4[2].z, gamma2_2);
		if (pixelWriteMask & 0x8) outColor4[3].z = powf(outColor4[3].z, gamma2_2);
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

template <const unsigned char writeMask, const unsigned char pixelWriteMask>
static inline void GammaCorrectSample4(D3DXVECTOR4 (&outColor4)[4])
{
	if (writeMask & 0x1)
	{
		if (pixelWriteMask & 0x1)
		{
			if (outColor4[0].x > 0.04045) outColor4[0].x = powf( (outColor4[0].x + 0.055f) * inv1055, gamma2_4);
			else outColor4[0].x = outColor4[0].x * inv1292;
		}
		if (pixelWriteMask & 0x2)
		{
			if (outColor4[1].x > 0.04045) outColor4[1].x = powf( (outColor4[1].x + 0.055f) * inv1055, gamma2_4);
			else outColor4[1].x = outColor4[1].x * inv1292;
		}
		if (pixelWriteMask & 0x4)
		{
			if (outColor4[2].x > 0.04045) outColor4[2].x = powf( (outColor4[2].x + 0.055f) * inv1055, gamma2_4);
			else outColor4[2].x = outColor4[2].x * inv1292;
		}
		if (pixelWriteMask & 0x8)
		{
			if (outColor4[3].x > 0.04045) outColor4[3].x = powf( (outColor4[3].x + 0.055f) * inv1055, gamma2_4);
			else outColor4[3].x = outColor4[3].x * inv1292;
		}
	}
	if (writeMask & 0x2)
	{
		if (pixelWriteMask & 0x1)
		{
			if (outColor4[0].y > 0.04045) outColor4[0].y = powf( (outColor4[0].y + 0.055f) * inv1055, gamma2_4);
			else outColor4[0].y = outColor4[0].y * inv1292;
		}
		if (pixelWriteMask & 0x2)
		{
			if (outColor4[1].y > 0.04045) outColor4[1].y = powf( (outColor4[1].y + 0.055f) * inv1055, gamma2_4);
			else outColor4[1].y = outColor4[1].y * inv1292;
		}
		if (pixelWriteMask & 0x4)
		{
			if (outColor4[2].y > 0.04045) outColor4[2].y = powf( (outColor4[2].y + 0.055f) * inv1055, gamma2_4);
			else outColor4[2].y = outColor4[2].y * inv1292;
		}
		if (pixelWriteMask & 0x8)
		{
			if (outColor4[3].y > 0.04045) outColor4[3].y = powf( (outColor4[3].y + 0.055f) * inv1055, gamma2_4);
			else outColor4[3].y = outColor4[3].y * inv1292;
		}
	}
	if (writeMask & 0x4)
	{
		if (pixelWriteMask & 0x1)
		{
			if (outColor4[0].z > 0.04045) outColor4[0].z = powf( (outColor4[0].z + 0.055f) * inv1055, gamma2_4);
			else outColor4[0].z = outColor4[0].z * inv1292;
		}
		if (pixelWriteMask & 0x2)
		{
			if (outColor4[1].z > 0.04045) outColor4[1].z = powf( (outColor4[1].z + 0.055f) * inv1055, gamma2_4);
			else outColor4[1].z = outColor4[1].z * inv1292;
		}
		if (pixelWriteMask & 0x4)
		{
			if (outColor4[2].z > 0.04045) outColor4[2].z = powf( (outColor4[2].z + 0.055f) * inv1055, gamma2_4);
			else outColor4[2].z = outColor4[2].z * inv1292;
		}
		if (pixelWriteMask & 0x8)
		{
			if (outColor4[3].z > 0.04045) outColor4[3].z = powf( (outColor4[3].z + 0.055f) * inv1055, gamma2_4);
			else outColor4[3].z = outColor4[3].z * inv1292;
		}
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

template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x1, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x2, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x3, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x4, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x5, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x6, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x7, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x8, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0x9, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xA, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xB, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xC, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xD, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xE, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0x3>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0x5>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0x6>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0x7>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0x9>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0xA>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0xB>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0xC>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0xD>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0xE>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;
template void IDirect3DSurface9Hook::GetPixelVec4<0xF, false, 0xF>(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const;

template <const unsigned char writeMask, const bool sRGBSurface, const unsigned char pixelWriteMask/* = 0xF*/>
void IDirect3DSurface9Hook::GetPixelVec4(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&outColor4)[4]) const
{
	if (pixelWriteMask == 0x0)
	{
#ifdef _DEBUG
		__debugbreak(); // Don't call this function if you don't intend to actually get any pixels!
#endif
		return;
	}

	const __m128i pixelIndex = _mm_add_epi32(_mm_mullo_epi32(y4, InternalWidthSplatted), x4);
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

		const __m128i ldrColor4 = _mm_i32gather_epi32( (const int* const)pixels, pixelIndex, 4);

		ColorDWORDToFloat4_4<(writeMask == 0xF) ? 0xF : (writeMask & 0x7)>(ldrColor4, outColor4);

		// Set the W components to 1.0f:
		if (pixelWriteMask & 0x1) *(__m128* const)&outColor4[0] = _mm_blend_ps(*(__m128* const)&outColor4[0], oneVecF, 1 << 3);
		if (pixelWriteMask & 0x2) *(__m128* const)&outColor4[1] = _mm_blend_ps(*(__m128* const)&outColor4[1], oneVecF, 1 << 3);
		if (pixelWriteMask & 0x4) *(__m128* const)&outColor4[2] = _mm_blend_ps(*(__m128* const)&outColor4[2], oneVecF, 1 << 3);
		if (pixelWriteMask & 0x8) *(__m128* const)&outColor4[3] = _mm_blend_ps(*(__m128* const)&outColor4[3], oneVecF, 1 << 3);

		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7, pixelWriteMask>(outColor4);
	}
		break;
	case D3DFMT_A8R8G8B8:
	{
		const D3DCOLOR* const pixels = (const D3DCOLOR* const)surfaceBytesRaw;
		const __m128i ldrColor4 = _mm_i32gather_epi32( (const int* const)pixels, pixelIndex, 4);
		ColorDWORDToFloat4_4<writeMask>(ldrColor4, outColor4);
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7, pixelWriteMask>(outColor4);
	}
		break;
	case D3DFMT_A16B16G16R16:
	{
		const A16B16G16R16* const pixels = (const A16B16G16R16* const)surfaceBytesRaw;
		const A16B16G16R16 pixel4[4] = 
		{
			pixels[pixelIndex.m128i_u32[0] ],
			pixels[pixelIndex.m128i_u32[1] ],
			pixels[pixelIndex.m128i_u32[2] ],
			pixels[pixelIndex.m128i_u32[3] ]
		};

		ColorA16B16G16R16ToFloat4_4<writeMask>(pixel4, outColor4);
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7, pixelWriteMask>(outColor4);
	}
		break;
	case D3DFMT_A16B16G16R16F:
	{
		const A16B16G16R16F* const pixels = (const A16B16G16R16F* const)surfaceBytesRaw;
		const A16B16G16R16F* const pixel4[4] = 
		{
			&pixels[pixelIndex.m128i_u32[0] ],
			&pixels[pixelIndex.m128i_u32[1] ],
			&pixels[pixelIndex.m128i_u32[2] ],
			&pixels[pixelIndex.m128i_u32[3] ]
		};

		ColorA16B16G16R16FToFloat4_4<writeMask>(pixel4, outColor4);
	}
	break;
	case D3DFMT_A32B32G32R32F:
	{
		const A32B32G32R32F* const pixels = (const A32B32G32R32F* const)surfaceBytesRaw;
		const A32B32G32R32F* const pixel4[4] = 
		{
			&pixels[pixelIndex.m128i_u32[0]  ],
			&pixels[pixelIndex.m128i_u32[1]  ],
			&pixels[pixelIndex.m128i_u32[2]  ],
			&pixels[pixelIndex.m128i_u32[3]  ]
		};

		ColorA32B32G32R32FToFloat4_4<writeMask>(pixel4, outColor4);
	}
		break;
	case D3DFMT_R16F:
	{
		const D3DXFLOAT16* const pixels = (const D3DXFLOAT16* const)surfaceBytesRaw;
		const D3DXFLOAT16 pixel4[4] = 
		{
			pixels[pixelIndex.m128i_u32[0] ],
			pixels[pixelIndex.m128i_u32[1] ],
			pixels[pixelIndex.m128i_u32[2] ],
			pixels[pixelIndex.m128i_u32[3] ]
		};

		ColorR16FToFloat4_4<writeMask>(pixel4, outColor4);
	}
		break;
	case D3DFMT_L8:
	{
		const unsigned char* const pixels = (const unsigned char* const)surfaceBytesRaw;
		__m128i pixelsSSE;
		if (pixelWriteMask & 0x1) pixelsSSE.m128i_u8[0] = pixels[pixelIndex.m128i_u32[0] ];
		if (pixelWriteMask & 0x2) pixelsSSE.m128i_u8[1] = pixels[pixelIndex.m128i_u32[1] ];
		if (pixelWriteMask & 0x4) pixelsSSE.m128i_u8[2] = pixels[pixelIndex.m128i_u32[2] ];
		if (pixelWriteMask & 0x8) pixelsSSE.m128i_u8[3] = pixels[pixelIndex.m128i_u32[3] ];

		L8ToFloat4_4<writeMask>(pixelsSSE, outColor4);
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7, pixelWriteMask>(outColor4);
	}
	break;
	case D3DFMT_DXT1:
	case D3DFMT_DXT2:
	case D3DFMT_DXT3:
	case D3DFMT_DXT4:
	case D3DFMT_DXT5:
	{
		const D3DCOLOR* const rawPixels = (const D3DCOLOR* const)auxSurfaceBytesRaw;
		const __m128i ldrColor4 = _mm_i32gather_epi32( (const int* const)rawPixels, pixelIndex, 4);
		ColorDWORDToFloat4_4<writeMask>(ldrColor4, outColor4);
		if (sRGBSurface)
			GammaCorrectSample4<writeMask & 0x7, pixelWriteMask>(outColor4);
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
#else
		__assume(0);
#endif
		break;
	}
	return 0.0f;
}

const __m128 IDirect3DSurface9Hook::GetDepth4(const __m128i x4, const __m128i y4) const
{
	const __m128i pixelIndex4 = _mm_add_epi32(_mm_mullo_epi32(y4, InternalWidthSplatted), x4);

	switch (InternalFormat)
	{
	case D3DFMT_D15S1:
	{
		const unsigned short* const pixels = (const unsigned short* const)surfaceBytesRaw;

		__m128i rawDepth4;
		rawDepth4.m128i_u32[0] = pixels[pixelIndex4.m128i_u32[0] ];
		rawDepth4.m128i_u32[1] = pixels[pixelIndex4.m128i_u32[1] ];
		rawDepth4.m128i_u32[2] = pixels[pixelIndex4.m128i_u32[2] ];
		rawDepth4.m128i_u32[3] = pixels[pixelIndex4.m128i_u32[3] ];

		const __m128 rawFloatDepth4 = _mm_cvtepi32_ps(rawDepth4);
		return _mm_mul_ps(rawFloatDepth4, d15divisor);
	}
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
	{
		const unsigned short* const pixels = (const unsigned short* const)surfaceBytesRaw;

		__m128i rawDepth4;
		rawDepth4.m128i_u32[0] = pixels[pixelIndex4.m128i_u32[0] ];
		rawDepth4.m128i_u32[1] = pixels[pixelIndex4.m128i_u32[1] ];
		rawDepth4.m128i_u32[2] = pixels[pixelIndex4.m128i_u32[2] ];
		rawDepth4.m128i_u32[3] = pixels[pixelIndex4.m128i_u32[3] ];

		const __m128 rawFloatDepth4 = _mm_cvtepi32_ps(rawDepth4);
		return _mm_mul_ps(rawFloatDepth4, d16divisor);
	}
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
	{
		const int* const pixels = (const int* const)surfaceBytesRaw;
		const __m128i rawDepth4 = _mm_i32gather_epi32(pixels, pixelIndex4, 4);

		const __m128 rawFloatDepth4 = _mm_cvtepi32_ps(rawDepth4);
		return _mm_mul_ps(rawFloatDepth4, d24divisor);
	}
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	{
		const int* const pixels = (const int* const)surfaceBytesRaw;
		const __m128i rawDepth4 = _mm_i32gather_epi32(pixels, pixelIndex4, 4);

		const __m128 rawFloatDepth4 = _mm_cvtepi32_ps(rawDepth4);
		return _mm_mul_ps(rawFloatDepth4, d32divisor);
	}
	case D3DFMT_D32F_LOCKABLE:
	{
		const float* const pixels = (const float* const)surfaceBytesRaw;
		const __m128 rawFloatDepth4 = _mm_i32gather_ps(pixels, pixelIndex4, 4);
		return rawFloatDepth4;
	}
	default:
#ifdef _DEBUG
		__debugbreak(); // Error: Can't call GetDepth() on non-depth formats!
#else
		__assume(0);
#endif
		break;
	}
	return zeroVecF;
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
#else
		__assume(0);
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
#else
		__assume(0);
#endif
		break;
	}
	return 0x00000000;
}

const __m128i IDirect3DSurface9Hook::GetRawDepth4(const __m128i x4, const __m128i y4) const
{
	const __m128i pixelIndex4 = _mm_add_epi32(_mm_mullo_epi32(y4, InternalWidthSplatted), x4);

	switch (InternalFormat)
	{
	case D3DFMT_D15S1:
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
	{
		const unsigned short* const pixels = (const unsigned short* const)surfaceBytesRaw;

		__m128i rawDepth4;
		rawDepth4.m128i_u32[0] = pixels[pixelIndex4.m128i_u32[0] ];
		rawDepth4.m128i_u32[1] = pixels[pixelIndex4.m128i_u32[1] ];
		rawDepth4.m128i_u32[2] = pixels[pixelIndex4.m128i_u32[2] ];
		rawDepth4.m128i_u32[3] = pixels[pixelIndex4.m128i_u32[3] ];
		return rawDepth4;
	}
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	case D3DFMT_D32F_LOCKABLE:
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	{
		const int* const pixels = (const int* const)surfaceBytesRaw;
		const __m128i rawDepth4 = _mm_i32gather_epi32(pixels, pixelIndex4, 4);
		return rawDepth4;
	}
	default:
#ifdef _DEBUG
		__debugbreak(); // Error: Can't call GetDepth() on non-depth formats!
#else
		__assume(0);
#endif
		break;
	}
	return zeroMaskVecI;
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
#else
		__assume(0);
#endif
		break;
	}
}

const __m128 depthScale15 = { 32767.0f, 32767.0f, 32767.0f, 32767.0f };
const __m128 depthScale16 = { 65535.0f, 65535.0f, 65535.0f, 65535.0f };
const __m128 depthScale24 = { 16777215.0f, 16777215.0f, 16777215.0f, 16777215.0f };
const __m128 depthScale32 = { 4294967295.0f, 4294967295.0f, 4294967295.0f, 4294967295.0f };

template void IDirect3DSurface9Hook::SetDepth4<0x3>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0x5>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0x6>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0x7>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0x9>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0xA>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0xB>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0xC>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0xD>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0xE>(const __m128i x4, const __m128i y4, const __m128 depth4);
template void IDirect3DSurface9Hook::SetDepth4<0xF>(const __m128i x4, const __m128i y4, const __m128 depth4);

template <const unsigned char pixelWriteMask>
void IDirect3DSurface9Hook::SetDepth4(const __m128i x4, const __m128i y4, const __m128 depth4)
{
	switch (pixelWriteMask)
	{
	case 0x0:
	case 0x1:
	case 0x2:
	case 0x4:
	case 0x8:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	}

	__m128i pixelIndex4;
	unsigned pixelIndex1; // Only used in the 0x3 and 0xC cases

	// Note: This code will need to be updated when depth buffers, stencil buffers, and render targets are quad-swizzled
	switch (pixelWriteMask)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case 0x3: // top row two, one write
		pixelIndex1 = x4.m128i_u32[0] + y4.m128i_u32[0] * InternalWidth;
		break;
	case 0xC: // bottom row two, one write
		pixelIndex1 = x4.m128i_u32[0] + y4.m128i_u32[2] * InternalWidth;
		break;
	case 0x5: // two writes
	case 0x6: // two writes
	case 0x9: // two writes
	case 0xA: // two writes
	case 0x7: // top row two, one bottom, two writes
	case 0xB: // top row two, one bottom, two writes
	case 0xD: // top row one, bottom row two, two writes
	case 0xE: // top row one, bottom row two, two writes
	case 0xF: // top row two, bottom row two, two writes
		pixelIndex4 = _mm_add_epi32(_mm_mullo_epi32(y4, InternalWidthSplatted), x4);
		break;
	}

	// Sadly we don't have scatter instructions in AVX2, so one or two consecutive writes is the best we can do if our write-mask isn't 0xF
	switch (InternalFormat)
	{
	case D3DFMT_D15S1:
	{
		unsigned short* const pixels = (unsigned short* const)surfaceBytesRaw;

		const __m128 scaledDepth4 = _mm_mul_ps(depthScale15, depth4);
		const __m128i uDepth4 = _mm_cvtps_epi32(scaledDepth4);
		__m128i shuffledUDepth16_4;
		switch (pixelWriteMask)
		{
		default:
#ifdef _DEBUG
			__debugbreak();
#else
			__assume(0);
#endif
		case 0x5: // two writes
		case 0x6: // two writes
		case 0x9: // two writes
		case 0xA: // two writes
			break;
		case 0x3: // top row two, one write
		case 0x7: // top row two, one bottom, two writes
		case 0xB: // top row two, one bottom, two writes
		case 0xC: // bottom row two, one write
		case 0xD: // top row one, bottom row two, two writes
		case 0xE: // top row one, bottom row two, two writes
		case 0xF: // top row two, bottom row two, two writes
			shuffledUDepth16_4 = _mm_shufflelo_epi16(uDepth4, _MM_SHUFFLE(3, 2, 1, 0) ); // Shuffle (8 16-bit words) from: { X, 0, Y, 0, Z, 0, W, 0 } to { X, Y, Z, W, Z, 0, W, 0 }
			break;
		}

		if (pixelWriteMask == 0x3)
		{
			*(unsigned* const)(pixels + pixelIndex1) = shuffledUDepth16_4.m128i_u32[0];
		}
		else if ( (pixelWriteMask & 0x3) == 0x3)
		{
			*(unsigned* const)(pixels + pixelIndex4.m128i_u32[0]) = shuffledUDepth16_4.m128i_u32[0];
		}
		else
		{
			if (pixelWriteMask & 0x1) pixels[pixelIndex4.m128i_u32[0] ] = uDepth4.m128i_u32[0];
			if (pixelWriteMask & 0x2) pixels[pixelIndex4.m128i_u32[1] ] = uDepth4.m128i_u32[1];
		}

		if (pixelWriteMask == 0xC)
		{
			*(unsigned* const)(pixels + pixelIndex1) = shuffledUDepth16_4.m128i_u32[1];
		}
		else if ( (pixelWriteMask & 0xC) == 0xC)
		{
			*(unsigned* const)(pixels + pixelIndex4.m128i_u32[2]) = shuffledUDepth16_4.m128i_u32[1];
		}
		else
		{
			if (pixelWriteMask & 0x4) pixels[pixelIndex4.m128i_u32[2] ] = uDepth4.m128i_u32[2];
			if (pixelWriteMask & 0x8) pixels[pixelIndex4.m128i_u32[3] ] = uDepth4.m128i_u32[3];
		}
	}
		break;
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
	{
		unsigned short* const pixels = (unsigned short* const)surfaceBytesRaw;

		const __m128 scaledDepth4 = _mm_mul_ps(depthScale16, depth4);
		const __m128i uDepth4 = _mm_cvtps_epi32(scaledDepth4);

		__m128i shuffledUDepth16_4;
		switch (pixelWriteMask)
		{
		default:
#ifdef _DEBUG
			__debugbreak();
#else
			__assume(0);
#endif
		case 0x5: // two writes
		case 0x6: // two writes
		case 0x9: // two writes
		case 0xA: // two writes
			break;
		case 0x3: // top row two, one write
		case 0x7: // top row two, one bottom, two writes
		case 0xB: // top row two, one bottom, two writes
		case 0xC: // bottom row two, one write
		case 0xD: // top row one, bottom row two, two writes
		case 0xE: // top row one, bottom row two, two writes
		case 0xF: // top row two, bottom row two, two writes
			shuffledUDepth16_4 = _mm_shufflelo_epi16(uDepth4, _MM_SHUFFLE(3, 2, 1, 0) ); // Shuffle (8 16-bit words) from: { X, 0, Y, 0, Z, 0, W, 0 } to { X, Y, Z, W, Z, 0, W, 0 }
			break;
		}

		if (pixelWriteMask == 0x3)
		{
			*(unsigned* const)(pixels + pixelIndex1) = shuffledUDepth16_4.m128i_u32[0];
		}
		else if ( (pixelWriteMask & 0x3) == 0x3)
		{
			*(unsigned* const)(pixels + pixelIndex4.m128i_u32[0]) = shuffledUDepth16_4.m128i_u32[0];
		}
		else
		{
			if (pixelWriteMask & 0x1) pixels[pixelIndex4.m128i_u32[0] ] = uDepth4.m128i_u32[0];
			if (pixelWriteMask & 0x2) pixels[pixelIndex4.m128i_u32[1] ] = uDepth4.m128i_u32[1];
		}

		if (pixelWriteMask == 0xC)
		{
			*(unsigned* const)(pixels + pixelIndex1) = shuffledUDepth16_4.m128i_u32[1];
		}
		else if ( (pixelWriteMask & 0xC) == 0xC)
		{
			*(unsigned* const)(pixels + pixelIndex4.m128i_u32[2]) = shuffledUDepth16_4.m128i_u32[1];
		}
		else
		{
			if (pixelWriteMask & 0x4) pixels[pixelIndex4.m128i_u32[2] ] = uDepth4.m128i_u32[2];
			if (pixelWriteMask & 0x8) pixels[pixelIndex4.m128i_u32[3] ] = uDepth4.m128i_u32[3];
		}
	}
		break;
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
	{
		unsigned* const pixels = (unsigned* const)surfaceBytesRaw;

		const __m128 scaledDepth4 = _mm_mul_ps(depthScale24, depth4);
		const __m128i uDepth4 = _mm_cvtps_epi32(scaledDepth4);

		if (pixelWriteMask == 0x3)
		{
			_mm_storel_epi64( (__m128i* const)(pixels + pixelIndex1), uDepth4);
		}
		else if ( (pixelWriteMask & 0x3) == 0x3)
		{
			_mm_storel_epi64( (__m128i* const)(pixels + pixelIndex4.m128i_u32[0]), uDepth4);
		}
		else
		{
			if (pixelWriteMask & 0x1) pixels[pixelIndex4.m128i_u32[0] ] = uDepth4.m128i_u32[0];
			if (pixelWriteMask & 0x2) pixels[pixelIndex4.m128i_u32[1] ] = uDepth4.m128i_u32[1];
		}

		if (pixelWriteMask == 0xC)
		{
			_mm_storeh_pi( (__m64* const)(pixels + pixelIndex1), _mm_castsi128_ps(uDepth4) );
		}
		else if ( (pixelWriteMask & 0xC) == 0xC)
		{
			_mm_storeh_pi( (__m64* const)(pixels + pixelIndex4.m128i_u32[2]), _mm_castsi128_ps(uDepth4) );
		}
		else
		{
			if (pixelWriteMask & 0x4) pixels[pixelIndex4.m128i_u32[2] ] = uDepth4.m128i_u32[2];
			if (pixelWriteMask & 0x8) pixels[pixelIndex4.m128i_u32[3] ] = uDepth4.m128i_u32[3];
		}
	}
		break;
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	case MAKEFOURCC('I', 'N', 'T', 'Z'):
	{
		unsigned* const pixels = (unsigned* const)surfaceBytesRaw;

		const __m128 scaledDepth4 = _mm_mul_ps(depthScale32, depth4);
		const __m128i uDepth4 = _mm_cvtps_epi32(scaledDepth4);

		if (pixelWriteMask == 0x3)
		{
			_mm_storel_epi64( (__m128i* const)(pixels + pixelIndex1), uDepth4);
		}
		else if ( (pixelWriteMask & 0x3) == 0x3)
		{
			_mm_storel_epi64( (__m128i* const)(pixels + pixelIndex4.m128i_u32[0]), uDepth4);
		}
		else
		{
			if (pixelWriteMask & 0x1) pixels[pixelIndex4.m128i_u32[0] ] = uDepth4.m128i_u32[0];
			if (pixelWriteMask & 0x2) pixels[pixelIndex4.m128i_u32[1] ] = uDepth4.m128i_u32[1];
		}

		if (pixelWriteMask == 0xC)
		{
			_mm_storeh_pi( (__m64* const)(pixels + pixelIndex1), _mm_castsi128_ps(uDepth4) );
		}
		else if ( (pixelWriteMask & 0xC) == 0xC)
		{
			_mm_storeh_pi( (__m64* const)(pixels + pixelIndex4.m128i_u32[2]), _mm_castsi128_ps(uDepth4) );
		}
		else
		{
			if (pixelWriteMask & 0x4) pixels[pixelIndex4.m128i_u32[2] ] = uDepth4.m128i_u32[2];
			if (pixelWriteMask & 0x8) pixels[pixelIndex4.m128i_u32[3] ] = uDepth4.m128i_u32[3];
		}
	}
		break;
	case D3DFMT_D32F_LOCKABLE:
	{
		float* const pixels = (float* const)surfaceBytesRaw;

		if (pixelWriteMask == 0x3)
		{
			_mm_storel_pi( (__m64* const)(pixels + pixelIndex1), depth4);
		}
		else if ( (pixelWriteMask & 0x3) == 0x3)
		{
			_mm_storel_pi( (__m64* const)(pixels + pixelIndex4.m128i_u32[0]), depth4);
		}
		else
		{
			if (pixelWriteMask & 0x1) pixels[pixelIndex4.m128i_u32[0] ] = depth4.m128_f32[0];
			if (pixelWriteMask & 0x2) pixels[pixelIndex4.m128i_u32[1] ] = depth4.m128_f32[1];
		}

		if (pixelWriteMask == 0xC)
		{
			_mm_storeh_pi( (__m64* const)(pixels + pixelIndex1), depth4);
		}
		else if ( (pixelWriteMask & 0xC) == 0xC)
		{
			_mm_storeh_pi( (__m64* const)(pixels + pixelIndex4.m128i_u32[2]), depth4);
		}
		else
		{
			if (pixelWriteMask & 0x4) pixels[pixelIndex4.m128i_u32[2] ] = depth4.m128_f32[2];
			if (pixelWriteMask & 0x8) pixels[pixelIndex4.m128i_u32[3] ] = depth4.m128_f32[3];
		}
	}
		break;
	default:
#ifdef _DEBUG
		__debugbreak(); // Error: Can't call SetDepth4() on non-depth formats!
#else
		__assume(0);
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

void IDirect3DSurface9Hook::SetStencil4(const __m128i x4, const __m128i y4, const DWORD stencil4)
{
	DWORD stencilMask4 = GetStencilFormatMask(InternalFormat);
	stencilMask4 |= (stencilMask4 << 8);
	stencilMask4 |= (stencilMask4 << 16);
	const DWORD maskedStencil = stencil4 & stencilMask4;

	const __m128i pixelIndex4 = _mm_add_epi32(_mm_mullo_epi32(y4, InternalWidthSplatted), x4);

	BYTE* const stencilBuffer = auxSurfaceBytesRaw;
	stencilBuffer[pixelIndex4.m128i_u32[0] ] = (const BYTE)maskedStencil;
	stencilBuffer[pixelIndex4.m128i_u32[1] ] = (const BYTE)(maskedStencil >> 8);
	stencilBuffer[pixelIndex4.m128i_u32[2] ] = (const BYTE)(maskedStencil >> 16);
	stencilBuffer[pixelIndex4.m128i_u32[3] ] = (const BYTE)(maskedStencil >> 24);

#ifdef WITH_SURFACE_HASHING
	RecomputeSurfaceHash();
#endif
}

const DWORD IDirect3DSurface9Hook::GetStencil(const unsigned x, const unsigned y) const
{
	const BYTE* const stencilBuffer = auxSurfaceBytesRaw;
	return stencilBuffer[y * InternalWidth + x];
}

const __m128i IDirect3DSurface9Hook::GetStencil4(const __m128i x4, const __m128i y4) const
{
	const BYTE* const stencilBuffer = auxSurfaceBytesRaw;

	const __m128i byteOffsets = _mm_add_epi32(_mm_mullo_epi32(InternalWidthSplatted, y4), x4);

	__m128i stencilLookups;
	stencilLookups.m128i_u32[0] = stencilBuffer[byteOffsets.m128i_u32[0] ];
	stencilLookups.m128i_u32[1] = stencilBuffer[byteOffsets.m128i_u32[1] ];
	stencilLookups.m128i_u32[2] = stencilBuffer[byteOffsets.m128i_u32[2] ];
	stencilLookups.m128i_u32[3] = stencilBuffer[byteOffsets.m128i_u32[3] ];

	return stencilLookups;
}

// Don't define the <0> version of this function. Anybody calling it is a dummy because it doesn't write anything anyway.
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

template <const unsigned char channelWriteMask>
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
		const D3DCOLOR ldrColor = Float4ToX8R8G8B8Clamp<channelWriteMask & 0x7>(color);
		SetPixel(x, y, ldrColor);
	}
		break;
	case D3DFMT_A8R8G8B8:
	{
		const D3DCOLOR ldrColor = Float4ToD3DCOLORClamp<channelWriteMask>(color);
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
		Float4ToA16B16G16R16<channelWriteMask>(color, writePixel);
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
		Float4ToA16B16G16R16F<channelWriteMask>(color, writePixel);
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
		Float4ToA32B32G32R32F<channelWriteMask>(color, writePixel);
#ifdef WITH_SURFACE_HASHING
		RecomputePartialSurfaceHashXor(writePixel);
#endif
	}
		break;
	case D3DFMT_R16F:
	{
		D3DXFLOAT16* const pixels = (D3DXFLOAT16* const)surfaceBytesRaw;
		D3DXFLOAT16& writePixel = pixels[y * InternalWidth + x];
		Float4ToR16F<channelWriteMask>(color, writePixel);

		// Can't update the surface hash in this case...
	}
		break;
	case D3DFMT_L8:
	{
		unsigned char* const pixels = (unsigned char* const)surfaceBytesRaw;
		unsigned char& writePixel = pixels[y * InternalWidth + x];
		Float4ToL8Clamp<channelWriteMask>(color, writePixel);

		// Can't update the surface hash in this case...
	}
		break;
	}
}

// Don't define the <0> version of this function. Anybody calling it is a dummy because it doesn't write anything anyway.
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0x3>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0x5>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0x6>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0x7>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0x9>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0xA>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0xB>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0xC>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0xD>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0xE>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x1, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x2, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x3, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x4, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x5, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x6, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x7, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x8, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0x9, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xA, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xB, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xC, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xD, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xE, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);
template void IDirect3DSurface9Hook::SetPixelVec4<0xF, 0xF>(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);

template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
void IDirect3DSurface9Hook::SetPixelVec4(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4])
{
	if (channelWriteMask == 0)
	{
#ifdef _DEBUG
		__debugbreak(); // Don't call this with 0 write masks!
#endif
		return;
	}

	// See if we can drop down to non-quad operations if possible:
	switch (pixelWriteMask)
	{
	default:
	case 0x0:
#ifdef _DEBUG
		__debugbreak(); // Don't call this with 0 write masks!
#else
		__assume(0);
#endif
		return;
	case 0x1:
		SetPixelVec<channelWriteMask>(x4.m128i_u32[0], y4.m128i_u32[0], color[0]);
		return;
	case 0x2:
		SetPixelVec<channelWriteMask>(x4.m128i_u32[1], y4.m128i_u32[1], color[1]);
		return;
	case 0x4:
		SetPixelVec<channelWriteMask>(x4.m128i_u32[2], y4.m128i_u32[2], color[2]);
		return;
	case 0x8:
		SetPixelVec<channelWriteMask>(x4.m128i_u32[3], y4.m128i_u32[3], color[3]);
		return;
	case 0x3:
	case 0x5:
	case 0x6:
	case 0x7:
	case 0x9:
	case 0xA:
	case 0xB:
	case 0xC:
	case 0xD:
	case 0xE:
	case 0xF:
		break;
	}

	const __m128i pixelIndex = _mm_add_epi32(_mm_mullo_epi32(y4, InternalWidthSplatted), x4);
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
		D3DCOLOR* const pixels = (D3DCOLOR* const)surfaceBytesRaw;
		const __m128i pixelByteOffset4 = _mm_slli_epi32(pixelIndex, 2); // Left-shift by 2 is the same as multiply by sizeof(D3DCOLOR)
#ifdef _M_X64
		#error This won't work on x64!
#else
		const __m128i writeAddresses = _mm_add_epi32(_mm_set1_epi32( (const unsigned)pixels), pixelByteOffset4);
#endif
		Float4ToX8R8G8B8_4Clamp4<channelWriteMask, pixelWriteMask>(color, writeAddresses);
	}
		break;
	case D3DFMT_A8R8G8B8:
	{
		D3DCOLOR* const pixels = (D3DCOLOR* const)surfaceBytesRaw;
		const __m128i pixelByteOffset4 = _mm_slli_epi32(pixelIndex, 2); // Left-shift by 2 is the same as multiply by sizeof(D3DCOLOR)
#ifdef _M_X64
		#error This won't work on x64!
#else
		const __m128i writeAddresses = _mm_add_epi32(_mm_set1_epi32( (const unsigned)pixels), pixelByteOffset4);
#endif
		Float4ToD3DCOLOR4Clamp4<channelWriteMask, pixelWriteMask>(color, writeAddresses);
	}
		break;
	case D3DFMT_A16B16G16R16:
	{
		A16B16G16R16* const pixels = (A16B16G16R16* const)surfaceBytesRaw;
		const __m128i pixelByteOffset4 = _mm_slli_epi32(pixelIndex, 3); // Left-shift by 3 is the same as multiply by sizeof(A16B16G16R16)
#ifdef _M_X64
		#error This won't work on x64!
#else
		const __m128i writeAddresses = _mm_add_epi32(_mm_set1_epi32( (const unsigned)pixels), pixelByteOffset4);
#endif
		Float4ToA16B16G16R16_4<channelWriteMask, pixelWriteMask>(color, writeAddresses);
	}
		break;
	case D3DFMT_A16B16G16R16F:
	{
		A16B16G16R16F* const pixels = (A16B16G16R16F* const)surfaceBytesRaw;
		const __m128i pixelByteOffset4 = _mm_slli_epi32(pixelIndex, 3); // Left-shift by 3 is the same as multiply by sizeof(A16B16G16R16F)
#ifdef _M_X64
		#error This won't work on x64!
#else
		const __m128i writeAddresses = _mm_add_epi32(_mm_set1_epi32( (const unsigned)pixels), pixelByteOffset4);
#endif
		Float4ToA16B16G16R16F4<channelWriteMask, pixelWriteMask>(color, writeAddresses);
	}
		break;
	case D3DFMT_A32B32G32R32F:
	{
		A32B32G32R32F* const pixels = (A32B32G32R32F* const)surfaceBytesRaw;
		const __m128i pixelByteOffset4 = _mm_slli_epi32(pixelIndex, 4); // Left-shift by 4 is the same as multiply by sizeof(A32B32G32R32F)
#ifdef _M_X64
		#error This won't work on x64!
#else
		const __m128i writeAddresses = _mm_add_epi32(_mm_set1_epi32( (const unsigned)pixels), pixelByteOffset4);
#endif
		Float4ToA32B32G32R32F4<channelWriteMask, pixelWriteMask>(color, writeAddresses);
	}
		break;
	case D3DFMT_R16F:
	{
		D3DXFLOAT16* const pixels = (D3DXFLOAT16* const)surfaceBytesRaw;
		const __m128i pixelByteOffset4 = _mm_slli_epi32(pixelIndex, 1); // Left-shift by 1 is the same as multiply by sizeof(D3DXFLOAT16)
#ifdef _M_X64
		#error This won't work on x64!
#else
		const __m128i writeAddresses = _mm_add_epi32(_mm_set1_epi32( (const unsigned)pixels), pixelByteOffset4);
#endif
		Float4ToR16F4<channelWriteMask, pixelWriteMask>(color, writeAddresses);

		// Can't update the surface hash in this case...
	}
		break;
	case D3DFMT_L8:
	{
		unsigned char* const pixels = (unsigned char* const)surfaceBytesRaw;
#ifdef _M_X64
		#error This won't work on x64!
#else
		const __m128i writeAddresses = _mm_add_epi32(_mm_set1_epi32( (const unsigned)pixels), pixelIndex);
#endif
		Float4ToL8Clamp4<channelWriteMask, pixelWriteMask>(color, writeAddresses);

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

		__declspec(align(16) ) const __m128i x4 = _mm_set_epi32(cuBotright, cuBotleft, cuTopright, cuTopleft);
		__declspec(align(16) ) const __m128i y4 = _mm_set_epi32(cvBotright, cvBotleft, cvTopright, cvTopleft);

		__declspec(align(16) ) D3DXVECTOR4 bilinearSamples[4];
		GetPixelVec4<writeMask, sRGBSurface, 0xF>(x4, y4, bilinearSamples);

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

template <const unsigned char writeMask, const bool sRGBSurface, const unsigned pixelWriteMask>
void IDirect3DSurface9Hook::SampleSurfaceInternal4(const float (&x4)[4], const float (&y4)[4], const D3DTEXTUREFILTERTYPE texf, D3DXVECTOR4 (&outColor4)[4]) const
{
	// Keep the simple case simple
	if (is1x1surface)
	{
		D3DXVECTOR4 broadcastPixel;
		GetPixelVec<writeMask, sRGBSurface>(0, 0, broadcastPixel);

		if (pixelWriteMask & 0x1) outColor4[0] = broadcastPixel;
		if (pixelWriteMask & 0x2) outColor4[1] = broadcastPixel;
		if (pixelWriteMask & 0x4) outColor4[2] = broadcastPixel;
		if (pixelWriteMask & 0x8) outColor4[3] = broadcastPixel;
		return;
	}

	const __m128 x4vec = *(const __m128* const)x4;
	const __m128 y4vec = *(const __m128* const)y4;

	__m128 u4 = _mm_mul_ps(x4vec, InternalWidthSplattedF);
	__m128 v4 = _mm_mul_ps(y4vec, InternalHeightSplattedF);

	// if (u4 > InternalWidthM1F) u4 = InternalWidthM1F
	u4 = _mm_min_ps(u4, InternalWidthM1SplattedF);

	// if (v4 > InternalHeightM1F) u4 = InternalHeightM1F
	v4 = _mm_min_ps(v4, InternalHeightM1SplattedF);

	const __m128i u4i = _mm_cvtps_epi32(u4);
	const __m128i v4i = _mm_cvtps_epi32(v4);

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
		__declspec(align(16) ) D3DXVECTOR4 topleft4[4];

		GetPixelVec4<writeMask, sRGBSurface, pixelWriteMask>(u4i, v4i, topleft4);

		const __m128i u4iRight = _mm_min_epi32(_mm_add_epi32(u4i, oneInt4), InternalWidthM1Splatted);

		__declspec(align(16) ) D3DXVECTOR4 topright4[4];
		GetPixelVec4<writeMask, sRGBSurface, pixelWriteMask>(u4iRight, v4i, topright4);

		const __m128i v4iBottom = _mm_min_epi32(_mm_add_epi32(v4i, oneInt4), InternalHeightM1Splatted);

		__declspec(align(16) ) D3DXVECTOR4 botleft4[4];
		GetPixelVec4<writeMask, sRGBSurface, pixelWriteMask>(u4i, v4iBottom, botleft4);

		__declspec(align(16) ) D3DXVECTOR4 botright4[4];
		GetPixelVec4<writeMask, sRGBSurface, pixelWriteMask>(u4iRight, v4iBottom, botright4);

		const __m128 xLerp4 = _mm_sub_ps(u4, _mm_cvtepi32_ps(u4i) );

		D3DXVECTOR4 topHorizLerp4[4];
		lrp4<writeMask, pixelWriteMask>(topHorizLerp4, xLerp4, topleft4, topright4);

		D3DXVECTOR4 botHorizLerp4[4];
		lrp4<writeMask, pixelWriteMask>(botHorizLerp4, xLerp4, botleft4, botright4);

		const __m128 yLerp4 = _mm_sub_ps(v4, _mm_cvtepi32_ps(v4i) );
		lrp4<writeMask, pixelWriteMask>(outColor4, yLerp4, topHorizLerp4, botHorizLerp4);
	}
		break;
	case D3DTEXF_NONE           :
	case D3DTEXF_POINT          :
	{
		GetPixelVec4<writeMask, sRGBSurface, pixelWriteMask>(u4i, v4i, outColor4);
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
		SampleSurfaceInternal4<writeMask, true, 0xF>(x4, y4, samplerState.stateUnion.namedStates.magFilter, outColor4);
	else
		SampleSurfaceInternal4<writeMask, false, 0xF>(x4, y4, samplerState.stateUnion.namedStates.magFilter, outColor4);
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
