#pragma once

#ifdef _DEBUG
	#define D3D_DEBUG_INFO 1
#else
	#undef D3D_DEBUG_INFO
#endif

// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#pragma warning(disable:4530)

#undef UNICODE
#undef _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <d3d9.h>

// Note: For this project (implemented as a d3d9.dll hook), we do *not* want to link against d3d9.lib because it would force
// our output-generated d3d9.dll to be itself dependent on the real d3d9.dll.
// #pragma comment(lib, "d3d9.lib")

#include <d3dx9.h>
#ifdef _DEBUG
	#pragma comment(lib, "d3dx9d.lib")
#else
	#pragma comment(lib, "d3dx9.lib")
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define DbgPrint(x) OutputDebugStringA(x " (" __FILE__ ":"  TOSTRING(__LINE__) ")\n")

#ifdef _DEBUG
	#define DbgBreakPrint(x) MessageBoxA(NULL, __FILE__ ":" TOSTRING(__LINE__) "\n" x, "Error", NULL); DbgPrint(x)
#else
	#define DbgBreakPrint(x) DbgPrint(x)
#endif
