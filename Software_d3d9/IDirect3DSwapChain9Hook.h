#pragma once

#include "IDirect3DDevice9Hook.h"

class IDirect3DSwapChain9Hook : public IDirect3DSwapChain9
{
public:
	IDirect3DSwapChain9Hook(LPDIRECT3DSWAPCHAIN9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1), backBuffer(NULL), tempBlitSurface(NULL)
	{
#ifdef _DEBUG
		memcpy(&PresentParameters, &realObject->PresentParameters, (char*)&realObject - (char*)&PresentParameters);
#endif

		// Init the gamma ramp to its default value:
		InitDefaultGammaRamp();
	}

	virtual ~IDirect3DSwapChain9Hook()
	{
#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) );
#endif
	}

	void InitializeSwapChain(const D3DPRESENT_PARAMETERS& _PresentParameters, IDirect3DSurface9Hook* swapChainBackBuffer);

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DSwapChain9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Present(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetFrontBufferData(THIS_ IDirect3DSurface9* pDestSurface) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetBackBuffer(THIS_ UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetRasterStatus(THIS_ D3DRASTER_STATUS* pRasterStatus) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDisplayMode(THIS_ D3DDISPLAYMODE* pMode) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPresentParameters(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) override;

	inline LPDIRECT3DSWAPCHAIN9 GetUnderlyingSwapChain(void) const
	{
		return realObject;
	}

	void SetGammaRamp(DWORD Flags, CONST D3DGAMMARAMP* pRamp);
	void GetGammaRamp(D3DGAMMARAMP* pRamp);

	void InitDefaultGammaRamp(void);

	void InitBlitSurface(void);

	const D3DDISPLAYMODE& GetInternalDisplayMode(void) const
	{
		return InternalDisplayMode;
	}

	IDirect3DSurface9Hook* const GetInternalBackBuffer(void)
	{
		return backBuffer;
	}

protected:
	LPDIRECT3DSWAPCHAIN9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

	// Blit from the software backbuffer to the hardware backbuffer:
	void InternalBlit(void);

	IDirect3DSurface9Hook* backBuffer;
	D3DPRESENT_PARAMETERS InternalPresentParameters;
	D3DDISPLAYMODE InternalDisplayMode;
	D3DGAMMARAMP gammaRamp;
	LPDIRECT3DSURFACE9 tempBlitSurface;
};
