#pragma once

#include "IDirect3D9Hook.h"
#include "ShaderBase.h"
#include "VShaderEngine.h"
#include "PShaderEngine.h"
#include "DeviceState.h"
#include <vector>
#include <map>
#include <intrin.h>

#include "SimpleInstrumentedProfiler.h"

// 16 is the maximum number of vertex input streams supported by D3D9
#define MAX_D3D9_STREAMS 16u

class IDirect3D9Hook;
class IDirect3DVertexBuffer9Hook;
class IDirect3DIndexBuffer9Hook;
class IDirect3DVertexShader9Hook;
class IDirect3DPixelShader9Hook;
class IDirect3DVertexDeclaration9Hook;
class IDirect3DSurface9Hook;
class IDirect3DSwapChain9Hook;
class IDirect3DBaseTexture9Hook;
class IDirect3DTexture9Hook;
class IDirect3DCubeTexture9Hook;
class IDirect3DVolumeTexture9Hook;
class IDirect3DStateBlock9Hook;

struct DeclarationSemanticMapping;
struct VStoPSMapping;

#ifdef MULTITHREAD_SHADING
enum workerJobType
{
	vertexShade1Job = 0,

#ifdef RUN_SHADERS_IN_WARPS
	vertexShade4Job,
	//vertexShade16Job,
	//vertexShade64Job,
#endif // #ifdef RUN_SHADERS_IN_WARPS

	VERTEX_SHADE_JOB_MAX,

	pixelShade1Job,
#ifdef RUN_SHADERS_IN_WARPS
	pixelShade4Job,
	pixelShade16Job,
	pixelShade64Job,
#endif // #ifdef RUN_SHADERS_IN_WARPS

	PIXEL_SHADE_JOB_MAX,

	triangleRasterizeJob
};
#endif

struct DebuggableD3DVERTEXELEMENT9
{
	USHORT    Stream;     // Stream index
    USHORT    Offset;     // Offset in the stream in bytes
    D3DDECLTYPE Type : 8;       // Data type
    D3DDECLMETHOD    Method : 8;     // Processing method
    D3DDECLUSAGE    Usage : 8;      // Semantics
    DWORD    UsageIndex : 8; // Semantic index
};
static_assert(sizeof(D3DVERTEXELEMENT9) == sizeof(DebuggableD3DVERTEXELEMENT9), "Error!");

// A debuggable usage type for resource usages
enum DebuggableUsage : DWORD
{
	UsageNone = 0x00000000, // This is a valid usage, it is also the default usage
	UsageRENDERTARGET = D3DUSAGE_RENDERTARGET,
	UsageDEPTHSTENCIL = D3DUSAGE_DEPTHSTENCIL,
	UsageRESERVED0 = 0x00000004,
	UsageWRITEONLY = D3DUSAGE_WRITEONLY,
	UsageSOFTWAREPROCESSING = D3DUSAGE_SOFTWAREPROCESSING,
	UsageDONOTCLIP = D3DUSAGE_DONOTCLIP,
	UsagePOINTS = D3DUSAGE_POINTS,
	UsageRTPATCHES = D3DUSAGE_RTPATCHES,
	UsageNPATCHES = D3DUSAGE_NPATCHES,
	UsageDYNAMIC = D3DUSAGE_DYNAMIC,
	UsageAUTOGENMIPMAP = D3DUSAGE_AUTOGENMIPMAP,
	UsageRESTRICTEDCONTENT = D3DUSAGE_RESTRICTED_CONTENT,
	UsageRESTRICTSHAREDRESOURCEDRIVER = D3DUSAGE_RESTRICT_SHARED_RESOURCE_DRIVER,
	UsageRESTRICTSHAREDRESOURCE = D3DUSAGE_RESTRICT_SHARED_RESOURCE,
	UsageDMAP = D3DUSAGE_DMAP,
	UsageNONSECURE = D3DUSAGE_NONSECURE,
	UsageTEXTAPI = D3DUSAGE_TEXTAPI
};

static inline const bool operator!=(const D3DVIEWPORT9& lhs, const D3DVIEWPORT9& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(D3DVIEWPORT9) ) != 0;
}

struct StreamSource
{
	StreamSource() : vertexBuffer(NULL), streamOffset(0), streamDividerFrequency(1), streamStride(0)
	{
	}

	// This function is used during state block captures to clone this struct (and make sure that pointer-fixups and AddRef() calls properly occur)
	void CopyForCapture(const StreamSource& rhs);

	~StreamSource()
	{
		vertexBuffer = NULL;
		streamOffset = 0;
		streamDividerFrequency = 1;
		streamStride = 0;
	}

	IDirect3DVertexBuffer9Hook* vertexBuffer;
	UINT streamOffset; // This offset is in bytes
	UINT streamDividerFrequency;
	unsigned short streamStride; // This stream stride is in bytes
};

struct StreamDataTypeEndPointers
{
	StreamDataTypeEndPointers()
	{
		ResetEndPointers();
	}

	~StreamDataTypeEndPointers()
	{
		ResetEndPointers();
	}

	inline void ResetEndPointers()
	{
		memset(this, 0, sizeof(*this) );
		dirty = true;
	}

	inline void SetDirty()
	{
#ifdef _DEBUG
		for (unsigned x = 0; x < ARRAYSIZE(dataTypeStreamEnds); ++x)
			dataTypeStreamEnds[x] = NULL;
		streamEndAbsolute = NULL;
#endif
		dirty = true;
	}

	const void* dataTypeStreamEnds[MAXD3DDECLTYPE];
	const BYTE* streamEndAbsolute;
	bool dirty; // The dirty bit can be set either by calling SetStreamSource() on this stream, or by calling SetVertexDeclarataion() with a new decl (or calling SetFVF() with a new FVF, although SetFVF internally calls SetVertexDeclaration)
};

struct TexturePaletteEntry
{
	PALETTEENTRY entries[256];
};

// Guard band clip code bitmasks
#define D3DCS_GBLEFT	0x00001000L
#define D3DCS_GBRIGHT	0x00002000L
#define D3DCS_GBTOP		0x00004000L
#define D3DCS_GBBOTTOM	0x00008000L

struct DeviceState_ShaderRegisters
{ 
	DeviceState_ShaderRegisters()
	{
		memset(this, 0, sizeof(*this) );
	}

	float4 floats[4096]; // 4096 is the maximum number of shader constants per constant buffer in D3D11 and D3D12, which acts as a nice superset of D3D9 functionality. Also we'll need at least 2048 + 64 slots available for supporting D3DRS_INDEXEDVERTEXBLENDENABLE.
	BOOL bools[16];
	int4 ints[16];
};

enum textureStageArgument : DWORD
{
	TA_DIFFUSE = D3DTA_DIFFUSE,
	TA_CURRENT = D3DTA_CURRENT,
	TA_TEXTURE = D3DTA_TEXTURE,
	TA_TFACTOR = D3DTA_TFACTOR,
	TA_SPECULAR = D3DTA_SPECULAR,
	TA_TEMP = D3DTA_TEMP,
	TA_CONSTANT = D3DTA_CONSTANT,

	TA_COMPLEMENT_DIFFUSE = D3DTA_DIFFUSE | D3DTA_COMPLEMENT,
	TA_COMPLEMENT_CURRENT = D3DTA_CURRENT | D3DTA_COMPLEMENT,
	TA_COMPLEMENT_TEXTURE = D3DTA_TEXTURE | D3DTA_COMPLEMENT,
	TA_COMPLEMENT_TFACTOR = D3DTA_TFACTOR | D3DTA_COMPLEMENT,
	TA_COMPLEMENT_SPECULAR = D3DTA_SPECULAR | D3DTA_COMPLEMENT,
	TA_COMPLEMENT_TEMP = D3DTA_TEMP | D3DTA_COMPLEMENT,
	TA_COMPLEMENT_CONSTANT = D3DTA_CONSTANT | D3DTA_COMPLEMENT,

	TA_DIFFUSE_ALPHAREPLICATE = D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE,
	TA_CURRENT_ALPHAREPLICATE = D3DTA_CURRENT | D3DTA_ALPHAREPLICATE,
	TA_TEXTURE_ALPHAREPLICATE = D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE,
	TA_TFACTOR_ALPHAREPLICATE = D3DTA_TFACTOR | D3DTA_ALPHAREPLICATE,
	TA_SPECULAR_ALPHAREPLICATE = D3DTA_SPECULAR | D3DTA_ALPHAREPLICATE,
	TA_TEMP_ALPHAREPLICATE = D3DTA_TEMP | D3DTA_ALPHAREPLICATE,
	TA_CONSTANT_ALPHAREPLICATE = D3DTA_CONSTANT | D3DTA_ALPHAREPLICATE,

	TA_COMPLEMENT_DIFFUSE_ALPHAREPLICATE = D3DTA_DIFFUSE | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
	TA_COMPLEMENT_CURRENT_ALPHAREPLICATE = D3DTA_CURRENT | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
	TA_COMPLEMENT_TEXTURE_ALPHAREPLICATE = D3DTA_TEXTURE | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
	TA_COMPLEMENT_TFACTOR_ALPHAREPLICATE = D3DTA_TFACTOR | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
	TA_COMPLEMENT_SPECULAR_ALPHAREPLICATE = D3DTA_SPECULAR | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
	TA_COMPLEMENT_TEMP_ALPHAREPLICATE = D3DTA_TEMP | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
	TA_COMPLEMENT_CONSTANT_ALPHAREPLICATE = D3DTA_CONSTANT | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE
};

struct TextureStageState
{
	TextureStageState()
	{
		memset(this, 0, sizeof(*this) );
		SetStageDefaults(0);
	}

	~TextureStageState()
	{
		memset(this, 0, sizeof(*this) );
		SetStageDefaults(0);
	}

	inline void SetStageDefaults(const unsigned stageNum)
	{
		// All of the invalid state indices have their values set to "0xBAADCAFE" in D3D9 under Windows 10
		static const DWORD invalidTypeValue = 0xBAADCAFE;
		for (unsigned x = 0; x < D3DTSS_CONSTANT + 1; ++x)
			stageStateUnion.state[x] = invalidTypeValue;

		if (stageNum == 0)
		{
			stageStateUnion.namedStates.colorOp = D3DTOP_MODULATE;
			stageStateUnion.namedStates.alphaOp = D3DTOP_SELECTARG1;
		}
		else
		{
			stageStateUnion.namedStates.colorOp = D3DTOP_DISABLE;
			stageStateUnion.namedStates.alphaOp = D3DTOP_DISABLE;
		}
		stageStateUnion.namedStates.colorArg1 = TA_TEXTURE;
		stageStateUnion.namedStates.colorArg2 = TA_CURRENT;
		stageStateUnion.namedStates.alphaArg1 = TA_TEXTURE;
		stageStateUnion.namedStates.alphaArg2 = TA_CURRENT;
		stageStateUnion.namedStates.bumpEnvMat00 = 0.0f;
		stageStateUnion.namedStates.bumpEnvMat01 = 0.0f;
		stageStateUnion.namedStates.bumpEnvMat10 = 0.0f;
		stageStateUnion.namedStates.bumpEnvMat11 = 0.0f;
		stageStateUnion.namedStates.texCoordIndex = D3DTSS_TCI_PASSTHRU | stageNum;
		stageStateUnion.namedStates.bumpEnvLScale = 0.0f; // Shouldn't this default to 1.0f instead?
		stageStateUnion.namedStates.bumpEnvLOffset = 0.0f;
		stageStateUnion.namedStates.textureTransformFlags = D3DTTFF_DISABLE;
		stageStateUnion.namedStates.colorArg0 = TA_CURRENT;
		stageStateUnion.namedStates.alphaArg0 = TA_CURRENT;
		stageStateUnion.namedStates.resultArg = TA_CURRENT;
		stageStateUnion.namedStates.constant = D3DCOLOR_ARGB(0, 0, 0, 0);
	}

	union _stageStateUnion
	{
		struct _namedStates
		{
			DWORD padding0; // 0
			D3DTEXTUREOP colorOp; // 1
			textureStageArgument colorArg1; // 2
			textureStageArgument colorArg2; // 3
			D3DTEXTUREOP alphaOp; // 4
			textureStageArgument alphaArg1; // 5
			textureStageArgument alphaArg2; // 6
			float bumpEnvMat00; // 7
			float bumpEnvMat01; // 8
			float bumpEnvMat10; // 9
			float bumpEnvMat11; // 10
			UINT texCoordIndex; // 11
			DWORD emptyAddress; // 12 - Used to be "D3DTSS_ADDRESS" which was of type D3DTEXTUREADDRESS in D3D7. It was a combination of ADDRESSU and ADDRESSV before they were split out - see d3d7types.h
			DWORD emptySamplerStates[9]; // 13 thru 21 (used to be where sampler state data was stored in D3D8 - see d3d8types.h)
			float bumpEnvLScale; // 22
			float bumpEnvLOffset; // 23
			D3DTEXTURETRANSFORMFLAGS textureTransformFlags; // 24
			DWORD emptyAddressW; // 25 - Used to be "D3DTSS_ADDRESSW" in D3D8 - see d3d8types.h
			textureStageArgument colorArg0; // 26
			textureStageArgument alphaArg0; // 27
			textureStageArgument resultArg; // 28
			DWORD emptyUnknown29[3]; // 29 thru 31 - Unknown what these used to be for
			D3DCOLOR constant; // 32
		} namedStates;
		DWORD state[D3DTSS_CONSTANT + 1];
	} stageStateUnion;
	static_assert(sizeof(_stageStateUnion) == sizeof(DWORD) * (D3DTSS_CONSTANT + 1), "Error: Unexpected union size!");
};

struct TexturePaletteState
{
	TexturePaletteState() : currentPaletteIndex(0xFFFF), paletteEntries(NULL)
	{
	}

	void CaptureCopyTexturePaletteState(const TexturePaletteState& rhs)
	{
		currentPaletteIndex = rhs.currentPaletteIndex;
		if (rhs.paletteEntries)
		{
			paletteEntries = new std::vector<TexturePaletteEntry>();
			*paletteEntries = *(rhs.paletteEntries);
		}
		else
			paletteEntries = NULL;
	}

	~TexturePaletteState()
	{
		currentPaletteIndex = 0xFFFF;

		if (paletteEntries)
		{
			paletteEntries->clear();
			delete paletteEntries;
			paletteEntries = NULL;
		}
	}

	unsigned short currentPaletteIndex; // This is a USHORT because the D3D9 spec says that there's a maximum of 64k texture palettes
	std::vector<TexturePaletteEntry>* paletteEntries;
};

union RGB565
{
	struct _bits565
	{
		unsigned short b : 5;
		unsigned short g : 6;
		unsigned short r : 5;
	} bits565;

	unsigned short word;
};
static_assert(sizeof(RGB565) == sizeof(unsigned short), "Error! Unexpected struct size!");

union A4R4G4B4
{
	struct _bits4444
	{
		unsigned short b : 4;
		unsigned short g : 4;
		unsigned short r : 4;
		unsigned short a : 4;
	} bits4444;

	unsigned short word;
};
static_assert(sizeof(A4R4G4B4) == sizeof(unsigned short), "Error! Unexpected struct size!");

union X4R4G4B4
{
	struct _bits4440
	{
		unsigned short b : 4;
		unsigned short g : 4;
		unsigned short r : 4;
		unsigned short x : 4;
	} bits4440;

	unsigned short word;
};
static_assert(sizeof(X4R4G4B4) == sizeof(unsigned short), "Error! Unexpected struct size!");

__declspec(align(16) ) struct Transforms
{
#define MAX_WORLD_TRANSFORMS 256

	Transforms() : wvpDirty(true)
	{
		memset(this, 0, sizeof(*this) );

		static const unsigned numTransformsTotal = sizeof(*this) / sizeof(D3DXMATRIXA16);
		for (unsigned x = 0; x < numTransformsTotal; ++x)
		{
			D3DXMatrixIdentity(&ViewTransform + x);
		}
	}

	~Transforms()
	{
		memset(this, 0, sizeof(*this) );
	}

	inline void SetViewTransform(const D3DXMATRIXA16& newView)
	{
		wvpDirty = true;
		wvDirty[0] = true;
		wvDirty[1] = true;
		wvDirty[2] = true;
		wvDirty[3] = true;
		ViewTransform = newView;
	}

	inline void SetProjectionTransform(const D3DXMATRIXA16& newProj)
	{
		wvpDirty = true;
		ProjectionTransform = newProj;
	}

	inline void SetWorldTransform(const D3DXMATRIXA16& newWorld, const unsigned char worldIndex)
	{
		if (worldIndex < 4)
		{
			if (worldIndex == 0)
				wvpDirty = true;
			wvDirty[worldIndex] = true;
		}
		WorldTransforms[worldIndex] = newWorld;
	}

	inline void SetTextureTransform(const D3DXMATRIXA16& newTexture, const unsigned char textureIndex)
	{
#ifdef _DEBUG
		if (textureIndex >= 8)
		{
			__debugbreak();
		}
#endif
		TextureTransforms[textureIndex] = newTexture;
	}

	D3DXMATRIXA16 ViewTransform;
	D3DXMATRIXA16 ProjectionTransform;
	D3DXMATRIXA16 TextureTransforms[D3DDP_MAXTEXCOORD];
	D3DXMATRIXA16 WorldTransforms[MAX_WORLD_TRANSFORMS];

	inline void ComputeWVMatrixForCache(const unsigned char wvMatrixIndex) const
	{
		D3DXMATRIXA16& cachedWV = CachedWVTransform[wvMatrixIndex];
		cachedWV = WorldTransforms[wvMatrixIndex] * ViewTransform;
		D3DXMatrixInverse(&(CachedInvWVTransform[wvMatrixIndex]), NULL, &cachedWV);
	}

	inline const D3DXMATRIXA16& GetWVPTransform() const
	{
		if (wvDirty[0])
		{
			wvDirty[0] = false;
			ComputeWVMatrixForCache(0);
		}
		if (wvpDirty)
		{
			wvpDirty = false;
			CachedWVPTransform = CachedWVTransform[0] * ProjectionTransform;
		}
		return CachedWVPTransform;
	}

	inline const D3DXMATRIXA16& GetWVTransformFromCache(const unsigned char worldTransformIndex) const
	{
#ifdef _DEBUG
		if (worldTransformIndex >= 4)
		{
			__debugbreak();
		}
#endif
		if (bool& isThisWVDirty = wvDirty[worldTransformIndex])
		{
			isThisWVDirty = false;
			ComputeWVMatrixForCache(worldTransformIndex);
		}
		return CachedWVTransform[worldTransformIndex];
	}

	inline void GetWVTransform(const unsigned char worldTransformIndex, D3DXMATRIXA16& outWVMatrix) const
	{
		if (worldTransformIndex < 4)
			outWVMatrix = GetWVTransformFromCache(worldTransformIndex);
		else
			outWVMatrix = WorldTransforms[worldTransformIndex] * ViewTransform;
	}

	inline const D3DXMATRIXA16& GetInvWVTransformFromCache(const unsigned char worldTransformIndex) const
	{
#ifdef _DEBUG
		if (worldTransformIndex >= 4)
		{
			__debugbreak();
		}
#endif
		if (bool& isThisWVDirty = wvDirty[worldTransformIndex])
		{
			isThisWVDirty = false;
			ComputeWVMatrixForCache(worldTransformIndex);
		}
		return CachedInvWVTransform[worldTransformIndex];
	}

	inline void GetInvWVTransform(const unsigned char worldTransformIndex, D3DXMATRIXA16& outInvWVMatrix) const
	{
		if (worldTransformIndex < 4)
			outInvWVMatrix = GetInvWVTransformFromCache(worldTransformIndex);
		else
			D3DXMatrixInverse(&outInvWVMatrix, NULL, &(CachedWVTransform[worldTransformIndex]) );
	}

	mutable D3DXMATRIXA16 CachedWVPTransform; // This is a cached version of worldMatrix0 * viewMatrix * projMatrix
	mutable D3DXMATRIXA16 CachedWVTransform[4];
	mutable D3DXMATRIXA16 CachedInvWVTransform[4];
	mutable bool wvpDirty;
	mutable bool wvDirty[4];
};

struct LightInfo
{
	LightInfo() : activeLightIndex(-1)
	{
		memset(&light, 0, sizeof(light) );
		light.Type = D3DLIGHT_DIRECTIONAL;
		light.Diffuse.r = light.Diffuse.g = light.Diffuse.b = 1.0f;
		light.Direction.z = 1.0f;
	}

	D3DLIGHT9 light;
	INT activeLightIndex;

	// This isn't marked const, but it is global, so please do not modify its data
	static LightInfo defaultLight;
};

struct CachedViewport
{
	CachedViewport()
	{
		memset(this, 0, sizeof(*this) );
	}

	~CachedViewport()
	{
		memset(this, 0, sizeof(*this) );
	}

	inline void RecomputeCache()
	{
		halfWidthF = viewport.Width * 0.5f;
		halfHeightF = viewport.Height * 0.5f;
		fLeft = (const float)viewport.X;
		fTop = (const float)viewport.Y;
		fWidth = (const float)(viewport.Width - 1);
		fHeight = (const float)(viewport.Height - 1);
		zScale = viewport.MaxZ - viewport.MinZ;
	}

	D3DVIEWPORT9 viewport;
	float halfWidthF;
	float halfHeightF;
	float fLeft;
	float fTop;
	float fWidth;
	float fHeight;
	float zScale;
};

struct scissorRectStruct
{
	scissorRectStruct()
	{
		memset(this, 0, sizeof(*this) );
	}

	~scissorRectStruct()
	{
		memset(this, 0, sizeof(*this) );
	}

	RECT scissorRect;
	float fleft, fright, ftop, fbottom;
	__m128 topleftF;
	__m128 botrightF;

	inline void RecomputeScissorRect()
	{
		fleft = (const float)scissorRect.left;
		fright = (const float)scissorRect.right;
		ftop = (const float)scissorRect.top;
		fbottom = (const float)scissorRect.bottom;

		topleftF = _mm_set_ps(0.0f, 0.0f, ftop, fleft);
		botrightF = _mm_set_ps(0.0f, 0.0f, fbottom, fright);
	}
};

struct DeviceState
{
	DeviceState() : currentIndexBuffer(NULL), currentVertexShader(NULL), currentPixelShader(NULL), currentVertexDecl(NULL), declTarget(targetFVF), currentDepthStencil(NULL), currentNPatchMode(0.0f)
	{
		currentFVF.rawFVF_DWORD = 0x00000000;

		memset(&currentRenderTargets, 0, sizeof(currentRenderTargets) );
		memset(&currentTextures, 0, sizeof(currentTextures) );
		memset(&currentCubeTextures, 0, sizeof(currentTextures) );
		memset(&currentVolumeTextures, 0, sizeof(currentTextures) );
		memset(&currentMaterial, 0, sizeof(currentMaterial) ); // Looks like our material gets initialized to all zeroes (as per GetMaterial() right after device creation)
		memset(&enabledLightIndices, 0, sizeof(enabledLightIndices) );

		for (unsigned x = 0; x < 8; ++x)
			currentStageStates[x].SetStageDefaults(x);

		for (unsigned x = 0; x < D3DMAXUSERCLIPPLANES; ++x)
			currentClippingPlanes[x] = D3DXPLANE(0.0f, 0.0f, 0.0f, 0.0f);

		lightInfoMap = new std::map<UINT, LightInfo*>();
	}

	// Used during state block captures to copy the device state over
	void CaptureCopyState(const DeviceState& rhs);

#define MAX_NUM_SAMPLERS (D3DVERTEXTEXTURESAMPLER3 + 1)
#define MAX_NUM_TEXTURE_STAGE_STATES 8

	~DeviceState()
	{
		currentIndexBuffer = NULL;
		currentVertexShader = NULL;
		currentPixelShader = NULL;
		for (unsigned x = 0; x < MAX_NUM_SAMPLERS; ++x)
		{
			currentTextures[x] = NULL;
			currentCubeTextures[x] = NULL;
			currentVolumeTextures[x] = NULL;
		}

		for (unsigned x = 0; x < ARRAYSIZE(enabledLightIndices); ++x)
			enabledLightIndices[x] = NULL;

		for (unsigned x = 0; x < ARRAYSIZE(currentRenderTargets); ++x)
			currentRenderTargets[x] = NULL;
		currentDepthStencil = NULL;

		currentNPatchMode = 0.0f;

		if (lightInfoMap)
		{
			for (std::map<UINT, LightInfo*>::iterator it = lightInfoMap->begin(); it != lightInfoMap->end(); ++it)
			{
				delete it->second;
				it->second = NULL;
			}
			delete lightInfoMap;
			lightInfoMap = NULL;
		}

		declTarget = targetFVF;
		currentVertexDecl = NULL;

		currentFVF.rawFVF_DWORD = 0x00000000;
	}

	IDirect3DIndexBuffer9Hook* currentIndexBuffer;
	IDirect3DIndexBuffer9Hook* currentSoftUPIndexBuffer;

	StreamSource currentStreams[MAX_D3D9_STREAMS]; // 16 is MaxStreams from the D3D9 Caps
	StreamDataTypeEndPointers currentStreamEnds[MAX_D3D9_STREAMS]; // 16 is MaxStreams from the D3D9 Caps
	StreamSource currentSoftUPStream;
	StreamDataTypeEndPointers currentSoftUPStreamEnd;
	IDirect3DVertexShader9Hook* currentVertexShader;
	DeviceState_ShaderRegisters vertexShaderRegisters;
	IDirect3DPixelShader9Hook* currentPixelShader;
	DeviceState_ShaderRegisters pixelShaderRegisters;
	TexturePaletteState currentPaletteState;

	// TODO: Refactor the hooks inheritance tree to make this work with baseTextures so we don't need these duplicates:
	IDirect3DTexture9Hook* currentTextures[MAX_NUM_SAMPLERS];
	IDirect3DCubeTexture9Hook* currentCubeTextures[MAX_NUM_SAMPLERS];
	IDirect3DVolumeTexture9Hook* currentVolumeTextures[MAX_NUM_SAMPLERS];
	SamplerState currentSamplerStates[MAX_NUM_SAMPLERS];
	TextureStageState currentStageStates[MAX_NUM_TEXTURE_STAGE_STATES];

	D3DXPLANE currentClippingPlanes[D3DMAXUSERCLIPPLANES];

	D3DMATERIAL9 currentMaterial;
	float currentNPatchMode;

	__declspec(align(16) ) RenderStates currentRenderStates;

	std::map<UINT, LightInfo*>* lightInfoMap;
#define MAX_ENABLED_LIGHTS 32 // CAPS only goes up to 8, but we can do better
	LightInfo* enabledLightIndices[MAX_ENABLED_LIGHTS];

	// Transforms:
	Transforms currentTransforms;

	// TODO: Initialize the first render target to the backbuffer of the implicit swap chain
	IDirect3DSurface9Hook* currentRenderTargets[D3D_MAX_SIMULTANEOUS_RENDERTARGETS];
	IDirect3DSurface9Hook* currentDepthStencil; // TODO: Initialize the depth-stencil surface to the implicitly-created depth-stencil (if any)

	// Initialized inside of InitializeState() after CreateDevice() or Reset() is called successfully
	scissorRectStruct currentScissorRect;
	CachedViewport cachedViewport;

	enum _declTarget
	{
		targetFVF = 0,
		targetVertexDecl
	} declTarget; // Are we currently in FVF-mode or in vertex-decl mode?
	IDirect3DVertexDeclaration9Hook* currentVertexDecl;
	debuggableFVF currentFVF;

	// Returns true if the current vertex FVF or decl has a COLOR0 component, or false otherwise
	const bool CurrentStateHasInputVertexColor0(void) const;

	// Returns true if the current vertex FVF or decl has a COLOR1 component, or false otherwise
	const bool CurrentStateHasInputVertexColor1(void) const;
};

struct DeviceFrameStats
{
	DeviceFrameStats()
	{
		Clear();
	}

	inline void Clear()
	{
		memset(this, 0, sizeof(*this) );
	}

	unsigned numVertsProcessed;
	unsigned numTrianglesProcessed;
	unsigned numAlphaTestFailPixels;
	unsigned numPixelsWritten;
	unsigned numPixelsShaded;
	unsigned numPixelsTexkilled;
	unsigned numVertsReused;
};

struct drawCallVertexJobData
{
	drawCallVertexJobData() : userClipPlanesEnabled(false), mapping(NULL)
	{
	}

	bool userClipPlanesEnabled; // true = any user clip planes are enabled, false = none of them are enabled
	const DeclarationSemanticMapping* mapping;
};

struct drawCallPixelJobData
{
	drawCallPixelJobData() : useShaderVerts(false), offsetIntoVertexForOPosition_Bytes(0)
	{
		vs_to_ps_mappings.vs_psMapping = NULL;
		vs_to_ps_mappings.vertexDeclMapping = NULL;
	}

	bool useShaderVerts; // true = shadeFromShaderOutputRegs, false = shadeFromStream
	UINT offsetIntoVertexForOPosition_Bytes; // The number of bytes to get to "float4 oPos" into either a fully-transformed user-provided vertex (stream) or a vertex shader output register buffer (shader)
	union
	{
		const VStoPSMapping* vs_psMapping;
		const DeclarationSemanticMapping* vertexDeclMapping;
		const void* sourceAgnosticMapping;
	} vs_to_ps_mappings;
};

__declspec(align(16) ) struct primitivePixelJobData
{
	primitivePixelJobData() : invZ(0.0f, 0.0f, 0.0f), invW(0.0f, 0.0f, 0.0f), barycentricNormalizeFactor(0.0f), primitiveID(0), VFace(true), vertex0index(0), vertex1index(0), vertex2index(0)
	{
		pixelShadeVertexData.shadeFromShader.v0 = NULL;
		pixelShadeVertexData.shadeFromShader.v1 = NULL;
		pixelShadeVertexData.shadeFromShader.v2 = NULL;

		pixelShadeVertexData.shadeFromStream.v0 = NULL;
		pixelShadeVertexData.shadeFromStream.v1 = NULL;
		pixelShadeVertexData.shadeFromStream.v2 = NULL;
	}

	// This is: float3(1.0f / v0.z, 1.0f / v1.z, 1.0f / v2.z)
	__declspec(align(16) ) D3DXVECTOR3 invZ;

	// This is: float3(1.0f / v0.w, 1.0f / v1.w, 1.0f / v2.w)
	__declspec(align(16) ) D3DXVECTOR3 invW;

	union _pixelShadeVertexData
	{
		struct _shadeFromShader
		{
			const VS_2_0_OutputRegisters* v0;
			const VS_2_0_OutputRegisters* v1;
			const VS_2_0_OutputRegisters* v2;
		} shadeFromShader;
		struct _shadeFromStream
		{
			CONST BYTE* v0;
			CONST BYTE* v1;
			CONST BYTE* v2;
		} shadeFromStream;
		struct _shadeFromAgnostic
		{
			const void* v0;
			const void* v1;
			const void* v2;
		} shadeFromAgnostic;
	} pixelShadeVertexData;
	UINT vertex0index;
	UINT vertex1index;
	UINT vertex2index;
	float barycentricNormalizeFactor;
	UINT primitiveID; // This is the ps_4_0 SV_PrimitiveID semantic for this primitive (not used in D3D9, but included here as being useful for debugging)
	// UINT instanceID; // This is the ps_4_0 SV_InstanceID semantic for this primitive (not used in D3D9)
	bool VFace; // This is the ps_3_0 VFACE semantic or the ps_4_0 SV_IsFrontFace semantic for this primitive
};

struct drawCallTriangleRasterizeJobsData
{
	float fWidth, fHeight;
	union
	{
		const DeclarationSemanticMapping* vertexDeclMapping;
		const VStoPSMapping* vStoPSMapping;
	};
	bool rasterizerUsesEarlyZTest;
	bool rasterizeTriangleFromShader; // true = from shader, false = from stream
};

struct currentDrawCallJobData
{
	drawCallVertexJobData vertexData;

	// TODO: Mutable is gross, try and find another way to do this...
	mutable drawCallPixelJobData pixelData;

	// TODO: Mutable is gross, try and find another way to do this...
	mutable drawCallTriangleRasterizeJobsData triangleRasterizeData;
};

__declspec(align(16) ) class IDirect3DDevice9Hook : public IDirect3DDevice9
{
public:

	// This always has to be the first member variable in this class (or the initial memcpy() will fail)
#ifdef _DEBUG
	bool m_FirstMember;
#endif

	IDirect3DDevice9Hook(LPDIRECT3DDEVICE9 _d3d9dev, IDirect3D9Hook* _parentHook);

	virtual ~IDirect3DDevice9Hook();

	inline LPDIRECT3DDEVICE9 GetUnderlyingDevice(void) const
	{
		return d3d9dev;
	}

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DDevice9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE TestCooperativeLevel(THIS) override;
    virtual COM_DECLSPEC_NOTHROW UINT STDMETHODCALLTYPE GetAvailableTextureMem(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE EvictManagedResources(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDirect3D(THIS_ IDirect3D9** ppD3D9) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDeviceCaps(THIS_ D3DCAPS9* pCaps) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDisplayMode(THIS_ UINT iSwapChain, D3DDISPLAYMODE* pMode) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetCreationParameters(THIS_ D3DDEVICE_CREATION_PARAMETERS* pParameters) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetCursorProperties(THIS_ UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) override;
    virtual COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE SetCursorPosition(THIS_ int X, int Y, DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW BOOL STDMETHODCALLTYPE ShowCursor(THIS_ BOOL bShow) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetSwapChain(THIS_ UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) override;
    virtual COM_DECLSPEC_NOTHROW UINT STDMETHODCALLTYPE GetNumberOfSwapChains(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Reset(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Present(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetBackBuffer(THIS_ UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetRasterStatus(THIS_ UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetDialogBoxMode(THIS_ BOOL bEnableDialogs) override;
    virtual COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE SetGammaRamp(THIS_ UINT iSwapChain,DWORD Flags, CONST D3DGAMMARAMP* pRamp) override;
    virtual COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE GetGammaRamp(THIS_ UINT iSwapChain, D3DGAMMARAMP* pRamp) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateTexture(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateVolumeTexture(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateCubeTexture(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateVertexBuffer(THIS_ UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateIndexBuffer(THIS_ UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateRenderTarget(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE UpdateSurface(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE UpdateTexture(THIS_ IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetRenderTargetData(THIS_ IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetFrontBufferData(THIS_ UINT iSwapChain, IDirect3DSurface9* pDestSurface) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE StretchRect(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE ColorFill(THIS_ IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetRenderTarget(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetRenderTarget(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(THIS_ IDirect3DSurface9* pNewZStencil) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(THIS_ IDirect3DSurface9** ppZStencilSurface) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE BeginScene(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE EndScene(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Clear(THIS_ DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetTransform(THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetTransform(THIS_ D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE MultiplyTransform(THIS_ D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetViewport(THIS_ CONST D3DVIEWPORT9* pViewport) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetViewport(THIS_ D3DVIEWPORT9* pViewport) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetMaterial(THIS_ CONST D3DMATERIAL9* pMaterial) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetMaterial(THIS_ D3DMATERIAL9* pMaterial) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetLight(THIS_ DWORD Index, CONST D3DLIGHT9*) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetLight(THIS_ DWORD Index, D3DLIGHT9*) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE LightEnable(THIS_ DWORD Index, BOOL Enable) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetLightEnable(THIS_ DWORD Index, BOOL* pEnable) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetClipPlane(THIS_ DWORD Index, CONST float* pPlane) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetClipPlane(THIS_ DWORD Index, float* pPlane) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetRenderState(THIS_ D3DRENDERSTATETYPE State, DWORD Value) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetRenderState(THIS_ D3DRENDERSTATETYPE State, DWORD* pValue) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateStateBlock(THIS_ D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE BeginStateBlock(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE EndStateBlock(THIS_ IDirect3DStateBlock9** ppSB) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetClipStatus(THIS_ CONST D3DCLIPSTATUS9* pClipStatus) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetClipStatus(THIS_ D3DCLIPSTATUS9* pClipStatus) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetTexture(THIS_ DWORD Stage, IDirect3DBaseTexture9** ppTexture) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetTexture(THIS_ DWORD Stage, IDirect3DBaseTexture9* pTexture) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetTextureStageState(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetTextureStageState(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetSamplerState(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetSamplerState(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE ValidateDevice(THIS_ DWORD* pNumPasses) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetPaletteEntries(THIS_ UINT PaletteNumber, CONST PALETTEENTRY* pEntries) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPaletteEntries(THIS_ UINT PaletteNumber, PALETTEENTRY* pEntries) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(THIS_ UINT PaletteNumber) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(THIS_ UINT *PaletteNumber) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetScissorRect(THIS_ CONST RECT* pRect) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetScissorRect(THIS_ RECT* pRect) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(THIS_ BOOL bSoftware) override;
    virtual COM_DECLSPEC_NOTHROW BOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetNPatchMode(THIS_ float nSegments) override;
    virtual COM_DECLSPEC_NOTHROW float STDMETHODCALLTYPE GetNPatchMode(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE DrawPrimitive(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(THIS_ D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE ProcessVertices(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetVertexDeclaration(THIS_ IDirect3DVertexDeclaration9* pDecl) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetVertexDeclaration(THIS_ IDirect3DVertexDeclaration9** ppDecl) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetFVF(THIS_ DWORD FVF) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetFVF(THIS_ DWORD* pFVF) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateVertexShader(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetVertexShader(THIS_ IDirect3DVertexShader9* pShader) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetVertexShader(THIS_ IDirect3DVertexShader9** ppShader) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(THIS_ UINT StreamNumber, UINT Setting) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(THIS_ UINT StreamNumber, UINT* pSetting) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetIndices(THIS_ IDirect3DIndexBuffer9* pIndexData) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetIndices(THIS_ IDirect3DIndexBuffer9** ppIndexData) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreatePixelShader(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetPixelShader(THIS_ IDirect3DPixelShader9* pShader) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPixelShader(THIS_ IDirect3DPixelShader9** ppShader) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE DrawRectPatch(THIS_ UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE DrawTriPatch(THIS_ UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE DeletePatch(THIS_ UINT Handle) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE CreateQuery(THIS_ D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) override;

	// This is not an official D3D9 function, even though it looks like one. It is only used internally.
	COM_DECLSPEC_NOTHROW HRESULT CreateVertexDeclarationFromFVF(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl, const debuggableFVF FVF);

	// If indexBuffer is NULL, then synthesize an index buffer (0, 1, 2, 3, 4, etc...)
	template <const bool useVertexBuffer, const bool useIndexBuffer>
	void ProcessVerticesToBuffer(const IDirect3DVertexDeclaration9Hook* const decl, const DeclarationSemanticMapping& mapping, const IDirect3DIndexBuffer9Hook* const indexBuffer, 
		const D3DPRIMITIVETYPE PrimitiveType, const INT BaseVertexIndex, const UINT MinVertexIndex, const UINT startIndex, const UINT primCount, const void* const vertStreamBytes, const unsigned short vertStreamStride) const;

	// If indexBuffer is NULL, then synthesize an index buffer (0, 1, 2, 3, 4, etc...)
	template <const bool useVertexBuffer, const D3DFORMAT indexFormat>
	void ProcessVerticesToBufferInner(const IDirect3DVertexDeclaration9Hook* const decl, const DeclarationSemanticMapping& mapping, const BYTE* const indexBuffer,
		const D3DPRIMITIVETYPE PrimitiveType, const INT BaseVertexIndex, const UINT MinVertexIndex, const UINT startIndex, const UINT primCount, const void* const vertStreamBytes, const unsigned short vertStreamStride) const;

	// Process a single vertex:
	template <const bool anyUserClipPlanesEnabled>
	void ProcessVertexToBuffer(const DeclarationSemanticMapping& mapping, VShaderEngine* const vertShader, VS_2_0_OutputRegisters* const outputVert, const unsigned vertexIndex) const;
	void LoadVertexInputElement(const DebuggableD3DVERTEXELEMENT9& element, const unsigned char* const dataPtr, const unsigned registerIndex, VShaderEngine* const vertShader) const;

#ifdef RUN_SHADERS_IN_WARPS
	// Process a quad of vertices:
	template <const bool anyUserClipPlanesEnabled>
	void ProcessVertexToBuffer4(const DeclarationSemanticMapping& mapping, VShaderEngine* const vertShader, VS_2_0_OutputRegisters* (&outputVerts)[4], const unsigned* const vertexIndex) const;
	void LoadVertexInputElement4(const DebuggableD3DVERTEXELEMENT9& element, const unsigned char* dataPtr[4], const unsigned registerIndex, VShaderEngine* const vertShader) const;
#endif // #ifdef RUN_SHADERS_IN_WARPS

	// void StoreVertexOutputElement(const DebuggableD3DVERTEXELEMENT9& element, unsigned char* const outputPtr, const D3DDECLUSAGE usage, const unsigned usageIndex) const;

	template <const bool useIndexBuffer, typename indexFormat, const bool rasterizerUsesEarlyZTest>
	void DrawPrimitiveUBPretransformedSkipVS(const D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT startIndex, UINT primCount) const;

	template <const bool rasterizerUsesEarlyZTest>
	void DrawPrimitiveUB(const D3DPRIMITIVETYPE PrimitiveType, const UINT PrimitiveCount) const;

	// Returns true for "should draw", or false for "should skip"
	const bool TotalDrawCallSkipTest(void) const;

	static const bool ShouldCullEntireTriangle(const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2) ;
	static const bool ShouldCullEntireLine(const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1);
	static const bool ShouldCullEntirePoint(const VS_2_0_OutputRegisters& v0);

	// True if using FVF, false if using vertex declaration
	inline const bool IsUsingFVF(void) const { return currentState.declTarget == DeviceState::targetFVF; }

	// Returns true if either the FVF or the vertex declaration denotes that it contains pretransformed vertices
	const bool SkipVertexProcessing(void) const;

	inline DeviceState& GetCurrentHookState(void) 
	{ 
		return currentState;
	}

	inline const DeviceState& GetCurrentHookState(void) const
	{ 
		return currentState;
	}

	inline const DWORD OcclusionQuery_GetNumPixelsPassedZTest(void) const
	{
		return numPixelsPassedZTest;
	}

	// Called after device creation or Reset()
	void InitializeState(const D3DPRESENT_PARAMETERS& d3dpp, const D3DDEVTYPE devType, const DWORD createFlags, const HWND focusWindow);

	// Returns true if the device is currently inside of a BeginScene()/EndScene() block
	inline const bool HasBegunScene(void) const
	{
		return sceneBegun > 0;
	}

	// In the case of a successful render-target set, the viewport is automatically resized to the
	// size of the largest set render-target:
	void AutoResizeViewport(void);

	// Returns true if the currently set pipeline can do early-Z testing, or false if it cannot (false if depth isn't enabled, or no depth buffer is bound, or the pixel shader outputs depth)
	const bool CurrentPipelineCanEarlyZTest(void) const;

	// Assumes pre-transformed vertices from a vertex declaration + raw vertex stream
	void RasterizeLineFromStream(const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0, CONST BYTE* const v1) const;

	// Assumes pre-transformed vertex from a vertex declaration + raw vertex stream
	void RasterizePointFromStream(const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0) const;

	// Assumes pre-transformed vertices (from a processed vertex shader or from a vertex declaration + pretransformed vertex stream)
	template <const bool rasterizerUsesEarlyZTest, const bool shadeFromShader>
	void RasterizeTriangle(PShaderEngine* const pShaderEngine, const void* const mappingData, const void* const v0, const void* const v1, const void* const v2,
		const float fWidth, const float fHeight, const UINT primitiveID, const UINT vertex0index, const UINT vertex1index, const UINT vertex2index) const;

	// Assumes pre-transformed vertices from a processed vertex shader
	void RasterizeLineFromShader(const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1) const;

	// Assumes pre-transformed vertex from a processed vertex shader
	void RasterizePointFromShader(const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0) const;

	// Handles pixel setup and depth and attribute interpolation before shading the pixel
	template <const bool setupFromShader>
	void SetupPixel(PShaderEngine* const pixelEngine, const void* const shaderOrStreamMapping, const unsigned x, const unsigned y, const __m128 barycentricInterpolants, 
		const UINT offsetBytesToOPosition, const void* const v0, const void* const v1, const void* const v2, const __m128 invZ, const __m128 invW) const;

	// Handles pixel quad setup and depth and attribute interpolation before shading the pixel quad
	template <const bool setupFromShader>
	void SetupPixel4(PShaderEngine* const pixelEngine, const void* const shaderOrStreamMapping, const __m128i x4, const __m128i y4, const __m128 (&barycentricInterpolants)[4], 
		const UINT offsetBytesToOPosition, const void* const v0, const void* const v1, const void* const v2, const __m128 invZ, const __m128 invW) const;

	// Handles blending and write-masking
	void RenderOutput(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const;

	// Handles blending and write-masking
	template <const unsigned char pixelWriteMask>
	void RenderOutput4(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const;

	// Handles blending and write-masking
	template <const unsigned char channelWriteMask>
	void ROPBlendWriteMask(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const;

	template <const unsigned char channelWriteMask>
	void ROPBlendWriteMask_AlphaBlendTest(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const;

	template <const unsigned char channelWriteMask>
	void ROPBlendWriteMask_AlphaBlend(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const;

	template <const unsigned char channelWriteMask>
	void ROPBlendWriteMask_NoAlphaBlend(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const;

	// Handles blending and write-masking
	template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
	void ROPBlendWriteMask4(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const;

	template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
	void ROPBlendWriteMask4_AlphaBlendTest(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const;

	template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
	void ROPBlendWriteMask4_AlphaBlend(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const;

	template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
	void ROPBlendWriteMask4_NoAlphaBlend(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const;

	// Handles interpolating pixel shader input registers from a vertex declaration + raw vertex stream
	void InterpolateStreamIntoRegisters(PShaderEngine* const pixelShader, const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, 
		const __m128 floatBarycentricsInvW_X, const __m128 floatBarycentricsInvW_Y, const __m128 floatBarycentricsInvW_Z, const __m128 barycentricInterpolants, const float interpolatedPixelW) const;

	// Handles interpolating pixel shader input registers from a vertex declaration + raw vertex stream
	template <const unsigned char pixelWriteMask>
	void InterpolateStreamIntoRegisters4(PShaderEngine* const pixelShader, const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, 
		const __m128 floatBarycentricsInvW_X, const __m128 floatBarycentricsInvW_Y, const __m128 floatBarycentricsInvW_Z, const __m128 (&barycentricInterpolants)[4], const __m128 interpolatedPixelW4) const;

	// Handles interpolating pixel shader input registers from vertex shader output registers
	void InterpolateShaderIntoRegisters(PShaderEngine* const pixelShader, const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, 
		const __m128 floatBarycentricsInvW_X, const __m128 floatBarycentricsInvW_Y, const __m128 floatBarycentricsInvW_Z, const __m128 barycentricInterpolants, const float interpolatedPixelW) const;

	// Handles interpolating pixel shader input registers from vertex shader output registers
	template <const unsigned char pixelWriteMask>
	void InterpolateShaderIntoRegisters4(PShaderEngine* const pixelShader, const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, 
		const __m128 floatBarycentricsInvW_X, const __m128 floatBarycentricsInvW_Y, const __m128 floatBarycentricsInvW_Z, const __m128 (&barycentricInterpolants)[4], const __m128 interpolatedPixelW4) const;

	const float InterpolatePixelDepth(const __m128 barycentricInterpolants, const __m128 invZ) const;
	void InterpolatePixelDepth4(const __m128 (&barycentricInterpolants4)[4], const __m128 invZ, __m128& outPixelDepth4) const;

	// Must be called before shading a pixel to reset the pixel shader state machine!
	void PreShadePixel(const unsigned x, const unsigned y, PShaderEngine* const pixelShader) const;

	// Must be called before shading a pixel to reset the pixel shader state machine!
	void PreShadePixel4(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const;

	void ShadePixel_FailStencil(const unsigned x, const unsigned y) const;
	void ShadePixel_FailDepth(const unsigned x, const unsigned y) const;
	void ShadePixel_RunShader(const unsigned x, const unsigned y, PShaderEngine* const pixelShader) const;
	void PostShadePixel_DiscardTest(const unsigned x, const unsigned y, const PS_2_0_OutputRegisters& pixelShaderOutput) const;
	void PostShadePixel_AlphaTest(const unsigned x, const unsigned y, const PS_2_0_OutputRegisters& pixelShaderOutput) const;
	void PostShadePixel_WriteOutput(const unsigned x, const unsigned y, const PS_2_0_OutputRegisters& pixelShaderOutput) const;
	void PostShadePixel_Discard(const unsigned x, const unsigned y) const;
	void PostShadePixel_FailAlphaTest(const unsigned x, const unsigned y) const;
	void PostShadePixel_WriteOutputColor(const unsigned x, const unsigned y, const PS_2_0_OutputRegisters& pixelOutputColor) const;
	void PostShadePixel_WriteOutputDepth(const unsigned x, const unsigned y, const float pixelOutputDepth) const;
	void PostShadePixel_WriteOutputStencil(const unsigned x, const unsigned y) const;

	template <const unsigned char pixelWriteMask>
	void ShadePixel4_FailStencil(const __m128i x4, const __m128i y4) const;

	template <const unsigned char pixelWriteMask>
	void ShadePixel4_FailDepth(const __m128i x4, const __m128i y4) const;

	template <const unsigned char pixelWriteMask>
	void ShadePixel4_RunShader(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const;

	template <const unsigned char pixelWriteMask>
	void PostShadePixel4_DiscardTest(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const;

	template <const unsigned char pixelWriteMask>
	void PostShadePixel4_AlphaTest(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const;

	template <const unsigned char pixelWriteMask>
	void PostShadePixel4_WriteOutput(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const;

	void PostShadePixel4_Discard(const unsigned char pixelDiscardMask) const;

	void PostShadePixel4_FailAlphaTest(const unsigned char pixelsFailAlphaTestMask) const;

	template <const unsigned char pixelWriteMask>
	void PostShadePixel4_WriteOutputColor(const __m128i x4, const __m128i y4, const PShaderEngine* const pixelShader) const;

	template <const unsigned char pixelWriteMask>
	void PostShadePixel4_WriteOutputDepth(const __m128i x4, const __m128i y4, const PShaderEngine* const pixelShader) const;

	template <const unsigned char pixelWriteMask>
	void PostShadePixel4_WriteOutputStencil(const __m128i x4, const __m128i y4) const;

	template <const unsigned char channelWriteMask>
	void LoadBlend(D3DXVECTOR4& outBlend, const D3DBLEND blendMode, const D3DXVECTOR4& srcColor, const D3DXVECTOR4& dstColor) const;

	template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
	void LoadBlend4(D3DXVECTOR4 (&outBlend)[4], const D3DBLEND blendMode, const D3DXVECTOR4 (&srcColor)[4], const D3DXVECTOR4 (&dstColor)[4]) const;

	template <const unsigned char channelWriteMask>
	void AlphaBlend(D3DXVECTOR4& outVec, const D3DBLENDOP blendOp, const D3DXVECTOR4& srcBlend, const D3DXVECTOR4& dstBlend, const D3DXVECTOR4& srcColor, const D3DXVECTOR4& dstColor) const;

	template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
	void AlphaBlend4(D3DXVECTOR4 (&outVec)[4], const D3DBLENDOP blendOp, const D3DXVECTOR4 (&srcBlend)[4], const D3DXVECTOR4 (&dstBlend)[4], const D3DXVECTOR4 (&srcColor)[4], const D3DXVECTOR4 (&dstColor)[4]) const;

	void InitVertexShader(const DeviceState& deviceState, const ShaderInfo& vertexShaderInfo) const;
	void InitPixelShader(const DeviceState& deviceState, const ShaderInfo& pixelShaderInfo) const;

#ifdef MULTITHREAD_SHADING
	void CreateNewVertexShadeJob(VS_2_0_OutputRegisters* const * const outputRegs, const unsigned* const vertexIndices, const workerJobType jobWidth) const;

#if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
	void CreateNewTriangleRasterJob(const UINT primitiveID, const UINT vertID0, const UINT vertID1, const UINT vertID2, const bool rasterizeFromShader, const void* const vert0, const void* const vert1, const void* const vert2) const;
#endif // #if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS

#endif // #ifdef MULTITHREAD_SHADING

	void CreateNewPixelShadeJob(const unsigned x, const unsigned y, const __m128i barycentricAdjusted, const primitivePixelJobData* const primitiveData) const;
#ifdef RUN_SHADERS_IN_WARPS
	void CreateNewPixelShadeJob4(const __m128i x4, const __m128i y4, const __m128i (&barycentricsAdjusted4)[4], const primitivePixelJobData* const primitiveData) const;
#endif

	// TODO: Find another way to do this other than mutable
	mutable __declspec(align(16) ) primitivePixelJobData allPrimitiveJobData[1024 * 1024];

	template <const bool shadeFromShader>
	const primitivePixelJobData* const GetNewPrimitiveJobData(const void* const v0, const void* const v1, const void* const v2, const float barycentricNormalizeFactor, const UINT primitiveID, const bool VFace,
		const UINT vertex0index, const UINT vertex1index, const UINT vertex2index, const __m128 p0, const __m128 p1, const __m128 p2) const;

	// true = "pass" (draw the pixel), false = "fail" (discard the pixel)
	const bool StencilTestNoWrite(const unsigned x, const unsigned y) const;

	// true = "pass" (draw the pixel), false = "fail" (discard the pixel)
	// This MSDN page says that alpha testing only happens against the alpha value from oC0: https://docs.microsoft.com/en-us/windows/desktop/direct3d9/multiple-render-targets
	const bool AlphaTest(const D3DXVECTOR4& outColor) const;

	// Returns a SSE vector mask (0xFF for "test pass" and 0x00 for "test fail")
	// This MSDN page says that alpha testing only happens against the alpha value from oC0: https://docs.microsoft.com/en-us/windows/desktop/direct3d9/multiple-render-targets
	template <const unsigned char pixelWriteMask>
	const __m128 AlphaTest4(const PS_2_0_OutputRegisters (&outColor4)[4]) const;

#ifdef MULTITHREAD_SHADING
	void InitThreadQueue();
#endif

	void StencilOperation(const unsigned x, const unsigned y, const D3DSTENCILOP stencilOp) const;
	void StencilFailOperation(const unsigned x, const unsigned y) const;
	void StencilZFailOperation(const unsigned x, const unsigned y) const;
	void StencilPassOperation(const unsigned x, const unsigned y) const;

	void StencilOperation4(const __m128i x4, const __m128i y4, const D3DSTENCILOP stencilOp) const;
	void StencilFailOperation4(const __m128i x4, const __m128i y4) const;
	void StencilZFailOperation4(const __m128i x4, const __m128i y4) const;
	void StencilPassOperation4(const __m128i x4, const __m128i y4) const;

	void RecomputeCachedStreamEndsIfDirty();
	void RecomputeCachedStreamEndsForUP(const BYTE* const stream0Data, const unsigned numVertices, const USHORT vertexStride);

	void PreReset(void);

	COM_DECLSPEC_NOTHROW void SetupCurrentDrawCallVertexData(const DeclarationSemanticMapping& mapping);
	COM_DECLSPEC_NOTHROW void SetupCurrentDrawCallPixelData(const bool useShaderVerts, const void* const vs_to_ps_mapping) const;
	COM_DECLSPEC_NOTHROW void SetupCurrentDrawCallTriangleRasterizeData(const float fWidth, const float fHeight, const bool rasterizerUsesEarlyZTest, const bool rasterizeTriangleFromShader, const void* const interpolantDeclInfo) const;

	void ApplyViewportTransform(D3DXVECTOR4& positionT) const;

	COM_DECLSPEC_NOTHROW static void ModifyDeviceCaps(D3DCAPS9& caps);

	template <const bool anyUserClipPlanesEnabled>
	void ComputeVertexClipCodes(const D3DXVECTOR4& vertexPosition, VS_2_0_OutputRegisters* const shadedVertex) const;

#ifdef RUN_SHADERS_IN_WARPS
	void ApplyViewportTransform4(D3DXVECTOR4* (&positionT4)[4]) const;

	template <const bool anyUserClipPlanesEnabled>
	void ComputeVertexClipCodes4(const D3DXVECTOR4* (&vertexPosition4)[4], VS_2_0_OutputRegisters* const (&shadedVerts)[4]) const;
#endif // #ifdef RUN_SHADERS_IN_WARPS

	// Called from initial device creation and from device reset to modify the present params:
	static void ModifyPresentParameters(D3DPRESENT_PARAMETERS& inOutStruct);

	// This is not an official D3D9 function, even though it looks like one. It is only used internally.
	IDirect3DVertexDeclaration9Hook* CreateVertexDeclFromFVFCode(const debuggableFVF FVF);

	mutable DeviceFrameStats frameStats;

	currentDrawCallJobData currentDrawCallData;

	inline const bool IsCurrentlyRecordingStateBlock() const
	{
		return currentlyRecordingStateBlock != NULL;
	}

	_Acquires_lock_(&deviceCS) inline void LockDeviceCS(void)
	{
		EnterCriticalSection(&deviceCS);
	}

	_Releases_lock_(&deviceCS) inline void UnlockDeviceCS(void)
	{
		LeaveCriticalSection(&deviceCS);
	}

	inline const D3DPRESENT_PARAMETERS& GetInternalPresentParams() const
	{
		return currentPresentParams;
	}

protected:
	LPDIRECT3DDEVICE9 d3d9dev;
	IDirect3D9Hook* parentHook;
	unsigned __int64 refCount;

	// Flags outside of state blocks:
	BOOL sceneBegun;

	// These flags are not modified on device-reset:
	D3DDEVTYPE initialDevType;
	DWORD initialCreateFlags;
	HWND initialCreateFocusWindow;
	HWND initialCreateDeviceWindow;
	BOOL enableDialogs;

	D3DPRESENT_PARAMETERS currentPresentParams;

	mutable __declspec(align(16) ) VShaderEngine deviceMainVShaderEngine;
	mutable __declspec(align(16) ) PShaderEngine deviceMainPShaderEngine;
	mutable __declspec(align(16) ) VS_2_0_ConstantsBuffer vsDrawCallCB;
	mutable __declspec(align(16) ) PS_2_0_ConstantsBuffer psDrawCallCB;

#ifndef NO_CACHING_FVF_VERT_DECLS
	std::map<DWORD, IDirect3DVertexDeclaration9Hook*>* FVFToVertDeclCache;
#endif // NO_CACHING_FVF_VERT_DECLS

	__declspec(align(16) ) DeviceState currentState;

	IDirect3DStateBlock9Hook* currentlyRecordingStateBlock;

	BOOL currentSwvpEnabled; // Note that this parameter is not supposed to be recorded by State Blocks (which is why it's not inside the DeviceState struct)

	// This is the implicit swap chain:
	IDirect3DSwapChain9Hook* implicitSwapChain;

	// For debug-printing efficiently
	HANDLE hConsoleHandle;

	std::vector<unsigned>* alreadyShadedVerts32;
	std::vector<unsigned short>* alreadyShadedVerts16;

	// This is the data that gets directly consumed by occlusion queries (D3DQUERYTYPE_OCCLUSION).
	// Aligned and volatile here because this variable is accessed by multiple threads using Interlocked operations
	volatile __declspec(align(16) ) DWORD numPixelsPassedZTest;

	// Saving a pointer to a persistent buffer on the device here to avoid reallocation of output vertex buffers all the time
	mutable VS_2_0_OutputRegisters* processedVertexBuffer;
	mutable unsigned processedVertsUsed;
	mutable unsigned processVertsAllocated;

	CRITICAL_SECTION deviceCS;
};

static const __m128 ColorDWORDToFloat4Divisor = { 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f };

template <const unsigned char writeMask = 0xF>
inline void ColorDWORDToFloat4(const D3DCOLOR inColor, D3DXVECTOR4& outColor)
{
	const __m128i colorbyte4 = _mm_castps_si128(_mm_load_ss( (const float* const)&inColor) );
	const __m128i coloruint4 = _mm_cvtepu8_epi32(colorbyte4);
	const __m128 colorfloat4 = _mm_cvtepi32_ps(coloruint4);
	const __m128 normalizedColorFloat4 = _mm_mul_ps(colorfloat4, ColorDWORDToFloat4Divisor);

	// Swizzle from ARGB -> RGBA
	const __m128 swizzledColorFloat4 = _mm_shuffle_ps(normalizedColorFloat4, normalizedColorFloat4, _MM_SHUFFLE(3, 0, 1, 2) );

	if (writeMask == 0xF)
	{
		*( (__m128* const)&outColor) = swizzledColorFloat4;
	}
	else
	{
		if (writeMask & 0x1)
			outColor.x = swizzledColorFloat4.m128_f32[0];
		if (writeMask & 0x2)
			outColor.y = swizzledColorFloat4.m128_f32[1];
		if (writeMask & 0x4)
			outColor.z = swizzledColorFloat4.m128_f32[2];
		if (writeMask & 0x8)
			outColor.w = swizzledColorFloat4.m128_f32[3];
	}
}

template <const unsigned char writeMask = 0xF>
inline void ColorDWORDToFloat4_4(const D3DCOLOR* const * const inColor4, D3DXVECTOR4* const outColor4[4])
{
	const __m128i colorbyte4[4] = 
	{
		_mm_castps_si128(_mm_load_ss( (const float* const)inColor4[0]) ),
		_mm_castps_si128(_mm_load_ss( (const float* const)inColor4[1]) ),
		_mm_castps_si128(_mm_load_ss( (const float* const)inColor4[2]) ),
		_mm_castps_si128(_mm_load_ss( (const float* const)inColor4[3]) )
	};

	const __m128i coloruint4[4] = 
	{
		_mm_cvtepu8_epi32(colorbyte4[0]),
		_mm_cvtepu8_epi32(colorbyte4[1]),
		_mm_cvtepu8_epi32(colorbyte4[2]),
		_mm_cvtepu8_epi32(colorbyte4[3])
	};

	const __m128 colorfloat4[4] = 
	{
		_mm_cvtepi32_ps(coloruint4[0]),
		_mm_cvtepi32_ps(coloruint4[1]),
		_mm_cvtepi32_ps(coloruint4[2]),
		_mm_cvtepi32_ps(coloruint4[3])
	};

	const __m128 normalizedColorFloat4[4] = 
	{
		_mm_mul_ps(colorfloat4[0], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[1], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[2], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[3], ColorDWORDToFloat4Divisor)
	};

	// Swizzle from RGBA -> ARGB
	const __m128 swizzledColorFloat4[4] =
	{
		_mm_shuffle_ps(normalizedColorFloat4[0], normalizedColorFloat4[0], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[1], normalizedColorFloat4[1], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[2], normalizedColorFloat4[2], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[3], normalizedColorFloat4[3], _MM_SHUFFLE(3, 0, 1, 2) )
	};

	if (writeMask == 0xF)
	{
		*(__m128* const)outColor4[0] = swizzledColorFloat4[0];
		*(__m128* const)outColor4[1] = swizzledColorFloat4[1];
		*(__m128* const)outColor4[2] = swizzledColorFloat4[2];
		*(__m128* const)outColor4[3] = swizzledColorFloat4[3];
	}
	else
	{
		if (writeMask & 0x1)
		{
			outColor4[0]->x = swizzledColorFloat4[0].m128_f32[0];
			outColor4[1]->x = swizzledColorFloat4[1].m128_f32[0];
			outColor4[2]->x = swizzledColorFloat4[2].m128_f32[0];
			outColor4[3]->x = swizzledColorFloat4[3].m128_f32[0];
		}
		if (writeMask & 0x2)
		{
			outColor4[0]->y = swizzledColorFloat4[0].m128_f32[1];
			outColor4[1]->y = swizzledColorFloat4[1].m128_f32[1];
			outColor4[2]->y = swizzledColorFloat4[2].m128_f32[1];
			outColor4[3]->y = swizzledColorFloat4[3].m128_f32[1];
		}
		if (writeMask & 0x4)
		{
			outColor4[0]->z = swizzledColorFloat4[0].m128_f32[2];
			outColor4[1]->z = swizzledColorFloat4[1].m128_f32[2];
			outColor4[2]->z = swizzledColorFloat4[2].m128_f32[2];
			outColor4[3]->z = swizzledColorFloat4[3].m128_f32[2];
		}
		if (writeMask & 0x8)
		{
			outColor4[0]->w = swizzledColorFloat4[0].m128_f32[3];
			outColor4[1]->w = swizzledColorFloat4[1].m128_f32[3];
			outColor4[2]->w = swizzledColorFloat4[2].m128_f32[3];
			outColor4[3]->w = swizzledColorFloat4[3].m128_f32[3];
		}
	}
}

template <const unsigned char writeMask = 0xF>
inline void ColorDWORDToFloat4_4(const D3DCOLOR (&inColor4)[4], D3DXVECTOR4 (&outColor4)[4])
{
	const __m128i inColor4vec = *(const __m128i* const)inColor4;

	const __m128i colorbyte4[4] = 
	{
		_mm_shuffle_epi32(inColor4vec, _MM_SHUFFLE(0, 0, 0, 0) ),
		_mm_shuffle_epi32(inColor4vec, _MM_SHUFFLE(1, 1, 1, 1) ),
		_mm_shuffle_epi32(inColor4vec, _MM_SHUFFLE(2, 2, 2, 2) ),
		_mm_shuffle_epi32(inColor4vec, _MM_SHUFFLE(3, 3, 3, 3) )
	};

	const __m128i coloruint4[4] = 
	{
		_mm_cvtepu8_epi32(colorbyte4[0]),
		_mm_cvtepu8_epi32(colorbyte4[1]),
		_mm_cvtepu8_epi32(colorbyte4[2]),
		_mm_cvtepu8_epi32(colorbyte4[3])
	};

	const __m128 colorfloat4[4] = 
	{
		_mm_cvtepi32_ps(coloruint4[0]),
		_mm_cvtepi32_ps(coloruint4[1]),
		_mm_cvtepi32_ps(coloruint4[2]),
		_mm_cvtepi32_ps(coloruint4[3])
	};

	const __m128 normalizedColorFloat4[4] = 
	{
		_mm_mul_ps(colorfloat4[0], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[1], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[2], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[3], ColorDWORDToFloat4Divisor)
	};

	// Swizzle from RGBA -> ARGB
	const __m128 swizzledColorFloat4[4] =
	{
		_mm_shuffle_ps(normalizedColorFloat4[0], normalizedColorFloat4[0], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[1], normalizedColorFloat4[1], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[2], normalizedColorFloat4[2], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[3], normalizedColorFloat4[3], _MM_SHUFFLE(3, 0, 1, 2) )
	};

	if (writeMask == 0xF)
	{
		*( (__m128* const)&(outColor4[0]) ) = swizzledColorFloat4[0];
		*( (__m128* const)&(outColor4[1]) ) = swizzledColorFloat4[1];
		*( (__m128* const)&(outColor4[2]) ) = swizzledColorFloat4[2];
		*( (__m128* const)&(outColor4[3]) ) = swizzledColorFloat4[3];
	}
	else
	{
		if (writeMask & 0x1)
		{
			outColor4[0].x = swizzledColorFloat4[0].m128_f32[0];
			outColor4[1].x = swizzledColorFloat4[1].m128_f32[0];
			outColor4[2].x = swizzledColorFloat4[2].m128_f32[0];
			outColor4[3].x = swizzledColorFloat4[3].m128_f32[0];
		}
		if (writeMask & 0x2)
		{
			outColor4[0].y = swizzledColorFloat4[0].m128_f32[1];
			outColor4[1].y = swizzledColorFloat4[1].m128_f32[1];
			outColor4[2].y = swizzledColorFloat4[2].m128_f32[1];
			outColor4[3].y = swizzledColorFloat4[3].m128_f32[1];
		}
		if (writeMask & 0x4)
		{
			outColor4[0].z = swizzledColorFloat4[0].m128_f32[2];
			outColor4[1].z = swizzledColorFloat4[1].m128_f32[2];
			outColor4[2].z = swizzledColorFloat4[2].m128_f32[2];
			outColor4[3].z = swizzledColorFloat4[3].m128_f32[2];
		}
		if (writeMask & 0x8)
		{
			outColor4[0].w = swizzledColorFloat4[0].m128_f32[3];
			outColor4[1].w = swizzledColorFloat4[1].m128_f32[3];
			outColor4[2].w = swizzledColorFloat4[2].m128_f32[3];
			outColor4[3].w = swizzledColorFloat4[3].m128_f32[3];
		}
	}
}

template <const unsigned char writeMask = 0xF>
inline void ColorDWORDToFloat4_4(const __m128i inColor4Vec, D3DXVECTOR4 (&outColor4)[4])
{
	const __m128i colorbyte4[4] = 
	{
		_mm_shuffle_epi32(inColor4Vec, _MM_SHUFFLE(0, 0, 0, 0) ),
		_mm_shuffle_epi32(inColor4Vec, _MM_SHUFFLE(1, 1, 1, 1) ),
		_mm_shuffle_epi32(inColor4Vec, _MM_SHUFFLE(2, 2, 2, 2) ),
		_mm_shuffle_epi32(inColor4Vec, _MM_SHUFFLE(3, 3, 3, 3) )
	};

	const __m128i coloruint4[4] = 
	{
		_mm_cvtepu8_epi32(colorbyte4[0]),
		_mm_cvtepu8_epi32(colorbyte4[1]),
		_mm_cvtepu8_epi32(colorbyte4[2]),
		_mm_cvtepu8_epi32(colorbyte4[3])
	};

	const __m128 colorfloat4[4] = 
	{
		_mm_cvtepi32_ps(coloruint4[0]),
		_mm_cvtepi32_ps(coloruint4[1]),
		_mm_cvtepi32_ps(coloruint4[2]),
		_mm_cvtepi32_ps(coloruint4[3])
	};

	const __m128 normalizedColorFloat4[4] = 
	{
		_mm_mul_ps(colorfloat4[0], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[1], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[2], ColorDWORDToFloat4Divisor),
		_mm_mul_ps(colorfloat4[3], ColorDWORDToFloat4Divisor)
	};

	// Swizzle from RGBA -> ARGB
	const __m128 swizzledColorFloat4[4] =
	{
		_mm_shuffle_ps(normalizedColorFloat4[0], normalizedColorFloat4[0], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[1], normalizedColorFloat4[1], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[2], normalizedColorFloat4[2], _MM_SHUFFLE(3, 0, 1, 2) ),
		_mm_shuffle_ps(normalizedColorFloat4[3], normalizedColorFloat4[3], _MM_SHUFFLE(3, 0, 1, 2) )
	};

	if (writeMask == 0xF)
	{
		*( (__m128* const)&(outColor4[0]) ) = swizzledColorFloat4[0];
		*( (__m128* const)&(outColor4[1]) ) = swizzledColorFloat4[1];
		*( (__m128* const)&(outColor4[2]) ) = swizzledColorFloat4[2];
		*( (__m128* const)&(outColor4[3]) ) = swizzledColorFloat4[3];
	}
	else
	{
		if (writeMask & 0x1)
		{
			outColor4[0].x = swizzledColorFloat4[0].m128_f32[0];
			outColor4[1].x = swizzledColorFloat4[1].m128_f32[0];
			outColor4[2].x = swizzledColorFloat4[2].m128_f32[0];
			outColor4[3].x = swizzledColorFloat4[3].m128_f32[0];
		}
		if (writeMask & 0x2)
		{
			outColor4[0].y = swizzledColorFloat4[0].m128_f32[1];
			outColor4[1].y = swizzledColorFloat4[1].m128_f32[1];
			outColor4[2].y = swizzledColorFloat4[2].m128_f32[1];
			outColor4[3].y = swizzledColorFloat4[3].m128_f32[1];
		}
		if (writeMask & 0x4)
		{
			outColor4[0].z = swizzledColorFloat4[0].m128_f32[2];
			outColor4[1].z = swizzledColorFloat4[1].m128_f32[2];
			outColor4[2].z = swizzledColorFloat4[2].m128_f32[2];
			outColor4[3].z = swizzledColorFloat4[3].m128_f32[2];
		}
		if (writeMask & 0x8)
		{
			outColor4[0].w = swizzledColorFloat4[0].m128_f32[3];
			outColor4[1].w = swizzledColorFloat4[1].m128_f32[3];
			outColor4[2].w = swizzledColorFloat4[2].m128_f32[3];
			outColor4[3].w = swizzledColorFloat4[3].m128_f32[3];
		}
	}
}

inline const D3DCOLOR Expand565To888(const RGB565 inColor)
{
	// TODO: Should alpha be 0.0f or 1.0f here?
	const float scaledR = inColor.bits565.r / 31.0f;
	const float scaledG = inColor.bits565.g / 63.0f;
	const float scaledB = inColor.bits565.b / 31.0f;
	const unsigned char r8 = (const unsigned char)(scaledR * 255.0f);
	const unsigned char g8 = (const unsigned char)(scaledG * 255.0f);
	const unsigned char b8 = (const unsigned char)(scaledB * 255.0f);
	return D3DCOLOR_ARGB(255, r8, g8, b8);
}

inline const D3DCOLOR Expand4444To8888(const A4R4G4B4 inColor)
{
	return D3DCOLOR_ARGB(inColor.bits4444.a * 17, inColor.bits4444.r * 17, 
		inColor.bits4444.g * 17, inColor.bits4444.b * 17);
}

inline const D3DCOLOR Expand4440To8888(const X4R4G4B4 inColor)
{
	// TODO: Should alpha be 0.0f or 1.0f here?
	return D3DCOLOR_ARGB(255, inColor.bits4440.r * 17, 
		inColor.bits4440.g * 17, inColor.bits4440.b * 17);
}

template <const unsigned char writeMask = 0xF>
inline void ColorA4R4G4B4ToFloat4(const A4R4G4B4 inColor, D3DXVECTOR4& outColor)
{
	const D3DCOLOR expandedColor8888 = Expand4444To8888(inColor);
	ColorDWORDToFloat4(expandedColor8888, outColor);
}

template <const unsigned char writeMask = 0xF>
inline void ColorX4R4G4B4ToFloat4(const X4R4G4B4 inColor, D3DXVECTOR4& outColor)
{
	const D3DCOLOR expandedColor8888 = Expand4440To8888(inColor);
	ColorDWORDToFloat4(expandedColor8888, outColor);
}

template <const unsigned char writeMask = 0xF>
inline void ColorRGB565ToFloat4(const RGB565 inColor, D3DXVECTOR4& outColor)
{
	const D3DCOLOR expandedColor888 = Expand565To888(inColor);
	ColorDWORDToFloat4(expandedColor888, outColor);
}

template <const unsigned char writeMask = 0xF>
inline const D3DCOLOR Float4ToD3DCOLOR(const D3DXVECTOR4& color)
{
	const unsigned r = writeMask & 0x1 ? ( (const unsigned)(color.x * 255.0f) ) : 0;
	const unsigned g = writeMask & 0x2 ? ( (const unsigned)(color.y * 255.0f) ) : 0;
	const unsigned b = writeMask & 0x4 ? ( (const unsigned)(color.z * 255.0f) ) : 0;
	const unsigned a = writeMask & 0x8 ? ( (const unsigned)(color.w * 255.0f) ) : 0;
	const D3DCOLOR ldrColor = D3DCOLOR_ARGB(a, r, g, b);
	return ldrColor;
}

static const __m128i byteEPI32VecToD3DCOLOR_PSHUFB[16] = 
{
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1), // 0x0
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, -1), // 0x1
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, -1), // 0x2
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, -1), // 0x3
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 8), // 0x4
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, -1, 8), // 0x5
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4, 8), // 0x6
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 4, 8), // 0x7
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, -1, -1, -1), // 0x8
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 0, -1, -1), // 0x9
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, -1, 4, -1), // 0xA
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 0, 4, -1), // 0xB
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, -1, -1, 8), // 0xC
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 0, -1, 8), // 0xD
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, -1, 4, 8), // 0xE
	_mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 0, 4, 8) // 0xF
};
static const __m128 saturateMax = { 1.0f, 1.0f, 1.0f, 1.0f };
static const __m128 scaleMax = { 255.0f, 255.0f, 255.0f, 255.0f };
template <const unsigned char writeMask = 0xF>
inline const D3DCOLOR Float4ToD3DCOLORClamp(const D3DXVECTOR4& color)
{
	const __m128 float4color = *(const __m128* const)&color;
	
	const __m128 float4colorClamped = _mm_min_ps(float4color, saturateMax);
	const __m128 float4colorExpanded = _mm_mul_ps(float4colorClamped, scaleMax);

	const __m128i dword4color = _mm_cvtps_epi32(float4colorExpanded);

	const __m128i shuffledD3DCOLOR = _mm_shuffle_epi8(dword4color, byteEPI32VecToD3DCOLOR_PSHUFB[writeMask]);
	return shuffledD3DCOLOR.m128i_u32[0];
}

template <const unsigned char writeMask = 0xF>
inline const D3DCOLOR Float4ToX8R8G8B8Clamp(const D3DXVECTOR4& color)
{
	return Float4ToD3DCOLORClamp<writeMask & 0x7>(color);
}

template <const unsigned char channelWriteMask = 0xF, const unsigned char pixelWriteMask>
inline void Float4ToD3DCOLOR4Clamp4(const D3DXVECTOR4 (&color4)[4], const __m128i outColorAddresses4)
{
	if (pixelWriteMask == 0)
	{
#ifdef _DEBUG
		__debugbreak(); // Shouldn't be executing this code if we're not going to write anything out
#endif
		return;
	}

	const __m128 (&float4color4)[4] =
	{
		*(const __m128* const)&color4[0],
		*(const __m128* const)&color4[1],
		*(const __m128* const)&color4[2],
		*(const __m128* const)&color4[3]
	};

	const __m128 float4colorClamped4[4] = 
	{
		_mm_min_ps(float4color4[0], saturateMax),
		_mm_min_ps(float4color4[1], saturateMax),
		_mm_min_ps(float4color4[2], saturateMax),
		_mm_min_ps(float4color4[3], saturateMax)
	};

	const __m128 float4colorExpanded4[4] =
	{
		_mm_mul_ps(float4colorClamped4[0], scaleMax),
		_mm_mul_ps(float4colorClamped4[1], scaleMax),
		_mm_mul_ps(float4colorClamped4[2], scaleMax),
		_mm_mul_ps(float4colorClamped4[3], scaleMax)
	};

	const __m128i dword4color4[4] =
	{
		_mm_cvtps_epi32(float4colorExpanded4[0]),
		_mm_cvtps_epi32(float4colorExpanded4[1]),
		_mm_cvtps_epi32(float4colorExpanded4[2]),
		_mm_cvtps_epi32(float4colorExpanded4[3])
	};

	const __m128i shuffledD3DCOLOR4[4] = 
	{
		_mm_shuffle_epi8(dword4color4[0], byteEPI32VecToD3DCOLOR_PSHUFB[channelWriteMask]),
		_mm_shuffle_epi8(dword4color4[1], byteEPI32VecToD3DCOLOR_PSHUFB[channelWriteMask]),
		_mm_shuffle_epi8(dword4color4[2], byteEPI32VecToD3DCOLOR_PSHUFB[channelWriteMask]),
		_mm_shuffle_epi8(dword4color4[3], byteEPI32VecToD3DCOLOR_PSHUFB[channelWriteMask])
	};

	if (pixelWriteMask & 0x1)
		*(D3DCOLOR* const)outColorAddresses4.m128i_u32[0] = shuffledD3DCOLOR4[0].m128i_u32[0];
	if (pixelWriteMask & 0x2)
		*(D3DCOLOR* const)outColorAddresses4.m128i_u32[1] = shuffledD3DCOLOR4[1].m128i_u32[0];
	if (pixelWriteMask & 0x4)
		*(D3DCOLOR* const)outColorAddresses4.m128i_u32[2] = shuffledD3DCOLOR4[2].m128i_u32[0];
	if (pixelWriteMask & 0x8)
		*(D3DCOLOR* const)outColorAddresses4.m128i_u32[3] = shuffledD3DCOLOR4[3].m128i_u32[0];
}

template <const unsigned char channelWriteMask = 0xF, const unsigned char pixelWriteMask>
inline void Float4ToX8R8G8B8_4Clamp4(const D3DXVECTOR4 (&color4)[4], const __m128i outColorAddresses4)
{
	if (pixelWriteMask == 0)
	{
#ifdef _DEBUG
		__debugbreak(); // Shouldn't be executing this code if we're not going to write anything out
#endif
		return;
	}

	Float4ToD3DCOLOR4Clamp4<channelWriteMask & 0x7, pixelWriteMask>(color4, outColorAddresses4);
}

struct A16B16G16R16
{
	unsigned short r;
	unsigned short g;
	unsigned short b;
	unsigned short a;
};

struct A16B16G16R16F
{
	D3DXFLOAT16 r;
	D3DXFLOAT16 g;
	D3DXFLOAT16 b;
	D3DXFLOAT16 a;
};

struct A32B32G32R32F
{
	float r;
	float g;
	float b;
	float a;
};

template <const unsigned char writeMask = 0xF>
inline void Float4ToA16B16G16R16(const D3DXVECTOR4& color, A16B16G16R16& outColor)
{
	// TODO: Vectorize this
	if (writeMask & 0x1)
	{
		if (color.x > 1.0f)
			outColor.r = 0xFFFF;
		else if (color.x < 0.0f)
			outColor.r = 0x0000;
		else
			outColor.r = (const unsigned short)(color.x * 65535.0f);
	}

	if (writeMask & 0x2)
	{
		if (color.y > 1.0f)
			outColor.g = 0xFFFF;
		else if (color.y < 0.0f)
			outColor.g = 0x0000;
		else
			outColor.g = (const unsigned short)(color.y * 65535.0f);
	}

	if (writeMask & 0x4)
	{
		if (color.z > 1.0f)
			outColor.b = 0xFFFF;
		else if (color.z < 0.0f)
			outColor.b = 0x0000;
		else
			outColor.b = (const unsigned short)(color.z * 65535.0f);
	}

	if (writeMask & 0x8)
	{
		if (color.w > 1.0f)
			outColor.a = 0xFFFF;
		else if (color.w < 0.0f)
			outColor.a = 0x0000;
		else
			outColor.a = (const unsigned short)(color.w * 65535.0f);
	}
}

template <const unsigned char writeMask = 0xF, const unsigned char pixelWriteMask>
inline void Float4ToA16B16G16R16_4(const D3DXVECTOR4 (&color)[4], const __m128i outColorAddresses)
{
	if (pixelWriteMask == 0)
	{
#ifdef _DEBUG
		__debugbreak(); // Don't call this if you're not going to write anything!
#endif
		return;
	}

	// TODO: Vectorize this
	for (unsigned x = 0; x < 4; ++x)
	{
		if (pixelWriteMask & (1 << x) )
			Float4ToA16B16G16R16<writeMask>(color[x], *(A16B16G16R16* const)outColorAddresses.m128i_u32[x]);
	}
}

static const __m128 ColorA16B16G16R16ToFloat4Divisor = { 1.0f / 65535.0f, 1.0f / 65535.0f, 1.0f / 65535.0f, 1.0f / 65535.0f };

template <const unsigned char writeMask = 0xF>
inline void ColorA16B16G16R16ToFloat4(const A16B16G16R16& color, D3DXVECTOR4& outColor)
{
	__m128i colorushort4;
	colorushort4.m128i_u16[0] = color.r;
	colorushort4.m128i_u16[1] = color.g;
	colorushort4.m128i_u16[2] = color.b;
	colorushort4.m128i_u16[3] = color.a;

	const __m128i coloruint4 = _mm_cvtepu16_epi32(colorushort4);
	const __m128 colorfloat4 = _mm_cvtepi32_ps(coloruint4);
	const __m128 normalizedColorFloat4 = _mm_mul_ps(colorfloat4, ColorA16B16G16R16ToFloat4Divisor);
	if (writeMask & 0x1)
		outColor.x = normalizedColorFloat4.m128_f32[0];
	if (writeMask & 0x2)
		outColor.y = normalizedColorFloat4.m128_f32[1];
	if (writeMask & 0x4)
		outColor.z = normalizedColorFloat4.m128_f32[2];
	if (writeMask & 0x8)
		outColor.w = normalizedColorFloat4.m128_f32[3];
}

template <const unsigned char writeMask = 0xF>
inline void ColorA16B16G16R16ToFloat4_4(const A16B16G16R16 (&color4)[4], D3DXVECTOR4 (&outColor4)[4])
{
	__m128i colorushort4_4[4];
	colorushort4_4[0].m128i_u16[0] = color4[0].r;
	colorushort4_4[0].m128i_u16[1] = color4[0].g;
	colorushort4_4[0].m128i_u16[2] = color4[0].b;
	colorushort4_4[0].m128i_u16[3] = color4[0].a;
	colorushort4_4[1].m128i_u16[0] = color4[1].r;
	colorushort4_4[1].m128i_u16[1] = color4[1].g;
	colorushort4_4[1].m128i_u16[2] = color4[1].b;
	colorushort4_4[1].m128i_u16[3] = color4[1].a;
	colorushort4_4[2].m128i_u16[0] = color4[2].r;
	colorushort4_4[2].m128i_u16[1] = color4[2].g;
	colorushort4_4[2].m128i_u16[2] = color4[2].b;
	colorushort4_4[2].m128i_u16[3] = color4[2].a;
	colorushort4_4[3].m128i_u16[0] = color4[3].r;
	colorushort4_4[3].m128i_u16[1] = color4[3].g;
	colorushort4_4[3].m128i_u16[2] = color4[3].b;
	colorushort4_4[3].m128i_u16[3] = color4[3].a;

	const __m128i coloruint4_4[4] = 
	{
		_mm_cvtepu16_epi32(colorushort4_4[0]),
		_mm_cvtepu16_epi32(colorushort4_4[1]),
		_mm_cvtepu16_epi32(colorushort4_4[2]),
		_mm_cvtepu16_epi32(colorushort4_4[3])
	};

	const __m128 colorfloat4_4[4] = 
	{
		_mm_cvtepi32_ps(coloruint4_4[0]),
		_mm_cvtepi32_ps(coloruint4_4[1]),
		_mm_cvtepi32_ps(coloruint4_4[2]),
		_mm_cvtepi32_ps(coloruint4_4[3])
	};

	const __m128 normalizedColorFloat4_4[4] = 
	{
		_mm_mul_ps(colorfloat4_4[0], ColorA16B16G16R16ToFloat4Divisor),
		_mm_mul_ps(colorfloat4_4[1], ColorA16B16G16R16ToFloat4Divisor),
		_mm_mul_ps(colorfloat4_4[2], ColorA16B16G16R16ToFloat4Divisor),
		_mm_mul_ps(colorfloat4_4[3], ColorA16B16G16R16ToFloat4Divisor)
	};

	if (writeMask & 0x1)
	{
		outColor4[0].x = normalizedColorFloat4_4[0].m128_f32[0];
		outColor4[1].x = normalizedColorFloat4_4[1].m128_f32[0];
		outColor4[2].x = normalizedColorFloat4_4[2].m128_f32[0];
		outColor4[3].x = normalizedColorFloat4_4[3].m128_f32[0];
	}
	if (writeMask & 0x2)
	{
		outColor4[0].y = normalizedColorFloat4_4[0].m128_f32[1];
		outColor4[1].y = normalizedColorFloat4_4[1].m128_f32[1];
		outColor4[2].y = normalizedColorFloat4_4[2].m128_f32[1];
		outColor4[3].y = normalizedColorFloat4_4[3].m128_f32[1];
	}
	if (writeMask & 0x4)
	{
		outColor4[0].z = normalizedColorFloat4_4[0].m128_f32[2];
		outColor4[1].z = normalizedColorFloat4_4[1].m128_f32[2];
		outColor4[2].z = normalizedColorFloat4_4[2].m128_f32[2];
		outColor4[3].z = normalizedColorFloat4_4[3].m128_f32[2];
	}
	if (writeMask & 0x8)
	{
		outColor4[0].w = normalizedColorFloat4_4[0].m128_f32[3];
		outColor4[1].w = normalizedColorFloat4_4[1].m128_f32[3];
		outColor4[2].w = normalizedColorFloat4_4[2].m128_f32[3];
		outColor4[3].w = normalizedColorFloat4_4[3].m128_f32[3];
	}
}

template <const unsigned char writeMask = 0xF>
inline void Float4ToA16B16G16R16F(const D3DXVECTOR4& color, A16B16G16R16F& outColor)
{
	const __m128 float4Color = *(const __m128* const)&color;
	
	// Floating point rules specify round-to-nearest as the rounding mode for float16's: https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-float-rules
	const __m128i half4Color = _mm_cvtps_ph(float4Color, _MM_FROUND_TO_NEAREST_INT);

	if (writeMask & 0x1)
		outColor.r = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[0]);
	if (writeMask & 0x2)
		outColor.g = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[1]);
	if (writeMask & 0x4)
		outColor.b = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[2]);
	if (writeMask & 0x8)
		outColor.a = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[3]);
}

template <const unsigned char channelWriteMask = 0xF, const unsigned char pixelWriteMask = 0xF>
inline void Float4ToA16B16G16R16F4(const D3DXVECTOR4 (&color)[4], const __m128i outColorAddresses4)
{
	if (pixelWriteMask == 0)
	{
#ifdef _DEBUG
		__debugbreak(); // Don't call this function if you're not going to write anything out!
#endif
		return;
	}

	const __m128 float4Color4[4] = 
	{
		*(const __m128* const)&color[0],
		*(const __m128* const)&color[1],
		*(const __m128* const)&color[2],
		*(const __m128* const)&color[3]
	};
	
	// Floating point rules specify round-to-nearest as the rounding mode for float16's: https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-float-rules
	const __m128i half4Color4[4] = 
	{
		_mm_cvtps_ph(float4Color4[0], _MM_FROUND_TO_NEAREST_INT),
		_mm_cvtps_ph(float4Color4[1], _MM_FROUND_TO_NEAREST_INT),
		_mm_cvtps_ph(float4Color4[2], _MM_FROUND_TO_NEAREST_INT),
		_mm_cvtps_ph(float4Color4[3], _MM_FROUND_TO_NEAREST_INT)
	};

	D3DXFLOAT16* const writeAddresses[4] =
	{
		(D3DXFLOAT16* const)outColorAddresses4.m128i_u32[0],
		(D3DXFLOAT16* const)outColorAddresses4.m128i_u32[1],
		(D3DXFLOAT16* const)outColorAddresses4.m128i_u32[2],
		(D3DXFLOAT16* const)outColorAddresses4.m128i_u32[3]
	};

	if (pixelWriteMask & 0x1)
	{
		D3DXFLOAT16* const outColor = writeAddresses[0];
		if (channelWriteMask & 0x1) outColor[0] = *(const D3DXFLOAT16* const)&(half4Color4[0].m128i_u16[0]);
		if (channelWriteMask & 0x2) outColor[1] = *(const D3DXFLOAT16* const)&(half4Color4[0].m128i_u16[1]);
		if (channelWriteMask & 0x4) outColor[2] = *(const D3DXFLOAT16* const)&(half4Color4[0].m128i_u16[2]);
		if (channelWriteMask & 0x8)	outColor[3] = *(const D3DXFLOAT16* const)&(half4Color4[0].m128i_u16[3]);
	}
	if (pixelWriteMask & 0x2)
	{
		D3DXFLOAT16* const outColor = writeAddresses[1];
		if (channelWriteMask & 0x1) outColor[0] = *(const D3DXFLOAT16* const)&(half4Color4[1].m128i_u16[0]);
		if (channelWriteMask & 0x2) outColor[1] = *(const D3DXFLOAT16* const)&(half4Color4[1].m128i_u16[1]);
		if (channelWriteMask & 0x4) outColor[2] = *(const D3DXFLOAT16* const)&(half4Color4[1].m128i_u16[2]);
		if (channelWriteMask & 0x8)	outColor[3] = *(const D3DXFLOAT16* const)&(half4Color4[1].m128i_u16[3]);
	}
	if (pixelWriteMask & 0x4)
	{
		D3DXFLOAT16* const outColor = writeAddresses[2];
		if (channelWriteMask & 0x1) outColor[0] = *(const D3DXFLOAT16* const)&(half4Color4[2].m128i_u16[0]);
		if (channelWriteMask & 0x2) outColor[1] = *(const D3DXFLOAT16* const)&(half4Color4[2].m128i_u16[1]);
		if (channelWriteMask & 0x4) outColor[2] = *(const D3DXFLOAT16* const)&(half4Color4[2].m128i_u16[2]);
		if (channelWriteMask & 0x8)	outColor[3] = *(const D3DXFLOAT16* const)&(half4Color4[2].m128i_u16[3]);
	}
	if (pixelWriteMask & 0x8)
	{
		D3DXFLOAT16* const outColor = writeAddresses[3];
		if (channelWriteMask & 0x1) outColor[0] = *(const D3DXFLOAT16* const)&(half4Color4[3].m128i_u16[0]);
		if (channelWriteMask & 0x2) outColor[1] = *(const D3DXFLOAT16* const)&(half4Color4[3].m128i_u16[1]);
		if (channelWriteMask & 0x4) outColor[2] = *(const D3DXFLOAT16* const)&(half4Color4[3].m128i_u16[2]);
		if (channelWriteMask & 0x8)	outColor[3] = *(const D3DXFLOAT16* const)&(half4Color4[3].m128i_u16[3]);
	}
}

template <const unsigned char writeMask = 0xF>
inline void ColorA16B16G16R16FToFloat4(const A16B16G16R16F& color, D3DXVECTOR4& outColor)
{
	__m128i half4color;
	half4color.m128i_u16[0] = *(const unsigned short* const)&color.r;
	half4color.m128i_u16[1] = *(const unsigned short* const)&color.g;
	half4color.m128i_u16[2] = *(const unsigned short* const)&color.b;
	half4color.m128i_u16[3] = *(const unsigned short* const)&color.a;

	const __m128 float4color = _mm_cvtph_ps(half4color);
	if (writeMask & 0x1)
		outColor.x = float4color.m128_f32[0];
	if (writeMask & 0x2)
		outColor.y = float4color.m128_f32[1];
	if (writeMask & 0x4)
		outColor.z = float4color.m128_f32[2];
	if (writeMask & 0x8)
		outColor.w = float4color.m128_f32[3];
}

template <const unsigned char writeMask = 0xF>
inline void ColorA16B16G16R16FToFloat4_4(const A16B16G16R16F* const (&color4)[4], D3DXVECTOR4 (&outColor4)[4])
{
	__m128i half4color4[4];
	half4color4[0].m128i_u16[0] = *(const unsigned short* const)&color4[0]->r;
	half4color4[0].m128i_u16[1] = *(const unsigned short* const)&color4[0]->g;
	half4color4[0].m128i_u16[2] = *(const unsigned short* const)&color4[0]->b;
	half4color4[0].m128i_u16[3] = *(const unsigned short* const)&color4[0]->a;
	half4color4[1].m128i_u16[0] = *(const unsigned short* const)&color4[1]->r;
	half4color4[1].m128i_u16[1] = *(const unsigned short* const)&color4[1]->g;
	half4color4[1].m128i_u16[2] = *(const unsigned short* const)&color4[1]->b;
	half4color4[1].m128i_u16[3] = *(const unsigned short* const)&color4[1]->a;
	half4color4[2].m128i_u16[0] = *(const unsigned short* const)&color4[2]->r;
	half4color4[2].m128i_u16[1] = *(const unsigned short* const)&color4[2]->g;
	half4color4[2].m128i_u16[2] = *(const unsigned short* const)&color4[2]->b;
	half4color4[2].m128i_u16[3] = *(const unsigned short* const)&color4[2]->a;
	half4color4[3].m128i_u16[0] = *(const unsigned short* const)&color4[3]->r;
	half4color4[3].m128i_u16[1] = *(const unsigned short* const)&color4[3]->g;
	half4color4[3].m128i_u16[2] = *(const unsigned short* const)&color4[3]->b;
	half4color4[3].m128i_u16[3] = *(const unsigned short* const)&color4[3]->a;

	const __m128 float4color4[4] = 
	{
		_mm_cvtph_ps(half4color4[0]),
		_mm_cvtph_ps(half4color4[1]),
		_mm_cvtph_ps(half4color4[2]),
		_mm_cvtph_ps(half4color4[3])
	};
	if (writeMask & 0x1)
	{
		outColor4[0].x = float4color4[0].m128_f32[0];
		outColor4[1].x = float4color4[1].m128_f32[0];
		outColor4[2].x = float4color4[2].m128_f32[0];
		outColor4[3].x = float4color4[3].m128_f32[0];
	}
	if (writeMask & 0x2)
	{
		outColor4[0].y = float4color4[0].m128_f32[1];
		outColor4[1].y = float4color4[1].m128_f32[1];
		outColor4[2].y = float4color4[2].m128_f32[1];
		outColor4[3].y = float4color4[3].m128_f32[1];
	}
	if (writeMask & 0x4)
	{
		outColor4[0].z = float4color4[0].m128_f32[2];
		outColor4[1].z = float4color4[1].m128_f32[2];
		outColor4[2].z = float4color4[2].m128_f32[2];
		outColor4[3].z = float4color4[3].m128_f32[2];
	}
	if (writeMask & 0x8)
	{
		outColor4[0].w = float4color4[0].m128_f32[3];
		outColor4[1].w = float4color4[1].m128_f32[3];
		outColor4[2].w = float4color4[2].m128_f32[3];
		outColor4[3].w = float4color4[3].m128_f32[3];
	}
}

template <const unsigned char writeMask = 0xF>
inline void Float4ToA32B32G32R32F(const D3DXVECTOR4& color, A32B32G32R32F& outColor)
{
	if (writeMask == 0x0)
		return;

	if (writeMask == 0xF)
	{
		*(__m128* const)&outColor = *(const __m128* const)&color;
		return;
	}

	if (writeMask & 0x1)
		outColor.r = color.x;
	if (writeMask & 0x2)
		outColor.g = color.y;
	if (writeMask & 0x4)
		outColor.b = color.z;
	if (writeMask & 0x8)
		outColor.a = color.w;
}

template <const unsigned char channelWriteMask = 0xF, const unsigned char pixelWriteMask>
inline void Float4ToA32B32G32R32F4(const D3DXVECTOR4 (&color)[4], const __m128i outColorAddresses4)
{
	if (pixelWriteMask == 0x0)
	{
#ifdef _DEBUG
		__debugbreak(); // Don't call this function if you're not going to write anything out!
#endif
		return;
	}

	if (channelWriteMask == 0x0)
		return;

	if (channelWriteMask == 0xF)
	{
		if (pixelWriteMask & 0x1)
			*(__m128* const)(outColorAddresses4.m128i_u32[0]) = *(const __m128* const)&(color[0]);
		if (pixelWriteMask & 0x2)
			*(__m128* const)(outColorAddresses4.m128i_u32[1]) = *(const __m128* const)&(color[1]);
		if (pixelWriteMask & 0x4)
			*(__m128* const)(outColorAddresses4.m128i_u32[2]) = *(const __m128* const)&(color[2]);
		if (pixelWriteMask & 0x8)
			*(__m128* const)(outColorAddresses4.m128i_u32[3]) = *(const __m128* const)&(color[3]);
		return;
	}

	A32B32G32R32F* const outColor4[4] =
	{
		(A32B32G32R32F* const)(outColorAddresses4.m128i_u32[0]),
		(A32B32G32R32F* const)(outColorAddresses4.m128i_u32[1]),
		(A32B32G32R32F* const)(outColorAddresses4.m128i_u32[2]),
		(A32B32G32R32F* const)(outColorAddresses4.m128i_u32[3])
	};

	for (unsigned x = 0; x < 4; ++x)
	{
		if (pixelWriteMask & (1 << x) )
		{
			A32B32G32R32F& outColor = *outColor4[x];
			const D3DXVECTOR4& thisColor = color[x];
			if (channelWriteMask & 0x1)
				outColor.r = thisColor.x;
			if (channelWriteMask & 0x2)
				outColor.g = thisColor.y;
			if (channelWriteMask & 0x4)
				outColor.b = thisColor.z;
			if (channelWriteMask & 0x8)
				outColor.a = thisColor.w;
		}
	}
}

template <const unsigned char writeMask = 0xF>
inline void ColorA32B32G32R32FToFloat4(const A32B32G32R32F& color, D3DXVECTOR4& outColor)
{
	if (writeMask == 0x0)
		return;

	if (writeMask == 0xF)
	{
		*(__m128* const)&outColor = *(const __m128* const)&color;
		return;
	}

	if (writeMask & 0x1)
		outColor.x = color.r;
	if (writeMask & 0x2)
		outColor.y = color.g;
	if (writeMask & 0x4)
		outColor.z = color.b;
	if (writeMask & 0x8)
		outColor.w = color.a;
}

template <const unsigned char writeMask = 0xF>
inline void ColorA32B32G32R32FToFloat4_4(const A32B32G32R32F* const (&color4)[4], D3DXVECTOR4 (&outColor4)[4])
{
	if (writeMask & 0x1)
	{
		outColor4[0].x = color4[0]->r;
		outColor4[1].x = color4[1]->r;
		outColor4[2].x = color4[2]->r;
		outColor4[3].x = color4[3]->r;
	}
	if (writeMask & 0x2)
	{
		outColor4[0].y = color4[0]->g;
		outColor4[1].y = color4[1]->g;
		outColor4[2].y = color4[2]->g;
		outColor4[3].y = color4[3]->g;
	}
	if (writeMask & 0x4)
	{
		outColor4[0].z = color4[0]->b;
		outColor4[1].z = color4[1]->b;
		outColor4[2].z = color4[2]->b;
		outColor4[3].z = color4[3]->b;
	}
	if (writeMask & 0x8)
	{
		outColor4[0].w = color4[0]->a;
		outColor4[1].w = color4[1]->a;
		outColor4[2].w = color4[2]->a;
		outColor4[3].w = color4[3]->a;
	}
}

template <const unsigned char channelWriteMask = 0xF>
inline void Float4ToL8(const D3DXVECTOR4& color, unsigned char& outColor)
{
	if (channelWriteMask & 0x1)
	{
		outColor = (const unsigned char)(color.x * 255.0f);
	}
}

template <const unsigned char channelWriteMask = 0xF>
inline void Float4ToL8Clamp(const D3DXVECTOR4& color, unsigned char& outColor)
{
	if (channelWriteMask & 0x1)
	{
		outColor = color.x > 0.0f ? color.x <= 1.0f ? (const unsigned char)(color.x * 255.0f) : 255 : 0;
	}
}

template <const unsigned char channelWriteMask = 0xF, const unsigned char pixelWriteMask = 0xF>
inline void Float4ToL8Clamp4(const D3DXVECTOR4 (&color)[4], const __m128i outColorAddresses4)
{
	if (pixelWriteMask == 0)
	{
#ifdef _DEBUG
		__debugbreak(); // Don't call this if you don't intend to write anything out!
#endif
		return;
	}
	if ( (channelWriteMask & 0x1) != 1)
		return;

	// TODO: Properly vectorize this
	for (unsigned x = 0; x < 4; ++x)
	{
		if (pixelWriteMask & (1 << x) )
			Float4ToL8Clamp<channelWriteMask>(color[x], *(unsigned char* const)(outColorAddresses4.m128i_u32) );
	}
}

static const float inv255 = 1.0f / 255.0f;
template <const unsigned char writeMask = 0xF>
inline void L8ToFloat4(const unsigned char& color, D3DXVECTOR4& outColor)
{
	const float l8color = color * inv255;
	if (writeMask & 0x1)
		outColor.x = l8color;
	if (writeMask & 0x2)
		outColor.y = l8color;
	if (writeMask & 0x4)
		outColor.z = l8color;
	if (writeMask & 0x8)
		outColor.w = 1.0f; // L8 textures always treat the alpha channel as 1.0f: https://msdn.microsoft.com/en-us/library/windows/desktop/bb206224(v=vs.85).aspx
}

template <const unsigned char writeMask = 0xF>
inline void L8ToFloat4_4(const __m128i l8_4, D3DXVECTOR4 (&outColor4)[4])
{
	__m128 colorFloat4 = _mm_mul_ps(_mm_cvtepi32_ps(l8_4), ColorDWORDToFloat4Divisor);
	if (writeMask & 0x1)
	{
		outColor4[0].x = colorFloat4.m128_f32[0];
		outColor4[1].x = colorFloat4.m128_f32[1];
		outColor4[2].x = colorFloat4.m128_f32[2];
		outColor4[3].x = colorFloat4.m128_f32[3];
	}
	if (writeMask & 0x2)
	{
		outColor4[0].y = colorFloat4.m128_f32[0];
		outColor4[1].y = colorFloat4.m128_f32[1];
		outColor4[2].y = colorFloat4.m128_f32[2];
		outColor4[3].y = colorFloat4.m128_f32[3];
	}
	if (writeMask & 0x4)
	{
		outColor4[0].z = colorFloat4.m128_f32[0];
		outColor4[1].z = colorFloat4.m128_f32[1];
		outColor4[2].z = colorFloat4.m128_f32[2];
		outColor4[3].z = colorFloat4.m128_f32[3];
	}
	if (writeMask & 0x8)
	{
		// L8 textures always treat the alpha channel as 1.0f: https://msdn.microsoft.com/en-us/library/windows/desktop/bb206224(v=vs.85).aspx
		outColor4[0].w = 1.0f;
		outColor4[1].w = 1.0f;
		outColor4[2].w = 1.0f;
		outColor4[3].w = 1.0f;
	}
}

template <const unsigned char channelWriteMask = 0xF>
inline void Float4ToR16F(const D3DXVECTOR4& color, D3DXFLOAT16& outColor)
{
	if ( (channelWriteMask & 0x1) != 1)
		return;

	const __m128 float4Color = *(const __m128* const)&color;

	// Floating point rules specify round-to-nearest as the rounding mode for float16's: https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-float-rules
	const __m128i half4Color = _mm_cvtps_ph(float4Color, _MM_FROUND_TO_NEAREST_INT);

	if (channelWriteMask & 0x1)
		outColor = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[0]);
}

template <const unsigned char channelWriteMask = 0xF, const unsigned char pixelWriteMask = 0xF>
inline void Float4ToR16F4(const D3DXVECTOR4 (&color)[4], const __m128i outColorAddresses4)
{
	if (pixelWriteMask == 0x0)
	{
#ifdef _DEBUG
		__debugbreak(); // Should never call this function if you don't intend to write anything out!
#endif
		return;
	}

	if ( (channelWriteMask & 0x1) != 0x1)
		return;

	const __m128 float4Color4[4] = 
	{
		*(const __m128* const)&color[0],
		*(const __m128* const)&color[1],
		*(const __m128* const)&color[2],
		*(const __m128* const)&color[3]
	};

	// Shuffle from 4 vectors' .x components into one vector's .xyzw components:
	const __m128 tempShufXY = _mm_shuffle_ps(*(const __m128* const)&(color[0]), *(const __m128* const)&(color[1]), _MM_SHUFFLE(0, 0, 1, 0) ); // X and Y
	const __m128 tempShufZW = _mm_shuffle_ps(*(const __m128* const)&(color[2]), *(const __m128* const)&(color[3]), _MM_SHUFFLE(0, 0, 1, 0) ); // Z and W
	const __m128 float4Color = _mm_shuffle_ps(tempShufXY, tempShufZW, _MM_SHUFFLE(1, 0, 1, 0) ); // XYZW

	// Floating point rules specify round-to-nearest as the rounding mode for float16's: https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-float-rules
	const __m128i half4Color = _mm_cvtps_ph(float4Color, _MM_FROUND_TO_NEAREST_INT);

	D3DXFLOAT16* const outColors4[4] =
	{
		(D3DXFLOAT16* const)(outColorAddresses4.m128i_u32[0]),
		(D3DXFLOAT16* const)(outColorAddresses4.m128i_u32[1]),
		(D3DXFLOAT16* const)(outColorAddresses4.m128i_u32[2]),
		(D3DXFLOAT16* const)(outColorAddresses4.m128i_u32[3])
	};

	if (pixelWriteMask & 0x1)
		*outColors4[0] = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[0]);
	if (pixelWriteMask & 0x2)
		*outColors4[1] = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[1]);
	if (pixelWriteMask & 0x4)
		*outColors4[2] = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[2]);
	if (pixelWriteMask & 0x8)
		*outColors4[3] = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[3]);
}

template <const unsigned char writeMask = 0xF>
inline void ColorR16FToFloat4(const D3DXFLOAT16& color, D3DXVECTOR4& outColor)
{
	if (writeMask & 0x1)
	{
		__m128i colorHalf4;
		colorHalf4.m128i_u16[0] = *(const unsigned short* const)&color;
		const __m128 colorFloat4 = _mm_cvtph_ps(colorHalf4);
		outColor.x = colorFloat4.m128_f32[0];
	}
	if (writeMask & 0x2)
		outColor.y = 1.0f; // https://msdn.microsoft.com/en-us/library/windows/desktop/bb206224(v=vs.85).aspx
	if (writeMask & 0x4)
		outColor.z = 1.0f;
	if (writeMask & 0x8)
		outColor.w = 1.0f;
}

template <const unsigned char writeMask = 0xF>
inline void ColorR16FToFloat4_4(const D3DXFLOAT16 (&color4)[4], D3DXVECTOR4 (&outColor4)[4])
{
	if (writeMask & 0x1)
	{
		__m128i colorHalf4;
		colorHalf4.m128i_u16[0] = *(const unsigned short* const)&(color4[0]);
		colorHalf4.m128i_u16[1] = *(const unsigned short* const)&(color4[0]);
		colorHalf4.m128i_u16[2] = *(const unsigned short* const)&(color4[0]);
		colorHalf4.m128i_u16[3] = *(const unsigned short* const)&(color4[0]);
		const __m128 colorFloat4 = _mm_cvtph_ps(colorHalf4);

		outColor4[0].x = colorFloat4.m128_f32[0];
		outColor4[1].x = colorFloat4.m128_f32[1];
		outColor4[2].x = colorFloat4.m128_f32[2];
		outColor4[3].x = colorFloat4.m128_f32[3];
	}
	if (writeMask & 0x2)
	{
		// https://msdn.microsoft.com/en-us/library/windows/desktop/bb206224(v=vs.85).aspx
		outColor4[0].y = 1.0f;
		outColor4[1].y = 1.0f;
		outColor4[2].y = 1.0f;
		outColor4[3].y = 1.0f;
	}
	if (writeMask & 0x4)
	{
		outColor4[0].z = 1.0f;
		outColor4[1].z = 1.0f;
		outColor4[2].z = 1.0f;
		outColor4[3].z = 1.0f;
	}
	if (writeMask & 0x8)
	{
		outColor4[0].w = 1.0f;
		outColor4[1].w = 1.0f;
		outColor4[2].w = 1.0f;
		outColor4[3].w = 1.0f;
	}
}

static inline const unsigned RoundUpTo4(const unsigned num)
{
	return ( (num + 3) & ~0x3);
}

static inline const unsigned RoundUpTo8(const unsigned num)
{
	return ( (num + 7) & ~0x7);
}

static inline const unsigned RoundUpTo16(const unsigned num)
{
	return ( (num + 15) & ~0xF);
}

template <typename T>
static inline void SwapWithCopy(T& left, T& right)
{
	T copy = left;
	left = right;
	right = copy;
}
