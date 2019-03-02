#pragma once

#include "FixedFunctionToShader.h"
#include "IDirect3DVertexShader9Hook.h"
#include "IDirect3DPixelShader9Hook.h"
#include "IDirect3DVertexDeclaration9Hook.h"
#include "GlobalToggles.h"
#include <vector> // for std::vector
#include <map> // for std::map

// Note: There's some good tidbits of information on this page: https://docs.microsoft.com/en-us/windows-hardware/drivers/display/converting-the-direct3d-fixed-function-state

#ifndef NO_CACHING_FIXED_FUNCTION_SHADERS

typedef unsigned __int64 FixedFunctionStateHash;

// These maps map from device state hashes into shaders:
static std::map<FixedFunctionStateHash, IDirect3DVertexShader9Hook*> vertShadersHashMap;
static std::map<FixedFunctionStateHash, IDirect3DPixelShader9Hook*> pixelShadersHashMap;

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

static inline const FixedFunctionStateHash HashVertexState(const DeviceState& state)
{
	FixedFunctionStateHash retHash = 0;

	// Fixed function vertex pipeline cares about:
	// - Clipping Planes
	// - Material Parameters
	// - Lights
	// - Transforms

	const DWORD enableMask = state.currentRenderStates.renderStatesUnion.namedStates.clipPlaneEnable & 0x3F;
	HashContinue(retHash, enableMask);
	for (unsigned x = 0; x < 6; ++x)
	{
		if (enableMask & (1 << x) )
		{
			const D3DXPLANE& clipPlane = state.currentClippingPlanes[x];
			HashStruct(retHash, clipPlane);
		}
	}

	const std::vector<DebuggableD3DVERTEXELEMENT9>& elements = state.currentVertexDecl->GetElementsInternal();
	const unsigned numVertexElements = elements.size();
	for (unsigned x = 0; x < numVertexElements; ++x)
	{
		HashStruct(retHash, elements[x]);
	}

	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.lighting);
	if (state.currentRenderStates.renderStatesUnion.namedStates.lighting)
	{
		HashStruct(retHash, state.currentMaterial);

		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.diffuseMaterialSource);
		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.specularMaterialSource);
		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.ambientMaterialSource);
		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.emissiveMaterialSource);
		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.specularEnable);
		const BOOL globalAmbientBool = state.currentRenderStates.renderStatesUnion.namedStates.ambient ? TRUE : FALSE;
		HashContinue(retHash, globalAmbientBool);

		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.localViewer);
		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.normalizeNormals);
	}

	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.colorVertex);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.vertexBlend);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.pointSpriteEnable);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.indexedVertexBlendEnable);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.tweenFactor);

	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogEnable);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogDensity);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogStart);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogEnd);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogTableMode);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogVertexMode);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.rangeFogEnable);

	// Render states:
	// - Fog (if vertex fog)
	// - Lighting enable
	// - Specular enable
	// - Ambient color
	// - Fog vertex mode
	// - Color vertex
	// - Local viewer
	// - Normalize normals
	// - Diffuse material source
	// - Specular material source
	// - Ambient material source
	// - Emissive material source
	// - Vertex blend
	// - Clip plane enable
	// - Point sprite enable
	// - Point size
	// - Point size min
	// - Point scale enable
	// - Point scale A, B, and C
	// - IndexedVertexBlendEnable
	// - Tween factor

	return retHash;
}

static inline const FixedFunctionStateHash HashPixelState(const DeviceState& state)
{
	FixedFunctionStateHash retHash = 0;

	// Fixed function pixel pipeline cares about:
	// - Texture state stages
	// - Texture transforms (or is this handled in the vert shader)?

	for (unsigned x = 0; x < MAX_NUM_TEXTURE_STAGE_STATES; ++x)
	{
		const TextureStageState& thisTSS = state.currentStageStates[x];
		HashStruct(retHash, thisTSS);
	}

	// Render states:
	// - Alpha testing (D3DRS_ALPHATESTENABLE)
	// - Alpha ref value
	// - Alpha func
	// - Fog (if pixel fog)
	// - Texture factor
	// - Shading mode
	// - Lighting enable
	// - Specular enable
	// - Color vertex

	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.lighting);
	if (state.currentRenderStates.renderStatesUnion.namedStates.lighting)
	{
		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.specularEnable);
	}

	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.alphaTestEnable);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.alphaRef);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.alphaFunc);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogEnable);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.rangeFogEnable);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.textureFactor);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.shadeMode);
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.colorVertex);

	return retHash;
}

#endif // NO_CACHING_FIXED_FUNCTION_SHADERS

static inline void BuildVertexStateDefines(const DeviceState& state, std::vector<D3DXMACRO>& defines)
{
	D3DXMACRO diffuseSourceMacro = {0};
	diffuseSourceMacro.Name = "MATERIAL_DIFFUSE_SOURCE";
	diffuseSourceMacro.Definition = "0";
	// From: Diffuse Lighting https://msdn.microsoft.com/en-us/library/windows/desktop/bb219656(v=vs.85).aspx
	// Note: If either DIFFUSEMATERIALSOURCE option is used, and the vertex color is not provided, the material diffuse color is used.
	if (state.currentRenderStates.renderStatesUnion.namedStates.diffuseMaterialSource == D3DMCS_COLOR1 && state.CurrentStateHasInputVertexColor0() )
		diffuseSourceMacro.Definition = "1";
	else if (state.currentRenderStates.renderStatesUnion.namedStates.diffuseMaterialSource == D3DMCS_COLOR2 && state.CurrentStateHasInputVertexColor1() )
		diffuseSourceMacro.Definition = "2";
	defines.push_back(diffuseSourceMacro);

	if (state.currentRenderStates.renderStatesUnion.namedStates.lighting)
	{
		D3DXMACRO lightingMacro = {0};
		lightingMacro.Name = "WITH_LIGHTING";
		lightingMacro.Definition = "1";
		defines.push_back(lightingMacro);

		D3DXMACRO specularSourceMacro = {0};
		specularSourceMacro.Name = "MATERIAL_SPECULAR_SOURCE";
		specularSourceMacro.Definition = "0";
		// From: Specular Lighting https://msdn.microsoft.com/en-us/library/windows/desktop/bb147399(v=vs.85).aspx
		// Note: If either specular material source option is used and the vertex color is not provided, then the material specular color is used.
		if (state.currentRenderStates.renderStatesUnion.namedStates.specularMaterialSource == D3DMCS_COLOR1 && state.CurrentStateHasInputVertexColor0() )
			specularSourceMacro.Definition = "1";
		else if (state.currentRenderStates.renderStatesUnion.namedStates.specularMaterialSource == D3DMCS_COLOR2 && state.CurrentStateHasInputVertexColor1() )
			specularSourceMacro.Definition = "2";
		defines.push_back(specularSourceMacro);

		D3DXMACRO ambientSourceMacro = {0};
		ambientSourceMacro.Name = "MATERIAL_AMBIENT_SOURCE";
		ambientSourceMacro.Definition = "0";
		// From: Ambient Lighting https://msdn.microsoft.com/en-us/library/windows/desktop/bb172256(v=vs.85).aspx
		// Note: If either AMBIENTMATERIALSOURCE option is used, and the vertex color is not provided, then the material ambient color is used.
		if (state.currentRenderStates.renderStatesUnion.namedStates.ambientMaterialSource == D3DMCS_COLOR1 && state.CurrentStateHasInputVertexColor0() )
		{
			ambientSourceMacro.Definition = "1";
		}
		else if (state.currentRenderStates.renderStatesUnion.namedStates.ambientMaterialSource == D3DMCS_COLOR2 && state.CurrentStateHasInputVertexColor1() )
		{
			ambientSourceMacro.Definition = "2";
		}
		defines.push_back(ambientSourceMacro);

		D3DXMACRO emissiveSourceMacro = {0};
		emissiveSourceMacro.Name = "MATERIAL_EMISSIVE_SOURCE";
		emissiveSourceMacro.Definition = "0";
		// From: Emissive Lighting https://msdn.microsoft.com/en-us/library/windows/desktop/bb173352(v=vs.85).aspx
		// Note: If either EMISSIVEMATERIALSOURCE option is used, and the vertex color is not provided, the material emissive color is used.
		if (state.currentRenderStates.renderStatesUnion.namedStates.emissiveMaterialSource == D3DMCS_COLOR1 && state.CurrentStateHasInputVertexColor0() )
			emissiveSourceMacro.Definition = "1";
		else if (state.currentRenderStates.renderStatesUnion.namedStates.emissiveMaterialSource == D3DMCS_COLOR2 && state.CurrentStateHasInputVertexColor1() )
			emissiveSourceMacro.Definition = "2";
		defines.push_back(emissiveSourceMacro);

		if (state.currentRenderStates.renderStatesUnion.namedStates.normalizeNormals)
		{
			D3DXMACRO normalizeNormalsMacro = {0};
			normalizeNormalsMacro.Name = "NORMALIZE_NORMALS";
			normalizeNormalsMacro.Definition = "1";
			defines.push_back(normalizeNormalsMacro);
		}
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.colorVertex)
	{
		D3DXMACRO colorVertexMacro = {0};
		colorVertexMacro.Name = "WITH_COLORVERTEX";
		colorVertexMacro.Definition = "1";
		defines.push_back(colorVertexMacro);
	}

	if (!defines.empty() )
	{
		D3DXMACRO emptyLastMacro = {0};
		defines.push_back(emptyLastMacro);
	}
}

static inline void BuildVertexShader(const DeviceState& state, IDirect3DDevice9Hook* const dev, IDirect3DVertexShader9Hook** const outNewShader)
{
#ifdef _DEBUG
	if (!outNewShader)
	{
		__debugbreak();
	}
#endif

	std::vector<D3DXMACRO> defines;
	BuildVertexStateDefines(state, defines);

	LPD3DXBUFFER outBytecode = NULL;
	LPD3DXBUFFER errorMessages = NULL;
	DWORD flags = D3DXSHADER_AVOID_FLOW_CONTROL; // Branching isn't currently well-supported in the software shader system
#ifdef _DEBUG
	flags |= D3DXSHADER_DEBUG;
#else
	flags |= D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif
	// TODO: Don't hardcode this path, it won't work on other people's computers as-is
	HRESULT hr = D3DXCompileShaderFromFileA("C:\\Users\\Tom\\Documents\\Visual Studio 2013\\Projects\\Software_d3d9\\trunk\\Software_d3d9\\FixedFunctionVertexShader.hlsl", defines.empty() ? NULL : &defines.front(), NULL,
		"main", "vs_3_0", flags, &outBytecode, &errorMessages, NULL);

	if (FAILED(hr) )
	{
		const char* const errorMessage = errorMessages ? ( (const char* const)errorMessages->GetBufferPointer() ) : NULL;
		OutputDebugStringA(errorMessage);
		OutputDebugStringA("\n");

		// Should never happen for fixed function shaders!
		__debugbreak();

		return;
	}

	IDirect3DVertexShader9* newVertShader = NULL;
	if (FAILED(dev->CreateVertexShader( (const DWORD* const)outBytecode->GetBufferPointer(), &newVertShader) ) || !newVertShader)
	{
		// Should never happen for fixed function shaders!
		__debugbreak();
	}

	IDirect3DVertexShader9Hook* newVertShaderHook = dynamic_cast<IDirect3DVertexShader9Hook*>(newVertShader);
	if (!newVertShaderHook)
	{
		DbgBreakPrint("Error: CreateVertexShader returned a non-hooked pointer!");
	}

	newVertShaderHook->GetModifyShaderInfo().fixedFunctionMacroDefines = defines;

	*outNewShader = newVertShaderHook;

	if (outBytecode)
	{
		outBytecode->Release();
		outBytecode = NULL;
	}
	if (errorMessages)
	{
		errorMessages->Release();
		errorMessages = NULL;
	}
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

static inline void BuildPixelStateDefines(const DeviceState& state, std::vector<D3DXMACRO>& defines)
{
	if (state.currentRenderStates.renderStatesUnion.namedStates.lighting)
	{
		D3DXMACRO lightingMacro = {0};
		lightingMacro.Name = "WITH_LIGHTING";
		lightingMacro.Definition = "1";
		defines.push_back(lightingMacro);

		if (state.currentRenderStates.renderStatesUnion.namedStates.specularEnable)
		{
			D3DXMACRO specularEnableMacro = {0};
			specularEnableMacro.Name = "SPECULAR_ENABLE";
			specularEnableMacro.Definition = "1";
			defines.push_back(specularEnableMacro);
		}
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.colorVertex)
	{
		D3DXMACRO colorVertexMacro = {0};
		colorVertexMacro.Name = "WITH_COLORVERTEX";
		colorVertexMacro.Definition = "1";
		defines.push_back(colorVertexMacro);
	}

	if (!defines.empty() )
	{
		D3DXMACRO emptyLastMacro = {0};
		defines.push_back(emptyLastMacro);
	}
}

static inline void BuildPixelShader(const DeviceState& state, IDirect3DDevice9Hook* const dev, IDirect3DPixelShader9Hook** const outNewShader)
{
#ifdef _DEBUG
	if (!outNewShader)
	{
		__debugbreak();
	}
#endif

	std::vector<D3DXMACRO> defines;
	BuildPixelStateDefines(state, defines);

	LPD3DXBUFFER outBytecode = NULL;
	LPD3DXBUFFER errorMessages = NULL;
	DWORD flags = D3DXSHADER_AVOID_FLOW_CONTROL; // Branching isn't currently well-supported in the software shader system
#ifdef _DEBUG
	flags |= D3DXSHADER_DEBUG;
#else
	flags |= D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif
	// TODO: Don't hardcode this path, it won't work on other people's computers as-is
	HRESULT hr = D3DXCompileShaderFromFileA("C:\\Users\\Tom\\Documents\\Visual Studio 2013\\Projects\\Software_d3d9\\trunk\\Software_d3d9\\FixedFunctionPixelShader.hlsl", defines.empty() ? NULL : &defines.front(), NULL,
		"main", "ps_3_0", flags, &outBytecode, &errorMessages, NULL);

	if (FAILED(hr) )
	{
		const char* const errorMessage = errorMessages ? ( (const char* const)errorMessages->GetBufferPointer() ) : NULL;
		printf("%s", errorMessage); // Don't optimize this away

		// Should never happen for fixed-function shaders!
		__debugbreak();

		return;
	}

	IDirect3DPixelShader9* newPixelShader = NULL;
	if (FAILED(dev->CreatePixelShader( (const DWORD* const)outBytecode->GetBufferPointer(), &newPixelShader) ) || !newPixelShader)
	{
		// Should never happen for fixed-function shaders!
		__debugbreak();
	}

	IDirect3DPixelShader9Hook* newPixelShaderHook = dynamic_cast<IDirect3DPixelShader9Hook*>(newPixelShader);
	if (!newPixelShaderHook)
	{
		DbgBreakPrint("Error: CreatePixelShader returned a non-hooked pointer!");
	}

	newPixelShaderHook->GetModifyShaderInfo().fixedFunctionMacroDefines = defines;

	*outNewShader = newPixelShaderHook;

	if (outBytecode)
	{
		outBytecode->Release();
		outBytecode = NULL;
	}
	if (errorMessages)
	{
		errorMessages->Release();
		errorMessages = NULL;
	}
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

void SetFixedFunctionVertexShaderState(const DeviceState& state, IDirect3DDevice9Hook* const dev)
{
	// WVP transform:
	const D3DXMATRIXA16& wvpTransform = state.currentTransforms.GetWVPTransform();
	dev->SetVertexShaderConstantF(252, (const float* const)&(wvpTransform), 4);

	// Material params:
	dev->SetVertexShaderConstantF(247, (const float* const)&(state.currentMaterial), 5);

	// Global ambient color:
	dev->SetPixelShaderConstantF(246, (const float* const)&state.currentRenderStates.cachedAmbient, 1);
}

void SetFixedFunctionPixelShaderState(const DeviceState& state, IDirect3DDevice9Hook* const dev)
{
	// Global ambient color:
	dev->SetPixelShaderConstantF(255, (const float* const)&state.currentRenderStates.cachedAmbient, 1);
}
