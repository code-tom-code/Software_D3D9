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
		hookRet->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool);
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
		hookRet->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool);
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

	LPDIRECT3DSTATEBLOCK9 realStateBlock = NULL;
	HRESULT ret = d3d9dev->CreateStateBlock(Type, &realStateBlock);
	if (FAILED(ret) )
		return ret;

	if (ppSB)
	{
		IDirect3DStateBlock9Hook* newStateBlock = new IDirect3DStateBlock9Hook(realStateBlock, this);
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

static const unsigned texCoordSizeLookup[4] =
{
	sizeof(D3DXVECTOR2),
	sizeof(D3DXVECTOR3),
	sizeof(D3DXVECTOR4),
	sizeof(float)
};

// This is not an official D3D9 function, even though it looks like one. It is only used internally.
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

	D3DVERTEXELEMENT9 elements[14] = {0};

	unsigned currentElementIndex = 0;
	unsigned short totalVertexSizeBytes = 0;
	switch (FVF.rawFVF_DWORD & D3DFVF_POSITION_MASK)
	{
	case D3DFVF_XYZ:
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR3);
		break;
	case D3DFVF_XYZRHW:
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR4);
		break;
	case D3DFVF_XYZB1:
	case D3DFVF_XYZB2:
	case D3DFVF_XYZB3:
	case D3DFVF_XYZB4:
	case D3DFVF_XYZB5:
		// Not yet handled...
		DbgBreakPrint("Error: D3DFVF_XYZBn are not yet supported by FVF -> Vertex Decl conversion");
		break;
	case D3DFVF_XYZW:
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0};
		totalVertexSizeBytes += sizeof(D3DXVECTOR4);
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

	const unsigned numTexCoords = ( (FVF.rawFVF_DWORD & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);
#ifdef _DEBUG
	if (numTexCoords > D3DDP_MAXTEXCOORD)
	{
		DbgBreakPrint("Error: Max number of texture coords for a FVF code is D3DDP_MAXTEXCOORD (8)");
	}
#endif

	for (BYTE x = 0; x < numTexCoords; ++x)
	{
		const unsigned texCoordTypeShift = (16 + x * 2);
		const unsigned texCoordTypeMask = 0x3 << texCoordTypeShift;
		const unsigned texCoordType = (FVF.rawFVF_DWORD & texCoordTypeMask) >> texCoordTypeShift;
		const D3DDECLTYPE texCoordTypeFormat = texCoordTypeLookup[texCoordType];
		const unsigned texCoordTypeSize = texCoordSizeLookup[texCoordType];
		elements[currentElementIndex++] = {0, totalVertexSizeBytes, (const BYTE)texCoordTypeFormat, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, x};
		totalVertexSizeBytes += texCoordTypeSize;
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