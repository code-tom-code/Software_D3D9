#pragma once

#include "ShaderJIT.h"
#include "IDirect3DDevice9Hook.h"
#include "resource.h"

extern HINSTANCE hLThisDLL;

#pragma warning(push)
#pragma warning(disable:4996)

void LoadPrefixFileInternal(std::vector<char>& cppfile)
{
#pragma warning(push)
#pragma warning(disable:4302) // warning C4302: 'type cast': truncation from 'LPSTR' to 'WORD'
	HRSRC cppResource = FindResourceA(hLThisDLL, MAKEINTRESOURCEA(IDR_CPP1), "CPP");
#pragma warning(pop)
	if (cppResource)
	{
		HGLOBAL loadedResource = LoadResource(hLThisDLL, cppResource);
		if (loadedResource)
		{
			const unsigned resourceSize = SizeofResource(hLThisDLL, cppResource);
			if (resourceSize > 0)
			{
				const void* const resourceBytes = LockResource(loadedResource);
				if (resourceBytes)
				{
					cppfile.resize(resourceSize);
					memcpy(&cppfile.front(), resourceBytes, resourceSize);
					return;
				}
			}
		}
	}

	__debugbreak(); // Should never be here!
}

// Not multithread-safe!
const char* const ConstructShaderJITName(const ShaderInfo& shaderInfo)
{
	static char buffer[MAX_PATH] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
	// Looks like: "ps_3_0_len114_hash0xD9FF5963d"
	sprintf(buffer, "%cs_%u_%u_len%u_hash0x%08X%c", shaderInfo.isPixelShader ? 'p' : 'v', shaderInfo.shaderMajorVersion, shaderInfo.shaderMinorVersion, shaderInfo.shaderLengthDWORDs, shaderInfo.shaderBytecodeHash, 
#ifdef _DEBUG
		'd'
#else
		'r'
#endif
		);
#pragma warning(pop)
	return buffer;
}

static inline const bool JITBATFile(const ShaderInfo& shaderInfo, const char* const shaderFilename)
{
	char batfilename[MAX_PATH] = {0};
	// Looks like: "shaderjit\ps_3_0_len114_hash0xD9FF5963d.bat"
	sprintf(batfilename, "%s\\%s.bat", shaderJITTempDirectory, shaderFilename);
	HANDLE hBatFile = CreateFileA(batfilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hBatFile == INVALID_HANDLE_VALUE)
	{
		DbgBreakPrint("Error in CreateFile");
		return false;
	}

	static std::vector<char> batfile;

	batfile.clear();

#ifdef _DEBUG
	static const char* const compileDefines = "/D \"WIN32\""
	" /D \"_DEBUG\""
	" /D \"_WINDOWS\""
	" /D \"_USRDLL\""
	" /D \"_WINDLL\"";
#else // Release
	static const char* const compileDefines = "/D \"WIN32\""
	" /D \"NDEBUG\""
	" /D \"_WINDOWS\""
	" /D \"_USRDLL\""
	" /D \"_WINDLL\"";
#endif // #ifdef _DEBUG

	// TODO: Don't hardcode these paths...
#ifdef _DEBUG
	static const char* const compileString = "cl.exe /c /I \"C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Include\" /FAcs /Fa /analyze- /W3 /Zc:wchar_t /ZI /Od /fp:precise /WX- /Zc:forScope /RTC1 /Gd /Oy- /MDd /EHsc /nologo /GS %s %s.cpp\r\n";
#else // Release
	static const char* const compileString = "cl.exe /c /I \"C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Include\" /FAcs /Fa /analyze- /W3 /Zc:wchar_t /Zi /GS- /GL /Gy /Gm- /O2 /Ob2 /fp:fast /GF /WX- /Zc:forScope /arch:AVX2 /Gd /Oy- /Oi /MT /Ot %s %s.cpp\r\n";
#endif // #ifdef _DEBUG

#ifdef _M_X64
	#define LIBPATH "/LIBPATH:\"C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x64\""
#else
	#define LIBPATH "/LIBPATH:\"C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x86\""
#endif

#ifdef _M_X64
	#define MACHINE_PLATFORM "/MACHINE:X64"
	#define SAFESEH_FLAG "" // SafeSEH is not supported on x64
#else
	#define MACHINE_PLATFORM "/MACHINE:X86"
	#define SAFESEH_FLAG "/SAFESEH"
#endif

	// TODO: Don't hardcode these paths...
#ifdef _DEBUG
	static const char* const linkString = "link.exe " LIBPATH " /DEBUG /DLL " MACHINE_PLATFORM " /SUBSYSTEM:WINDOWS /NOLOGO /NXCOMPAT %s.obj\r\n";
#else // Release
	static const char* const linkString = "link.exe " LIBPATH " /DEBUG /DLL " MACHINE_PLATFORM " /SUBSYSTEM:WINDOWS /NODEFAULTLIB /ENTRY:DllMain /NOLOGO /NXCOMPAT /LTCG /DLL /DYNAMICBASE \"Kernel32.lib\" \"libucrt.lib\" /OPT:REF " SAFESEH_FLAG " /INCREMENTAL:NO /OPT:ICF %s.obj\r\n";
#endif // #ifdef _DEBUG

	// Set up VS command prompt
	{
		// TODO: Don't hardcode this path...
#ifdef _M_X64
		static const char* const invokeVSDevCmd = "call \"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat\"\r\n";
#else
		static const char* const invokeVSDevCmd = "call \"C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Enterprise\\Common7\\Tools\\VsDevCmd.bat\"\r\n";
#endif
		AppendString(batfile, invokeVSDevCmd);
	}

	// Compile
	{
		char batBuffer[1024] = {0};
		sprintf(batBuffer, compileString, compileDefines, shaderFilename);
		AppendString(batfile, batBuffer);
	}

	// Link
	{
		char batBuffer[1024] = {0};
		sprintf(batBuffer, linkString, shaderFilename);
		AppendString(batfile, batBuffer);
	}

	DWORD numBytesWritten = 0;
	if (!WriteFile(hBatFile, &(batfile.front() ), batfile.size(), &numBytesWritten, NULL) )
	{
		DbgBreakPrint("Error in WriteFile");
		return false;
	}

	if (batfile.size() != numBytesWritten)
	{
		DbgBreakPrint("Error: Num bytes written doesn't match for bat file");
		return false;
	}

	if (!CloseHandle(hBatFile) )
	{
		DbgBreakPrint("Error in CloseHandle");
		return false;
	}

	return true;
}

static inline const bool CompileLinkDLL(const ShaderInfo& shaderInfo, const char* const shaderFilename)
{
	char batfilename[MAX_PATH] = {0};
	// Looks like: "ps_3_0_len114_hash0xD9FF5963d.bat"
	sprintf(batfilename, "%s.bat", shaderFilename);

	char commandLine[MAX_PATH] = {0};
	sprintf(commandLine, "cmd.exe /C \"%s\"", batfilename);

	STARTUPINFOA si = {0};
	si.cb = sizeof(STARTUPINFOA);
	PROCESS_INFORMATION pi = {0};

	char currentDirectory[MAX_PATH] = {0};
	sprintf(currentDirectory, ".\\%s\\", shaderJITTempDirectory);

	// Either show the shadercompile window (default), or hide it
	const DWORD createProcessFlags = 
#ifdef DEBUG_SHOW_SHADERCOMPILE_WINDOW
		0x00000000
#else
		CREATE_NO_WINDOW
#endif
		;

	if (!CreateProcessA(NULL, commandLine, NULL, NULL, FALSE, createProcessFlags, NULL, currentDirectory, &si, &pi) )
	{
		DbgBreakPrint("Error in CreateProcess");
		return false;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD processExitCode = STILL_ACTIVE;
	if (!GetExitCodeProcess(pi.hProcess, &processExitCode) )
	{
		DbgBreakPrint("Error in GetExitCodeProcess");
		return false;
	}

	if (processExitCode == STILL_ACTIVE)
	{
		DbgBreakPrint("Error: Process did not terminate yet");
		return false;
	}

	if (processExitCode == S_OK)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	DbgBreakPrint("Error: There was an error while compiling or linking the shader JIT DLL");
	OutputDebugStringA("Failure in shader JIT for shader: ");
	OutputDebugStringA(shaderFilename);
	OutputDebugStringA("\n");

	return false;
}

const bool JITNewShader(const ShaderInfo& shaderInfo, const char* const shaderFilename)
{
	if (!CreateDirectoryA(shaderJITTempDirectory, NULL) )
	{
		switch (GetLastError() )
		{
		case ERROR_ALREADY_EXISTS:
			// This is fine
			break;
		case ERROR_PATH_NOT_FOUND:
			// This is not fine!
			DbgBreakPrint("Error in CreateDirectory");
			return false;
		}
	}

	if (!JITCPPFileInternal(shaderInfo, shaderFilename) )
	{
		return false;
	}

	if (!JITBATFile(shaderInfo, shaderFilename) )
	{
		return false;
	}

	if (!CompileLinkDLL(shaderInfo, shaderFilename) )
	{
		return false;
	}

	return true;
}

#pragma warning(pop)
