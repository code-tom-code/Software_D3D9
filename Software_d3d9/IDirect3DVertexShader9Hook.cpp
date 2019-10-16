#pragma once

#include "IDirect3DVertexShader9Hook.h"
#include "ShaderBase.h"
#include "ShaderJIT.h"

static std::vector<IDirect3DVertexShader9Hook*> aliveVertShaders;

void IDirect3DVertexShader9Hook::CreateVertexShader(const DWORD* const pFunction)
{
	if (!pFunction)
		return;

	ShaderInfo tempVertexShaderInfo;
	AnalyzeShader(pFunction, tempVertexShaderInfo
#ifdef _DEBUG
		,"NoneVS"
#endif
		);

	if (tempVertexShaderInfo.isPixelShader)
	{
		// This is not a vertex shader!
		__debugbreak();
	}

	switch (tempVertexShaderInfo.shaderMajorVersion)
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

	shaderBytecode.resize(tempVertexShaderInfo.shaderLengthDWORDs);
	memcpy(&shaderBytecode.front(), pFunction, tempVertexShaderInfo.shaderLengthDWORDs * sizeof(DWORD) );

	// Bad hack to rebase all of the pointer offsets, fix plz!
	AnalyzeShader(&shaderBytecode.front(), vertexShaderInfo
#ifdef _DEBUG
		,"NoneVS"
#endif
		);

	if (tempVertexShaderInfo.shaderMajorVersion == 3)
	{
		const std::vector<DeclaredRegister>& declaredRegs = tempVertexShaderInfo.declaredRegisters;
		const unsigned numDeclaredRegs = declaredRegs.size();
#ifdef _DEBUG
		outPositionRegisterIndex = 0xFFFFFFFF;
#endif
		for (unsigned x = 0; x < numDeclaredRegs; ++x)
		{
			const DeclaredRegister& thisReg = declaredRegs[x];
			if (!thisReg.isOutputRegister)
				continue;
			if (thisReg.usageType == D3DDECLUSAGE_POSITION || thisReg.usageType == D3DDECLUSAGE_POSITIONT)
			{
				static const unsigned NUM_PRE_OT_REGISTERS = 3u; // This is the dedicated oPos, oFog, and oPts registers used by pre-vs_3_0 shader models
				outPositionRegisterIndex = NUM_PRE_OT_REGISTERS + thisReg.registerIndex;
				break;
			}
		}
#ifdef _DEBUG
		if (outPositionRegisterIndex == 0xFFFFFFFF)
		{
			__debugbreak(); // Output Position register not found!
		}
#endif
	}
	else
	{
		// For vertex shaders before version vs_3_0, this should always be the dedicated oPos output register
		outPositionRegisterIndex = 0;
	}

	parentDevice->LockDeviceCS();
	aliveVertShaders.push_back(this);
	parentDevice->UnlockDeviceCS();

	//JitLoadShader();
}

/*virtual*/ IDirect3DVertexShader9Hook::~IDirect3DVertexShader9Hook()
{
	parentDevice->LockDeviceCS();
	const unsigned numAliveShaders = aliveVertShaders.size();
	bool foundAndErased = false;
	for (unsigned x = 0; x < numAliveShaders; ++x)
	{
		if (aliveVertShaders[x] == this)
		{
			aliveVertShaders.erase(aliveVertShaders.begin() + x);
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
	memset(this, 0x00000000, sizeof(*this) - (sizeof(shaderBytecode) + sizeof(vertexShaderInfo) ) );
#endif
}

/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexShader9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	HRESULT ret = realObject->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DVertexShader9Hook::AddRef(THIS)
{
	ULONG ret = realObject->AddRef();
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DVertexShader9Hook::Release(THIS)
{
	ULONG ret = realObject->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked Vertex Shader %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

/*** IDirect3DVertexShader9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexShader9Hook::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
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

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexShader9Hook::GetFunction(THIS_ void* pData,UINT* pSizeOfData)
{
	HRESULT ret = realObject->GetFunction(pData, pSizeOfData);
	return ret;
}

void IDirect3DVertexShader9Hook::JitLoadShader()
{
#ifdef FORCE_INTERPRETED_VERTEX_SHADER
	return;
#endif

	const char* const jitName = ConstructShaderJITName(vertexShaderInfo);
	char jitFilenameBuffer[MAX_PATH] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
	sprintf(jitFilenameBuffer, "%s\\%s.dll", shaderJITTempDirectory, jitName);
#pragma warning(pop)
	HMODULE hm = LoadLibraryA(jitFilenameBuffer);

	triedJit = true;

	static const char* const shaderMainExportName = "@VertexShaderMain@4";
	static const char* const shaderMainExportName4 = "@VertexShaderMain4@4";

	if (hm)
	{
		jitShaderMain = (VSEntry)GetProcAddress(hm, shaderMainExportName);
		jitShaderMain4 = (VSEntry)GetProcAddress(hm, shaderMainExportName4);
		if (!jitShaderMain)
		{
			DbgBreakPrint("Error: Cannot find VertexShaderMain in existing JIT DLL");
		} 
		else if (!jitShaderMain4)
		{
			DbgPrint("Warning: Cannot find VertexShaderMain4 in existing JIT DLL");
		}
	}
	else
	{
		if (!JITNewShader(vertexShaderInfo, jitName) )
		{
			DbgBreakPrint("Error: Failed to JIT Vertex Shader");
		}
		else
		{
			HMODULE hm2 = LoadLibraryA(jitFilenameBuffer);
			if (!hm2)
			{
				DbgBreakPrint("Error: Failed to load recently JIT'd Vertex Shader");
			}
			jitShaderMain = (VSEntry)GetProcAddress(hm2, shaderMainExportName);
			jitShaderMain4 = (VSEntry)GetProcAddress(hm2, shaderMainExportName4);
			if (!jitShaderMain)
			{
				DbgBreakPrint("Error: Cannot find VertexShaderMain in newly created JIT DLL");
			} 
			else if (!jitShaderMain4)
			{
				DbgPrint("Warning: Cannot find VertexShaderMain4 in newly created JIT DLL");
			}
		}
	}
}
