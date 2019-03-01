#pragma once

#include "IDirect3DDevice9Hook.h"
#include "ShaderAnalysis.h"

class PShaderEngine;

class IDirect3DPixelShader9Hook : public IDirect3DPixelShader9
{
public:
	IDirect3DPixelShader9Hook(LPDIRECT3DPIXELSHADER9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1), jitShaderMain(NULL), triedJit(false)
	{
#ifdef _DEBUG
		memcpy(&Version, &realObject->Version, (char*)&realObject - (char*)&Version);
#endif
	}

	inline LPDIRECT3DPIXELSHADER9 GetUnderlyingPixelShader(void) const
	{
		return realObject;
	}

	virtual ~IDirect3DPixelShader9Hook();

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DPixelShader9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetFunction(THIS_ void* pData,UINT* pSizeOfData) override;

	void CreatePixelShader(const DWORD* const pFunction);

	inline const ShaderInfo& GetShaderInfo() const
	{
		return pixelShaderInfo;
	}

	inline ShaderInfo& GetModifyShaderInfo()
	{
		return pixelShaderInfo;
	}

	void JitLoadShader();

protected:
	LPDIRECT3DPIXELSHADER9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

public:
	typedef void (__fastcall *PSEntry)(PShaderEngine& ps);
	PSEntry jitShaderMain;

	bool triedJit;
protected:

	std::vector<DWORD> shaderBytecode;
	ShaderInfo pixelShaderInfo;
};
