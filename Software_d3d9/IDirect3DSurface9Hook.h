#pragma once

#include "IDirect3DDevice9Hook.h"

#include "IDirect3DResource9Hook.h"

#if defined(DUMP_TEXTURES_ON_FIRST_SET) || defined(COMPUTE_SURFACE_HASHES_FOR_DEBUGGING) // No need to compute surface hashes if we're never going to need them
	#define WITH_SURFACE_HASHING 1
#endif

static inline const DWORD GetStencilFormatMask(const D3DFORMAT format)
{
	switch (format)
	{
	case D3DFMT_D24X4S4:
		return 0x0F;
	case D3DFMT_D15S1:
		return 0x01;
	case D3DFMT_D24S8:
	case D3DFMT_D24FS8:
	case D3DFMT_S8_LOCKABLE:
		return 0xFF;
	default:
#ifdef _DEBUG
		__debugbreak(); // Should never be here
#else
		__assume(0);
#endif
		return 0x00;
	}
}

static inline const bool HasStencil(const D3DFORMAT format)
{
	switch (format)
	{
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24FS8:
	case D3DFMT_D15S1:
	case D3DFMT_S8_LOCKABLE:
		return true;
	default:
		return false;
	}
}

#ifdef WITH_SURFACE_HASHING
struct surfaceHash
{
	surfaceHash() : sizeBytes(0), format(D3DFMT_UNKNOWN), hashVal(0)
	{
	}

	unsigned sizeBytes;
	D3DFORMAT format;
	unsigned long hashVal;
};
#endif

#ifdef SURFACE_MAGIC_COOKIE
static const DWORD surfaceMagicBytes = 0x13579BDF;
static inline void ValidateSurfaceMagicCookie(const std::vector<BYTE>& bytes)
{
	const DWORD* const magicDword = (const DWORD* const)(&bytes.front() ) + (bytes.size() / sizeof(DWORD) - 1);
	if (*magicDword != surfaceMagicBytes)
	{
		__debugbreak();
	}
}
#endif

__declspec(align(16) ) class IDirect3DSurface9Hook : public IDirect3DSurface9
{
public:
	IDirect3DSurface9Hook(LPDIRECT3DSURFACE9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1),
		InternalWidth(0), InternalHeight(0), TextureSurfaceLevel(0), InternalFormat(D3DFMT_UNKNOWN), InternalPool(D3DPOOL_DEFAULT), InternalUsage(UsageNone), DiscardRT(FALSE), LockableRT(FALSE), HookParentTexturePtr(NULL),
		creationMethod(unknown), surfaceBytesRaw(NULL), auxSurfaceBytesRaw(NULL), surfaceBytesRawSize(0), auxSurfaceBytesRawSize(0), InternalMultiSampleType(D3DMULTISAMPLE_NONE), InternalMultiSampleQuality(0),
		is1x1surface(false), InternalWidthM1(0), InternalHeightM1(0), InternalWidthM1F(0.0f), InternalHeightM1F(0.0f)
	{
#ifdef _DEBUG
		if (realObject)
			memcpy(&Name, &realObject->Name, (char*)&realObject - (char*)&Name);
		else
			memset(&Name, 0, (char*)&realObject - (char*)&Name);
#endif
	}

	virtual ~IDirect3DSurface9Hook()
	{
#ifdef SURFACE_ALLOC_PAGE_NOACCESS
		if (surfaceBytesRaw)
		{
			VirtualFree( (void*)( (size_t)surfaceBytesRaw & 4095), 0, MEM_RELEASE);
			surfaceBytesRaw = NULL;
		}
		if (auxSurfaceBytesRaw)
		{
			VirtualFree( (void*)( (size_t)auxSurfaceBytesRaw & 4095), 0, MEM_RELEASE);
			auxSurfaceBytesRaw = NULL;
		}
#else // #ifdef SURFACE_ALLOC_PAGE_NOACCESS
		if (surfaceBytesRaw)
		{
			free(surfaceBytesRaw);
			surfaceBytesRaw = NULL;
		}
		if (auxSurfaceBytesRaw)
		{
			free(auxSurfaceBytesRaw);
			auxSurfaceBytesRaw = NULL;
		}
#endif // #ifdef SURFACE_ALLOC_PAGE_NOACCESS

#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) );
#endif
	}

	void* operator new(size_t siz)
	{
		return _mm_malloc(siz, 16);
	}

	void operator delete(void* ptr)
	{
		_mm_free(ptr);
	}

	// Creation functions:
	void CreateOffscreenPlainSurface(UINT _Width, UINT _Height, D3DFORMAT _Format, D3DPOOL _Pool);
	void CreateDepthStencilSurface(UINT _Width, UINT _Height, D3DFORMAT _Format, D3DMULTISAMPLE_TYPE _MultiSample, DWORD _MultisampleQuality, BOOL _Discard);
	void CreateRenderTarget(UINT _Width, UINT _Height, D3DFORMAT _Format, D3DMULTISAMPLE_TYPE _MultiSample, DWORD _MultisampleQuality, BOOL _Lockable);
	void CreateTextureImplicitSurface(UINT _Width, UINT _Height, D3DFORMAT _Format, D3DPOOL _Pool, const DebuggableUsage _Usage, UINT _Level, IDirect3DTexture9Hook* _HookParentTexturePtr);
	void CreateDeviceImplicitSurface(const D3DPRESENT_PARAMETERS& d3dpp);
	void CreateDeviceImplicitDepthStencil(const D3DPRESENT_PARAMETERS& d3dpp);

	void UpdateCachedValuesOnCreate();

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DResource9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetPrivateData(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPrivateData(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE FreePrivateData(THIS_ REFGUID refguid) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE SetPriority(THIS_ DWORD PriorityNew) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE GetPriority(THIS) override;
    virtual COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE PreLoad(THIS) override;
    virtual COM_DECLSPEC_NOTHROW D3DRESOURCETYPE STDMETHODCALLTYPE GetType(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetContainer(THIS_ REFIID riid,void** ppContainer) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDesc(THIS_ D3DSURFACE_DESC *pDesc) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE LockRect(THIS_ D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE UnlockRect(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDC(THIS_ HDC *phdc) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE ReleaseDC(THIS_ HDC hdc) override;

	static const unsigned GetSurfaceSizeBytes(const unsigned Width, const unsigned Height, const D3DFORMAT Format);
	static const unsigned GetSurfacePitchBytes(const unsigned Width, const D3DFORMAT Format);

	const BYTE* const GetSurfaceBytesRaw(void) const
	{
		return surfaceBytesRaw;
	}
	
	const unsigned GetSurfaceBytesSize(void) const
	{
		return surfaceBytesRawSize;
	}

	inline LPDIRECT3DSURFACE9 GetUnderlyingSurface(void) const
	{
		return realObject;
	}

	// Internal use only:
	// Fills a rect on this surface with a color (.rgba). A NULL rectangle means to fill the entire surface.
	void InternalColor4Fill(const D3DXVECTOR4& color, const D3DRECT* const pRect = NULL);

	// Fills a rect on this surface with a color (.rgba). A NULL rectangle means to fill the entire surface.
	void InternalColorFill(const D3DCOLOR color, const D3DRECT* const pRect = NULL);

	// Fills a rect on this surface with a depth value. A NULL rectangle means to fill the entire surface.
	void InternalDepthFill(const float depth, const D3DRECT* const pRect = NULL);

	// Fills a rect on this surface with a stencil value. A NULL rectangle means to fill the entire surface.
	void InternalStencilFill(const DWORD stencil, const D3DRECT* const pRect = NULL);

	void SetPixel(const unsigned x, const unsigned y, const D3DCOLOR color);

	template <const unsigned char writeMask>
	void SetPixelVec(const unsigned x, const unsigned y, const D3DXVECTOR4& color);

	template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
	void SetPixelVec4(const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&color)[4]);

	const D3DCOLOR GetPixel(const unsigned x, const unsigned y) const;

	template <const unsigned char writeMask, const bool sRGBSurface>
	void GetPixelVec(const unsigned x, const unsigned y, D3DXVECTOR4& outColor) const;

	template <const unsigned char writeMask, const bool sRGBSurface>
	void GetPixelVec4(const unsigned (&x4)[4], const unsigned (&y4)[4], D3DXVECTOR4 (&outColor4)[4]) const;

	// Depth functions:
	void SetDepth(const unsigned x, const unsigned y, const float depth);
	void SetDepth4(const __m128i x4, const __m128i y4, const __m128 depth4);

	const float GetDepth(const unsigned x, const unsigned y) const;
	const __m128 GetDepth4(const __m128i x4, const __m128i y4) const;

	const unsigned GetRawDepthValueFromFloatDepth(const float floatDepth) const;

	const unsigned GetRawDepth(const unsigned x, const unsigned y) const;
	const __m128i GetRawDepth4(const __m128i x4, const __m128i y4) const;

	// Stencil functions:
	void SetStencil(const unsigned x, const unsigned y, const DWORD stencil);
	void SetStencil4(const __m128i x4, const __m128i y4, const DWORD stencil4);
	const DWORD GetStencil(const unsigned x, const unsigned y) const;
	const __m128i GetStencil4(const __m128i x4, const __m128i y4) const;

	void UpdateSurfaceInternal(const IDirect3DSurface9Hook* const sourceSurface, const RECT* const sourceRect, const POINT* const destPoint);

	template <const unsigned char writeMask>
	void SampleSurface(const float x, const float y, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;

	template <const unsigned char writeMask>
	void SampleSurface4(const float (&x4)[4], const float (&y4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;

	void DecompressSurfaceDXT1();
	void DecompressSurfaceDXT3();
	void DecompressSurfaceDXT5();
	void DecompressSurfaceToAuxBuffer();

	void DumpSurfaceToDisk(void) const;
	static void InitDumpSurfaces(void);

	inline const D3DFORMAT GetInternalFormat(void) const
	{
		return InternalFormat;
	}

	inline const unsigned GetInternalWidth(void) const
	{
		return InternalWidth;
	}

	inline const unsigned GetInternalHeight(void) const
	{
		return InternalHeight;
	}

	inline const bool IsTexelValid(const unsigned x, const unsigned y) const
	{
		return x < InternalWidth && y < InternalHeight;
	}

#ifdef WITH_SURFACE_HASHING
	inline const surfaceHash& GetSurfaceHash(void) const
	{
		return hash;
	}

	void RecomputeSurfaceHash(void);
	void RecomputePartialSurfaceHash(const DWORD oldValue, const DWORD newValue);

	template <typename T>
	inline void RecomputePartialSurfaceHashXor(const T& value)
	{
		static_assert(sizeof(T) % sizeof(unsigned long) == 0, "Error: Unexpected pixel struct size alignment!");
		static_assert(sizeof(T) / sizeof(unsigned long) > 0, "Error: Unexpected pixel struct size!");

		const unsigned long* const ptr = (const unsigned long* const)&value;

		for (unsigned x = 0; x < sizeof(T) / sizeof(unsigned long); ++x)
			hash.hashVal ^= ptr[x];
	}
#endif

protected:
	template <const unsigned char writeMask, const bool sRGBSurface>
	void SampleSurfaceInternal(const float x, const float y, const D3DTEXTUREFILTERTYPE texf, D3DXVECTOR4& outColor) const;

	template <const unsigned char writeMask, const bool sRGBSurface>
	void SampleSurfaceInternal4(const float (&x4)[4], const float (&y4)[4], const D3DTEXTUREFILTERTYPE texf, D3DXVECTOR4 (&outColor4)[4]) const;

	LPDIRECT3DSURFACE9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

	UINT InternalWidth;
	UINT InternalHeight;
	UINT TextureSurfaceLevel;
	D3DFORMAT InternalFormat;
	D3DPOOL InternalPool;
	DebuggableUsage InternalUsage;
	D3DMULTISAMPLE_TYPE InternalMultiSampleType;
	DWORD InternalMultiSampleQuality;

	BOOL DiscardRT;
	BOOL LockableRT;
	IDirect3DTexture9Hook* HookParentTexturePtr;

#ifdef WITH_SURFACE_HASHING
	surfaceHash hash;
#endif

	enum surfaceCreationMethod
	{
		unknown = 0,
		createOffscreenPlain,
		createDepthStencil,
		createRenderTarget,
		createTexture,
		deviceImplicitBackbuffer,
		deviceImplicitDepthStencil
	} creationMethod;

	BYTE* surfaceBytesRaw;
	BYTE* auxSurfaceBytesRaw; // Aux surface data is nly present for compressed surface types and stencil buffers
	UINT surfaceBytesRawSize;
	UINT auxSurfaceBytesRawSize;

	__declspec(align(16) ) __m128i InternalWidthSplatted; // This is in the format of (uint32[4])(InternalWidth, InternalWidth, InternalWidth, InternalWidth)
	UINT InternalWidthM1;
	UINT InternalHeightM1;
	float InternalWidthM1F;
	float InternalHeightM1F;
	bool is1x1surface;
};
