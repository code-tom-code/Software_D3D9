#pragma once

#include "IDirect3DDevice9Hook.h"

class IDirect3DVolumeTexture9Hook : public IDirect3DVolumeTexture9
{
public:
	IDirect3DVolumeTexture9Hook(LPDIRECT3DVOLUMETEXTURE9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1)
	{
#ifdef _DEBUG
		memcpy(&Name, &realObject->Name, (char*)&realObject - (char*)&Name);
#endif
	}

	virtual ~IDirect3DVolumeTexture9Hook()
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
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetLevelDesc(THIS_ UINT Level,D3DVOLUME_DESC *pDesc) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetVolumeLevel(THIS_ UINT Level,IDirect3DVolume9** ppVolumeLevel) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE LockBox(THIS_ UINT Level,D3DLOCKED_BOX* pLockedVolume,CONST D3DBOX* pBox,DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE UnlockBox(THIS_ UINT Level) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE AddDirtyBox(THIS_ CONST D3DBOX* pDirtyBox) override;

	inline LPDIRECT3DVOLUMETEXTURE9 GetUnderlyingVolumeTexture(void) const
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

	inline const UINT GetInternalDepth() const
	{
		return InternalDepth;
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

	void CreateVolumeTexture(UINT _Width, UINT _Height, UINT _Depth, UINT _Levels, DebuggableUsage _Usage, D3DFORMAT _Format, D3DPOOL _Pool);

protected:
	LPDIRECT3DVOLUMETEXTURE9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

	UINT InternalWidth;
	UINT InternalHeight;
	UINT InternalDepth;
	UINT InternalLevels;
	DebuggableUsage InternalUsage;
	D3DFORMAT InternalFormat;
	D3DPOOL InternalPool;
};
