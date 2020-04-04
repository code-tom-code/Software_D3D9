#pragma once

#include "IDirect3DQuery9Hook.h"

/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DQuery9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj)
{
	HRESULT ret = realObject->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DQuery9Hook::AddRef(THIS)
{
	ULONG ret = realObject->AddRef();
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DQuery9Hook::Release(THIS)
{
	ULONG ret = realObject->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked Query %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

/*** IDirect3DQuery9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DQuery9Hook::GetDevice(THIS_ IDirect3DDevice9** ppDevice)
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

COM_DECLSPEC_NOTHROW D3DQUERYTYPE STDMETHODCALLTYPE IDirect3DQuery9Hook::GetType(THIS)
{
#ifdef _DEBUG
	if (realObject->GetType() != queryType)
	{
		DbgBreakPrint("Error: Unsynchronized query types");
	}
#endif
	return queryType;
}

COM_DECLSPEC_NOTHROW DWORD STDMETHODCALLTYPE IDirect3DQuery9Hook::GetDataSize(THIS)
{
	DWORD ret = 0;
	switch (queryType)
	{
	case D3DQUERYTYPE_EVENT:
		ret = sizeof(BOOL);
		break;
	case D3DQUERYTYPE_OCCLUSION:
		ret = sizeof(DWORD);
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid query type specified");
#endif
	case D3DQUERYTYPE_VCACHE:
		ret = sizeof(D3DDEVINFO_VCACHE);
		break;
	case D3DQUERYTYPE_RESOURCEMANAGER:
		ret = sizeof(D3DDEVINFO_RESOURCEMANAGER);
		break;
	case D3DQUERYTYPE_VERTEXSTATS:
		ret = sizeof(D3DDEVINFO_D3DVERTEXSTATS);
		break;
	case D3DQUERYTYPE_TIMESTAMP:
		ret = sizeof(UINT64);
		break;
	case D3DQUERYTYPE_TIMESTAMPDISJOINT:
		ret = sizeof(BOOL);
		break;
	case D3DQUERYTYPE_TIMESTAMPFREQ:
		ret = sizeof(UINT64);
		break;
	case D3DQUERYTYPE_PIPELINETIMINGS:
		ret = sizeof(D3DDEVINFO_D3D9PIPELINETIMINGS);
		break;
	case D3DQUERYTYPE_INTERFACETIMINGS:
		ret = sizeof(D3DDEVINFO_D3D9INTERFACETIMINGS);
		break;
	case D3DQUERYTYPE_VERTEXTIMINGS:
		ret = sizeof(D3DDEVINFO_D3D9STAGETIMINGS);
		break;
	case D3DQUERYTYPE_PIXELTIMINGS: // Uhhhhhhhhhh, I guess this does the same thing as VERTEXTIMINGS?
		ret = sizeof(D3DDEVINFO_D3D9STAGETIMINGS);
		break;
	case D3DQUERYTYPE_BANDWIDTHTIMINGS:
		ret = sizeof(D3DDEVINFO_D3D9BANDWIDTHTIMINGS);
		break;
	case D3DQUERYTYPE_CACHEUTILIZATION:
		ret = sizeof(D3DDEVINFO_D3D9CACHEUTILIZATION);
		break;
	case D3DQUERYTYPE_MEMORYPRESSURE:
		ret = sizeof(D3DMEMORYPRESSURE);
		break;
	}

#ifdef _DEBUG
	if (ret != realObject->GetDataSize() )
	{
		DbgBreakPrint("Error: Mismatched query sizes");
	}
#endif
	return ret;
}

static inline const bool QueryTypeSupportsIssueBegin(const D3DQUERYTYPE TestType)
{
	switch (TestType)
	{
	default:
#ifdef _DEBUG
	{
		__debugbreak(); // Should never be here!
	}
#endif
	case D3DQUERYTYPE_EVENT:
	case D3DQUERYTYPE_RESOURCEMANAGER:
	case D3DQUERYTYPE_TIMESTAMP:
	case D3DQUERYTYPE_TIMESTAMPFREQ:
	case D3DQUERYTYPE_VCACHE:
	case D3DQUERYTYPE_VERTEXSTATS:
		return false;
	case D3DQUERYTYPE_OCCLUSION:
	case D3DQUERYTYPE_TIMESTAMPDISJOINT:
	case D3DQUERYTYPE_PIPELINETIMINGS:
	case D3DQUERYTYPE_INTERFACETIMINGS:
	case D3DQUERYTYPE_VERTEXTIMINGS:
	case D3DQUERYTYPE_PIXELTIMINGS:
	case D3DQUERYTYPE_BANDWIDTHTIMINGS:
	case D3DQUERYTYPE_CACHEUTILIZATION:
	case D3DQUERYTYPE_MEMORYPRESSURE:
		return true;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DQuery9Hook::Issue(THIS_ DWORD dwIssueFlags)
{
	if (dwIssueFlags > D3DISSUE_BEGIN)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid argument to IDirect3DQuery9::Issue()!");
#endif
		return D3DERR_INVALIDCALL;
	}

	if (dwIssueFlags & D3DISSUE_BEGIN)
	{
		if (!QueryTypeSupportsIssueBegin(queryType) )
		{
#ifdef _DEBUG
			DbgBreakPrint("Error: Can't call Issue(D3DISSUE_BEGIN) on this query type!");
#endif
			return D3DERR_INVALIDCALL;
		}
	}

#ifdef _DEBUG
	if (FAILED(realObject->Issue(dwIssueFlags) ) )
	{
		DbgBreakPrint("Error: Mismatched issue error code");
	}
#endif

	if (dwIssueFlags & D3DISSUE_BEGIN)
	{
		occlusionQueryStartPixelsPassed_Begin = parentDevice->OcclusionQuery_GetNumPixelsPassedZTest();
	}
	if (dwIssueFlags & D3DISSUE_END)
	{
		occlusionQueryStartPixelsPassed_End = parentDevice->OcclusionQuery_GetNumPixelsPassedZTest();
	}
	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DQuery9Hook::GetData(THIS_ void* pData, DWORD dwSize, DWORD dwGetDataFlags)
{
	// If you set dwSize to zero, you can use this method to poll the resource for the query status. pData may be NULL only if dwSize is 0.
	// The method returns S_OK if the query data is available and S_FALSE if it is not. These are considered successful return values.
	if (!pData && dwSize == 0)
	{
		return S_OK;
	}

	if (!pData)
	{
		return D3DERR_INVALIDCALL;
	}

	const DWORD requiredMinimumDataSize = GetDataSize();
	if (dwSize < requiredMinimumDataSize)
	{
		return D3DERR_INVALIDCALL;
	}

	if (dwGetDataFlags > D3DGETDATA_FLUSH)
	{
		return D3DERR_INVALIDCALL;
	}

#ifdef _DEBUG
	if (FAILED(realObject->GetData(pData, dwSize, dwGetDataFlags) ) )
	{
		DbgBreakPrint("Error: Mismatched GetData error code");
	}
#endif

	switch (queryType)
	{
	case D3DQUERYTYPE_EVENT:
	{
		BOOL* const bData = (BOOL* const)pData;
		*bData = TRUE;
		break;
	}
	case D3DQUERYTYPE_OCCLUSION:
	{
		DWORD* const dwData = (DWORD* const)pData;
		*dwData = (occlusionQueryStartPixelsPassed_End - occlusionQueryStartPixelsPassed_Begin);
		break;
	}
	case D3DQUERYTYPE_TIMESTAMP:
	{
		LARGE_INTEGER* const timestampData = (LARGE_INTEGER* const)pData;
		QueryPerformanceCounter(timestampData);
		break;
	}
	case D3DQUERYTYPE_TIMESTAMPDISJOINT:
	{
		BOOL* const timestampDisjointData = (BOOL* const)pData;
		*timestampDisjointData = FALSE; // Timestamp data is never disjoint for a purely CPU renderer because API's like QueryPerformanceCounter() will correct for changes in CPU frequency automatically.
		break;
	}
	case D3DQUERYTYPE_TIMESTAMPFREQ:
	{
		LARGE_INTEGER* const timestampFreq = (LARGE_INTEGER* const)pData;
		QueryPerformanceFrequency(timestampFreq);
		break;
	}
	case D3DQUERYTYPE_VCACHE:
	case D3DQUERYTYPE_RESOURCEMANAGER:
	case D3DQUERYTYPE_VERTEXSTATS:
	case D3DQUERYTYPE_PIPELINETIMINGS:
	case D3DQUERYTYPE_INTERFACETIMINGS:
	case D3DQUERYTYPE_VERTEXTIMINGS:
	case D3DQUERYTYPE_PIXELTIMINGS:
	case D3DQUERYTYPE_BANDWIDTHTIMINGS:
	case D3DQUERYTYPE_CACHEUTILIZATION:
	case D3DQUERYTYPE_MEMORYPRESSURE:
		break; // Not yet handled
	default:
#ifdef _DEBUG
	{
		__debugbreak(); // Should never be here
	}
#else
		__assume(0);
#endif
	}

	return S_OK;
}

void IDirect3DQuery9Hook::CreateQuery(const D3DQUERYTYPE _queryType)
{
	queryType = _queryType;

	switch (queryType)
	{
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid query type specified");
#endif
	case D3DQUERYTYPE_VCACHE:
	case D3DQUERYTYPE_EVENT:
	case D3DQUERYTYPE_OCCLUSION:
	case D3DQUERYTYPE_RESOURCEMANAGER:
	case D3DQUERYTYPE_VERTEXSTATS:
	case D3DQUERYTYPE_TIMESTAMP:
	case D3DQUERYTYPE_TIMESTAMPDISJOINT:
	case D3DQUERYTYPE_TIMESTAMPFREQ:
	case D3DQUERYTYPE_PIPELINETIMINGS:
	case D3DQUERYTYPE_INTERFACETIMINGS:
	case D3DQUERYTYPE_VERTEXTIMINGS:
	case D3DQUERYTYPE_BANDWIDTHTIMINGS:
	case D3DQUERYTYPE_CACHEUTILIZATION:
	case D3DQUERYTYPE_MEMORYPRESSURE:
	case D3DQUERYTYPE_PIXELTIMINGS:
		break;
	}
}
