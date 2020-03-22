#pragma once

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

#ifdef _DEBUG
// These are bit-fields for each of the deprecated render states (up to 255) as of D3D9.
// Note that the presence of a "1" bit indicates "deprecated" whereas a "0" bit can mean either "unused" or "not deprecated"
static const unsigned long deprecatedD3D9RenderStatesBits[8] =
{
	0xc0263c7e,
	0x000a7a83,
	0xffffffff,
	0x00000000,
	0x02010000,
	0x00000010,
	0x00000030,
	0x00000000
};
static const bool IsRenderStateD3D9Deprecated(const D3DRENDERSTATETYPE renderState)
{
	// Out of bounds states cannot be deprecated
	if (renderState > 255)
		return false;

	const unsigned bitmaskDWORDIndex = renderState / 32;
	const unsigned bitmask = 1 << (renderState % 32);
	return (deprecatedD3D9RenderStatesBits[bitmaskDWORDIndex] & bitmask) != 0;
}
#endif // #ifdef _DEBUG

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

	enum _simplifiedAlphaBlendMode
	{
		noAlphaBlending = 0, // Alpha blending disabled
		alphaBlending, // D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA
		additiveBlending, // D3DBLEND_ONE, D3DBLEND_ONE
		multiplicativeBlending, // D3DBLEND_DESTCOLOR, D3DBLEND_ZERO or D3DBLEND_ZERO, D3DBLEND_SRCCOLOR
		otherAlphaBlending // Any other mode with alpha blending enabled (including separate alpha blend modes)
	} simplifiedAlphaBlendMode;

	bool alphaBlendNeedsDestRead;
};

enum renderStateStateBlockType : unsigned char
{
	notIncludedInStateBlockState = 0x0,
	vertexState = 0x1,
	pixelState = 0x2,
	SBT_ALL_State = (vertexState | pixelState)
};

// As a note, state-blocks didn't exist prior to Direct3D 7.0, so we should assume that states that don't exist in D3D7+ are unaffected by state blocks (unless the states clearly map to existing D3D8/D3D9 states that have been moved
// outside of SetRenderState, such as all of the TextureStageStates)
const renderStateStateBlockType renderStateStateBlockTypes[] =
{
	notIncludedInStateBlockState, // DWORD empty0; // 0
	notIncludedInStateBlockState, // DWORD textureHandle_D3D1; // 1 (only used in D3D1 thru D3D6)
	notIncludedInStateBlockState, // D3DANTIALIASMODE antialias_D3D5; // 2 (only used in D3D5, D3D6, and D3D7)
	notIncludedInStateBlockState, // D3DTEXTUREADDRESS textureAddress_D3D1; // 3 (only used in D3D1 thru D3D6)
	notIncludedInStateBlockState, // BOOL texturePerspective_D3D1; // 4 (only used in D3D1 thru D3D7)
	notIncludedInStateBlockState, // BOOL wrapU_D3D1; // 5 (only used in D3D1 thru D3D6)
	notIncludedInStateBlockState, // BOOL wrapV_D3D1; // 6 (only used in D3D1 thru D3D6)
	pixelState, // D3DZBUFFERTYPE zEnable; // 7
	pixelState, // D3DFILLMODE fillmode; // 8
	SBT_ALL_State, // D3DSHADEMODE shadeMode; // 9 // This isn't documented as such anywhere, but it empirically gets captured in vertex state blocks for some reason
	notIncludedInStateBlockState, // D3DLINEPATTERN linePattern_D3D1; // 10 (only used in D3D1 thru D3D8)
	notIncludedInStateBlockState, // BOOL monoEnable_D3D1; // 11 (only used in D3D1 thru D3D6)
	notIncludedInStateBlockState, // ROP2 rop2_D3D1; // 12 (only used in D3D1 thru D3D6)
	notIncludedInStateBlockState, // DWORD planeMask_D3D1; // 13 (only used in D3D1 thru D3D6)
	pixelState, // BOOL zWriteEnable; // 14
	pixelState, // BOOL alphaTestEnable; // 15
	pixelState, // BOOL lastPixel; // 16
	notIncludedInStateBlockState, // D3DTEXTUREFILTER textureMag_D3D1; // 17 (only used in D3D1 thru D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
	notIncludedInStateBlockState, // D3DTEXTUREFILTER textureMin_D3D1; // 18 (only used in D3D1 thru D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
	pixelState, // D3DBLEND srcBlend; // 19
	pixelState, // D3DBLEND destBlend;// 20
	notIncludedInStateBlockState, // D3DTEXTUREBLEND textureMapBlend_D3D1; // 21 (only used in D3D1 thru D3D6)
	vertexState, // D3DCULL cullmode; // 22
	pixelState, // D3DCMPFUNC zFunc; // 23
	pixelState, // D3DFIXED alphaRef; // 24
	pixelState, // D3DCMPFUNC alphaFunc; // 25
	pixelState, // BOOL ditherEnable; // 26
	pixelState, // BOOL alphaBlendEnable; // 27 (D3D5+ only)
	vertexState, // BOOL fogEnable; // 28 // The D3D8 and D3D9 docs don't list this one as getting captured under vertex state, but it does get captured under vertex state, and the D3D7 docs correctly state as such.
	vertexState, // BOOL specularEnable; // 29 // The D3D8 and D3D9 docs don't list this one as getting captured under vertex state, but it does get captured under vertex state, and the D3D7 docs correctly state as such.
	notIncludedInStateBlockState, // BOOL zVisible_D3D1; // 30 (only used in D3D1 thru D3D8, seems to do a very similar thing as D3DRS_ZENABLE?)
	notIncludedInStateBlockState, // BOOL subPixel_D3D1; // 31 (only used in D3D1 thru D3D6)
	notIncludedInStateBlockState, // BOOL subPixelX_D3D1; // 32 (only used in D3D1 thru D3D6)
	notIncludedInStateBlockState, // BOOL stippledAlpha_D3D1; // 33 (only used in D3D1 thru D3D7)
	vertexState, // D3DCOLOR fogColor; // 34
	vertexState, // D3DFOGMODE fogTableMode; // 35
	SBT_ALL_State, // float fogStart; // 36 (only used in D3D7+) // These three fog states (fogstart, fogend, and fogdensity) are shared with both vertex and pixel state blocks
	SBT_ALL_State, // float fogEnd; // 37 (only used in D3D7+)
	SBT_ALL_State, // float fogDensity; // 38 (only used in D3D7+)
	notIncludedInStateBlockState, // BOOL stippleEnable_D3D1; // 39 (only used in D3D1 thru D3D6)
	pixelState, // BOOL edgeAntialias_D3D5; // 40 (only used in D3D5 thru D3D8)
	notIncludedInStateBlockState, // BOOL colorKeyEnable_D3D5; // 41 (only used in D3D5, D3D6, and D3D7)
	notIncludedInStateBlockState, // DWORD oldAlphaBlendEnable_D3D1; // 42 (only used in D3D1 thru D3D6, replaced with D3DRS_ALPHABLENDENABLE in D3D7+)
	notIncludedInStateBlockState, // D3DCOLOR borderColor_D3D5; // 43 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7+)
	notIncludedInStateBlockState, // D3DTEXTUREADDRESS textureAddressU_D3D5; // 44 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
	notIncludedInStateBlockState, // D3DTEXTUREADDRESS textureAddressV_D3D5; // 45 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
	notIncludedInStateBlockState, // D3DVALUE mipMapLoDBias_D3D5; // 46 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
	notIncludedInStateBlockState, // LONG zBias_D3D8; // 47 (only used in D3D5 thru D3D8)
	vertexState, // BOOL rangeFogEnable; // 48 (D3D5+ only)
	notIncludedInStateBlockState, // DWORD maxAnisotropy_D3D5; // 49 (only used in D3D5 and D3D6, replaced with SetTextureStageState() in D3D7 and D3D8, then replaced with SetSamplerState() in D3D9)
	notIncludedInStateBlockState, // BOOL flushBatch_D3D5; // 50 (D3D5 only)
	notIncludedInStateBlockState, // BOOL translucentSortIndependent_D3D6; // 51 (D3D6 only)
	pixelState, // BOOL stencilEnable; // 52 (D3D6+ only)
	pixelState, // D3DSTENCILOP stencilFail; // 53 (D3D6+ only)
	pixelState, // D3DSTENCILOP stencilZFail; // 54 (D3D6+ only)
	pixelState, // D3DSTENCILOP stencilPass; // 55 (D3D6+ only)
	pixelState, // D3DCMPFUNC stencilFunc; // 56 (D3D6+ only)
	pixelState, // UINT stencilRef; // 57 (D3D6+ only)
	pixelState, // DWORD stencilMask; // 58 (D3D6+ only)
	pixelState, // DWORD stencilWriteMask; // 59 (D3D6+ only)
	pixelState, // D3DCOLOR textureFactor; // 60 (D3D6+ only)
	notIncludedInStateBlockState, // DWORD empty61[3]; // 61 thru 63
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, // DWORD stipplePattern[32]; // 64 thru 95 (D3D1 thru D3D6 only)
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, // DWORD empty96[32]; // 96 thru 127
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	notIncludedInStateBlockState, 
	pixelState, // DWORD wrap0; // 128 (D3D6+ only) (This used to be "D3DRENDERSTATE_WRAPBIAS" in D3D6)
	pixelState, // DWORD wrap1; // 129	(D3D6+ only)
	pixelState, // DWORD wrap2; // 130	(D3D6+ only)
	pixelState, // DWORD wrap3; // 131	(D3D6+ only)
	pixelState, // DWORD wrap4; // 132	(D3D6+ only)
	pixelState, // DWORD wrap5; // 133	(D3D6+ only)
	pixelState, // DWORD wrap6; // 134	(D3D6+ only)
	pixelState, // DWORD wrap7; // 135	(D3D6+ only)
	vertexState, // BOOL clipping; // 136 (D3D7+ only)
	vertexState, // BOOL lighting; // 137 (D3D7+ only)
	notIncludedInStateBlockState, // BOOL extents_D3D7; // 138 (D3D7 only)
	vertexState, // D3DCOLOR ambient; // 139 (D3D7+ only)
	vertexState, // D3DFOGMODE fogVertexMode; // 140 (D3D7+ only)
	vertexState, // BOOL colorVertex; // 141 (D3D7+ only)
	vertexState, // BOOL localViewer; // 142 (D3D7+ only) // The D3D9 docs lists this as a pixel state for some reason...
	vertexState, // BOOL normalizeNormals; // 143 (D3D7+ only) // Oddly enough, this is listed in the D3D8 and D3D7 documentation for vertex state block capture, but not in the D3D9 documentation for vertex state block capture.
	notIncludedInStateBlockState, // BOOL colorKeyBlendEnable_D3D7; // 144 (D3D7 only)
	vertexState, // D3DMATERIALCOLORSOURCE diffuseMaterialSource; // 145 (D3D7+ only) // The D3D9 docs mistakenly list these material source renderstates as a pixel state for some reason
	vertexState, // D3DMATERIALCOLORSOURCE specularMaterialSource; // 146 (D3D7+ only) // The D3D9 docs mistakenly list these material source renderstates as a pixel state for some reason
	vertexState, // D3DMATERIALCOLORSOURCE ambientMaterialSource; // 147 (D3D7+ only) // The D3D9 docs mistakenly list these material source renderstates as a pixel state for some reason
	vertexState, // D3DMATERIALCOLORSOURCE emissiveMaterialSource; // 148 (D3D7+ only) // The D3D9 docs mistakenly list these material source renderstates as a pixel state for some reason
	notIncludedInStateBlockState, // DWORD empty149[2]; // 149 and 150
	notIncludedInStateBlockState, 
	vertexState, // D3DVERTEXBLENDFLAGS vertexBlend; // 151 (D3D7+ only)
	vertexState, // DWORD clipPlaneEnable; // 152 (D3D7+ only)
	notIncludedInStateBlockState, // BOOL softwareVertexProcessing_D3D8; // 153 (only used in D3D8, replaced with IDirect3DDevice9::SetSoftwareVertexProcessing() in D3D9). Note that in D3D9 this value is *not* captured by state blocks on purpose!
	vertexState, // float pointSize; // 154
	vertexState, // float pointSize_Min; // 155
	vertexState, // BOOL pointSpriteEnable; // 156
	vertexState, // BOOL pointScaleEnable; // 157
	vertexState, // float pointScale_A; // 158
	vertexState, // float pointScale_B; // 159
	vertexState, // float pointScale_C; // 160
	vertexState, // BOOL multisampleAntialias; // 161
	vertexState, // DWORD multisampleMask; // 162
	vertexState, // D3DPATCHEDGESTYLE patchEdgeStyle; // 163
	notIncludedInStateBlockState, // DWORD patchSegments_D3D8; // 164 (only used in D3D8)
	notIncludedInStateBlockState, // D3DDEBUGMONITORTOKENS debugMonitorToken; // 165
	vertexState, // float pointSize_Max; // 166
	vertexState, // BOOL indexedVertexBlendEnable; // 167
	pixelState, // DWORD colorWriteEnable; // 168
	notIncludedInStateBlockState, // DWORD empty169; // 169
	vertexState, // float tweenFactor; // 170
	pixelState, // D3DBLENDOP blendOp; // 171
	vertexState, // D3DDEGREETYPE positionDegree; // 172 (in D3D8 this was called "D3DRS_POSITIONORDER")
	vertexState, // D3DDEGREETYPE normalDegree; // 173 (in D3D8 this was called "D3DRS_NORMALORDER")
	pixelState, // BOOL scissorTestEnable; // 174
	pixelState, // float slopeScaledDepthBias; // 175
	pixelState, // BOOL antialiasedLineEnable; // 176
	notIncludedInStateBlockState, // DWORD empty177; // 177
	vertexState, // float minTessellationLevel; // 178
	vertexState, // float maxTessellationLevel; // 179
	vertexState, // float adaptiveness_X; // 180
	vertexState, // float adaptiveness_Y; // 181
	vertexState, // float adaptiveness_Z; // 182
	vertexState, // float adaptiveness_W; // 183
	vertexState, // BOOL enableAdaptiveTessellation; // 184
	pixelState, // BOOL twoSidedStencilMode; // 185
	pixelState, // D3DSTENCILOP ccw_StencilFail; // 186
	pixelState, // D3DSTENCILOP ccw_StencilZFail; // 187
	pixelState, // D3DSTENCILOP ccw_StencilPass; // 188
	pixelState, // D3DCMPFUNC ccw_StencilFunc; // 189
	pixelState, // DWORD colorWriteEnable1; // 190
	pixelState, // DWORD colorWriteEnable2; // 191
	pixelState, // DWORD colorWriteEnable3; // 192
	pixelState, // D3DCOLOR blendFactor; // 193
	pixelState, // BOOL sRGBWriteEnable; // 194
	pixelState, // float depthBias; // 195
	notIncludedInStateBlockState, // DWORD empty196[2]; // 196 and 197
	notIncludedInStateBlockState,
	pixelState, // DWORD wrap8; // 198
	pixelState, // DWORD wrap9; // 199
	pixelState, // DWORD wrap10; // 200
	pixelState, // DWORD wrap11; // 201
	pixelState, // DWORD wrap12; // 202
	pixelState, // DWORD wrap13; // 203
	pixelState, // DWORD wrap14; // 204
	pixelState, // DWORD wrap15; // 205
	pixelState, // BOOL separateAlphaBlendEnable; // 206
	pixelState, // D3DBLEND srcBlendAlpha; // 207
	pixelState, // D3DBLEND destBlendAlpha; // 208
	pixelState // D3DBLENDOP blendOpAlpha; // 209
};
static_assert(ARRAYSIZE(renderStateStateBlockTypes) == MAX_NUM_RENDERSTATES, "Error! Array size mismatches enum size!");
