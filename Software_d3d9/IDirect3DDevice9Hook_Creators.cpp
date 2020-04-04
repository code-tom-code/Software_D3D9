#pragma once

#include "GlobalToggles.h"
#include "IDirect3DDevice9Hook.h"
#include "IDirect3DSurface9Hook.h"
#include "IDirect3DTexture9Hook.h"
#include "IDirect3DVolumeTexture9Hook.h"
#include "IDirect3DCubeTexture9Hook.h"
#include "IDirect3DVertexBuffer9Hook.h"
#include "IDirect3DIndexBuffer9Hook.h"
#include "IDirect3DVertexShader9Hook.h"
#include "IDirect3DPixelShader9Hook.h"
#include "IDirect3DVertexDeclaration9Hook.h"
#include "IDirect3DStateBlock9Hook.h"
#include "IDirect3DQuery9Hook.h"

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateAdditionalSwapChain(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain)
{
	// TODO: Implement this for reals
	HRESULT ret = d3d9dev->CreateAdditionalSwapChain(pPresentationParameters, pSwapChain);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateTexture(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
	HRESULT ret = d3d9dev->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
	if (FAILED(ret) )
		return ret;

	if (ppTexture)
	{
		IDirect3DTexture9Hook* hookRet = new IDirect3DTexture9Hook(*ppTexture, this);
		hookRet->CreateTexture(Width, Height, Levels, (const DebuggableUsage)Usage, Format, Pool);
		*ppTexture = hookRet;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateVolumeTexture(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)
{
	HRESULT ret = d3d9dev->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);
	if (FAILED(ret) )
		return ret;

	if (ppVolumeTexture)
	{
		IDirect3DVolumeTexture9Hook* hookRet = new IDirect3DVolumeTexture9Hook(*ppVolumeTexture, this);
		hookRet->CreateVolumeTexture(Width, Height, Depth, Levels, (const DebuggableUsage)Usage, Format, Pool);
		*ppVolumeTexture = hookRet;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateCubeTexture(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle)
{
	HRESULT ret = d3d9dev->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);
	if (FAILED(ret) )
		return ret;

	if (ppCubeTexture)
	{
		IDirect3DCubeTexture9Hook* hookRet = new IDirect3DCubeTexture9Hook(*ppCubeTexture, this);
		hookRet->CreateCubeTexture(EdgeLength, Levels, (const DebuggableUsage)Usage, Format, Pool);
		*ppCubeTexture = hookRet;
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateVertexBuffer(THIS_ UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle)
{
	LPDIRECT3DVERTEXBUFFER9 realObject = NULL;
	HRESULT ret = d3d9dev->CreateVertexBuffer(Length, Usage, FVF, Pool, &realObject, pSharedHandle);
	if (FAILED(ret) )
		return ret;

	IDirect3DVertexBuffer9Hook* const newVertexBuffer = new IDirect3DVertexBuffer9Hook(realObject, this);
	newVertexBuffer->CreateVertexBuffer(Length, (const DebuggableUsage)Usage, FVF, Pool);
	*ppVertexBuffer = newVertexBuffer;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateIndexBuffer(THIS_ UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle)
{
	if (Length == 0)
		return D3DERR_INVALIDCALL;

	switch (Format)
	{
	case D3DFMT_INDEX16:
		if (Length < sizeof(unsigned short) )
			return D3DERR_INVALIDCALL;
		if (Length % sizeof(unsigned short) != 0)
			return D3DERR_INVALIDCALL;
		break;
	case D3DFMT_INDEX32:
		if (Length < sizeof(unsigned long) )
			return D3DERR_INVALIDCALL;
		if (Length % sizeof(unsigned long) != 0)
			return D3DERR_INVALIDCALL;
		break;
	default:
		return D3DERR_INVALIDCALL;
	}

	LPDIRECT3DINDEXBUFFER9 realObject = NULL;
	HRESULT ret = d3d9dev->CreateIndexBuffer(Length, Usage, Format, Pool, &realObject, pSharedHandle);
	if (FAILED(ret) )
		return ret;

	IDirect3DIndexBuffer9Hook* const newIndexBuffer = new IDirect3DIndexBuffer9Hook(realObject, this);
	newIndexBuffer->CreateIndexBuffer(Length, (const DebuggableUsage)Usage, Format, Pool);
	*ppIndexBuffer = newIndexBuffer;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateRenderTarget(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
	LPDIRECT3DSURFACE9 realObject = NULL;
	HRESULT ret = d3d9dev->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, &realObject, pSharedHandle);
	if (FAILED(ret) )
		return ret;

	IDirect3DSurface9Hook* const newSurface = new IDirect3DSurface9Hook(realObject, this);
	newSurface->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable);
	*ppSurface = newSurface;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateDepthStencilSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
	LPDIRECT3DSURFACE9 realObject = NULL;
	HRESULT ret = d3d9dev->CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard, &realObject, pSharedHandle);
	if (FAILED(ret) )
		return ret;

	IDirect3DSurface9Hook* const newSurface = new IDirect3DSurface9Hook(realObject, this);
	newSurface->CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard);
	*ppSurface = newSurface;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateOffscreenPlainSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle)
{
	LPDIRECT3DSURFACE9 realObject = NULL;
	HRESULT ret = d3d9dev->CreateOffscreenPlainSurface(Width, Height, Format, Pool, &realObject, pSharedHandle);
	if (FAILED(ret) )
		return ret;

	if (ppSurface)
	{
		IDirect3DSurface9Hook* const newSurface = new IDirect3DSurface9Hook(realObject, this);
		newSurface->CreateOffscreenPlainSurface(Width, Height, Format, Pool);
		*ppSurface = newSurface;
		return ret;
	}
	else
	{
		realObject->Release();
		return D3DERR_INVALIDCALL;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateStateBlock(THIS_ D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)
{
#ifdef _DEBUG
	char buffer[256];
#pragma warning(push)
#pragma warning(disable:4996)
	sprintf(buffer, "CreateStateBlock(Type: %u)\n", Type);
#pragma warning(pop)
	OutputDebugStringA(buffer);
#endif

	if (ppSB)
	{
		LPDIRECT3DSTATEBLOCK9 realStateBlock = NULL;
		HRESULT ret = d3d9dev->CreateStateBlock(Type, &realStateBlock);
		if (FAILED(ret) )
			return ret;

		void* const newStateBlockMemory = _aligned_malloc(sizeof(IDirect3DStateBlock9Hook), 16);
		if (!newStateBlockMemory)
			return D3DERR_OUTOFVIDEOMEMORY;

		const bool isCompleteStateBlock = true;
		IDirect3DStateBlock9Hook* newStateBlock = new (newStateBlockMemory) IDirect3DStateBlock9Hook(realStateBlock, this, isCompleteStateBlock);
		newStateBlock->InitializeListAndCapture(Type);
		*ppSB = newStateBlock;
		return ret;
	}
	else
		return D3DERR_INVALIDCALL;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateVertexDeclaration(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl)
{
	HRESULT ret = d3d9dev->CreateVertexDeclaration(pVertexElements, ppDecl);
	if (FAILED(ret) )
		return ret;

	IDirect3DVertexDeclaration9Hook* hook = new IDirect3DVertexDeclaration9Hook(*ppDecl, this);
	debuggableFVF noFVF;
	noFVF.rawFVF_DWORD = 0x00000000;
	hook->CreateVertexDeclaration( (const DebuggableD3DVERTEXELEMENT9* const)pVertexElements, noFVF);
	*ppDecl = hook;

	return ret;
}

// This is not an official D3D9 function, even though it looks like one. It is only used internally.
COM_DECLSPEC_NOTHROW HRESULT IDirect3DDevice9Hook::CreateVertexDeclarationFromFVF(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl, const debuggableFVF FVF)
{
	HRESULT ret = d3d9dev->CreateVertexDeclaration(pVertexElements, ppDecl);
	if (FAILED(ret) )
		return ret;

	IDirect3DVertexDeclaration9Hook* hook = new IDirect3DVertexDeclaration9Hook(*ppDecl, this);
	hook->CreateVertexDeclaration( (const DebuggableD3DVERTEXELEMENT9* const)pVertexElements, FVF);
	*ppDecl = hook;

	return ret;
}

static const D3DDECLTYPE texCoordTypeLookup[4] =
{
	D3DDECLTYPE_FLOAT2,
	D3DDECLTYPE_FLOAT3,
	D3DDECLTYPE_FLOAT4,
	D3DDECLTYPE_FLOAT1
};

static const unsigned char texCoordSizeLookup[4] =
{
	sizeof(D3DXVECTOR2),
	sizeof(D3DXVECTOR3),
	sizeof(D3DXVECTOR4),
	sizeof(float)
};

// Copied this from the Windows 10 DDK header file "d3dhal.h"
// This is the rather underdocumented 0x2000 flag.
// It is a float1 channel on stream 0 with usage 0 that implies D3DDECLUSAGE_FOG.
// It is placed between the lighting attributes and the texcoords in the vertex struct.
// There's a little bit more info about it here: https://docs.microsoft.com/en-us/windows-hardware/drivers/display/clamping-fog-intensity-per-pixel
#ifndef D3DFVF_FOG
	#define D3DFVF_FOG 0x00002000L /* There is a separate fog value in the FVF vertex */
#endif

// This is not an official D3D9 function, even though it looks like one. It is only used internally.
// Most of these conversions came from this MSDN page: https://docs.microsoft.com/en-us/windows/win32/direct3d9/mapping-fvf-codes-to-a-directx-9-declaration
// The remainder of the conversions came from testing and seeing what happens.
// The test-set contained all 2^32 unique FVF codes, which should encompass all combinations of FVF codes. For all of those tested codes, the output of this function matches that of what the D3D9 runtime
// internally uses to convert FVF codes into vertex declarations.
/*
Undocumented/underdocumented D3D9 weird things:
- D3DFVF_FOG is listed as part of D3DFVF_RESERVED2 in the d3d9 headers, but it is listed in the Windows DDK as a separate FVF flag.
- If the FVF is 0x00000000 then the DrawPrimitive call gets dropped entirely for the fixed-function pipeline (although this could probably still work for the case of a vertex shader rendering a single point using shader constants for data).
- If both D3DFVF_LASTBETA_UBYTE4 and D3DFVF_LASTBETA_D3DCOLOR are set (probably not a "valid FVF", but the runtime seems to accept it anyway) then D3DFVF_LASTBETA_UBYTE4 takes precedence and D3DFVF_LASTBETA_D3DCOLOR gets ignored.
- D3DFVF_LASTBETA_UBYTE4 and D3DFVF_LASTBETA_D3DCOLOR are ignored for D3DFVF_XYZ, D3DFVF_XYZRHW, and D3DFVF_XYZW positions (they only work if one of the XYZBn positions are used).
- D3DFVF_XYZB1 with one of the LASTBETA flags produces a vertex with blend indices but no blend weights.
- D3DFVF_XYZB5 with none of the LASTBETA flags defined means that you get a "float4 blendweights0, float1 blendindices0" rather than what I would've initially thought ("float4 blendweights0, float1 blendweights1"). I guess B5 always implies indices, although I'm not sure what float indices means.
- It seems like the runtime handles D3DFVF_XYZB2 specially when it comes to the LASTBETA flags. It seems to *always* want to make the last element of type D3DDECLTYPE_UBYTE4, and then it switches between using D3DDECLTYPE_FLOAT1 and D3DDECLTYPE_D3DCOLOR for the first DWORD.
*/
IDirect3DVertexDeclaration9Hook* IDirect3DDevice9Hook::CreateVertexDeclFromFVFCode(const debuggableFVF FVF)
{
#ifndef NO_CACHING_FVF_VERT_DECLS
	const std::map<DWORD, IDirect3DVertexDeclaration9Hook*>::const_iterator it = FVFToVertDeclCache->find(FVF.rawFVF_DWORD);
	if (it != FVFToVertDeclCache->end() )
	{
		IDirect3DVertexDeclaration9Hook* const fvfVertDecl = it->second;
		SetVertexDeclaration(fvfVertDecl);
		return fvfVertDecl;
	}
#endif // NO_CACHING_FVF_VERT_DECLS

	// 24 is the worst-case number of decl elements (including the D3DDECL_END delimiter) because we could have this monsterous FVF code:
	// (D3DFVF_XYZB5 | D3DFVF_LASTBETA_D3DCOLOR | D3DFVF_LASTBETA_UBYTE4 | D3DFVF_NORMAL | D3DFVF_PSIZE | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | TEX15 | D3DFVF_FOG |
	// D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEXCOORDSIZE4(1) | D3DFVF_TEXCOORDSIZE4(2) | D3DFVF_TEXCOORDSIZE4(3) | 
	// D3DFVF_TEXCOORDSIZE4(4) | D3DFVF_TEXCOORDSIZE4(5) | D3DFVF_TEXCOORDSIZE4(6) | D3DFVF_TEXCOORDSIZE4(7) )
	// That's 3 tokens from the "position" section (D3DFVF_XYZB5), 4 from the "attributes" section (D3DFVF_NORMAL | D3DFVF_PSIZE | D3DFVF_DIFFUSE | D3DFVF_SPECULAR), one from fog,
	// 15 tokens from the "texcoords" section (TEX8 and all of the D3DFVF_TEXCOORDSIZE4 macros), and one for the D3DDECL_END token.
	D3DVERTEXELEMENT9 elements[24] = {0};

	unsigned currentElementIndex = 0;
	unsigned short totalVertexSizeBytes = 0;
	const bool hasLastbeta = (FVF.rawFVF_DWORD & (D3DFVF_LASTBETA_D3DCOLOR | D3DFVF_LASTBETA_UBYTE4) ) != 0;
	BYTE lastBetaType;
	if (hasLastbeta)
	{
		// Check for D3DFVF_LASTBETA_UBYTE4 here first because it takes precedence over D3DFVF_LASTBETA_D3DCOLOR in the case that they are both defined at the same time
		if (FVF.namedFVF.lastBeta_UBYTE4)
		{
			lastBetaType = (const BYTE)D3DDECLTYPE_UBYTE4;
		}
		else
		{
			lastBetaType = (const BYTE)D3DDECLTYPE_D3DCOLOR;
		}
	}

	switch (FVF.rawFVF_DWORD & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ:
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR3);
	}
		break;
	case D3DFVF_XYZRHW:
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR4);
	}
		break;
	case D3DFVF_XYZW:
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR4);
	}
		break;
	case D3DFVF_XYZB1:
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR3);

		if (hasLastbeta)
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, lastBetaType, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0};
		else
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
		totalVertexSizeBytes += sizeof(float);
	}
		break;
	case D3DFVF_XYZB2:
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR3);

		if (hasLastbeta)
		{
			// Okay so this is undocumented and weird (and might be a bug?) but it seems like the runtime handles D3DFVF_XYZB2 specially when it comes to the LASTBETA flags. It seems to *always* want to make
			// the last element of type D3DDECLTYPE_UBYTE4, and then it switches between using D3DDECLTYPE_FLOAT1 and D3DDECLTYPE_D3DCOLOR for the first DWORD.
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, (const BYTE)(FVF.namedFVF.lastBeta_UBYTE4 ? D3DDECLTYPE_FLOAT1 : D3DDECLTYPE_D3DCOLOR), D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
			totalVertexSizeBytes += sizeof(float);

			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0};
			totalVertexSizeBytes += sizeof(DWORD);
		}
		else
		{
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
			totalVertexSizeBytes += sizeof(D3DXVECTOR2);
		}
	}
		break;
	case D3DFVF_XYZB3:
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR3);

		if (hasLastbeta)
		{
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
			totalVertexSizeBytes += sizeof(D3DXVECTOR2);

			elements[currentElementIndex++] = {0, totalVertexSizeBytes, lastBetaType, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0};
			totalVertexSizeBytes += sizeof(DWORD);
		}
		else
		{
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
			totalVertexSizeBytes += sizeof(D3DXVECTOR3);
		}
	}
		break;
	case D3DFVF_XYZB4:
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR3);

		if (hasLastbeta)
		{
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
			totalVertexSizeBytes += sizeof(D3DXVECTOR3);

			elements[currentElementIndex++] = {0, totalVertexSizeBytes, lastBetaType, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0};
			totalVertexSizeBytes += sizeof(DWORD);
		}
		else
		{
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
			totalVertexSizeBytes += sizeof(D3DXVECTOR4);
		}
	}
		break;
	case D3DFVF_XYZB5:
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR3);
		if (hasLastbeta)
		{
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
			totalVertexSizeBytes += sizeof(D3DXVECTOR4);

			elements[currentElementIndex++] = {0, totalVertexSizeBytes, lastBetaType, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0};
			totalVertexSizeBytes += sizeof(DWORD);
		}
		else
		{
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0};
			totalVertexSizeBytes += sizeof(D3DXVECTOR4);

			// This is a weird thing, but D3D9 has the D3DFVF_XYZB5 case create "float1 blendindices0" for D3DFVF_XYZB5 when one of the LASTBETA flags aren't specified. We're going to
			// follow this exact behavior so that we can fully match the official D3D9 FVF to Decl conversion.
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0};
			totalVertexSizeBytes += sizeof(float);
		}
	}
		break;
	}

	if (FVF.rawFVF_DWORD & D3DFVF_NORMAL)
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR3);
	}

	if (FVF.rawFVF_DWORD & D3DFVF_PSIZE)
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE, 0};
		totalVertexSizeBytes += sizeof(float);
	}

	if (FVF.rawFVF_DWORD & D3DFVF_DIFFUSE)
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0};
		totalVertexSizeBytes += sizeof(D3DCOLOR);
	}

	if (FVF.rawFVF_DWORD & D3DFVF_SPECULAR)
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1};
		totalVertexSizeBytes += sizeof(D3DCOLOR);
	}

	// The rather undocumented D3DFVF_FOG flag breaks the convention of all the other FVF flags of appearing "in order" as it actually comes *before* texcoords but *after* position and lighting attributes
	if (FVF.rawFVF_DWORD & D3DFVF_FOG)
	{
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_FOG, 0};
		totalVertexSizeBytes += sizeof(float);
	}

	const unsigned numTexCoords = ( (FVF.rawFVF_DWORD & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);
	for (BYTE x = 0; x < numTexCoords; ++x)
	{
		if (x < 8)
		{
			const unsigned texCoordTypeShift = (16 + x * 2);
			const unsigned texCoordTypeMask = 0x3 << texCoordTypeShift;
			const unsigned texCoordType = (FVF.rawFVF_DWORD & texCoordTypeMask) >> texCoordTypeShift;
			const D3DDECLTYPE texCoordTypeFormat = texCoordTypeLookup[texCoordType];
			const unsigned char texCoordTypeSize = texCoordSizeLookup[texCoordType];
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, (const BYTE)texCoordTypeFormat, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, x};
			totalVertexSizeBytes += texCoordTypeSize;
		}
		else // Since there are no more bits to read, all texcoords after TEX8 are assumed to be of "float2" format
		{
			const D3DDECLTYPE texCoordTypeFormat = D3DDECLTYPE_FLOAT2;
			const unsigned texCoordTypeSize = sizeof(D3DXVECTOR2);
			elements[currentElementIndex++] = {0, totalVertexSizeBytes, (const BYTE)texCoordTypeFormat, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, x};
			totalVertexSizeBytes += texCoordTypeSize;
		}
	}

	elements[currentElementIndex] = D3DDECL_END();

	IDirect3DVertexDeclaration9Hook* fvfVertexDecl = NULL;
	if (FAILED(CreateVertexDeclarationFromFVF(elements, (LPDIRECT3DVERTEXDECLARATION9*)&fvfVertexDecl, FVF) ) || !fvfVertexDecl)
	{
		DbgBreakPrint("Error: Failed to create vertex declaration");
	}

#ifndef NO_CACHING_FVF_VERT_DECLS
	fvfVertexDecl->AddRef();
	FVFToVertDeclCache->insert(std::make_pair(FVF.rawFVF_DWORD, fvfVertexDecl) );
#endif // NO_CACHING_FVF_VERT_DECLS

	if (FAILED(SetVertexDeclaration(fvfVertexDecl) ) )
	{
		DbgBreakPrint("Error: Failed to set vertex declaration");
	}

	return fvfVertexDecl;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateVertexShader(THIS_ CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader)
{
	HRESULT ret = d3d9dev->CreateVertexShader(pFunction, ppShader);
	if (FAILED(ret) )
		return ret;

	IDirect3DVertexShader9Hook* hookObj = new IDirect3DVertexShader9Hook(*ppShader, this);
	hookObj->CreateVertexShader(pFunction);
	*ppShader = hookObj;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreatePixelShader(THIS_ CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader)
{
	HRESULT ret = d3d9dev->CreatePixelShader(pFunction, ppShader);
	if (FAILED(ret) )
		return ret;

	IDirect3DPixelShader9Hook* hookObj = new IDirect3DPixelShader9Hook(*ppShader, this);
	hookObj->CreatePixelShader(pFunction);
	*ppShader = hookObj;

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::CreateQuery(THIS_ D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) 
{
	HRESULT ret = d3d9dev->CreateQuery(Type, ppQuery);
	if (FAILED(ret) )
		return ret;

	// This parameter can be set to NULL to see if a query type is supported. If the query type is not supported, the method returns D3DERR_NOTAVAILABLE.
	// Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb174360(v=vs.85).aspx
	if (!ppQuery)
	{
		switch (Type)
		{
			// These are the only query types we support right now (they are also by far the most common types of queries used in D3D9):
		case D3DQUERYTYPE_EVENT:
		case D3DQUERYTYPE_OCCLUSION:
		case D3DQUERYTYPE_TIMESTAMP:
		case D3DQUERYTYPE_TIMESTAMPDISJOINT:
		case D3DQUERYTYPE_TIMESTAMPFREQ:
			break;
		default:
			return D3DERR_NOTAVAILABLE;
		}
	}
	else
	{
		IDirect3DQuery9Hook* newQuery = new IDirect3DQuery9Hook(*ppQuery, this);
		newQuery->CreateQuery(Type);
		*ppQuery = newQuery;
	}

	return ret;
}