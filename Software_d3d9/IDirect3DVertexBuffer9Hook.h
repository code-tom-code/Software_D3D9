#pragma once

#include "IDirect3DDevice9Hook.h"

class IDirect3DVertexBuffer9Hook : public IDirect3DVertexBuffer9
{
public:
	IDirect3DVertexBuffer9Hook(LPDIRECT3DVERTEXBUFFER9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1),
		InternalLength(0), InternalUsage(UsageNone), InternalFVF(0x00000000), InternalPool(D3DPOOL_DEFAULT), lockCount(0), data(NULL), isSoftVertexBufferUP(false)
	{
#ifdef _DEBUG
		if (realObject)
			memcpy(&Name, &realObject->Name, (char*)&realObject - (char*)&Name);
		else
			memset(&Name, 0x00000000, (char*)&realObject - (char*)&Name);
#endif
	}

	inline LPDIRECT3DVERTEXBUFFER9 GetUnderlyingVertexBuffer(void) const
	{
		return realObject;
	}

	virtual ~IDirect3DVertexBuffer9Hook()
	{
#ifdef VERTEX_BUFFER_ALLOC_PAGE_NOACCESS
		if (!isSoftVertexBufferUP)
		{
			if (data)
			{
				VirtualFree(data, 0, MEM_RELEASE);
				data = NULL;
			}
		}
#else // #ifdef VERTEX_BUFFER_ALLOC_PAGE_NOACCESS
		if (!isSoftVertexBufferUP)
		{
			if (data)
			{
				free(data);
				data = NULL;
			}
		}
#endif
		if (isSoftVertexBufferUP)
			data = NULL;

#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) );
#endif
	}

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DResource9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE SetPrivateData(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetPrivateData(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE FreePrivateData(THIS_ REFGUID refguid) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE SetPriority(THIS_ DWORD PriorityNew) override;
    virtual COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE GetPriority(THIS) override;
    virtual COM_DECLSPEC_NOTHROW void STDMETHODCALLTYPE PreLoad(THIS) override;
    virtual COM_DECLSPEC_NOTHROW D3DRESOURCETYPE STDMETHODCALLTYPE GetType(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Lock(THIS_ UINT OffsetToLock,UINT SizeToLock,void** ppbData,DWORD Flags) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Unlock(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDesc(THIS_ D3DVERTEXBUFFER_DESC *pDesc) override;

	void CreateVertexBuffer(UINT _Length, const DebuggableUsage _Usage, DWORD _FVF, D3DPOOL _Pool);

	inline void MarkSoftBufferUP(const bool isSoftUPVertexBuffer)
	{
		isSoftVertexBufferUP = isSoftUPVertexBuffer;
	}

	inline void SoftUPSetInternalPointer(const BYTE* const stream0BytesUP, const UINT BufferLengthBytes)
	{
#ifdef _DEBUG
		if (!isSoftVertexBufferUP)
		{
			__debugbreak(); // What are you doing calling this on a regular vertex buffer?
		}
		if (data != NULL)
		{
			__debugbreak(); // Probably forgot a reset somewhere!
		}
		if (stream0BytesUP == NULL)
		{
			__debugbreak(); // This should never happen, draw calls with NULL stream0UP data are invalid
		}
#endif
		data = (BYTE* const)stream0BytesUP;
		InternalLength = BufferLengthBytes;
	}

	inline void SoftUPResetInternalPointer(void)
	{
#ifdef _DEBUG
		if (!isSoftVertexBufferUP)
		{
			__debugbreak(); // What are you doing calling this on a regular vertex buffer?
		}
		if (data == NULL)
		{
			__debugbreak(); // You either did a reset twice in a row, or you forgot to set the pointer in the first place!
		}
#endif
		data = NULL;
		InternalLength = 0;
	}

	inline const BYTE* const GetInternalDataBuffer(void) const
	{
		return data;
	}

	inline const UINT GetInternalLength_Bytes(void) const
	{
		return InternalLength;
	}

	inline const DWORD GetInternalFVF(void) const
	{
		return InternalFVF;
	}

	inline const bool IsUnlocked(void) const
	{
		return lockCount == 0;
	}

protected:
	LPDIRECT3DVERTEXBUFFER9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

	UINT InternalLength;
	DebuggableUsage InternalUsage;
	DWORD InternalFVF;
	D3DPOOL InternalPool;

	long lockCount;
	bool isSoftVertexBufferUP; // If this is true, then the memory pointed to by the buffer is managed by the application and should not be modified or freed!

	BYTE* data;
};
