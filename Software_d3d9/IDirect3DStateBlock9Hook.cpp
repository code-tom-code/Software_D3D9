/*
State block notes:
IDirect3DDevice9::CreateStateBlock creates a new state block object and fills it with all of the (vertex | pixel | all) current device states.
Using IDirect3DDevice9::BeginStateBlock and ::EndStateBlock pairs also creates a new state block, but the state block returned via EndStateBlock() will only contain states set inbetween the pair of calls (and it will *not* contain any values for states not set inbetween those two calls). Any render-state changes set inside the Begin/End block do *not* make it through to the device! Instead, they are written to the recording state block.
Using IDirect3DStateBlock9::Apply applies the state data (which may be none) inside the state block to the device. 
IDirect3DStateBlock9::Capture copies the current device state into the state block, but unlike IDirect3DDevice9::CreateStateBlock(), the Capture() method only copies in state that exists in the state block (so if a render-state was not set in that state block, then that render-state won't be recorded into the state block when Capture() is called).

So implementation pseudocode might look like this:
IDirect3DDevice9::CreateStateBlock makes a new full state block with (all vertex | all pixel | all all) renderstates set to "recorded", and copies the complete device state inside of it. Once a render-state becomes "recorded" for a state block, that render state can never be reset to "not recorded".
IDirect3DDevice9::BeginStateBlock can create a new empty state block internally (with all states set to "not recorded"), and then whenever a state change occurs, that state in the internal state block becomes "recorded" and its state is copied into the state block rather than to the normal device's state. Then when ::EndStateBlock is called, the internal state block is returned to the caller.
IDirect3DStateBlock9::Apply iterates over all render-states that are marked as "recorded" and sets those on the owning IDirect3DDevice9 (the one that created it in the first place).
IDirect3DStateBlock9::Capture copies only the render-states that are marked as "recorded" from the owning IDirect3DDevice9 (the one that created it in the first place) at the time of the call to Capture(). This is a way to update the states within state blocks (although the list of "recorded" and "not recorded" states may never change after a state block is created).

Assorted verified behaviors and verified edge-cases:
BeginStateBlock(), BeginStateBlock() is disallowed without an EndStateBlock() inbetween (there can only be 0 or 1 active state-blocks per device).
Illegal calls to BeginStateBlock() while a state block is already being recorded do not result in the state block's state getting reset! The second BeginStateBlock returns without changing anything!
Similarly, EndStateBlock(), EndStateBlock() is disallowed without a BeginStateBlock() before each (you may only call EndStateBlock() when there is an actively recording state block). Doing an EndStateBlock(), EndStateBlock() pair results in the second call returning NULL.
You can apparently create empty state blocks with nothing in them that change no states.
Interestingly, calling GetRenderState() while in the process of capturing state for a state block returns the device's current state, *not* the currently-recording state block's current state.
Similarly, draw calls while in the process of capturing state for a state block seem to use the device state *before* the current state block began recording (so the process of recording states into a state block do not seem to modify the underlying device state until EndStateBlock() is called)
Calling Apply() on one state block while another is in the process of being recorded seems to be return an error code and drops the Apply() call from happening (the Apply() call during recording seems to not affect either the in-progress recording state block nor the underlying device, the call simply early-outs).
Calling Capture() on one state block while another is in the process of being recorded seems to be return an error code and drops the Capture() call from happening (the Capture() call during recording seems to not affect either the in-progress recording state block nor the underlying device, the call simply early-outs).
It seems that calling to record a state even if it's the same state that's currently set on the device still captures that state into the state block (so don't let the SetRenderState(), etc. caching logic allow state capturing to occur).

Open question notes:
Does MultiplyTransform() source its source matrix from an ongoing stateblock or have its result stored in a state block if there's a state block currently being recorded?
Does SetClipStatus() get captured?
Does SetCursorProperties() get captured?
Does SetDialogBoxMode() get captured?
Does SetGammaRamp() get captured?
Does ShowCursor() get captured?
Does SetPaletteEntries() get captured?
Does the state for SetFVF get captured in vertex/pixel/all state blocks?
If you enable or disable a light (using LightEnable) that doesn't exist in a state block and then apply that state block to a device where that light does exist, what happens?
Does the order which you re-apply saved states matter (for example, SetRenderTarget() also internally invokes SetViewport(), and SetFVF() also internally invokes SetVertexDeclaration(), so what happens if you capture both of those calls?)
Does the state for SetClipPlane() get captured in vertex/pixel/all state blocks?
Does capturing SetVertexDeclaration() then un-capture SetFVF() or vice-versa?
If a device is capturing while it performs a Draw() call with the fixed-function pipeline that internally invokes SetPixelShaderConstantF/SetVertexShaderConstantF, do those calls get captured or not captured?

Known captured API's:
* IDirect3DDevice9::LightEnable
* IDirect3DDevice9::SetClipPlane
* IDirect3DDevice9::SetCurrentTexturePalette
* IDirect3DDevice9::SetFVF
* IDirect3DDevice9::SetIndices
* IDirect3DDevice9::SetLight
* IDirect3DDevice9::SetMaterial
* IDirect3DDevice9::SetNPatchMode
* IDirect3DDevice9::SetPixelShader
* IDirect3DDevice9::SetPixelShaderConstantB
* IDirect3DDevice9::SetPixelShaderConstantF
* IDirect3DDevice9::SetPixelShaderConstantI
* IDirect3DDevice9::SetRenderState
* IDirect3DDevice9::SetSamplerState
* IDirect3DDevice9::SetScissorRect
* IDirect3DDevice9::SetStreamSource
* IDirect3DDevice9::SetStreamSourceFreq
* IDirect3DDevice9::SetTexture
* IDirect3DDevice9::SetTextureStageState
* IDirect3DDevice9::SetTransform
* IDirect3DDevice9::SetViewport
* IDirect3DDevice9::SetVertexDeclaration
* IDirect3DDevice9::SetVertexShader
* IDirect3DDevice9::SetVertexShaderConstantB
* IDirect3DDevice9::SetVertexShaderConstantF
* IDirect3DDevice9::SetVertexShaderConstantI
*/

#pragma once

#include "IDirect3DStateBlock9Hook.h"
#include "IDirect3DIndexBuffer9Hook.h"
#include "IDirect3DVertexBuffer9Hook.h"
#include "IDirect3DPixelShader9Hook.h"
#include "IDirect3DVertexShader9Hook.h"
#include "IDirect3DTexture9Hook.h"
#include "IDirect3DCubeTexture9Hook.h"
#include "IDirect3DVolumeTexture9Hook.h"
#include "IDirect3DVertexDeclaration9Hook.h"

/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DStateBlock9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	HRESULT ret = realObject->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DStateBlock9Hook::AddRef(THIS)
{
	ULONG ret = realObject->AddRef();
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DStateBlock9Hook::Release(THIS)
{
	ULONG ret = realObject->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked State Block %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif

		// Because we are allocated via _aligned_alloc() and not from new, we need to manually handle this object's deletion rather than just calling operator delete on it:
		IDirect3DStateBlock9Hook* const thisPtr = this;
		thisPtr->~IDirect3DStateBlock9Hook();
		_aligned_free(thisPtr);
		// Make sure not to access any class members after this point, we've been deleted and freed now!
	}
	return ret;
}

/*** IDirect3DStateBlock9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DStateBlock9Hook::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
{
	LPDIRECT3DDEVICE9 realD3D9dev = NULL;
	HRESULT ret = realObject->GetDevice(&realD3D9dev);
	if (FAILED(ret) )
	{
		*ppDevice = NULL;
		return ret;
	}

	// Check that the parentHook's underlying IDirect3DDevice9* matches the realD3D9dev pointer
	if (parentDevice->GetUnderlyingDevice() != realD3D9dev)
	{
		DbgBreakPrint("Error: Unknown d3d9 device hook detected!");
	}
	parentDevice->AddRef(); // Super important to increment the ref-count here, otherwise our parent object will get destroyed when Release() is called on it!

	*ppDevice = parentDevice;
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DStateBlock9Hook::Capture(THIS)
{
#ifdef _DEBUG
	char buffer[256];
#pragma warning(push)
#pragma warning(disable:4996)
	sprintf(buffer, "IDirect3DStateBlock9::Capture()\n");
#pragma warning(pop)
	OutputDebugStringA(buffer);
#endif

	// It is illegal to call IDirect3DStateBlock9::Capture() while you are in the middle of recording another, different state block.
	// If you do this, then the capture is not actually applied, and the call gets dropped.
	if (parentDevice->IsCurrentlyRecordingStateBlock() )
	{
		return D3DERR_INVALIDCALL;
	}

	HRESULT ret = realObject->Capture();
	if (FAILED(ret) )
		return ret;

	const IDirect3DDevice9Hook* const constDevice = parentDevice;
	const DeviceState& currentParentDeviceState = constDevice->GetCurrentHookState();
	stateBlockState.CaptureCopyState(currentParentDeviceState);

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DStateBlock9Hook::Apply(THIS)
{
#ifdef _DEBUG
	char buffer[256];
#pragma warning(push)
#pragma warning(disable:4996)
	sprintf(buffer, "IDirect3DStateBlock9::Apply()\n");
#pragma warning(pop)
	OutputDebugStringA(buffer);
#endif

	// It is illegal to call IDirect3DStateBlock9::Apply() while you are in the middle of recording another, different state block.
	// If you do this, then the state is not actually applied, and the call gets dropped.
	if (parentDevice->IsCurrentlyRecordingStateBlock() )
	{
		return D3DERR_INVALIDCALL;
	}

	HRESULT ret = realObject->Apply();
	if (FAILED(ret) )
		return ret;

	// Set all captured states from the state block to the main device:

	// Render states:
	for (unsigned char x = 0; x < ARRAYSIZE(renderStateStateBlockTypes); ++x)
	{
		const D3DRENDERSTATETYPE typedRenderState = (const D3DRENDERSTATETYPE)x;
		const unsigned rsDwordIndex = typedRenderState / 32;
		const unsigned rsBitmask = 1 << (typedRenderState % 32);
		if (capturedStates.capturedRenderstates.captureRenderstate[rsDwordIndex] & rsBitmask)
			parentDevice->SetRenderState(typedRenderState, stateBlockState.currentRenderStates.renderStatesUnion.states[typedRenderState]);
	}

	for (unsigned char x = 0; x < D3DMAXUSERCLIPPLANES; ++x)
	{
		const unsigned cpBitmask = (1 << x);
		if (capturedStates.capturedUserClipPlanes.userClipPlanesBitmask & cpBitmask)
			parentDevice->SetClipPlane(x, &(stateBlockState.currentClippingPlanes[x].a) );
	}

	for (unsigned char x = 0; x < MAX_D3D9_STREAMS; ++x)
	{
		const unsigned streamNumberBitmask = (1 << x);
		if (capturedStates.capturedStreamSources.streamSourcesBitmask & streamNumberBitmask)
		{
			const StreamSource& thisStream = stateBlockState.currentStreams[x];
			parentDevice->SetStreamSource(x, thisStream.vertexBuffer, thisStream.streamOffset, thisStream.streamStride);
		}

		if (capturedStates.capturedStreamSourceFreq.streamSourceFreqBitmask & streamNumberBitmask)
			parentDevice->SetStreamSourceFreq(x, stateBlockState.currentStreams[x].streamDividerFrequency);
	}

	// Transforms:
	if (capturedStates.capturedTransforms.projectionCaptured)
		parentDevice->SetTransform(D3DTS_PROJECTION, &stateBlockState.currentTransforms.ProjectionTransform);
	if (capturedStates.capturedTransforms.viewCaptured)
		parentDevice->SetTransform(D3DTS_VIEW, &stateBlockState.currentTransforms.ViewTransform);
	for (unsigned char texTransformIndex = 0; texTransformIndex < D3DDP_MAXTEXCOORD; ++texTransformIndex)
	{
		const unsigned char texTransformBitmask = (1 << texTransformIndex);
		const D3DTRANSFORMSTATETYPE texTransformEnum = (const D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + texTransformIndex);
		if (capturedStates.capturedTransforms.textureTransformsCaptured.textureTransformsCapturedBitmask & texTransformBitmask)
			parentDevice->SetTransform(texTransformEnum, &(stateBlockState.currentTransforms.TextureTransforms[texTransformIndex]) );
	}
	for (unsigned short worldTransformIndex = 0; worldTransformIndex < MAX_WORLD_TRANSFORMS; ++worldTransformIndex)
	{
		const unsigned worldTransformDwordIndex = worldTransformIndex / 32;
		const unsigned worldTransformBitmask = 1 << (worldTransformIndex % 32);
		const D3DTRANSFORMSTATETYPE worldTransformEnum = (const D3DTRANSFORMSTATETYPE)(D3DTS_WORLD + worldTransformIndex);
		if (capturedStates.capturedTransforms.captureWorldTransforms[worldTransformDwordIndex] & worldTransformBitmask)
			parentDevice->SetTransform(worldTransformEnum, &(stateBlockState.currentTransforms.WorldTransforms[worldTransformIndex]) );
	}

	// TODO: Do all of the other captured device state (textures, vertex decls, VB's, IB's, texture stage states, etc.)
	if (capturedStates.singleCallStatesCaptured[SBT_SetIndices])
		parentDevice->SetIndices(stateBlockState.currentIndexBuffer);

	if (capturedStates.singleCallStatesCaptured[SBT_SetMaterial])
		parentDevice->SetMaterial(&stateBlockState.currentMaterial);

	if (capturedStates.singleCallStatesCaptured[SBT_SetNPatchMode])
		parentDevice->SetNPatchMode(stateBlockState.currentNPatchMode);

	if (capturedStates.singleCallStatesCaptured[SBT_SetViewport])
		parentDevice->SetViewport(&stateBlockState.cachedViewport.viewport);

	if (capturedStates.singleCallStatesCaptured[SBT_SetFVF])
		parentDevice->SetFVF(stateBlockState.currentFVF.rawFVF_DWORD);

	// We're currently setting the vertex declaration *after* the FVF
	if (capturedStates.singleCallStatesCaptured[SBT_SetVertexDeclaration])
		parentDevice->SetVertexDeclaration(stateBlockState.currentVertexDecl);

	if (capturedStates.singleCallStatesCaptured[SBT_SetScissorRect])
		parentDevice->SetScissorRect(&stateBlockState.currentScissorRect.scissorRect);

	if (capturedStates.singleCallStatesCaptured[SBT_SetPixelShader])
		parentDevice->SetPixelShader(stateBlockState.currentPixelShader);

	if (capturedStates.singleCallStatesCaptured[SBT_SetVertexShader])
		parentDevice->SetVertexShader(stateBlockState.currentVertexShader);

	if (capturedStates.singleCallStatesCaptured[SBT_SetCurrentTexturePalette])
		parentDevice->SetCurrentTexturePalette(stateBlockState.currentPaletteState.currentPaletteIndex);

	// Testing and setting constants one at a time is not particularly efficient. It'd be better in the case of large numbers of captured constants to set them all at once.
	for (unsigned psConstantIndexF = 0; psConstantIndexF < 4096; ++psConstantIndexF)
	{
		const unsigned psConstantDwordIndex = psConstantIndexF / 32;
		const unsigned psConstantBitmask = 1 << (psConstantIndexF % 32);
		if (capturedStates.capturedPixelShaderConstants.floatsCaptured[psConstantDwordIndex] & psConstantBitmask)
			parentDevice->SetPixelShaderConstantF(psConstantIndexF, &(stateBlockState.pixelShaderRegisters.floats[psConstantIndexF].x), 1);
	}
	for (unsigned char psConstantIndexI = 0; psConstantIndexI < 16; ++psConstantIndexI)
	{
		const unsigned short psConstantBitmask = 1 << psConstantIndexI;
		if (capturedStates.capturedPixelShaderConstants.intCaptured.capturedConstantsBitmask & psConstantBitmask)
			parentDevice->SetPixelShaderConstantI(psConstantIndexI, &(stateBlockState.pixelShaderRegisters.ints[psConstantIndexI].x), 1);
	}
	for (unsigned char psConstantIndexB = 0; psConstantIndexB < 16; ++psConstantIndexB)
	{
		const unsigned short psConstantBitmask = 1 << psConstantIndexB;
		if (capturedStates.capturedPixelShaderConstants.boolCaptured.capturedConstantsBitmask & psConstantBitmask)
			parentDevice->SetPixelShaderConstantB(psConstantIndexB, &(stateBlockState.pixelShaderRegisters.bools[psConstantIndexB]), 1);
	}

	// Testing and setting constants one at a time is not particularly efficient. It'd be better in the case of large numbers of captured constants to set them all at once.
	for (unsigned vsConstantIndexF = 0; vsConstantIndexF < 4096; ++vsConstantIndexF)
	{
		const unsigned vsConstantDwordIndex = vsConstantIndexF / 32;
		const unsigned vsConstantBitmask = 1 << (vsConstantIndexF % 32);
		if (capturedStates.capturedVertexShaderConstants.floatsCaptured[vsConstantDwordIndex] & vsConstantBitmask)
			parentDevice->SetVertexShaderConstantF(vsConstantIndexF, &(stateBlockState.vertexShaderRegisters.floats[vsConstantIndexF].x), 1);
	}
	for (unsigned char vsConstantIndexI = 0; vsConstantIndexI < 16; ++vsConstantIndexI)
	{
		const unsigned short vsConstantBitmask = 1 << vsConstantIndexI;
		if (capturedStates.capturedVertexShaderConstants.intCaptured.capturedConstantsBitmask & vsConstantBitmask)
			parentDevice->SetVertexShaderConstantI(vsConstantIndexI, &(stateBlockState.vertexShaderRegisters.ints[vsConstantIndexI].x), 1);
	}
	for (unsigned char vsConstantIndexB = 0; vsConstantIndexB < 16; ++vsConstantIndexB)
	{
		const unsigned short vsConstantBitmask = 1 << vsConstantIndexB;
		if (capturedStates.capturedVertexShaderConstants.boolCaptured.capturedConstantsBitmask & vsConstantBitmask)
			parentDevice->SetVertexShaderConstantB(vsConstantIndexB, &(stateBlockState.vertexShaderRegisters.bools[vsConstantIndexB]), 1);
	}

	for (unsigned short textureIndex = 0; textureIndex < MAX_NUM_SAMPLERS; ++textureIndex)
	{
		if (textureIndex >= 16 && textureIndex < D3DDMAPSAMPLER)
			continue;

		const unsigned textureDWORD = textureIndex / 32;
		const unsigned textureBitflag = 1 << (textureIndex % 32);
		if (capturedStates.capturedTextures.captureTextures[textureDWORD] & textureBitflag)
		{
			if (stateBlockState.currentTextures[textureIndex])
				parentDevice->SetTexture(textureIndex, stateBlockState.currentTextures[textureIndex]);
			else if (stateBlockState.currentCubeTextures[textureIndex])
				parentDevice->SetTexture(textureIndex, stateBlockState.currentCubeTextures[textureIndex]);
			else if (stateBlockState.currentVolumeTextures[textureIndex])
				parentDevice->SetTexture(textureIndex, stateBlockState.currentVolumeTextures[textureIndex]);
			else
				parentDevice->SetTexture(textureIndex, NULL);
		}

		const unsigned short thisTextureSamplerCapture = capturedStates.capturedSamplerStates.samplerStates[textureIndex].samplerStateBitmask;
		if (thisTextureSamplerCapture != 0x0000)
		{
			for (unsigned char samplerTypeIndex = D3DSAMP_ADDRESSU; samplerTypeIndex <= D3DSAMP_DMAPOFFSET; ++samplerTypeIndex)
			{
				const D3DSAMPLERSTATETYPE samplerType = (const D3DSAMPLERSTATETYPE)samplerTypeIndex;
				const unsigned short samplerTypeBitmask = 1 << samplerTypeIndex;
				if (thisTextureSamplerCapture & samplerTypeBitmask)
					parentDevice->SetSamplerState(textureIndex, samplerType, stateBlockState.currentSamplerStates[textureIndex].stateUnion.state[samplerType]);
			}
		}
	}

	for (unsigned char textureStage = 0; textureStage < MAX_NUM_TEXTURE_STAGE_STATES; ++textureStage)
	{
		const capturedStateBitmask::_capturedTextureStageStates::_stageStateBits& thisCapturedStage = capturedStates.capturedTextureStageStates.capturedTextureStages[textureStage];
		const TextureStageState& stateBlockStageState = stateBlockState.currentStageStates[textureStage];
		for (unsigned stateNum = D3DTSS_COLOROP; stateNum <= D3DTSS_CONSTANT; ++stateNum)
		{
			const D3DTEXTURESTAGESTATETYPE stateType = (const D3DTEXTURESTAGESTATETYPE)stateNum;
			const unsigned stateIndex = stateNum - 1;
			const unsigned stateBitflag = 1 << stateIndex;
			if (thisCapturedStage.stageStateBitfields & stateBitflag)
				parentDevice->SetTextureStageState(textureStage, stateType, stateBlockStageState.stageStateUnion.state[stateType]);
		}
	}

	if (capturedStates.capturedLights != NULL)
	{
		for (std::map<UINT, capturedStateBitmask::lightCaptureStruct>::const_iterator lightIter = capturedStates.capturedLights->begin(); lightIter != capturedStates.capturedLights->end(); ++lightIter)
		{
			const UINT lightIndex = lightIter->first;
			const capturedStateBitmask::lightCaptureStruct& lightCaptureData = lightIter->second;

			if (lightCaptureData.captureSetLight)
			{
				const std::map<UINT, LightInfo*>::const_iterator lightInfoIter = stateBlockState.lightInfoMap->find(lightIndex);
#ifdef _DEBUG
				if (lightInfoIter == stateBlockState.lightInfoMap->end() )
				{
					__debugbreak(); // This should never happen. It indicates a mismatch between our captured data and the fact that we captured it at all.
				}
#endif
				const LightInfo* const foundLightInfo = lightInfoIter->second;
				parentDevice->SetLight(lightIndex, &(foundLightInfo->light) );
			}

			// Make sure to potentially enable/disable the light *after* we create it with SetLight first!
			if (lightCaptureData.captureLightEnable)
				parentDevice->LightEnable(lightIndex, lightCaptureData.lightEnable);
		}
	}
	
	return ret;
}

void IDirect3DStateBlock9Hook::InitializeListAndCapture(const D3DSTATEBLOCKTYPE type)
{
#ifdef _DEBUG
	if (type < D3DSBT_ALL || type > D3DSBT_VERTEXSTATE)
	{
		DbgBreakPrint("Error: Only call this function with a valid state block type");
	}
#endif
	internalStateBlockType = type;

	renderStateStateBlockType rsType;
	switch (type)
	{
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Should never be here!");
#endif
	case D3DSBT_ALL:
		rsType = SBT_ALL_State;
		break;
	case D3DSBT_PIXELSTATE:
		rsType = pixelState;
		break;
	case D3DSBT_VERTEXSTATE:
		rsType = vertexState;
		break;
	}

	// TODO: This can and should be precomputed instead of being computed at runtime:
	for (unsigned char x = 0; x < ARRAYSIZE(renderStateStateBlockTypes); ++x)
	{
		const D3DRENDERSTATETYPE typedRenderState = (const D3DRENDERSTATETYPE)x;
		if (renderStateStateBlockTypes[typedRenderState] & rsType)
		{
			const unsigned char rsDWORDIndex = (const unsigned char)(typedRenderState / 32);
			const unsigned rsBitmask = 1 << (typedRenderState % 32);

			capturedStates.capturedRenderstates.captureRenderstate[rsDWORDIndex] |= rsBitmask;
		}
	}

	// Only if all: https://docs.microsoft.com/en-us/windows/win32/direct3d9/saving-all-device-states-with-a-stateblock
	if (rsType == SBT_ALL_State)
	{
		MarkSetCallAsCaptured<SBT_SetIndices>();
		MarkSetCallAsCaptured<SBT_SetMaterial>();
		MarkSetCallAsCaptured<SBT_SetScissorRect>();
		MarkSetCallAsCaptured<SBT_SetViewport>();
		MarkSetCallAsCaptured<SBT_SetCurrentTexturePalette>();

		capturedStates.capturedUserClipPlanes.userClipPlanesBitmask = 0xFFFFFFFF;
		capturedStates.capturedStreamSources.streamSourcesBitmask = 0xFFFF; // TODO: Check that this isn't actually stored as part of vertexState. The D3D9 docs are inconsistent here and it seems weird that they mention that the frequency gets stored in vertexState, but not the rest of the stream source state?

		// Transforms:
		capturedStates.capturedTransforms.viewCaptured = true;
		capturedStates.capturedTransforms.projectionCaptured = true;
		capturedStates.capturedTransforms.textureTransformsCaptured.textureTransformsCapturedBitmask = 0xFF;
		for (unsigned char x = 0; x < ARRAYSIZE(capturedStates.capturedTransforms.captureWorldTransforms); ++x)
			capturedStates.capturedTransforms.captureWorldTransforms[x] = 0xFFFFFFFF;
	}

	// Only if all or vertex: https://docs.microsoft.com/en-us/windows/win32/direct3d9/saving-vertex-states-with-a-stateblock
	if (rsType & (vertexState) )
	{
		MarkSetCallAsCaptured<SBT_SetNPatchMode>();
		MarkSetCallAsCaptured<SBT_SetVertexShader>();
		MarkSetCallAsCaptured<SBT_SetFVF>();
		MarkSetCallAsCaptured<SBT_SetVertexDeclaration>();

		capturedStates.capturedStreamSourceFreq.streamSourceFreqBitmask = 0xFFFF;

		capturedStates.capturedVertexShaderConstants.MarkSetAllShaderConstantsCaptured();

		// The vertex pipeline gets to capture the DMAP (displacement map) texture, plus the four vertex texture fetch samplers available to the vertex shader
		// TODO: Double check that DMAP is actually supposed to get captured by D3DSBT_VERTEXSTATE state blocks because the docs are a little bit unclear about that
		for (unsigned vertexTextureIndex = D3DDMAPSAMPLER; vertexTextureIndex < MAX_NUM_SAMPLERS; ++vertexTextureIndex)
		{
			MarkSetTextureCaptured(vertexTextureIndex);

			capturedStates.capturedSamplerStates.samplerStates[vertexTextureIndex].samplerStateBitmask = 0xFFFF;
		}

		const DeviceState& parentState = parentDevice->GetCurrentHookState();
		if (parentState.lightInfoMap != NULL)
		{
			for (std::map<UINT, LightInfo*>::const_iterator it = parentState.lightInfoMap->begin(); it != parentState.lightInfoMap->end(); ++it)
			{
				if (it->second != NULL)
				{
					const UINT lightNum = it->first;
					MarkSetLightCaptured(lightNum);
				}
			}

			for (unsigned lightIndex = 0; lightIndex < ARRAYSIZE(parentState.enabledLightIndices); ++lightIndex)
			{
				if (parentState.enabledLightIndices[lightIndex] != NULL)
					MarkLightEnableCaptured(lightIndex, true);
			}
		}
	}

	// Only if all or pixel: https://docs.microsoft.com/en-us/windows/win32/direct3d9/saving-pixel-states-with-a-stateblock
	if (rsType & (pixelState) )
	{
		MarkSetCallAsCaptured<SBT_SetPixelShader>();

		// The pixel pipeline gets to capture all of the textures except for the DMAP and vertex shader samplers
		for (unsigned pixelTextureIndex = 0; pixelTextureIndex < D3DDMAPSAMPLER; ++pixelTextureIndex)
		{
			MarkSetTextureCaptured(pixelTextureIndex);

			capturedStates.capturedSamplerStates.samplerStates[pixelTextureIndex].samplerStateBitmask = 0xFFFF;
		}

		for (unsigned char stage = 0; stage < MAX_NUM_TEXTURE_STAGE_STATES; ++stage)
			capturedStates.capturedTextureStageStates.capturedTextureStages[stage].stageStateBitfields = 0xFFFFFFFF;

		capturedStates.capturedPixelShaderConstants.MarkSetAllShaderConstantsCaptured();
	}

	// Finally, capture the current device state from the owning device:
	Capture();
}
