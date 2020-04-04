#pragma once

#include "IDirect3DVertexDeclaration9Hook.h"

static inline const bool operator==(const DebuggableD3DVERTEXELEMENT9& lhs, const DebuggableD3DVERTEXELEMENT9& rhs)
{
	return (lhs.Stream == rhs.Stream) && (lhs.Offset == rhs.Offset) && (lhs.Type == rhs.Type) && (lhs.Method == rhs.Method) && (lhs.Usage == rhs.Usage) && (lhs.UsageIndex == rhs.UsageIndex);
}

static inline const bool operator!=(const DebuggableD3DVERTEXELEMENT9& lhs, const DebuggableD3DVERTEXELEMENT9& rhs)
{
	return !(lhs == rhs);
}

void IDirect3DVertexDeclaration9Hook::CreateVertexDeclaration(const DebuggableD3DVERTEXELEMENT9* const pVertexElements, const debuggableFVF _vertDeclAutoCreatedFromFVF)
{
	if (!pVertexElements)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	unsigned numElements = 0;
	static const DebuggableD3DVERTEXELEMENT9 endDecl = D3DDECL_END();
	while (pVertexElements[numElements++] != endDecl);

	elements.resize(numElements);
	memcpy(&elements.front(), pVertexElements, numElements * sizeof(DebuggableD3DVERTEXELEMENT9) );

	vertDeclAutoCreatedFromFVF = _vertDeclAutoCreatedFromFVF;

	// Compute skipVertexProcessing:
	skipVertexProcessing = false;
	for (unsigned x = 0; x < numElements; ++x)
	{
		if (elements[x].Usage == D3DDECLUSAGE_POSITIONT && elements[x].UsageIndex == 0)
		{
			skipVertexProcessing = true;
			foundPositionT0 = &elements[x];
			break;
		}
	}

	// Compute hasColor0 and hasColor1:
	for (unsigned x = 0; x < numElements; ++x)
	{
		const DebuggableD3DVERTEXELEMENT9& thisElement = elements[x];
		if (thisElement.Usage == D3DDECLUSAGE_COLOR)
		{
			if (thisElement.UsageIndex == 0)
				hasColor0 = true;
			else if (thisElement.UsageIndex == 1)
				hasColor1 = true;
		}
	}

	USHORT currentOffset = 0;

	{
		DebuggableD3DVERTEXELEMENT9 newElement = elements[0];
		newElement.Usage = D3DDECLUSAGE_POSITIONT;
		newElement.Type = D3DDECLTYPE_FLOAT4;
		newElement.Offset = 0;
		newElement.Stream = 0;
		vertShaderOutputElements.push_back(newElement);
		currentOffset += GetElementSizeFromType(newElement.Type);
	}

	for (unsigned x = 0; x < numElements; ++x)
	{
		const DebuggableD3DVERTEXELEMENT9& thisElement = elements[x];
		if (thisElement.Usage == D3DDECLUSAGE_POSITION || thisElement.Usage == D3DDECLUSAGE_POSITIONT)
			continue; // Skip these
		else
		{
			DebuggableD3DVERTEXELEMENT9 newElement = thisElement;
			newElement.Stream = 0;
			newElement.Offset = currentOffset;
			vertShaderOutputElements.push_back(newElement);
			currentOffset += GetElementSizeFromType(newElement.Type);
		}
	}

	vertShaderOutputElements.push_back(D3DDECL_END() );

	if (FAILED(parentDevice->GetUnderlyingDevice()->CreateVertexDeclaration( (const D3DVERTEXELEMENT9* const)&vertShaderOutputElements.front(), &vertShaderOutputDecl) ) )
	{
		__debugbreak();
	}

	ComputeVertexSizes();
}

void IDirect3DVertexDeclaration9Hook::ComputeVertexSizes()
{
	{
		UINT totalSize = 0;
		const unsigned numElements = elements.size();
		for (unsigned x = 0; x < numElements; ++x)
		{
			const DebuggableD3DVERTEXELEMENT9& thisElement = elements[x];
			if (thisElement.Type != MAXD3DDECLTYPE)
			{
				totalSize += GetElementSizeFromType(thisElement.Type);
			}
		}
		inVertexSize = totalSize;
	}

	{
		UINT totalSize = 0;
		const unsigned numElements = vertShaderOutputElements.size();
		for (unsigned x = 0; x < numElements; ++x)
		{
			const DebuggableD3DVERTEXELEMENT9& thisElement = vertShaderOutputElements[x];
			if (thisElement.Type != MAXD3DDECLTYPE)
			{
				totalSize += GetElementSizeFromType(thisElement.Type);
			}
		}
		outVertexSize = totalSize;
	}
}

const UINT IDirect3DVertexDeclaration9Hook::GetElementSizeFromType(const D3DDECLTYPE elementType)
{
	switch (elementType)
	{
	case D3DDECLTYPE_FLOAT1    :
		return sizeof(float);
	case D3DDECLTYPE_FLOAT2    :
		return 2 * sizeof(float);
	case D3DDECLTYPE_FLOAT3    :
		return 3 * sizeof(float);
	case D3DDECLTYPE_FLOAT4    :
		return 4 * sizeof(float);
	case D3DDECLTYPE_D3DCOLOR  :
		return sizeof(D3DCOLOR);		                      
	case D3DDECLTYPE_UBYTE4    :
	case D3DDECLTYPE_UBYTE4N   :
		return 4 * sizeof(BYTE);
	case D3DDECLTYPE_SHORT2    :
	case D3DDECLTYPE_SHORT2N   :
	case D3DDECLTYPE_USHORT2N  :
		return 2 * sizeof(SHORT);
	case D3DDECLTYPE_SHORT4    :
	case D3DDECLTYPE_SHORT4N   :
	case D3DDECLTYPE_USHORT4N  :
		return 4 * sizeof(SHORT);		
	case D3DDECLTYPE_UDEC3     :
	case D3DDECLTYPE_DEC3N     :
		return sizeof(DWORD);
	case D3DDECLTYPE_FLOAT16_2 :
		return 2 * sizeof(D3DXFLOAT16);
	case D3DDECLTYPE_FLOAT16_4 :
		return 4 * sizeof(D3DXFLOAT16);
	default:
	case D3DDECLTYPE_UNUSED    :
#ifdef _DEBUG
		__debugbreak();
#endif
		return 0;
	}
}

/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexDeclaration9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	HRESULT ret = realObject->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DVertexDeclaration9Hook::AddRef(THIS)
{
	ULONG ret = realObject->AddRef();
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DVertexDeclaration9Hook::Release(THIS)
{
	ULONG ret = realObject->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked Vertex Declaration %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

/*** IDirect3DVertexDeclaration9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexDeclaration9Hook::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
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

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexDeclaration9Hook::GetDeclaration(THIS_ D3DVERTEXELEMENT9* pElement,UINT* pNumElements)
{
	HRESULT ret = realObject->GetDeclaration(pElement, pNumElements);
	return ret;
}

UINT IDirect3DVertexDeclaration9Hook::GetStream0Float4PositionTOffset(void) const
{
	const unsigned numOutputElements = vertShaderOutputElements.size();
	for (unsigned x = 0; x < numOutputElements; ++x)
	{
		const DebuggableD3DVERTEXELEMENT9& thisElement = vertShaderOutputElements[x];
		if (thisElement.Usage == D3DDECLUSAGE_POSITIONT && thisElement.Type == D3DDECLTYPE_FLOAT4 && thisElement.Stream == 0)
			return thisElement.Offset;
	}

	// Should never be here!
	__debugbreak();
	return 0;
}

// Returns true if vertex offset is found, otherwise returns false and then vertOffset and streamIndex are undefined
const bool IDirect3DVertexDeclaration9Hook::GetVertexOffset(unsigned& vertOffset, unsigned& streamIndex, const D3DDECLUSAGE usage, const unsigned usageIndex) const
{
	const DebuggableD3DVERTEXELEMENT9* const foundElement = GetVertexElement(usage, usageIndex);
	if (!foundElement)
	{
		vertOffset = 0;
		streamIndex = 0;
		return false;
	}
	else
	{
		vertOffset = foundElement->Offset;
		streamIndex = foundElement->Stream;
		return true;
	}
}

// Returns NULL if no such element is found
const DebuggableD3DVERTEXELEMENT9* const IDirect3DVertexDeclaration9Hook::GetVertexElement(const D3DDECLUSAGE usage, const unsigned usageIndex) const
{
	const unsigned numElements = elements.size();
	for (unsigned x = 0; x < numElements; ++x)
	{
		const DebuggableD3DVERTEXELEMENT9& thisElement = elements[x];
		if (thisElement.Usage == usage && thisElement.UsageIndex == usageIndex)
		{
			return &thisElement;
		}
	}

	return NULL;
}

// Returns NULL if no such element is found
const DebuggableD3DVERTEXELEMENT9* const IDirect3DVertexDeclaration9Hook::GetVertexElementOutput(const D3DDECLUSAGE usage, const unsigned usageIndex) const
{
	const unsigned numElements = vertShaderOutputElements.size();
	for (unsigned x = 0; x < numElements; ++x)
	{
		const DebuggableD3DVERTEXELEMENT9& thisElement = vertShaderOutputElements[x];
		if (thisElement.Usage == usage && thisElement.UsageIndex == usageIndex)
		{
			return &thisElement;
		}
	}

	return NULL;
}
