#pragma once

#include "d3d9include.h"

#include <stddef.h> // for offsetof

#include "IDirect3DDevice9Hook.h"

class IDirect3D9Hook : public IDirect3D9
{
public:
	IDirect3D9Hook(LPDIRECT3D9 _d3d9) : d3d9(_d3d9), refCount(1)
	{
#ifdef _DEBUG
		memcpy(&Version, &d3d9->Version, (char*)&d3d9 - (char*)&Version);
#endif
	}

	inline LPDIRECT3D9 GetUnderlyingD3D9(void) const
	{
		return d3d9;
	}

	virtual ~IDirect3D9Hook()
	{
		d3d9 = NULL;
		refCount = 0;
#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) );
#endif
	}

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3D9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(THIS_ void* pInitializeFunction) override;
    virtual COM_DECLSPEC_NOTHROW UINT STDMETHODCALLTYPE GetAdapterCount(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(THIS_ UINT Adapter,DWORD Flags,D3DADAPTER_IDENTIFIER9* pIdentifier) override;
    virtual COM_DECLSPEC_NOTHROW UINT STDMETHODCALLTYPE GetAdapterModeCount(THIS_ UINT Adapter,D3DFORMAT Format) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE EnumAdapterModes(THIS_ UINT Adapter,D3DFORMAT Format,UINT Mode,D3DDISPLAYMODE* pMode) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(THIS_ UINT Adapter,D3DDISPLAYMODE* pMode) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CheckDeviceType(THIS_ UINT Adapter,D3DDEVTYPE DevType,D3DFORMAT AdapterFormat,D3DFORMAT BackBufferFormat,BOOL bWindowed) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CheckDeviceFormat(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,DWORD Usage,D3DRESOURCETYPE RType,D3DFORMAT CheckFormat) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SurfaceFormat,BOOL Windowed,D3DMULTISAMPLE_TYPE MultiSampleType,DWORD* pQualityLevels) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,D3DFORMAT RenderTargetFormat,D3DFORMAT DepthStencilFormat) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SourceFormat,D3DFORMAT TargetFormat) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDeviceCaps(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DCAPS9* pCaps) override;
    virtual COM_DECLSPEC_NOTHROW HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(THIS_ UINT Adapter) override;

	virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateDevice(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, 
		DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) override;

protected:
	LPDIRECT3D9 d3d9;
	unsigned __int64 refCount;
};
