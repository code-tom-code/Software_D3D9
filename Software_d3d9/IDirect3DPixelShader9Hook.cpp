#pragma once

#include "IDirect3DPixelShader9Hook.h"
#include "ShaderBase.h"
#include "ShaderJIT.h"

static std::vector<IDirect3DPixelShader9Hook*> alivePixelShaders;

void IDirect3DPixelShader9Hook::CreatePixelShader(const DWORD* const pFunction)
{
	if (!pFunction)
		return;

	ShaderInfo tempPixelShaderInfo;
	AnalyzeShader(pFunction, tempPixelShaderInfo
#ifdef _DEBUG
		,"NonePS"
#endif
		);

	if (!tempPixelShaderInfo.isPixelShader)
	{
		// This is not a pixel shader!
		__debugbreak();
	}

	switch (tempPixelShaderInfo.shaderMajorVersion)
	{
	case 1:
	case 2:
	case 3:
		// We're good!
		break;
	default:
		// This is not a valid D3D9 shader!
		{
			__debugbreak();
		}
		break;
	}

	shaderBytecode.resize(tempPixelShaderInfo.shaderLengthDWORDs);
	memcpy(&shaderBytecode.front(), pFunction, tempPixelShaderInfo.shaderLengthDWORDs * sizeof(DWORD) );

	// Bad hack to rebase all of the pointer offsets, fix plz!
	AnalyzeShader(&shaderBytecode.front(), pixelShaderInfo
#ifdef _DEBUG
		,"NonePS"
#endif
		);

	parentDevice->LockDeviceCS();
	alivePixelShaders.push_back(this);
	parentDevice->UnlockDeviceCS();

	//JitLoadShader();
}

/*virtual*/ IDirect3DPixelShader9Hook::~IDirect3DPixelShader9Hook()
{
	parentDevice->LockDeviceCS();
	const unsigned numAliveShaders = alivePixelShaders.size();
	bool foundAndErased = false;
	for (unsigned x = 0; x < numAliveShaders; ++x)
	{
		if (alivePixelShaders[x] == this)
		{
			alivePixelShaders.erase(alivePixelShaders.begin() + x);
			foundAndErased = true;
			break;
		}
	}
#ifdef _DEBUG
	if (!foundAndErased)
	{
		__debugbreak(); // Should never be here!
	}
#endif
	parentDevice->UnlockDeviceCS();

	shaderBytecode.clear();
#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
	memset(this, 0x00000000, sizeof(*this) - (sizeof(shaderBytecode) + sizeof(pixelShaderInfo) ) );
#endif
}

void IDirect3DPixelShader9Hook::JitLoadShader()
{
#ifdef FORCE_INTERPRETED_PIXEL_SHADER
	return;
#endif

	const char* const jitName = ConstructShaderJITName(pixelShaderInfo);
	char jitFilenameBuffer[MAX_PATH] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
	sprintf(jitFilenameBuffer, "%s\\%s.dll", shaderJITTempDirectory, jitName);
#pragma warning(pop)
	HMODULE hm = LoadLibraryA(jitFilenameBuffer);

#ifdef _M_X64
	static const char* const shaderMainExportName = "PixelShaderMain";
#else
	static const char* const shaderMainExportName = "@PixelShaderMain@4";
#endif

	triedJit = true;

	if (hm)
	{
		jitShaderMain = (PSEntry)GetProcAddress(hm, shaderMainExportName);
		if (!jitShaderMain)
		{
			DbgBreakPrint("Error: Cannot find PixelShaderMain in existing JIT DLL");
		}
	}
	else
	{
		if (!JITNewShader(pixelShaderInfo, jitName) )
		{
			DbgBreakPrint("Error: Failed to JIT Pixel Shader");
		}
		else
		{
			HMODULE hm2 = LoadLibraryA(jitFilenameBuffer);
			if (!hm2)
			{
				DbgBreakPrint("Error: Failed to load recently JIT'd Pixel Shader");
				return;
			}
			jitShaderMain = (PSEntry)GetProcAddress(hm2, shaderMainExportName);
			if (!jitShaderMain)
			{
				DbgBreakPrint("Error: Cannot find PixelShaderMain in newly created JIT DLL");
				return;
			}
		}
	}
}

/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DPixelShader9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	HRESULT ret = realObject->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DPixelShader9Hook::AddRef(THIS)
{
	ULONG ret = realObject->AddRef();
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DPixelShader9Hook::Release(THIS)
{
	ULONG ret = realObject->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked Pixel Shader %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

/*** IDirect3DPixelShader9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DPixelShader9Hook::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
{
	LPDIRECT3DDEVICE9 realD3D9dev = NULL;
	HRESULT ret = realObject->GetDevice(&realD3D9dev);
	if (FAILED(ret) )
	{
		*ppDevice = NULL;
		return ret;
	}

	// Check that the parentHook's underlying IDirect3DDevice9* matches the realD3D9dev pointer
	if (parentDevice->GetUnderlyingDevice() != realD3D9dev)
	{
		DbgBreakPrint("Error: Unknown d3d9 device hook detected!");
	}
	parentDevice->AddRef(); // Super important to increment the ref-count here, otherwise our parent object will get destroyed when Release() is called on it!

	*ppDevice = parentDevice;
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DPixelShader9Hook::GetFunction(THIS_ void* pData,UINT* pSizeOfData)
{
	HRESULT ret = realObject->GetFunction(pData, pSizeOfData);
	return ret;
}
