#pragma once

#include "IDirect3DDevice9Hook.h"

class IDirect3DVertexDeclaration9Hook : public IDirect3DVertexDeclaration9
{
public:
	IDirect3DVertexDeclaration9Hook(LPDIRECT3DVERTEXDECLARATION9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1),
		inVertexSize(0), outVertexSize(0), foundPositionT0(NULL), skipVertexProcessing(false), hasColor0(false), hasColor1(false)
	{
		vertDeclAutoCreatedFromFVF.rawFVF_DWORD = 0x00000000;
#ifdef _DEBUG
		memcpy(&CreationCallStack, &realObject->CreationCallStack, (char*)&realObject - (char*)&CreationCallStack);
#endif
	}

	inline LPDIRECT3DVERTEXDECLARATION9 GetUnderlyingVertexDeclaration(void) const
	{
		return realObject;
	}

	LPDIRECT3DVERTEXDECLARATION9 GetUnderlyingPostTransformVertexDeclaration(void) const
	{
		return vertShaderOutputDecl;
	}

	virtual ~IDirect3DVertexDeclaration9Hook()
	{
		// TODO: Remove this vertex decl from the FVFToVertDeclCache when it gets fully released

#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) - (sizeof(elements) + sizeof(vertShaderOutputElements) ) );
#endif
	}

	const std::vector<DebuggableD3DVERTEXELEMENT9>& GetElementsInternal(void) const
	{
		return elements;
	}

	const std::vector<DebuggableD3DVERTEXELEMENT9>& GetOutputElementsInternal(void) const
	{
		return vertShaderOutputElements;
	}

	// Returns true if this vertex decl contains pretransformed vertices
	inline const bool SkipVertexProcessing(void) const
	{
		return skipVertexProcessing;
	}

	// This may return NULL in the case that this is not a "pre transformed" vertex decl
	inline const DebuggableD3DVERTEXELEMENT9* const GetPositionT0Element(void) const
	{
		return foundPositionT0;
	}

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

	/*** IDirect3DVertexDeclaration9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDeclaration(THIS_ D3DVERTEXELEMENT9* pElement,UINT* pNumElements) override;

	void CreateVertexDeclaration(const DebuggableD3DVERTEXELEMENT9* const pVertexElements, const debuggableFVF _vertDeclAutoCreatedFromFVF);

	inline const UINT GetVertexSize(void) const
	{
		return inVertexSize;
	}

	inline const UINT GetPostTransformVertexSize(void) const
	{
		return outVertexSize;
	}

	static const UINT GetElementSizeFromType(const D3DDECLTYPE elementType);

	// Returns true if vertex offset is found, otherwise returns false and then vertOffset and streamIndex undefined
	const bool GetVertexOffset(unsigned& vertOffset, unsigned& streamIndex, const D3DDECLUSAGE usage, const unsigned usageIndex) const;

	// Returns NULL if no such element is found
	const DebuggableD3DVERTEXELEMENT9* const GetVertexElement(const D3DDECLUSAGE usage, const unsigned usageIndex) const;
	const DebuggableD3DVERTEXELEMENT9* const GetVertexElementOutput(const D3DDECLUSAGE usage, const unsigned usageIndex) const;

	UINT GetStream0Float4PositionTOffset(void) const;

	inline const bool GetHasCOLOR0(void) const
	{
		return hasColor0;
	}

	inline const bool GetHasCOLOR1(void) const
	{
		return hasColor1;
	}

protected:
	LPDIRECT3DVERTEXDECLARATION9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

	// This may be NULL for decl's that don't specify a POSITIONT0 element
	const DebuggableD3DVERTEXELEMENT9* foundPositionT0;

	// This is the case if someone passes in an element with POSITIONT in it
	bool skipVertexProcessing;

	// These two bools determine if the COLOR0 and COLOR1 semantics are present for this vertex decl
	bool hasColor0;
	bool hasColor1;

	debuggableFVF vertDeclAutoCreatedFromFVF;

	unsigned inVertexSize;
	unsigned outVertexSize;

	LPDIRECT3DVERTEXDECLARATION9 vertShaderOutputDecl;

	std::vector<DebuggableD3DVERTEXELEMENT9> elements;
	std::vector<DebuggableD3DVERTEXELEMENT9> vertShaderOutputElements;

	void ComputeVertexSizes();
};
