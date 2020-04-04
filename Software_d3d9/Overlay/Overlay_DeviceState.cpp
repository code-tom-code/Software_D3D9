#include "DebugOverlay.h"
#include "..\IDirect3DDevice9Hook.h"
#include "..\IDirect3DTexture9Hook.h"
#include "..\IDirect3DCubeTexture9Hook.h"
#include "..\IDirect3DVolumeTexture9Hook.h"
#include "..\IDirect3DSurface9Hook.h"
#include "..\IDirect3DIndexBuffer9Hook.h"
#include "..\IDirect3DVertexBuffer9Hook.h"
#include "..\IDirect3DVertexDeclaration9Hook.h"

static const D3DCOLOR stateEnabledColor = D3DCOLOR_ARGB(255, 255, 255, 255);
static const D3DCOLOR stateDisabledColor = D3DCOLOR_ARGB(255, 160, 160, 160);
static const D3DCOLOR stateEnabledWarningColor = D3DCOLOR_ARGB(255, 255, 255, 0);
static const D3DCOLOR stateDisabledWarningColor = D3DCOLOR_ARGB(255, 160, 160, 0);

enum deviceStatePage : unsigned char
{
	deviceGeneralState = 0,
	lightState,
	viewportScissorBackbufferState,
	fogState,
	inputAssemblerState,
	tessellationState,
	depthStencilState,
	outputState,
	textureStageStates,
	samplerStates,
	textureState,
	materialLightingState,
	transformsState,
	vsConstantsState,
	psConstantsState,

	MAX_NUM_STATE_PAGES // This must always be last
};

static const char* const deviceStatePageNames[] =
{
	"General",
	"Lights",
	"View/Scissor",
	"Fog",
	"IA",
	"Tess",
	"Depth-Stencil",
	"Output",
	"TexStageStates",
	"Samplers",
	"Textures",
	"Lighting/Material",
	"Transforms",
	"VS Constants",
	"PS Constants"
};
static_assert(ARRAYSIZE(deviceStatePageNames) == MAX_NUM_STATE_PAGES, "Error: Missing page name string table entries!");

typedef void (*DrawFunctionType)(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);

void DrawDeviceGeneral(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawLightState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawViewportScissorBackbufferState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawFogState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawInputAssemblerState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawTessellationState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawDepthStencilState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawOutputState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawTextureStageStates(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawSamplerStates(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawTextureState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawMaterialLightingState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawTransformsState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawVSConstantsState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);
void DrawPSConstantsState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset);

static const DrawFunctionType drawDeviceStateFunctions[] =
{
	&DrawDeviceGeneral,
	&DrawLightState,
	&DrawViewportScissorBackbufferState,
	&DrawFogState,
	&DrawInputAssemblerState,
	&DrawTessellationState,
	&DrawDepthStencilState,
	&DrawOutputState,
	&DrawTextureStageStates,
	&DrawSamplerStates,
	&DrawTextureState,
	&DrawMaterialLightingState,
	&DrawTransformsState,
	&DrawVSConstantsState,
	&DrawPSConstantsState
};
static_assert(ARRAYSIZE(drawDeviceStateFunctions) == MAX_NUM_STATE_PAGES, "Error: Missing draw function entry");

union deviceStateScreenState
{
	struct
	{
		deviceStatePage currentPage;
		unsigned short currentLine;
	} deviceStateData;

	void* genericData;
};
static_assert(sizeof(deviceStateScreenState) == sizeof(void*), "Error: Unexpected union size!");

static inline const char* const GetAbbreviatedFormatStringFromFormat(const D3DFORMAT fmt)
{
	switch (fmt)
	{
	case D3DFMT_UNKNOWN:        // = 0
		return "UNKNOWN";
	case D3DFMT_R8G8B8:              // = 20,
		return "R8G8B8";
    case D3DFMT_A8R8G8B8:            // = 21,
		return "A8R8G8B8";
    case D3DFMT_X8R8G8B8:            // = 22,
		return "X8R8G8B8";
	case D3DFMT_R5G6B5:              // = 23,
		return "R5G6B5";
    case D3DFMT_X1R5G5B5:            // = 24,
		return "X1R5G5B5";
    case D3DFMT_A1R5G5B5:            // = 25,
		return "A1R5G5B5";
    case D3DFMT_A4R4G4B4:            // = 26,
		return "A4R4G4B4";
    case D3DFMT_R3G3B2:              // = 27,
		return "R3G3B2";
    case D3DFMT_A8:                  // = 28,
		return "A8";
    case D3DFMT_A8R3G3B2:            // = 29,
		return "A8R3G3B2";
    case D3DFMT_X4R4G4B4:            // = 30,
		return "X4R4G4B4";
    case D3DFMT_A2B10G10R10:         // = 31,
		return "A2B10G10R10";
    case D3DFMT_A8B8G8R8:            // = 32,
		return "A8B8G8R8";
    case D3DFMT_X8B8G8R8:            // = 33,
		return "X8B8G8R8";
    case D3DFMT_G16R16:              // = 34,
		return "G16R16";
    case D3DFMT_A2R10G10B10:         // = 35,
		return "A2R10G10B10";
    case D3DFMT_A16B16G16R16:        // = 36,
		return "A16B16G16R16";
    case D3DFMT_A8P8:                // = 40,
		return "A8P8";
    case D3DFMT_P8:                  // = 41,
		return "P8";
    case D3DFMT_L8:                  // = 50,
		return "L8";
    case D3DFMT_A8L8:                // = 51,
		return "A8L8";
    case D3DFMT_A4L4:                // = 52,
		return "A4L4";
    case D3DFMT_V8U8:                // = 60,
		return "V8U8";
    case D3DFMT_L6V5U5:              // = 61,
		return "L6V5U5";
    case D3DFMT_X8L8V8U8:            // = 62,
		return "X8L8V8U8";
    case D3DFMT_Q8W8V8U8:            // = 63,
		return "Q8W8V8U8";
    case D3DFMT_V16U16:              // = 64,
		return "V16U16";
    case D3DFMT_A2W10V10U10:         // = 67,
		return "A2W10V10U10";
    case D3DFMT_UYVY:                // = MAKEFOURCC('U', 'Y', 'V', 'Y'),
		return "UYVY";
    case D3DFMT_R8G8_B8G8:           // = MAKEFOURCC('R', 'G', 'B', 'G'),
		return "R8G8_B8G8";
    case D3DFMT_YUY2:                // = MAKEFOURCC('Y', 'U', 'Y', '2'),
		return "YUY2";
    case D3DFMT_G8R8_G8B8:           // = MAKEFOURCC('G', 'R', 'G', 'B'),
		return "G8R8_G8B8";
    case D3DFMT_DXT1:                // = MAKEFOURCC('D', 'X', 'T', '1'),
		return "DXT1";
    case D3DFMT_DXT2:                // = MAKEFOURCC('D', 'X', 'T', '2'),
		return "DXT2";
    case D3DFMT_DXT3:                // = MAKEFOURCC('D', 'X', 'T', '3'),
		return "DXT3";
    case D3DFMT_DXT4:                // = MAKEFOURCC('D', 'X', 'T', '4'),
		return "DXT4";
    case D3DFMT_DXT5:                // = MAKEFOURCC('D', 'X', 'T', '5'),
		return "DXT5";
    case D3DFMT_D16_LOCKABLE:        // = 70,
		return "D16_LOCKABLE";
    case D3DFMT_D32:                 // = 71,
		return "D32";
    case D3DFMT_D15S1:               // = 73,
		return "D15S1";
    case D3DFMT_D24S8:               // = 75,
		return "D24S8";
    case D3DFMT_D24X8:               // = 77,
		return "D24X8";
    case D3DFMT_D24X4S4:             // = 79,
		return "D24X4S4";
    case D3DFMT_D16:                 // = 80,
		return "D16";
    case D3DFMT_D32F_LOCKABLE:       // = 82,
		return "D32F_LOCKABLE";
    case D3DFMT_D24FS8:              // = 83,
		return "D24FS8";
	case D3DFMT_D32_LOCKABLE:        // = 84,
		return "D32_LOCKABLE";
    case D3DFMT_S8_LOCKABLE:         // = 85,
		return "S8_LOCKABLE";
	case D3DFMT_L16:                 // = 81,
		return "L16";
    case D3DFMT_VERTEXDATA:          // =100,
		return "VERTEXDATA";
    case D3DFMT_INDEX16:             // =101,
		return "INDEX16";
    case D3DFMT_INDEX32:             // =102,
		return "INDEX32";
    case D3DFMT_Q16W16V16U16:        // =110,
		return "Q16W16V16U16";
    case D3DFMT_MULTI2_ARGB8:        // = MAKEFOURCC('M','E','T','1'),
		return "MULTI2_ARGB8";
    case D3DFMT_R16F:                // = 111,
		return "R16F";
    case D3DFMT_G16R16F:             // = 112,
		return "G16R16F";
    case D3DFMT_A16B16G16R16F:       // = 113,
		return "A16B16G16R16F";
    case D3DFMT_R32F:                // = 114,
		return "R32F";
    case D3DFMT_G32R32F:             // = 115,
		return "G32R32F";
    case D3DFMT_A32B32G32R32F:       // = 116,
		return "A32B32G32R32F";
    case D3DFMT_CxV8U8:              // = 117,
		return "CxV8U8";
	case D3DFMT_A1:                  // = 118,
		return "A1";
    case D3DFMT_A2B10G10R10_XR_BIAS: // = 119,
		return "A2B10G10R10_XR";
    case D3DFMT_BINARYBUFFER:        // = 199,
		return "BINARYBUFFER";
	default:
		return "D3DFMT_???";
	}
}

static const bool UpdateOverlay_DeviceState(const IDirect3DDevice9Hook* const hookDev, deviceStateScreenState& outCurrentScreenState)
{
	deviceStateScreenState currentScreenState;
	currentScreenState.genericData = GetOverlayPerScreenData(hookDev);

	if (GetAsyncKeyState(VK_HOME) & 0x1)
	{
		SetOverlayPerScreenData(hookDev, NULL);
		SetOverlayScreenState(hookDev, overlay_welcomeScreen);
		return false;
	}

	if (GetAsyncKeyState(VK_PRIOR) & 0x1)
	{
		if (currentScreenState.deviceStateData.currentPage == 0)
			currentScreenState.deviceStateData.currentPage = (const deviceStatePage)(MAX_NUM_STATE_PAGES - 1);
		else
			currentScreenState.deviceStateData.currentPage = (const deviceStatePage)(currentScreenState.deviceStateData.currentPage - 1u);
		currentScreenState.deviceStateData.currentLine = 0;
	}
	else if (GetAsyncKeyState(VK_NEXT) & 0x1)
	{
		currentScreenState.deviceStateData.currentPage = (const deviceStatePage)(currentScreenState.deviceStateData.currentPage + 1u);
		if (currentScreenState.deviceStateData.currentPage == MAX_NUM_STATE_PAGES)
			currentScreenState.deviceStateData.currentPage = (const deviceStatePage)0u;
		currentScreenState.deviceStateData.currentLine = 0;
	}

	if (GetAsyncKeyState(VK_DOWN) )
	{
		if (currentScreenState.deviceStateData.currentLine < 0xFFFF)
			++currentScreenState.deviceStateData.currentLine;
	}
	if (GetAsyncKeyState(VK_UP) )
	{
		if (currentScreenState.deviceStateData.currentLine > 0)
			--currentScreenState.deviceStateData.currentLine;
	}

	SetOverlayPerScreenData(hookDev, currentScreenState.genericData);

	outCurrentScreenState = currentScreenState;

	return true;
}

void UpdateAndDrawOverlay_DeviceState(class IDirect3DDevice9Hook* const hookDev)
{
	deviceStateScreenState currentScreenState;
	if (!UpdateOverlay_DeviceState(hookDev, currentScreenState) )
		return;

	const deviceStatePage currentDeviceStatePage = currentScreenState.deviceStateData.currentPage;

	OverlaySetDeviceStateForText(hookDev);


	// Skip drawing this header if we've scrolled down in order to save lines
	if (currentScreenState.deviceStateData.currentLine == 0)
		OverlayDrawPrintString(hookDev, 0, 0, stateEnabledColor, "Device State Page: [%s]", deviceStatePageNames[currentDeviceStatePage]);

	const IDirect3DDevice9Hook* const constDevice = hookDev;
	const DeviceState& deviceState = constDevice->GetCurrentHookState();

	(*(drawDeviceStateFunctions[currentDeviceStatePage]) )(hookDev, deviceState, currentScreenState.deviceStateData.currentLine);
}

static inline const char* const GetMultisampleTypeToString(const D3DMULTISAMPLE_TYPE multiSampleType)
{
	switch (multiSampleType)
	{
	case D3DMULTISAMPLE_NONE:
		return "None";
	case D3DMULTISAMPLE_NONMASKABLE:
		return "Nonmaskable";
	case D3DMULTISAMPLE_2_SAMPLES:
		return "2Samples";
	case D3DMULTISAMPLE_3_SAMPLES:
		return "3Samples";
	case D3DMULTISAMPLE_4_SAMPLES:
		return "4Samples";
	case D3DMULTISAMPLE_5_SAMPLES:
		return "5Samples";
	case D3DMULTISAMPLE_6_SAMPLES:
		return "6Samples";
	case D3DMULTISAMPLE_7_SAMPLES:
		return "7Samples";
	case D3DMULTISAMPLE_8_SAMPLES:
		return "8Samples";
	case D3DMULTISAMPLE_9_SAMPLES:
		return "9Samples";
	case D3DMULTISAMPLE_10_SAMPLES:
		return "10Samples";
	case D3DMULTISAMPLE_11_SAMPLES:
		return "11Samples";
	case D3DMULTISAMPLE_12_SAMPLES:
		return "12Samples";
	case D3DMULTISAMPLE_13_SAMPLES:
		return "13Samples";
	case D3DMULTISAMPLE_14_SAMPLES:
		return "14Samples";
	case D3DMULTISAMPLE_15_SAMPLES:
		return "15Samples";
	case D3DMULTISAMPLE_16_SAMPLES:
		return "16Samples";
	default:
		return "Unknown";
	}
}

static inline const char* const GetSwapEffectString(const D3DSWAPEFFECT swapEffect)
{
	switch (swapEffect)
	{
	case D3DSWAPEFFECT_DISCARD:
		return "Discard";
	case D3DSWAPEFFECT_FLIP:
		return "Flip";
	case D3DSWAPEFFECT_COPY:
		return "Copy";
	case D3DSWAPEFFECT_OVERLAY:
		return "Overlay";
	case D3DSWAPEFFECT_FLIPEX:
		return "FlipEx";
	default:
		return "Unknown";
	}
}

static inline const char* const GetPresentationIntervalString(const UINT PresentInterval)
{
	switch (PresentInterval)
	{
	case D3DPRESENT_INTERVAL_DEFAULT:
		return "DEFAULT";
	case D3DPRESENT_INTERVAL_ONE:
		return "ONE";
	case D3DPRESENT_INTERVAL_TWO:
		return "TWO";
	case D3DPRESENT_INTERVAL_THREE:
		return "THREE";
	case D3DPRESENT_INTERVAL_FOUR:
		return "FOUR";
	case D3DPRESENT_INTERVAL_IMMEDIATE:
		return "IMMEDIATE";
	default:
		return "Unknown";
	}
}

void DrawDeviceGeneral(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	const D3DPRESENT_PARAMETERS& presentParams = hookDev->GetInternalPresentParams();

	OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, stateEnabledColor, "Backbuffer W: %u H: %u FMT: %s Count: %u", presentParams.BackBufferWidth, 
		presentParams.BackBufferHeight, 
		GetAbbreviatedFormatStringFromFormat(presentParams.BackBufferFormat), 
		presentParams.BackBufferCount);
	OverlayDrawPrintString(hookDev, 0, 2 - currentLineOffset, stateEnabledColor, "Backbuffer Multisample Type: %s Quality: %u", GetMultisampleTypeToString(presentParams.MultiSampleType), presentParams.MultiSampleQuality);

	OverlayDrawPrintString(hookDev, 0, 3 - currentLineOffset, stateEnabledColor, "SwapEffect: %s Windowed: %s", GetSwapEffectString(presentParams.SwapEffect), presentParams.Windowed ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 4 - currentLineOffset, stateEnabledColor, "EnableAutoDepthStencil: %s", 
		presentParams.EnableAutoDepthStencil ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 5 - currentLineOffset, stateEnabledColor, "AutoDepthStencilFormat: %s", 
		GetAbbreviatedFormatStringFromFormat(presentParams.AutoDepthStencilFormat) );

	char refreshRateBuffer[16] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
	if (presentParams.FullScreen_RefreshRateInHz > 0)
		itoa(presentParams.FullScreen_RefreshRateInHz, refreshRateBuffer, 10);
	else
		strcpy(refreshRateBuffer, "Auto (0)");
#pragma warning(pop)
	OverlayDrawPrintString(hookDev, 0, 6 - currentLineOffset, stateEnabledColor, "Refresh Rate: %s Flags: 0x%08X", 
		refreshRateBuffer,
		presentParams.Flags);

	OverlayDrawPrintString(hookDev, 0, 7 - currentLineOffset, stateEnabledColor, "Presentation Interval: %s", 
		GetPresentationIntervalString(presentParams.PresentationInterval) );
}

void DrawLightState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	const unsigned definedLights = deviceState.lightInfoMap->size();
	OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, stateEnabledColor, "%u Lights Defined", definedLights);

	unsigned yOffsetLines = 2u;

	if (deviceState.lightInfoMap)
	{
		for (std::map<UINT, LightInfo*>::const_iterator iter = deviceState.lightInfoMap->begin(); iter != deviceState.lightInfoMap->end(); ++iter)
		{
			const LightInfo* const lightInfo = iter->second;
			if (lightInfo == NULL)
				continue;

			const char* lightTypeString;
			switch (lightInfo->light.Type)
			{
			default:
				lightTypeString = "UNK";
				break;
			case D3DLIGHT_POINT:
				lightTypeString = "PNT";
				break;
			case D3DLIGHT_DIRECTIONAL:
				lightTypeString = "DIR";
				break;
			case D3DLIGHT_SPOT:
				lightTypeString = "SPT";
				break;
			}

			const D3DCOLOR diffuse = Float4ToD3DCOLOR(*(const D3DXVECTOR4* const)&(lightInfo->light.Diffuse) );
			const D3DCOLOR ambient = Float4ToD3DCOLOR(*(const D3DXVECTOR4* const)&(lightInfo->light.Ambient) );
			const D3DCOLOR specular = Float4ToD3DCOLOR(*(const D3DXVECTOR4* const)&(lightInfo->light.Specular) );

			OverlayDrawPrintString(hookDev, 0, yOffsetLines - currentLineOffset, stateEnabledColor, "[%u] (%i) %s D:%08X A:%08X S:%08X", iter->first, lightInfo->activeLightIndex, lightTypeString, diffuse, ambient, specular);
			++yOffsetLines;
		}
	}
}

void DrawViewportScissorBackbufferState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, stateEnabledColor, "Viewport Rect --- X:%u, Y:%u, W:%u, H:%u", 
		deviceState.cachedViewport.viewport.X, 
		deviceState.cachedViewport.viewport.Y, 
		deviceState.cachedViewport.viewport.Width, 
		deviceState.cachedViewport.viewport.Height);
	OverlayDrawPrintString(hookDev, 0, 2 - currentLineOffset, stateEnabledColor, "zMin:%f, zMax:%f", 
		deviceState.cachedViewport.viewport.MinZ, 
		deviceState.cachedViewport.viewport.MaxZ);
	const BOOL scissorTestEnable = deviceState.currentRenderStates.renderStatesUnion.namedStates.scissorTestEnable;
	OverlayDrawPrintString(hookDev, 0, 3 - currentLineOffset, stateEnabledColor, "D3DRS_SCISSORTESTENABLE : %s", 
		scissorTestEnable ? "TRUE" : "FALSE");
	OverlayDrawPrintString(hookDev, 0, 4 - currentLineOffset, scissorTestEnable ? stateEnabledColor : stateDisabledColor, "Scissor Rect --- L:%i, R:%i, T:%i, B:%i", 
		deviceState.currentScissorRect.scissorRect.left,
		deviceState.currentScissorRect.scissorRect.right,
		deviceState.currentScissorRect.scissorRect.top,
		deviceState.currentScissorRect.scissorRect.bottom);

	if (deviceState.currentDepthStencil)
		OverlayDrawPrintString(hookDev, 0, 5 - currentLineOffset, stateEnabledColor, "DepthStencil Width: %u, Height: %u", deviceState.currentDepthStencil->GetInternalWidth(), deviceState.currentDepthStencil->GetInternalHeight() );
	else
		OverlayDrawString(hookDev, "DepthStencil is NULL", 0, 5 - currentLineOffset, stateEnabledColor);

	for (unsigned char rtID = 0; rtID < ARRAYSIZE(deviceState.currentRenderTargets); ++rtID)
	{
		const IDirect3DSurface9Hook* const thisRT = deviceState.currentRenderTargets[rtID];
		if (thisRT)
			OverlayDrawPrintString(hookDev, 0, (6 + rtID) - currentLineOffset, stateEnabledColor, "RT%u Width: %u, Height: %u", rtID, thisRT->GetInternalWidth(), thisRT->GetInternalHeight() );
		else
			OverlayDrawPrintString(hookDev, 0, (6 + rtID) - currentLineOffset, stateEnabledColor, "RT%u is NULL", rtID);
	}
}

static const char* const D3DFOGMODEStrings[] =
{
	"D3DFOG_NONE", // 0
	"D3DFOG_EXP", // 1
	"D3DFOG_EXP2", // 2
	"D3DFOG_LINEAR" // 3
};

static inline const char* const GetFogModeString(const D3DFOGMODE fogMode)
{
	if (fogMode > D3DFOG_LINEAR)
		return "UNKNOWN";
	else
		return D3DFOGMODEStrings[fogMode];
}

void DrawFogState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	const RenderStates::_renderStatesUnion::_namedStates& namedRenderStates = deviceState.currentRenderStates.renderStatesUnion.namedStates;
	const BOOL fogEnable = namedRenderStates.fogEnable;
	OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, stateEnabledColor, "D3DRS_FOGENABLE : %s", 
		fogEnable ? "TRUE" : "FALSE");
	OverlayDrawPrintString(hookDev, 0, 2 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "FogColor : 0x%08X", 
		namedRenderStates.fogColor);
	OverlayDrawString(hookDev, "FogColor Colored Text", 0, 3 - currentLineOffset, namedRenderStates.fogColor | D3DCOLOR_ARGB(0xFF, 0, 0, 0) );

	OverlayDrawPrintString(hookDev, 0, 4 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "D3DRS_FOGTABLEMODE : %s (%u)",
		GetFogModeString(namedRenderStates.fogTableMode),
		namedRenderStates.fogTableMode);

	OverlayDrawPrintString(hookDev, 0, 5 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "D3DRS_FOGVERTEXMODE : %s (%u)",
		GetFogModeString(namedRenderStates.fogVertexMode),
		namedRenderStates.fogVertexMode);

	OverlayDrawPrintString(hookDev, 0, 6 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "D3DRS_RANGEFOGENABLE : %s",
		namedRenderStates.rangeFogEnable ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 7 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "D3DRS_FOGSTART : %f",
		namedRenderStates.fogStart);

	OverlayDrawPrintString(hookDev, 0, 8 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "D3DRS_FOGEND : %f",
		namedRenderStates.fogEnd);

	OverlayDrawPrintString(hookDev, 0, 9 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "D3DRS_FOGDENSITY : %f",
		namedRenderStates.fogDensity);

	if (namedRenderStates.fogTableMode > D3DFOG_NONE && namedRenderStates.fogVertexMode > D3DFOG_NONE)
	{
		OverlayDrawPrintString(hookDev, 0, 10 - currentLineOffset, fogEnable ? stateEnabledWarningColor : stateDisabledWarningColor, "Warning: Both Table (Pixel) Fog and Vertex Fog are enabled.\nTable (Pixel) Fog overrides Vertex Fog settings.\nSelected fog mode: %s (%u)",
			GetFogModeString(namedRenderStates.fogTableMode),
			namedRenderStates.fogTableMode);
	}
	else if (namedRenderStates.fogTableMode == D3DFOG_NONE && namedRenderStates.fogVertexMode == D3DFOG_NONE)
	{
		OverlayDrawPrintString(hookDev, 0, 10 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "No Fog Selected. Fog disabled.\nSelected fog mode: %s (%u)",
			GetFogModeString(namedRenderStates.fogTableMode),
			namedRenderStates.fogTableMode);
	}
	else if (namedRenderStates.fogTableMode > D3DFOG_NONE)
	{
		OverlayDrawPrintString(hookDev, 0, 10 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "Table Fog (Pixel Fog) Selected.\nSelected fog mode: %s (%u)",
			GetFogModeString(namedRenderStates.fogTableMode),
			namedRenderStates.fogTableMode);
	}
	else
	{
		OverlayDrawPrintString(hookDev, 0, 10 - currentLineOffset, fogEnable ? stateEnabledColor : stateDisabledColor, "Vertex Fog Selected.\nSelected fog mode: %s (%u)",
			GetFogModeString(namedRenderStates.fogVertexMode),
			namedRenderStates.fogVertexMode);
	}
}

static inline const char* const GetDeclTypeString(const D3DDECLTYPE type)
{
	switch (type)
	{
	case D3DDECLTYPE_FLOAT1:
		return "float1";
	case D3DDECLTYPE_FLOAT2:
		return "float2";
	case D3DDECLTYPE_FLOAT3:
		return "float3";
	case D3DDECLTYPE_FLOAT4:
		return "float4";
	case D3DDECLTYPE_D3DCOLOR:
		return "d3dcolor";
	case D3DDECLTYPE_UBYTE4:
		return "ubyte4";
	case D3DDECLTYPE_SHORT2:
		return "short2";
	case D3DDECLTYPE_SHORT4:
		return "short4";
	case D3DDECLTYPE_UBYTE4N:
		return "ubyte4n";
	case D3DDECLTYPE_SHORT2N:
		return "short2n";
	case D3DDECLTYPE_SHORT4N:
		return "short4n";
	case D3DDECLTYPE_USHORT2N:
		return "ushort2n";
	case D3DDECLTYPE_USHORT4N:
		return "ushort4n";
	case D3DDECLTYPE_UDEC3:
		return "udec3";
	case D3DDECLTYPE_DEC3N:
		return "dec3n";
	case D3DDECLTYPE_FLOAT16_2:
		return "float16_2";
	case D3DDECLTYPE_FLOAT16_4:
		return "float16_4";
	case D3DDECLTYPE_UNUSED:
		return "unused";
	default:
		return "Unknown";
	}
}

static inline const char* const GetDeclUsageString(const D3DDECLUSAGE usage)
{
	switch (usage)
	{
	case D3DDECLUSAGE_POSITION:      // = 0,
		return "POSITION";
	case D3DDECLUSAGE_BLENDWEIGHT:   // 1
		return "BLENDWEIGHT";
	case D3DDECLUSAGE_BLENDINDICES:  // 2
		return "BLENDINDICES";
	case D3DDECLUSAGE_NORMAL:        // 3
		return "NORMAL";
	case D3DDECLUSAGE_PSIZE:         // 4
		return "PSIZE";
	case D3DDECLUSAGE_TEXCOORD:      // 5
		return "TEXCOORD";
	case D3DDECLUSAGE_TANGENT:       // 6
		return "TANGENT";
	case D3DDECLUSAGE_BINORMAL:      // 7
		return "BINORMAL";
	case D3DDECLUSAGE_TESSFACTOR:    // 8
		return "TESSFACTOR";
	case D3DDECLUSAGE_POSITIONT:     // 9
		return "POSITIONT";
	case D3DDECLUSAGE_COLOR:         // 10
		return "COLOR";
	case D3DDECLUSAGE_FOG:           // 11
		return "FOG";
	case D3DDECLUSAGE_DEPTH:         // 12
		return "DEPTH";
	case D3DDECLUSAGE_SAMPLE:        // 13
		return "SAMPLE";
	default:
		return "Unknown";
	}
}

static inline const char* const GetDeclMethodString(const D3DDECLMETHOD method)
{
	switch (method)
	{
	case D3DDECLMETHOD_DEFAULT:// = 0,
		return "DEFAULT";
	case D3DDECLMETHOD_PARTIALU:// 1
		return "PARTIALU";
	case D3DDECLMETHOD_PARTIALV:// 2
		return "PARTIALV";
	case D3DDECLMETHOD_CROSSUV:// 3
		return "CROSSUV";
	case D3DDECLMETHOD_UV:// 4
		return "UV";
	case D3DDECLMETHOD_LOOKUP:// 5 
		return "LOOKUP";
	case D3DDECLMETHOD_LOOKUPPRESAMPLED:// 6
		return "LOOKUPPRESAMPLED";
	default:
		return "Unknown";
	}
}

static const D3DVERTEXELEMENT9 EndDecl = D3DDECL_END();

void DrawInputAssemblerState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	OverlayDrawString(hookDev, deviceState.declTarget == DeviceState::targetFVF ? "Currently using FVF vertex declaration" : "Currently using custom vertex declaration", 0, 1 - currentLineOffset, stateEnabledColor);
	OverlayDrawPrintString(hookDev, 0, 2 - currentLineOffset, (deviceState.declTarget == DeviceState::targetFVF) ? stateEnabledColor : stateDisabledColor, "Set FVF: 0x%08X", deviceState.currentFVF.rawFVF_DWORD);
	OverlayDrawPrintString(hookDev, 0, 3 - currentLineOffset, (deviceState.declTarget == DeviceState::targetVertexDecl) ? stateEnabledColor : stateDisabledColor, "Set Decl: 0x%08X", deviceState.currentVertexDecl);

	unsigned short lineOffset = 4;
	if (deviceState.currentVertexDecl)
	{
		const D3DCOLOR vertexDeclEnabledColor = (deviceState.declTarget == DeviceState::targetVertexDecl) ? stateEnabledColor : stateDisabledColor;
		OverlayDrawPrintString(hookDev, 0, (lineOffset++) - currentLineOffset, vertexDeclEnabledColor, "Decl vertex size: %u bytes", deviceState.currentVertexDecl->GetVertexSize() );

		const std::vector<DebuggableD3DVERTEXELEMENT9>& declElements = deviceState.currentVertexDecl->GetElementsInternal();
		const unsigned numElements = declElements.size();
		for (unsigned x = 0; x < numElements; ++x)
		{
			const DebuggableD3DVERTEXELEMENT9& thisElement = declElements[x];
			if (memcmp(&EndDecl, &thisElement, sizeof(D3DVERTEXELEMENT9) ) == 0)
				OverlayDrawString(hookDev, "{ D3DDECL_END }", 0, (lineOffset++) - currentLineOffset, vertexDeclEnabledColor);
			else
			{
				OverlayDrawPrintString(hookDev, 0, (lineOffset++) - currentLineOffset, vertexDeclEnabledColor, "{%u, %u, %s, %s, %s, %u}", 
					thisElement.Stream,
					thisElement.Offset,
					GetDeclTypeString(thisElement.Type),
					GetDeclMethodString(thisElement.Method),
					GetDeclUsageString(thisElement.Usage),
					thisElement.UsageIndex);
			}
		}
	}

	if (!deviceState.currentIndexBuffer)
		OverlayDrawString(hookDev, "Index buffer: NULL", 0, (lineOffset++) - currentLineOffset, stateEnabledColor);
	else
	{
		OverlayDrawPrintString(hookDev, 0, (lineOffset++) - currentLineOffset, stateEnabledColor, "Index buffer: 0x%08X Fmt: %s Bytes: %u", 
			deviceState.currentIndexBuffer, 
			GetAbbreviatedFormatStringFromFormat(deviceState.currentIndexBuffer->GetFormat() ),
			deviceState.currentIndexBuffer->GetInternalLength() );
	}

	for (unsigned streamID = 0; streamID < ARRAYSIZE(deviceState.currentStreams); ++streamID)
	{
		const StreamSource& thisStream = deviceState.currentStreams[streamID];
		if (!thisStream.vertexBuffer)
			OverlayDrawPrintString(hookDev, 0, (lineOffset++) - currentLineOffset, stateEnabledColor, "Stream %u: NULL Offset: %u bytes Stride: %u bytes", streamID, thisStream.streamOffset, thisStream.streamStride);
		else
		{
			OverlayDrawPrintString(hookDev, 0, (lineOffset++) - currentLineOffset, stateEnabledColor, "Stream %u: 0x%08X Offset: %u bytes Stride: %u bytes", streamID, thisStream.vertexBuffer, thisStream.streamOffset, thisStream.streamStride);
			const UINT vertexBufferLengthBytes = thisStream.vertexBuffer->GetInternalLength_Bytes();
			const UINT verticesAtCurrentStride = thisStream.streamStride ? (vertexBufferLengthBytes - thisStream.streamOffset) / thisStream.streamStride : 1;
			OverlayDrawPrintString(hookDev, 0, (lineOffset++) - currentLineOffset, stateEnabledColor, "VB 0x%08X Length: %u bytes FVF: 0x%08X %u vertices", thisStream.vertexBuffer, vertexBufferLengthBytes, thisStream.vertexBuffer->GetInternalFVF(), verticesAtCurrentStride);
		}
		
		OverlayDrawPrintString(hookDev, 0, (lineOffset++) - currentLineOffset, stateEnabledColor, "Freq: %u%s%s", thisStream.streamDividerFrequency, 
			(thisStream.streamDividerFrequency & D3DSTREAMSOURCE_INDEXEDDATA) ? " | INDEXEDDATA" : "",
			(thisStream.streamDividerFrequency & D3DSTREAMSOURCE_INSTANCEDATA) ? " | INSTANCEDATA" : "");
	}
}

static inline const char* const GetPatchEdgeStyleString(const D3DPATCHEDGESTYLE patchEdgeStyle)
{
	switch (patchEdgeStyle)
	{
	case D3DPATCHEDGE_DISCRETE:
		return "D3DPATCHEDGE_DISCRETE";
	case D3DPATCHEDGE_CONTINUOUS:
		return "D3DPATCHEDGE_CONTINUOUS";
	default:
		return "Unknown PatchEdgeStyle";
	}
}

static inline const char* const GetDegreeTypeString(const D3DDEGREETYPE degreeType)
{
	switch (degreeType)
	{
	case D3DDEGREE_LINEAR:
		return "D3DDEGREE_LINEAR";
	case D3DDEGREE_QUADRATIC:
		return "D3DDEGREE_QUADRATIC";
	case D3DDEGREE_CUBIC:
		return "D3DDEGREE_CUBIC";
	case D3DDEGREE_QUINTIC:
		return "D3DDEGREE_QUINTIC";
	default:
		return "Unknown degree type";
	}
}

void DrawTessellationState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	const RenderStates::_renderStatesUnion::_namedStates& namedRenderStates = deviceState.currentRenderStates.renderStatesUnion.namedStates;

	OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, stateEnabledColor, "Current N-Patch Mode (segments): %f", deviceState.currentNPatchMode);
	const bool nPatchesEnabled = deviceState.currentNPatchMode >= 1.0f;
	const D3DCOLOR nPatchEnabledColor = nPatchesEnabled ? stateEnabledColor : stateDisabledColor;
	OverlayDrawString(hookDev, nPatchesEnabled ? "N-Patches Enabled" : "N-Patches Disabled", 0, 2 - currentLineOffset, nPatchEnabledColor);
	OverlayDrawPrintString(hookDev, 0, 3 - currentLineOffset, nPatchEnabledColor, "Patch Edge Style: %s", GetPatchEdgeStyleString(namedRenderStates.patchEdgeStyle) );
	OverlayDrawPrintString(hookDev, 0, 4 - currentLineOffset, nPatchEnabledColor, "Positons Degree: %s", GetDegreeTypeString(namedRenderStates.positionDegree) );
	OverlayDrawPrintString(hookDev, 0, 5 - currentLineOffset, nPatchEnabledColor, "Normals Degree: %s", GetDegreeTypeString(namedRenderStates.normalDegree) );

	OverlayDrawString(hookDev, namedRenderStates.enableAdaptiveTessellation ? "Adaptive Tessellation Enabled" : "Adaptive Tessellation Disabled", 0, 6 - currentLineOffset, stateEnabledColor);
	const D3DCOLOR adaptiveTessEnabledColor = namedRenderStates.enableAdaptiveTessellation ? stateEnabledColor : stateDisabledColor;
	OverlayDrawPrintString(hookDev, 0, 7 - currentLineOffset, adaptiveTessEnabledColor, "Adaptive Tessellation Factors:\n    (%f, %f, %f, %f)", namedRenderStates.adaptiveness_X, namedRenderStates.adaptiveness_Y, namedRenderStates.adaptiveness_Z, namedRenderStates.adaptiveness_W);
	OverlayDrawPrintString(hookDev, 0, 9 - currentLineOffset, adaptiveTessEnabledColor, "Adaptive Tessellation Min (%f) Max (%f)", namedRenderStates.minTessellationLevel, namedRenderStates.maxTessellationLevel);
}

static inline const char* const GetZBufferModeToString(const D3DZBUFFERTYPE zBufferMode)
{
	switch (zBufferMode)
	{
	case D3DZB_FALSE:
		return "FALSE";
	case D3DZB_TRUE:
		return "ZB_TRUE";
	case D3DZB_USEW:
		return "WB_TRUE";
	default:
		return "Unknown";
	}
}

static inline const char* const GetCmpFuncString(const D3DCMPFUNC cmpFunc)
{
	switch (cmpFunc)
	{
	case D3DCMP_NEVER:
		return "NEVER";
	case D3DCMP_LESS:
		return "LESS";
	case D3DCMP_EQUAL:
		return "EQUAL";
	case D3DCMP_LESSEQUAL:
		return "LESSEQUAL";
	case D3DCMP_GREATER:
		return "GREATER";
	case D3DCMP_NOTEQUAL:
		return "NOTEQUAL";
	case D3DCMP_GREATEREQUAL:
		return "GREATEREQUAL";
	case D3DCMP_ALWAYS:
		return "ALWAYS";
	default:
		return "Unknown";
	}
}

static inline const char* const GetStencilOpString(const D3DSTENCILOP stencilOp)
{
	switch (stencilOp)
	{
	case D3DSTENCILOP_KEEP:
		return "KEEP";
	case D3DSTENCILOP_ZERO:
		return "ZERO";
	case D3DSTENCILOP_REPLACE:
		return "REPLACE";
	case D3DSTENCILOP_INCRSAT:
		return "INCSAT";
	case D3DSTENCILOP_DECRSAT:
		return "DECSAT";
	case D3DSTENCILOP_INVERT:
		return "INVERT";
	case D3DSTENCILOP_INCR:
		return "INC";
	case D3DSTENCILOP_DECR:
		return "DEC";
	default:
		return "Unknown";
	}
}

void DrawDepthStencilState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	const RenderStates::_renderStatesUnion::_namedStates& namedRenderStates = deviceState.currentRenderStates.renderStatesUnion.namedStates;

	const bool depthIsEnabled = (deviceState.currentDepthStencil != NULL) && namedRenderStates.zEnable;
	const D3DCOLOR depthEnabledColor = depthIsEnabled ? stateEnabledColor : stateDisabledColor;

	if (deviceState.currentDepthStencil != NULL)
	{
		OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, depthEnabledColor, "DepthStencil W: %u H: %u Fmt: %s", 
			deviceState.currentDepthStencil->GetInternalWidth(), 
			deviceState.currentDepthStencil->GetInternalHeight(), 
			GetAbbreviatedFormatStringFromFormat(deviceState.currentDepthStencil->GetInternalFormat() ) );
	}
	else
		OverlayDrawString(hookDev, "DepthStencil: NULL", 0, 1 - currentLineOffset, depthEnabledColor);

	OverlayDrawPrintString(hookDev, 0, 2 - currentLineOffset, depthEnabledColor, "zEnable: %s zWriteEnable: %s zFunc: %s",
		GetZBufferModeToString(namedRenderStates.zEnable),
		namedRenderStates.zWriteEnable ? "TRUE" : "FALSE",
		GetCmpFuncString(namedRenderStates.zFunc) );

	OverlayDrawPrintString(hookDev, 0, 3 - currentLineOffset, depthEnabledColor, "DepthBias: %f SlopeScaledDepthBias: %f",
		namedRenderStates.depthBias,
		namedRenderStates.slopeScaledDepthBias);

	const D3DCOLOR stencilEnabledColor = namedRenderStates.stencilEnable ? stateEnabledColor : stateDisabledColor;

	OverlayDrawPrintString(hookDev, 0, 4 - currentLineOffset, stateEnabledColor, "Stencil Enable: %s",
		namedRenderStates.stencilEnable ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 5 - currentLineOffset, stencilEnabledColor, "CW Stencil Func: %s",
		GetCmpFuncString(namedRenderStates.stencilFunc) );

	OverlayDrawPrintString(hookDev, 0, 6 - currentLineOffset, stencilEnabledColor, "CWPass: %s CWFail: %s CWzFail: %s",
		GetStencilOpString(namedRenderStates.stencilPass),
		GetStencilOpString(namedRenderStates.stencilFail),
		GetStencilOpString(namedRenderStates.stencilZFail) );

	OverlayDrawPrintString(hookDev, 0, 7 - currentLineOffset, stencilEnabledColor, "Two-Sided Stencil: %s",
		namedRenderStates.twoSidedStencilMode ? "TRUE" : "FALSE");

	const D3DCOLOR twoSidedStencilEnabledColor = (namedRenderStates.stencilEnable && namedRenderStates.twoSidedStencilMode) ? stateEnabledColor : stateDisabledColor;

	OverlayDrawPrintString(hookDev, 0, 8 - currentLineOffset, twoSidedStencilEnabledColor, "CCW Stencil Func: %s",
		GetCmpFuncString(namedRenderStates.ccw_StencilFunc) );

	OverlayDrawPrintString(hookDev, 0, 9 - currentLineOffset, twoSidedStencilEnabledColor, "CCWPass: %s CCWFail: %s CCWzFail: %s",
		GetStencilOpString(namedRenderStates.ccw_StencilPass),
		GetStencilOpString(namedRenderStates.ccw_StencilFail),
		GetStencilOpString(namedRenderStates.ccw_StencilZFail) );

	OverlayDrawPrintString(hookDev, 0, 10 - currentLineOffset, stencilEnabledColor, "Stencil Ref: %u\nStencil Read Mask: 0x%08X\nStencil Write Mask: 0x%08X",
		namedRenderStates.stencilRef,
		namedRenderStates.stencilMask,
		namedRenderStates.stencilWriteMask);
}

static inline const char* const GetBlendOpString(const D3DBLENDOP blendOp)
{
	switch (blendOp)
	{
	case D3DBLENDOP_ADD:
		return "ADD";
	case D3DBLENDOP_SUBTRACT:
		return "SUBTRACT";
	case D3DBLENDOP_REVSUBTRACT:
		return "REVSUBTRACT";
	case D3DBLENDOP_MIN:
		return "MIN";
	case D3DBLENDOP_MAX:
		return "MAX";
	default:
		return "Unknown";
	}
}

static inline const char* const GetBlendString(const D3DBLEND blend)
{
	switch (blend)
	{
	case D3DBLEND_ZERO:
		return "ZERO";
	case D3DBLEND_ONE:
		return "ONE";
	case D3DBLEND_SRCCOLOR:
		return "SRCCOLOR";
	case D3DBLEND_INVSRCCOLOR:
		return "INVSRCCOLOR";
	case D3DBLEND_SRCALPHA:
		return "SRCALPHA";
	case D3DBLEND_INVSRCALPHA:
		return "INVSRCALPHA";
	case D3DBLEND_DESTALPHA:
		return "DESTALPHA";
	case D3DBLEND_INVDESTALPHA:
		return "INVDESTALPHA";
	case D3DBLEND_DESTCOLOR:
		return "DESTCOLOR";
	case D3DBLEND_INVDESTCOLOR:
		return "INVDESTCOLOR";
	case D3DBLEND_SRCALPHASAT:
		return "SRCALPHASAT";
	case D3DBLEND_BOTHSRCALPHA:
		return "BOTHSRCALPHA";
	case D3DBLEND_BOTHINVSRCALPHA:
		return "BOTHINVSRCALPHA";
	case D3DBLEND_BLENDFACTOR:
		return "BLENDFACTOR";
	case D3DBLEND_INVBLENDFACTOR:
		return "INVBLENDFACTOR";
	case D3DBLEND_SRCCOLOR2:
		return "SRCCOLOR2";
	case D3DBLEND_INVSRCCOLOR2:
		return "INVSRCCOLOR2";
	default:
		return "Unknown";
	}
}

static const char* const colorWriteStrings[] = 
{
	"None", // 0 - None
	"Red", // 1 - R
	"Green", // 2 - G
	"Red | Green",// 3 - RG
	"Blue", // 4 - B
	"Red | Blue", // 5 - RB
	"Green | Blue", // 6 - GB
	"Red | Green | Blue", // 7 - RGB
	"Alpha", // 8 - A
	"Red | Alpha", // 9 - RA
	"Green | Alpha", // 10 - GA
	"Red | Green | Alpha", // 11 - RGA
	"Blue | Alpha", // 12 - BA
	"Red | Blue | Alpha", // 13 - RBA
	"Green | Blue | Alpha", // 14 - GBA
	"Red | Green | Blue | Alpha" // 15 - RGBA
};

void DrawOutputState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	const RenderStates::_renderStatesUnion::_namedStates& namedRenderStates = deviceState.currentRenderStates.renderStatesUnion.namedStates;

	OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, stateEnabledColor, "Alpha Blend Enable: %s", namedRenderStates.alphaBlendEnable ? "TRUE" : "FALSE");

	const D3DCOLOR alphaBlendEnableColor = namedRenderStates.alphaBlendEnable ? stateEnabledColor : stateDisabledColor;
	const D3DCOLOR separateAlphaBlendEnableColor = namedRenderStates.separateAlphaBlendEnable ? stateEnabledColor : stateDisabledColor;

	OverlayDrawPrintString(hookDev, 0, 2 - currentLineOffset, alphaBlendEnableColor, "%s %s %s", GetBlendString(namedRenderStates.srcBlend), GetBlendOpString(namedRenderStates.blendOp), GetBlendString(namedRenderStates.destBlend) );

	OverlayDrawPrintString(hookDev, 0, 3 - currentLineOffset, stateEnabledColor, "Separate Alpha Blend Enable: %s", namedRenderStates.separateAlphaBlendEnable ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 4 - currentLineOffset, separateAlphaBlendEnableColor, "%s %s %s", GetBlendString(namedRenderStates.srcBlendAlpha), GetBlendOpString(namedRenderStates.blendOpAlpha), GetBlendString(namedRenderStates.destBlendAlpha) );

	OverlayDrawPrintString(hookDev, 0, 5 - currentLineOffset, stateEnabledColor, "BlendFactor: 0x%08X", namedRenderStates.blendFactor);

	OverlayDrawPrintString(hookDev, 0, 6 - currentLineOffset, stateEnabledColor, "Alpha Test Enable: %s", namedRenderStates.alphaTestEnable ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 7 - currentLineOffset, namedRenderStates.alphaTestEnable ? stateEnabledColor : stateDisabledColor, "Alpha Test Reference: %u\nAlpha Test Function: %s", namedRenderStates.alphaRef, GetCmpFuncString(namedRenderStates.alphaFunc) );

	OverlayDrawPrintString(hookDev, 0, 9 - currentLineOffset, stateEnabledColor, "sRGB Write Enable: %s", namedRenderStates.sRGBWriteEnable ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 10 - currentLineOffset, stateEnabledColor, "Multisample Antialias: %s", namedRenderStates.multisampleAntialias ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 11 - currentLineOffset, namedRenderStates.multisampleAntialias ? stateEnabledColor : stateDisabledColor, "Multisample Mask: 0x%08X", namedRenderStates.multisampleMask);

	OverlayDrawPrintString(hookDev, 0, 12 - currentLineOffset, stateEnabledColor, "Color Write Enable 0: %s", colorWriteStrings[namedRenderStates.colorWriteEnable % 16]);
	OverlayDrawPrintString(hookDev, 0, 13 - currentLineOffset, stateEnabledColor, "Color Write Enable 1: %s", colorWriteStrings[namedRenderStates.colorWriteEnable1 % 16]);
	OverlayDrawPrintString(hookDev, 0, 14 - currentLineOffset, stateEnabledColor, "Color Write Enable 2: %s", colorWriteStrings[namedRenderStates.colorWriteEnable2 % 16]);
	OverlayDrawPrintString(hookDev, 0, 15 - currentLineOffset, stateEnabledColor, "Color Write Enable 3: %s", colorWriteStrings[namedRenderStates.colorWriteEnable3 % 16]);

	for (unsigned char rtID = 0; rtID < ARRAYSIZE(deviceState.currentRenderTargets); ++rtID)
	{
		const unsigned short lineNumber = (16 + rtID) - currentLineOffset;
		const IDirect3DSurface9Hook* const thisRT = deviceState.currentRenderTargets[rtID];
		if (thisRT != NULL)
			OverlayDrawPrintString(hookDev, 0, lineNumber, stateEnabledColor, "RT%u is: %ux%u %s %s %s", rtID, 
				thisRT->GetInternalWidth(), 
				thisRT->GetInternalHeight(), 
				GetAbbreviatedFormatStringFromFormat(thisRT->GetInternalFormat() ),
				thisRT->GetInternalDiscard() ? "DISCARD" : "NODISCARD",
				thisRT->GetInternalLockable() ? "LOCKABLE" : "NOLOCK");
		else
			OverlayDrawPrintString(hookDev, 0, lineNumber, stateEnabledColor, "RT%u is: NULL", rtID);
	}
}

static inline const char* const GetTextureOpString(const D3DTEXTUREOP textureOp)
{
	switch (textureOp)
	{
	case D3DTOP_DISABLE://              = 1,
		return "DISABLE";
	case D3DTOP_SELECTARG1://           = 2,
		return "SELECTARG1";
	case D3DTOP_SELECTARG2://           = 3,
		return "SELECTARG2";
	case D3DTOP_MODULATE://             = 4,
		return "MODULATE";
	case D3DTOP_MODULATE2X://           = 5,
		return "MODULATE2X";
	case D3DTOP_MODULATE4X://           = 6,
		return "MODULATE4X";
	case D3DTOP_ADD://                  =  7
		return "ADD";
	case D3DTOP_ADDSIGNED://            =  8
		return "ADDSIGNED";
	case D3DTOP_ADDSIGNED2X://          =  9
		return "ADDSIGNED2X";
	case D3DTOP_SUBTRACT://             = 10
		return "SUBTRACT";
	case D3DTOP_ADDSMOOTH://            = 11
		return "ADDSMOOTH";
	case D3DTOP_BLENDDIFFUSEALPHA://    = 12
		return "BLENDDIFFUSEALPHA";
	case D3DTOP_BLENDTEXTUREALPHA://    = 13
		return "BLENDTEXTUREALPHA";
	case D3DTOP_BLENDFACTORALPHA://     = 14
		return "BLENDFACTORALPHA";
	case D3DTOP_BLENDTEXTUREALPHAPM://  = 15
		return "BLENDTEXTUREALPHAPM";
	case D3DTOP_BLENDCURRENTALPHA://    = 16
		return "BLENDCURRENTALPHA";
	case D3DTOP_PREMODULATE://            = 17
		return "PREMODULATE";
	case D3DTOP_MODULATEALPHA_ADDCOLOR:// = 18
		return "MODALPHA_ADDCOLOR";
	case D3DTOP_MODULATECOLOR_ADDALPHA:// = 19
		return "MODCOLOR_ADDALPHA";
	case D3DTOP_MODULATEINVALPHA_ADDCOLOR:// = 20
		return "MODINVALPHA_ADDCOLOR";
	case D3DTOP_MODULATEINVCOLOR_ADDALPHA:// = 21
		return "MODINVCOLOR_ADDALPHA";
	case D3DTOP_BUMPENVMAP://           = 22
		return "BUMPENVMAP";
	case D3DTOP_BUMPENVMAPLUMINANCE://  = 23
		return "BUMPENVMAPLUM";
	case D3DTOP_DOTPRODUCT3://          = 24
		return "DOTPRODUCT3";
	case D3DTOP_MULTIPLYADD://          = 25
		return "MULTIPLYADD";
	case D3DTOP_LERP://                 = 26
		return "LERP";
	default:
		return "Unknown";
	}
}

static inline const char* const GetTextureArgString(const textureStageArgument textureArg)
{
	switch (textureArg)
	{
	case TA_DIFFUSE:// = D3DTA_DIFFUSE,
		return "DIFFUSE";
	case TA_CURRENT:// = D3DTA_CURRENT,
		return "CURRENT";
	case TA_TEXTURE:// = D3DTA_TEXTURE,
		return "TEXTURE";
	case TA_TFACTOR:// = D3DTA_TFACTOR,
		return "TFACTOR";
	case TA_SPECULAR:// = D3DTA_SPECULAR,
		return "SPECULAR";
	case TA_TEMP:// = D3DTA_TEMP,
		return "TEMP";
	case TA_CONSTANT:// = D3DTA_CONSTANT,
		return "CONSTANT";

	case TA_COMPLEMENT_DIFFUSE:// = D3DTA_DIFFUSE | D3DTA_COMPLEMENT,
		return "DIFFUSE|COMP";
	case TA_COMPLEMENT_CURRENT:// = D3DTA_CURRENT | D3DTA_COMPLEMENT,
		return "CURRENT|COMP";
	case TA_COMPLEMENT_TEXTURE:// = D3DTA_TEXTURE | D3DTA_COMPLEMENT,
		return "TEXTURE|COMP";
	case TA_COMPLEMENT_TFACTOR:// = D3DTA_TFACTOR | D3DTA_COMPLEMENT,
		return "TFACTOR|COMP";
	case TA_COMPLEMENT_SPECULAR:// = D3DTA_SPECULAR | D3DTA_COMPLEMENT,
		return "SPECULAR|COMP";
	case TA_COMPLEMENT_TEMP:// = D3DTA_TEMP | D3DTA_COMPLEMENT,
		return "TEMP|COMP";
	case TA_COMPLEMENT_CONSTANT:// = D3DTA_CONSTANT | D3DTA_COMPLEMENT,
		return "CONSTANT|COMP";

	case TA_DIFFUSE_ALPHAREPLICATE:// = D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE,
		return "DIFFUSE|ALPHAREP";
	case TA_CURRENT_ALPHAREPLICATE:// = D3DTA_CURRENT | D3DTA_ALPHAREPLICATE,
		return "CURRENT|ALPHAREP";
	case TA_TEXTURE_ALPHAREPLICATE:// = D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE,
		return "TEXTURE|ALPHAREP";
	case TA_TFACTOR_ALPHAREPLICATE:// = D3DTA_TFACTOR | D3DTA_ALPHAREPLICATE,
		return "TFACTOR|ALPHAREP";
	case TA_SPECULAR_ALPHAREPLICATE:// = D3DTA_SPECULAR | D3DTA_ALPHAREPLICATE,
		return "SPECULAR|ALPHAREP";
	case TA_TEMP_ALPHAREPLICATE:// = D3DTA_TEMP | D3DTA_ALPHAREPLICATE,
		return "TEMP|ALPHAREP";
	case TA_CONSTANT_ALPHAREPLICATE:// = D3DTA_CONSTANT | D3DTA_ALPHAREPLICATE,
		return "CONSTANT|ALPHAREP";

	case TA_COMPLEMENT_DIFFUSE_ALPHAREPLICATE:// = D3DTA_DIFFUSE | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
		return "DIFFUSE|COMP|ALPHAREP";
	case TA_COMPLEMENT_CURRENT_ALPHAREPLICATE:// = D3DTA_CURRENT | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
		return "CURRENT|COMP|ALPHAREP";
	case TA_COMPLEMENT_TEXTURE_ALPHAREPLICATE:// = D3DTA_TEXTURE | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
		return "TEXTURE|COMP|ALPHAREP";
	case TA_COMPLEMENT_TFACTOR_ALPHAREPLICATE:// = D3DTA_TFACTOR | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
		return "TFACTOR|COMP|ALPHAREP";
	case TA_COMPLEMENT_SPECULAR_ALPHAREPLICATE:// = D3DTA_SPECULAR | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
		return "SPECULAR|COMP|ALPHAREP";
	case TA_COMPLEMENT_TEMP_ALPHAREPLICATE:// = D3DTA_TEMP | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE,
		return "TEMP|COMP|ALPHAREP";
	case TA_COMPLEMENT_CONSTANT_ALPHAREPLICATE:// = D3DTA_CONSTANT | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE
		return "CONSTANT|COMP|ALPHAREP";
	default:
		return "Unknown";
	}
}

static inline const char* const GetTextureTransformFlagsString(const D3DTEXTURETRANSFORMFLAGS flags)
{
	switch (flags)
	{
	case D3DTTFF_DISABLE:
		return "DISABLE";
	case D3DTTFF_COUNT1:
		return "COUNT1";
	case D3DTTFF_COUNT2:
		return "COUNT2";
	case D3DTTFF_COUNT3:
		return "COUNT3";
	case D3DTTFF_COUNT4:
		return "COUNT4";

	case D3DTTFF_COUNT1|D3DTTFF_PROJECTED:
		return "COUNT1|PROJ";
	case D3DTTFF_COUNT2|D3DTTFF_PROJECTED:
		return "COUNT2|PROJ";
	case D3DTTFF_COUNT3|D3DTTFF_PROJECTED:
		return "COUNT3|PROJ";
	case D3DTTFF_COUNT4|D3DTTFF_PROJECTED:
		return "COUNT4|PROJ";

	default:
		return "Unknown";
	}
}

static inline const char* const GetTCIString(const UINT texcoordIndex)
{
	switch (texcoordIndex & 0xFFFF0000)
	{
	case D3DTSS_TCI_PASSTHRU://                             0x00000000
		return "PASSTHRU";
	case D3DTSS_TCI_CAMERASPACENORMAL://                    0x00010000
		return "VIEWNORMAL";
	case D3DTSS_TCI_CAMERASPACEPOSITION://                  0x00020000
		return "VIEWPOS";
	case D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR://          0x00030000
		return "VIEWREFLECTION";
	case D3DTSS_TCI_SPHEREMAP://                            0x00040000
		return "SPHEREMAP";
	default:
		return "Unknown";
	}
}

void DrawTextureStageStates(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, stateEnabledColor, "TFACTOR: 0x%08X", deviceState.currentRenderStates.renderStatesUnion.namedStates.textureFactor);

	bool disableFutureStages = false;
	for (unsigned char stageNum = 0; stageNum < ARRAYSIZE(deviceState.currentStageStates); ++stageNum)
	{
		const TextureStageState& stage = deviceState.currentStageStates[stageNum];
		const TextureStageState::_stageStateUnion::_namedStates& stageStateNames = stage.stageStateUnion.namedStates;

		const unsigned short stageStartLineNumber = (stageNum * 12 + 3) - currentLineOffset;

		if (stageStateNames.colorOp == D3DTOP_DISABLE)
			disableFutureStages = true;

		const D3DCOLOR stageColor = (stageStateNames.colorOp != D3DTOP_DISABLE && !disableFutureStages) ? stateEnabledColor : stateDisabledColor;
		const D3DCOLOR stageColorAlpha = (stageStateNames.colorOp != D3DTOP_DISABLE && stageStateNames.alphaOp != D3DTOP_DISABLE && !disableFutureStages) ? stateEnabledColor : stateDisabledColor;

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber, stageColor, "Stage: %u", stageNum);

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 1, stageColor, "ColorOp: %s", GetTextureOpString(stageStateNames.colorOp) );

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 2, stageColor, "Color: Arg1: %s Arg2: %s Arg0: %s",
			GetTextureArgString(stageStateNames.colorArg1),
			GetTextureArgString(stageStateNames.colorArg2),
			GetTextureArgString(stageStateNames.colorArg0) );

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 3, stageColorAlpha, "AlphaOp: %s", GetTextureOpString(stageStateNames.alphaOp) );

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 4, stageColor, "Alpha: Arg1: %s Arg2: %s Arg0: %s",
			GetTextureArgString(stageStateNames.alphaArg1),
			GetTextureArgString(stageStateNames.alphaArg2),
			GetTextureArgString(stageStateNames.alphaArg0) );

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 5, stageColor, "Result: %s StageConstant: 0x%08X", GetTextureArgString(stageStateNames.resultArg), stageStateNames.constant);

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 6, stageColor, "TexCoordIndex: %u TexGen: %s", 
			LOWORD(stageStateNames.texCoordIndex), 
			GetTCIString(stageStateNames.texCoordIndex) );

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 7, stageColor, "TexTransform: %s", 
			GetTextureTransformFlagsString(stageStateNames.textureTransformFlags) );

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 8, stageColor, "BumpEnvMat: [%f, %f]\n"
			"            [%f, %f]",
			stageStateNames.bumpEnvMat00, stageStateNames.bumpEnvMat01,
			stageStateNames.bumpEnvMat10, stageStateNames.bumpEnvMat11);

		OverlayDrawPrintString(hookDev, 0, stageStartLineNumber + 10, stageColor, "BumpEnvLScale: %f BumpEnvLOffset: %f", stageStateNames.bumpEnvLScale, stageStateNames.bumpEnvLOffset);
	}
}

static const char* const abbreviatedAddressModes[] =
{
	"", // Zero entry
	"WRAP",
	"MIRROR",
	"CLAMP",
	"BORDER",
	"MIRROR1"
};
static inline const char* const GetAbbreviatedAddressModeString(const D3DTEXTUREADDRESS textureAddressMode)
{
	if (textureAddressMode < D3DTADDRESS_WRAP || D3DTADDRESS_MIRRORONCE > D3DTADDRESS_MIRRORONCE)
		return "UNKNOWN";
	return abbreviatedAddressModes[textureAddressMode];
}

static const char* const abbreviatedTextureFilters[] =
{
	"NONE",
	"POINT",
	"LINEAR",
	"ANISO",
	"PYRMIDQ",
	"GAUSSQ",
	"CONVMONO"
};
static inline const char* const GetAbbreviatedTextureFilterString(const D3DTEXTUREFILTERTYPE texFilter)
{
	if (texFilter > D3DTEXF_CONVOLUTIONMONO)
		return "UNKNOWN";
	return abbreviatedTextureFilters[texFilter];
}

void DrawSamplerStates(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	for (unsigned texID = 0; texID < MAX_NUM_SAMPLERS; ++texID)
	{
		// Skip all textures after 16 and before the DMAP sampler and VTEX samplers
		if (texID >= 16 && texID < D3DDMAPSAMPLER)
			continue;

		const SamplerState& currentSamplerState = deviceState.currentSamplerStates[texID];
		const SamplerState::_stateUnion::_namedStates& namedSamplerStates = currentSamplerState.stateUnion.namedStates;

		const unsigned printedLineNumber = (texID >= 16 ? texID - (D3DDMAPSAMPLER - 16) : texID) * 4;

		char textureIndexString[16];
#pragma warning(push)
#pragma warning(disable:4996)
		switch (texID)
		{
		case D3DDMAPSAMPLER:
			strcpy(textureIndexString, "DMAP");
			break;
		case D3DVERTEXTEXTURESAMPLER0:
			strcpy(textureIndexString, "VTEX0");
			break;
		case D3DVERTEXTEXTURESAMPLER1:
			strcpy(textureIndexString, "VTEX1");
			break;
		case D3DVERTEXTEXTURESAMPLER2:
			strcpy(textureIndexString, "VTEX2");
			break;
		case D3DVERTEXTEXTURESAMPLER3:
			strcpy(textureIndexString, "VTEX3");
			break;
		default:
			sprintf(textureIndexString, "Tex%u", texID);
			break;
		}
#pragma warning(pop)

		OverlayDrawPrintString(hookDev, 0, (2 + printedLineNumber) - currentLineOffset, stateEnabledColor, "%s Min%s Mag%s Mip%s U%s V%s W%s\n"
			"MaxMip:%u MaxAniso:%u sRGB:%c DMAPOffset:%u eIndex:%u\n"
			"LoDBias: %f BorderColor: 0x%08X", 
			textureIndexString, GetAbbreviatedTextureFilterString(namedSamplerStates.minFilter), GetAbbreviatedTextureFilterString(namedSamplerStates.magFilter), GetAbbreviatedTextureFilterString(namedSamplerStates.mipFilter),
			GetAbbreviatedAddressModeString(namedSamplerStates.addressU), GetAbbreviatedAddressModeString(namedSamplerStates.addressV), GetAbbreviatedAddressModeString(namedSamplerStates.addressW),
			namedSamplerStates.maxMipLevel, namedSamplerStates.maxAnisotropy, namedSamplerStates.sRGBTexture ? 'Y' : 'N', namedSamplerStates.dMapOffset, namedSamplerStates.elementIndex,
			namedSamplerStates.mipMapLoDBias, namedSamplerStates.borderColor);
	}
}

void DrawTextureState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	for (unsigned texID = 0; texID < MAX_NUM_SAMPLERS; ++texID)
	{
		// Skip all textures after 16 and before the DMAP sampler and VTEX samplers
		if (texID >= 16 && texID < D3DDMAPSAMPLER)
			continue;

		IDirect3DTexture9Hook* tex2D = deviceState.currentTextures[texID];
		IDirect3DCubeTexture9Hook* texCube = deviceState.currentCubeTextures[texID];
		IDirect3DVolumeTexture9Hook* tex3D = deviceState.currentVolumeTextures[texID];

		const unsigned printedLineNumber = texID >= 16 ? texID - (D3DDMAPSAMPLER - 16) : texID;

		char textureIndexString[16];
#pragma warning(push)
#pragma warning(disable:4996)
		switch (texID)
		{
		case D3DDMAPSAMPLER:
			strcpy(textureIndexString, "DMAP");
			break;
		case D3DVERTEXTEXTURESAMPLER0:
			strcpy(textureIndexString, "VTEX0");
			break;
		case D3DVERTEXTEXTURESAMPLER1:
			strcpy(textureIndexString, "VTEX1");
			break;
		case D3DVERTEXTEXTURESAMPLER2:
			strcpy(textureIndexString, "VTEX2");
			break;
		case D3DVERTEXTEXTURESAMPLER3:
			strcpy(textureIndexString, "VTEX3");
			break;
		default:
			sprintf(textureIndexString, "Tex%u", texID);
			break;
		}
#pragma warning(pop)

		const bool allTexturesNull = (tex2D == NULL) && (texCube == NULL) && (tex3D == NULL);
		if (allTexturesNull)
		{
			OverlayDrawPrintString(hookDev, 0, (1 + printedLineNumber) - currentLineOffset, stateEnabledColor, "%s: NULL", textureIndexString);
			continue;
		}

		const char* textureType = NULL;
		char resolutionBuffer[32] = {0};
		D3DFORMAT format = D3DFMT_UNKNOWN;
		unsigned mipLevels = 0;
		D3DPOOL pool = D3DPOOL_DEFAULT;
		DebuggableUsage usage = UsageNone;
#pragma warning(push)
#pragma warning(disable:4996)
		if (tex2D)
		{
			textureType = "2D";
			sprintf(resolutionBuffer, "%ux%u", tex2D->GetInternalWidth(), tex2D->GetInternalHeight() );
			format = tex2D->GetInternalFormat();
			mipLevels = tex2D->GetInternalMipLevels();
			usage = tex2D->GetInternalUsage();
			pool = tex2D->GetInternalPool();
		}
		else if (texCube)
		{
			textureType = "Cube";
			sprintf(resolutionBuffer, "%ux%ux6", texCube->GetInternalEdgeLength(), texCube->GetInternalEdgeLength() );
			format = texCube->GetInternalFormat();
			mipLevels = texCube->GetInternalMipLevels();
			usage = texCube->GetInternalUsage();
			pool = texCube->GetInternalPool();
		}
		else
		{
			textureType = "3D";
			sprintf(resolutionBuffer, "%ux%ux%u", tex3D->GetInternalWidth(), tex3D->GetInternalHeight(), tex3D->GetInternalDepth() );
			format = tex3D->GetInternalFormat();
			mipLevels = tex3D->GetInternalMipLevels();
			usage = tex3D->GetInternalUsage();
			pool = tex3D->GetInternalPool();
		}
#pragma warning(pop)

		const char* formatString = NULL;
		formatString = GetAbbreviatedFormatStringFromFormat(format);

		const char* poolString = NULL;
		switch (pool)
		{
		case D3DPOOL_DEFAULT:
			poolString = "D";
			break;
		case D3DPOOL_MANAGED:
			poolString = "M";
			break;
		case D3DPOOL_SYSTEMMEM:
			poolString = "SM";
			break;
		case D3DPOOL_SCRATCH:
			poolString = "SC";
			break;
		default:
			poolString = "?";
			break;
		}

		OverlayDrawPrintString(hookDev, 0, (1 + printedLineNumber) - currentLineOffset, stateEnabledColor, "%s %s %s %u mip %s (%u) [%s] 0x%08X", textureIndexString, textureType, resolutionBuffer, mipLevels, formatString, format, poolString, usage);
	}
}

static inline const char* const GetMaterialSourceString(const D3DMATERIALCOLORSOURCE source)
{
	switch (source)
	{
	case D3DMCS_MATERIAL:
		return "MATERIAL";
	case D3DMCS_COLOR1:
		return "DIFFUSE";
	case D3DMCS_COLOR2:
		return "SPECULAR";
	default:
		return "Unknown";
	}
}

void DrawMaterialLightingState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	const RenderStates::_renderStatesUnion::_namedStates& namedRenderStates = deviceState.currentRenderStates.renderStatesUnion.namedStates;

	const bool lightingEnabled = namedRenderStates.lighting;
	const D3DCOLOR lightingEnabledColor = lightingEnabled ? stateEnabledColor : stateDisabledColor;

	OverlayDrawPrintString(hookDev, 0, 1 - currentLineOffset, stateEnabledColor, "Lighting Enabled: %s", lightingEnabled ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 2 - currentLineOffset, lightingEnabledColor, "Diffuse Source: %s", GetMaterialSourceString(namedRenderStates.diffuseMaterialSource) );
	OverlayDrawPrintString(hookDev, 0, 3 - currentLineOffset, lightingEnabledColor, "Ambient Source: %s", GetMaterialSourceString(namedRenderStates.ambientMaterialSource) );
	OverlayDrawPrintString(hookDev, 0, 4 - currentLineOffset, lightingEnabledColor, "Specular Source: %s", GetMaterialSourceString(namedRenderStates.specularMaterialSource) );
	OverlayDrawPrintString(hookDev, 0, 5 - currentLineOffset, lightingEnabledColor, "Emissive Source: %s", GetMaterialSourceString(namedRenderStates.emissiveMaterialSource) );
	OverlayDrawPrintString(hookDev, 0, 6 - currentLineOffset, lightingEnabledColor, "Global Ambient Color: 0x%08X", namedRenderStates.ambient);
	OverlayDrawPrintString(hookDev, 0, 7 - currentLineOffset, lightingEnabledColor, "Local Viewer Enabled: %s", namedRenderStates.localViewer ? "TRUE" : "FALSE");
	OverlayDrawPrintString(hookDev, 0, 8 - currentLineOffset, lightingEnabledColor, "Specular Enabled: %s", namedRenderStates.specularEnable ? "TRUE" : "FALSE");
	OverlayDrawPrintString(hookDev, 0, 9 - currentLineOffset, lightingEnabledColor, "Color Vertex Enabled: %s", namedRenderStates.colorVertex ? "TRUE" : "FALSE");
	OverlayDrawPrintString(hookDev, 0, 10 - currentLineOffset, stateEnabledColor, "NormalizeNormals Enabled: %s", namedRenderStates.normalizeNormals ? "TRUE" : "FALSE");

	OverlayDrawPrintString(hookDev, 0, 11 - currentLineOffset, lightingEnabledColor, "Current Material:\n"
		"Diffuse RGBA: (%f, %f, %f, %f)\n"
		"Ambient RGBA: (%f, %f, %f, %f)\n"
		"Specular RGBA: (%f, %f, %f, %f)\n"
		"Emissive RGBA: (%f, %f, %f, %f)\n"
		"Power: %f",
		deviceState.currentMaterial.Diffuse.r, deviceState.currentMaterial.Diffuse.g, deviceState.currentMaterial.Diffuse.b, deviceState.currentMaterial.Diffuse.a,
		deviceState.currentMaterial.Ambient.r, deviceState.currentMaterial.Ambient.g, deviceState.currentMaterial.Ambient.b, deviceState.currentMaterial.Ambient.a,
		deviceState.currentMaterial.Specular.r, deviceState.currentMaterial.Specular.g, deviceState.currentMaterial.Specular.b, deviceState.currentMaterial.Specular.a,
		deviceState.currentMaterial.Emissive.r, deviceState.currentMaterial.Emissive.g, deviceState.currentMaterial.Emissive.b, deviceState.currentMaterial.Emissive.a,
		deviceState.currentMaterial.Power);
}

static const char* const indentSpaces = "                                ";
static inline void PrintTransform4x4(IDirect3DDevice9Hook* const hookDev, const D3DXMATRIXA16& matrix, const unsigned short currentLine, const char* const label, const int index = -1)
{
	char labelNumberAppended[64] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
	if (index >= 0)
		sprintf(labelNumberAppended, "%s%i", label, index);
	else
		sprintf(labelNumberAppended, "%s", label);
#pragma warning(pop)

	const unsigned indentLength = strlen(labelNumberAppended);
	if (indentLength >= 32)
	{
#ifdef _DEBUG
		__debugbreak(); // What the heck?
#endif
		return;
	}
	const char* const indentStr = indentSpaces + (32 - indentLength);

	OverlayDrawPrintString(hookDev, 0, currentLine, stateEnabledColor, 
		"%s: { %4.4f, %4.4f, %4.4f, %4.4f }\n"
		"%s  { %4.4f, %4.4f, %4.4f, %4.4f }\n"
		"%s  { %4.4f, %4.4f, %4.4f, %4.4f }\n"
		"%s  { %4.4f, %4.4f, %4.4f, %4.4f }",
		labelNumberAppended, matrix.m[0][0], matrix.m[0][1], matrix.m[0][2], matrix.m[0][3], 
		indentStr, matrix.m[1][0], matrix.m[1][1], matrix.m[1][2], matrix.m[1][3], 
		indentStr, matrix.m[2][0], matrix.m[2][1], matrix.m[2][2], matrix.m[2][3], 
		indentStr, matrix.m[3][0], matrix.m[3][1], matrix.m[3][2], matrix.m[3][3]);
}

void DrawTransformsState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	PrintTransform4x4(hookDev, deviceState.currentTransforms.WorldTransforms[0], 1 - currentLineOffset, "World", 0);
	PrintTransform4x4(hookDev, deviceState.currentTransforms.ViewTransform, 5 - currentLineOffset, "View", -1);
	PrintTransform4x4(hookDev, deviceState.currentTransforms.ProjectionTransform, 9 - currentLineOffset, "Proj", -1);

	for (int texID = 0; texID < ARRAYSIZE(deviceState.currentTransforms.TextureTransforms); ++texID)
	{
		const unsigned baseTexTransformLineOffset = 13 + texID * 4;
		PrintTransform4x4(hookDev, deviceState.currentTransforms.TextureTransforms[texID], baseTexTransformLineOffset - currentLineOffset, "TexMat", texID);
	}

	const unsigned baseWorldTransformLineOffset = 13 + ARRAYSIZE(deviceState.currentTransforms.TextureTransforms) * 4;
	for (int worldID = 0; worldID < ARRAYSIZE(deviceState.currentTransforms.WorldTransforms); ++worldID)
	{
		const unsigned thisWorldTransformLineOffset = baseWorldTransformLineOffset + worldID * 4;
		PrintTransform4x4(hookDev, deviceState.currentTransforms.WorldTransforms[worldID], thisWorldTransformLineOffset - currentLineOffset, "World", worldID);
	}
}

void DrawVSConstantsState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	unsigned currentLine = 1;
	for (unsigned i = 0; i < ARRAYSIZE(deviceState.vertexShaderRegisters.ints); ++i)
	{
		const int4& thisReg = deviceState.vertexShaderRegisters.ints[i];
		OverlayDrawPrintString(hookDev, 0, currentLine++ - currentLineOffset, stateEnabledColor, "i%02u: (%i, %i, %i, %i)", i, thisReg.x, thisReg.y, thisReg.z, thisReg.w);
	}

	for (unsigned b = 0; b < ARRAYSIZE(deviceState.vertexShaderRegisters.bools); ++b)
	{
		const BOOL thisReg = deviceState.vertexShaderRegisters.bools[b];
		OverlayDrawPrintString(hookDev, 0, currentLine++ - currentLineOffset, stateEnabledColor, "b%02u: (%s)", b, thisReg ? "TRUE" : "FALSE");
	}

	for (unsigned f = 0; f < ARRAYSIZE(deviceState.vertexShaderRegisters.floats); ++f)
	{
		const float4& thisReg = deviceState.vertexShaderRegisters.floats[f];
		OverlayDrawPrintString(hookDev, 0, currentLine++ - currentLineOffset, stateEnabledColor, "c%03u: (%4.5f, %4.5f, %4.5f, %4.5f)", f, thisReg.x, thisReg.y, thisReg.z, thisReg.w);
	}
}

void DrawPSConstantsState(IDirect3DDevice9Hook* const hookDev, const DeviceState& deviceState, const unsigned short currentLineOffset)
{
	unsigned currentLine = 1;
	for (unsigned i = 0; i < ARRAYSIZE(deviceState.pixelShaderRegisters.ints); ++i)
	{
		const int4& thisReg = deviceState.pixelShaderRegisters.ints[i];
		OverlayDrawPrintString(hookDev, 0, currentLine++ - currentLineOffset, stateEnabledColor, "i%02u: (%i, %i, %i, %i)", i, thisReg.x, thisReg.y, thisReg.z, thisReg.w);
	}

	for (unsigned b = 0; b < ARRAYSIZE(deviceState.pixelShaderRegisters.bools); ++b)
	{
		const BOOL thisReg = deviceState.pixelShaderRegisters.bools[b];
		OverlayDrawPrintString(hookDev, 0, currentLine++ - currentLineOffset, stateEnabledColor, "b%02u: (%s)", b, thisReg ? "TRUE" : "FALSE");
	}

	for (unsigned f = 0; f < ARRAYSIZE(deviceState.pixelShaderRegisters.floats); ++f)
	{
		const float4& thisReg = deviceState.pixelShaderRegisters.floats[f];
		OverlayDrawPrintString(hookDev, 0, currentLine++ - currentLineOffset, stateEnabledColor, "c%03u: (%4.5f, %4.5f, %4.5f, %4.5f)", f, thisReg.x, thisReg.y, thisReg.z, thisReg.w);
	}
}
