#pragma once

#include "IDirect3DDevice9Hook.h"

extern HINSTANCE hLThisDLL;

#ifndef NO_CACHING_FIXED_FUNCTION_SHADERS
typedef unsigned __int64 FixedFunctionStateHash;

template <typename T>
static inline void HashContinue(FixedFunctionStateHash& hash, const T data)
{
	const unsigned rotateAmount = (sizeof(T) * 8) % (sizeof(FixedFunctionStateHash) * 8);
	if (rotateAmount != 0)
		hash = _rotl64(hash, rotateAmount);
	hash ^= data;
}

// Need an explicit template specialization here because operator^= is not defined for floats:
template <>
static inline void HashContinue(FixedFunctionStateHash& hash, const float data)
{
	const unsigned rotateAmount = (sizeof(float) * 8) % (sizeof(FixedFunctionStateHash) * 8);
	if (rotateAmount != 0)
		hash = _rotl64(hash, rotateAmount);
	const DWORD dwFlt = *(const DWORD* const)&data;
	hash ^= dwFlt;
}

template <>
static inline void HashContinue(FixedFunctionStateHash& hash, const BOOL data)
{
	const unsigned rotateAmount = 1;
	hash = _rotl64(hash, rotateAmount);
	if (data)
		hash ^= 0x1;
}

template <>
static inline void HashContinue(FixedFunctionStateHash& hash, const bool data)
{
	const unsigned rotateAmount = 1;
	hash = _rotl64(hash, rotateAmount);
	if (data)
		hash ^= 0x1;
}

template <typename T>
static inline void HashStruct(FixedFunctionStateHash& hash, const T& data)
{
	const unsigned rotateAmount = (sizeof(T) * 8) % (sizeof(FixedFunctionStateHash) * 8);
	if (rotateAmount != 0)
		hash = _rotl64(hash, rotateAmount);

	const unsigned char* const uData = (const unsigned char* const)&data;
	for (unsigned x = 0; x < sizeof(T) / sizeof(unsigned char); ++x)
	{
		hash = _rotl64(hash, sizeof(unsigned char) * 8);
		hash ^= uData[x];
	}
}

// Forward-declares:
const FixedFunctionStateHash HashVertexState(const DeviceState& state);
const FixedFunctionStateHash HashPixelState(const DeviceState& state);

#endif // #ifndef NO_CACHING_FIXED_FUNCTION_SHADERS

void FixedFunctionStateToVertexShader(const DeviceState& state, IDirect3DVertexShader9Hook** const outVertShader, IDirect3DDevice9Hook* const dev);
void FixedFunctionStateToPixelShader(const DeviceState& state, IDirect3DPixelShader9Hook** const outPixelShader, IDirect3DDevice9Hook* const dev);
void SetFixedFunctionVertexShaderState(const DeviceState& state, IDirect3DDevice9Hook* const dev);
void SetFixedFunctionPixelShaderState(const DeviceState& state, IDirect3DDevice9Hook* const dev);
void BuildVertexShader(const DeviceState& state, IDirect3DDevice9Hook* const dev, IDirect3DVertexShader9Hook** const outNewShader);
void BuildPixelShader(const DeviceState& state, IDirect3DDevice9Hook* const dev, IDirect3DPixelShader9Hook** const outNewShader);

struct D3DXIncludeHandler : public ID3DXInclude
{
	STDMETHOD(Open)(THIS_ D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes);
	STDMETHOD(Close)(THIS_ LPCVOID pData);

	virtual ~D3DXIncludeHandler()
	{
	}

	static inline D3DXIncludeHandler* const GetGlobalIncludeHandlerSingleton()
	{
		return &globalIncludeHandlerSingleton;
	}

private:
	static D3DXIncludeHandler globalIncludeHandlerSingleton;
};

// The resourceName passed in may also be a string from the MAKEINTRESOURCEA() macro.
// Returns NULL if the resource cannot be found!
const void* const GetShaderResourceFile(const char* const resourceNameA, unsigned& outFoundResourceSize, const char* const resourceCategory = "HLSL");
