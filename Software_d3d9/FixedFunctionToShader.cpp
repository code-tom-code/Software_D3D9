#pragma once

#include "FixedFunctionToShader.h"
#include "IDirect3DVertexShader9Hook.h"
#include "IDirect3DPixelShader9Hook.h"
#include "GlobalToggles.h"
#include <vector> // for std::vector
#include <map> // for std::map
#include "resource.h"

// Note: There's some good tidbits of information on this page: https://docs.microsoft.com/en-us/windows-hardware/drivers/display/converting-the-direct3d-fixed-function-state

/*static*/ D3DXIncludeHandler D3DXIncludeHandler::globalIncludeHandlerSingleton;

#ifndef NO_CACHING_FIXED_FUNCTION_SHADERS
// These maps map from device state hashes into shaders:
static std::map<FixedFunctionStateHash, IDirect3DVertexShader9Hook*> vertShadersHashMap;
static std::map<FixedFunctionStateHash, IDirect3DPixelShader9Hook*> pixelShadersHashMap;
#endif // NO_CACHING_FIXED_FUNCTION_SHADERS

DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE D3DXIncludeHandler::Open(THIS_ D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
{
	UNREFERENCED_PARAMETER(pParentData);
	UNREFERENCED_PARAMETER(IncludeType);

	unsigned resourceSize = 0;
	const void* includeFileBytes = GetShaderResourceFile(pFileName, resourceSize);
	if (includeFileBytes != NULL)
	{
		*ppData = includeFileBytes;
		*pBytes = resourceSize;
		return S_OK;
	}
	else
		return E_FAIL;
}

DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE D3DXIncludeHandler::Close(THIS_ LPCVOID pData)
{
	UNREFERENCED_PARAMETER(pData);

	// We don't have to do anything on close, so this is okay!
	return S_OK;
}

// The resourceName passed in may also be a string from the MAKEINTRESOURCEA() macro.
// Returns NULL if the resource cannot be found!
const void* const GetShaderResourceFile(const char* const resourceNameA, unsigned& outFoundResourceSize, const char* const resourceCategory/* = "HLSL"*/)
{
	HRSRC hlslResource = FindResourceA(hLThisDLL, resourceNameA, resourceCategory);
	if (hlslResource)
	{
		HGLOBAL loadedResource = LoadResource(hLThisDLL, hlslResource);
		if (loadedResource)
		{
			const unsigned resourceSize = SizeofResource(hLThisDLL, hlslResource);
			if (resourceSize > 0)
			{
				const void* const resourceBytes = LockResource(loadedResource);
				if (resourceBytes)
				{
					outFoundResourceSize = resourceSize;
					return resourceBytes;
				}
			}
		}
	}
	outFoundResourceSize = 0;
	return NULL;
}

void FixedFunctionStateToVertexShader(const DeviceState& state, IDirect3DVertexShader9Hook** const outVertShader, IDirect3DDevice9Hook* const dev)
{
#ifdef _DEBUG
	if (!outVertShader)
	{
		__debugbreak();
	}
#endif

#ifndef NO_CACHING_FIXED_FUNCTION_SHADERS
	const FixedFunctionStateHash stateHash = HashVertexState(state);

	// If we can match the device state to a shader (should happen every time after the first) then just return that shader
	const std::map<FixedFunctionStateHash, IDirect3DVertexShader9Hook*>::const_iterator it = vertShadersHashMap.find(stateHash);
	if (it != vertShadersHashMap.end() )
	{
		*outVertShader = it->second;
		return;
	}
#endif // NO_CACHING_FIXED_FUNCTION_SHADERS

	// Construct new vertex shader from device state
	IDirect3DVertexShader9Hook* newVertShader = NULL;
	BuildVertexShader(state, dev, &newVertShader);

#ifndef NO_CACHING_FIXED_FUNCTION_SHADERS
	// Insert new vertex shader into vert shaders hash map
	vertShadersHashMap.insert(std::make_pair(stateHash, newVertShader) );
#endif // NO_CACHING_FIXED_FUNCTION_SHADERS

	*outVertShader = newVertShader;
}

void FixedFunctionStateToPixelShader(const DeviceState& state, IDirect3DPixelShader9Hook** const outPixelShader, IDirect3DDevice9Hook* const dev)
{
#ifdef _DEBUG
	if (!outPixelShader)
	{
		__debugbreak();
	}
#endif

#ifndef NO_CACHING_FIXED_FUNCTION_SHADERS
	const FixedFunctionStateHash stateHash = HashPixelState(state);

	// If we can match the device state to a shader (should happen every time after the first) then just return that shader
	const std::map<FixedFunctionStateHash, IDirect3DPixelShader9Hook*>::const_iterator it = pixelShadersHashMap.find(stateHash);
	if (it != pixelShadersHashMap.end() )
	{
		*outPixelShader = it->second;
		return;
	}
#endif // NO_CACHING_FIXED_FUNCTION_SHADERS

	// Construct new pixel shader from device state
	IDirect3DPixelShader9Hook* newPixelShader = NULL;
	BuildPixelShader(state, dev, &newPixelShader);

#ifndef NO_CACHING_FIXED_FUNCTION_SHADERS
	// Insert new pixel shader into pixel shaders hash map
	pixelShadersHashMap.insert(std::make_pair(stateHash, newPixelShader) );
#endif // NO_CACHING_FIXED_FUNCTION_SHADERS

	*outPixelShader = newPixelShader;
}
