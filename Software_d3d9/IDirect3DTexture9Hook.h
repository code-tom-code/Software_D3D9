#pragma once

#include "IDirect3DDevice9Hook.h"
#include "GlobalToggles.h"
struct SamplerState;

class IDirect3DTexture9Hook : public IDirect3DTexture9
{
public:
	IDirect3DTexture9Hook(LPDIRECT3DTEXTURE9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1), AutoGenFilter(D3DTEXF_LINEAR), 
		surfaceCountMinusOne(0), surfaceCountMinusTwo(0), surfaceCountMinusOneF(0.0f), surfaceLevel0(NULL)
#ifdef DUMP_TEXTURES_ON_FIRST_SET
		, dumped(0)
#endif
	{
#ifdef _DEBUG
		memcpy(&Name, &realObject->Name, (char*)&realObject - (char*)&Name);
#endif
	}

	virtual ~IDirect3DTexture9Hook()
	{
#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) - sizeof(surfaces) );
#endif
	}

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DBaseTexture9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetPrivateData(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPrivateData(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE FreePrivateData(THIS_ REFGUID refguid) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE SetPriority(THIS_ DWORD PriorityNew) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE GetPriority(THIS) override;
    virtual COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE PreLoad(THIS) override;
    virtual COM_DECLSPEC_NOTHROW D3DRESOURCETYPE STDMETHODCALLTYPE GetType(THIS) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE SetLOD(THIS_ DWORD LODNew) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE GetLOD(THIS) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE GetLevelCount(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(THIS_ D3DTEXTUREFILTERTYPE FilterType) override;
    virtual COM_DECLSPEC_NOTHROW D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType(THIS) override;
    virtual COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE GenerateMipSubLevels(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetLevelDesc(THIS_ UINT Level,D3DSURFACE_DESC *pDesc) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetSurfaceLevel(THIS_ UINT Level,IDirect3DSurface9** ppSurfaceLevel) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE LockRect(THIS_ UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE UnlockRect(THIS_ UINT Level) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE AddDirtyRect(THIS_ CONST RECT* pDirtyRect) override;

	inline LPDIRECT3DTEXTURE9 GetUnderlyingTexture(void) const
	{
		return realObject;
	}

	inline const UINT GetInternalWidth() const
	{
		return InternalWidth;
	}

	inline const UINT GetInternalHeight() const
	{
		return InternalHeight;
	}

	inline const D3DFORMAT GetInternalFormat() const
	{
		return InternalFormat;
	}

	const UINT GetInternalMipLevels() const
	{
		return InternalLevels;
	}

	const DebuggableUsage GetInternalUsage() const
	{
		return InternalUsage;
	}

	const D3DPOOL GetInternalPool() const
	{
		return InternalPool;
	}

	void CreateTexture(const UINT _Width, const UINT _Height, const UINT _Levels, const DebuggableUsage _Usage, const D3DFORMAT _Format, const D3DPOOL _Pool);

	const bool UpdateTextureInternal(const IDirect3DTexture9Hook* const sourceTexture);

	template <const unsigned char writeMask>
	void SampleTextureLoD(float x, float y, float mip, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;

	template <const unsigned char writeMask>
	void SampleTextureGrad(float x, float y, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;

	template <const unsigned char writeMask>
	void SampleTextureGradBias(float x, float y, const float mipBias, const D3DXVECTOR4& texDdx, const D3DXVECTOR4& texDdy, const SamplerState& samplerState, D3DXVECTOR4& outColor) const;

	template <const unsigned char writeMask>
	void SampleTextureLoD4(float (&x4)[4], float (&y4)[4], float (&mip4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;

	template <const unsigned char writeMask>
	void SampleTextureGrad4(float (&x4)[4], float (&y4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;

	template <const unsigned char writeMask>
	void SampleTextureGradBias4(float (&x4)[4], float (&y4)[4], const float (&mipBias4)[4], const D3DXVECTOR4 (&texDdx4)[4], const D3DXVECTOR4 (&texDdy4)[4], const SamplerState& samplerState, D3DXVECTOR4 (&outColor4)[4]) const;

#ifdef DUMP_TEXTURES_ON_FIRST_SET
	unsigned dumped;
#endif

	inline const std::vector<IDirect3DSurface9Hook*>& GetUnderlyingSurfaces(void) const
	{
		return surfaces;
	}

protected:
	LPDIRECT3DTEXTURE9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

	UINT InternalWidth;
	UINT InternalHeight;
	UINT InternalLevels;
	DebuggableUsage InternalUsage;
	D3DFORMAT InternalFormat;
	D3DPOOL InternalPool;
	D3DTEXTUREFILTERTYPE AutoGenFilter;

	UINT surfaceCountMinusOne;
	INT surfaceCountMinusTwo;
	float surfaceCountMinusOneF;

	IDirect3DSurface9Hook* surfaceLevel0;

	std::vector<IDirect3DSurface9Hook*> surfaces;
};
