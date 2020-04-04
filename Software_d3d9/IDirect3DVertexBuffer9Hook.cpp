#pragma once

#include "IDirect3DVertexBuffer9Hook.h"
#include "IDirect3DResource9Hook.h"
#include "GlobalToggles.h"

void IDirect3DVertexBuffer9Hook::CreateVertexBuffer(UINT _Length, const DebuggableUsage _Usage, DWORD _FVF, D3DPOOL _Pool)
{
	InternalLength = _Length;
	InternalUsage = _Usage;
	InternalFVF = _FVF;
	InternalPool = _Pool;

	const unsigned magicDWORDLen =
#ifdef VERTEX_BUFFER_MAGIC_COOKIE
		sizeof(DWORD);
#else
		0;
#endif

#ifdef VERTEX_BUFFER_ALLOC_PAGE_NOACCESS
	data = PageAllocWithNoAccessPage(InternalLength + magicDWORDLen);
#else
	data = (BYTE* const)malloc(InternalLength + magicDWORDLen);
#endif
	if (!data)
	{
		__debugbreak(); // Can't alloc our vertex buffer!
	}

#ifdef VERTEX_BUFFER_MAGIC_COOKIE
	*(DWORD* const)&data[InternalLength] = 'VRTX';
#endif
}

#ifdef VERTEX_BUFFER_MAGIC_COOKIE
static inline void ValidateMagicCookie(const std::vector<unsigned char>& bytes, const unsigned length)
{
	if (*(const DWORD* const)&bytes[length] != 'VRTX')
	{
		__debugbreak();
	}
}
#endif // VERTEX_BUFFER_MAGIC_COOKIE

/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	if (realObject)
	{
		HRESULT ret = realObject->QueryInterface(riid, ppvObj);
		if (ret == NOERROR)
		{
			*ppvObj = this;
			AddRef();
		}
		return ret;
	}
	// TODO: Fix this
	return S_OK;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::AddRef(THIS)
{
	ULONG ret = realObject ? realObject->AddRef() : (const ULONG)(refCount + 1);
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::Release(THIS)
{
	ULONG ret = realObject ? realObject->Release() : (const ULONG)(refCount - 1);
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked Vertex Buffer %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

/*** IDirect3DResource9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
{
	if (!ppDevice)
		return D3DERR_INVALIDCALL;

	if (realObject)
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
	else
	{
		*ppDevice = parentDevice;
		return S_OK;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::SetPrivateData(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
{
	if (realObject)
	{
		HRESULT ret = realObject->SetPrivateData(refguid, pData, SizeOfData, Flags);
		return ret;
	}
	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::GetPrivateData(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData)
{
	if (realObject)
	{
		HRESULT ret = realObject->GetPrivateData(refguid, pData, pSizeOfData);
		return ret;
	}
	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::FreePrivateData(THIS_ REFGUID refguid)
{
	if (realObject)
	{
		HRESULT ret = realObject->FreePrivateData(refguid);
		return ret;
	}
	return S_OK;
}

COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::SetPriority(THIS_ DWORD PriorityNew)
{
	if (realObject)
	{
		DWORD ret = realObject->SetPriority(PriorityNew);
		return ret;
	}
	return 0;
}

COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::GetPriority(THIS)
{
	if (realObject)
	{
		DWORD ret = realObject->GetPriority();
		return ret;
	}
	return S_OK;
}

COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::PreLoad(THIS)
{
	if (realObject)
		realObject->PreLoad();
}

COM_DECLSPEC_NOTHROW D3DRESOURCETYPE STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::GetType(THIS)
{
	if (realObject)
	{
		D3DRESOURCETYPE ret = realObject->GetType();
		if (ret != D3DRTYPE_VERTEXBUFFER)
		{
			__debugbreak(); // Huh?
		}
		return ret;
	}
	return D3DRTYPE_VERTEXBUFFER;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::Lock(THIS_ UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags)
{
	if (!ppbData)
		return D3DERR_INVALIDCALL;

	const DWORD validLockFlags = D3DLOCK_DISCARD | D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY | D3DLOCK_NOOVERWRITE; 
	if (Flags & (~validLockFlags) )
		return D3DERR_INVALIDCALL; // These are the only D3DLOCK flags valid for this function call

	if (SizeToLock == 0 && OffsetToLock > 0)
		return D3DERR_INVALIDCALL;

	if (OffsetToLock + SizeToLock > InternalLength)
		return D3DERR_INVALIDCALL;

	if (Flags & (D3DLOCK_DISCARD | D3DLOCK_NOOVERWRITE) )
	{
		// D3D9 in Release mode ignores this error without returning a failure code
		if (!(InternalUsage & D3DUSAGE_DYNAMIC) )
			return D3DERR_INVALIDCALL;
	}

#ifdef VERTEX_BUFFER_MAGIC_COOKIE
	ValidateMagicCookie(data, InternalLength);
#endif

#ifdef VERTEX_BUFFER_ENFORCE_READONLY_WHILE_UNLOCKED
	if (IsUnlocked() )
	{
		BYTE* const pageStartAddr = (BYTE* const)( ( (SIZE_T)data) - ( (SIZE_T)data % 4096) );
		VirtualAlloc(pageStartAddr, InternalLength, MEM_COMMIT, PAGE_READWRITE);
	}
#endif

#ifdef VERTEX_BUFFER_ENFORCE_DISCARD_ON_LOCK
	if ( (InternalUsage & D3DUSAGE_DYNAMIC) && (Flags & D3DLOCK_DISCARD) )
	{
		memset(data, 0, InternalLength);
	}
#endif

	if (OffsetToLock == 0 && SizeToLock == 0)
		*ppbData = data;//&(data.front() );
	else
		*ppbData = /*&(data.front() )*/data + OffsetToLock;

#ifdef _DEBUG
	if (realObject)
	{
		void* tempLockPtr = NULL;
		HRESULT ret = realObject->Lock(OffsetToLock, SizeToLock, &tempLockPtr, Flags);
		if (FAILED(ret) )
		{
			// There was an error that we should've caught but didn't
			__debugbreak();
			return ret;
		}
	}
#endif

	++lockCount;

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::Unlock(THIS)
{
#ifdef _DEBUG
	if (realObject)
	{
		HRESULT ret = realObject->Unlock();
		if (FAILED(ret) )
		{
			__debugbreak(); // There was an error somewhere that we should've caught
			return ret;
		}
	}
#endif

#ifdef VERTEX_BUFFER_MAGIC_COOKIE
	ValidateMagicCookie(data, InternalLength);
#endif

	--lockCount;

#ifdef VERTEX_BUFFER_ENFORCE_READONLY_WHILE_UNLOCKED
	if (IsUnlocked() )
	{
		BYTE* const pageStartAddr = (BYTE* const)( ( (SIZE_T)data) - ( (SIZE_T)data % 4096) );
		VirtualAlloc(pageStartAddr, InternalLength, MEM_COMMIT, PAGE_READONLY);
	}
#endif

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DVertexBuffer9Hook::GetDesc(THIS_ D3DVERTEXBUFFER_DESC *pDesc)
{
	if (!pDesc)
		return D3DERR_INVALIDCALL;

#ifdef _DEBUG
	if (realObject)
	{
		HRESULT ret = realObject->GetDesc(pDesc);
		if (FAILED(ret) )
		{
			__debugbreak(); // Should've been an error but we didn't find one
			return ret;
		}

		if (pDesc->Size != InternalLength)
		{
			__debugbreak();
		}
		if (pDesc->Usage != InternalUsage)
		{
			__debugbreak();
		}
		if (pDesc->FVF != InternalFVF)
		{
			__debugbreak();
		}
		if (pDesc->Pool != InternalPool)
		{
			__debugbreak();
		}
		if (pDesc->Format != D3DFMT_VERTEXDATA)
		{
			__debugbreak();
		}
		if (pDesc->Type != D3DRTYPE_VERTEXBUFFER)
		{
			__debugbreak();
		}
	}
#endif

	pDesc->Size = InternalLength;
	pDesc->Usage = InternalUsage;
	pDesc->FVF = InternalFVF;
	pDesc->Pool = InternalPool;
	pDesc->Format = D3DFMT_VERTEXDATA;
	pDesc->Type = D3DRTYPE_VERTEXBUFFER;

	return S_OK;
}
