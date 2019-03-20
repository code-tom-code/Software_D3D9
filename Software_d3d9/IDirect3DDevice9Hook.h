#pragma once

#include "IDirect3D9Hook.h"
#include "ShaderBase.h"
#include "VShaderEngine.h"
#include "PShaderEngine.h"
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
enum DebuggableUsage : long
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
	StreamSource() : vertexBuffer(NULL), streamOffset(0), streamDividerFrequency(D3DSTREAMSOURCE_INDEXEDDATA), streamStride(0)
	{
	}

	~StreamSource()
	{
		vertexBuffer = NULL;
		streamOffset = 0;
		streamDividerFrequency = D3DSTREAMSOURCE_INDEXEDDATA;
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

	float4 floats[256]; // 256 is the maximum number of shader constants in both PS_3_0 and VS_3_0
	BOOL bools[16];
	int4 ints[16];
};

struct TextureStageState
{
	TextureStageState()
	{
		memset(this, 0, sizeof(*this) );
		SetStageDefaults();
	}

	~TextureStageState()
	{
		memset(this, 0, sizeof(*this) );
		SetStageDefaults();
	}

	inline void SetStageDefaults(void)
	{
		stageStateUnion.namedStates.colorOp = D3DTOP_DISABLE;
		stageStateUnion.namedStates.colorArg1 = D3DCOLOR_ARGB(0, 0, 0, 0);
		stageStateUnion.namedStates.colorArg2 = D3DCOLOR_ARGB(0, 0, 0, 0);
		stageStateUnion.namedStates.alphaOp = D3DTOP_DISABLE;
		stageStateUnion.namedStates.alphaArg1 = D3DCOLOR_ARGB(0, 0, 0, 0);
		stageStateUnion.namedStates.alphaArg2 = D3DCOLOR_ARGB(0, 0, 0, 0);
		stageStateUnion.namedStates.bumpEnvMat00 = 0.0f;
		stageStateUnion.namedStates.bumpEnvMat01 = 0.0f;
		stageStateUnion.namedStates.bumpEnvMat10 = 0.0f;
		stageStateUnion.namedStates.bumpEnvMat11 = 0.0f;
		stageStateUnion.namedStates.texCoordIndex = 0;
		stageStateUnion.namedStates.bumpEnvLScale = 0.0f; // Shouldn't this default to 1.0f instead?
		stageStateUnion.namedStates.bumpEnvLOffset = 0.0f;
		stageStateUnion.namedStates.textureTransformFlags = D3DTTFF_DISABLE;
		stageStateUnion.namedStates.colorArg0 = D3DCOLOR_ARGB(0, 0, 0, 0);
		stageStateUnion.namedStates.alphaArg0 = D3DCOLOR_ARGB(0, 0, 0, 0);
		stageStateUnion.namedStates.resultArg = D3DTA_CURRENT;
		stageStateUnion.namedStates.constant = D3DCOLOR_ARGB(0, 0, 0, 0);
	}

	inline void SetStage0Defaults(void)
	{
		stageStateUnion.namedStates.colorOp = D3DTOP_MODULATE;
		stageStateUnion.namedStates.alphaOp = D3DTOP_SELECTARG1;
	}

	union _stageStateUnion
	{
		struct _namedStates
		{
			DWORD padding0; // 0
			D3DTEXTUREOP colorOp; // 1
			D3DCOLOR colorArg1; // 2
			D3DCOLOR colorArg2; // 3
			D3DTEXTUREOP alphaOp; // 4
			D3DCOLOR alphaArg1; // 5
			D3DCOLOR alphaArg2; // 6
			float bumpEnvMat00; // 7
			float bumpEnvMat01; // 8
			float bumpEnvMat10; // 9
			float bumpEnvMat11; // 10
			UINT texCoordIndex; // 11
			DWORD emptyAddress; // 12 - Used to be "D3DTSS_ADDRESS" which was of type D3DTEXTUREADDRESS in D3D7. It was a combination of ADDRESSU and ADDRESSV before they were split out - see d3d7types.h
			DWORD emptySamplerStates[8]; // 13 thru 21 (used to be where sampler state data was stored in D3D8 - see d3d8types.h)
			float bumpEnvLScale; // 22
			float bumpEnvLOffset; // 23
			D3DTEXTURETRANSFORMFLAGS textureTransformFlags; // 24
			DWORD emptyAddressW; // 25 - Used to be "D3DTSS_ADDRESSW" in D3D8 - see d3d8types.h
			D3DCOLOR colorArg0; // 26
			D3DCOLOR alphaArg0; // 27
			DWORD resultArg; // 28
			DWORD emptyUnknown29[3]; // 29 thru 31 - Unknown what these used to be for
			D3DCOLOR constant; // 32
		} namedStates;
		DWORD state[D3DTSS_CONSTANT];
	} stageStateUnion;
	static_assert(sizeof(_stageStateUnion) == sizeof(DWORD) * D3DTSS_CONSTANT, "Error: Unexpected union size!");
};

struct TexturePaletteState
{
	TexturePaletteState() : currentPaletteIndex(0), paletteEntries(NULL)
	{
	}

	~TexturePaletteState()
	{
		currentPaletteIndex = 0;

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

// This is the line pattern struct from D3D8. It is used in the D3DRS_LINEPATTERN (10) render state.
// Header: D3d8types.h
/* This structure describes a line pattern.
These values are used by the D3DRS_LINEPATTERN render state in the D3DRENDERSTATETYPE enumerated type.
A line pattern specifies how a line is drawn. The line pattern is always the same, no matter where it is started. (This differs from stippling, which affects how objects are rendered; that is, to imitate transparency.)
The line pattern specifies up to a 16-pixel pattern of on and off pixels along the line. The wRepeatFactor member specifies how many pixels are repeated for each entry in wLinePattern. */
typedef struct _D3DLINEPATTERN
{
	WORD wRepeatFactor; // Number of times to repeat each series of 1s and 0s specified in the wLinePattern member. This allows an application to stretch the line pattern.
	WORD wLinePattern; // Bits specifying the line pattern. For example, the following value would produce a dotted line: 1100110011001100.
} D3DLINEPATTERN;

// This is from d3dtypes.h
typedef LONG D3DFIXED;

// This is from d3dtypes.h
typedef enum _D3DANTIALIASMODE
{
    D3DANTIALIAS_NONE          = 0,
    D3DANTIALIAS_SORTDEPENDENT = 1,
    D3DANTIALIAS_SORTINDEPENDENT = 2,
    D3DANTIALIAS_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DANTIALIASMODE;

// This corresponds to the SetROP2 GDI function's mix modes. The default D3D render state value is R2_COPYPEN. Read here for more info: https://docs.microsoft.com/en-us/windows/desktop/api/wingdi/nf-wingdi-setrop2
typedef DWORD ROP2;

// This is from d3dtypes.h
// Note that the values do *not* line up with the D3D9 D3DTEXTUREFILTERTYPE enum after 2!
typedef enum _D3DTEXTUREFILTER
{
    D3DFILTER_NEAREST          = 1,
    D3DFILTER_LINEAR           = 2,
    D3DFILTER_MIPNEAREST       = 3,
    D3DFILTER_MIPLINEAR        = 4,
    D3DFILTER_LINEARMIPNEAREST = 5,
    D3DFILTER_LINEARMIPLINEAR  = 6,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DFILTER_FORCE_DWORD      = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DTEXTUREFILTER;

// This is from d3dtypes.h
typedef enum _D3DTEXTUREBLEND
{
    D3DTBLEND_DECAL            = 1,
    D3DTBLEND_MODULATE         = 2,
    D3DTBLEND_DECALALPHA       = 3,
    D3DTBLEND_MODULATEALPHA    = 4,
    D3DTBLEND_DECALMASK        = 5,
    D3DTBLEND_MODULATEMASK     = 6,
    D3DTBLEND_COPY             = 7,
#if(DIRECT3D_VERSION >= 0x0500)
    D3DTBLEND_ADD              = 8,
    D3DTBLEND_FORCE_DWORD      = 0x7fffffff, /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
} D3DTEXTUREBLEND;

// This is from d3dtypes.h
typedef float D3DVALUE;

// All of the render states available via SetRenderState() and GetRenderState()
// Reference for the D3D9-supported ones: https://msdn.microsoft.com/en-us/library/windows/desktop/bb172599(v=vs.85).aspx
__declspec(align(16) ) struct RenderStates
{
	RenderStates();

	~RenderStates()
	{
		memset(this, 0, sizeof(*this) );
	}

#define MAX_NUM_RENDERSTATES (D3DRS_BLENDOPALPHA + 1)

	union _renderStatesUnion
	{
		struct _namedStates
		{
			DWORD empty0; // 0
			DWORD textureHandle_D3D1; // 1 (only used in D3D1 thru D3D6)
			D3DANTIALIASMODE antialias_D3D5; // 2 (only used in D3D5, D3D6, and D3D7)
			D3DTEXTUREADDRESS textureAddress_D3D1; // 3 (only used in D3D1 thru D3D6)
			BOOL texturePerspective_D3D1; // 4 (only used in D3D1 thru D3D7)
			BOOL wrapU_D3D1; // 5 (only used in D3D1 thru D3D6)
			BOOL wrapV_D3D1; // 6 (only used in D3D1 thru D3D6)
			D3DZBUFFERTYPE zEnable; // 7
			D3DFILLMODE fillmode; // 8
			D3DSHADEMODE shadeMode; // 9
			D3DLINEPATTERN linePattern_D3D1; // 10 (only used in D3D1 thru D3D8)
			BOOL monoEnable_D3D1; // 11 (only used in D3D1 thru D3D6)
			ROP2 rop2_D3D1; // 12 (only used in D3D1 thru D3D6)
			DWORD planeMask_D3D1; // 13 (only used in D3D1 thru D3D6)
			BOOL zWriteEnable; // 14
			BOOL alphaTestEnable; // 15
			BOOL lastPixel; // 16
			D3DTEXTUREFILTER textureMag_D3D1; // 17 (only used in D3D1 thru D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
			D3DTEXTUREFILTER textureMin_D3D1; // 18 (only used in D3D1 thru D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
			D3DBLEND srcBlend; // 19
			D3DBLEND destBlend;// 20
			D3DTEXTUREBLEND textureMapBlend_D3D1; // 21 (only used in D3D1 thru D3D6)
			D3DCULL cullmode; // 22
			D3DCMPFUNC zFunc; // 23
			D3DFIXED alphaRef; // 24
			D3DCMPFUNC alphaFunc; // 25
			BOOL ditherEnable; // 26
			BOOL alphaBlendEnable; // 27 (D3D5+ only)
			BOOL fogEnable; // 28
			BOOL specularEnable; // 29
			BOOL zVisible_D3D1; // 30 (only used in D3D1 thru D3D8, seems to do a very similar thing as D3DRS_ZENABLE?)
			BOOL subPixel_D3D1; // 31 (only used in D3D1 thru D3D6)
			BOOL subPixelX_D3D1; // 32 (only used in D3D1 thru D3D6)
			BOOL stippledAlpha_D3D1; // 33 (only used in D3D1 thru D3D7)
			D3DCOLOR fogColor; // 34
			D3DFOGMODE fogTableMode; // 35
			float fogStart; // 36 (only used in D3D7+)
			float fogEnd; // 37 (only used in D3D7+)
			float fogDensity; // 38 (only used in D3D7+)
			BOOL stippleEnable_D3D1; // 39 (only used in D3D1 thru D3D6)
			BOOL edgeAntialias_D3D5; // 40 (only used in D3D5 thru D3D8)
			BOOL colorKeyEnable_D3D5; // 41 (only used in D3D5, D3D6, and D3D7)
			DWORD oldAlphaBlendEnable_D3D1; // 42 (only used in D3D1 thru D3D6, replaced with D3DRS_ALPHABLENDENABLE in D3D7+)
			D3DCOLOR borderColor_D3D5; // 43 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7+)
			D3DTEXTUREADDRESS textureAddressU_D3D5; // 44 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
			D3DTEXTUREADDRESS textureAddressV_D3D5; // 45 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
			D3DVALUE mipMapLoDBias_D3D5; // 46 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
			LONG zBias_D3D8; // 47 (only used in D3D5 thru D3D8)
			BOOL rangeFogEnable; // 48 (D3D5+ only)
			DWORD maxAnisotropy_D3D5; // 49 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
			BOOL flushBatch_D3D5; // 50 (D3D5 only)
			BOOL translucentSortIndependent_D3D6; // 51 (D3D6 only)
			BOOL stencilEnable; // 52 (D3D6+ only)
			D3DSTENCILOP stencilFail; // 53 (D3D6+ only)
			D3DSTENCILOP stencilZFail; // 54 (D3D6+ only)
			D3DSTENCILOP stencilPass; // 55 (D3D6+ only)
			D3DCMPFUNC stencilFunc; // 56 (D3D6+ only)
			UINT stencilRef; // 57 (D3D6+ only)
			DWORD stencilMask; // 58 (D3D6+ only)
			DWORD stencilWriteMask; // 59 (D3D6+ only)
			D3DCOLOR textureFactor; // 60 (D3D6+ only)
			DWORD empty61[3]; // 61 thru 63
			DWORD stipplePattern[32]; // 64 thru 95 (D3D1 thru D3D6 only)
			DWORD empty96[32]; // 96 thru 127
			DWORD wrap0; // 128 (D3D6+ only) (This used to be "D3DRENDERSTATE_WRAPBIAS" in D3D6)
			DWORD wrap1; // 129	(D3D6+ only)
			DWORD wrap2; // 130	(D3D6+ only)
			DWORD wrap3; // 131	(D3D6+ only)
			DWORD wrap4; // 132	(D3D6+ only)
			DWORD wrap5; // 133	(D3D6+ only)
			DWORD wrap6; // 134	(D3D6+ only)
			DWORD wrap7; // 135	(D3D6+ only)
			BOOL clipping; // 136 (D3D7+ only)
			BOOL lighting; // 137 (D3D7+ only)
			BOOL extents_D3D7; // 138 (D3D7 only)
			D3DCOLOR ambient; // 139 (D3D7+ only)
			D3DFOGMODE fogVertexMode; // 140 (D3D7+ only)
			BOOL colorVertex; // 141 (D3D7+ only)
			BOOL localViewer; // 142 (D3D7+ only)
			BOOL normalizeNormals; // 143 (D3D7+ only)
			BOOL colorKeyBlendEnable_D3D7; // 144 (D3D7 only)
			D3DMATERIALCOLORSOURCE diffuseMaterialSource; // 145 (D3D7+ only)
			D3DMATERIALCOLORSOURCE specularMaterialSource; // 146 (D3D7+ only)
			D3DMATERIALCOLORSOURCE ambientMaterialSource; // 147 (D3D7+ only)
			D3DMATERIALCOLORSOURCE emissiveMaterialSource; // 148 (D3D7+ only)
			DWORD empty149[2]; // 149 and 150
			D3DVERTEXBLENDFLAGS vertexBlend; // 151 (D3D7+ only)
			DWORD clipPlaneEnable; // 152 (D3D7+ only)
			BOOL softwareVertexProcessing_D3D8; // 153 (only used in D3D8, replaced with IDirect3DDevice9::SetSoftwareVertexProcessing() in D3D9)
			float pointSize; // 154
			float pointSize_Min; // 155
			BOOL pointSpriteEnable; // 156
			BOOL pointScaleEnable; // 157
			float pointScale_A; // 158
			float pointScale_B; // 159
			float pointScale_C; // 160
			BOOL multisampleAntialias; // 161
			DWORD multisampleMask; // 162
			D3DPATCHEDGESTYLE patchEdgeStyle; // 163
			DWORD patchSegments_D3D8; // 164 (only used in D3D8)
			D3DDEBUGMONITORTOKENS debugMonitorToken; // 165
			float pointSize_Max; // 166
			BOOL indexedVertexBlendEnable; // 167
			DWORD colorWriteEnable; // 168
			DWORD empty169; // 169
			float tweenFactor; // 170
			D3DBLENDOP blendOp; // 171
			D3DDEGREETYPE positionDegree; // 172 (in D3D8 this was called "D3DRS_POSITIONORDER")
			D3DDEGREETYPE normalDegree; // 173 (in D3D8 this was called "D3DRS_NORMALORDER")
			BOOL scissorTestEnable; // 174
			float slopeScaledDepthBias; // 175
			BOOL antialiasedLineEnable; // 176
			DWORD empty177; // 177
			float minTessellationLevel; // 178
			float maxTessellationLevel; // 179
			float adaptiveness_X; // 180
			float adaptiveness_Y; // 181
			float adaptiveness_Z; // 182
			float adaptiveness_W; // 183
			BOOL enableAdaptiveTessellation; // 184
			BOOL twoSidedStencilMode; // 185
			D3DSTENCILOP ccw_StencilFail; // 186
			D3DSTENCILOP ccw_StencilZFail; // 187
			D3DSTENCILOP ccw_StencilPass; // 188
			D3DCMPFUNC ccw_StencilFunc; // 189
			DWORD colorWriteEnable1; // 190
			DWORD colorWriteEnable2; // 191
			DWORD colorWriteEnable3; // 192
			D3DCOLOR blendFactor; // 193
			BOOL sRGBWriteEnable; // 194
			float depthBias; // 195
			DWORD empty196[2]; // 196 and 197
			DWORD wrap8; // 198
			DWORD wrap9; // 199
			DWORD wrap10; // 200
			DWORD wrap11; // 201
			DWORD wrap12; // 202
			DWORD wrap13; // 203
			DWORD wrap14; // 204
			DWORD wrap15; // 205
			BOOL separateAlphaBlendEnable; // 206
			D3DBLEND srcBlendAlpha; // 207
			D3DBLEND destBlendAlpha; // 208
			D3DBLENDOP blendOpAlpha; // 209
		} namedStates;
		DWORD states[MAX_NUM_RENDERSTATES];
	} renderStatesUnion;
	static_assert(sizeof(_renderStatesUnion) == sizeof(DWORD) * MAX_NUM_RENDERSTATES, "Error: Unexpected union size!");

	// Cache some derived values from SetRenderState() for more efficient runtime usage:
	__declspec(align(16) ) float cachedAlphaRefFloat;
	__declspec(align(16) ) D3DXVECTOR4 cachedAmbient;
	__declspec(align(16) ) D3DXVECTOR4 cachedBlendFactor;
	__declspec(align(16) ) D3DXVECTOR4 cachedInvBlendFactor;
	__declspec(align(16) ) __m128 depthBiasSplatted;
	__declspec(align(16) ) __m128 alphaRefSplatted;
};

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
		ViewTransform = newView;
	}

	inline void SetProjectionTransform(const D3DXMATRIXA16& newProj)
	{
		wvpDirty = true;
		ProjectionTransform = newProj;
	}

	inline void SetWorldTransform(const D3DXMATRIXA16& newWorld, const unsigned worldIndex)
	{
		wvpDirty = true;
		WorldTransforms[worldIndex] = newWorld;
	}

	inline void SetTextureTransform(const D3DXMATRIXA16& newTexture, const unsigned textureIndex)
	{
		TextureTransforms[textureIndex] = newTexture;
	}

	D3DXMATRIXA16 ViewTransform;
	D3DXMATRIXA16 ProjectionTransform;
	D3DXMATRIXA16 TextureTransforms[D3DDP_MAXTEXCOORD];
	D3DXMATRIXA16 WorldTransforms[MAX_WORLD_TRANSFORMS];

	inline const D3DXMATRIXA16& GetWVPTransform() const
	{
		if (wvpDirty)
		{
			wvpDirty = false;
			CachedWVPTransform = WorldTransforms[0] * ViewTransform * ProjectionTransform;
		}
		return CachedWVPTransform;
	}

	mutable D3DXMATRIXA16 CachedWVPTransform;
	mutable bool wvpDirty;
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
	DeviceState() : currentIndexBuffer(NULL), currentVertexShader(NULL), currentPixelShader(NULL), currentVertexDecl(NULL), declTarget(targetFVF), currentSwvpEnabled(FALSE), currentDepthStencil(NULL)
	{
		currentFVF.rawFVF_DWORD = 0x00000000;

		memset(&currentRenderTargets, 0, sizeof(currentRenderTargets) );
		memset(&currentTextures, 0, sizeof(currentTextures) );
		memset(&currentCubeTextures, 0, sizeof(currentTextures) );
		memset(&currentVolumeTextures, 0, sizeof(currentTextures) );
		currentStageStates[0].SetStage0Defaults();

		memset(&currentMaterial, 0, sizeof(currentMaterial) );
		memset(&enabledLightIndices, 0, sizeof(enabledLightIndices) );

		for (unsigned x = 0; x < D3DMAXUSERCLIPPLANES; ++x)
		{
			currentClippingPlanes[x] = D3DXPLANE(0.0f, 0.0f, 0.0f, 0.0f);
		}

		lightInfoMap = new std::map<UINT, LightInfo*>();
	}

#define MAX_NUM_SAMPLERS D3DVERTEXTEXTURESAMPLER3
#define MAX_NUM_TEXTURE_STAGE_STATES 8

	~DeviceState()
	{
		currentIndexBuffer = NULL;
		currentVertexShader = NULL;
		currentPixelShader = NULL;
		currentSwvpEnabled = FALSE;
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
	BOOL currentSwvpEnabled; // Note that this parameter is not recorded by State Blocks (TODO: Move this out of the device state)

	// TODO: Refactor the hooks inheritance tree to make this work with baseTextures so we don't need these duplicates:
	IDirect3DTexture9Hook* currentTextures[MAX_NUM_SAMPLERS];
	IDirect3DCubeTexture9Hook* currentCubeTextures[MAX_NUM_SAMPLERS];
	IDirect3DVolumeTexture9Hook* currentVolumeTextures[MAX_NUM_SAMPLERS];
	SamplerState currentSamplerStates[MAX_NUM_SAMPLERS];
	TextureStageState currentStageStates[MAX_NUM_TEXTURE_STAGE_STATES];

	D3DXPLANE currentClippingPlanes[D3DMAXUSERCLIPPLANES];

	D3DMATERIAL9 currentMaterial;

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
	} vs_to_ps_mappings;
};

__declspec(align(16) ) struct primitivePixelJobData
{
	primitivePixelJobData() : invZ(0.0f, 0.0f, 0.0f), barycentricNormalizeFactor(0.0f), primitiveID(0), VFace(true), vertex0index(0), vertex1index(0), vertex2index(0)
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

	// Handles running the pixel shader and interpolating input for this pixel from a vertex declaration + raw vertex stream
	void ShadePixelFromStream(PShaderEngine* const pixelEngine, const DeclarationSemanticMapping& vertexDeclMapping, const unsigned x, const unsigned y, const __m128 barycentricInterpolants, 
		const UINT offsetBytesToOPosition, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, const __m128 invZ) const;

	// Handles running the pixel shader and interpolating input for this pixel from a vertex declaration + raw vertex stream
	void ShadePixelFromStream4(PShaderEngine* const pixelEngine, const DeclarationSemanticMapping& vertexDeclMapping, const __m128i x4, const __m128i y4, const __m128 (&barycentricInterpolants)[4], 
		const UINT offsetBytesToOPosition, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, const __m128 invZ) const;

	// Handles running the pixel shader from a processed vertex shade
	void ShadePixelFromShader(PShaderEngine* const pixelEngine, const VStoPSMapping& vs_psMapping, const unsigned x, const unsigned y, const __m128 barycentricInterpolants, 
		const UINT byteOffsetToOPosition, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, const __m128 invZ) const;

	// Handles running the pixel shader from a processed vertex shade
	void ShadePixelFromShader4(PShaderEngine* const pixelEngine, const VStoPSMapping& vs_psMapping, const __m128i x4, const __m128i y4, const __m128 (&barycentricInterpolants)[4], 
		const UINT byteOffsetToOPosition, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, const __m128 invZ) const;

	// Handles blending and write-masking
	void RenderOutput(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const;

	// Handles blending and write-masking
	template <const unsigned char channelWriteMask>
	void ROPBlendWriteMask(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const;

	// Handles blending and write-masking
	template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
	void ROPBlendWriteMask4(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const D3DXVECTOR4 (&value)[4]) const;

	// Handles interpolating pixel shader input registers from a vertex declaration + raw vertex stream
	void InterpolateStreamIntoRegisters(PShaderEngine* const pixelShader, const DeclarationSemanticMapping& vertexDeclMapping, const __m128 barycentricInterpolants, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, const __m128 invZ, const float pixelZ) const;

	// Handles interpolating pixel shader input registers from a vertex declaration + raw vertex stream
	template <const unsigned char pixelWriteMask>
	void InterpolateStreamIntoRegisters4(PShaderEngine* const pixelShader, const DeclarationSemanticMapping& vertexDeclMapping, const __m128 (&barycentricInterpolants)[4], CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, const __m128 invZ, const __m128 pixelZ4) const;

	// Handles interpolating pixel shader input registers from vertex shader output registers
	void InterpolateShaderIntoRegisters(PShaderEngine* const pixelShader, const VStoPSMapping& vs_psMapping, const __m128 barycentricInterpolants, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, const __m128 invZ, const float pixelZ) const;

	// Handles interpolating pixel shader input registers from vertex shader output registers
	template <const unsigned char pixelWriteMask>
	void InterpolateShaderIntoRegisters4(PShaderEngine* const pixelShader, const VStoPSMapping& vs_psMapping, const __m128 (&barycentricInterpolants)[4], const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, const __m128 invZ, const __m128 pixelZ4) const;

	const float InterpolatePixelDepth(const __m128 barycentricInterpolants, const __m128 invZ) const;
	void InterpolatePixelDepth4(const __m128 (&barycentricInterpolants4)[4], const __m128 invZ, __m128& outPixelDepth4) const;

	// Must be called before shading a pixel to reset the pixel shader state machine!
	void PreShadePixel(const unsigned x, const unsigned y, PShaderEngine* const pixelShader) const;

	// Must be called before shading a pixel to reset the pixel shader state machine!
	void PreShadePixel4(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const;

	void ShadePixel(const unsigned x, const unsigned y, PShaderEngine* const pixelShader) const;
	
	template <const unsigned pixelWriteMask>
	void ShadePixel4(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const;

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
	void CreateNewPixelShadeJob4(const __m128i x4, const __m128i y4, const __m128i (&barycentricsAdjusted4)[4], const primitivePixelJobData* const primitiveData) const;

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

	inline void LockDeviceCS(void)
	{
		EnterCriticalSection(&deviceCS);
	}

	inline void UnlockDeviceCS(void)
	{
		LeaveCriticalSection(&deviceCS);
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

	mutable __declspec(align(16) ) VShaderEngine deviceMainVShaderEngine;
	mutable __declspec(align(16) ) PShaderEngine deviceMainPShaderEngine;
	mutable __declspec(align(16) ) VS_2_0_ConstantsBuffer vsDrawCallCB;
	mutable __declspec(align(16) ) PS_2_0_ConstantsBuffer psDrawCallCB;

#ifndef NO_CACHING_FVF_VERT_DECLS
	std::map<DWORD, IDirect3DVertexDeclaration9Hook*>* FVFToVertDeclCache;
#endif // NO_CACHING_FVF_VERT_DECLS

	DeviceState currentState;

	// This is the implicit swap chain:
	IDirect3DSwapChain9Hook* implicitSwapChain;

	LightInfo defaultLight;

	// For debug-printing efficiently
	HANDLE hConsoleHandle;

	// This is the data that gets directly consumed by occlusion queries (D3DQUERYTYPE_OCCLUSION).
	// Aligned and volatile here because this variable is accessed by multiple threads using Interlocked operations
	volatile __declspec(align(16) ) DWORD numPixelsPassedZTest;

	// Saving a pointer to a persistent buffer on the device here to avoid reallocation of output vertex buffers all the time
	mutable VS_2_0_OutputRegisters* processedVertexBuffer;
	mutable unsigned processedVertsUsed;
	mutable unsigned processVertsAllocated;

	// TODO: Render some stats with this loaded overlay font
	IDirect3DTexture9Hook* overlayFontTexture;

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

	// Swizzle from RGBA -> ARGB
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
inline void ColorDWORDToFloat4_4(const D3DCOLOR** const inColor4, D3DXVECTOR4* const outColor4[4])
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

	const __m128 float4color4[4] =
	{
		*(const __m128* const)&color4[0],
		*(const __m128* const)&color4[1],
		*(const __m128* const)&color4[2],
		*(const __m128* const)&color4[3]
	};

	const __m128 float4colorClamped4 = 
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
inline const __m128i Float4ToX8R8G8B8_4Clamp4(const D3DXVECTOR4 (&color4)[4], const __m128i outColorAddresses4)
{
	if (pixelWriteMask == 0)
	{
#ifdef _DEBUG
		__debugbreak(); // Shouldn't be executing this code if we're not going to write anything out
#endif
		return;
	}

	Float4ToD3DCOLOR4Clamp4<channelWriteMask & 0x7>(color4, outColorAddresses4);
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
		D3DXFLOAT16& outColor = *(writeAddresses[0]);
		if (channelWriteMask & 0x1) outColor.r = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[0]);
		if (channelWriteMask & 0x2) outColor.g = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[1]);
		if (channelWriteMask & 0x4) outColor.b = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[2]);
		if (channelWriteMask & 0x8)	outColor.a = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[3]);
	}
	if (pixelWriteMask & 0x2)
	{
		D3DXFLOAT16& outColor = *(writeAddresses[1]);
		if (channelWriteMask & 0x1) outColor.r = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[0]);
		if (channelWriteMask & 0x2) outColor.g = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[1]);
		if (channelWriteMask & 0x4) outColor.b = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[2]);
		if (channelWriteMask & 0x8)	outColor.a = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[3]);
	}
	if (pixelWriteMask & 0x4)
	{
		D3DXFLOAT16& outColor = *(writeAddresses[2]);
		if (channelWriteMask & 0x1) outColor.r = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[0]);
		if (channelWriteMask & 0x2) outColor.g = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[1]);
		if (channelWriteMask & 0x4) outColor.b = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[2]);
		if (channelWriteMask & 0x8)	outColor.a = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[3]);
	}
	if (pixelWriteMask & 0x8)
	{
		D3DXFLOAT16& outColor = *(writeAddresses[3]);
		if (channelWriteMask & 0x1) outColor.r = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[0]);
		if (channelWriteMask & 0x2) outColor.g = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[1]);
		if (channelWriteMask & 0x4) outColor.b = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[2]);
		if (channelWriteMask & 0x8)	outColor.a = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[3]);
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
			A32B32G32R32F& outColor = outColor4[x];
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

	if ( (channelWriteMask & 0x1) != 1)
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
	{
		if (channelWriteMask & 0x1)
			outColor = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[0]);
	}
	if (pixelWriteMask & 0x2)
	{
		if (channelWriteMask & 0x1)
			outColor = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[1]);
	}
	if (pixelWriteMask & 0x4)
	{
		if (channelWriteMask & 0x1)
			outColor = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[2]);
	}
	if (pixelWriteMask & 0x8)
	{
		if (channelWriteMask & 0x1)
			outColor = *(const D3DXFLOAT16* const)&(half4Color.m128i_u16[3]);
	}
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
