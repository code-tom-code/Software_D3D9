#pragma once

#include "IDirect3DDevice9Hook.h"

class IDirect3DStateBlock9Hook : public IDirect3DStateBlock9
{
public:
	IDirect3DStateBlock9Hook(LPDIRECT3DSTATEBLOCK9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1)
	{
#ifdef _DEBUG
		CreationCallStack = realObject->CreationCallStack;
#endif
	}

	virtual ~IDirect3DStateBlock9Hook()
	{
#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) );
#endif
	}

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DStateBlock9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Capture(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Apply(THIS) override;

protected:
	LPDIRECT3DSTATEBLOCK9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;
};
