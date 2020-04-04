#pragma once

enum overlayState
{
	overlay_uninitialized = -1,
	overlay_welcomeScreen = 0,
	overlay_deviceState = 1,

	// This enum member must always be last
	overlay_NUM_SCREENS
};

static const unsigned textCharWidth = 12u;
static const unsigned textCharHeight = textCharWidth;

typedef void (*UpdateAndDrawOverlayFunc)(class IDirect3DDevice9Hook* const hookDev);

void UpdateOverlay(class IDirect3DDevice9Hook* const hookDev);
void DeleteOverlay(const class IDirect3DDevice9Hook* const hookDev);
void ResetOverlay(const class IDirect3DDevice9Hook* const hookDev);

void SetOverlayScreenState(const class IDirect3DDevice9Hook* const hookDev, const overlayState newState);
void SetOverlayPerScreenData(const class IDirect3DDevice9Hook* const hookDev, void* const newScreenData);
void* const GetOverlayPerScreenData(const class IDirect3DDevice9Hook* const hookDev);

void OverlaySetDeviceStateForText(class IDirect3DDevice9Hook* const hookDev);
void OverlayDrawString(class IDirect3DDevice9Hook* const hookDev, const char* const str, const unsigned xLeftChars, const unsigned yTopChars, const unsigned long color = 0xFFFFFFFF);
void OverlayDrawPrintString(class IDirect3DDevice9Hook* const hookDev, const unsigned xLeftChars, const unsigned yTopChars, const unsigned long color, const char* const formatStr, ...);

// Add screen-specific overlay functions here:
void UpdateAndDrawOverlay_WelcomeScreen(class IDirect3DDevice9Hook* const hookDev);
void UpdateAndDrawOverlay_DeviceState(class IDirect3DDevice9Hook* const hookDev);
