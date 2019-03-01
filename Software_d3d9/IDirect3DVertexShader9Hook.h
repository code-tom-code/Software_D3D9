#pragma once

#include "IDirect3DDevice9Hook.h"
#include "ShaderAnalysis.h"

class VShaderEngine;

class IDirect3DVertexShader9Hook : public IDirect3DVertexShader9
{
public:
	IDirect3DVertexShader9Hook(LPDIRECT3DVERTEXSHADER9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1), outPositionRegisterIndex(0), 
		jitShaderMain(NULL), jitShaderMain4(NULL), triedJit(false)
	{
#ifdef _DEBUG
		memcpy(&Version, &realObject->Version, (char*)&realObject - (char*)&Version);
#endif
	}

	inline LPDIRECT3DVERTEXSHADER9 GetUnderlyingVertexShader(void) const
	{
		return realObject;
	}

	virtual ~IDirect3DVertexShader9Hook();

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DVertexShader9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetFunction(THIS_ void* pData,UINT* pSizeOfData) override;

	void CreateVertexShader(const DWORD* const pFunction);

	inline const ShaderInfo& GetShaderInfo() const
	{
		return vertexShaderInfo;
	}

	inline ShaderInfo& GetModifyShaderInfo()
	{
		return vertexShaderInfo;
	}

	inline const UINT GetOPosRegisterIndex(void) const
	{
		return outPositionRegisterIndex;
	}

	inline const UINT GetOPosOffset_Bytes(void) const
	{
		return GetOPosRegisterIndex() * sizeof(D3DXVECTOR4);
	}

	inline const D3DXVECTOR4& GetPosition(const VS_2_0_OutputRegisters& thisOutputRegisters) const
	{
		// Uhhhhhh, is this correct? Should we be checking vertex shader versions here before reaching in and grabbing a register?
		return ( (const D3DXVECTOR4* const)&thisOutputRegisters)[outPositionRegisterIndex];
	}

	inline D3DXVECTOR4& GetPosition(VS_2_0_OutputRegisters& thisOutputRegisters) const
	{
		// Uhhhhhh, is this correct? Should we be checking vertex shader versions here before reaching in and grabbing a register?
		return ( (D3DXVECTOR4* const)&thisOutputRegisters)[outPositionRegisterIndex];
	}

	inline void GetPosition4(const VS_2_0_OutputRegisters* (&thisOutputRegisters)[4], const D3DXVECTOR4* (&outPositionPtrs)[4]) const
	{
		// Uhhhhhh, is this correct? Should we be checking vertex shader versions here before reaching in and grabbing a register?
		outPositionPtrs[0] = ( (const D3DXVECTOR4* const)thisOutputRegisters[0]) + outPositionRegisterIndex;
		outPositionPtrs[1] = ( (const D3DXVECTOR4* const)thisOutputRegisters[1]) + outPositionRegisterIndex;
		outPositionPtrs[2] = ( (const D3DXVECTOR4* const)thisOutputRegisters[2]) + outPositionRegisterIndex;
		outPositionPtrs[3] = ( (const D3DXVECTOR4* const)thisOutputRegisters[3]) + outPositionRegisterIndex;
	}

	inline void GetPosition4(VS_2_0_OutputRegisters* (&thisOutputRegisters)[4], D3DXVECTOR4* (&outPositionPtrs)[4]) const
	{
		// Uhhhhhh, is this correct? Should we be checking vertex shader versions here before reaching in and grabbing a register?
		outPositionPtrs[0] = ( (D3DXVECTOR4* const)thisOutputRegisters[0]) + outPositionRegisterIndex;
		outPositionPtrs[1] = ( (D3DXVECTOR4* const)thisOutputRegisters[1]) + outPositionRegisterIndex;
		outPositionPtrs[2] = ( (D3DXVECTOR4* const)thisOutputRegisters[2]) + outPositionRegisterIndex;
		outPositionPtrs[3] = ( (D3DXVECTOR4* const)thisOutputRegisters[3]) + outPositionRegisterIndex;
	}

	void JitLoadShader();

protected:
	LPDIRECT3DVERTEXSHADER9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

public:
	typedef void (__fastcall *VSEntry)(VShaderEngine& ps);
	VSEntry jitShaderMain;
	VSEntry jitShaderMain4;

	bool triedJit;
protected:
	unsigned outPositionRegisterIndex;

	std::vector<DWORD> shaderBytecode;
	ShaderInfo vertexShaderInfo;
};
