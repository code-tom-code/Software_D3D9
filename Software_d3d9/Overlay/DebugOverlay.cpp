#include "DebugOverlay.h"

#include "..\IDirect3DDevice9Hook.h"
#include "..\IDirect3DStateBlock9Hook.h"
#include "..\resource.h"
#include <vector>
#include <map>

extern HINSTANCE hLThisDLL;

#pragma pack(push)
#pragma pack(1)
struct tgaHeader
{
	BYTE idlength;
	BYTE colormaptype;
	BYTE datatypecode;
	USHORT origin;
	USHORT length;
	BYTE colordepth;
	SHORT x_origin;
	SHORT y_origin;
	SHORT width;
	SHORT height;
	BYTE bitsperpixel;
	char imageDescriptor[1];
};
static_assert(sizeof(tgaHeader) == 18, "Error: Unexpected header size!");
#pragma pack(pop)

static const UpdateAndDrawOverlayFunc OverlayUpdateFuncs[overlay_NUM_SCREENS] =
{
	UpdateAndDrawOverlay_WelcomeScreen, // overlay_welcomeScreen
	UpdateAndDrawOverlay_DeviceState // overlay_deviceState
};

struct perDeviceResources
{
	perDeviceResources() : fontMapTexture(NULL), stateBlock(NULL), state(overlay_uninitialized)
	{
		for (unsigned x = 0; x < overlay_NUM_SCREENS; ++x)
			perScreenState[x] = NULL;
	}

	LPDIRECT3DTEXTURE9 fontMapTexture;
	LPDIRECT3DSTATEBLOCK9 stateBlock;
	overlayState state;
	void* perScreenState[overlay_NUM_SCREENS];
};

static std::map<const IDirect3DDevice9Hook*, perDeviceResources> overlayEnabledDevicesSet;

struct vert2D
{
	D3DXVECTOR4 xyzRhw;
	D3DXVECTOR2 texCoord;
};

perDeviceResources* const GetPerDeviceResources(const IDirect3DDevice9Hook* const hookDev)
{
	std::map<const IDirect3DDevice9Hook*, perDeviceResources>::iterator findDev = overlayEnabledDevicesSet.find(hookDev);
	if (findDev == overlayEnabledDevicesSet.end() )
		return NULL;
	return &(findDev->second);
}

static inline const bool IsDeviceOverlayEnabled(const IDirect3DDevice9Hook* const hookDev)
{
	return GetPerDeviceResources(hookDev) != NULL;
}

static inline const bool CheckForOverlayEnableHotkey()
{
	return (GetAsyncKeyState(VK_SHIFT) & 0x8000) && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F9) & 0x8000);
}

static inline const bool CheckForOverlayDisableHotkey()
{
	return (GetAsyncKeyState(VK_SHIFT) & 0x8000) && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F8) & 0x8000);
}

static inline void InitializeOverlayResourcesForDevice(IDirect3DDevice9Hook* const hookDev, perDeviceResources& thisResources)
{
	if (thisResources.stateBlock != NULL)
	{
		thisResources.stateBlock->Release();
		thisResources.stateBlock = NULL;
	}

	if (thisResources.fontMapTexture != NULL)
	{
		thisResources.fontMapTexture->Release();
		thisResources.fontMapTexture = NULL;
	}

	if (FAILED(hookDev->CreateStateBlock(D3DSBT_ALL, &thisResources.stateBlock) ) )
	{
#ifdef _DEBUG
		__debugbreak(); // This shouldn't fail if the device is in a presentable state
#endif
		return;
	}

#pragma warning(push)
#pragma warning(disable:4302) // warning C4302: 'type cast': truncation from 'LPSTR' to 'WORD'
	HRSRC bitmapResource = FindResourceA(hLThisDLL, MAKEINTRESOURCEA(IDR_TGA1), "TGA");
#pragma warning(pop)
	if (!bitmapResource)
	{
#ifdef _DEBUG
		__debugbreak(); // This shouldn't fail if the device is in a presentable state
#endif
		return;
	}

	HGLOBAL loadedResource = LoadResource(hLThisDLL, bitmapResource);
	if (!loadedResource)
	{
#ifdef _DEBUG
		__debugbreak(); // This shouldn't fail if the device is in a presentable state
#endif
		return;
	}

	const unsigned resourceSize = SizeofResource(hLThisDLL, bitmapResource);
	if (resourceSize == 0)
	{
#ifdef _DEBUG
		__debugbreak(); // This shouldn't fail if the device is in a presentable state
#endif
		return;
	}

	const unsigned char* const resourceBytes = (const unsigned char* const)LockResource(loadedResource);
	if (!resourceBytes)
	{
#ifdef _DEBUG
		__debugbreak(); // This shouldn't fail if the device is in a presentable state
#endif
		return;
	}

	const tgaHeader* const headerPtr = (const tgaHeader* const)resourceBytes;
	if (headerPtr->width < 1 || headerPtr->height < 1 || headerPtr->bitsperpixel != 32)
	{
#ifdef _DEBUG
		__debugbreak(); // Somehow we've embedded a bad resource!
#endif
		return;
	}

	if (headerPtr->width != 256 || headerPtr->height != 256)
	{
#ifdef _DEBUG
		__debugbreak(); // This will cause huge problems when we get to font rendering, so this is bad!
#endif
		return;
	}

	if (FAILED(hookDev->CreateTexture(headerPtr->width, headerPtr->height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &thisResources.fontMapTexture, NULL) ) )
	{
#ifdef _DEBUG
		__debugbreak(); // This shouldn't fail if the device is in a presentable state
#endif
		return;
	}

	D3DLOCKED_RECT d3dlr = {0};
	if (FAILED(thisResources.fontMapTexture->LockRect(0, &d3dlr, NULL, 0) ) )
	{
#ifdef _DEBUG
		__debugbreak(); // This shouldn't fail if the device is in a presentable state
#endif
		return;
	}

#ifdef _DEBUG
	if (d3dlr.Pitch != headerPtr->width * sizeof(D3DCOLOR) )
	{
		__debugbreak(); // Unexpected nonlinear pitch!
	}
#endif

	const D3DCOLOR* const startOfImageData = (const D3DCOLOR* const)(headerPtr->idlength + (const BYTE* const)(headerPtr + 1) );

	D3DCOLOR* const lockedTexels = (D3DCOLOR* const)d3dlr.pBits;
	for (int y = 0; y < headerPtr->height; ++y)
	{
		for (int x = 0; x < headerPtr->width; ++x)
		{
			const D3DCOLOR thisFileColor = startOfImageData[x + y * headerPtr->width];
			D3DCOLOR& lockedTexelColor = lockedTexels[x + y * headerPtr->width];

			// Color keying magenta to transparent black
			if (thisFileColor == D3DCOLOR_ARGB(255, 255, 0, 255) )
				lockedTexelColor = D3DCOLOR_ARGB(0, 0, 0, 0);
			else
				lockedTexelColor = thisFileColor;
		}
	}

	if (FAILED(thisResources.fontMapTexture->UnlockRect(0) ) )
	{
#ifdef _DEBUG
		__debugbreak(); // This shouldn't fail if the device is in a presentable state
#endif
		return;
	}

	thisResources.state = overlay_welcomeScreen;
}

static inline const bool IsOverlayStateInitialized(const perDeviceResources& thisResources)
{
	return thisResources.state >= 0;
}

void UpdateOverlay(class IDirect3DDevice9Hook* hookDev)
{
	if (!hookDev)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	if (!IsDeviceOverlayEnabled(hookDev) )
	{
		if (CheckForOverlayEnableHotkey() )
		{
			overlayEnabledDevicesSet.insert(std::make_pair(hookDev, perDeviceResources() ) );
		}
		else
			return;
	}
	else
	{
		if (CheckForOverlayDisableHotkey() )
		{
			DeleteOverlay(hookDev);
			return;
		}
	}

	perDeviceResources* const thisDeviceResources = GetPerDeviceResources(hookDev);
	if (thisDeviceResources == NULL)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	if (!IsOverlayStateInitialized(*thisDeviceResources) )
	{
		InitializeOverlayResourcesForDevice(hookDev, *thisDeviceResources);
		if (!IsOverlayStateInitialized(*thisDeviceResources) )
			return;
	}

	thisDeviceResources->stateBlock->Capture();

	(OverlayUpdateFuncs[thisDeviceResources->state])(hookDev);

	thisDeviceResources->stateBlock->Apply();
}

void OverlayDrawString(class IDirect3DDevice9Hook* const hookDev, const char* const str, const unsigned xLeftChars, const unsigned yTopChars, const unsigned long color /*= 0xFFFFFFFF*/)
{
	if (!hookDev)
		return;

	if (!str)
		return;

	const unsigned len = strlen(str);
	if (!len)
		return;

	D3DVIEWPORT9 viewport = {0};
	if (FAILED(hookDev->GetViewport(&viewport) ) )
		return;

	if (viewport.Width < 1 || viewport.Height < 1)
		return;

	const unsigned maxViewportLine = viewport.Height / textCharHeight;

	// Don't draw lines that are off the bottom edge of the screen, it's just a waste of draw calls!
	if (yTopChars >= maxViewportLine)
		return;

	static std::vector<vert2D> stringVerts;
	stringVerts.clear();

	static std::vector<unsigned> stringIndices;
	stringIndices.clear();

	unsigned currentRowX = xLeftChars * textCharWidth;
	unsigned currentRowY = yTopChars * textCharHeight;

	unsigned numUnprintedCharacters = 0;

	stringVerts.resize(len * 4);
	stringIndices.reserve(len * 6);
	for (unsigned x = 0; x < len; ++x)
	{
		const char thisChar = str[x];

		if (thisChar == '\n')
		{
			currentRowX = xLeftChars * textCharWidth;
			currentRowY += textCharHeight;
			++numUnprintedCharacters;
			continue;
		}
		else if (thisChar == ' ')
		{
			currentRowX += textCharWidth;
			++numUnprintedCharacters;
			continue;
		}

		const float fontMapCharX = (thisChar % 16) / 16.0f;
		const float fontMapCharY = (thisChar / 16) / 16.0f;

		const unsigned thisPrintedCharIndex = x - numUnprintedCharacters;

		for (unsigned y = 0; y < 4; ++y)
		{
			vert2D& thisVert = stringVerts[thisPrintedCharIndex * 4 + y];

			const float cornerCoordNormalizedX = (const float)(y & 0x1);
			const float cornerCoordNormalizedY = (const float)( (y >> 1) & 0x1);

			thisVert.texCoord.x = cornerCoordNormalizedX / 16.0f + fontMapCharX;
			thisVert.texCoord.y = cornerCoordNormalizedY / 16.0f + fontMapCharY;

			thisVert.xyzRhw.x = cornerCoordNormalizedX * (const float)textCharWidth + currentRowX - 0.5f;
			thisVert.xyzRhw.y = cornerCoordNormalizedY * (const float)textCharHeight + currentRowY - 0.5f;
			thisVert.xyzRhw.z = 0.0f;
			thisVert.xyzRhw.w = 1.0f;
		}

		stringIndices.push_back(thisPrintedCharIndex * 4 + 0);
		stringIndices.push_back(thisPrintedCharIndex * 4 + 1);
		stringIndices.push_back(thisPrintedCharIndex * 4 + 2);
		stringIndices.push_back(thisPrintedCharIndex * 4 + 1);
		stringIndices.push_back(thisPrintedCharIndex * 4 + 3);
		stringIndices.push_back(thisPrintedCharIndex * 4 + 2);

		currentRowX += textCharWidth;
	}

	const unsigned numPrintedCharacters = len - numUnprintedCharacters;
	if (numPrintedCharacters == 0)
		return;

	hookDev->SetRenderState(D3DRS_TEXTUREFACTOR, color);
	hookDev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, numPrintedCharacters * 4, numPrintedCharacters * 2, &stringIndices.front(), D3DFMT_INDEX32, &stringVerts.front(), sizeof(vert2D) );
}

void OverlayDrawPrintString(class IDirect3DDevice9Hook* const hookDev, const unsigned xLeftChars, const unsigned yTopChars, const unsigned long color, const char* const formatStr, ...)
{
	va_list printArgs;
	va_start(printArgs, formatStr);

#pragma warning(push)
#pragma warning(disable:4996)
	char printfBuffer[4096];
	const int len = vsprintf(printfBuffer, formatStr, printArgs);
#pragma warning(pop)
	va_end(printArgs);

	if (len < 1)
		return;

	OverlayDrawString(hookDev, printfBuffer, xLeftChars, yTopChars, color);
}

void OverlaySetDeviceStateForText(class IDirect3DDevice9Hook* const hookDev)
{
	const perDeviceResources* const thisDeviceResources = GetPerDeviceResources(hookDev);
	if (thisDeviceResources == NULL)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	LPDIRECT3DSURFACE9 backbufferSurf = NULL;
	D3DSURFACE_DESC backbufferDesc = {};
	hookDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbufferSurf);
	backbufferSurf->GetDesc(&backbufferDesc);
	backbufferSurf->Release();
	backbufferSurf = NULL;

	// Setup default full-screen scissor rect:
	RECT scissorRect = {0};
	scissorRect.right = backbufferDesc.Width;
	scissorRect.bottom = backbufferDesc.Height;
	hookDev->SetScissorRect(&scissorRect);

	// Setup default full-screen viewport:
	D3DVIEWPORT9 viewport = {0};
	viewport.Width = backbufferDesc.Width;
	viewport.Height = backbufferDesc.Height;
	viewport.MaxZ = 1.0f;
	hookDev->SetViewport(&viewport);

	// Setup our FVF and shaders for text rendering:
	hookDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
	hookDev->SetPixelShader(NULL);
	hookDev->SetVertexShader(NULL);

	// Configure a bunch of renderstates back to their default values:
	hookDev->SetRenderState(D3DRS_CLIPPLANEENABLE, 0x00000000); // Disable all clipping planes
	hookDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
	hookDev->SetRenderState(D3DRS_LIGHTING, TRUE);
	hookDev->SetRenderState(D3DRS_ZENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_COLORVERTEX, FALSE);
	hookDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	hookDev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	hookDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_LASTPIXEL, TRUE);
	hookDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	hookDev->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_FOGENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_RANGEFOGENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_CLIPPING, TRUE);
	hookDev->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
	hookDev->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	hookDev->SetRenderState(D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF);
	hookDev->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
	hookDev->SetRenderState(D3DRS_ENABLEADAPTIVETESSELLATION, FALSE);
	hookDev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0x00000000);
	hookDev->SetRenderState(D3DRS_DEPTHBIAS, 0x00000000);
	hookDev->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

	// Configure material states:
	hookDev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	hookDev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	hookDev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	hookDev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
	hookDev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_ARGB(255, 255, 255, 255) );
	D3DMATERIAL9 textMaterial = {0};
	textMaterial.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
	textMaterial.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	textMaterial.Specular = { 1.0f, 1.0f, 1.0f, 1.0f };
	textMaterial.Emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
	textMaterial.Power = 0.0f;
	hookDev->SetMaterial(&textMaterial);

	// Configure alpha-blend for alpha blending mode:
	hookDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	hookDev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	hookDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	hookDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	hookDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);

	// Setup sampler states for text rendering:
	hookDev->SetTexture(0, thisDeviceResources->fontMapTexture);
	hookDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	hookDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	hookDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	hookDev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);

	// Configure texture stage states for text rendering:
	hookDev->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, 255, 255, 255) );
	hookDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	hookDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	hookDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
	hookDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	hookDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	hookDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	hookDev->SetTextureStageState(0, D3DTSS_RESULTARG, D3DTA_CURRENT);
	hookDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	hookDev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

void DeleteOverlay(const class IDirect3DDevice9Hook* const hookDev)
{
	std::map<const IDirect3DDevice9Hook*, perDeviceResources>::iterator foundIt = overlayEnabledDevicesSet.find(hookDev);
	if (foundIt == overlayEnabledDevicesSet.end() )
		return; // This is okay, we may have a device that never created an overlay

	foundIt->second.fontMapTexture->Release();
	foundIt->second.stateBlock->Release();
	overlayEnabledDevicesSet.erase(foundIt);
}

void ResetOverlay(const class IDirect3DDevice9Hook* const hookDev)
{
	DeleteOverlay(hookDev);
}

void SetOverlayScreenState(const class IDirect3DDevice9Hook* const hookDev, const overlayState newState)
{
	std::map<const IDirect3DDevice9Hook*, perDeviceResources>::iterator foundIt = overlayEnabledDevicesSet.find(hookDev);
	if (foundIt == overlayEnabledDevicesSet.end() )
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return; // Skip setting overlay state for uncreated overlays
	}

	foundIt->second.state = newState;
}

void SetOverlayPerScreenData(const class IDirect3DDevice9Hook* const hookDev, void* const newScreenData)
{
	std::map<const IDirect3DDevice9Hook*, perDeviceResources>::iterator foundIt = overlayEnabledDevicesSet.find(hookDev);
	if (foundIt == overlayEnabledDevicesSet.end() )
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return; // Skip setting per-screen data for uncreated overlays
	}

	perDeviceResources& thisDeviceResources = foundIt->second;
	if (IsOverlayStateInitialized(thisDeviceResources) )
	{
		thisDeviceResources.perScreenState[thisDeviceResources.state] = newScreenData;
	}
}

void* const GetOverlayPerScreenData(const class IDirect3DDevice9Hook* const hookDev)
{
	std::map<const IDirect3DDevice9Hook*, perDeviceResources>::iterator foundIt = overlayEnabledDevicesSet.find(hookDev);
	if (foundIt == overlayEnabledDevicesSet.end() )
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return NULL; // Skip setting per-screen data for uncreated overlays
	}

	const perDeviceResources& thisDeviceResources = foundIt->second;
	if (IsOverlayStateInitialized(thisDeviceResources) )
		return thisDeviceResources.perScreenState[thisDeviceResources.state];
	else
		return NULL;
}
