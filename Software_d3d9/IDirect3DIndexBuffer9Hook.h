#pragma once

#include "IDirect3DDevice9Hook.h"
#include "IDirect3DResource9Hook.h"

class IDirect3DIndexBuffer9Hook : public IDirect3DIndexBuffer9
{
public:
	IDirect3DIndexBuffer9Hook(LPDIRECT3DINDEXBUFFER9 _realObject, IDirect3DDevice9Hook* _parentDevice) : realObject(_realObject), parentDevice(_parentDevice), refCount(1),
		InternalLength(0), InternalUsage(UsageNone), InternalFormat(D3DFMT_UNKNOWN), InternalPool(D3DPOOL_DEFAULT), lockCount(0), isSoftIndexBufferUP(false)
	{
		rawBytes.voidBytes = NULL;

#ifdef _DEBUG
		if (realObject)
			memcpy(&Name, &realObject->Name, (char*)&realObject - (char*)&Name);
		else
			memset(&Name, 0x00000000, (char*)&realObject - (char*)&Name);
#endif
	}

	inline LPDIRECT3DINDEXBUFFER9 GetUnderlyingIndexBuffer(void) const
	{
		return realObject;
	}

	virtual ~IDirect3DIndexBuffer9Hook()
	{
#ifdef INDEX_BUFFER_ALLOC_PAGE_NOACCESS
		if (!isSoftIndexBufferUP)
		{
			if (rawBytes.voidBytes)
			{
				VirtualFree(rawBytes.voidBytes, 0, MEM_RELEASE);
				rawBytes.voidBytes = NULL;
			}
		}
#else
		if (isSoftIndexBufferUP)
		{
			if (rawBytes.voidBytes)
			{
				free(rawBytes.voidBytes);
				rawBytes.voidBytes = NULL;
			}
		}
#endif
		if (isSoftIndexBufferUP)
		{
			rawBytes.voidBytes = NULL;
			InternalFormat = D3DFMT_UNKNOWN;
		}

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
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDesc(THIS_ D3DINDEXBUFFER_DESC *pDesc) override;

	inline const bool IsUnlocked(void) const
	{
		return lockCount == 0;
	}

	void CreateIndexBuffer(UINT _Length, const DebuggableUsage _Usage, D3DFORMAT _Format, D3DPOOL _Pool);

	inline const D3DFORMAT GetFormat(void) const
	{
		return InternalFormat;
	}

	inline const BYTE* const GetBufferBytes(void) const
	{
		return rawBytes.u8Bytes;
	}

	inline const UINT GetInternalLength(void) const
	{
		return InternalLength;
	}

	inline void MarkSoftBufferUP(const bool isSoftUPIndexBuffer)
	{
		isSoftIndexBufferUP = isSoftUPIndexBuffer;
	}

	inline void SoftUPSetInternalPointer(const void* const stream0IndicesUP, const D3DFORMAT newFormat)
	{
#ifdef _DEBUG
		if (!isSoftIndexBufferUP)
		{
			__debugbreak(); // What are you doing calling this on a regular index buffer?
		}
		if (rawBytes.voidBytes != NULL)
		{
			__debugbreak(); // Probably forgot a reset somewhere!
		}
		if (stream0IndicesUP == NULL)
		{
			__debugbreak(); // This should never happen, draw calls with NULL index data are invalid
		}
		if (newFormat < D3DFMT_INDEX16 || newFormat > D3DFMT_INDEX32)
		{
			__debugbreak(); // Invalid index format
		}
#endif
		rawBytes.voidBytes = (BYTE* const)stream0IndicesUP;
		InternalFormat = newFormat;
	}

	inline void SoftUPResetInternalPointer(void)
	{
#ifdef _DEBUG
		if (!isSoftIndexBufferUP)
		{
			__debugbreak(); // What are you doing calling this on a regular index buffer?
		}
		if (rawBytes.voidBytes == NULL)
		{
			__debugbreak(); // You either did a reset twice in a row, or you forgot to set the pointer in the first place!
		}
#endif
		rawBytes.voidBytes = NULL;
		InternalFormat = D3DFMT_UNKNOWN;
	}

protected:
	LPDIRECT3DINDEXBUFFER9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

public:
	UINT InternalLength;
	DebuggableUsage InternalUsage;
	D3DFORMAT InternalFormat;
	D3DPOOL InternalPool;

	long lockCount;
	bool isSoftIndexBufferUP;

	union
	{
		void* voidBytes;
		BYTE* u8Bytes;
		unsigned short* shortBytes;
		unsigned long* longBytes;
		SIZE_T pointerNumber;
	} rawBytes;
};
