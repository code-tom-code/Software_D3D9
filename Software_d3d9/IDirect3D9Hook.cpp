#pragma once

#include "IDirect3D9Hook.h"

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj) 
{
	HRESULT ret = d3d9->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3D9Hook::AddRef(THIS) 
{
	ULONG ret = d3d9->AddRef();
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3D9Hook::Release(THIS) 
{
	ULONG ret = d3d9->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked D3D9 %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::RegisterSoftwareDevice(THIS_ void* pInitializeFunction) 
{
	HRESULT ret = d3d9->RegisterSoftwareDevice(pInitializeFunction);
	return ret;
}

COM_DECLSPEC_NOTHROW UINT STDMETHODCALLTYPE IDirect3D9Hook::GetAdapterCount(THIS)
{
	UINT ret = d3d9->GetAdapterCount();
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::GetAdapterIdentifier(THIS_ UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) 
{
	HRESULT ret = d3d9->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
	return ret;
}

COM_DECLSPEC_NOTHROW UINT STDMETHODCALLTYPE IDirect3D9Hook::GetAdapterModeCount(THIS_ UINT Adapter, D3DFORMAT Format)
{
	UINT ret = d3d9->GetAdapterModeCount(Adapter, Format);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::EnumAdapterModes(THIS_ UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode)
{
	HRESULT ret = d3d9->EnumAdapterModes(Adapter, Format, Mode, pMode);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::GetAdapterDisplayMode(THIS_ UINT Adapter, D3DDISPLAYMODE* pMode)
{
	HRESULT ret = d3d9->GetAdapterDisplayMode(Adapter, pMode);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::CheckDeviceType(THIS_ UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed)
{
	HRESULT ret = d3d9->CheckDeviceType(Adapter, DevType, AdapterFormat, BackBufferFormat, bWindowed);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::CheckDeviceFormat(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
{
	HRESULT ret = d3d9->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::CheckDeviceMultiSampleType(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels)
{
	HRESULT ret = d3d9->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::CheckDepthStencilMatch(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)
{
	HRESULT ret = d3d9->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::CheckDeviceFormatConversion(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat)
{
	HRESULT ret = d3d9->CheckDeviceFormatConversion(Adapter, DeviceType, SourceFormat, TargetFormat);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::GetDeviceCaps(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps)
{
	HRESULT ret = d3d9->GetDeviceCaps(Adapter, DeviceType, pCaps);
	if (FAILED(ret) )
		return ret;

	if (pCaps && DeviceType == D3DDEVTYPE_HAL)
	{
		IDirect3DDevice9Hook::ModifyDeviceCaps(*pCaps);
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HMONITOR STDMETHODCALLTYPE IDirect3D9Hook::GetAdapterMonitor(THIS_ UINT Adapter)
{
	HMONITOR ret = d3d9->GetAdapterMonitor(Adapter);
	return ret;
}

static inline const char* const GetBackbufferFormatString(const D3DFORMAT fmt)
{
	// These are the valid backbuffer surface formats for D3D9
	switch (fmt)
	{
	case D3DFMT_A2R10G10B10:
		return "D3DFMT_A2R10G10B10";
	case D3DFMT_A8R8G8B8:
		return "D3DFMT_A8R8G8B8";
	case D3DFMT_X8R8G8B8:
		return "D3DFMT_X8R8G8B8";
	case D3DFMT_A1R5G5B5:
		return "D3DFMT_A1R5G5B5";
	case D3DFMT_X1R5G5B5:
		return "D3DFMT_X1R5G5B5";
	case D3DFMT_R5G6B5:
		return "D3DFMT_R5G6B5";
	default:
		return "Unknown";
	}
}

static inline const char* const GetDepthbufferFormatString(const D3DFORMAT fmt)
{
	switch (fmt)
	{
	case D3DFMT_D16_LOCKABLE:
		return "D3DFMT_D16_LOCKABLE";
	case D3DFMT_D32:
		return "D3DFMT_D32";
	case D3DFMT_D15S1:
		return "D3DFMT_D15S1";
	case D3DFMT_D24S8:
		return "D3DFMT_D24S8";
	case D3DFMT_D24X8:
		return "D3DFMT_D24X8";
	case D3DFMT_D24X4S4:
		return "D3DFMT_D24X4S4";
	case D3DFMT_D32F_LOCKABLE:
		return "D3DFMT_D32F_LOCKABLE";
	case D3DFMT_D24FS8:
		return "D3DFMT_D24FS8";
	case D3DFMT_D32_LOCKABLE:
		return "D3DFMT_D32_LOCKABLE";
	case D3DFMT_S8_LOCKABLE:
		return "D3DFMT_S8_LOCKABLE";
	case D3DFMT_D16:
		return "D3DFMT_D16";
	default:
		return "Unknown";
	}
}

static inline const char* const GetDeviceTypeString(const D3DDEVTYPE DeviceType)
{
	switch (DeviceType)
	{
	case D3DDEVTYPE_HAL:
		return "D3DDEVTYPE_HAL";
	case D3DDEVTYPE_REF:
		return "D3DDEVTYPE_REF";
	case D3DDEVTYPE_SW:
		return "D3DDEVTYPE_SW";
	case D3DDEVTYPE_NULLREF:
		return "D3DDEVTYPE_NULLREF";
	default:
		return "Unknown";
	}
}

static const DWORD BehaviorCreateFlags[] =
{
	D3DCREATE_ADAPTERGROUP_DEVICE,
	D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
	D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX,
	D3DCREATE_DISABLE_PRINTSCREEN,
	D3DCREATE_DISABLE_PSGP_THREADING,
	D3DCREATE_ENABLE_PRESENTSTATS,
	D3DCREATE_FPU_PRESERVE,
	D3DCREATE_HARDWARE_VERTEXPROCESSING,
	D3DCREATE_MIXED_VERTEXPROCESSING,
	D3DCREATE_SOFTWARE_VERTEXPROCESSING,
	D3DCREATE_MULTITHREADED,
	D3DCREATE_NOWINDOWCHANGES,
	D3DCREATE_PUREDEVICE,
	D3DCREATE_SCREENSAVER
};

static const char* const BehaviorCreateFlagStrings[] =
{
	"D3DCREATE_ADAPTERGROUP_DEVICE",
	"D3DCREATE_DISABLE_DRIVER_MANAGEMENT",
	"D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX",
	"D3DCREATE_DISABLE_PRINTSCREEN",
	"D3DCREATE_DISABLE_PSGP_THREADING",
	"D3DCREATE_ENABLE_PRESENTSTATS",
	"D3DCREATE_FPU_PRESERVE",
	"D3DCREATE_HARDWARE_VERTEXPROCESSING",
	"D3DCREATE_MIXED_VERTEXPROCESSING",
	"D3DCREATE_SOFTWARE_VERTEXPROCESSING",
	"D3DCREATE_MULTITHREADED",
	"D3DCREATE_NOWINDOWCHANGES",
	"D3DCREATE_PUREDEVICE",
	"D3DCREATE_SCREENSAVER"
};

static inline const char* const StaticGetDeviceBehaviorFlagsString(const DWORD BehaviorFlags)
{
	static char staticBuffer[512] = {0};
	bool hasBehaviorFlags = false;
	for (unsigned x = 0; x < ARRAYSIZE(BehaviorCreateFlags); ++x)
	{
		if (BehaviorFlags & BehaviorCreateFlags[x])
		{
#pragma warning(push)
#pragma warning(disable:4996)
			if (hasBehaviorFlags)
				strcat(staticBuffer, " | ");
			strcat(staticBuffer, BehaviorCreateFlagStrings[x]);
#pragma warning(pop)
			hasBehaviorFlags = true;
		}
	}
	return staticBuffer;
}

static inline const char* const SwapEffectToString(const D3DSWAPEFFECT SwapEffect)
{
	switch (SwapEffect)
	{
	case D3DSWAPEFFECT_DISCARD:
		return "D3DSWAPEFFECT_DISCARD";
	case D3DSWAPEFFECT_FLIP:
		return "D3DSWAPEFFECT_FLIP";
	case D3DSWAPEFFECT_COPY:
		return "D3DSWAPEFFECT_COPY";
	case D3DSWAPEFFECT_OVERLAY:
		return "D3DSWAPEFFECT_OVERLAY";
	case D3DSWAPEFFECT_FLIPEX:
		return "D3DSWAPEFFECT_FLIPEX";
	default:
		return "Unknown";
	}
}

static const DWORD PresentFlags[] =
{
	D3DPRESENTFLAG_LOCKABLE_BACKBUFFER,
	D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL,
	D3DPRESENTFLAG_DEVICECLIP,
	D3DPRESENTFLAG_VIDEO,
	D3DPRESENTFLAG_NOAUTOROTATE,
	D3DPRESENTFLAG_UNPRUNEDMODE,
	D3DPRESENTFLAG_OVERLAY_LIMITEDRGB,
	D3DPRESENTFLAG_OVERLAY_YCbCr_BT709,
	D3DPRESENTFLAG_OVERLAY_YCbCr_xvYCC,
	D3DPRESENTFLAG_RESTRICTED_CONTENT,
	D3DPRESENTFLAG_RESTRICT_SHARED_RESOURCE_DRIVER
};

static const char* const PresentFlagStrings[] =
{
	"D3DPRESENTFLAG_LOCKABLE_BACKBUFFER",
	"D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL",
	"D3DPRESENTFLAG_DEVICECLIP",
	"D3DPRESENTFLAG_VIDEO",
	"D3DPRESENTFLAG_NOAUTOROTATE",
	"D3DPRESENTFLAG_UNPRUNEDMODE",
	"D3DPRESENTFLAG_OVERLAY_LIMITEDRGB",
	"D3DPRESENTFLAG_OVERLAY_YCbCr_BT709",
	"D3DPRESENTFLAG_OVERLAY_YCbCr_xvYCC",
	"D3DPRESENTFLAG_RESTRICTED_CONTENT",
	"D3DPRESENTFLAG_RESTRICT_SHARED_RESOURCE_DRIVER"
};

static inline const char* const StaticGetPresentFlagsString(const DWORD Flagss)
{
	static char staticBuffer[512] = {0};
	bool hasPresentFlags = false;
	for (unsigned x = 0; x < ARRAYSIZE(PresentFlags); ++x)
	{
		if (Flagss & PresentFlags[x])
		{
#pragma warning(push)
#pragma warning(disable:4996)
			if (hasPresentFlags)
				strcat(staticBuffer, " | ");
			strcat(staticBuffer, PresentFlagStrings[x]);
#pragma warning(pop)
			hasPresentFlags = true;
		}
	}
	return staticBuffer;
}

static inline const char* const GetPresentationIntervalString(const UINT PresentInterval)
{
	switch (PresentInterval)
	{
	case D3DPRESENT_INTERVAL_DEFAULT:
		return "D3DPRESENT_INTERVAL_DEFAULT";
	case D3DPRESENT_INTERVAL_ONE:
		return "D3DPRESENT_INTERVAL_ONE";
	case D3DPRESENT_INTERVAL_TWO:
		return "D3DPRESENT_INTERVAL_TWO";
	case D3DPRESENT_INTERVAL_THREE:
		return "D3DPRESENT_INTERVAL_THREE";
	case D3DPRESENT_INTERVAL_FOUR:
		return "D3DPRESENT_INTERVAL_FOUR";
	case D3DPRESENT_INTERVAL_IMMEDIATE:
		return "D3DPRESENT_INTERVAL_IMMEDIATE";
	default:
		return "Unknown";
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3D9Hook::CreateDevice(THIS_ UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, 
		DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
	D3DPRESENT_PARAMETERS modifiedParams = *pPresentationParameters;
	IDirect3DDevice9Hook::ModifyPresentParameters(modifiedParams);

#ifdef _DEBUG
	if (!(BehaviorFlags & (D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MIXED_VERTEXPROCESSING) ) )
	{
		// At least one of Hardware, Software, or Mixed must be specified on device-creation!
		DbgBreakPrint("Error: At least one of Hardware, Software, or Mixed must be specified at device-creation time!");
	}
	if (__popcnt(BehaviorFlags & (D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MIXED_VERTEXPROCESSING) ) > 1)
	{
		// Only one of Hardware, Software, or Mixed is allowed to be specified
		DbgBreakPrint("Error: Only one of Hardware, Software, or Mixed must be specified at device-creation time!");
	}
#endif

	LPDIRECT3DDEVICE9 realDevice = NULL;
	HRESULT ret = d3d9->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags, &modifiedParams, &realDevice);
	if (FAILED(ret) || !realDevice)
	{
		*ppReturnedDeviceInterface = NULL;
		MessageBoxA(NULL, "Failed createdevice", NULL, MB_OK);
		return ret;
	}

	{
#pragma warning(push)
#pragma warning(disable:4996)
		char adapterString[32] = {0};
		if (Adapter == D3DADAPTER_DEFAULT)
			strcpy(adapterString, "D3DADAPTER_DEFAULT (0)");
		else
			sprintf(adapterString, "%u", Adapter);
		char createDeviceSettings[4096] = {0};
		sprintf(createDeviceSettings, "CreateDevice params:\n\tAdapter = %s\n\tDeviceType = %s\n\tBehaviorFlags = 0x%08X (%s)\n\tppBackBufferWidth = %u\n\tppBackBufferHeight = %u\n\tppBackBufferFormat = %s(%i)\n\tppBackBufferCount = %u\n\tppMultiSampleType = %u\n\tppMultiSampleQuality = %u\n\tppSwapEffect = %s\n\tppWindowed = %s\n\tppEnableAutoDepthStencil = %s\n\tppAutoDepthStencilFormat = %s (%i)\n\tppFlags = 0x%08X (%s)\n\tppFullScreen_RefreshRateInHz = %u\n\tppPresentationInterval = %s (%u)\n\n",
			adapterString, GetDeviceTypeString(DeviceType), BehaviorFlags, StaticGetDeviceBehaviorFlagsString(BehaviorFlags), modifiedParams.BackBufferWidth, modifiedParams.BackBufferHeight, GetBackbufferFormatString(modifiedParams.BackBufferFormat), modifiedParams.BackBufferFormat, modifiedParams.BackBufferCount, modifiedParams.MultiSampleType, modifiedParams.MultiSampleQuality, SwapEffectToString(modifiedParams.SwapEffect), modifiedParams.Windowed ? "TRUE" : "FALSE", modifiedParams.EnableAutoDepthStencil ? "TRUE" : "FALSE", GetDepthbufferFormatString(modifiedParams.AutoDepthStencilFormat), modifiedParams.AutoDepthStencilFormat, modifiedParams.Flags, StaticGetPresentFlagsString(modifiedParams.Flags), modifiedParams.FullScreen_RefreshRateInHz, GetPresentationIntervalString(modifiedParams.PresentationInterval), modifiedParams.PresentationInterval);
#pragma warning(pop)
		OutputDebugStringA(createDeviceSettings);
		MessageBoxA(NULL, createDeviceSettings, "CreateDevice success", NULL);
	}

	void* const alignedAlloc = _aligned_malloc(sizeof(IDirect3DDevice9Hook), 16);
	IDirect3DDevice9Hook* newHookDevice = new (alignedAlloc) IDirect3DDevice9Hook(realDevice, this);
	*ppReturnedDeviceInterface = newHookDevice;
	newHookDevice->InitializeState(modifiedParams, DeviceType, BehaviorFlags, hFocusWindow);

	return ret;
}
