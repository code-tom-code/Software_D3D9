#pragma once

#undef UNICODE
#undef _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#pragma pack(1)

#include "d3d9include.h"

#include "IDirect3D9Hook.h"

HINSTANCE hLThisDLL = 0;
HINSTANCE hL = 0;
FARPROC p[16] = {0};

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

void CircumventSteamAntiDebugging(void)
{
	// Having a message box here was convenient for debugging steam stuff, but it really messes up some applications to have a message box pop up during a DllMain!
	//MessageBoxA(NULL, NULL, NULL, NULL);

	HookIsDebuggerPresent();

	HookNtSetInformationThread();

	// No need to separately hook ZwSetInformationThread because ntdll aliases them to the exact same function call
	//HookZwSetInformationThread();

	// TODO: Hook other functions used to hide threads from debugger
}
#endif

__declspec(dllexport) BOOL WINAPI DllMain(_In_ HINSTANCE hInst, _In_ DWORD reason, _In_ LPVOID /*lpvReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		hLThisDLL = hInst;

#ifdef _DEBUG
		CircumventSteamAntiDebugging();
#endif

		// x86 32-bit version:
		// TODO: Don't hardcode this path
		hL = LoadLibraryA("C:\\Windows\\SysWOW64\\d3d9.dll");

		if (!hL) return false;

		p[0] = GetProcAddress(hL, "D3DPERF_BeginEvent");
		p[1] = GetProcAddress(hL, "D3DPERF_EndEvent");
		p[2] = GetProcAddress(hL, "D3DPERF_GetStatus");
		p[3] = GetProcAddress(hL, "D3DPERF_QueryRepeatFrame");
		p[4] = GetProcAddress(hL, "D3DPERF_SetMarker");
		p[5] = GetProcAddress(hL, "D3DPERF_SetOptions");
		p[6] = GetProcAddress(hL, "D3DPERF_SetRegion");
		p[7] = GetProcAddress(hL, "DebugSetLevel");
		p[8] = GetProcAddress(hL, "DebugSetMute");
		p[9] = GetProcAddress(hL, "Direct3D9EnableMaximizedWindowedModeShim");
		p[10] = GetProcAddress(hL, "Direct3DCreate9");
		p[11] = GetProcAddress(hL, "Direct3DCreate9Ex");
		p[12] = GetProcAddress(hL, "Direct3DShaderValidatorCreate9");
		p[13] = GetProcAddress(hL, "PSGPError");
		p[14] = GetProcAddress(hL, "PSGPSampleTexture");
		p[15] = GetProcAddress(hL, (LPCSTR)LOWORD(16u) ); // Apparently this is some undocumented function added in the Windows 8.1 update: "DWORD Direct3D9ForceHybridEnumeration(UINT)"
	}
	if (reason == DLL_PROCESS_DETACH)
	{
		FreeLibrary(hL);
	}

	return 1;
}

// D3DPERF_BeginEvent
extern "C" int WINAPI __E__0__(D3DCOLOR col, LPCWSTR wszName)
{
	typedef int (WINAPI *D3DPERF_BeginEventType)(D3DCOLOR col, LPCWSTR wszName);
	D3DPERF_BeginEventType beginEventPtr = (D3DPERF_BeginEventType)p[0];
	int ret = (*beginEventPtr)(col, wszName);
	return ret;
}

// D3DPERF_EndEvent
extern "C" int WINAPI __E__1__(void)
{
	typedef int (WINAPI *D3DPERF_EndEventType)(void);
	D3DPERF_EndEventType endEventPtr = (D3DPERF_EndEventType)p[1];
	int ret = (*endEventPtr)();
	return ret;
}

// D3DPERF_GetStatus
extern "C" DWORD WINAPI __E__2__(void)
{
	typedef DWORD (WINAPI *D3DPERF_GetStatusType)(void);
	D3DPERF_GetStatusType getStatusPtr = (D3DPERF_GetStatusType)p[2];
	DWORD ret = (*getStatusPtr)();
	return ret;
}

// D3DPERF_QueryRepeatFrame
extern "C" BOOL WINAPI __E__3__(void)
{
	typedef BOOL (WINAPI *D3DPERF_QueryRepeatFrameType)(void);
	D3DPERF_QueryRepeatFrameType queryRepeatFramePtr = (D3DPERF_QueryRepeatFrameType)p[3];
	BOOL ret = (*queryRepeatFramePtr)();
	return ret;
}

// D3DPERF_SetMarker
extern "C" void WINAPI __E__4__(D3DCOLOR col, LPCWSTR wszName)
{
	typedef void (WINAPI *D3DPERF_SetMarker)(D3DCOLOR col, LPCWSTR wszName);
	D3DPERF_SetMarker setMarkerPtr = (D3DPERF_SetMarker)p[4];
	(*setMarkerPtr)(col, wszName);
}

// D3DPERF_SetOptions
extern "C" void WINAPI __E__5__(DWORD dwOptions)
{
	typedef void (WINAPI *D3DPERF_SetOptions)(DWORD dwOptions);
	D3DPERF_SetOptions setOptionsPtr = (D3DPERF_SetOptions)p[5];
	(*setOptionsPtr)(dwOptions);
}

// D3DPERF_SetRegion
extern "C" void WINAPI __E__6__(D3DCOLOR col, LPCWSTR wszName)
{
	typedef void (WINAPI *D3DPERF_SetRegion)(D3DCOLOR col, LPCWSTR wszName);
	D3DPERF_SetRegion setRegionPtr = (D3DPERF_SetRegion)p[6];
	(*setRegionPtr)(col, wszName);
}

// DebugSetLevel
extern "C" __declspec(naked) void __stdcall __E__7__()
{
__asm
	{
	jmp p[7*4];
	}
}

// DebugSetMute
extern "C" __declspec(naked) void __stdcall __E__8__()
{
__asm
	{
	jmp p[8*4];
	}
}

// Direct3D9EnableMaximizedWindowedModeShim
extern "C" __declspec(naked) void __stdcall __E__9__()
{
__asm
	{
	jmp p[9*4];
	}
}

// Direct3DCreate9
extern "C" IDirect3D9* WINAPI __E__10__(_In_ UINT SDKVersion)
{	
	// Pre-hook code
	typedef IDirect3D9* (WINAPI *Direct3DCreate9Type)(_In_ UINT SDKVersion);
	Direct3DCreate9Type d3dCreate9Ptr = (Direct3DCreate9Type)p[10];
	IDirect3D9* ret = (*d3dCreate9Ptr)(SDKVersion);
	// Post-hook code

	if (!ret)
	{
		return NULL;
	}

	IDirect3D9Hook* newHook = new IDirect3D9Hook(ret);
	return newHook;
}

// Direct3DCreate9Ex
extern "C" HRESULT WINAPI __E__11__(_In_ UINT SDKVersion, _Out_ IDirect3D9Ex** ppD3D)
{
	// Pre-hook code
	typedef HRESULT (WINAPI *Direct3DCreate9ExType)(_In_ UINT SDKVersion, _Out_ IDirect3D9Ex**);
	Direct3DCreate9ExType d3dCreate9ExPtr = (Direct3DCreate9ExType)p[11];
	HRESULT ret = (*d3dCreate9ExPtr)(SDKVersion, ppD3D);
	// Post-hook code
	//MessageBoxA(NULL, "Direct3DCreate9Ex", NULL, NULL);
	return ret;
}

// Direct3DShaderValidatorCreate9
extern "C" __declspec(naked) void __stdcall __E__12__()
{
__asm
	{
	jmp p[12*4];
	}
}

// PSGPError
extern "C" __declspec(naked) void __stdcall __E__13__()
{
__asm
	{
	jmp p[13*4];
	}
}

// PSGPSampleTexture
extern "C" __declspec(naked) void __stdcall __E__14__()
{
__asm
	{
	jmp p[14*4];
	}
}

// ___XXX___16
extern "C" __declspec(naked) void __stdcall __E__15__()
{
__asm
	{
	jmp p[15*4];
	}
}

