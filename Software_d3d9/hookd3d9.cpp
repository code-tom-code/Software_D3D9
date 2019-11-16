#pragma once

#undef UNICODE
#undef _UNICODE

#include <windows.h>
#pragma pack(1)

#include "d3d9include.h"

#include "IDirect3D9Hook.h"

#ifdef INCREASE_SYSTEM_SCHEDULER_RESOLUTION
	#pragma comment(lib, "Winmm.lib")
#endif // #ifdef INCREASE_SYSTEM_SCHEDULER_RESOLUTION

HINSTANCE hLThisDLL = 0;
HINSTANCE hL = 0;

// Undocumented enum!
enum Force9On12Mode : UINT
{
	Force9On12Mode_D3D9_Default = 0, // This will create a true D3D9 device (no 9On12 layer)
	Force9On12Mode_D3D9On12 = 1, // This will create a D3D9On12 device
	Force9On12Mode_D3D9_Unknown = 2 // I am not sure what this does, but it seems to act almost exactly like Force9On12Mode_D3D9_Default (value "0")
};

#define MAX_D3D9ON12_QUEUES        2

typedef struct _D3D9ON12_ARGS
{
    BOOL Enable9On12;
    IUnknown* pD3D12Device;
    IUnknown* ppD3D12Queues[MAX_D3D9ON12_QUEUES];
    UINT NumQueues;
    UINT NodeMask;
} D3D9ON12_ARGS;

// Undocumented enum
typedef enum _D3DSVERROR_ID
{
} D3DSVERROR_ID;

typedef HRESULT (CALLBACK *IDirect3DShaderValidator9_InstructionCallback)(LPCSTR, UINT messageType, D3DSVERROR_ID messageID, UINT, LPCSTR lpMessage, LPVOID lParam);

// This is an undocumented interface returned from the exported function Direct3DShaderValidatorCreate9()
struct DECLSPEC_NOVTABLE IDirect3DShaderValidator9 : public IUnknown
{
	/*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

	/*** IDirect3DShaderValidator9 methods ***/
	STDMETHOD(Begin)(THIS_ IDirect3DShaderValidator9_InstructionCallback lpCallbackFunc, LPVOID lParam, DWORD unknown) PURE;
	STDMETHOD(Instruction)(THIS_ CONST char* unknownString, UINT unknown, const DWORD* pdwInst /*Pointer to the instruction token*/, DWORD dwCount /*The instruction length, in DWORD tokens*/) PURE;
	STDMETHOD(End)(THIS) PURE;
};

typedef int (WINAPI *D3DPERF_BeginEventType)(D3DCOLOR col, LPCWSTR wszName);
typedef int (WINAPI *D3DPERF_EndEventType)(void);
typedef DWORD (WINAPI *D3DPERF_GetStatusType)(void);
typedef BOOL (WINAPI *D3DPERF_QueryRepeatFrameType)(void);
typedef void (WINAPI *D3DPERF_SetMarkerType)(D3DCOLOR col, LPCWSTR wszName);
typedef void (WINAPI *D3DPERF_SetOptionsType)(DWORD dwOptions);
typedef void (WINAPI *D3DPERF_SetRegionType)(D3DCOLOR col, LPCWSTR wszName);
typedef void (WINAPI *DebugSetLevelType)(void);
typedef void (WINAPI *DebugSetMuteType)(void);
typedef HRESULT (WINAPI *Direct3D9EnableMaximizedWindowedModeShimType)(BOOL ShimEnable);
typedef IDirect3D9* (WINAPI *Direct3DCreate9Type)(_In_ UINT SDKVersion);
typedef HRESULT (WINAPI *Direct3DCreate9ExType)(_In_ UINT SDKVersion, _Out_ IDirect3D9Ex**);
typedef IDirect3DShaderValidator9* (WINAPI *Direct3DShaderValidatorCreate9Type)(void);
typedef void (WINAPI *PSGPErrorType)(class D3DFE_PROCESSVERTICES* verticesPtr, enum PSGPERRORID errID, UINT UnknownUInt);
typedef void (WINAPI *PSGPSampleTextureType)(class D3DFE_PROCESSVERTICES* verticesPtr, UINT UnknownUInt1, float (*const UnknownFloats1)[4], unsigned int UnknownUInt2, float (*const UnknownFloats2)[4]);
typedef void (WINAPI *Direct3D9ForceHybridEnumerationType)(_In_ BOOL ForceHybridEnumeration);
typedef void (WINAPI *Direct3D9SetMaximizedWindowedModeShimType)(_In_ BOOL unknown0, _In_ BOOL unknown1);
typedef INT (WINAPI *Direct3D9SetSwapEffectUpgradeShimType)(_In_ BOOL ShimEnable);
typedef void (WINAPI *Direct3D9Force9On12Type)(_In_ Force9On12Mode Mode);
typedef IDirect3D9* (WINAPI *Direct3DCreate9On12Type)(_In_ UINT SDKVersion, _In_ D3D9ON12_ARGS* pOverrideList, _In_ UINT NumOverrideEntries);
typedef HRESULT (WINAPI *Direct3DCreate9On12ExType)(_In_ UINT SDKVersion, _In_ D3D9ON12_ARGS* pOverrideList, UINT NumOverrideEntries, _Inout_ IDirect3D9Ex** ppOutputInterface);
typedef void (WINAPI *Direct3D9SetMaximizedWindowHwndOverrideType)(_In_ BOOL Override);
typedef void (WINAPI *Direct3D9SetVendorIDLieFor9On12Type)(_In_ BOOL VendorIDLie);

// All of these function pointer values will be pulled from the real d3d9.dll using GetProcAddress().
// Not all of these functions may exist if we are running versions of Windows older than Windows 10, so in
// those cases we'll just ignore the function calls entirely if possible.
static D3DPERF_BeginEventType Real_D3DPERF_BeginEvent = NULL;
static D3DPERF_EndEventType Real_D3DPERF_EndEvent = NULL;
static D3DPERF_GetStatusType Real_D3DPERF_GetStatus = NULL;
static D3DPERF_QueryRepeatFrameType Real_D3DPERF_QueryRepeatFrame = NULL;
static D3DPERF_SetMarkerType Real_D3DPERF_SetMarker = NULL;
static D3DPERF_SetOptionsType Real_D3DPERF_SetOptions = NULL;
static D3DPERF_SetRegionType Real_D3DPERF_SetRegion = NULL;
static DebugSetLevelType Real_DebugSetLevel = NULL;
static DebugSetMuteType Real_DebugSetMute = NULL;
static Direct3D9EnableMaximizedWindowedModeShimType Real_Direct3D9EnableMaximizedWindowedModeShim = NULL;
static Direct3DCreate9Type Real_Direct3DCreate9 = NULL;
static Direct3DCreate9ExType Real_Direct3DCreate9Ex = NULL;
static Direct3DShaderValidatorCreate9Type Real_Direct3DShaderValidatorCreate9 = NULL;
static PSGPErrorType Real_PSGPError = NULL;
static PSGPSampleTextureType Real_PSGPSampleTexture = NULL;
static Direct3D9ForceHybridEnumerationType Real_Direct3D9ForceHybridEnumeration = NULL;
static Direct3D9SetMaximizedWindowedModeShimType Real_Direct3D9SetMaximizedWindowedModeShim = NULL;
static Direct3D9SetSwapEffectUpgradeShimType Real_Direct3D9SetSwapEffectUpgradeShim = NULL;
static Direct3D9Force9On12Type Real_Direct3D9Force9On12 = NULL;
static Direct3DCreate9On12Type Real_Direct3DCreate9On12 = NULL;
static Direct3DCreate9On12ExType Real_Direct3DCreate9On12Ex = NULL;
static Direct3D9SetMaximizedWindowHwndOverrideType Real_Direct3D9SetMaximizedWindowHwndOverride = NULL;
static Direct3D9SetVendorIDLieFor9On12Type Real_Direct3D9SetVendorIDLieFor9On12 = NULL;

#ifdef _DEBUG

static unsigned char* NtSetInformationThreadFuncBytes = NULL;
static unsigned char* NtSetInformationThreadPostHook = NULL;

// Thanks to NT Internals for this one: http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FThread%2FNtSetInformationThread.html
typedef enum _REAL_THREAD_INFORMATION_CLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger
} REAL_THREAD_INFORMATION_CLASS, *PREAL_THREAD_INFORMATION_CLASS;
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

#ifdef _M_IX86
extern "C" __declspec(naked) void __stdcall NtSetInformationThreadHook(IN HANDLE ThreadHandle, IN REAL_THREAD_INFORMATION_CLASS ThreadInformationClass, IN PVOID ThreadInformation, IN ULONG ThreadInformationLength)
{
	// Uhhhhh, have to do this because for some reason C++ thinks that our parameters are each slid down by 1 (ThreadHandle is really ThreadInformationClass and ThreadInformation is really ThreadInformationLength, etc.)
	if ( (REAL_THREAD_INFORMATION_CLASS)(int)(void*)ThreadHandle == ThreadHideFromDebugger)
	{
		__asm
		{
			xor eax, eax
			ret 10h // return 0 (NTSUCCESS)
		}
	}

	__asm
	{
		mov eax, 0x0D
		jmp [NtSetInformationThreadPostHook]
	}
}

static inline void HookIsDebuggerPresent()
{
	static unsigned char* IsDebuggerPresentFuncBytes = NULL;

	// We already hooked it!
	if (IsDebuggerPresentFuncBytes != NULL)
		return;

	// KernelBase.dll in Windows 8 and up, kernel32.dll in lower versions of Windows
	HMODULE kerneldll = LoadLibraryA("KernelBase.dll");
	if (!kerneldll)
	{
		DbgBreakPrint("Error: Cannot load KernelBase.dll!");
	}

	IsDebuggerPresentFuncBytes = (unsigned char* const)GetProcAddress(kerneldll, "IsDebuggerPresent");
	if (!IsDebuggerPresentFuncBytes)
	{
		DbgBreakPrint("Error: Cannot find KernelBase.dll!IsDebuggerPresent()");
	}

	DWORD oldprotect = 0x00000000;
	if (!VirtualProtect(IsDebuggerPresentFuncBytes, 3, PAGE_EXECUTE_READWRITE, &oldprotect) )
	{
		DbgBreakPrint("Error: Fail in VirtualProtect");
	}

	IsDebuggerPresentFuncBytes[0] = 0x33; // xor eax, eax
	IsDebuggerPresentFuncBytes[1] = 0xC0;

	IsDebuggerPresentFuncBytes[2] = 0xC3; // ret

	if (!VirtualProtect(IsDebuggerPresentFuncBytes, 3, oldprotect, &oldprotect) )
	{
		DbgBreakPrint("Error: Fail in VirtualProtect");
	}
}

static inline void HookNtSetInformationThread()
{
	// We already hooked it!
	if (NtSetInformationThreadFuncBytes != NULL)
		return;

	HMODULE ntdll = LoadLibraryA("ntdll.dll");
	if (!ntdll)
	{
		DbgBreakPrint("Error: Cannot load ntdll.dll");
	}

	NtSetInformationThreadFuncBytes = (unsigned char* const)GetProcAddress(ntdll, "NtSetInformationThread");
	if (!NtSetInformationThreadFuncBytes)
	{
		DbgBreakPrint("Error: Cannot find ntdll.dll!NtSetInformationThread()");
	}

	DWORD oldprotect = 0x00000000;
	if (!VirtualProtect(NtSetInformationThreadFuncBytes, 5, PAGE_EXECUTE_READWRITE, &oldprotect) )
	{
		DbgBreakPrint("Error: Fail in VirtualProtect");
	}

	if (NtSetInformationThreadFuncBytes[0] != 0xB8) // MOV EAX instruction
	{
		DbgBreakPrint("Error: Unknown instruction encountered");
	}
	else if ( *(const unsigned long* const)(NtSetInformationThreadFuncBytes + 1) != 0x0D) // We expect this to be "MOV EAX, 0x0D"
	{
		DbgBreakPrint("Error: Unknown instruction encountered");
	}

	// Assemble the relative JMP:
	NtSetInformationThreadFuncBytes[0] = 0xE9;
	*(unsigned* const)(NtSetInformationThreadFuncBytes + 1) = (unsigned)&NtSetInformationThreadHook - (unsigned)(unsigned* const)(NtSetInformationThreadFuncBytes + 5);

	NtSetInformationThreadPostHook = NtSetInformationThreadFuncBytes + 5;

	if (!VirtualProtect(NtSetInformationThreadFuncBytes, 5, oldprotect, &oldprotect) )
	{
		DbgBreakPrint("Error: Fail in VirtualProtect");
	}
}
#endif // #ifdef _M_IX86

void CircumventSteamAntiDebugging(void)
{
	// Having a message box here was convenient for debugging steam stuff, but it really messes up some applications to have a message box pop up during a DllMain!
	//MessageBoxA(NULL, NULL, NULL, NULL);

#ifdef _M_IX86
	HookIsDebuggerPresent();

	HookNtSetInformationThread();
#endif // #ifdef _M_IX86

	// No need to separately hook ZwSetInformationThread because ntdll aliases them to the exact same function call
	//HookZwSetInformationThread();

	// TODO: Hook other functions used to hide threads from debugger
}
#endif

// x64 on x64: "C:\Windows\System32"
// x86 on x64: "C:\Windows\SysWoW64"
// x86 on x86: "C:\Windows\System32"
static inline const char* const GetSystemDirectoryHelper()
{
	static char systemDirectoryBuffer[MAX_PATH] = {0};

	if (systemDirectoryBuffer[0] != '\0')
		return systemDirectoryBuffer;

#ifdef _M_IX86 // x86 target
	BOOL isRunningWoW64Process = FALSE;
	if (!IsWow64Process(GetCurrentProcess(), &isRunningWoW64Process) )
	{
#ifdef _DEBUG
		__debugbreak(); // Should never be here!
#endif
		return NULL;
	}

	if (isRunningWoW64Process)
	{
		if (GetSystemWow64DirectoryA(systemDirectoryBuffer, sizeof(systemDirectoryBuffer) / sizeof(systemDirectoryBuffer[0]) ) < 2)
		{
#ifdef _DEBUG
			__debugbreak(); // Should never be here!
#endif
			return NULL;
		}
		return systemDirectoryBuffer;
	}
#elif defined(_M_X64) // x64 target
#else // Other target (ARM target perhaps?)
	#error Error: Only x86 and x64 are currently supported!
#endif // #ifdef _M_IX86

	if (GetSystemDirectoryA(systemDirectoryBuffer, sizeof(systemDirectoryBuffer) / sizeof(systemDirectoryBuffer[0]) ) < 2)
	{
#ifdef _DEBUG
		__debugbreak(); // Should never be here!
#endif
		return NULL;
	}

	return systemDirectoryBuffer;
}

BOOL WINAPI DllMain(_In_ HINSTANCE hInst, _In_ DWORD reason, _In_ LPVOID /*lpvReserved*/)
{
#ifdef INCREASE_SYSTEM_SCHEDULER_RESOLUTION
	static TIMECAPS timeCaps = {0};
#endif // #ifdef INCREASE_SYSTEM_SCHEDULER_RESOLUTION

	if (reason == DLL_PROCESS_ATTACH)
	{
		hLThisDLL = hInst;

#ifdef _DEBUG
		CircumventSteamAntiDebugging();
#endif

#ifdef INCREASE_SYSTEM_SCHEDULER_RESOLUTION
		if (timeGetDevCaps(&timeCaps, sizeof(timeCaps) ) != MMSYSERR_NOERROR)
		{
			return FALSE;
		}

		timeBeginPeriod(timeCaps.wPeriodMin);
#endif // #ifdef INCREASE_SYSTEM_SCHEDULER_RESOLUTION

		char loadLibraryBuffer[MAX_PATH + 16] = {0};
#pragma warning(push)
#pragma warning(disable:4996) // warning C4996: 'strcpy': This function or variable may be unsafe. Consider using strcpy_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
		strcpy(loadLibraryBuffer, GetSystemDirectoryHelper() );
		strcat(loadLibraryBuffer, "\\d3d9.dll");
#pragma warning(pop)

		hL = LoadLibraryA(loadLibraryBuffer);

		if (!hL)
			return FALSE;

		Real_D3DPERF_BeginEvent = (const D3DPERF_BeginEventType)GetProcAddress(hL, "D3DPERF_BeginEvent");
		Real_D3DPERF_EndEvent = (const D3DPERF_EndEventType)GetProcAddress(hL, "D3DPERF_EndEvent");
		Real_D3DPERF_GetStatus = (const D3DPERF_GetStatusType)GetProcAddress(hL, "D3DPERF_GetStatus");
		Real_D3DPERF_QueryRepeatFrame = (const D3DPERF_QueryRepeatFrameType)GetProcAddress(hL, "D3DPERF_QueryRepeatFrame");
		Real_D3DPERF_SetMarker = (const D3DPERF_SetMarkerType)GetProcAddress(hL, "D3DPERF_SetMarker");
		Real_D3DPERF_SetOptions = (const D3DPERF_SetOptionsType)GetProcAddress(hL, "D3DPERF_SetOptions");
		Real_D3DPERF_SetRegion = (const D3DPERF_SetRegionType)GetProcAddress(hL, "D3DPERF_SetRegion");
		Real_DebugSetLevel = (const DebugSetLevelType)GetProcAddress(hL, "DebugSetLevel");
		Real_DebugSetMute = (const DebugSetMuteType)GetProcAddress(hL, "DebugSetMute");
		Real_Direct3D9EnableMaximizedWindowedModeShim = (const Direct3D9EnableMaximizedWindowedModeShimType)GetProcAddress(hL, "Direct3D9EnableMaximizedWindowedModeShim");
		Real_Direct3DCreate9 = (const Direct3DCreate9Type)GetProcAddress(hL, "Direct3DCreate9");
		Real_Direct3DCreate9Ex = (const Direct3DCreate9ExType)GetProcAddress(hL, "Direct3DCreate9Ex");
		Real_Direct3DShaderValidatorCreate9 = (const Direct3DShaderValidatorCreate9Type)GetProcAddress(hL, "Direct3DShaderValidatorCreate9");
		Real_PSGPError = (const PSGPErrorType)GetProcAddress(hL, "PSGPError");
		Real_PSGPSampleTexture = (const PSGPSampleTextureType)GetProcAddress(hL, "PSGPSampleTexture");
		Real_Direct3D9ForceHybridEnumeration = (const Direct3D9ForceHybridEnumerationType)GetProcAddress(hL, (LPCSTR)LOWORD(16u) ); // "void Direct3D9ForceHybridEnumeration(BOOL)"
		Real_Direct3D9SetMaximizedWindowedModeShim = (const Direct3D9SetMaximizedWindowedModeShimType)GetProcAddress(hL, (LPCSTR)LOWORD(17u) ); // "void Direct3D9SetMaximizedWindowedModeShim()"
		Real_Direct3D9SetSwapEffectUpgradeShim = (const Direct3D9SetSwapEffectUpgradeShimType)GetProcAddress(hL, (LPCSTR)LOWORD(18u) ); // "void Direct3D9SetSwapEffectUpgradeShim()"
		Real_Direct3D9Force9On12 = (const Direct3D9Force9On12Type)GetProcAddress(hL, (LPCSTR)LOWORD(19u) ); // "void Direct3D9Force9On12()"
		Real_Direct3DCreate9On12 = (const Direct3DCreate9On12Type)GetProcAddress(hL, (LPCSTR)LOWORD(20u) ); // "IDirect3D9* Direct3DCreate9On12()"
		Real_Direct3DCreate9On12Ex = (const Direct3DCreate9On12ExType)GetProcAddress(hL, (LPCSTR)LOWORD(21u) ); // "HRESULT Direct3DCreate9On12Ex()"
		Real_Direct3D9SetMaximizedWindowHwndOverride = (const Direct3D9SetMaximizedWindowHwndOverrideType)GetProcAddress(hL, (LPCSTR)LOWORD(22u) ); // "void Direct3D9SetMaximizedWindowHwndOverride()"
		Real_Direct3D9SetVendorIDLieFor9On12 = (const Direct3D9SetVendorIDLieFor9On12Type)GetProcAddress(hL, (LPCSTR)LOWORD(23u) ); // "void Direct3D9SetVendorIDLieFor9On12(BOOL)"
	}

	if (reason == DLL_PROCESS_DETACH)
	{
#ifdef INCREASE_SYSTEM_SCHEDULER_RESOLUTION
		timeEndPeriod(timeCaps.wPeriodMin);
#endif // #ifdef INCREASE_SYSTEM_SCHEDULER_RESOLUTION

		FreeLibrary(hL);
	}

	return TRUE;
}

// D3DPERF_BeginEvent
extern "C" int WINAPI HookD3DPERF_BeginEvent(D3DCOLOR col, LPCWSTR wszName)
{
	int ret = (*Real_D3DPERF_BeginEvent)(col, wszName);
	return ret;
}

// D3DPERF_EndEvent
extern "C" int WINAPI HookD3DPERF_EndEvent(void)
{
	int ret = (*Real_D3DPERF_EndEvent)();
	return ret;
}

// D3DPERF_GetStatus
extern "C" DWORD WINAPI HookD3DPERF_GetStatus(void)
{
	DWORD ret = (*Real_D3DPERF_GetStatus)();
	return ret;
}

// D3DPERF_QueryRepeatFrame
extern "C" BOOL WINAPI HookD3DPERF_QueryRepeatFrame(void)
{
	BOOL ret = (*Real_D3DPERF_QueryRepeatFrame)();
	return ret;
}

// D3DPERF_SetMarker
extern "C" void WINAPI HookD3DPERF_SetMarker(D3DCOLOR col, LPCWSTR wszName)
{
	(*Real_D3DPERF_SetMarker)(col, wszName);
}

// D3DPERF_SetOptions
extern "C" void WINAPI HookD3DPERF_SetOptions(DWORD dwOptions)
{
	(*Real_D3DPERF_SetOptions)(dwOptions);
}

// D3DPERF_SetRegion
extern "C" void WINAPI HookD3DPERF_SetRegion(D3DCOLOR col, LPCWSTR wszName)
{
	(*Real_D3DPERF_SetRegion)(col, wszName);
}

// DebugSetLevel
extern "C" void WINAPI HookDebugSetLevel()
{
	(*Real_DebugSetLevel)();
}

// DebugSetMute
extern "C" void WINAPI HookDebugSetMute()
{
	(*Real_DebugSetMute)();
}

// Direct3D9EnableMaximizedWindowedModeShim
extern "C"  HRESULT WINAPI HookDirect3D9EnableMaximizedWindowedModeShim(BOOL ShimEnable)
{
	HRESULT ret = (*Real_Direct3D9EnableMaximizedWindowedModeShim)(ShimEnable);
	return ret;
}

// Direct3DCreate9
extern "C" IDirect3D9* WINAPI HookDirect3DCreate9(_In_ UINT SDKVersion)
{	
	// Pre-hook code
	IDirect3D9* ret = (*Real_Direct3DCreate9)(SDKVersion);
	// Post-hook code

	if (!ret)
	{
		return NULL;
	}

	IDirect3D9Hook* newHook = new IDirect3D9Hook(ret);
	return newHook;
}

// Direct3DCreate9Ex
extern "C" HRESULT WINAPI HookDirect3DCreate9Ex(_In_ UINT SDKVersion, _Out_ IDirect3D9Ex** ppD3D)
{
	// Pre-hook code
	HRESULT ret = (*Real_Direct3DCreate9Ex)(SDKVersion, ppD3D);
	// Post-hook code
	//MessageBoxA(NULL, "Direct3DCreate9Ex", NULL, NULL);
	return ret;
}

// Direct3DShaderValidatorCreate9
extern "C" IDirect3DShaderValidator9* WINAPI HookDirect3DShaderValidatorCreate9(void)
{
	IDirect3DShaderValidator9* ret = (*Real_Direct3DShaderValidatorCreate9)();
	return ret;
}

// PSGPError
extern "C" void __stdcall HookPSGPError(class D3DFE_PROCESSVERTICES* verticesPtr, enum PSGPERRORID errID, UINT UnknownUInt)
{
	(*Real_PSGPError)(verticesPtr, errID, UnknownUInt);
}

// PSGPSampleTexture
extern "C" void __stdcall HookPSGPSampleTexture(class D3DFE_PROCESSVERTICES* verticesPtr, UINT UnknownUInt1, float (*const UnknownFloats1)[4], unsigned int UnknownUInt2, float (*const UnknownFloats2)[4])
{
	(*Real_PSGPSampleTexture)(verticesPtr, UnknownUInt1, UnknownFloats1, UnknownUInt2, UnknownFloats2);
}

// Direct3D9ForceHybridEnumeration
extern "C" void __stdcall HookDirect3D9ForceHybridEnumeration(UINT ForceHybridEnumeration)
{
	(*Real_Direct3D9ForceHybridEnumeration)(ForceHybridEnumeration);
}

// Direct3D9SetMaximizedWindowedModeShim
extern "C" void __stdcall HookDirect3D9SetMaximizedWindowedModeShim(BOOL a, BOOL b)
{
	(*Real_Direct3D9SetMaximizedWindowedModeShim)(a, b);
}

// Direct3D9SetSwapEffectUpgradeShim
extern "C" INT __stdcall HookDirect3D9SetSwapEffectUpgradeShim(BOOL ShimEnable)
{
	INT ret = (*Real_Direct3D9SetSwapEffectUpgradeShim)(ShimEnable);
	return ret;
}

// Direct3D9Force9On12
// Sets a global value in d3d9.dll that, when set to "1", causes future calls to Direct3DCreate9 and Direct3DCreate9Ex to be internally
// turned into calls to Direct3DCreate9On12 and Direct3DCreate9On12Ex respectively, thus transparently creating D3D9On12 devices
// for the callers without any user code modifications (except for the one initial call to Direct3D9Force9on12() ).
// Note that this does not transform any existing D3D9 or D3D9Ex devices into D3D9On12/D3D9On12Ex devices, it only affects
// devices created after the force is enabled.
extern "C" void __stdcall HookDirect3D9Force9On12(_In_ Force9On12Mode Mode)
{
	(*Real_Direct3D9Force9On12)(Mode);
}

// Direct3DCreate9On12
extern "C" IDirect3D9* __stdcall HookDirect3DCreate9On12(_In_ UINT SDKVersion, _In_ D3D9ON12_ARGS* pOverrideList, _In_ UINT NumOverrideEntries)
{
	IDirect3D9* ret = (*Real_Direct3DCreate9On12)(SDKVersion, pOverrideList, NumOverrideEntries);
	return ret;
}

// Direct3DCreate9On12Ex
extern "C" HRESULT __stdcall HookDirect3DCreate9On12Ex(_In_ UINT SDKVersion, _In_ D3D9ON12_ARGS* pOverrideList, UINT NumOverrideEntries, _Inout_ IDirect3D9Ex** ppOutputInterface)
{
	HRESULT ret = (*Real_Direct3DCreate9On12Ex)(SDKVersion, pOverrideList, NumOverrideEntries, ppOutputInterface);
	return ret;
}

// Direct3D9SetMaximizedWindowHwndOverride
extern "C" void __stdcall HookDirect3D9SetMaximizedWindowHwndOverride(_In_ BOOL Override)
{
	(*Real_Direct3D9SetMaximizedWindowHwndOverride)(Override);
}

// Direct3D9SetVendorIDLieFor9On12
extern "C" void __stdcall HookDirect3D9SetVendorIDLieFor9On12(_In_ BOOL VendorIDLie)
{
	(*Real_Direct3D9SetVendorIDLieFor9On12)(VendorIDLie);
}
