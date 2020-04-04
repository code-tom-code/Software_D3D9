#pragma once

#include "GlobalToggles.h"
#include "IDirect3DDevice9Hook.h"
#include "IDirect3DSwapChain9Hook.h"
#include "IDirect3DSurface9Hook.h"
#include "IDirect3DIndexBuffer9Hook.h"
#include "IDirect3DVertexBuffer9Hook.h"
#include "IDirect3DBaseTexture9Hook.h"
#include "IDirect3DTexture9Hook.h"
#include "IDirect3DCubeTexture9Hook.h"
#include "IDirect3DVolumeTexture9Hook.h"
#include "IDirect3DVertexDeclaration9Hook.h"
#include "IDirect3DVertexShader9Hook.h"
#include "IDirect3DPixelShader9Hook.h"
#include "IDirect3DStateBlock9Hook.h"

COM_DECLSPEC_NOTHROW UINT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetAvailableTextureMem(THIS)
{
	UINT ret = d3d9dev->GetAvailableTextureMem();
	// TODO: Implement this...
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetDirect3D(THIS_ IDirect3D9** ppD3D9)
{
	LPDIRECT3D9 realD3D9 = NULL;
	HRESULT ret = d3d9dev->GetDirect3D(&realD3D9);
	if (FAILED(ret) )
		return ret;

	if (!ppD3D9)
		return D3DERR_INVALIDCALL;

	// Check that the parentHook's underlying IDirect3D9* matches the realD3D9 pointer
#ifdef _DEBUG
	if (parentHook->GetUnderlyingD3D9() != realD3D9)
	{
		DbgBreakPrint("Error: Unknown IDirect3D9 interface detected on D3D9 device (not hooked)");
	}
#endif
	*ppD3D9 = parentHook;
	parentHook->AddRef(); // Super important to increment the ref-count here, otherwise our parent object will get destroyed when Release() is called on it!
	return ret;
}

COM_DECLSPEC_NOTHROW /*static*/ void IDirect3DDevice9Hook::ModifyDeviceCaps(D3DCAPS9& caps)
{
	caps.VertexShaderVersion = D3DVS_VERSION(3, 0); // VS_3_0
	caps.PixelShaderVersion = D3DPS_VERSION(3, 0); // PS_3_0

	// We can only draw up to 1 million primitives per draw call
	caps.MaxPrimitiveCount = ( (1024 * 1024) - 1);

	// We can only draw up to 1 million vertices per draw call
	caps.MaxVertexIndex = ( (1024 * 1024) - 1);

	// We do not support tessellation
	caps.DevCaps &= (~(D3DDEVCAPS_NPATCHES | D3DDEVCAPS_QUINTICRTPATCHES | D3DDEVCAPS_RTPATCHES | D3DDEVCAPS_RTPATCHHANDLEZERO) );
	caps.DevCaps2 &= (~(D3DDEVCAPS2_ADAPTIVETESSRTPATCH | D3DDEVCAPS2_ADAPTIVETESSNPATCH | D3DDEVCAPS2_DMAPNPATCH | D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH) );

	// So this wouldn't be very hard to implement support for, but currently we only support one output channel write mask for all render targets
	caps.PrimitiveMiscCaps &= (~D3DPMISCCAPS_INDEPENDENTWRITEMASKS);

	// We don't support convolution mono texture filtering
	caps.TextureFilterCaps &= (~D3DPTFILTERCAPS_CONVOLUTIONMONO);

	// We don't support D3DTEXF_GAUSSIANQUAD or D3DTEXF_PYRAMIDALQUAD texture filtering
	caps.TextureFilterCaps &= (~(D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD | D3DPTFILTERCAPS_MINFGAUSSIANQUAD) );
	caps.CubeTextureFilterCaps &= (~(D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD | D3DPTFILTERCAPS_MINFGAUSSIANQUAD) );
	caps.VolumeTextureFilterCaps &= (~(D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD | D3DPTFILTERCAPS_MINFGAUSSIANQUAD) );

	// We don't support point sprites (yet)
	caps.MaxPointSize = 1.0f; // If set to 1.0f then device does not support point size control.
	caps.PrimitiveMiscCaps &= (~D3DPMISCCAPS_CLIPPLANESCALEDPOINTS); // Don't support clipping scaled point sprites (yet)
	caps.FVFCaps &= (~D3DFVFCAPS_PSIZE); // Don't support PSIZE in FVF's (yet)

	// Limit to 6 user clip-planes (see vertexClipStruct for why we can only support 6)
	caps.MaxUserClipPlanes = 6;

	// Cap the max stream stride such that it'll fit in a uint16
	if (caps.MaxStreamStride > 0xFFFF)
		caps.MaxStreamStride = 0xFFFF;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetDeviceCaps(THIS_ D3DCAPS9* pCaps)
{
	HRESULT ret = d3d9dev->GetDeviceCaps(pCaps);
	if (FAILED(ret) )
		return ret;

	if (pCaps)
	{
		ModifyDeviceCaps(*pCaps);
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetDisplayMode(THIS_ UINT iSwapChain, D3DDISPLAYMODE* pMode)
{
	if (iSwapChain == 0)
	{
		HRESULT ret = implicitSwapChain->GetDisplayMode(pMode);
		return ret;
	}
	else
	{
#ifdef _DEBUG
		// Non-implicit swap-chains not yet supported
		DbgBreakPrint("Error: Non-implicit swap chains not yet supported");
#endif
		return D3DERR_INVALIDCALL;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetCreationParameters(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
	HRESULT ret = d3d9dev->GetCreationParameters(pParameters);
	// TODO: Implement this...
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetCursorProperties(THIS_ UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap)
{
	HRESULT ret = d3d9dev->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap);
	return ret;
}

COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE IDirect3DDevice9Hook::SetCursorPosition(THIS_ int X, int Y, DWORD Flags)
{
	d3d9dev->SetCursorPosition(X, Y, Flags);
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetSwapChain(THIS_ UINT iSwapChain, IDirect3DSwapChain9** pSwapChain)
{
	LPDIRECT3DSWAPCHAIN9 realSwapChain = NULL;
	HRESULT ret = d3d9dev->GetSwapChain(iSwapChain, &realSwapChain);
	if (FAILED(ret) )
		return ret;

	if (!pSwapChain)
		return D3DERR_INVALIDCALL;

	if (iSwapChain == 0)
	{
#ifdef _DEBUG
		if (implicitSwapChain->GetUnderlyingSwapChain() != realSwapChain)
		{
			DbgBreakPrint("Error: Unknown swap chain used (not hooked)");
		}
#endif
		*pSwapChain = implicitSwapChain;
		implicitSwapChain->AddRef();
	}
	else
	{
#ifdef _DEBUG
		// TODO: Implement CreateAdditionalSwapChain()
		DbgBreakPrint("Error: Additional swap chains not yet supported");
#endif
		return D3DERR_INVALIDCALL;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW UINT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetNumberOfSwapChains(THIS)
{
	UINT ret = d3d9dev->GetNumberOfSwapChains();
#ifdef _DEBUG
	if (ret != 1)
	{
		// Uhhhhhhhh, maybe they called CreateAdditionalSwapChain() and that incremented the count?
		DbgBreakPrint("Error: Additional swap chains not yet supported");
	}
#endif
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetBackBuffer(THIS_ UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer)
{
	if (iSwapChain == 0)
	{
		HRESULT ret = implicitSwapChain->GetBackBuffer(iBackBuffer, Type, ppBackBuffer);
		return ret;
	}
	else
	{
#ifdef _DEBUG
		// Swap chains beyond 0 are not yet supported!
		DbgBreakPrint("Error: Additional swap chains not yet supported");
#endif
		return D3DERR_INVALIDCALL;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetRasterStatus(THIS_ UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)
{
	if (iSwapChain == 0)
	{
		HRESULT ret = implicitSwapChain->GetRasterStatus(pRasterStatus);
		return ret;
	}
	else
	{
#ifdef _DEBUG
		// Swap chains beyond 0 are not yet supported!
		DbgBreakPrint("Error: Additional swap chains not yet supported");
#endif
		return D3DERR_INVALIDCALL;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetDialogBoxMode(THIS_ BOOL bEnableDialogs)
{
	HRESULT ret = d3d9dev->SetDialogBoxMode(bEnableDialogs);
	if (FAILED(ret) )
		return ret;

	// TODO: Enforce all of the conditions set forth in: https://msdn.microsoft.com/en-us/library/windows/desktop/bb174432(v=vs.85).aspx
	enableDialogs = bEnableDialogs;

	return ret;
}

COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE IDirect3DDevice9Hook::SetGammaRamp(THIS_ UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp)
{
	if (iSwapChain == 0)
	{
		// Set the gamma ramp on the actual device:
		d3d9dev->SetGammaRamp(0, Flags, pRamp);

		// Set the gamma ramp on the virtual software device too:
		implicitSwapChain->SetGammaRamp(Flags, pRamp);
	}
	else
	{
#ifdef _DEBUG
		// Swap chains beyond 0 are not yet supported!
		DbgBreakPrint("Error: Additional swap chains not yet supported");
#endif
	}
}

COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE IDirect3DDevice9Hook::GetGammaRamp(THIS_ UINT iSwapChain, D3DGAMMARAMP* pRamp)
{
	if (iSwapChain == 0)
	{
		implicitSwapChain->GetGammaRamp(pRamp);
	}
	else
	{
#ifdef _DEBUG
		// Swap chains beyond 0 are not yet supported!
		DbgBreakPrint("Error: Additional swap chains not yet supported");
#endif
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetRenderTargetData(THIS_ IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface)
{
	const IDirect3DSurface9Hook* const renderTargetHookPtr = dynamic_cast<IDirect3DSurface9Hook*>(pRenderTarget);
	if (renderTargetHookPtr)
		pRenderTarget = renderTargetHookPtr->GetUnderlyingSurface();
#ifdef _DEBUG
	else if (pRenderTarget != NULL)
	{
		DbgBreakPrint("Error: GetRenderTargetData called with a non-hooked render target pointer");
	}
#endif
	const IDirect3DSurface9Hook* const destHookPtr = dynamic_cast<IDirect3DSurface9Hook*>(pDestSurface);
	if (destHookPtr)
		pDestSurface = destHookPtr->GetUnderlyingSurface();
#ifdef _DEBUG
	else if (pDestSurface != NULL)
	{
		DbgBreakPrint("Error: GetRenderTargetData called with a non-hooked dest surface pointer");
	}
#endif
	HRESULT ret = d3d9dev->GetRenderTargetData(renderTargetHookPtr ? renderTargetHookPtr->GetUnderlyingSurface() : NULL, destHookPtr ? destHookPtr->GetUnderlyingSurface() : NULL);
	if (SUCCEEDED(ret) )
	{
		// TODO: Implement this...
	}
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetFrontBufferData(THIS_ UINT iSwapChain, IDirect3DSurface9* pDestSurface)
{
	if (iSwapChain == 0)
	{
		HRESULT ret = implicitSwapChain->GetFrontBufferData(pDestSurface);
		return ret;
	}
	else
	{
#ifdef _DEBUG
		// Non-zero swap chains not yet supported
		DbgBreakPrint("Error: Additional swap chains not yet supported!");
#endif
		return D3DERR_INVALIDCALL;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetRenderTarget(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget)
{
	LPDIRECT3DSURFACE9 realSurface = NULL;
	HRESULT ret = d3d9dev->GetRenderTarget(RenderTargetIndex, &realSurface);
	if (FAILED(ret) )
		return ret;

	if (!ppRenderTarget)
		return D3DERR_INVALIDCALL;

	if (RenderTargetIndex < D3D_MAX_SIMULTANEOUS_RENDERTARGETS)
	{
		IDirect3DSurface9Hook* const currentRenderTargetHook = currentState.currentRenderTargets[RenderTargetIndex];
		if (currentRenderTargetHook)
		{
#ifdef _DEBUG
			LPDIRECT3DSURFACE9 underlyingSurface = currentRenderTargetHook->GetUnderlyingSurface();
			if (realSurface != underlyingSurface)
			{
				DbgBreakPrint("Error: Render target underlying surface has a non-hooked surface pointer");
			}
#endif
		}

		*ppRenderTarget = currentRenderTargetHook;
		if (currentRenderTargetHook)
			currentRenderTargetHook->AddRef();
	}
#ifdef _DEBUG
	else
	{
		DbgBreakPrint("Error: Render target index is out of bounds");
	}
#endif

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetDepthStencilSurface(THIS_ IDirect3DSurface9* pNewZStencil)
{
	IDirect3DSurface9Hook* hookPtr = dynamic_cast<IDirect3DSurface9Hook*>(pNewZStencil);
	if (hookPtr)
		pNewZStencil = hookPtr->GetUnderlyingSurface();
#ifdef _DEBUG
	else if (pNewZStencil != NULL)
	{
		DbgBreakPrint("Error: SetDepthStencilSurface called with a non-hooked surface pointer");
	}
#endif
	HRESULT ret = d3d9dev->SetDepthStencilSurface(pNewZStencil);
	if (FAILED(ret) )
		return ret;

	currentState.currentDepthStencil = hookPtr;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetDepthStencilSurface(THIS_ IDirect3DSurface9** ppZStencilSurface)
{
	LPDIRECT3DSURFACE9 realSurface = NULL;
	HRESULT ret = d3d9dev->GetDepthStencilSurface(&realSurface);
	if (FAILED(ret) )
		return ret;

	if (ppZStencilSurface)
	{
		IDirect3DSurface9Hook* const currentDepthStencilHook = currentState.currentDepthStencil;
		if (currentDepthStencilHook)
		{
#ifdef _DEBUG
			LPDIRECT3DSURFACE9 underlyingSurface = currentDepthStencilHook->GetUnderlyingSurface();
			if (realSurface != underlyingSurface)
			{
				DbgBreakPrint("Error: Current depth-stencil surface has a non-hooked surface pointer");
			}
#endif
		}

		*ppZStencilSurface = currentDepthStencilHook;
		if (currentDepthStencilHook)
			currentDepthStencilHook->AddRef();
	}
#ifdef _DEBUG
	else
	{
		DbgBreakPrint("Error: GetDepthStencilSurface called with NULL pointer");
	}
#endif

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetTransform(THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
	HRESULT ret = d3d9dev->SetTransform(State, pMatrix);
	if (FAILED(ret) )
		return ret;

	if (pMatrix)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			currentlyRecordingStateBlock->MarkSetTransformCaptured(State);
		}
		else
			targetDeviceState = &currentState;

		if (State < D3DTS_WORLD)
		{
			switch (State)
			{
			case D3DTS_VIEW:
				targetDeviceState->currentTransforms.SetViewTransform(*pMatrix);
				break;
			case D3DTS_PROJECTION:
				targetDeviceState->currentTransforms.SetProjectionTransform(*pMatrix);
				break;
			case D3DTS_TEXTURE0:
			case D3DTS_TEXTURE1:
			case D3DTS_TEXTURE2:
			case D3DTS_TEXTURE3:
			case D3DTS_TEXTURE4:
			case D3DTS_TEXTURE5:
			case D3DTS_TEXTURE6:
			case D3DTS_TEXTURE7:
				targetDeviceState->currentTransforms.SetTextureTransform(*pMatrix, (const unsigned char)(State - D3DTS_TEXTURE0) );
				break;
			default:
#ifdef _DEBUG
				// Invalid transform state enum!
				DbgBreakPrint("Error: Invalid transform state enum for SetTransform()");
#endif
				break;
			}
		}
		else if (State < D3DTS_WORLDMATRIX(MAX_WORLD_TRANSFORMS) ) // World transforms
		{
			targetDeviceState->currentTransforms.SetWorldTransform(*pMatrix, (const unsigned char)(State - D3DTS_WORLD) );
		}
#ifdef _DEBUG
		else
		{
			// Index too high, can't be valid state!
			DbgBreakPrint("Error: SetTransform state index out of bounds");
		}
#endif
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetTransform(THIS_ D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
	D3DMATRIX realMatrix = {0};
	HRESULT ret = d3d9dev->GetTransform(State, &realMatrix);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (State < D3DTS_WORLD)
	{
		switch (State)
		{
		case D3DTS_VIEW:
			if (memcmp(&currentState.currentTransforms.ViewTransform, &realMatrix, sizeof(D3DMATRIX) ) != 0)
			{
				DbgBreakPrint("Error: View matrix doesn't match internal view matrix");
			}
			break;
		case D3DTS_PROJECTION:
			if (memcmp(&currentState.currentTransforms.ProjectionTransform, &realMatrix, sizeof(D3DMATRIX) ) != 0)
			{
				DbgBreakPrint("Error: Proj matrix doesn't match internal proj matrix");
			}
			break;
		case D3DTS_TEXTURE0:
		case D3DTS_TEXTURE1:
		case D3DTS_TEXTURE2:
		case D3DTS_TEXTURE3:
		case D3DTS_TEXTURE4:
		case D3DTS_TEXTURE5:
		case D3DTS_TEXTURE6:
		case D3DTS_TEXTURE7:
			if (memcmp(&currentState.currentTransforms.TextureTransforms[State - D3DTS_TEXTURE0], &realMatrix, sizeof(D3DMATRIX) ) != 0)
			{
				DbgBreakPrint("Error: Texture matrix doesn't match internal texture matrix");
			}
			break;
		default:
			// Invalid transform state enum!
			DbgBreakPrint("Error: Invalid transform enum passed to GetTransform");
			break;
		}
	}
	else if (State < D3DTS_WORLDMATRIX(MAX_WORLD_TRANSFORMS) ) // World transforms
	{
		if (memcmp(&currentState.currentTransforms.WorldTransforms[State - D3DTS_WORLD], &realMatrix, sizeof(D3DMATRIX) ) != 0)
		{
			DbgBreakPrint("Error: World matrix doesn't match internal world matrix");
		}
	}
	else
	{
		// Index too high, can't be valid state!
		DbgBreakPrint("Error: Transform state out of bounds");
	}
#endif

	if (pMatrix)
	{
		if (State < D3DTS_WORLD)
		{
			switch (State)
			{
			case D3DTS_VIEW:
				memcpy(pMatrix, &currentState.currentTransforms.ViewTransform, sizeof(D3DMATRIX) );
				break;
			case D3DTS_PROJECTION:
				memcpy(pMatrix, &currentState.currentTransforms.ProjectionTransform, sizeof(D3DMATRIX) );
				break;
			case D3DTS_TEXTURE0:
			case D3DTS_TEXTURE1:
			case D3DTS_TEXTURE2:
			case D3DTS_TEXTURE3:
			case D3DTS_TEXTURE4:
			case D3DTS_TEXTURE5:
			case D3DTS_TEXTURE6:
			case D3DTS_TEXTURE7:
				memcpy(pMatrix, &currentState.currentTransforms.TextureTransforms[State - D3DTS_TEXTURE0], sizeof(D3DMATRIX) );
				break;
			default:
#ifdef _DEBUG
				// Invalid transform state enum!
				DbgBreakPrint("Error: Invalid transform enum passed to GetTransform");
#endif
				break;
			}
		}
		else if (State < D3DTS_WORLDMATRIX(MAX_WORLD_TRANSFORMS) ) // World transforms
		{
			memcpy(pMatrix, &currentState.currentTransforms.WorldTransforms[State - D3DTS_WORLD], sizeof(D3DMATRIX) );
		}
#ifdef _DEBUG
		else
		{
			// Index too high, can't be valid state!
			DbgBreakPrint("Error: Transform state is out of bounds");
		}
#endif
	}
	else
		return D3DERR_INVALIDCALL;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::MultiplyTransform(THIS_ D3DTRANSFORMSTATETYPE Transform, CONST D3DMATRIX* pMatrix)
{
	HRESULT ret = d3d9dev->MultiplyTransform(Transform, pMatrix);
	if (FAILED(ret) )
		return ret;

	if (pMatrix)
	{
		D3DXMATRIXA16 d3dxMatrix(*pMatrix);
		if (Transform < D3DTS_WORLD)
		{
			switch (Transform)
			{
			case D3DTS_VIEW:
				currentState.currentTransforms.ViewTransform = d3dxMatrix * currentState.currentTransforms.ViewTransform;
				break;
			case D3DTS_PROJECTION:
				currentState.currentTransforms.ProjectionTransform = d3dxMatrix * currentState.currentTransforms.ProjectionTransform;
				break;
			case D3DTS_TEXTURE0:
			case D3DTS_TEXTURE1:
			case D3DTS_TEXTURE2:
			case D3DTS_TEXTURE3:
			case D3DTS_TEXTURE4:
			case D3DTS_TEXTURE5:
			case D3DTS_TEXTURE6:
			case D3DTS_TEXTURE7:
				currentState.currentTransforms.TextureTransforms[Transform - D3DTS_TEXTURE0] = d3dxMatrix * currentState.currentTransforms.TextureTransforms[Transform - D3DTS_TEXTURE0];
				break;
			default:
#ifdef _DEBUG
				// Invalid transform state enum!
				DbgBreakPrint("Error: Invalid transform enum passed to MultiplyTransform");
#endif
				break;
			}
		}
		else if (Transform < D3DTS_WORLDMATRIX(MAX_WORLD_TRANSFORMS) ) // World transforms
		{
			currentState.currentTransforms.WorldTransforms[Transform - D3DTS_WORLD] = d3dxMatrix * currentState.currentTransforms.WorldTransforms[Transform - D3DTS_WORLD];
		}
#ifdef _DEBUG
		else
		{
			// Index too high, can't be valid state!
			DbgBreakPrint("Error: Transform state is out of bounds");
		}
#endif
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetViewport(THIS_ CONST D3DVIEWPORT9* pViewport)
{
	HRESULT ret = d3d9dev->SetViewport(pViewport);
	if (FAILED(ret) )
		return ret;

	// Real D3D9 crashes if you pass in a NULL pointer, so we can do basically whatever we want here
	if (pViewport)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetViewport>();
		}
		else
			targetDeviceState = &currentState;

		targetDeviceState->cachedViewport.viewport = *pViewport;
		targetDeviceState->cachedViewport.RecomputeCache();
	}
#ifdef _DEBUG
	else
	{
		DbgBreakPrint("Error: SetViewport called with NULL viewport pointer");
	}
#endif

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetViewport(THIS_ D3DVIEWPORT9* pViewport)
{
	D3DVIEWPORT9 localRet = {0};
	HRESULT ret = d3d9dev->GetViewport(&localRet);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (localRet != currentState.cachedViewport.viewport)
	{
		DbgBreakPrint("Error: Internal viewport doesn't match viewport");
	}
#endif

	if (!pViewport)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: GetViewport called with a NULL pointer");
#endif
		return D3DERR_INVALIDCALL;
	}
	else
		*pViewport = currentState.cachedViewport.viewport;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetMaterial(THIS_ CONST D3DMATERIAL9* pMaterial)
{
	HRESULT ret = d3d9dev->SetMaterial(pMaterial);
	if (FAILED(ret) )
		return ret;

	// D3D9 just crashes if you pass a NULL pointer pMaterial, so we can do whatever in that case
	if (pMaterial)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetMaterial>();
		}
		else
			targetDeviceState = &currentState;

		targetDeviceState->currentMaterial = *pMaterial;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetMaterial(THIS_ D3DMATERIAL9* pMaterial)
{
	D3DMATERIAL9 realMaterial = {0};
	HRESULT ret = d3d9dev->GetMaterial(&realMaterial);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (memcmp(&realMaterial, &currentState.currentMaterial, sizeof(D3DMATERIAL9) ) != 0)
	{
		DbgBreakPrint("Error: Internal material doesn't match material");
	}
#endif

	if (pMaterial)
	{
		*pMaterial = currentState.currentMaterial;
		return ret;
	}
	else
		return D3DERR_INVALIDCALL;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetLight(THIS_ DWORD Index, CONST D3DLIGHT9* pLight)
{
	HRESULT ret = d3d9dev->SetLight(Index, pLight);
	if (FAILED(ret) )
		return ret;

	if (pLight)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			currentlyRecordingStateBlock->MarkSetLightCaptured(Index);
		}
		else
			targetDeviceState = &currentState;

		std::map<UINT, LightInfo*>::iterator it = targetDeviceState->lightInfoMap->find(Index);
		if (it == targetDeviceState->lightInfoMap->end() )
		{
			LightInfo* newLightInfo = new LightInfo;
			newLightInfo->light = *pLight;
			targetDeviceState->lightInfoMap->insert(std::make_pair(Index, newLightInfo) );
		}
		else
		{
			if (it->second != &LightInfo::defaultLight)
				it->second->light = *pLight;
			else
			{
				LightInfo* newLightInfo = new LightInfo;
				newLightInfo->light = *pLight;
				it->second = newLightInfo;
			}
		}
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetLight(THIS_ DWORD Index, D3DLIGHT9* pLight)
{
	D3DLIGHT9 realLight = {};
	HRESULT ret = d3d9dev->GetLight(Index, &realLight);
	if (FAILED(ret) )
		return ret;

	const std::map<UINT, LightInfo*>::const_iterator it = currentState.lightInfoMap->find(Index);
	if (it == currentState.lightInfoMap->end() )
	{
		return D3DERR_INVALIDCALL;
	}
	else
	{
#ifdef _DEBUG
		if (memcmp(&realLight, &it->second->light, sizeof(D3DLIGHT9) ) != 0)
		{
			DbgBreakPrint("Error: Internal light doesn't match material");
		}
#endif
		if (pLight)
		{
			*pLight = it->second->light;
			return ret;
		}
		else
			return D3DERR_INVALIDCALL;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::LightEnable(THIS_ DWORD Index, BOOL Enable)
{
	HRESULT ret = d3d9dev->LightEnable(Index, Enable);
	if (FAILED(ret) )
		return ret;

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkLightEnableCaptured(Index, Enable);
	}
	else
		targetDeviceState = &currentState;

	const std::map<UINT, LightInfo*>::const_iterator it = targetDeviceState->lightInfoMap->find(Index);
	if (it == targetDeviceState->lightInfoMap->end() )
	{
		targetDeviceState->lightInfoMap->insert(std::make_pair(Index, &LightInfo::defaultLight) );
		for (unsigned x = 0; x < MAX_ENABLED_LIGHTS; ++x)
		{
			if (!targetDeviceState->enabledLightIndices[x])
			{
				targetDeviceState->enabledLightIndices[x] = &LightInfo::defaultLight;
				return ret;
			}
		}

#ifdef _DEBUG
		// Can't enable more than MAX_ENABLED_LIGHTS lights simultaneously!
		DbgBreakPrint("Error: Can't enable more than MAX_ENABLED_LIGHTS (8) simultaneously!");
#endif

		return D3DERR_INVALIDCALL;
	}
	else
	{
		if (it->second->activeLightIndex >= 0) // Expecting an enabled light
		{
			if (Enable)
			{
#ifdef _DEBUG
				DbgBreakPrint("Warning: Enabling an already-enabled light");
#endif
				// Do nothing, light is already enabled
				return ret;
			}
			else
			{
				targetDeviceState->enabledLightIndices[it->second->activeLightIndex] = NULL;
				it->second->activeLightIndex = -1;
				return ret;
			}
		}
		else // Expecting a disabled light
		{
			if (Enable)
			{
				for (unsigned x = 0; x < MAX_ENABLED_LIGHTS; ++x)
				{
					if (targetDeviceState->enabledLightIndices[x] == NULL)
					{
						targetDeviceState->enabledLightIndices[x] = it->second;
						it->second->activeLightIndex = x;
						return ret;
					}
				}

#ifdef _DEBUG
				// Can't enable more than MAX_ENABLED_LIGHTS lights simultaneously!
				DbgBreakPrint("Error: Can't enable more than MAX_ENABLED_LIGHTS (8) simultaneously!");
#endif

				return D3DERR_INVALIDCALL;
			}
			else
			{
#ifdef _DEBUG
				OutputDebugStringA("Warning: Disabling an already-disabled light");
#endif
				// Do nothing, light is already disabled
				return ret;
			}
		}
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetLightEnable(THIS_ DWORD Index, BOOL* pEnable)
{
	BOOL realEnable = FALSE;
	HRESULT ret = d3d9dev->GetLightEnable(Index, &realEnable);
	if (FAILED(ret) )
		return ret;

	const std::map<UINT, LightInfo*>::const_iterator it = currentState.lightInfoMap->find(Index);
	if (it == currentState.lightInfoMap->end() )
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Light does not exist in light lookup. Call SetLight() first before enabling your light.");
#endif
		return D3DERR_INVALIDCALL;
	}
	else
	{
		const BOOL softwareEnable = it->second->activeLightIndex >= 0 ? TRUE : FALSE;
#ifdef _DEBUG
		if (realEnable != softwareEnable)
		{
			DbgBreakPrint("Error: Internal light enable different than light enable");
		}
#endif
		if (pEnable)
		{
			*pEnable = softwareEnable;
			return ret;
		}
		else
			return D3DERR_INVALIDCALL;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetClipPlane(THIS_ DWORD Index, CONST float* pPlane)
{
	HRESULT ret = d3d9dev->SetClipPlane(Index, pPlane);
	if (FAILED(ret) )
		return ret;

	// The real D3D9 seems to handle clip planes greater than D3DMAXUSERCLIPPLANES by allocating dynamic memory in a tree structure, however
	// most GPU's don't read clip planes beyond the first six anyway, so we don't actually need to keep track of those high-numbered clip planes for rendering.
	if (pPlane && Index < D3DMAXUSERCLIPPLANES)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetMaterial>();
		}
		else
			targetDeviceState = &currentState;

		memcpy(&(targetDeviceState->currentClippingPlanes[Index]), pPlane, sizeof(D3DXPLANE) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetClipPlane(THIS_ DWORD Index, float* pPlane)
{
	D3DXPLANE realPlane(0.0f, 0.0f, 0.0f, 0.0f);
	HRESULT ret = d3d9dev->GetClipPlane(Index, (float* const)&realPlane);
	if (FAILED(ret) )
		return ret;

	if (Index < D3DMAXUSERCLIPPLANES)
	{
#ifdef _DEBUG
		if (memcmp(&realPlane, &currentState.currentClippingPlanes[Index], sizeof(D3DXPLANE) ) != 0)
		{
			DbgBreakPrint("Error: Internal clipping plane different than clipping plane");
		}
#endif

		if (pPlane)
		{
			memcpy(pPlane, &currentState.currentClippingPlanes[Index], sizeof(D3DXPLANE) );
		}
	}
	else
	{
		if (pPlane)
		{
			memset(pPlane, 0, sizeof(D3DXPLANE) );
		}
	}

	return ret;
}

#ifdef _DEBUG
// Note that range checks (less than zero and greater than D3DRS_BLENDOPALPHA) should be performed outside of this function
static inline const bool IsRenderStateValidForD3D9(const D3DRENDERSTATETYPE State)
{
	switch (State)
	{
		case D3DRS_ZENABLE                   :
		case D3DRS_FILLMODE                  :
		case D3DRS_SHADEMODE                 :
		case D3DRS_ZWRITEENABLE              :
		case D3DRS_ALPHATESTENABLE           :
		case D3DRS_LASTPIXEL                 :
		case D3DRS_SRCBLEND                  :
		case D3DRS_DESTBLEND                 :
		case D3DRS_CULLMODE                  :
		case D3DRS_ZFUNC                     :
		case D3DRS_ALPHAREF                  :
		case D3DRS_ALPHAFUNC                 :
		case D3DRS_DITHERENABLE              :
		case D3DRS_ALPHABLENDENABLE          :
		case D3DRS_FOGENABLE                 :
		case D3DRS_SPECULARENABLE            :
		case D3DRS_FOGCOLOR                  :
		case D3DRS_FOGTABLEMODE              :
		case D3DRS_FOGSTART                  :
		case D3DRS_FOGEND                    :
		case D3DRS_FOGDENSITY                :
		case D3DRS_RANGEFOGENABLE            :
		case D3DRS_STENCILENABLE             :
		case D3DRS_STENCILFAIL               :
		case D3DRS_STENCILZFAIL              :
		case D3DRS_STENCILPASS               :
		case D3DRS_STENCILFUNC               :
		case D3DRS_STENCILREF                :
		case D3DRS_STENCILMASK               :
		case D3DRS_STENCILWRITEMASK          :
		case D3DRS_TEXTUREFACTOR             :
		case D3DRS_WRAP0                     :
		case D3DRS_WRAP1                     :
		case D3DRS_WRAP2                     :
		case D3DRS_WRAP3                     :
		case D3DRS_WRAP4                     :
		case D3DRS_WRAP5                     :
		case D3DRS_WRAP6                     :
		case D3DRS_WRAP7                     :
		case D3DRS_CLIPPING                  :
		case D3DRS_LIGHTING                  :
		case D3DRS_AMBIENT                   :
		case D3DRS_FOGVERTEXMODE             :
		case D3DRS_COLORVERTEX               :
		case D3DRS_LOCALVIEWER               :
		case D3DRS_NORMALIZENORMALS          :
		case D3DRS_DIFFUSEMATERIALSOURCE     :
		case D3DRS_SPECULARMATERIALSOURCE    :
		case D3DRS_AMBIENTMATERIALSOURCE     :
		case D3DRS_EMISSIVEMATERIALSOURCE    :
		case D3DRS_VERTEXBLEND               :
		case D3DRS_CLIPPLANEENABLE           :
		case D3DRS_POINTSIZE                 :
		case D3DRS_POINTSIZE_MIN             :
		case D3DRS_POINTSPRITEENABLE         :
		case D3DRS_POINTSCALEENABLE          :
		case D3DRS_POINTSCALE_A              :
		case D3DRS_POINTSCALE_B              :
		case D3DRS_POINTSCALE_C              :
		case D3DRS_MULTISAMPLEANTIALIAS      :
		case D3DRS_MULTISAMPLEMASK           :
		case D3DRS_PATCHEDGESTYLE            :
		case D3DRS_DEBUGMONITORTOKEN         :
		case D3DRS_POINTSIZE_MAX             :
		case D3DRS_INDEXEDVERTEXBLENDENABLE  :
		case D3DRS_COLORWRITEENABLE          :
		case D3DRS_TWEENFACTOR               :
		case D3DRS_BLENDOP                   :
		case D3DRS_POSITIONDEGREE            :
		case D3DRS_NORMALDEGREE              :
		case D3DRS_SCISSORTESTENABLE         :
		case D3DRS_SLOPESCALEDEPTHBIAS       :
		case D3DRS_ANTIALIASEDLINEENABLE     :
		case D3DRS_MINTESSELLATIONLEVEL      :
		case D3DRS_MAXTESSELLATIONLEVEL      :
		case D3DRS_ADAPTIVETESS_X            :
		case D3DRS_ADAPTIVETESS_Y            :
		case D3DRS_ADAPTIVETESS_Z            :
		case D3DRS_ADAPTIVETESS_W            :
		case D3DRS_ENABLEADAPTIVETESSELLATION :
		case D3DRS_TWOSIDEDSTENCILMODE       :
		case D3DRS_CCW_STENCILFAIL           :
		case D3DRS_CCW_STENCILZFAIL          :
		case D3DRS_CCW_STENCILPASS           :
		case D3DRS_CCW_STENCILFUNC           :
		case D3DRS_COLORWRITEENABLE1         :
		case D3DRS_COLORWRITEENABLE2         :
		case D3DRS_COLORWRITEENABLE3         :
		case D3DRS_BLENDFACTOR               :
		case D3DRS_SRGBWRITEENABLE           :
		case D3DRS_DEPTHBIAS                 :
		case D3DRS_WRAP8                     :
		case D3DRS_WRAP9                     :
		case D3DRS_WRAP10                    :
		case D3DRS_WRAP11                    :
		case D3DRS_WRAP12                    :
		case D3DRS_WRAP13                    :
		case D3DRS_WRAP14                    :
		case D3DRS_WRAP15                    :
		case D3DRS_SEPARATEALPHABLENDENABLE  :
		case D3DRS_SRCBLENDALPHA             :
		case D3DRS_DESTBLENDALPHA            :
		case D3DRS_BLENDOPALPHA              :
			return true;
		default:
			return false;
	}
}
#endif // #ifdef _DEBUG

static inline const bool DoesSrcBlendRequireDestData(const D3DBLEND srcBlend)
{
	switch (srcBlend)
	{
	default:
		return false;
	case D3DBLEND_DESTALPHA:
	case D3DBLEND_INVDESTALPHA:
	case D3DBLEND_DESTCOLOR:
	case D3DBLEND_INVDESTCOLOR:
		return true;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetRenderState(THIS_ D3DRENDERSTATETYPE State, DWORD Value)
{
	HRESULT ret = d3d9dev->SetRenderState(State, Value);
	if (FAILED(ret) )
		return ret;

	if (State < 0 || State >= MAX_NUM_RENDERSTATES)
		return D3DERR_INVALIDCALL;

	// Active state block recording redirects state writes into the state block rather than to the real device
	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkRenderStateAsCaptured(State);
	}
	else
		targetDeviceState = &currentState;
	RenderStates& targetRenderStates = targetDeviceState->currentRenderStates;
	RenderStates::_renderStatesUnion& targetRenderStatesUnion = targetRenderStates.renderStatesUnion;

#ifdef _DEBUG
	if (!IsRenderStateValidForD3D9(State) )
	{
		char buffer[256] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(buffer, "Warning: Render state enum is in bounds, but not a recognized render state and will not have any effect. This may be due to driver-specific render-states, or via people using D3D7 or D3D8 render states by accident. Render State Index: %u, Value: %u\n", State, Value);
#pragma warning(pop)
		OutputDebugStringA(buffer);
	}
#endif

	targetRenderStatesUnion.states[State] = Value;

	switch (State)
	{
	case D3DRS_ALPHAREF:
		targetRenderStates.cachedAlphaRefFloat = targetRenderStatesUnion.namedStates.alphaRef / 255.0f;
		targetRenderStates.alphaRefSplatted = _mm_set1_ps(targetRenderStates.cachedAlphaRefFloat);
		break;
	case D3DRS_AMBIENT:
		ColorDWORDToFloat4<0xF>(targetRenderStatesUnion.namedStates.ambient, targetRenderStates.cachedAmbient);
		break;
	case D3DRS_BLENDFACTOR:
		ColorDWORDToFloat4(targetRenderStatesUnion.states[D3DRS_BLENDFACTOR], targetRenderStates.cachedBlendFactor);
		targetRenderStates.cachedInvBlendFactor = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f) - targetRenderStates.cachedBlendFactor;
		break;
	case D3DRS_DEPTHBIAS:
		targetRenderStates.depthBiasSplatted = _mm_set1_ps(targetRenderStatesUnion.namedStates.depthBias);
		break;

	// Alpha blend type caching:
	case D3DRS_ALPHABLENDENABLE:
	case D3DRS_SEPARATEALPHABLENDENABLE:
	case D3DRS_SRCBLEND:
	case D3DRS_DESTBLEND:
		if (targetRenderStatesUnion.namedStates.alphaBlendEnable)
		{
			// Separate alpha blending
			if (targetRenderStatesUnion.namedStates.separateAlphaBlendEnable)
			{
				targetRenderStates.simplifiedAlphaBlendMode = RenderStates::otherAlphaBlending;
				targetRenderStates.alphaBlendNeedsDestRead = true; // TODO: Figure this out for reals rather than defaulting to this conservative "true" value
			}
			// Unified alpha blending
			else
			{
				if (targetRenderStatesUnion.namedStates.srcBlend == D3DBLEND_SRCALPHA && 
					targetRenderStatesUnion.namedStates.destBlend == D3DBLEND_INVSRCALPHA) // TODO: Check the for the less common Min/Max/Reversesubtract blend ops
				{
					targetRenderStates.simplifiedAlphaBlendMode = RenderStates::alphaBlending;
				}
				else if (targetRenderStatesUnion.namedStates.srcBlend == D3DBLEND_ONE &&
						targetRenderStatesUnion.namedStates.destBlend == D3DBLEND_ONE) // TODO: Check the for the less common Min/Max/Reversesubtract blend ops
				{
					targetRenderStates.simplifiedAlphaBlendMode = RenderStates::additiveBlending;
				}
				else if ( (targetRenderStatesUnion.namedStates.srcBlend == D3DBLEND_DESTCOLOR &&
						targetRenderStatesUnion.namedStates.destBlend == D3DBLEND_ZERO) || 
						(targetRenderStatesUnion.namedStates.srcBlend == D3DBLEND_ZERO &&
						targetRenderStatesUnion.namedStates.destBlend == D3DBLEND_SRCCOLOR) ) // TODO: Check the for the less common Min/Max/Reversesubtract blend ops
				{
					targetRenderStates.simplifiedAlphaBlendMode = RenderStates::multiplicativeBlending;
				}
				else
				{
					targetRenderStates.simplifiedAlphaBlendMode = RenderStates::otherAlphaBlending;
				}
			}

			targetRenderStates.alphaBlendNeedsDestRead = (targetRenderStatesUnion.namedStates.destBlend > D3DBLEND_ZERO)
				|| DoesSrcBlendRequireDestData(targetRenderStatesUnion.namedStates.srcBlend);
		}
		else
		{
			targetRenderStates.simplifiedAlphaBlendMode = RenderStates::noAlphaBlending;
			targetRenderStates.alphaBlendNeedsDestRead = false;
		}
		break;
	default: // All other states are fine, we don't need caching for them
		break;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetRenderState(THIS_ D3DRENDERSTATETYPE State, DWORD* pValue)
{
	DWORD stateValue = 0x00000000;
	HRESULT ret = d3d9dev->GetRenderState(State, &stateValue);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (stateValue != currentState.currentRenderStates.renderStatesUnion.states[State])
	{
		DbgBreakPrint("Error: Internal render state different than render state");
	}

	if (IsRenderStateD3D9Deprecated(State) )
	{
		DbgBreakPrint("Warning: Render-state deprecation is supposed to cause GetRenderState() to return D3DERR_INVALIDARG, so we shouldn't make it this far");
	}
#endif

	if (State < 0 || State >= MAX_NUM_RENDERSTATES)
		return D3DERR_INVALIDCALL;

	if (!pValue)
		return D3DERR_INVALIDCALL;

#ifdef _DEBUG
	if (!IsRenderStateValidForD3D9(State) )
	{
		char buffer[256] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(buffer, "Warning: Render state enum is in bounds, but not a recognized render state and will not have any effect. This may be due to driver-specific render-states, or via people using D3D7 or D3D8 render states by accident. Render State Index: %u, Value: %u\n", State, *pValue);
#pragma warning(pop)
		OutputDebugStringA(buffer);
	}
#endif
	*pValue = currentState.currentRenderStates.renderStatesUnion.states[State];
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetClipStatus(THIS_ CONST D3DCLIPSTATUS9* pClipStatus)
{
	// Don't support this
	DbgBreakPrint("Error: SetClipStatus is unsupported");
	HRESULT ret = d3d9dev->SetClipStatus(pClipStatus);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetClipStatus(THIS_ D3DCLIPSTATUS9* pClipStatus)
{
	// Don't support this
	DbgBreakPrint("Error: GetClipStatus is unsupported");
	HRESULT ret = d3d9dev->GetClipStatus(pClipStatus);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetTexture(THIS_ DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
	if (Stage >= 16 && Stage < D3DDMAPSAMPLER)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Texture stage out of addressable bounds");
#endif
		return D3DERR_INVALIDCALL;
	}
	if (Stage >= MAX_NUM_SAMPLERS)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Texture stage out of addressable bounds");
#endif
		return D3DERR_INVALIDCALL;
	}

	LPDIRECT3DBASETEXTURE9 realTexture = NULL;
	HRESULT ret = d3d9dev->GetTexture(Stage, &realTexture);
	if (FAILED(ret) )
		return ret;

	if (!ppTexture)
		return D3DERR_INVALIDCALL;

	if (currentState.currentTextures[Stage])
	{
#ifdef _DEBUG
		if (currentState.currentTextures[Stage]->GetUnderlyingTexture() != realTexture)
		{
			DbgBreakPrint("Error: Internal texture doesn't match texture");
		}
#endif
		*ppTexture = currentState.currentTextures[Stage];
		currentState.currentTextures[Stage]->AddRef();
	}
	else if (currentState.currentCubeTextures[Stage])
	{
#ifdef _DEBUG
		if (currentState.currentCubeTextures[Stage]->GetUnderlyingCubeTexture() != realTexture)
		{
			DbgBreakPrint("Error: Internal cube texture doesn't match cube texture");
		}
#endif
		*ppTexture = currentState.currentCubeTextures[Stage];
		currentState.currentCubeTextures[Stage]->AddRef();
	}
	else if (currentState.currentVolumeTextures[Stage])
	{
#ifdef _DEBUG
		if (currentState.currentVolumeTextures[Stage]->GetUnderlyingVolumeTexture() != realTexture)
		{
			DbgBreakPrint("Error: Internal volume texture doesn't match volume texture");
		}
#endif
		*ppTexture = currentState.currentVolumeTextures[Stage];
		currentState.currentVolumeTextures[Stage]->AddRef();
	}
	else
	{
		*ppTexture = NULL;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetTexture(THIS_ DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
	if (Stage >= 16 && Stage < D3DDMAPSAMPLER)
	{
#ifdef _DEBUG
		__debugbreak(); // This ends up crashing real D3D9, so don't do it!
#endif
		return D3DERR_INVALIDCALL;
	}

	IDirect3DTexture9Hook* textureHookPtr = dynamic_cast<IDirect3DTexture9Hook*>(pTexture);
	IDirect3DCubeTexture9Hook* cubeHookPtr = dynamic_cast<IDirect3DCubeTexture9Hook*>(pTexture);
	IDirect3DVolumeTexture9Hook* volumeHookPtr = dynamic_cast<IDirect3DVolumeTexture9Hook*>(pTexture);
	const IDirect3DBaseTexture9Hook* const baseHookPtr = dynamic_cast<IDirect3DBaseTexture9Hook*>(pTexture);
	if (textureHookPtr)
		pTexture = textureHookPtr->GetUnderlyingTexture();
	else if (cubeHookPtr)
		pTexture = cubeHookPtr->GetUnderlyingCubeTexture();
	else if (volumeHookPtr)
		pTexture = volumeHookPtr->GetUnderlyingVolumeTexture();
	else if (baseHookPtr)
		pTexture = baseHookPtr->GetUnderlyingBaseTexture();
#ifdef _DEBUG
	else if (pTexture != NULL)
	{
		// Unknown texture type passed to SetTexture!
		DbgBreakPrint("Error: Unknown texture type passed to SetTexture (not a 2D texture, volume texture, or cube texture)");
	}
#endif

	HRESULT ret = d3d9dev->SetTexture(Stage, pTexture);
	if (FAILED(ret) )
		return ret;

#ifdef DUMP_TEXTURES_ON_FIRST_SET
	if (textureHookPtr)
	{
		++textureHookPtr->dumped;
		if (textureHookPtr->dumped == 16)
		{
			LPDIRECT3DSURFACE9 surf0 = NULL;
			textureHookPtr->GetSurfaceLevel(0, &surf0);
			IDirect3DSurface9Hook* surfHook = dynamic_cast<IDirect3DSurface9Hook*>(surf0);
			if (!surfHook)
			{
				DbgBreakPrint("Error: Unable to GetSurfaceLevel0 from texture");
			}
			surfHook->DumpSurfaceToDisk();
		}
	}
#endif

	if (Stage >= ARRAYSIZE(currentState.currentTextures) )
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Texture index out of bounds");
#endif
		return D3DERR_INVALIDCALL;
	}

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetTextureCaptured(Stage);
	}
	else
		targetDeviceState = &currentState;

	if (textureHookPtr)
	{
		targetDeviceState->currentTextures[Stage] = textureHookPtr;
		targetDeviceState->currentCubeTextures[Stage] = NULL;
		targetDeviceState->currentVolumeTextures[Stage] = NULL;
	}
	else if (cubeHookPtr)
	{
		targetDeviceState->currentTextures[Stage] = NULL;
		targetDeviceState->currentCubeTextures[Stage] = cubeHookPtr;
		targetDeviceState->currentVolumeTextures[Stage] = NULL;
	}
	else if (volumeHookPtr)
	{
		targetDeviceState->currentTextures[Stage] = NULL;
		targetDeviceState->currentCubeTextures[Stage] = NULL;
		targetDeviceState->currentVolumeTextures[Stage] = volumeHookPtr;
	}
	else
	{
		targetDeviceState->currentTextures[Stage] = NULL;
		targetDeviceState->currentCubeTextures[Stage] = NULL;
		targetDeviceState->currentVolumeTextures[Stage] = NULL;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetTextureStageState(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
	DWORD realValue = 0x00000000;
	HRESULT ret = d3d9dev->GetTextureStageState(Stage, Type, &realValue);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (realValue != currentState.currentStageStates[Stage].stageStateUnion.state[Type])
	{
		DbgBreakPrint("Error: Internal texture stage state doesn't match texture stage state");
	}
#endif

	if (Stage >= MAX_NUM_TEXTURE_STAGE_STATES)
		return D3DERR_INVALIDCALL;

	if (Type < D3DTSS_COLOROP || Type > D3DTSS_CONSTANT)
		return D3DERR_INVALIDCALL;
	
	if (pValue)
	{
		*pValue = currentState.currentStageStates[Stage].stageStateUnion.state[Type];
		return ret;
	}
	else
		return D3DERR_INVALIDCALL;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetTextureStageState(THIS_ DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
	HRESULT ret = d3d9dev->SetTextureStageState(Stage, Type, Value);
	if (FAILED(ret) )
		return ret;

	if (Stage >= MAX_NUM_TEXTURE_STAGE_STATES)
		return D3DERR_INVALIDCALL;

	if (Type < D3DTSS_COLOROP || Type > D3DTSS_CONSTANT)
		return D3DERR_INVALIDCALL;

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetTextureStageStateCaptured(Stage, Type);
	}
	else
		targetDeviceState = &currentState;

	targetDeviceState->currentStageStates[Stage].stageStateUnion.state[Type] = Value;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetSamplerState(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
	DWORD outVal = 0x00000000;
	HRESULT ret = d3d9dev->GetSamplerState(Sampler, Type, &outVal);
	if (FAILED(ret) )
		return ret;

	if (Type < D3DSAMP_ADDRESSU || Type > D3DSAMP_DMAPOFFSET)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Type is out of bounds");
#endif
		return D3DERR_INVALIDCALL;
	}

#ifdef _DEBUG
	if (currentState.currentSamplerStates[Sampler].stateUnion.state[Type] != outVal)
	{
		DbgBreakPrint("Error: Internal sampler state doesn't match sampler state");
	}
#endif

	if (pValue)
	{
		*pValue = currentState.currentSamplerStates[Sampler].stateUnion.state[Type];
		return ret;
	}
	else
		return D3DERR_INVALIDCALL;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetSamplerState(THIS_ DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
	HRESULT ret = d3d9dev->SetSamplerState(Sampler, Type, Value);
	if (FAILED(ret) )
		return ret;

	if (Type < D3DSAMP_ADDRESSU)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Sampler state type out of bounds (0 is not a valid sampler state enum)");
#endif
		return D3DERR_INVALIDCALL;
	}
	else if (Type > D3DSAMP_DMAPOFFSET)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Sampler state type out of bounds");
#endif
		return D3DERR_INVALIDCALL;
	}

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetSamplerStateCaptured(Sampler, Type);
	}
	else
		targetDeviceState = &currentState;

	SamplerState& thisSampler = targetDeviceState->currentSamplerStates[Sampler];
	thisSampler.stateUnion.state[Type] = Value;

	if (Type == D3DSAMP_MAXMIPLEVEL)
	{
		thisSampler.cachedFloatMaxMipLevel = (const float)Value;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetPaletteEntries(THIS_ UINT PaletteNumber, CONST PALETTEENTRY* pEntries)
{
	HRESULT ret = d3d9dev->SetPaletteEntries(PaletteNumber, pEntries);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (PaletteNumber > 65535)
	{
		// Spec says the limit is 64k on PaletteNumbers
		DbgBreakPrint("Error: Palette number out of bounds");
	}
#endif

	if (PaletteNumber >= currentState.currentPaletteState.paletteEntries->size() )
	{
		currentState.currentPaletteState.paletteEntries->resize(PaletteNumber + 1);
	}

	if (pEntries != NULL)
	{
		memcpy( (*currentState.currentPaletteState.paletteEntries)[PaletteNumber].entries, pEntries, sizeof(TexturePaletteEntry) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetPaletteEntries(THIS_ UINT PaletteNumber, PALETTEENTRY* pEntries)
{
	HRESULT ret = d3d9dev->GetPaletteEntries(PaletteNumber, pEntries);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (PaletteNumber > 65535)
	{
		// Spec says the limit is 64k on PaletteNumbers
		DbgBreakPrint("Error: Palette number out of bounds");
	}
#endif

	if (PaletteNumber >= currentState.currentPaletteState.paletteEntries->size() )
	{
		currentState.currentPaletteState.paletteEntries->resize(PaletteNumber + 1);
	}

#ifdef _DEBUG
	if (pEntries != NULL)
	{
		if (memcmp(pEntries, (*currentState.currentPaletteState.paletteEntries)[PaletteNumber].entries, sizeof(TexturePaletteEntry) ) != 0)
		{
			DbgBreakPrint("Error: Internal palette doesn't match palette");
		}
	}
#endif

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetCurrentTexturePalette(THIS_ UINT PaletteNumber)
{
	HRESULT ret = d3d9dev->SetCurrentTexturePalette(PaletteNumber);
	if (FAILED(ret) )
		return ret;

	if (PaletteNumber > 0xFFFF)
	{
#ifdef _DEBUG
		__debugbreak(); // Error: Palette index out of bounds!
#endif
		return D3DERR_INVALIDCALL;
	}

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetCurrentTexturePalette>();
	}
	else
		targetDeviceState = &currentState;

	targetDeviceState->currentPaletteState.currentPaletteIndex = (const unsigned short)PaletteNumber;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetCurrentTexturePalette(THIS_ UINT *PaletteNumber)
{
	HRESULT ret = d3d9dev->GetCurrentTexturePalette(PaletteNumber);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (PaletteNumber != NULL)
	{
		if (*PaletteNumber != currentState.currentPaletteState.currentPaletteIndex)
		{
			DbgBreakPrint("Error: Internal palette doesn't match palette");
		}
	}
#endif

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetScissorRect(THIS_ CONST RECT* pRect)
{
	HRESULT ret = d3d9dev->SetScissorRect(pRect);
	if (FAILED(ret) )
		return ret;

	// Real D3D9 crashes if you try to call SetScissorRect() with a NULL pointer, so we can do pretty much whatever we want in this case
	if (pRect != NULL)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetScissorRect>();
		}
		else
			targetDeviceState = &currentState;

		targetDeviceState->currentScissorRect.scissorRect = *pRect;
		targetDeviceState->currentScissorRect.RecomputeScissorRect();
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetScissorRect(THIS_ RECT* pRect)
{
	HRESULT ret = d3d9dev->GetScissorRect(pRect);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (pRect)
	{
		if (memcmp(pRect, &currentState.currentScissorRect.scissorRect, sizeof(RECT) ) != 0)
		{
			DbgBreakPrint("Error: Internal scissor rect doesn't match scissor rect");
		}
	}
#endif

	return ret;
}

// One interesting note about SetSoftwareVertexProcessing is that the documentation explicitly calls out that because this used to be a render-state back in the D3D8 days, in
// D3D9 this API is *not* captured by state-blocks on purpose.
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetSoftwareVertexProcessing(THIS_ BOOL bSoftware)
{
	HRESULT ret = d3d9dev->SetSoftwareVertexProcessing(bSoftware);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (initialCreateFlags & D3DCREATE_MIXED_VERTEXPROCESSING)
	{
	}
	else
	{
		// Spec says that it's illegal to call SetSoftwareVertexProcessing() on a non-mixed-mode device: https://msdn.microsoft.com/en-us/library/windows/desktop/bb174458(v=vs.85).aspx
		DbgBreakPrint("Error: Spec says that it's illegal to call SetSoftwareVertexProcessing() on a non-mixed-mode device");
	}

	// Spec says it's illegal to call SetSoftwareVertexProcessing() from within a BeginScene()/EndScene() block
	if (HasBegunScene() )
	{
		DbgBreakPrint("Error: Spec says it's illegal to call SetSoftwareVertexProcessing() from within a BeginScene()/EndScene() block");
	}
#endif

	currentSwvpEnabled = bSoftware;

	return ret;
}

COM_DECLSPEC_NOTHROW BOOL STDMETHODCALLTYPE IDirect3DDevice9Hook::GetSoftwareVertexProcessing(THIS)
{
#ifdef _DEBUG
	BOOL realRet = d3d9dev->GetSoftwareVertexProcessing();
	if (realRet != currentSwvpEnabled)
	{
		DbgBreakPrint("Error: Internal SWVP doesn't match SWVP");
	}
#endif

	return currentSwvpEnabled;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetNPatchMode(THIS_ float nSegments)
{
	HRESULT ret = d3d9dev->SetNPatchMode(nSegments);
	if (FAILED(ret) )
		return ret;

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetNPatchMode>();
	}
	else
		targetDeviceState = &currentState;

	targetDeviceState->currentNPatchMode = nSegments;

	return ret;
}

COM_DECLSPEC_NOTHROW float STDMETHODCALLTYPE IDirect3DDevice9Hook::GetNPatchMode(THIS)
{
#ifdef _DEBUG
	const float realNPatchMode = d3d9dev->GetNPatchMode();
	if (realNPatchMode != currentState.currentNPatchMode)
	{
		// Real device divergence detected!
		__debugbreak();
	}
#endif
	return currentState.currentNPatchMode;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetVertexDeclaration(THIS_ IDirect3DVertexDeclaration9* pDecl)
{
	IDirect3DVertexDeclaration9Hook* hook = dynamic_cast<IDirect3DVertexDeclaration9Hook*>(pDecl);
	if (hook)
		pDecl = hook->GetUnderlyingVertexDeclaration();
#ifdef _DEBUG
	else if (pDecl != NULL)
	{
		DbgBreakPrint("Error: Vertex declaration is not hooked");
	}
#endif

	HRESULT ret = d3d9dev->SetVertexDeclaration(pDecl);
	if (FAILED(ret) )
		return ret;

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetVertexDeclaration>();
	}
	else
		targetDeviceState = &currentState;

	if (targetDeviceState->currentVertexDecl != NULL)
	{
		// targetDeviceState->currentVertexDecl->Release();
	}

	const IDirect3DVertexDeclaration9Hook* const oldVertDecl = targetDeviceState->currentVertexDecl;
	if (oldVertDecl != hook)
	{
		// Set dirty flags on stream-ends for used streams for this new decl:
		if (hook != NULL)
		{
			const std::vector<DebuggableD3DVERTEXELEMENT9>& elements = hook->GetElementsInternal();
			const unsigned numElements = elements.size() - 1; // Minus one here because we want to ignore the D3DDECL_END element
			for (unsigned x = 0; x < numElements; ++x)
			{
				const DebuggableD3DVERTEXELEMENT9& thisElement = elements[x];
				targetDeviceState->currentStreamEnds[thisElement.Stream].SetDirty();
			}
		}
	}

	targetDeviceState->currentVertexDecl = hook;

	if (pDecl)
	{
		targetDeviceState->declTarget = DeviceState::targetVertexDecl;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetVertexDeclaration(THIS_ IDirect3DVertexDeclaration9** ppDecl)
{
	LPDIRECT3DVERTEXDECLARATION9 realVertDecl = NULL;
	HRESULT ret = d3d9dev->GetVertexDeclaration(&realVertDecl);
	if (FAILED(ret) )
		return ret;

	if (!ppDecl)
		return D3DERR_INVALIDCALL;

#ifdef _DEBUG
	if (currentState.declTarget == DeviceState::targetFVF)
	{
		// TODO: Implement this case of a FVF auto-converted to a valid vertex decl!
		DbgBreakPrint("Error: Automatic up-conversion of FVF code to vertex declaration is not yet supported");
	}

	if ( (realVertDecl != NULL) != (currentState.currentVertexDecl != NULL) )
	{
		DbgBreakPrint("Error: Internal vertex declaration doesn't match vertex declaration (one is NULL, one is non-NULL)");
	}
	if (realVertDecl != currentState.currentVertexDecl->GetUnderlyingVertexDeclaration() )
	{
		DbgBreakPrint("Error: Internal vertex declaration doesn't match vertex declaration");
	}
#endif
	*ppDecl = currentState.currentVertexDecl;
	if (currentState.currentVertexDecl)
		currentState.currentVertexDecl->AddRef();

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetFVF(THIS_ DWORD FVF)
{
	debuggableFVF dbgFVF;
	dbgFVF.rawFVF_DWORD = FVF;

	HRESULT ret = d3d9dev->SetFVF(dbgFVF.rawFVF_DWORD);
	if (FAILED(ret) )
		return ret;

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetFVF>();
	}
	else
		targetDeviceState = &currentState;

	const DWORD oldFVF_DWORD = targetDeviceState->currentFVF.rawFVF_DWORD;
	targetDeviceState->currentFVF.rawFVF_DWORD = dbgFVF.rawFVF_DWORD;

	if (dbgFVF.rawFVF_DWORD != 0x00000000)
	{
		// Skip setting the same FVF/decl twice
		if (targetDeviceState->declTarget == DeviceState::targetFVF && oldFVF_DWORD == dbgFVF.rawFVF_DWORD)
			return ret;

		targetDeviceState->declTarget = DeviceState::targetFVF;

		// D3D9 does some weird stuff under the hood if you call SetFVF with a valid FVF-code. It needs to:
		// 1) Release any existing set vertex declarations
		// 2) Create a new implicit vertex declaration (retrievable only via GetVertexDeclaration() )
		// 3) Assign that new implicit vertex declaration
		CreateVertexDeclFromFVFCode(dbgFVF);
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetFVF(THIS_ DWORD* pFVF)
{
	if (!pFVF)
		return D3DERR_INVALIDCALL;

	*pFVF = currentState.currentFVF.rawFVF_DWORD;

#ifdef _DEBUG
	debuggableFVF dbgFVF;
	dbgFVF.rawFVF_DWORD = 0x00000000;
	HRESULT ret = d3d9dev->GetFVF(&dbgFVF.rawFVF_DWORD);
	if (FAILED(ret) )
	{
		__debugbreak(); // We missed an error case!
	}
	if (dbgFVF.rawFVF_DWORD != currentState.currentFVF.rawFVF_DWORD)
	{
		DbgBreakPrint("Error: Internal FVF doesn't match FVF");
	}
	return ret;
#else // #ifdef _DEBUG
	return S_OK;
#endif // #ifdef _DEBUG
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetVertexShader(THIS_ IDirect3DVertexShader9* pShader)
{
	IDirect3DVertexShader9Hook* hookPtr = dynamic_cast<IDirect3DVertexShader9Hook*>(pShader);
	if (hookPtr)
		pShader = hookPtr->GetUnderlyingVertexShader();
#ifdef _DEBUG
	else if (pShader != NULL)
	{
		DbgBreakPrint("Error: Vertex shader is not hooked");
	}
#endif

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetVertexShader>();
	}
	else
		targetDeviceState = &currentState;

	if (targetDeviceState->currentVertexShader != NULL)
	{
		// targetDeviceState->currentVertexShader->Release();
	}

	targetDeviceState->currentVertexShader = hookPtr;

	if (hookPtr)
	{
		if (!hookPtr->triedJit)
		{
			hookPtr->JitLoadShader();
		}
	}

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetVertexShader(THIS_ IDirect3DVertexShader9** ppShader)
{
	LPDIRECT3DVERTEXSHADER9 realShader = NULL;
	HRESULT ret = d3d9dev->GetVertexShader(&realShader);
	if (FAILED(ret) )
		return ret;

	if (!ppShader)
		return D3DERR_INVALIDCALL;

#ifdef _DEBUG
	if ( (realShader != NULL) != (currentState.currentVertexShader != NULL) )
	{
		DbgBreakPrint("Error: Internal vertex shader doesn't match vertex shader (one is non-NULL and the other is NULL)");
	}
	if (realShader != NULL)
	{
		if (realShader != currentState.currentVertexShader->GetUnderlyingVertexShader() )
		{
			DbgBreakPrint("Error: Internal vertex shader doesn't match vertex shader");
		}
	}
#endif

	*ppShader = currentState.currentVertexShader;
	if (currentState.currentVertexShader)
		currentState.currentVertexShader->AddRef();

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetVertexShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
	HRESULT ret = d3d9dev->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.vertexShaderRegisters.floats) )
	{
		DbgBreakPrint("Error: Vertex shader constant index out of range");
	}
	if (StartRegister + Vector4fCount > ARRAYSIZE(currentState.vertexShaderRegisters.floats) )
	{
		DbgBreakPrint("Error: Vertex shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			const unsigned finalSetRegister = StartRegister + Vector4fCount;
			for (unsigned constantIndex = StartRegister; constantIndex < finalSetRegister; ++constantIndex)
				currentlyRecordingStateBlock->MarkSetVertexShaderConstantF(constantIndex);
		}
		else
			targetDeviceState = &currentState;

		memcpy(targetDeviceState->vertexShaderRegisters.floats + StartRegister, pConstantData, Vector4fCount * sizeof(float4) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetVertexShaderConstantF(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
	HRESULT ret = d3d9dev->GetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.vertexShaderRegisters.floats) )
	{
		DbgBreakPrint("Error: Vertex shader constant index out of range");
	}
	if (StartRegister + Vector4fCount > ARRAYSIZE(currentState.vertexShaderRegisters.floats) )
	{
		DbgBreakPrint("Error: Vertex shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
#ifdef _DEBUG
		if (memcmp(currentState.vertexShaderRegisters.floats + StartRegister, pConstantData, Vector4fCount * sizeof(float4) ) != 0)
		{
			DbgBreakPrint("Error: Internal vertex shader constant doesn't match vertex shader constant");
		}
#endif
		memcpy(pConstantData, currentState.vertexShaderRegisters.floats + StartRegister, Vector4fCount * sizeof(float4) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetVertexShaderConstantI(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
	HRESULT ret = d3d9dev->SetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.vertexShaderRegisters.ints) )
	{
		DbgBreakPrint("Error: Vertex shader constant index out of range");
	}
	if (StartRegister + Vector4iCount > ARRAYSIZE(currentState.vertexShaderRegisters.ints) )
	{
		DbgBreakPrint("Error: Vertex shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			const unsigned finalSetRegister = StartRegister + Vector4iCount;
			for (unsigned constantIndex = StartRegister; constantIndex < finalSetRegister; ++constantIndex)
				currentlyRecordingStateBlock->MarkSetVertexShaderConstantI(constantIndex);
		}
		else
			targetDeviceState = &currentState;

		memcpy(targetDeviceState->vertexShaderRegisters.ints + StartRegister, pConstantData, Vector4iCount * sizeof(int4) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetVertexShaderConstantI(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
	HRESULT ret = d3d9dev->GetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.vertexShaderRegisters.ints) )
	{
		DbgBreakPrint("Error: Vertex shader constant index out of range");
	}
	if (StartRegister + Vector4iCount > ARRAYSIZE(currentState.vertexShaderRegisters.ints) )
	{
		DbgBreakPrint("Error: Vertex shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
#ifdef _DEBUG
		if (memcmp(currentState.vertexShaderRegisters.ints + StartRegister, pConstantData, Vector4iCount * sizeof(int4) ) != 0)
		{
			DbgBreakPrint("Error: Internal vertex shader constant doesn't match vertex shader constant");
		}
#endif
		memcpy(pConstantData, currentState.vertexShaderRegisters.ints + StartRegister, Vector4iCount * sizeof(int4) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetVertexShaderConstantB(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
	HRESULT ret = d3d9dev->SetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.vertexShaderRegisters.bools) )
	{
		DbgBreakPrint("Error: Vertex shader constant index out of range");
	}
	if (StartRegister + BoolCount > ARRAYSIZE(currentState.vertexShaderRegisters.bools) )
	{
		DbgBreakPrint("Error: Vertex shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			const unsigned finalSetRegister = StartRegister + BoolCount;
			for (unsigned constantIndex = StartRegister; constantIndex < finalSetRegister; ++constantIndex)
				currentlyRecordingStateBlock->MarkSetVertexShaderConstantB(constantIndex);
		}
		else
			targetDeviceState = &currentState;

		memcpy(targetDeviceState->vertexShaderRegisters.bools + StartRegister, pConstantData, BoolCount * sizeof(BOOL) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetVertexShaderConstantB(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
	HRESULT ret = d3d9dev->GetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.vertexShaderRegisters.bools) )
	{
		DbgBreakPrint("Error: Vertex shader constant index out of range");
	}
	if (StartRegister + BoolCount > ARRAYSIZE(currentState.vertexShaderRegisters.bools) )
	{
		DbgBreakPrint("Error: Vertex shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
#ifdef _DEBUG
		if (memcmp(currentState.vertexShaderRegisters.bools + StartRegister, pConstantData, BoolCount * sizeof(BOOL) ) != 0)
		{
			DbgBreakPrint("Error: Internal vertex shader constant doesn't match vertex shader constant");
		}
#endif
		memcpy(pConstantData, currentState.vertexShaderRegisters.bools + StartRegister, BoolCount * sizeof(BOOL) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
	if (StreamNumber >= MAX_D3D9_STREAMS)
		return D3DERR_INVALIDCALL;

	IDirect3DVertexBuffer9Hook* hookPtr = dynamic_cast<IDirect3DVertexBuffer9Hook*>(pStreamData);
	if (hookPtr)
		pStreamData = hookPtr->GetUnderlyingVertexBuffer();
#ifdef _DEBUG
	else if (pStreamData != NULL)
	{
		DbgBreakPrint("Error: Vertex buffer is not hooked!");
	}
#endif
	// We need to stuff our stream stride into a uint16, so we can't have larger strides than that
	if (Stride > 0xFFFF)
		return D3DERR_INVALIDCALL;

	HRESULT ret = d3d9dev->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride);
	if (FAILED(ret) )
		return ret;

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetStreamSourceCaptured(StreamNumber);
	}
	else
		targetDeviceState = &currentState;

	StreamSource& thisStreamSource = targetDeviceState->currentStreams[StreamNumber];

	if (thisStreamSource.vertexBuffer != NULL)
	{
		// thisStreamSource.vertexBuffer->Release();
	}

	const IDirect3DVertexBuffer9Hook* const oldVertexBuffer = thisStreamSource.vertexBuffer;

	thisStreamSource.vertexBuffer = hookPtr;
	thisStreamSource.streamOffset = OffsetInBytes;
	thisStreamSource.streamStride = (const unsigned short)Stride;

	if (oldVertexBuffer != hookPtr)
	{
		targetDeviceState->currentStreamEnds[StreamNumber].SetDirty();
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride)
{
	if (StreamNumber >= MAX_D3D9_STREAMS)
		return D3DERR_INVALIDCALL;

	LPDIRECT3DVERTEXBUFFER9 realStream = NULL;
	HRESULT ret = d3d9dev->GetStreamSource(StreamNumber, &realStream, pOffsetInBytes, pStride);
	if (FAILED(ret) )
		return ret;

	const StreamSource& currentStream = currentState.currentStreams[StreamNumber];

#ifdef _DEBUG
	if ( (realStream != NULL) != (currentStream.vertexBuffer != NULL) )
	{
		DbgBreakPrint("Error: Internal Vertex stream source doesn't match stream source (one is non-NULL and the other is NULL)");
	}
	if (realStream != currentStream.vertexBuffer->GetUnderlyingVertexBuffer() )
	{
		DbgBreakPrint("Error: Internal Vertex stream source doesn't match stream source");
	}
	if (*pOffsetInBytes != currentStream.streamOffset)
	{
		DbgBreakPrint("Error: Internal Vertex stream source doesn't match stream source offset");
	}
	if (*pStride != currentStream.streamStride)
	{
		DbgBreakPrint("Error: Internal Vertex stream source doesn't match stream source stride");
	}
#endif
	if (ppStreamData)
	{
		*ppStreamData = currentStream.vertexBuffer;
		if (currentStream.vertexBuffer)
			currentStream.vertexBuffer->AddRef();
	}

	if (pOffsetInBytes)
		*pOffsetInBytes = currentStream.streamOffset;

	if (pStride)
		*pStride = currentStream.streamStride;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetStreamSourceFreq(THIS_ UINT StreamNumber, UINT Setting)
{
	if (StreamNumber >= MAX_D3D9_STREAMS)
		return D3DERR_INVALIDCALL;

	HRESULT ret = d3d9dev->SetStreamSourceFreq(StreamNumber, Setting);
	if (FAILED(ret) )
		return ret;

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetStreamSourceFreqCaptured(StreamNumber);
	}
	else
		targetDeviceState = &currentState;

	targetDeviceState->currentStreams[StreamNumber].streamDividerFrequency = Setting;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetStreamSourceFreq(THIS_ UINT StreamNumber, UINT* pSetting)
{
	HRESULT ret = d3d9dev->GetStreamSourceFreq(StreamNumber, pSetting);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (*pSetting != currentState.currentStreams[StreamNumber].streamDividerFrequency)
	{
		DbgBreakPrint("Error: Vertex stream source freq doesn't match stream source freq");
	}
#endif

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetIndices(THIS_ IDirect3DIndexBuffer9* pIndexData)
{
	IDirect3DIndexBuffer9Hook* hook = dynamic_cast<IDirect3DIndexBuffer9Hook*>(pIndexData);
	if (hook)
		pIndexData = hook->GetUnderlyingIndexBuffer();
#ifdef _DEBUG
	else if (pIndexData != NULL)
	{
		DbgBreakPrint("Error: Index buffer is not hooked!");
	}
#endif

	if (pIndexData == NULL)
	{
		return D3DERR_INVALIDCALL;
	}

	if (currentState.currentIndexBuffer != NULL)
	{
		// currentState.currentIndexBuffer->Release();
	}

	// Active state block recording redirects state writes into the state block rather than to the real device
	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetIndices>();
	}
	else
		targetDeviceState = &currentState;

	targetDeviceState->currentIndexBuffer = hook;

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetIndices(THIS_ IDirect3DIndexBuffer9** ppIndexData)
{
	LPDIRECT3DINDEXBUFFER9 realIndices = NULL;
	HRESULT ret = d3d9dev->GetIndices(&realIndices);
	if (FAILED(ret) )
		return ret;

	if (!ppIndexData)
		return D3DERR_INVALIDCALL;

#ifdef _DEBUG
	if ( (realIndices != NULL) != (currentState.currentIndexBuffer != NULL) )
	{
		DbgBreakPrint("Error: Internal indices don't match indices (one is non-NULL and the other is NULL)");
	}
	if (realIndices != currentState.currentIndexBuffer->GetUnderlyingIndexBuffer() )
	{
		DbgBreakPrint("Error: Internal indices don't match indices");
	}
#endif
	*ppIndexData = currentState.currentIndexBuffer;
	if (currentState.currentIndexBuffer)
		currentState.currentIndexBuffer->AddRef();

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetPixelShader(THIS_ IDirect3DPixelShader9* pShader)
{
	IDirect3DPixelShader9Hook* hookPtr = dynamic_cast<IDirect3DPixelShader9Hook*>(pShader);
	if (hookPtr)
		pShader = hookPtr->GetUnderlyingPixelShader();
#ifdef _DEBUG
	else if (pShader != NULL)
	{
		DbgBreakPrint("Error: Pixel shader is not hooked");
	}
#endif

	DeviceState* targetDeviceState;
	if (IsCurrentlyRecordingStateBlock() )
	{
		targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
		currentlyRecordingStateBlock->MarkSetCallAsCaptured<SBT_SetPixelShader>();
	}
	else
		targetDeviceState = &currentState;

	if (targetDeviceState->currentPixelShader != NULL)
	{
		// targetDeviceState->currentPixelShader->Release();
	}

	targetDeviceState->currentPixelShader = hookPtr;

	if (hookPtr)
	{
		if (!hookPtr->triedJit)
		{
			hookPtr->JitLoadShader();
		}
	}

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetPixelShader(THIS_ IDirect3DPixelShader9** ppShader)
{
	LPDIRECT3DPIXELSHADER9 realShader = NULL;
	HRESULT ret = d3d9dev->GetPixelShader(&realShader);
	if (FAILED(ret) )
		return ret;

	if (!ppShader)
		return D3DERR_INVALIDCALL;

#ifdef _DEBUG
	if ( (realShader != NULL) != (currentState.currentPixelShader != NULL) )
	{
		DbgBreakPrint("Error: Internal pixel shader doesn't match pixel shader (one is non-NULL and the other is NULL)");
	}
	if (realShader != NULL)
	{
		if (realShader != currentState.currentPixelShader->GetUnderlyingPixelShader() )
		{
			DbgBreakPrint("Error: Internal pixel shader doesn't match pixel shader");
		}
	}
#endif

	*ppShader = currentState.currentPixelShader;
	if (currentState.currentPixelShader)
		currentState.currentPixelShader->AddRef();

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetPixelShaderConstantF(THIS_ UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
	HRESULT ret = d3d9dev->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.pixelShaderRegisters.floats) )
	{
		DbgBreakPrint("Error: Pixel shader constant index out of range");
	}
	if (StartRegister + Vector4fCount > ARRAYSIZE(currentState.pixelShaderRegisters.floats) )
	{
		DbgBreakPrint("Error: Pixel shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			const unsigned finalSetRegister = StartRegister + Vector4fCount;
			for (unsigned constantIndex = StartRegister; constantIndex < finalSetRegister; ++constantIndex)
				currentlyRecordingStateBlock->MarkSetPixelShaderConstantF(constantIndex);
		}
		else
			targetDeviceState = &currentState;

		memcpy(targetDeviceState->pixelShaderRegisters.floats + StartRegister, pConstantData, Vector4fCount * sizeof(float4) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetPixelShaderConstantF(THIS_ UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
	HRESULT ret = d3d9dev->GetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.pixelShaderRegisters.floats) )
	{
		DbgBreakPrint("Error: Pixel shader constant index out of range");
	}
	if (StartRegister + Vector4fCount > ARRAYSIZE(currentState.pixelShaderRegisters.floats) )
	{
		DbgBreakPrint("Error: Pixel shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
#ifdef _DEBUG
		if (memcmp(currentState.pixelShaderRegisters.floats + StartRegister, pConstantData, Vector4fCount * sizeof(float4) ) != 0)
		{
			DbgBreakPrint("Error: Internal pixel shader constant doesn't match pixel shader constant");
		}
#endif
		memcpy(pConstantData, currentState.pixelShaderRegisters.floats + StartRegister, Vector4fCount * sizeof(float4) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetPixelShaderConstantI(THIS_ UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
	HRESULT ret = d3d9dev->SetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.pixelShaderRegisters.ints) )
	{
		DbgBreakPrint("Error: Pixel shader constant index out of range");
	}
	if (StartRegister + Vector4iCount > ARRAYSIZE(currentState.pixelShaderRegisters.ints) )
	{
		DbgBreakPrint("Error: Pixel shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			const unsigned finalSetRegister = StartRegister + Vector4iCount;
			for (unsigned constantIndex = StartRegister; constantIndex < finalSetRegister; ++constantIndex)
				currentlyRecordingStateBlock->MarkSetPixelShaderConstantI(constantIndex);
		}
		else
			targetDeviceState = &currentState;

		memcpy(targetDeviceState->pixelShaderRegisters.ints + StartRegister, pConstantData, Vector4iCount * sizeof(int4) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetPixelShaderConstantI(THIS_ UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
	HRESULT ret = d3d9dev->GetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.pixelShaderRegisters.ints) )
	{
		DbgBreakPrint("Error: Pixel shader constant index out of range");
	}
	if (StartRegister + Vector4iCount > ARRAYSIZE(currentState.pixelShaderRegisters.ints) )
	{
		DbgBreakPrint("Error: Pixel shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
#ifdef _DEBUG
		if (memcmp(currentState.pixelShaderRegisters.ints + StartRegister, pConstantData, Vector4iCount * sizeof(int4) ) != 0)
		{
			DbgBreakPrint("Error: Internal pixel shader constant doesn't match pixel shader constant");
		}
#endif
		memcpy(pConstantData, currentState.pixelShaderRegisters.ints + StartRegister, Vector4iCount * sizeof(int4) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetPixelShaderConstantB(THIS_ UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
	HRESULT ret = d3d9dev->SetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.pixelShaderRegisters.bools) )
	{
		DbgBreakPrint("Error: Pixel shader constant index out of range");
	}
	if (StartRegister + BoolCount > ARRAYSIZE(currentState.pixelShaderRegisters.bools) )
	{
		DbgBreakPrint("Error: Pixel shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
		DeviceState* targetDeviceState;
		if (IsCurrentlyRecordingStateBlock() )
		{
			targetDeviceState = currentlyRecordingStateBlock->GetDeviceStateForWrite();
			const unsigned finalSetRegister = StartRegister + BoolCount;
			for (unsigned constantIndex = StartRegister; constantIndex < finalSetRegister; ++constantIndex)
				currentlyRecordingStateBlock->MarkSetPixelShaderConstantB(constantIndex);
		}
		else
			targetDeviceState = &currentState;

		memcpy(targetDeviceState->pixelShaderRegisters.bools + StartRegister, pConstantData, BoolCount * sizeof(BOOL) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::GetPixelShaderConstantB(THIS_ UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
	HRESULT ret = d3d9dev->GetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	if (StartRegister > ARRAYSIZE(currentState.pixelShaderRegisters.bools) )
	{
		DbgBreakPrint("Error: Pixel shader constant index out of range");
	}
	if (StartRegister + BoolCount > ARRAYSIZE(currentState.pixelShaderRegisters.bools) )
	{
		DbgBreakPrint("Error: Pixel shader constant range extends out of range");
	}
#endif

	if (pConstantData != NULL)
	{
#ifdef _DEBUG
		if (memcmp(currentState.pixelShaderRegisters.bools + StartRegister, pConstantData, BoolCount * sizeof(BOOL) ) != 0)
		{
			DbgBreakPrint("Error: Internal pixel shader constant doesn't match pixel shader constant");
		}
#endif
		memcpy(pConstantData, currentState.pixelShaderRegisters.bools + StartRegister, BoolCount * sizeof(BOOL) );
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::SetRenderTarget(THIS_ DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
	IDirect3DSurface9Hook* hookPtr = dynamic_cast<IDirect3DSurface9Hook*>(pRenderTarget);
	if (hookPtr)
		pRenderTarget = hookPtr->GetUnderlyingSurface();
#ifdef _DEBUG
	else if (pRenderTarget != NULL)
	{
		DbgBreakPrint("Error: SetRenderTarget called with a non-hooked surface pointer");
	}
#endif
	HRESULT ret = d3d9dev->SetRenderTarget(RenderTargetIndex, pRenderTarget);
	if (FAILED(ret) )
		return ret;

#ifdef _DEBUG
	// Illegal to set render target index 0 to NULL, it must always be pointing to a valid surface
	if (RenderTargetIndex == 0 && !hookPtr)
	{
		DbgBreakPrint("Error: Illegal to set render target 0 to NULL");
	}
#endif

	if (RenderTargetIndex < D3D_MAX_SIMULTANEOUS_RENDERTARGETS)
		currentState.currentRenderTargets[RenderTargetIndex] = hookPtr;
#ifdef _DEBUG
	else
	{
		DbgBreakPrint("Error: Render target index out of range");
	}
#endif

	if (hookPtr)
	{
		hookPtr->AddRef();

		// In the case of a successful render-target set, the viewport is automatically resized to the
		// size of the largest set render-target:
		AutoResizeViewport();

		// TODO: Reset the scissor rect as well
		// "IDirect3DDevice9::SetRenderTarget resets the scissor rectangle to the full render target, analogous to the viewport reset."
		// Source: https://docs.microsoft.com/en-us/windows/desktop/direct3d9/scissor-test
	}

	return ret;
}
