#pragma once

#include "IDirect3DDevice9Hook.h"

class IDirect3DCubeTexture9Hook : public IDirect3DCubeTexture9
{
public:
	IDirect3DCubeTexture9Hook(LPDIRECT3DCUBETEXTURE9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1)
	{
#ifdef _DEBUG
		memcpy(&Name, &realObject->Name, (char*)&realObject - (char*)&Name);
#endif
	}

	virtual ~IDirect3DCubeTexture9Hook()
	{
#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) );
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
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetCubeMapSurface(THIS_ D3DCUBEMAP_FACES FaceType,UINT Level,IDirect3DSurface9** ppCubeMapSurface) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE LockRect(THIS_ D3DCUBEMAP_FACES FaceType,UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE UnlockRect(THIS_ D3DCUBEMAP_FACES FaceType,UINT Level) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE AddDirtyRect(THIS_ D3DCUBEMAP_FACES FaceType,CONST RECT* pDirtyRect) override;

	inline LPDIRECT3DCUBETEXTURE9 GetUnderlyingCubeTexture(void) const
	{
		return realObject;
	}

	inline const UINT GetInternalEdgeLength() const
	{
		return InternalEdgeLength;
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

	void CreateCubeTexture(UINT _EdgeLength, UINT _Levels, DebuggableUsage _Usage, D3DFORMAT _Format, D3DPOOL _Pool);

protected:
	LPDIRECT3DCUBETEXTURE9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

	UINT InternalEdgeLength;
	UINT InternalLevels;
	DebuggableUsage InternalUsage;
	D3DFORMAT InternalFormat;
	D3DPOOL InternalPool;
};
