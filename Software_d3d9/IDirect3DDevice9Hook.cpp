#pragma once

#include "GlobalToggles.h"
#include "IDirect3DDevice9Hook.h"
#include "IDirect3DIndexBuffer9Hook.h"
#include "IDirect3DVertexBuffer9Hook.h"
#include "IDirect3DVertexShader9Hook.h"
#include "IDirect3DPixelShader9Hook.h"
#include "IDirect3DVertexDeclaration9Hook.h"
#include "IDirect3DSurface9Hook.h"
#include "IDirect3DSwapChain9Hook.h"
#include "IDirect3DTexture9Hook.h"
#include "IDirect3DStateBlock9Hook.h"
#include "FixedFunctionToShader.h"
#include "SemanticMappings.h"
#include "resource.h"

#ifdef MULTITHREAD_SHADING
	#if PARALLEL_LIBRARY == PARALLELLIB_CONCRT
		#include <concrt.h>
		#include <ppltasks.h>
		#include <ppl.h>
	#elif PARALLEL_LIBRARY == PARALLELLIB_TBB
		#include <include/tbb/parallel_for.h>
	#endif
#endif

// Rasterizer constants:
// TODO: Fix bug in core rasterizer loop when this is tuned > 0 (ie, when subpixel precision is enabled)
static const unsigned SUBPIXEL_ACCURACY_BITS = 0u;
static const unsigned SUBPIXEL_ACCURACY_BIASMULT = 1 << SUBPIXEL_ACCURACY_BITS;
static const int SUBPIXEL_MAX_VALUE = MAXINT >> SUBPIXEL_ACCURACY_BITS;
static const int SUBPIXEL_MIN_VALUE = MININT >> SUBPIXEL_ACCURACY_BITS;
static const float SUBPIXEL_MAX_VALUEF = 8191.0f;//(const float)SUBPIXEL_MAX_VALUE;
static const float SUBPIXEL_MIN_VALUEF = -8191.0f;//(const float)SUBPIXEL_MIN_VALUE;
static const float SUBPIXEL_ACCURACY_BIASMULTF = (const float)SUBPIXEL_ACCURACY_BIASMULT;
static const D3DXVECTOR4 zeroVec(0.0f, 0.0f, 0.0f, 0.0f);
static const D3DXVECTOR4 vertShaderInputRegisterDefault(0.0f, 0.0f, 0.0f, 1.0f);

static const D3DXVECTOR4 staticColorWhiteOpaque(1.0f, 1.0f, 1.0f, 1.0f);
static const D3DXVECTOR4 staticColorBlackTranslucent(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
	static __int64 totalVertexShadeTicks = 0;
	static __int64 numVertexShadeTasks = 0;
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

#ifdef PROFILE_AVERAGE_PIXEL_SHADE_TIMES
	static __int64 totalPixelShadeTicks = 0;
	static __int64 numPixelShadeTasks = 0;
#endif // #ifdef PROFILE_AVERAGE_PIXEL_SHADE_TIMES

extern HINSTANCE hLThisDLL;

#ifdef MULTITHREAD_SHADING
#define NUM_THREADS 16

#define NUM_JOBS_PER_PIXEL 4
static unsigned MAX_NUM_JOBS = 0;

static volatile struct _workStatus
{
	volatile long numJobs; // Read by worker threads, written by driver thread
	char cacheLinePadding[64];
	volatile long numFinishedJobs; // Read + written by worker threads, read by driver thread
	char cacheLinePadding2[64];
	volatile long currentJobID; // Read + written by worker threads
	char cacheLinePadding3[64];
	unsigned currentWorkID; // Read + written by driver thread only
} workStatus = {0};

struct _threadItem
{
	IDirect3DDevice9Hook* devHook;
	HANDLE hThread;
	VShaderEngine threadVS_2_0;
	PShaderEngine threadPS_2_0;
} threadItem [NUM_THREADS * 2 + 1] = {0};

// This is volatile and aligned because it will be used with Interlocked operations
static volatile long __declspec(align(16) ) tlsThreadNumber = 0;

// Threadpool implementation was slower than single-threaded implementation in some cases
//static TP_CALLBACK_ENVIRON mainThreadpoolCallbackEnv = {0};
//static PTP_POOL mainThreadpool = NULL;
//static PTP_CLEANUP_GROUP mainThreadpoolCleanupGroup = NULL;

static_assert(sizeof(_threadItem) > 64, "Error: False sharing may occur if thread struct is smaller than a cache line!");

__declspec(align(16) ) struct slist_item
{
	workerJobType jobType;
	union _jobData
	{
		struct _vertexJobData
		{
			VS_2_0_OutputRegisters* outputRegs[4];
			UINT vertexIndex[4]; // This is the SV_VertexID semantic in vs_4_0
		} vertexJobData;
		struct _pixelJobData
		{
			const primitivePixelJobData* primitiveData;
			unsigned x;
			unsigned y;
			int barycentricA;
			int barycentricB;
			int barycentricC;
		} pixelJobData;
	} jobData;
};
static_assert(sizeof(slist_item) % 16 == 0, "Error, bad struct alignment!");

static slist_item* allWorkItems = NULL;
static DWORD tlsIndex = TLS_OUT_OF_INDEXES;

static inline void VertexShadeJob1(slist_item& job, _threadItem* const myPtr)
{
	const IDirect3DDevice9Hook* const devHook = myPtr->devHook;
	const drawCallVertexJobData& drawCallData = devHook->currentDrawCallData.vertexData;
	if (drawCallData.userClipPlanesEnabled)
	{
		devHook->ProcessVertexToBuffer<true>(*(drawCallData.mapping), &myPtr->threadVS_2_0, job.jobData.vertexJobData.outputRegs[0], job.jobData.vertexJobData.vertexIndex[0]);
	}
	else
	{
		devHook->ProcessVertexToBuffer<false>(*(drawCallData.mapping), &myPtr->threadVS_2_0, job.jobData.vertexJobData.outputRegs[0], job.jobData.vertexJobData.vertexIndex[0]);
	}
}

#ifdef RUN_SHADERS_IN_WARPS
static inline void VertexShadeJob4(slist_item& job, _threadItem* const myPtr)
{
	const IDirect3DDevice9Hook* const devHook = myPtr->devHook;
	const drawCallVertexJobData& drawCallData = devHook->currentDrawCallData.vertexData;
	if (drawCallData.userClipPlanesEnabled)
	{
		devHook->ProcessVertexToBuffer4<true>(*(drawCallData.mapping), &myPtr->threadVS_2_0, job.jobData.vertexJobData.outputRegs, job.jobData.vertexJobData.vertexIndex);
	}
	else
	{
		devHook->ProcessVertexToBuffer4<false>(*(drawCallData.mapping), &myPtr->threadVS_2_0, job.jobData.vertexJobData.outputRegs, job.jobData.vertexJobData.vertexIndex);
	}
}
#endif // #ifdef RUN_SHADERS_IN_WARPS

static inline void PixelShadeJob1(slist_item& job, _threadItem* const myPtr)
{
	const IDirect3DDevice9Hook* const devHook = myPtr->devHook;
	const drawCallPixelJobData& drawCallData = devHook->currentDrawCallData.pixelData;
	const slist_item::_jobData::_pixelJobData& pixelJobData = job.jobData.pixelJobData;
	const primitivePixelJobData* const primitiveData = pixelJobData.primitiveData;
	const D3DXVECTOR3 barycentricInterpolants(pixelJobData.barycentricA * primitiveData->barycentricNormalizeFactor,
		pixelJobData.barycentricB * primitiveData->barycentricNormalizeFactor,
		pixelJobData.barycentricC * primitiveData->barycentricNormalizeFactor);
	if (drawCallData.useShaderVerts)
	{
		const primitivePixelJobData::_pixelShadeVertexData::_shadeFromShader& vertsFromShader = primitiveData->pixelShadeVertexData.shadeFromShader;
		devHook->ShadePixelFromShader(&myPtr->threadPS_2_0, *(drawCallData.vs_to_ps_mappings.vs_psMapping), pixelJobData.x, pixelJobData.y, 
			barycentricInterpolants, drawCallData.offsetIntoVertexForOPosition_Bytes, *vertsFromShader.v0, *vertsFromShader.v1, *vertsFromShader.v2);
	}
	else
	{
		const primitivePixelJobData::_pixelShadeVertexData::_shadeFromStream& vertsFromStream = primitiveData->pixelShadeVertexData.shadeFromStream;
		devHook->ShadePixelFromStream(&myPtr->threadPS_2_0, *(drawCallData.vs_to_ps_mappings.vertexDeclMapping), pixelJobData.x, pixelJobData.y, 
			barycentricInterpolants, drawCallData.offsetIntoVertexForOPosition_Bytes, vertsFromStream.v0, vertsFromStream.v1, vertsFromStream.v2);
	}
}

/*VOID NTAPI WorkerThreadCallback(_Inout_ PTP_CALLBACK_INSTANCE Instance, _Inout_opt_ PVOID, _Inout_ PTP_WORK Work)
{
	while (true)
	{
		__declspec(align(16) ) PSLIST_ENTRY entry = InterlockedPopEntrySList(&workQueue);
		if (entry == NULL)
		{
			return;
		}

		const long myThreadId = (InterlockedIncrement(&jobID) % NUM_THREADS);
	//#ifdef _DEBUG
		if (myThreadId >= NUM_THREADS)
		{
			DbgBreakPrint("Error: Invalid threadID!");
		}
	//#endif

		_threadItem& myThreadItem = threadItem[myThreadId];

		//slist_item* const item = (slist_item* const)Context;
		slist_item* const item = (slist_item* const)entry;
		switch (item->jobType)
		{
			default:
				DbgBreakPrint("Error: Unknown job type!");
			case vertexShadeJob:
				VertexShadeJob(*item, &myThreadItem);
				break;
			case pixelShadeJob:
				PixelShadeJob(*item, &myThreadItem);
				break;
		}

		InterlockedPushEntrySList(&completedWorkQueue, &item->itemEntry);

		//InterlockedDecrement(&jobID);
	}
}*/

static inline void WorkUntilNoMoreWork(void* const jobData)
{
	//while (true)
	{
		// Some other thread has finished all of the work, we can just return
/*		if (workStatus.currentJobID >= workStatus.numJobs)
		{
			YieldProcessor();
			return;
		}

		const long jobID = InterlockedIncrement(&workStatus.currentJobID) - 1;
		if (jobID >= workStatus.numJobs)
		{
			InterlockedDecrement(&workStatus.currentJobID);
			YieldProcessor();
			return;
		}
		if (jobID < 0)
		{
			YieldProcessor();
			return;
		}*/

		void* tlsEntry = TlsGetValue(tlsIndex);
		if (tlsEntry == NULL)
		{
			const long thisThreadNumber = InterlockedIncrement(&tlsThreadNumber) - 1;
			if (thisThreadNumber >= ARRAYSIZE(threadItem) )
			{
				__debugbreak();
			}
			_threadItem& thisThreadItem = threadItem[thisThreadNumber];
			if (!TlsSetValue(tlsIndex, &thisThreadItem) )
			{
				__debugbreak();
			}
			tlsEntry = &thisThreadItem;
		}
		_threadItem* const myPtr = (_threadItem* const)tlsEntry;

		//slist_item* const item = &(allWorkItems[jobID]);
		slist_item* const item = (slist_item* const)jobData;

		//slist_item* const item = (slist_item* const)entry;
		switch (item->jobType)
		{
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Unknown job type!");
#else
			__assume(0);
#endif
		case vertexShade1Job:
			VertexShadeJob1(*item, myPtr);
			break;
#ifdef RUN_SHADERS_IN_WARPS
		case vertexShade4Job:
			VertexShadeJob4(*item, myPtr);
			break;
#endif // #ifdef RUN_SHADERS_IN_WARPS
		case pixelShadeJob:
			PixelShadeJob1(*item, myPtr);
			break;
		// Important TODO: pixelShadeJob4
		}

		//InterlockedPushEntrySList(&completedWorkQueue, entry);
		
		// The last job to finish the last work item sets the event that we're done
		/*if (InterlockedIncrement(&workStatus.numFinishedJobs) == workStatus.numJobs)
		{
			YieldProcessor();
			return;
		}*/

		/*if (InterlockedDecrement(&numWorkItems) <= 0)
		{
			SetEvent(notifyAllJobsCompletedEvent);
		}*/
	}
}

/*static inline DWORD WINAPI WorkerThreadMain(LPVOID ptr)
{
	_threadItem* const myPtr = (_threadItem* const)ptr;
	while (true)
	{
		//WaitForSingleObject(moreWorkReadyEvent, INFINITE);

		WorkUntilNoMoreWork(myPtr);
		//Sleep(1);
	}
	return 0;
}*/

//static TP_CALLBACK_ENVIRON poolEnvironment = {0};
//static TP_CLEANUP_GROUP* cleanup = NULL;
//static TP_POOL* threadPool = NULL;
//static TP_WORK* threadWork = NULL;

/*static inline void RefreshThreadpoolWork()
{
#ifdef _DEBUG
	threadWork = NULL;
#endif
	threadWork = CreateThreadpoolWork(&WorkerThreadCallback, NULL, &poolEnvironment);
	if (!threadWork) __debugbreak();
}*/

static inline void SynchronizeThreads()
{
	// WorkUntilNoMoreWork(&threadItem[NUM_THREADS]);
	//workStatus.numJobs = workStatus.currentWorkID;

	if (workStatus.numJobs == 0)
		return;

	//SetEvent(moreWorkReadyEvent);

	const long cacheNumJobs = workStatus.numJobs;
	if (cacheNumJobs <= 4) // Don't actually use multiple threads for incredibly small jobs, it's just a waste of scheduling time overall and it makes debugging more confusing than it has to be
	{
		for (long jobID = 0; jobID < cacheNumJobs; ++jobID)
		{
			WorkUntilNoMoreWork(allWorkItems + jobID);
		}
	}
	else
	{
#if PARALLEL_LIBRARY == PARALLELLIB_CONCRT
		concurrency::parallel_for( (const long)0, cacheNumJobs, [](const int jobID)
		{
			WorkUntilNoMoreWork(allWorkItems + jobID);
		});
#elif PARALLEL_LIBRARY == PARALLELLIB_TBB
		tbb::parallel_for( (const long)0, cacheNumJobs, [](const int jobID)
		{
			WorkUntilNoMoreWork(allWorkItems + jobID);
		});
#endif
	}

	//CloseThreadpoolCleanupGroupMembers(mainThreadpoolCleanupGroup, FALSE, NULL);
	//WaitForSingleObject(allJobsCompletedEvent, INFINITE);
	/*while (workStatus.numFinishedJobs != cacheNumJobs)
	{
		printf("", workStatus.numFinishedJobs, cacheNumJobs);
		__debugbreak();
		//YieldProcessor();
	}*/
	//ResetEvent(moreWorkReadyEvent);
	//ResetEvent(allJobsCompletedEvent);

	const long numJobsTotal = workStatus.numJobs;

	// The order of these assignments is important as these variables may be read by another thread
	workStatus.numJobs = 0;
	workStatus.numFinishedJobs = 0;
	workStatus.currentJobID = 0;
	workStatus.currentWorkID = 0;
}

static VOID NTAPI CleanupGroupCancelCallback(_Inout_opt_ PVOID ObjectContext, _Inout_opt_ PVOID CleanupContext)
{
	// Do nothing.
	// We'll probably never call this callback anyway
}

void IDirect3DDevice9Hook::InitThreadQueue()
{
	//InitializeSListHead(&completedWorkQueue);
	//InitializeSListHead(&workQueue);
	/*notifyAllJobsCompletedEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	if (notifyAllJobsCompletedEvent == NULL)
	{
		DbgBreakPrint("Error: Unable to create worker thread notify finish event!");
		__debugbreak(); // Yup, even in Release builds
	}

	notifyJobsBeginEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	if (notifyJobsBeginEvent == NULL)
	{
		DbgBreakPrint("Error: Unable to create worker thread notify event!");
		__debugbreak(); // Yup, even in Release builds
	}*/

	/*threadPool = CreateThreadpool(NULL);
	if (!threadPool) __debugbreak();
	SetThreadpoolThreadMaximum(threadPool, NUM_THREADS);
	if (!SetThreadpoolThreadMinimum(threadPool, NUM_THREADS) ) __debugbreak();
	InitializeThreadpoolEnvironment(&poolEnvironment);
	SetThreadpoolCallbackPool(&poolEnvironment, threadPool);
	cleanup = CreateThreadpoolCleanupGroup();
	if (!cleanup) __debugbreak();
	SetThreadpoolCallbackCleanupGroup(&poolEnvironment, cleanup, NULL);
	RefreshThreadpoolWork();*/

	/*allJobsCompletedEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	moreWorkReadyEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
	if (!allJobsCompletedEvent || !moreWorkReadyEvent)
	{
		__debugbreak();
	}*/

	tlsIndex = TlsAlloc();
	if (tlsIndex == TLS_OUT_OF_INDEXES)
	{
		__debugbreak(); // Uhhhh, we shouldn't run out of TLS slots unless something has gone really wrong...
	}

	for (unsigned x = 0; x < ARRAYSIZE(threadItem); ++x)
	{
		threadItem[x].devHook = this;
	}
	
	/*for (unsigned x = 0; x < NUM_THREADS; ++x)
	{
		threadItem[x].hThread = CreateThread(NULL, 0, &WorkerThreadMain, &(threadItem[x]), 0, NULL);
		SetThreadDescription(threadItem[x].hThread, L"SoftwareD3D9 Worker Thread");

		if (!threadItem[x].hThread)
		{
			DbgBreakPrint("Error: Unable to create worker thread!");
			__debugbreak(); // Yup, even in Release builds
		}
	}*/

	/*mainThreadpool = CreateThreadpool(NULL);
	if (!mainThreadpool)
	{
		__debugbreak();
	}

	if (!SetThreadpoolThreadMinimum(mainThreadpool, NUM_THREADS) )
	{
		__debugbreak();
	}

	SetThreadpoolThreadMaximum(mainThreadpool, NUM_THREADS);

	InitializeThreadpoolEnvironment(&mainThreadpoolCallbackEnv);
	SetThreadpoolCallbackPool(&mainThreadpoolCallbackEnv, mainThreadpool);

	mainThreadpoolCleanupGroup = CreateThreadpoolCleanupGroup();
	if (!mainThreadpoolCleanupGroup)
	{
		__debugbreak();
	}
	SetThreadpoolCallbackCleanupGroup(&mainThreadpoolCallbackEnv, mainThreadpoolCleanupGroup, &CleanupGroupCancelCallback);*/
}

#endif // MULTITHREAD_SHADING

// Returns true if the pixel "passes" the depth test (should be written) and false if the pixel "fails" the depth test (should be discarded)
static inline const bool DepthTest(const float pixelDepth, const unsigned bufferDepth, const D3DCMPFUNC comparison, const D3DFORMAT depthFmt)
{
	unsigned quantizedPixelDepth;
	switch (depthFmt)
	{
	case D3DFMT_D15S1:
		quantizedPixelDepth = (const unsigned)(pixelDepth * 32768.0f);
		break;
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
		quantizedPixelDepth = (const unsigned)(pixelDepth * 65536.0f);
		break;
	default:
		DbgBreakPrint("Error: Unknown Depth buffer format!");
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
		quantizedPixelDepth = (const unsigned)(pixelDepth * 16777216.0f);
		break;
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
		quantizedPixelDepth = (const unsigned)(pixelDepth * 4294967296.0f);
		break;
	case D3DFMT_D32F_LOCKABLE:
		quantizedPixelDepth = *(const unsigned* const)&pixelDepth;
		break;
	}

	switch (comparison)
	{
	case D3DCMP_NEVER       :
		return false;
	case D3DCMP_LESS        :
		return quantizedPixelDepth < bufferDepth;
	case D3DCMP_EQUAL       :
		return quantizedPixelDepth == bufferDepth;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Undefined D3DCMP specified for DepthTest");
#endif
	case D3DCMP_LESSEQUAL   :
		return quantizedPixelDepth <= bufferDepth;
	case D3DCMP_GREATER     :
		return quantizedPixelDepth > bufferDepth;
	case D3DCMP_NOTEQUAL    :
		return quantizedPixelDepth != bufferDepth;
	case D3DCMP_GREATEREQUAL:
		return quantizedPixelDepth >= bufferDepth;
	case D3DCMP_ALWAYS      :
		return true;
	}
}

// Most of these were pulled from d3dtypes.h, but some of them were pulled from d3d8.h as well
enum RETIRED_D3D_RENDERSTATES : DWORD
{
    D3DRENDERSTATE_TEXTUREHANDLE      = 1,    /* Texture handle for legacy interfaces (Texture,Texture2) */
	D3DRENDERSTATE_ANTIALIAS          = 2,    /* D3DANTIALIASMODE */
    D3DRENDERSTATE_TEXTUREADDRESS     = 3,    /* D3DTEXTUREADDRESS  */
	D3DRENDERSTATE_TEXTUREPERSPECTIVE = 4,    /* TRUE for perspective correction */
    D3DRENDERSTATE_WRAPU              = 5,    /* TRUE for wrapping in u */
    D3DRENDERSTATE_WRAPV              = 6,    /* TRUE for wrapping in v */
	D3DRENDERSTATE_LINEPATTERN        = 10,   /* D3DLINEPATTERN */
    D3DRENDERSTATE_MONOENABLE         = 11,   /* TRUE to enable mono rasterization */
    D3DRENDERSTATE_ROP2               = 12,   /* ROP2 */
    D3DRENDERSTATE_PLANEMASK          = 13,   /* DWORD physical plane mask */
    D3DRENDERSTATE_TEXTUREMAG         = 17,   /* D3DTEXTUREFILTER */
    D3DRENDERSTATE_TEXTUREMIN         = 18,   /* D3DTEXTUREFILTER */
    D3DRENDERSTATE_TEXTUREMAPBLEND    = 21,   /* D3DTEXTUREBLEND */
	D3DRENDERSTATE_ZVISIBLE           = 30,   /* TRUE to enable z checking */
    D3DRENDERSTATE_SUBPIXEL           = 31,   /* TRUE to enable subpixel correction */
    D3DRENDERSTATE_SUBPIXELX          = 32,   /* TRUE to enable correction in X only */
	D3DRENDERSTATE_STIPPLEDALPHA      = 33,   /* TRUE to enable stippled alpha (RGB device only) */
    D3DRENDERSTATE_STIPPLEENABLE      = 39,   /* TRUE to enable stippling */
#if(DIRECT3D_VERSION >= 0x0500)
    D3DRENDERSTATE_EDGEANTIALIAS      = 40,   /* TRUE to enable edge antialiasing */
    D3DRENDERSTATE_COLORKEYENABLE     = 41,   /* TRUE to enable source colorkeyed textures */
	D3DRENDERSTATE_OLDALPHABLENDENABLE = 42,
    D3DRENDERSTATE_BORDERCOLOR        = 43,   /* Border color for texturing w/border */
    D3DRENDERSTATE_TEXTUREADDRESSU    = 44,   /* Texture addressing mode for U coordinate */
    D3DRENDERSTATE_TEXTUREADDRESSV    = 45,   /* Texture addressing mode for V coordinate */
    D3DRENDERSTATE_MIPMAPLODBIAS      = 46,   /* D3DVALUE Mipmap LOD bias */
	D3DRS_ZBIAS                       = 47,   /* LONG Z bias */
    D3DRENDERSTATE_ANISOTROPY         = 49,   /* Max. anisotropy. 1 = no anisotropy */
#endif /* DIRECT3D_VERSION >= 0x0500 */
    D3DRENDERSTATE_FLUSHBATCH         = 50,   /* Explicit flush for DP batching (DX5 Only) */
#if(DIRECT3D_VERSION >= 0x0600)
    D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT=51, /* BOOL enable sort-independent transparency */
#endif /* DIRECT3D_VERSION >= 0x0600 */
    D3DRENDERSTATE_STIPPLEPATTERN00   = 64,   /* Stipple pattern 01...  */
    D3DRENDERSTATE_STIPPLEPATTERN01   = 65,
    D3DRENDERSTATE_STIPPLEPATTERN02   = 66,
    D3DRENDERSTATE_STIPPLEPATTERN03   = 67,
    D3DRENDERSTATE_STIPPLEPATTERN04   = 68,
    D3DRENDERSTATE_STIPPLEPATTERN05   = 69,
    D3DRENDERSTATE_STIPPLEPATTERN06   = 70,
    D3DRENDERSTATE_STIPPLEPATTERN07   = 71,
    D3DRENDERSTATE_STIPPLEPATTERN08   = 72,
    D3DRENDERSTATE_STIPPLEPATTERN09   = 73,
    D3DRENDERSTATE_STIPPLEPATTERN10   = 74,
    D3DRENDERSTATE_STIPPLEPATTERN11   = 75,
    D3DRENDERSTATE_STIPPLEPATTERN12   = 76,
    D3DRENDERSTATE_STIPPLEPATTERN13   = 77,
    D3DRENDERSTATE_STIPPLEPATTERN14   = 78,
    D3DRENDERSTATE_STIPPLEPATTERN15   = 79,
    D3DRENDERSTATE_STIPPLEPATTERN16   = 80,
    D3DRENDERSTATE_STIPPLEPATTERN17   = 81,
    D3DRENDERSTATE_STIPPLEPATTERN18   = 82,
    D3DRENDERSTATE_STIPPLEPATTERN19   = 83,
    D3DRENDERSTATE_STIPPLEPATTERN20   = 84,
    D3DRENDERSTATE_STIPPLEPATTERN21   = 85,
    D3DRENDERSTATE_STIPPLEPATTERN22   = 86,
    D3DRENDERSTATE_STIPPLEPATTERN23   = 87,
    D3DRENDERSTATE_STIPPLEPATTERN24   = 88,
    D3DRENDERSTATE_STIPPLEPATTERN25   = 89,
    D3DRENDERSTATE_STIPPLEPATTERN26   = 90,
    D3DRENDERSTATE_STIPPLEPATTERN27   = 91,
    D3DRENDERSTATE_STIPPLEPATTERN28   = 92,
    D3DRENDERSTATE_STIPPLEPATTERN29   = 93,
    D3DRENDERSTATE_STIPPLEPATTERN30   = 94,
    D3DRENDERSTATE_STIPPLEPATTERN31   = 95,
	
#if(DIRECT3D_VERSION >= 0x0700)
	D3DRENDERSTATE_EXTENTS             = 138,
	D3DRENDERSTATE_COLORKEYBLENDENABLE = 144,
#endif /* DIRECT3D_VERSION >= 0x0700 */

#if(DIRECT3D_VERSION >= 0x0800)
	D3DRS_SOFTWAREVERTEXPROCESSING     = 153,
	D3DRS_PATCHSEGMENTS                = 164,  // Number of segments per edge when drawing patches
#endif /* DIRECT3D_VERSION >= 0x0800 */

#if(DIRECT3D_VERSION >= 0x0500)
    D3DRENDERSTATE_FORCE_DWORD        = 0x7fffffff /* force 32-bit size enum */
#endif /* DIRECT3D_VERSION >= 0x0500 */
};

RenderStates::RenderStates() : cachedAlphaRefFloat(0.0f)
{
	// All states initially start out zeroed:
	memset(this, 0, sizeof(*this) );

	static const float zerof = 0.0f;
	static const DWORD dwordZeroF = *(const DWORD* const)&zerof;
	static const float onef = 1.0f;
	static const DWORD dwordOneF = *(const DWORD* const)&onef;
	static const float f64 = 64.0f;
	static const DWORD dword64F = *(const DWORD* const)&f64;

	// Now let's set the default values one by one. Source is: https://msdn.microsoft.com/en-us/library/windows/desktop/bb172599(v=vs.85).aspx
	renderStatesUnion.states[D3DRENDERSTATE_TEXTUREHANDLE             ] = NULL; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_ANTIALIAS                 ] = D3DANTIALIAS_NONE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_TEXTUREADDRESS            ] = D3DTADDRESS_WRAP; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_TEXTUREPERSPECTIVE        ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_WRAPU                     ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_WRAPV                     ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_ZENABLE                            ] = D3DZB_TRUE;
	renderStatesUnion.states[D3DRS_FILLMODE                           ] = D3DFILL_SOLID;
	renderStatesUnion.states[D3DRS_SHADEMODE                          ] = D3DSHADE_GOURAUD;
	renderStatesUnion.states[D3DRENDERSTATE_LINEPATTERN               ] = ( (0 << 16) | (0xFFFF) ); // Repeat = 0, Pattern = 1111111111111111b; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_MONOENABLE                ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_ROP2                      ] = R2_COPYPEN; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_PLANEMASK                 ] = 0x00000000; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_ZWRITEENABLE                       ] = TRUE;
	renderStatesUnion.states[D3DRS_ALPHATESTENABLE                    ] = FALSE;
	renderStatesUnion.states[D3DRS_LASTPIXEL                          ] = TRUE;
	renderStatesUnion.states[D3DRENDERSTATE_TEXTUREMAG                ] = D3DFILTER_NEAREST; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_TEXTUREMIN                ] = D3DFILTER_NEAREST; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_SRCBLEND                           ] = D3DBLEND_ONE;
	renderStatesUnion.states[D3DRS_DESTBLEND                          ] = D3DBLEND_ZERO;
	renderStatesUnion.states[D3DRENDERSTATE_TEXTUREMAPBLEND           ] = D3DTBLEND_MODULATE; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_CULLMODE                           ] = D3DCULL_CCW;
	renderStatesUnion.states[D3DRS_ZFUNC                              ] = D3DCMP_LESSEQUAL;
	renderStatesUnion.states[D3DRS_ALPHAREF                           ] = 0x00000000;
	renderStatesUnion.states[D3DRS_ALPHAFUNC                          ] = D3DCMP_ALWAYS;
	renderStatesUnion.states[D3DRS_DITHERENABLE                       ] = FALSE;
	renderStatesUnion.states[D3DRS_ALPHABLENDENABLE                   ] = FALSE;
	renderStatesUnion.states[D3DRS_FOGENABLE                          ] = FALSE;
	renderStatesUnion.states[D3DRS_SPECULARENABLE                     ] = FALSE;
	renderStatesUnion.states[D3DRENDERSTATE_ZVISIBLE                  ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_SUBPIXEL                  ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_SUBPIXELX                 ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_STIPPLEDALPHA             ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_FOGCOLOR                           ] = D3DCOLOR_ARGB(0, 0, 0, 0);
	renderStatesUnion.states[D3DRS_FOGTABLEMODE                       ] = D3DFOG_NONE;
	renderStatesUnion.states[D3DRS_FOGSTART                           ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_FOGEND                             ] = dwordOneF;
	renderStatesUnion.states[D3DRS_FOGDENSITY                         ] = dwordOneF;
	renderStatesUnion.states[D3DRENDERSTATE_STIPPLEENABLE             ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_EDGEANTIALIAS             ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_COLORKEYENABLE            ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_OLDALPHABLENDENABLE       ] = FALSE;
	renderStatesUnion.states[D3DRENDERSTATE_BORDERCOLOR               ] = D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00); // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_TEXTUREADDRESSU           ] = D3DTADDRESS_WRAP; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_TEXTUREADDRESSV           ] = D3DTADDRESS_WRAP; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_MIPMAPLODBIAS             ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_ZBIAS                              ] = 0; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_RANGEFOGENABLE                     ] = FALSE;
	renderStatesUnion.states[D3DRENDERSTATE_ANISOTROPY                ] = 1; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_FLUSHBATCH                ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_STENCILENABLE                      ] = FALSE;
	renderStatesUnion.states[D3DRS_STENCILFAIL                        ] = D3DSTENCILOP_KEEP;
	renderStatesUnion.states[D3DRS_STENCILZFAIL                       ] = D3DSTENCILOP_KEEP;
	renderStatesUnion.states[D3DRS_STENCILPASS                        ] = D3DSTENCILOP_KEEP;
	renderStatesUnion.states[D3DRS_STENCILFUNC                        ] = D3DCMP_ALWAYS;
	renderStatesUnion.states[D3DRS_STENCILREF                         ] = 0x00;
	renderStatesUnion.states[D3DRS_STENCILMASK                        ] = 0xFFFFFFFF;
	renderStatesUnion.states[D3DRS_STENCILWRITEMASK                   ] = 0xFFFFFFFF;
	renderStatesUnion.states[D3DRS_TEXTUREFACTOR                      ] = D3DCOLOR_ARGB(0xFF, 0xFF, 0xFF, 0xFF);
	for (unsigned char stippleLine = 0; stippleLine < 32; ++stippleLine)
	{
		renderStatesUnion.states[D3DRENDERSTATE_STIPPLEPATTERN00 + stippleLine] = (stippleLine & 0x1) ? 0x55555555 : 0xAAAAAAAA; // Deprecated legacy states
	}
	renderStatesUnion.states[D3DRS_WRAP0                              ] = 0;
	renderStatesUnion.states[D3DRS_WRAP1                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP2                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP3                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP4                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP5                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP6                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP7                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_CLIPPING                           ] = TRUE;
	renderStatesUnion.states[D3DRS_LIGHTING                           ] = TRUE;
	renderStatesUnion.states[D3DRS_AMBIENT                            ] = D3DCOLOR_ARGB(0, 0, 0, 0);
	renderStatesUnion.states[D3DRS_FOGVERTEXMODE                      ] = D3DFOG_NONE;
	renderStatesUnion.states[D3DRS_COLORVERTEX                        ] = TRUE;
	renderStatesUnion.states[D3DRS_LOCALVIEWER                        ] = TRUE;
	renderStatesUnion.states[D3DRS_NORMALIZENORMALS                   ] = FALSE;
	renderStatesUnion.states[D3DRENDERSTATE_COLORKEYBLENDENABLE       ] = FALSE;
	renderStatesUnion.states[D3DRS_DIFFUSEMATERIALSOURCE              ] = D3DMCS_COLOR1;
	renderStatesUnion.states[D3DRS_SPECULARMATERIALSOURCE             ] = D3DMCS_COLOR2;
	renderStatesUnion.states[D3DRS_AMBIENTMATERIALSOURCE              ] = D3DMCS_MATERIAL;
	renderStatesUnion.states[D3DRS_EMISSIVEMATERIALSOURCE             ] = D3DMCS_MATERIAL;
	renderStatesUnion.states[D3DRS_VERTEXBLEND                        ] = D3DVBF_DISABLE;
	renderStatesUnion.states[D3DRS_CLIPPLANEENABLE                    ] = 0x00000000;
	renderStatesUnion.states[D3DRS_SOFTWAREVERTEXPROCESSING           ] = FALSE; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_POINTSIZE                          ] = dwordOneF;
	renderStatesUnion.states[D3DRS_POINTSIZE_MIN                      ] = dwordOneF;
	renderStatesUnion.states[D3DRS_POINTSPRITEENABLE                  ] = FALSE;
	renderStatesUnion.states[D3DRS_POINTSCALEENABLE                   ] = FALSE;
	renderStatesUnion.states[D3DRS_POINTSCALE_A                       ] = dwordOneF;
	renderStatesUnion.states[D3DRS_POINTSCALE_B                       ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_POINTSCALE_C                       ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_MULTISAMPLEANTIALIAS               ] = TRUE;
	renderStatesUnion.states[D3DRS_MULTISAMPLEMASK                    ] = 0xFFFFFFFF;
	renderStatesUnion.states[D3DRS_PATCHEDGESTYLE                     ] = D3DPATCHEDGE_DISCRETE;
	renderStatesUnion.states[D3DRS_PATCHSEGMENTS                      ] = 0; // Deprecated legacy state
	renderStatesUnion.states[D3DRS_DEBUGMONITORTOKEN                  ] = D3DDMT_ENABLE;
	renderStatesUnion.states[D3DRS_POINTSIZE_MAX                      ] = dword64F;
	renderStatesUnion.states[D3DRS_INDEXEDVERTEXBLENDENABLE           ] = FALSE;
	renderStatesUnion.states[D3DRS_COLORWRITEENABLE                   ] = D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;
	renderStatesUnion.states[D3DRS_TWEENFACTOR                        ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_BLENDOP                            ] = D3DBLENDOP_ADD;
	renderStatesUnion.states[D3DRS_POSITIONDEGREE                     ] = D3DDEGREE_CUBIC;
	renderStatesUnion.states[D3DRS_NORMALDEGREE                       ] = D3DDEGREE_LINEAR;
	renderStatesUnion.states[D3DRS_SCISSORTESTENABLE                  ] = FALSE;
	renderStatesUnion.states[D3DRS_SLOPESCALEDEPTHBIAS                ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_ANTIALIASEDLINEENABLE              ] = FALSE;
	renderStatesUnion.states[D3DRS_MINTESSELLATIONLEVEL               ] = dwordOneF;
	renderStatesUnion.states[D3DRS_MAXTESSELLATIONLEVEL               ] = dwordOneF;
	renderStatesUnion.states[D3DRS_ADAPTIVETESS_X                     ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_ADAPTIVETESS_Y                     ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_ADAPTIVETESS_Z                     ] = dwordOneF;
	renderStatesUnion.states[D3DRS_ADAPTIVETESS_W                     ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_ENABLEADAPTIVETESSELLATION         ] = FALSE;
	renderStatesUnion.states[D3DRS_TWOSIDEDSTENCILMODE                ] = FALSE;
	renderStatesUnion.states[D3DRS_CCW_STENCILFAIL                    ] = D3DSTENCILOP_KEEP;
	renderStatesUnion.states[D3DRS_CCW_STENCILZFAIL                   ] = D3DSTENCILOP_KEEP;
	renderStatesUnion.states[D3DRS_CCW_STENCILPASS                    ] = D3DSTENCILOP_KEEP;
	renderStatesUnion.states[D3DRS_CCW_STENCILFUNC                    ] = D3DCMP_ALWAYS;
	renderStatesUnion.states[D3DRS_COLORWRITEENABLE1                  ] = renderStatesUnion.states[D3DRS_COLORWRITEENABLE];
	renderStatesUnion.states[D3DRS_COLORWRITEENABLE2                  ] = renderStatesUnion.states[D3DRS_COLORWRITEENABLE];
	renderStatesUnion.states[D3DRS_COLORWRITEENABLE3                  ] = renderStatesUnion.states[D3DRS_COLORWRITEENABLE];
	renderStatesUnion.states[D3DRS_BLENDFACTOR                        ] = D3DCOLOR_ARGB(0xFF, 0xFF, 0xFF, 0xFF);
	renderStatesUnion.states[D3DRS_SRGBWRITEENABLE                    ] = FALSE;
	renderStatesUnion.states[D3DRS_DEPTHBIAS                          ] = dwordZeroF;
	renderStatesUnion.states[D3DRS_WRAP8                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP9                              ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP10                             ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP11                             ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP12                             ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP13                             ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP14                             ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_WRAP15                             ] = renderStatesUnion.states[D3DRS_WRAP0];
	renderStatesUnion.states[D3DRS_SEPARATEALPHABLENDENABLE           ] = FALSE;
	renderStatesUnion.states[D3DRS_SRCBLENDALPHA                      ] = D3DBLEND_ONE;
	renderStatesUnion.states[D3DRS_DESTBLENDALPHA                     ] = D3DBLEND_ZERO;
	renderStatesUnion.states[D3DRS_BLENDOPALPHA                       ] = D3DBLENDOP_ADD;

	cachedAlphaRefFloat = 0.0f;
	cachedAmbient = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
	ColorDWORDToFloat4(renderStatesUnion.states[D3DRS_BLENDFACTOR], cachedBlendFactor);
	cachedInvBlendFactor = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f) - cachedBlendFactor;
}

/*** IUnknown methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::QueryInterface(THIS_ REFIID riid, void** ppvObj) 
{
	HRESULT ret = d3d9dev->QueryInterface(riid, ppvObj);
	if (ret == NOERROR)
	{
		*ppvObj = this;
		AddRef();
	}
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DDevice9Hook::AddRef(THIS)
{
	ULONG ret = d3d9dev->AddRef();
	++refCount;
	return ret;
}

COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE IDirect3DDevice9Hook::Release(THIS)
{
	ULONG ret = d3d9dev->Release();
	if (--refCount == 0)
	{
#ifdef DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES
		char printBuffer[128] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(printBuffer, "Fully releasing hooked D3D9 Device %p\n", this);
#pragma warning(pop)
		OutputDebugStringA(printBuffer);
#endif
		delete this;
	}
	return ret;
}

/*** IDirect3DDevice9 methods ***/
COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::TestCooperativeLevel(THIS)
{
	HRESULT ret = d3d9dev->TestCooperativeLevel();
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::EvictManagedResources(THIS)
{
	HRESULT ret = d3d9dev->EvictManagedResources();
	return ret;
}

COM_DECLSPEC_NOTHROW BOOL STDMETHODCALLTYPE IDirect3DDevice9Hook::ShowCursor(THIS_ BOOL bShow)
{
	BOOL ret = d3d9dev->ShowCursor(bShow);
	return ret;
}

void IDirect3DDevice9Hook::PreReset(void)
{
	if (currentState.currentRenderTargets[0])
	{
		currentState.currentRenderTargets[0]->GetUnderlyingSurface()->Release();
		currentState.currentRenderTargets[0]->Release();
		currentState.currentRenderTargets[0]->Release();
		currentState.currentRenderTargets[0] = NULL;
	}

	if (currentState.currentDepthStencil)
	{
		currentState.currentDepthStencil->GetUnderlyingSurface()->Release();
		currentState.currentDepthStencil->Release();
		currentState.currentDepthStencil = NULL;
	}

	implicitSwapChain->GetUnderlyingSwapChain()->Release();
	implicitSwapChain->Release();
	implicitSwapChain = NULL;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::Reset(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	D3DPRESENT_PARAMETERS modifiedParams = *pPresentationParameters;
	ModifyPresentParameters(modifiedParams);

	PreReset();

	HRESULT ret = d3d9dev->Reset(&modifiedParams);
	if (FAILED(ret) )
	{
		char resetFail[256] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(resetFail, "HR: 0x%08X\n", ret);
#pragma warning(pop)
		MessageBoxA(NULL, resetFail, "Failed reset", MB_OK);
		return ret;
	}

	// Re-initialize the device state: 
	InitializeState(modifiedParams, initialDevType, initialCreateFlags, initialCreateFocusWindow);

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::Present(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	HRESULT ret = implicitSwapChain->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);
	if (FAILED(ret) )
		return ret;

#ifdef SURFACE_MAGIC_COOKIE
	ValidateSurfaceMagicCookie(implicitSwapChain->GetInternalBackBuffer()->GetSurfaceBytes() );
#endif

	static LARGE_INTEGER lastPresentTime = {0};
	static long double ldFreq = 0.0;
	if (lastPresentTime.QuadPart == 0)
	{
		QueryPerformanceCounter(&lastPresentTime);
		LARGE_INTEGER freq = {0};
		QueryPerformanceFrequency(&freq);
		ldFreq = (const long double)(freq.QuadPart);
	}
	else
	{
		LARGE_INTEGER currentPresentTime = {0};
		QueryPerformanceCounter(&currentPresentTime);
		const LONGLONG timeDelta = currentPresentTime.QuadPart - lastPresentTime.QuadPart;
		const long double timeDeltaSeconds = timeDelta / ldFreq;

		{
			static DWORD lastPrintTime = 0;
			const unsigned currentTime = GetTickCount();
			if (currentTime - lastPrintTime > 33)
			{
				char buffer[64] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
				const unsigned len = sprintf(buffer, "%03.3fms per frame (%03.3fFPS)\n", timeDeltaSeconds * 1000.0f, 1.0f / timeDeltaSeconds);
#pragma warning(pop)
				DWORD numCharsWritten = 0;
				WriteConsoleA(hConsoleHandle, buffer, len, &numCharsWritten, NULL);
				lastPrintTime = currentTime;
			}
		}

		lastPresentTime = currentPresentTime;

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
		{
			char buffer[128] = {0};
			const long double avgTimeSpentVertexShading_us = (totalVertexShadeTicks / (const long double)numVertexShadeTasks) / ldFreq * 1000000.0;
#pragma warning(push)
#pragma warning(disable:4996)
			sprintf(buffer, "%03.05gus average vertex shade time for %I64u vertices\n", avgTimeSpentVertexShading_us, numVertexShadeTasks);
#pragma warning(pop)
			OutputDebugStringA(buffer);

			totalVertexShadeTicks = 0;
			numVertexShadeTasks = 0;
		}
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

#ifdef PROFILE_AVERAGE_PIXEL_SHADE_TIMES
		{
			char buffer[128] = {0};
			const long double avgTimeSpentPixelShading_us = (totalPixelShadeTicks / (const long double)numPixelShadeTasks) / ldFreq * 1000000.0;
#pragma warning(push)
#pragma warning(disable:4996)
			sprintf(buffer, "%03.05gus average pixel shade time for %I64u pixels\n", avgTimeSpentPixelShading_us, numPixelShadeTasks);
#pragma warning(pop)
			OutputDebugStringA(buffer);

			totalPixelShadeTicks = 0;
			numPixelShadeTasks = 0;
		}
#endif // #ifdef PROFILE_AVERAGE_PIXEL_SHADE_TIMES
	}

	frameStats.Clear();

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::UpdateSurface(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint)
{
	IDirect3DSurface9Hook* sourceHookPtr = dynamic_cast<IDirect3DSurface9Hook*>(pSourceSurface);
#ifdef _DEBUG
	if (sourceHookPtr)
#endif
		pSourceSurface = sourceHookPtr->GetUnderlyingSurface();
#ifdef _DEBUG
	else if (pSourceSurface != NULL)
	{
		DbgBreakPrint("Error: UpdateSurface called with a non-hooked surface source pointer");
	}
#endif

	IDirect3DSurface9Hook* destHookPtr = dynamic_cast<IDirect3DSurface9Hook*>(pDestinationSurface);
#ifdef _DEBUG
	if (destHookPtr)
#endif
		pDestinationSurface = destHookPtr->GetUnderlyingSurface();
#ifdef _DEBUG
	else if (pDestinationSurface != NULL)
	{
		DbgBreakPrint("Error: UpdateSurface called with a non-hooked surface destination pointer");
	}
#endif

	HRESULT ret = d3d9dev->UpdateSurface(pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
	if (FAILED(ret) )
		return ret;

	destHookPtr->UpdateSurfaceInternal(sourceHookPtr, pSourceRect, pDestPoint);

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::UpdateTexture(THIS_ IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture)
{
	IDirect3DTexture9Hook* sourceHookPtr = dynamic_cast<IDirect3DTexture9Hook*>(pSourceTexture);
#ifdef _DEBUG
	if (sourceHookPtr)
#endif
		pSourceTexture = sourceHookPtr->GetUnderlyingTexture();
#ifdef _DEBUG
	else if (pSourceTexture != NULL)
	{
		DbgBreakPrint("Error: UpdateTexture called with a non-hooked texture source pointer");
	}
#endif

	IDirect3DTexture9Hook* destHookPtr = dynamic_cast<IDirect3DTexture9Hook*>(pDestinationTexture);
#ifdef _DEBUG
	if (destHookPtr)
#endif
		pDestinationTexture = destHookPtr->GetUnderlyingTexture();
#ifdef _DEBUG
	else if (pDestinationTexture != NULL)
	{
		DbgBreakPrint("Error: UpdateTexture called with a non-hooked texture destination pointer");
	}
#endif

	HRESULT ret = d3d9dev->UpdateTexture(pSourceTexture, pDestinationTexture);
	if (FAILED(ret) )
		return ret;

	destHookPtr->UpdateTextureInternal(sourceHookPtr);

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::StretchRect(THIS_ IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
	switch (Filter)
	{
	case D3DTEXF_NONE:
	case D3DTEXF_POINT:
	case D3DTEXF_LINEAR:
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: StretchRect only supports NONE, POINT, and LINEAR filters");
#endif
		return D3DERR_INVALIDCALL;
	}

	if (!pSourceSurface)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Missing source surface!");
#endif
		return D3DERR_INVALIDCALL;
	}

	if (!pDestSurface)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Missing destination surface!");
#endif
		return D3DERR_INVALIDCALL;
	}

	const IDirect3DSurface9Hook* const hookSource = dynamic_cast<IDirect3DSurface9Hook*>(pSourceSurface);
	if (!hookSource)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Source surface is not hooked!");
#endif
		return D3DERR_INVALIDCALL;
	}

	IDirect3DSurface9Hook* const hookDest = dynamic_cast<IDirect3DSurface9Hook*>(pDestSurface);
	if (!hookDest)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Destination surface is not hooked!");
#endif
		return D3DERR_INVALIDCALL;
	}

	const unsigned Width = hookSource->GetInternalWidth();
	const unsigned Height = hookSource->GetInternalHeight();
	const unsigned DestWidth = hookDest->GetInternalWidth();
	const unsigned DestHeight = hookDest->GetInternalHeight();

	// Simple and common copy case of copying an entire surface over to another entire surface (D3DX9 uses this internally a lot for format conversions
	// and to save from D3DPOOL_DEFAULT to D3DPOOL_SYSMEM surfaces)
	if (!pSourceRect && !pDestRect)
	{
		D3DXVECTOR4 tempColor;
		for (unsigned y = 0; y < Height; ++y)
		{
			for (unsigned x = 0; x < Width; ++x)
			{
				hookSource->GetPixelVec<D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA, false>(x, y, tempColor);
				hookDest->SetPixelVec<D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA>(x, y, tempColor);
			}
		}
		return S_OK;
	}

	RECT srcRect = {0};
	if (pSourceRect)
		srcRect = *pSourceRect;
	else
	{
		srcRect.right = Width;
		srcRect.bottom = Height;
	}

	RECT destRect = {0};
	if (pDestRect)
		destRect = *pDestRect;
	else
	{
		destRect.right = Width;
		destRect.bottom = Height;
	}

	const int srcWidth = srcRect.right - srcRect.left;
	const int srcHeight = srcRect.bottom - srcRect.top;
	const int destWidth = destRect.right - destRect.left;
	const int destHeight = destRect.bottom - destRect.top;

	if (srcWidth <= 0 || srcHeight <= 0)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid source rect");
#endif
		return D3DERR_INVALIDCALL;
	}

	if (srcWidth > (const int)Width || srcHeight > (const int)Height)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Source rect extends outside of source surface bounds");
#endif
		return D3DERR_INVALIDCALL;
	}

	if (destWidth <= 0 || destHeight <= 0)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid dest rect");
#endif
		return D3DERR_INVALIDCALL;
	}

	if (destWidth > (const int)DestWidth || destHeight > (const int)DestHeight)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Destination rect extends outside of destination surface bounds");
#endif
		return D3DERR_INVALIDCALL;
	}

	const int copyIterationsWidth = srcWidth < destWidth ? srcWidth : destWidth;
	const int copyIterationsHeight = srcHeight < destHeight ? srcHeight : destHeight;

	D3DXVECTOR4 tempColor;
	for (int y = 0; y < copyIterationsHeight; ++y)
	{
		for (int x = 0; x < copyIterationsWidth; ++x)
		{
			hookSource->GetPixelVec<D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA, false>(srcRect.left + x, srcRect.top + y, tempColor);
			hookDest->SetPixelVec<D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA>(destRect.left + x, destRect.top + y, tempColor);
		}
	}

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::ColorFill(THIS_ IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color)
{
	IDirect3DSurface9Hook* hookPtr = dynamic_cast<IDirect3DSurface9Hook*>(pSurface);
#ifdef _DEBUG
	if (hookPtr)
#endif
		pSurface = hookPtr->GetUnderlyingSurface();
#ifdef _DEBUG
	else if (pSurface != NULL)
	{
		DbgBreakPrint("Error: ColorFill called with a non-hooked surface pointer");
	}
#endif

	HRESULT ret = d3d9dev->ColorFill(pSurface, pRect, color);
	if (FAILED(ret) )
		return ret;

	if (hookPtr)
	{
		if (pRect)
		{
			// Convert from RECT to D3DRECT:
			D3DRECT d3dr;
			d3dr.x1 = pRect->left;
			d3dr.x2 = pRect->right;
			d3dr.y1 = pRect->top;
			d3dr.y2 = pRect->bottom;
			hookPtr->InternalColorFill(color, &d3dr);
		}
		else
		{
			hookPtr->InternalColorFill(color, NULL);
		}		
	}

	return ret;
}

// In the case of a successful render-target set, the viewport is automatically resized to the
// size of the largest set render-target:
void IDirect3DDevice9Hook::AutoResizeViewport(void)
{
	D3DVIEWPORT9 currentViewport = currentState.cachedViewport.viewport;
	currentViewport.Width = 1;
	currentViewport.Height = 1;
	for (unsigned x = 0; x < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++x)
	{
		IDirect3DSurface9Hook* renderTarget = currentState.currentRenderTargets[x];
		if (!renderTarget)
			continue;
		D3DSURFACE_DESC desc = {};
		renderTarget->GetDesc(&desc);
		if (desc.Width > currentViewport.Width)
			currentViewport.Width = desc.Width;
		if (desc.Height > currentViewport.Height)
			currentViewport.Height = desc.Height;
	}
	SetViewport(&currentViewport);
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::BeginScene(THIS)
{
	HRESULT ret = d3d9dev->BeginScene();
	if (FAILED(ret) )
		return ret;

	if (!HasBegunScene() )
		sceneBegun = TRUE;
#ifdef _DEBUG
	else
	{
		// Each BeginScene() needs to have an EndScene() pair to match it!
		DbgBreakPrint("Error: Calling BeginScene() without first calling EndScene()");
	}
#endif

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::EndScene(THIS)
{
	HRESULT ret = d3d9dev->EndScene();
	if (FAILED(ret) )
		return ret;

	if (HasBegunScene() )
		sceneBegun = FALSE;
#ifdef _DEBUG
	else
	{
		// Every EndScene() needs to have a BeginScene() pair to match it!
		DbgBreakPrint("Error: Trying to end a scene without first beginning one. Must call BeginScene() before calling EndScene()");
	}
#endif

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::Clear(THIS_ DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
	const unsigned validClearFlags = D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
	if (Flags & (~validClearFlags) )
		return D3DERR_INVALIDCALL; // These are the only D3DCLEAR flags valid for this function call

	if (Flags & (D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER) )
	{
		if (!currentState.currentDepthStencil)
			return D3DERR_INVALIDCALL;

		if (Flags & D3DCLEAR_ZBUFFER)
		{
			if (Z < 0.0f)
			{
				// Z-clear value must be between 0.0f and 1.0f!
				DbgBreakPrint("Error: Clear Z-value must be between 0.0f and 1.0f");
				return D3DERR_INVALIDCALL;
			}
			if (Z > 1.0f)
			{
				// Z-clear value must be between 0.0f and 1.0f!
				DbgBreakPrint("Error: Clear Z-value must be between 0.0f and 1.0f");
				return D3DERR_INVALIDCALL;
			}
		}
		if (Flags & D3DCLEAR_STENCIL)
		{
			if (!HasStencil(currentState.currentDepthStencil->GetInternalFormat() ) )
				return D3DERR_INVALIDCALL;

			// TODO: Stencil buffer clear value range validation (should only allow between 0 and 2^n)
		}
	}

	if (Count > 0)
	{
		if (!pRects)
		{
			// Count must be 0 if pRects are NULL!
			DbgBreakPrint("Error: Clear() Count must be 0 if pRects are NULL");
			return D3DERR_INVALIDCALL;
		}

		for (unsigned x = 0; x < Count; ++x)
		{
			// Validate rects
		}
	}
	else
	{
		if (pRects != NULL)
		{
			// pRects must be NULL if Count is 0!
			DbgBreakPrint("Error: Clear() pRects must be NULL if Count is 0");
			return D3DERR_INVALIDCALL;
		}
	}

	if (!(Flags & (D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER) ) )
	{
		// At least one of: Target, Stencil, ZBuffer must be set in the Clear() call flags!
		DbgBreakPrint("Error: Clear call requires at least one of the Target, Stencil, or ZBuffer flags");
		return D3DERR_INVALIDCALL;
	}

#ifdef _DEBUG
	HRESULT ret = d3d9dev->Clear(Count, pRects, Flags, Color, Z, Stencil);
	if (FAILED(ret) )
	{
		__debugbreak(); // We must've missed an error somewhere
		return ret;
	}
#endif
	bool rectFillsWholeRenderTarget = false;
	if (Count > 0 && pRects != NULL && (Flags & D3DCLEAR_TARGET) )
	{
		unsigned char rectFillsRenderTargets = 0;
		for (unsigned x = 0; x < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++x)
		{
			IDirect3DSurface9Hook* const currentRT = currentState.currentRenderTargets[x];
			if (currentRT)
			{
				const int surfWidth = (const int)currentRT->GetInternalWidth();
				const int surfHeight = (const int)currentRT->GetInternalHeight();

				// Look for any one rect that encompasses the entirety of the target surface
				for (unsigned y = 0; y < Count; ++y)
				{
					const D3DRECT& thisRect = pRects[y];

					if (thisRect.x1 <= 0 && thisRect.y1 <= 0 &&
						thisRect.x2 >= surfWidth && thisRect.y2 >= surfHeight)
					{
						++rectFillsRenderTargets;
						break;
					}
				}
			}
			else
				++rectFillsRenderTargets;
		}

		if (rectFillsRenderTargets == D3D_MAX_SIMULTANEOUS_RENDERTARGETS)
			rectFillsWholeRenderTarget = true;
	}

	bool rectFillsWholeDepthStencil = false;
	if (Count > 0 && pRects != NULL && (Flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) ) )
	{
		if (currentState.currentDepthStencil)
		{
			const int dsWidth = currentState.currentDepthStencil->GetInternalWidth();
			const int dsHeight = currentState.currentDepthStencil->GetInternalHeight();
			// Look for any one rect that encompasses the entirety of the target depthstencil buffer
			for (unsigned y = 0; y < Count; ++y)
			{
				const D3DRECT& thisRect = pRects[y];

				if (thisRect.x1 <= 0 && thisRect.y1 <= 0 &&
					thisRect.x2 >= dsWidth && thisRect.y2 >= dsHeight)
				{
					rectFillsWholeDepthStencil = true;
					break;
				}
			}
		}
	}

	if (Flags & D3DCLEAR_TARGET)
	{
		// Quick and simple fill the whole surface!
		if (Count == 0 || pRects == NULL || rectFillsWholeRenderTarget)
		{
			for (unsigned x = 0; x < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++x)
			{
				IDirect3DSurface9Hook* const currentRT = currentState.currentRenderTargets[x];
				if (currentRT)
				{
					currentRT->InternalColorFill(Color);
				}
			}
		}
		else
		{
			for (unsigned x = 0; x < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++x)
			{
				IDirect3DSurface9Hook* const currentRT = currentState.currentRenderTargets[x];
				if (currentRT)
				{
					for (unsigned y = 0; y < Count; ++y)
					{
						const D3DRECT* clearRect = pRects + y;
						currentRT->InternalColorFill(Color, clearRect);
					}
				}
			}
		}
	}

	if (Flags & D3DCLEAR_ZBUFFER)
	{
		// Quick and simple fill the whole depth buffer!
		if (Count == 0 || pRects == NULL || rectFillsWholeDepthStencil)
		{
			if (currentState.currentDepthStencil)
			{
				currentState.currentDepthStencil->InternalDepthFill(Z);
			}
		}
		else
		{
			if (currentState.currentDepthStencil)
			{
				for (unsigned y = 0; y < Count; ++y)
				{
					const D3DRECT* clearRect = pRects + y;
					currentState.currentDepthStencil->InternalDepthFill(Z, clearRect);
				}
			}
		}
	}

	if (Flags & D3DCLEAR_STENCIL)
	{
		// Quick and simple fill the whole stencil buffer!
		if (Count == 0 || pRects == NULL || rectFillsWholeDepthStencil)
		{
			if (currentState.currentDepthStencil)
			{
				currentState.currentDepthStencil->InternalStencilFill(Stencil);
			}
		}
		else
		{
			if (currentState.currentDepthStencil)
			{
				for (unsigned y = 0; y < Count; ++y)
				{
					const D3DRECT* clearRect = pRects + y;
					currentState.currentDepthStencil->InternalStencilFill(Stencil, clearRect);
				}
			}
		}
	}	

	return S_OK;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::BeginStateBlock(THIS)
{
	HRESULT ret = d3d9dev->BeginStateBlock();
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::EndStateBlock(THIS_ IDirect3DStateBlock9** ppSB)
{
	LPDIRECT3DSTATEBLOCK9 realStateBlock = NULL;
	HRESULT ret = d3d9dev->EndStateBlock(&realStateBlock);
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

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::ValidateDevice(THIS_ DWORD* pNumPasses)
{
	HRESULT ret = d3d9dev->ValidateDevice(pNumPasses);
	return ret;
}

struct int2
{
	int x;
	int y;
};

struct ubyte4
{
	unsigned char x;
	unsigned char y;
	unsigned char z;
	unsigned char w;
};
static_assert(sizeof(ubyte4) == 4, "Error!");

struct short4
{
	short x, y, z, w;
};
static_assert(sizeof(short4) == 8, "Error!");

struct ushort4
{
	unsigned short x;
	unsigned short y;
	unsigned short z;
	unsigned short w;
};
static_assert(sizeof(ushort4) == 8, "Error!");

static const unsigned udec3mask = ( (1 << 10) - 1);
static const unsigned dec3mask = ( (1 << 9) - 1);
static inline void LoadElementToRegister(D3DXVECTOR4& outRegister, const D3DDECLTYPE elemType, const void* const data)
{
	switch (elemType)
	{
	case D3DDECLTYPE_FLOAT1    : // 1D float expanded to (value, 0., 0., 1.)
	{
		const float* const fData = (const float* const)data;
		outRegister.x = *fData;
		outRegister.y = 0.0f;
		outRegister.z = 0.0f;
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT2    : // 2D float expanded to (value, value, 0., 1.)
	{
		const float* const fData = (const float* const)data;
		outRegister.x = fData[0];
		outRegister.y = fData[1];
		outRegister.z = 0.0f;
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT3    : // 3D float expanded to (value, value, value, 1.)
	{
		const float* const fData = (const float* const)data;
		outRegister.x = fData[0];
		outRegister.y = fData[1];
		outRegister.z = fData[2];
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT4    : // 4D float
	{
		const D3DXVECTOR4* const vecData = (const D3DXVECTOR4* const)data;
		outRegister = *vecData;
	}
		break;
	case D3DDECLTYPE_D3DCOLOR  : // 4D packed unsigned bytes mapped to 0. to 1. range; Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
	{
		const D3DCOLOR* const colorData = (const D3DCOLOR* const)data;
		ColorDWORDToFloat4(*colorData, outRegister);
	}
		break;
	case D3DDECLTYPE_UBYTE4    : // 4D unsigned byte
	{
		const ubyte4* const ubyte4Data = (const ubyte4* const)data;
		outRegister.x = (const float)ubyte4Data->x;
		outRegister.y = (const float)ubyte4Data->y;
		outRegister.z = (const float)ubyte4Data->z;
		outRegister.w = (const float)ubyte4Data->w;
	}
		break;
	case D3DDECLTYPE_SHORT2    : // 2D signed short expanded to (value, value, 0., 1.)
	{
		const short4* const short4Data = (const short4* const)data;
		outRegister.x = (const float)short4Data->x;
		outRegister.y = (const float)short4Data->y;
		outRegister.z = 0.0f;
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_SHORT4    : // 4D signed short
	{
		const short4* const short4Data = (const short4* const)data;
		outRegister.x = (const float)short4Data->x;
		outRegister.y = (const float)short4Data->y;
		outRegister.z = (const float)short4Data->z;
		outRegister.w = (const float)short4Data->w;
	}
		break;
	case D3DDECLTYPE_UBYTE4N   : // Each of 4 bytes is normalized by dividing to 255.0 
	{
		const ubyte4* const ubyte4Data = (const ubyte4* const)data;
		outRegister.x = ubyte4Data->x / 255.0f;
		outRegister.y = ubyte4Data->y / 255.0f;
		outRegister.z = ubyte4Data->z / 255.0f;
		outRegister.w = ubyte4Data->w / 255.0f;
	}
		break;
	case D3DDECLTYPE_SHORT2N   : // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
	{
		const short4* const short4Data = (const short4* const)data;
		outRegister.x = short4Data->x / 32767.0f;
		outRegister.y = short4Data->y / 32767.0f;
		outRegister.z = 0.0f;
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_SHORT4N   : // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
	{
		const short4* const short4Data = (const short4* const)data;
		outRegister.x = short4Data->x / 32767.0f;
		outRegister.y = short4Data->y / 32767.0f;
		outRegister.z = short4Data->z / 32767.0f;
		outRegister.w = short4Data->w / 32767.0f;
	}
		break;
	case D3DDECLTYPE_USHORT2N  : // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
	{
		const ushort4* const ushort4Data = (const ushort4* const)data;
		outRegister.x = ushort4Data->x / 65535.0f;
		outRegister.y = ushort4Data->y / 65535.0f;
		outRegister.z = 0.0f;
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_USHORT4N  : // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
	{
		const ushort4* const ushort4Data = (const ushort4* const)data;
		outRegister.x = ushort4Data->x / 65535.0f;
		outRegister.y = ushort4Data->y / 65535.0f;
		outRegister.z = ushort4Data->z / 65535.0f;
		outRegister.w = ushort4Data->w / 65535.0f;
	}
		break;
	case D3DDECLTYPE_UDEC3     : // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
	{
		const unsigned* const uData = (const unsigned* const)data;
		const unsigned x = *uData & udec3mask;
		const unsigned y = (*uData >> 10) & udec3mask;
		const unsigned z = (*uData >> 20) & udec3mask;
		outRegister.x = (const float)x;
		outRegister.y = (const float)y;
		outRegister.z = (const float)z;
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_DEC3N     : // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
	{
		const unsigned* const uData = (const unsigned* const)data;
		const unsigned xData = *uData & udec3mask;
		// TODO: Test this for negative DEC3N values to make sure that this sign-math is correct:
		const int xDatai = xData > dec3mask ? -(const int)xData : (const int)xData;
		const unsigned yData = (*uData >> 10) & udec3mask;
		const int yDatai = yData > dec3mask ? -(const int)yData : (const int)yData;
		const unsigned zData = (*uData >> 20) & udec3mask;
		const int zDatai = zData > dec3mask ? -(const int)zData : (const int)zData;
		outRegister.x = xDatai / 511.0f;
		outRegister.y = yDatai / 511.0f;
		outRegister.z = zDatai / 511.0f;
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT16_2 : // Two 16-bit floating point values, expanded to (value, value, 0, 1)
	{
		const D3DXVECTOR2_16F* const f16Data2 = (const D3DXVECTOR2_16F* const)data;
		outRegister = D3DXVECTOR4(*f16Data2);
		outRegister.z = 0.0f;
		outRegister.w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT16_4 : // Four 16-bit floating point values
	{
		const D3DXVECTOR4_16F* const f16Data4 = (const D3DXVECTOR4_16F* const)data;
		outRegister = D3DXVECTOR4(*f16Data4);
	}
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Undefined vertex decl element type");
#endif
	case D3DDECLTYPE_UNUSED    : // When the type field in a decl is unused.
	{
		// Default register initialization:
		outRegister = vertShaderInputRegisterDefault;
	}
		break;
	}
}

static inline void LoadElementToRegisterValidated(D3DXVECTOR4& outRegister, const D3DDECLTYPE elemType, const void* const data, const void* const streamEndPtr)
{
	if (data > streamEndPtr)
	{
		outRegister.x = 0.0f;
		outRegister.y = 0.0f;
		outRegister.z = 0.0f;
		outRegister.w = 1.0f;
		return;
	}

	LoadElementToRegister(outRegister, elemType, data);
}

#ifdef RUN_SHADERS_IN_WARPS
static inline void LoadElementToRegister4(D3DXVECTOR4* const (&outRegister4)[4], const D3DDECLTYPE elemType, const void** const data4)
{
	switch (elemType)
	{
	case D3DDECLTYPE_FLOAT1    : // 1D float expanded to (value, 0., 0., 1.)
	{
		const float** const fData4 = (const float** const)data4;

		outRegister4[0]->x = fData4[0][0];
		outRegister4[1]->x = fData4[1][0];
		outRegister4[2]->x = fData4[2][0];
		outRegister4[3]->x = fData4[3][0];
		outRegister4[0]->y = 0.0f;
		outRegister4[1]->y = 0.0f;
		outRegister4[2]->y = 0.0f;
		outRegister4[3]->y = 0.0f;
		outRegister4[0]->z = 0.0f;
		outRegister4[1]->z = 0.0f;
		outRegister4[2]->z = 0.0f;
		outRegister4[3]->z = 0.0f;
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT2    : // 2D float expanded to (value, value, 0., 1.)
	{
		const float** const fData4 = (const float** const)data4;

		outRegister4[0]->x = fData4[0][0];
		outRegister4[1]->x = fData4[1][0];
		outRegister4[2]->x = fData4[2][0];
		outRegister4[3]->x = fData4[3][0];
		outRegister4[0]->y = fData4[0][1];
		outRegister4[1]->y = fData4[1][1];
		outRegister4[2]->y = fData4[2][1];
		outRegister4[3]->y = fData4[3][1];
		outRegister4[0]->z = 0.0f;
		outRegister4[1]->z = 0.0f;
		outRegister4[2]->z = 0.0f;
		outRegister4[3]->z = 0.0f;
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT3    : // 3D float expanded to (value, value, value, 1.)
	{
		const float** const fData4 = (const float** const)data4;
		outRegister4[0]->x = fData4[0][0];
		outRegister4[1]->x = fData4[1][0];
		outRegister4[2]->x = fData4[2][0];
		outRegister4[3]->x = fData4[3][0];
		outRegister4[0]->y = fData4[0][1];
		outRegister4[1]->y = fData4[1][1];
		outRegister4[2]->y = fData4[2][1];
		outRegister4[3]->y = fData4[3][1];
		outRegister4[0]->z = fData4[0][2];
		outRegister4[1]->z = fData4[1][2];
		outRegister4[2]->z = fData4[2][2];
		outRegister4[3]->z = fData4[3][2];
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT4    : // 4D float
	{
		const D3DXVECTOR4** const vecData4 = (const D3DXVECTOR4** const)data4;
		*outRegister4[0] = *vecData4[0];
		*outRegister4[1] = *vecData4[1];
		*outRegister4[2] = *vecData4[2];
		*outRegister4[3] = *vecData4[3];
	}
		break;
	case D3DDECLTYPE_D3DCOLOR  : // 4D packed unsigned bytes mapped to 0. to 1. range; Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
	{
		const D3DCOLOR** const colorData4 = (const D3DCOLOR** const)data4;
		ColorDWORDToFloat4_4(colorData4, outRegister4);
	}
		break;
	case D3DDECLTYPE_UBYTE4    : // 4D unsigned byte
	{
		const ubyte4** const ubyte4Data4 = (const ubyte4** const)data4;
		outRegister4[0]->x = (const float)ubyte4Data4[0]->x;
		outRegister4[1]->x = (const float)ubyte4Data4[1]->x;
		outRegister4[2]->x = (const float)ubyte4Data4[2]->x;
		outRegister4[3]->x = (const float)ubyte4Data4[3]->x;
		outRegister4[0]->y = (const float)ubyte4Data4[0]->y;
		outRegister4[1]->y = (const float)ubyte4Data4[1]->y;
		outRegister4[2]->y = (const float)ubyte4Data4[2]->y;
		outRegister4[3]->y = (const float)ubyte4Data4[3]->y;
		outRegister4[0]->z = (const float)ubyte4Data4[0]->z;
		outRegister4[1]->z = (const float)ubyte4Data4[1]->z;
		outRegister4[2]->z = (const float)ubyte4Data4[2]->z;
		outRegister4[3]->z = (const float)ubyte4Data4[3]->z;
		outRegister4[0]->w = (const float)ubyte4Data4[0]->w;
		outRegister4[1]->w = (const float)ubyte4Data4[1]->w;
		outRegister4[2]->w = (const float)ubyte4Data4[2]->w;
		outRegister4[3]->w = (const float)ubyte4Data4[3]->w;
	}
		break;
	case D3DDECLTYPE_SHORT2    : // 2D signed short expanded to (value, value, 0., 1.)
	{
		const short4** const short4Data4 = (const short4** const)data4;
		outRegister4[0]->x = (const float)short4Data4[0]->x;
		outRegister4[1]->x = (const float)short4Data4[1]->x;
		outRegister4[2]->x = (const float)short4Data4[2]->x;
		outRegister4[3]->x = (const float)short4Data4[3]->x;
		outRegister4[0]->y = (const float)short4Data4[0]->y;
		outRegister4[1]->y = (const float)short4Data4[1]->y;
		outRegister4[2]->y = (const float)short4Data4[2]->y;
		outRegister4[3]->y = (const float)short4Data4[3]->y;
		outRegister4[0]->z = 0.0f;
		outRegister4[1]->z = 0.0f;
		outRegister4[2]->z = 0.0f;
		outRegister4[3]->z = 0.0f;
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_SHORT4    : // 4D signed short
	{
		const short4** const short4Data4 = (const short4** const)data4;
		outRegister4[0]->x = (const float)short4Data4[0]->x;
		outRegister4[1]->x = (const float)short4Data4[1]->x;
		outRegister4[2]->x = (const float)short4Data4[2]->x;
		outRegister4[3]->x = (const float)short4Data4[3]->x;
		outRegister4[0]->y = (const float)short4Data4[0]->y;
		outRegister4[1]->y = (const float)short4Data4[1]->y;
		outRegister4[2]->y = (const float)short4Data4[2]->y;
		outRegister4[3]->y = (const float)short4Data4[3]->y;
		outRegister4[0]->z = (const float)short4Data4[0]->z;
		outRegister4[1]->z = (const float)short4Data4[1]->z;
		outRegister4[2]->z = (const float)short4Data4[2]->z;
		outRegister4[3]->z = (const float)short4Data4[3]->z;
		outRegister4[0]->w = (const float)short4Data4[0]->w;
		outRegister4[1]->w = (const float)short4Data4[1]->w;
		outRegister4[2]->w = (const float)short4Data4[2]->w;
		outRegister4[3]->w = (const float)short4Data4[3]->w;
	}
		break;
	case D3DDECLTYPE_UBYTE4N   : // Each of 4 bytes is normalized by dividing to 255.0 
	{
		const ubyte4** const ubyte4Data4 = (const ubyte4** const)data4;
		outRegister4[0]->x = ubyte4Data4[0]->x / 255.0f;
		outRegister4[1]->x = ubyte4Data4[1]->x / 255.0f;
		outRegister4[2]->x = ubyte4Data4[2]->x / 255.0f;
		outRegister4[3]->x = ubyte4Data4[3]->x / 255.0f;
		outRegister4[0]->y = ubyte4Data4[0]->y / 255.0f;
		outRegister4[1]->y = ubyte4Data4[1]->y / 255.0f;
		outRegister4[2]->y = ubyte4Data4[2]->y / 255.0f;
		outRegister4[3]->y = ubyte4Data4[3]->y / 255.0f;
		outRegister4[0]->z = ubyte4Data4[0]->z / 255.0f;
		outRegister4[1]->z = ubyte4Data4[1]->z / 255.0f;
		outRegister4[2]->z = ubyte4Data4[2]->z / 255.0f;
		outRegister4[3]->z = ubyte4Data4[3]->z / 255.0f;
		outRegister4[0]->w = ubyte4Data4[0]->w / 255.0f;
		outRegister4[1]->w = ubyte4Data4[1]->w / 255.0f;
		outRegister4[2]->w = ubyte4Data4[2]->w / 255.0f;
		outRegister4[3]->w = ubyte4Data4[3]->w / 255.0f;
	}
		break;
	case D3DDECLTYPE_SHORT2N   : // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
	{
		const short4** const short4Data4 = (const short4** const)data4;
		outRegister4[0]->x = short4Data4[0]->x / 32767.0f;
		outRegister4[1]->x = short4Data4[1]->x / 32767.0f;
		outRegister4[2]->x = short4Data4[2]->x / 32767.0f;
		outRegister4[3]->x = short4Data4[3]->x / 32767.0f;
		outRegister4[0]->y = short4Data4[0]->y / 32767.0f;
		outRegister4[1]->y = short4Data4[1]->y / 32767.0f;
		outRegister4[2]->y = short4Data4[2]->y / 32767.0f;
		outRegister4[3]->y = short4Data4[3]->y / 32767.0f;
		outRegister4[0]->z = 0.0f;
		outRegister4[1]->z = 0.0f;
		outRegister4[2]->z = 0.0f;
		outRegister4[3]->z = 0.0f;
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_SHORT4N   : // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
	{
		const short4** const short4Data4 = (const short4** const)data4;
		outRegister4[0]->x = short4Data4[0]->x / 32767.0f;
		outRegister4[1]->x = short4Data4[1]->x / 32767.0f;
		outRegister4[2]->x = short4Data4[2]->x / 32767.0f;
		outRegister4[3]->x = short4Data4[3]->x / 32767.0f;
		outRegister4[0]->y = short4Data4[0]->y / 32767.0f;
		outRegister4[1]->y = short4Data4[1]->y / 32767.0f;
		outRegister4[2]->y = short4Data4[2]->y / 32767.0f;
		outRegister4[3]->y = short4Data4[3]->y / 32767.0f;
		outRegister4[0]->z = short4Data4[0]->z / 32767.0f;
		outRegister4[1]->z = short4Data4[1]->z / 32767.0f;
		outRegister4[2]->z = short4Data4[2]->z / 32767.0f;
		outRegister4[3]->z = short4Data4[3]->z / 32767.0f;
		outRegister4[0]->w = short4Data4[0]->w / 32767.0f;
		outRegister4[1]->w = short4Data4[1]->w / 32767.0f;
		outRegister4[2]->w = short4Data4[2]->w / 32767.0f;
		outRegister4[3]->w = short4Data4[3]->w / 32767.0f;
	}
		break;
	case D3DDECLTYPE_USHORT2N  : // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
	{
		const ushort4** const ushort4Data4 = (const ushort4** const)data4;
		outRegister4[0]->x = ushort4Data4[0]->x / 65535.0f;
		outRegister4[1]->x = ushort4Data4[1]->x / 65535.0f;
		outRegister4[2]->x = ushort4Data4[2]->x / 65535.0f;
		outRegister4[3]->x = ushort4Data4[3]->x / 65535.0f;
		outRegister4[0]->y = ushort4Data4[0]->y / 65535.0f;
		outRegister4[1]->y = ushort4Data4[1]->y / 65535.0f;
		outRegister4[2]->y = ushort4Data4[2]->y / 65535.0f;
		outRegister4[3]->y = ushort4Data4[3]->y / 65535.0f;
		outRegister4[0]->z = 0.0f;
		outRegister4[1]->z = 0.0f;
		outRegister4[2]->z = 0.0f;
		outRegister4[3]->z = 0.0f;
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_USHORT4N  : // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
	{
		const ushort4** const ushort4Data4 = (const ushort4** const)data4;
		outRegister4[0]->x = ushort4Data4[0]->x / 65535.0f;
		outRegister4[1]->x = ushort4Data4[1]->x / 65535.0f;
		outRegister4[2]->x = ushort4Data4[2]->x / 65535.0f;
		outRegister4[3]->x = ushort4Data4[3]->x / 65535.0f;
		outRegister4[0]->y = ushort4Data4[0]->y / 65535.0f;
		outRegister4[1]->y = ushort4Data4[1]->y / 65535.0f;
		outRegister4[2]->y = ushort4Data4[2]->y / 65535.0f;
		outRegister4[3]->y = ushort4Data4[3]->y / 65535.0f;
		outRegister4[0]->z = ushort4Data4[0]->z / 65535.0f;
		outRegister4[1]->z = ushort4Data4[1]->z / 65535.0f;
		outRegister4[2]->z = ushort4Data4[2]->z / 65535.0f;
		outRegister4[3]->z = ushort4Data4[3]->z / 65535.0f;
		outRegister4[0]->w = ushort4Data4[0]->w / 65535.0f;
		outRegister4[1]->w = ushort4Data4[1]->w / 65535.0f;
		outRegister4[2]->w = ushort4Data4[2]->w / 65535.0f;
		outRegister4[3]->w = ushort4Data4[3]->w / 65535.0f;
	}
		break;
	case D3DDECLTYPE_UDEC3     : // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
	{
		const unsigned** const uData4 = (const unsigned** const)data4;

		const unsigned x4[4] =
		{
			(*(uData4[0]) ) & udec3mask,
			(*(uData4[1]) ) & udec3mask,
			(*(uData4[2]) ) & udec3mask,
			(*(uData4[3]) ) & udec3mask
		};

		const unsigned y4[4] =
		{
			(*(uData4[0]) >> 10) & udec3mask,
			(*(uData4[1]) >> 10) & udec3mask,
			(*(uData4[2]) >> 10) & udec3mask,
			(*(uData4[3]) >> 10) & udec3mask
		};

		const unsigned z4[4] =
		{
			(*(uData4[0]) >> 20) & udec3mask,
			(*(uData4[1]) >> 20) & udec3mask,
			(*(uData4[2]) >> 20) & udec3mask,
			(*(uData4[3]) >> 20) & udec3mask
		};

		outRegister4[0]->x = (const float)x4[0];
		outRegister4[1]->x = (const float)x4[1];
		outRegister4[2]->x = (const float)x4[2];
		outRegister4[3]->x = (const float)x4[3];
		outRegister4[0]->y = (const float)y4[0];
		outRegister4[1]->y = (const float)y4[1];
		outRegister4[2]->y = (const float)y4[2];
		outRegister4[3]->y = (const float)y4[3];
		outRegister4[0]->z = (const float)z4[0];
		outRegister4[1]->z = (const float)z4[1];
		outRegister4[2]->z = (const float)z4[2];
		outRegister4[3]->z = (const float)z4[3];
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_DEC3N     : // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
	{
		const unsigned** const uData4 = (const unsigned** const)data4;

		const unsigned xData4[4] =
		{
			*uData4[0] & udec3mask,
			*uData4[1] & udec3mask,
			*uData4[2] & udec3mask,
			*uData4[3] & udec3mask
		};

		// TODO: Test this for negative DEC3N values to make sure that this sign-math is correct:
		const int xDatai4[4] =
		{
			xData4[0] > dec3mask ? -(const int)xData4[0] : (const int)xData4[0],
			xData4[1] > dec3mask ? -(const int)xData4[1] : (const int)xData4[1],
			xData4[2] > dec3mask ? -(const int)xData4[2] : (const int)xData4[2],
			xData4[3] > dec3mask ? -(const int)xData4[3] : (const int)xData4[3]
		};

		const unsigned yData4[4] =
		{
			(*uData4[0] >> 10) & udec3mask,
			(*uData4[1] >> 10) & udec3mask,
			(*uData4[2] >> 10) & udec3mask,
			(*uData4[3] >> 10) & udec3mask
		};

		const int yDatai4[4] = 
		{
			yData4[0] > dec3mask ? -(const int)yData4[0] : (const int)yData4[0],
			yData4[1] > dec3mask ? -(const int)yData4[1] : (const int)yData4[1],
			yData4[2] > dec3mask ? -(const int)yData4[2] : (const int)yData4[2],
			yData4[3] > dec3mask ? -(const int)yData4[3] : (const int)yData4[3]
		};

		const unsigned zData4[4] =
		{
			(*uData4[0] >> 20) & udec3mask,
			(*uData4[1] >> 20) & udec3mask,
			(*uData4[2] >> 20) & udec3mask,
			(*uData4[3] >> 20) & udec3mask
		};

		const int zDatai4[4] = 
		{
			zData4[0] > dec3mask ? -(const int)zData4[0] : (const int)zData4[0],
			zData4[1] > dec3mask ? -(const int)zData4[1] : (const int)zData4[1],
			zData4[2] > dec3mask ? -(const int)zData4[2] : (const int)zData4[2],
			zData4[3] > dec3mask ? -(const int)zData4[3] : (const int)zData4[3]
		};

		outRegister4[0]->x = xDatai4[0] / 511.0f;
		outRegister4[1]->x = xDatai4[1] / 511.0f;
		outRegister4[2]->x = xDatai4[2] / 511.0f;
		outRegister4[3]->x = xDatai4[3] / 511.0f;
		outRegister4[0]->y = yDatai4[0] / 511.0f;
		outRegister4[1]->y = yDatai4[1] / 511.0f;
		outRegister4[2]->y = yDatai4[2] / 511.0f;
		outRegister4[3]->y = yDatai4[3] / 511.0f;
		outRegister4[0]->z = zDatai4[0] / 511.0f;
		outRegister4[1]->z = zDatai4[1] / 511.0f;
		outRegister4[2]->z = zDatai4[2] / 511.0f;
		outRegister4[3]->z = zDatai4[3] / 511.0f;
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT16_2 : // Two 16-bit floating point values, expanded to (value, value, 0, 1)
	{
		const D3DXVECTOR2_16F** const f16Data2_4 = (const D3DXVECTOR2_16F** const)data4;

		const D3DXVECTOR4 tempVecs[4] =
		{
			D3DXVECTOR4(*f16Data2_4[0]),
			D3DXVECTOR4(*f16Data2_4[1]),
			D3DXVECTOR4(*f16Data2_4[2]),
			D3DXVECTOR4(*f16Data2_4[3])
		};

		outRegister4[0]->x = tempVecs[0].x;
		outRegister4[1]->x = tempVecs[1].x;
		outRegister4[2]->x = tempVecs[2].x;
		outRegister4[3]->x = tempVecs[3].x;
		outRegister4[0]->y = tempVecs[0].y;
		outRegister4[1]->y = tempVecs[1].y;
		outRegister4[2]->y = tempVecs[2].y;
		outRegister4[3]->y = tempVecs[3].y;
		outRegister4[0]->z = 0.0f;
		outRegister4[1]->z = 0.0f;
		outRegister4[2]->z = 0.0f;
		outRegister4[3]->z = 0.0f;
		outRegister4[0]->w = 1.0f;
		outRegister4[1]->w = 1.0f;
		outRegister4[2]->w = 1.0f;
		outRegister4[3]->w = 1.0f;
	}
		break;
	case D3DDECLTYPE_FLOAT16_4 : // Four 16-bit floating point values
	{
		const D3DXVECTOR2_16F** const f16Data2_4 = (const D3DXVECTOR2_16F** const)data4;

		const D3DXVECTOR4 tempVecs[4] =
		{
			D3DXVECTOR4(*f16Data2_4[0]),
			D3DXVECTOR4(*f16Data2_4[1]),
			D3DXVECTOR4(*f16Data2_4[2]),
			D3DXVECTOR4(*f16Data2_4[3])
		};

		outRegister4[0]->x = tempVecs[0].x;
		outRegister4[1]->x = tempVecs[1].x;
		outRegister4[2]->x = tempVecs[2].x;
		outRegister4[3]->x = tempVecs[3].x;
		outRegister4[0]->y = tempVecs[0].y;
		outRegister4[1]->y = tempVecs[1].y;
		outRegister4[2]->y = tempVecs[2].y;
		outRegister4[3]->y = tempVecs[3].y;
		outRegister4[0]->z = tempVecs[0].z;
		outRegister4[1]->z = tempVecs[1].z;
		outRegister4[2]->z = tempVecs[2].z;
		outRegister4[3]->z = tempVecs[3].z;
		outRegister4[0]->w = tempVecs[0].w;
		outRegister4[1]->w = tempVecs[1].w;
		outRegister4[2]->w = tempVecs[2].w;
		outRegister4[3]->w = tempVecs[3].w;
	}
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Undefined vertex decl element type");
#endif
	case D3DDECLTYPE_UNUSED    : // When the type field in a decl is unused.
	{
		// Default register initialization:
		*outRegister4[0] = vertShaderInputRegisterDefault;
		*outRegister4[1] = vertShaderInputRegisterDefault;
		*outRegister4[2] = vertShaderInputRegisterDefault;
		*outRegister4[3] = vertShaderInputRegisterDefault;
	}
		break;
	}
}

static inline const void* const GetDefaultInputRegisterForType(const D3DDECLTYPE elemType)
{
	switch (elemType)
	{
	default:
	case D3DDECLTYPE_UNUSED:
#ifdef _DEBUG
	{
		__debugbreak(); // Should never be here!
	}
#else
		__assume(0);
#endif
	case D3DDECLTYPE_FLOAT1://    =  0,  // 1D float expanded to (value, 0., 0., 1.)
	case D3DDECLTYPE_FLOAT2://    =  1,  // 2D float expanded to (value, value, 0., 1.)
	case D3DDECLTYPE_FLOAT3://    =  2,  // 3D float expanded to (value, value, value, 1.)
	case D3DDECLTYPE_FLOAT4://    =  3,  // 4D float
		return &vertShaderInputRegisterDefault;
	case D3DDECLTYPE_D3DCOLOR://  =  4,  // 4D packed unsigned bytes mapped to 0. to 1. range. Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
	{
		static const D3DCOLOR defaultRegisterColor = D3DCOLOR_ARGB(255, 0, 0, 0);
		return &defaultRegisterColor;
	}
	case D3DDECLTYPE_UBYTE4://    =  5,  // 4D unsigned byte
	{
		static const BYTE defaultByte4[4] = { 0, 0, 0, 1 };
		return defaultByte4;
	}
	case D3DDECLTYPE_SHORT2://    =  6,  // 2D signed short expanded to (value, value, 0., 1.)
	case D3DDECLTYPE_SHORT4://    =  7,  // 4D signed short
	{
		static const short defaultSHORT4[4] = { 0, 0, 0, 1 };
		return defaultSHORT4;
	}
	case D3DDECLTYPE_UBYTE4N://   =  8,  // Each of 4 bytes is normalized by dividing to 255.0
	{
		static const BYTE defaultUBYTE4[4] = { 0, 0, 0, 255 };
		return defaultUBYTE4;
	}
	case D3DDECLTYPE_SHORT2N://   =  9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
	case D3DDECLTYPE_SHORT4N://   = 10,  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
	{
		static const short defaultSHORTN4[4] = { 0, 0, 0, 32767 };
		return defaultSHORTN4;
	}
    case D3DDECLTYPE_USHORT2N://  = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
    case D3DDECLTYPE_USHORT4N://  = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
	{
		static const unsigned short defaultUSHORTN4[4] = { 0, 0, 0, 65535 };
		return defaultUSHORTN4;
	}
    case D3DDECLTYPE_UDEC3://     = 13,  // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
    case D3DDECLTYPE_DEC3N://     = 14,  // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
	{
		static const DWORD defaultDec3 = 0x00000000;
		return &defaultDec3;
	}
    case D3DDECLTYPE_FLOAT16_2:// = 15,  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
    case D3DDECLTYPE_FLOAT16_4:// = 16,  // Four 16-bit floating point values
	{
		static const D3DXVECTOR4_16F defaultHalf4(D3DXFLOAT16(0.0f), D3DXFLOAT16(0.0f), D3DXFLOAT16(0.0f), D3DXFLOAT16(1.0f) );
		return &defaultHalf4;
	}
	}
}

static inline void LoadElementToRegister4Validated(D3DXVECTOR4* const (&outRegister4)[4], const D3DDECLTYPE elemType, const void** data4, const void* const streamEndPtr)
{
	const void* const defaultInputRegisterValue = GetDefaultInputRegisterForType(elemType);
	if (data4[0] > streamEndPtr)
		data4[0] = defaultInputRegisterValue;
	if (data4[1] > streamEndPtr)
		data4[1] = defaultInputRegisterValue;
	if (data4[2] > streamEndPtr)
		data4[2] = defaultInputRegisterValue;
	if (data4[3] > streamEndPtr)
		data4[3] = defaultInputRegisterValue;

	LoadElementToRegister4(outRegister4, elemType, data4);
}
#endif // #ifdef RUN_SHADERS_IN_WARPS

void IDirect3DDevice9Hook::LoadVertexInputElement(const DebuggableD3DVERTEXELEMENT9& element, const unsigned char* const dataPtr, const unsigned registerIndex, VShaderEngine* const vertShader) const
{
	const unsigned elementSize = IDirect3DVertexDeclaration9Hook::GetElementSizeFromType(element.Type);

	// TODO: Parse vertex shader bytecode to determine mapping between inputRegisters and element usages (and usage indices)
	D3DXVECTOR4* const elementPtr = &vertShader->inputRegisters[0].v[registerIndex];

#ifdef _DEBUG
	switch (element.Usage)
	{
	case D3DDECLUSAGE_POSITIONT:
	case D3DDECLUSAGE_POSITION:
	case D3DDECLUSAGE_PSIZE:
	case D3DDECLUSAGE_BLENDWEIGHT:
	case D3DDECLUSAGE_BLENDINDICES:
	case D3DDECLUSAGE_NORMAL:
	case D3DDECLUSAGE_TEXCOORD:
	case D3DDECLUSAGE_TANGENT:
	case D3DDECLUSAGE_BINORMAL:
	case D3DDECLUSAGE_COLOR:
	case D3DDECLUSAGE_FOG:
		break;
	default:
	case D3DDECLUSAGE_TESSFACTOR: // Unsupported
	case D3DDECLUSAGE_DEPTH:
	case D3DDECLUSAGE_SAMPLE:
		DbgBreakPrint("Error: Pixel-shader only vertex element detected in vertex shader declaration"); // These should be pixel-shader only
		break;
	}
#endif

	// Do the actual store:
	const void* const streamEndPtr = currentState.currentStreamEnds[element.Stream].dataTypeStreamEnds[element.Type];
	LoadElementToRegisterValidated(*elementPtr, element.Type, dataPtr, streamEndPtr);
}

#ifdef RUN_SHADERS_IN_WARPS
void IDirect3DDevice9Hook::LoadVertexInputElement4(const DebuggableD3DVERTEXELEMENT9& element, const unsigned char* dataPtr[4], const unsigned registerIndex, VShaderEngine* const vertShader) const
{
	const unsigned elementSize = IDirect3DVertexDeclaration9Hook::GetElementSizeFromType(element.Type);
	D3DXVECTOR4* const elementPtr4[4] = 
	{
		&vertShader->inputRegisters[0].v[registerIndex],
		&vertShader->inputRegisters[1].v[registerIndex],
		&vertShader->inputRegisters[2].v[registerIndex],
		&vertShader->inputRegisters[3].v[registerIndex]
	};

#ifdef _DEBUG
	switch (element.Usage)
	{
	case D3DDECLUSAGE_POSITIONT:
	case D3DDECLUSAGE_POSITION:
	case D3DDECLUSAGE_PSIZE:
	case D3DDECLUSAGE_BLENDWEIGHT:
	case D3DDECLUSAGE_BLENDINDICES:
	case D3DDECLUSAGE_NORMAL:
	case D3DDECLUSAGE_TEXCOORD:
	case D3DDECLUSAGE_TANGENT:
	case D3DDECLUSAGE_BINORMAL:
	case D3DDECLUSAGE_COLOR:
	case D3DDECLUSAGE_FOG:
		break;
	default:
	case D3DDECLUSAGE_TESSFACTOR: // Unsupported
	case D3DDECLUSAGE_DEPTH:
	case D3DDECLUSAGE_SAMPLE:
		DbgBreakPrint("Error: Pixel-shader only vertex element detected in vertex shader declaration"); // These should be pixel-shader only
		break;
	}
#endif

	// Do the actual store:
	const void* const streamEndPtr = currentState.currentStreamEnds[element.Stream].dataTypeStreamEnds[element.Type];
	LoadElementToRegister4Validated(elementPtr4, element.Type, (const void**)dataPtr, streamEndPtr);
}
#endif // #ifdef RUN_SHADERS_IN_WARPS

/*void IDirect3DDevice9Hook::StoreVertexOutputElement(const DebuggableD3DVERTEXELEMENT9& element, unsigned char* const outputPtr, const D3DDECLUSAGE usage, const unsigned usageIndex) const
{
	const unsigned elementSize = IDirect3DVertexDeclaration9Hook::GetElementSizeFromType(element.Type);
	const void* elementPtr = NULL;
	switch (usage)
	{
	case D3DDECLUSAGE_POSITIONT:
	case D3DDECLUSAGE_POSITION:
		elementPtr = &deviceMainVShaderEngine.outputRegisters->oPos;
		break;
	case D3DDECLUSAGE_PSIZE:
		elementPtr = &deviceMainVShaderEngine.outputRegisters->oPts;
		break;
	case D3DDECLUSAGE_BLENDWEIGHT:
	case D3DDECLUSAGE_BLENDINDICES:
	case D3DDECLUSAGE_NORMAL:
	case D3DDECLUSAGE_TEXCOORD:
	case D3DDECLUSAGE_TANGENT:
	case D3DDECLUSAGE_BINORMAL:
		elementPtr = &deviceMainVShaderEngine.outputRegisters->vs_interpolated_outputs.vs_2_0_outputs.oT[usageIndex];
		break;
	case D3DDECLUSAGE_COLOR:
		elementPtr = &deviceMainVShaderEngine.outputRegisters->vs_interpolated_outputs.vs_2_0_outputs.oD[usageIndex];
		break;
	case D3DDECLUSAGE_FOG:
		elementPtr = &deviceMainVShaderEngine.outputRegisters->oFog;
		break;
	default:
	case D3DDECLUSAGE_TESSFACTOR: // Unsupported
	case D3DDECLUSAGE_DEPTH:
	case D3DDECLUSAGE_SAMPLE:
		DbgBreakPrint("Error: Pixel-shader only vertex element detected in vertex shader declaration"); // These should be pixel-shader only
		break;
	}

	// Do the actual store:
	memcpy(outputPtr, elementPtr, elementSize);
}*/

void IDirect3DDevice9Hook::ApplyViewportTransform(D3DXVECTOR4& positionT) const
{
	// For reference, see this MSDN page that describes the D3D9 Viewport Transform: https://msdn.microsoft.com/en-us/library/windows/desktop/bb206341(v=vs.85).aspx

	const float reciprocalHomogenousW = 1.0f / positionT.w;

	// Scale [-1, 1] space to [0.5, width + 0.5] space
	positionT.x = positionT.x * reciprocalHomogenousW * currentState.cachedViewport.halfWidthF + currentState.cachedViewport.halfWidthF
#ifdef ADD_D3D9_HALFPIXEL_OFFSET
		+ 0.5f
#endif
		;

	// Scale [-1, 1] space to [0.5, height + 0.5] space
	positionT.y = positionT.y * reciprocalHomogenousW * -currentState.cachedViewport.halfHeightF + currentState.cachedViewport.halfHeightF // Note, do the Y-flip here
#ifdef ADD_D3D9_HALFPIXEL_OFFSET
		+ 0.5f
#endif
		;

	// Scale from [0, 1] space to [minZ, maxZ] space
	positionT.z = positionT.z * reciprocalHomogenousW * currentState.cachedViewport.zScale + currentState.cachedViewport.viewport.MinZ;
}

#ifdef RUN_SHADERS_IN_WARPS
void IDirect3DDevice9Hook::ApplyViewportTransform4(D3DXVECTOR4* (&positionT4)[4]) const
{
	// For reference, see this MSDN page that describes the D3D9 Viewport Transform: https://msdn.microsoft.com/en-us/library/windows/desktop/bb206341(v=vs.85).aspx
	const float reciprocalHomogenousW4[4] = 
	{
		1.0f / positionT4[0]->w,
		1.0f / positionT4[1]->w,
		1.0f / positionT4[2]->w,
		1.0f / positionT4[3]->w,
	};

	// Scale [-1, 1] space to [0.5, width + 0.5] space
	positionT4[0]->x *= reciprocalHomogenousW4[0];
	positionT4[1]->x *= reciprocalHomogenousW4[1];
	positionT4[2]->x *= reciprocalHomogenousW4[2];
	positionT4[3]->x *= reciprocalHomogenousW4[3];
	positionT4[0]->x *= currentState.cachedViewport.halfWidthF;
	positionT4[1]->x *= currentState.cachedViewport.halfWidthF;
	positionT4[2]->x *= currentState.cachedViewport.halfWidthF;
	positionT4[3]->x *= currentState.cachedViewport.halfWidthF;
	positionT4[0]->x += currentState.cachedViewport.halfWidthF;
	positionT4[1]->x += currentState.cachedViewport.halfWidthF;
	positionT4[2]->x += currentState.cachedViewport.halfWidthF;
	positionT4[3]->x += currentState.cachedViewport.halfWidthF;
#ifdef ADD_D3D9_HALFPIXEL_OFFSET
	positionT4[0]->x += 0.5f;
	positionT4[1]->x += 0.5f;
	positionT4[2]->x += 0.5f;
	positionT4[3]->x += 0.5f;
#endif

	// Scale [-1, 1] space to [0.5, height + 0.5] space
	positionT4[0]->y *= reciprocalHomogenousW4[0];
	positionT4[1]->y *= reciprocalHomogenousW4[1];
	positionT4[2]->y *= reciprocalHomogenousW4[2];
	positionT4[3]->y *= reciprocalHomogenousW4[3];
	positionT4[0]->y *= -currentState.cachedViewport.halfHeightF;
	positionT4[1]->y *= -currentState.cachedViewport.halfHeightF;
	positionT4[2]->y *= -currentState.cachedViewport.halfHeightF;
	positionT4[3]->y *= -currentState.cachedViewport.halfHeightF;
	positionT4[0]->y += currentState.cachedViewport.halfHeightF; // Note, do the Y-flip here
	positionT4[1]->y += currentState.cachedViewport.halfHeightF;
	positionT4[2]->y += currentState.cachedViewport.halfHeightF;
	positionT4[3]->y += currentState.cachedViewport.halfHeightF;
#ifdef ADD_D3D9_HALFPIXEL_OFFSET
	positionT4[0]->y += 0.5f;
	positionT4[1]->y += 0.5f;
	positionT4[2]->y += 0.5f;
	positionT4[3]->y += 0.5f;
#endif

	// Scale from [0, 1] space to [minZ, maxZ] space
	positionT4[0]->z *= reciprocalHomogenousW4[0];
	positionT4[1]->z *= reciprocalHomogenousW4[1];
	positionT4[2]->z *= reciprocalHomogenousW4[2];
	positionT4[3]->z *= reciprocalHomogenousW4[3];
	positionT4[0]->z *= currentState.cachedViewport.zScale;
	positionT4[1]->z *= currentState.cachedViewport.zScale;
	positionT4[2]->z *= currentState.cachedViewport.zScale;
	positionT4[3]->z *= currentState.cachedViewport.zScale;
	positionT4[0]->z += currentState.cachedViewport.viewport.MinZ;
	positionT4[1]->z += currentState.cachedViewport.viewport.MinZ;
	positionT4[2]->z += currentState.cachedViewport.viewport.MinZ;
	positionT4[3]->z += currentState.cachedViewport.viewport.MinZ;
}
#endif // #ifdef RUN_SHADERS_IN_WARPS

template <const bool anyUserClipPlanesEnabled>
void IDirect3DDevice9Hook::ComputeVertexClipCodes(const D3DXVECTOR4& vertexPosition, VS_2_0_OutputRegisters* const shadedVertex) const
{
	const float W = vertexPosition.w;
	const float negW = -W;

	// TODO: Should BOTTOM and TOP be flipped here since clipping is performed prior to the viewport
	// transform which flips the Y-axis?

	VS_2_0_OutputRegisters::vertexClipUnion vertexClipLocal = {0};

	if (vertexPosition.x < negW)
		vertexClipLocal.clipCodesCombined |= D3DCS_LEFT;
	if (vertexPosition.x > W)
		vertexClipLocal.clipCodesCombined |= D3DCS_RIGHT;
	if (vertexPosition.y < negW)
		vertexClipLocal.clipCodesCombined |= D3DCS_BOTTOM;
	if (vertexPosition.y > W)
		vertexClipLocal.clipCodesCombined |= D3DCS_TOP;
	if (vertexPosition.z < 0.0f)
		vertexClipLocal.clipCodesCombined |= D3DCS_FRONT;
	if (vertexPosition.z > W)
		vertexClipLocal.clipCodesCombined |= D3DCS_BACK;

	if (anyUserClipPlanesEnabled)
	{
		for (unsigned char userClipPlaneIndex = 0; userClipPlaneIndex < MAX_USER_CLIP_PLANES_SUPPORTED; ++userClipPlaneIndex)
		{
			if (currentState.currentRenderStates.renderStatesUnion.namedStates.clipPlaneEnable & (1 << userClipPlaneIndex) )
			{
				float planeDistance;
				dp4(planeDistance, vertexPosition, *(const D3DXVECTOR4* const)&(currentState.currentClippingPlanes[userClipPlaneIndex]) );
				const bool clipSucceed = planeDistance >= 0.0f;
				const unsigned short clipSucceedDWORD = ( (const unsigned short)clipSucceed) << (6 + userClipPlaneIndex);
				vertexClipLocal.clipCodesCombined |= clipSucceedDWORD;
			}
		}
	}

	const float rhw = 1.0f / W;
	const float clipX = vertexPosition.x * rhw;
	const float clipY = vertexPosition.y * rhw;

	if (clipX < SUBPIXEL_MIN_VALUEF)
		vertexClipLocal.clipCodesCombined |= D3DCS_GBLEFT;
	if (clipX > SUBPIXEL_MAX_VALUEF)
		vertexClipLocal.clipCodesCombined |= D3DCS_GBRIGHT;
	if (clipY < SUBPIXEL_MIN_VALUEF)
		vertexClipLocal.clipCodesCombined |= D3DCS_GBTOP;
	if (clipY > SUBPIXEL_MAX_VALUEF)
		vertexClipLocal.clipCodesCombined |= D3DCS_GBBOTTOM;

	shadedVertex->vertexClip.clipCodesCombined = vertexClipLocal.clipCodesCombined;
}

#ifdef RUN_SHADERS_IN_WARPS
template <const bool anyUserClipPlanesEnabled>
void IDirect3DDevice9Hook::ComputeVertexClipCodes4(const D3DXVECTOR4* (&vertexPosition4)[4], VS_2_0_OutputRegisters* const (&shadedVerts)[4]) const
{
	VS_2_0_OutputRegisters::vertexClipUnion vertexClipLocal4[4] = {0};

	const float W4[4] =
	{
		vertexPosition4[0]->w,
		vertexPosition4[1]->w,
		vertexPosition4[2]->w,
		vertexPosition4[3]->w
	};

	const float negW4[4] =
	{
		-W4[0],
		-W4[1],
		-W4[2],
		-W4[3]
	};

	// TODO: Should BOTTOM and TOP be flipped here since clipping is performed prior to the viewport
	// transform which flips the Y-axis?

	if (vertexPosition4[0]->x < negW4[0]) vertexClipLocal4[0].clipCodesCombined |= D3DCS_LEFT;
	if (vertexPosition4[1]->x < negW4[1]) vertexClipLocal4[1].clipCodesCombined |= D3DCS_LEFT;
	if (vertexPosition4[2]->x < negW4[2]) vertexClipLocal4[2].clipCodesCombined |= D3DCS_LEFT;
	if (vertexPosition4[3]->x < negW4[3]) vertexClipLocal4[3].clipCodesCombined |= D3DCS_LEFT;

	if (vertexPosition4[0]->x > W4[0]) vertexClipLocal4[0].clipCodesCombined |= D3DCS_RIGHT;
	if (vertexPosition4[1]->x > W4[1]) vertexClipLocal4[1].clipCodesCombined |= D3DCS_RIGHT;
	if (vertexPosition4[2]->x > W4[2]) vertexClipLocal4[2].clipCodesCombined |= D3DCS_RIGHT;
	if (vertexPosition4[3]->x > W4[3]) vertexClipLocal4[3].clipCodesCombined |= D3DCS_RIGHT;

	if (vertexPosition4[0]->y < negW4[0]) vertexClipLocal4[0].clipCodesCombined |= D3DCS_BOTTOM;
	if (vertexPosition4[1]->y < negW4[1]) vertexClipLocal4[1].clipCodesCombined |= D3DCS_BOTTOM;
	if (vertexPosition4[2]->y < negW4[2]) vertexClipLocal4[2].clipCodesCombined |= D3DCS_BOTTOM;
	if (vertexPosition4[3]->y < negW4[3]) vertexClipLocal4[3].clipCodesCombined |= D3DCS_BOTTOM;

	if (vertexPosition4[0]->y > W4[0]) vertexClipLocal4[0].clipCodesCombined |= D3DCS_TOP;
	if (vertexPosition4[1]->y > W4[1]) vertexClipLocal4[1].clipCodesCombined |= D3DCS_TOP;
	if (vertexPosition4[2]->y > W4[2]) vertexClipLocal4[2].clipCodesCombined |= D3DCS_TOP;
	if (vertexPosition4[3]->y > W4[3]) vertexClipLocal4[3].clipCodesCombined |= D3DCS_TOP;

	if (vertexPosition4[0]->z < 0.0f) vertexClipLocal4[0].clipCodesCombined |= D3DCS_FRONT;
	if (vertexPosition4[1]->z < 0.0f) vertexClipLocal4[1].clipCodesCombined |= D3DCS_FRONT;
	if (vertexPosition4[2]->z < 0.0f) vertexClipLocal4[2].clipCodesCombined |= D3DCS_FRONT;
	if (vertexPosition4[3]->z < 0.0f) vertexClipLocal4[3].clipCodesCombined |= D3DCS_FRONT;

	if (vertexPosition4[0]->z > W4[0]) vertexClipLocal4[0].clipCodesCombined |= D3DCS_BACK;
	if (vertexPosition4[1]->z > W4[1]) vertexClipLocal4[1].clipCodesCombined |= D3DCS_BACK;
	if (vertexPosition4[2]->z > W4[2]) vertexClipLocal4[2].clipCodesCombined |= D3DCS_BACK;
	if (vertexPosition4[3]->z > W4[3]) vertexClipLocal4[3].clipCodesCombined |= D3DCS_BACK;

	if (anyUserClipPlanesEnabled)
	{
		for (unsigned char userClipPlaneIndex = 0; userClipPlaneIndex < MAX_USER_CLIP_PLANES_SUPPORTED; ++userClipPlaneIndex)
		{
			if (currentState.currentRenderStates.renderStatesUnion.namedStates.clipPlaneEnable & (1 << userClipPlaneIndex) )
			{
				float planeDistance4[4];
				dp4_4(planeDistance4, vertexPosition4, *(const D3DXVECTOR4* const)&(currentState.currentClippingPlanes[userClipPlaneIndex]) );
				const bool clipSucceed4[4] = 
				{
					planeDistance4[0] >= 0.0f,
					planeDistance4[1] >= 0.0f,
					planeDistance4[2] >= 0.0f,
					planeDistance4[3] >= 0.0f
				};
				const unsigned short clipSucceedDWORD4[4] = 
				{
					(const unsigned short)( ( (const unsigned short)clipSucceed4[0]) << (6 + userClipPlaneIndex) ),
					(const unsigned short)( ( (const unsigned short)clipSucceed4[1]) << (6 + userClipPlaneIndex) ),
					(const unsigned short)( ( (const unsigned short)clipSucceed4[2]) << (6 + userClipPlaneIndex) ),
					(const unsigned short)( ( (const unsigned short)clipSucceed4[3]) << (6 + userClipPlaneIndex) )
				};

				vertexClipLocal4[0].clipCodesCombined |= clipSucceedDWORD4[0];
				vertexClipLocal4[1].clipCodesCombined |= clipSucceedDWORD4[1];
				vertexClipLocal4[2].clipCodesCombined |= clipSucceedDWORD4[2];
				vertexClipLocal4[3].clipCodesCombined |= clipSucceedDWORD4[3];
			}
		}
	}

	const float rhw4[4] =
	{
		1.0f / W4[0],
		1.0f / W4[1],
		1.0f / W4[2],
		1.0f / W4[3]
	};

	const float clipX4[4] =
	{
		vertexPosition4[0]->x * rhw4[0],
		vertexPosition4[1]->x * rhw4[1],
		vertexPosition4[2]->x * rhw4[2],
		vertexPosition4[3]->x * rhw4[3]
	};

	const float clipY4[4] =
	{
		vertexPosition4[0]->y * rhw4[0],
		vertexPosition4[1]->y * rhw4[1],
		vertexPosition4[2]->y * rhw4[2],
		vertexPosition4[3]->y * rhw4[3]
	};

	if (clipX4[0] < SUBPIXEL_MIN_VALUEF) vertexClipLocal4[0].clipCodesCombined |= D3DCS_GBLEFT;
	if (clipX4[1] < SUBPIXEL_MIN_VALUEF) vertexClipLocal4[1].clipCodesCombined |= D3DCS_GBLEFT;
	if (clipX4[2] < SUBPIXEL_MIN_VALUEF) vertexClipLocal4[2].clipCodesCombined |= D3DCS_GBLEFT;
	if (clipX4[3] < SUBPIXEL_MIN_VALUEF) vertexClipLocal4[3].clipCodesCombined |= D3DCS_GBLEFT;

	if (clipX4[0] > SUBPIXEL_MAX_VALUEF) vertexClipLocal4[0].clipCodesCombined |= D3DCS_GBRIGHT;
	if (clipX4[1] > SUBPIXEL_MAX_VALUEF) vertexClipLocal4[1].clipCodesCombined |= D3DCS_GBRIGHT;
	if (clipX4[2] > SUBPIXEL_MAX_VALUEF) vertexClipLocal4[2].clipCodesCombined |= D3DCS_GBRIGHT;
	if (clipX4[3] > SUBPIXEL_MAX_VALUEF) vertexClipLocal4[3].clipCodesCombined |= D3DCS_GBRIGHT;

	if (clipY4[0] < SUBPIXEL_MIN_VALUEF) vertexClipLocal4[0].clipCodesCombined |= D3DCS_GBTOP;
	if (clipY4[1] < SUBPIXEL_MIN_VALUEF) vertexClipLocal4[1].clipCodesCombined |= D3DCS_GBTOP;
	if (clipY4[2] < SUBPIXEL_MIN_VALUEF) vertexClipLocal4[2].clipCodesCombined |= D3DCS_GBTOP;
	if (clipY4[3] < SUBPIXEL_MIN_VALUEF) vertexClipLocal4[3].clipCodesCombined |= D3DCS_GBTOP;

	if (clipY4[0] > SUBPIXEL_MAX_VALUEF) vertexClipLocal4[0].clipCodesCombined |= D3DCS_GBBOTTOM;
	if (clipY4[1] > SUBPIXEL_MAX_VALUEF) vertexClipLocal4[1].clipCodesCombined |= D3DCS_GBBOTTOM;
	if (clipY4[2] > SUBPIXEL_MAX_VALUEF) vertexClipLocal4[2].clipCodesCombined |= D3DCS_GBBOTTOM;
	if (clipY4[3] > SUBPIXEL_MAX_VALUEF) vertexClipLocal4[3].clipCodesCombined |= D3DCS_GBBOTTOM;

	shadedVerts[0]->vertexClip.clipCodesCombined = vertexClipLocal4[0].clipCodesCombined;
	shadedVerts[1]->vertexClip.clipCodesCombined = vertexClipLocal4[1].clipCodesCombined;
	shadedVerts[2]->vertexClip.clipCodesCombined = vertexClipLocal4[2].clipCodesCombined;
	shadedVerts[3]->vertexClip.clipCodesCombined = vertexClipLocal4[3].clipCodesCombined;
}
#endif // #ifdef RUN_SHADERS_IN_WARPS

template <const bool anyUserClipPlanesEnabled>
void IDirect3DDevice9Hook::ProcessVertexToBuffer(const DeclarationSemanticMapping& mapping, VShaderEngine* const vertShader, VS_2_0_OutputRegisters* const outputVert, const unsigned vertexIndex) const
{
	// Load input vert:
	const ShaderInfo& vertexShaderInfo = currentState.currentVertexShader->GetShaderInfo();
	const unsigned numDeclaredRegisters = vertexShaderInfo.declaredRegisters.size();
	for (unsigned x = 0; x < numDeclaredRegisters; ++x)
	{
		const DeclaredRegister& thisReg = vertexShaderInfo.declaredRegisters[x];

		if (thisReg.registerType != D3DSPR_INPUT)
			continue;

		const DebuggableD3DVERTEXELEMENT9* const element = mapping.vals[thisReg.usageType][thisReg.usageIndex];
		if (!element)
		{
			D3DXVECTOR4& loadRegister = vertShader->inputRegisters[0].v[thisReg.registerIndex];
			loadRegister = vertShaderInputRegisterDefault;
			continue;
		}

		const StreamSource& thisElementStream = currentState.currentStreams[element->Stream];
		if (thisElementStream.vertexBuffer != NULL)
		{
			const BYTE* const thisStreamBuffer = thisElementStream.vertexBuffer->GetInternalDataBuffer() + thisElementStream.streamOffset;
			const unsigned char* thisStreamVertStart = thisStreamBuffer + thisElementStream.streamStride * vertexIndex;
			LoadVertexInputElement(*element, thisStreamVertStart + element->Offset, thisReg.registerIndex, vertShader);
		}
		else
		{
			vertShader->inputRegisters[0].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
		}
	}

	// Very important to reset the state machine back to its original settings!
	vertShader->Reset(&outputVert, 1);

	// Run vertex shader:
#ifndef FORCE_INTERPRETED_VERTEX_SHADER
	if (currentState.currentVertexShader->jitShaderMain)
	{
		// JIT engine:
		currentState.currentVertexShader->jitShaderMain(*vertShader);
	}
	else
#endif
	{
		// Interpreter engine:
		while (vertShader->InterpreterExecStep1() );
	}

	// Apply the viewport transform to our vertex position:
#ifdef _DEBUG
	if (SkipVertexProcessing() )
	{
		DbgBreakPrint("Error: Shouldn't be processing this vertex!");
	}
#endif

	D3DXVECTOR4& vertexPosition = currentState.currentVertexShader->GetPosition(*outputVert);
	ComputeVertexClipCodes<anyUserClipPlanesEnabled>(vertexPosition, outputVert);
	ApplyViewportTransform(vertexPosition);
}

#ifdef RUN_SHADERS_IN_WARPS
template <const bool anyUserClipPlanesEnabled>
void IDirect3DDevice9Hook::ProcessVertexToBuffer4(const DeclarationSemanticMapping& mapping, VShaderEngine* const vertShader, VS_2_0_OutputRegisters* (&outputVerts)[4], const unsigned* const vertexIndex) const
{
	// Load input vert:
	const ShaderInfo& vertexShaderInfo = currentState.currentVertexShader->GetShaderInfo();
	const unsigned numDeclaredRegisters = vertexShaderInfo.declaredRegisters.size();
	for (unsigned x = 0; x < numDeclaredRegisters; ++x)
	{
		const DeclaredRegister& thisReg = vertexShaderInfo.declaredRegisters[x];

		if (thisReg.registerType != D3DSPR_INPUT)
			continue;

		const DebuggableD3DVERTEXELEMENT9* const element = mapping.vals[thisReg.usageType][thisReg.usageIndex];
		if (!element)
		{
			vertShader->inputRegisters[0].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
			vertShader->inputRegisters[1].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
			vertShader->inputRegisters[2].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
			vertShader->inputRegisters[3].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
			continue;
		}

		const StreamSource& thisElementStream = currentState.currentStreams[element->Stream];
		if (thisElementStream.vertexBuffer != NULL)
		{
			const BYTE* const thisStreamBuffer = thisElementStream.vertexBuffer->GetInternalDataBuffer() + thisElementStream.streamOffset;
			const unsigned char* thisStreamVertStart4[4] = 
			{
				thisStreamBuffer + thisElementStream.streamStride * vertexIndex[0] + element->Offset,
				thisStreamBuffer + thisElementStream.streamStride * vertexIndex[1] + element->Offset,
				thisStreamBuffer + thisElementStream.streamStride * vertexIndex[2] + element->Offset,
				thisStreamBuffer + thisElementStream.streamStride * vertexIndex[3] + element->Offset
			};
			LoadVertexInputElement4(*element, thisStreamVertStart4, thisReg.registerIndex, vertShader);
		}
		else
		{
			vertShader->inputRegisters[0].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
			vertShader->inputRegisters[1].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
			vertShader->inputRegisters[2].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
			vertShader->inputRegisters[3].v[thisReg.registerIndex] = vertShaderInputRegisterDefault;
		}
	}

	// Very important to reset the state machine back to its original settings!
	vertShader->Reset(outputVerts, 4);

	// Run vertex shader:
#ifndef FORCE_INTERPRETED_VERTEX_SHADER
	if (currentState.currentVertexShader->jitShaderMain4)
	{
		// JIT engine:
		currentState.currentVertexShader->jitShaderMain4(*vertShader);
	}
	else
#endif
	{
		// Interpreter engine:
		while (vertShader->InterpreterExecStep4() );
	}

	// Apply the viewport transform to our vertex position:
#ifdef _DEBUG
	if (SkipVertexProcessing() )
	{
		DbgBreakPrint("Error: Shouldn't be processing this vertex!");
	}
#endif

	D3DXVECTOR4* vertexPositions[4];
	currentState.currentVertexShader->GetPosition4(outputVerts, vertexPositions);
	ComputeVertexClipCodes4<anyUserClipPlanesEnabled>( (const D3DXVECTOR4* (&)[4])(vertexPositions), outputVerts);
	ApplyViewportTransform4(vertexPositions);
}
#endif // #ifdef RUN_SHADERS_IN_WARPS

#ifdef MULTITHREAD_SHADING
static inline slist_item* const GetNewWorkerJob(const bool canImmediateFlushJobs)
{
	slist_item* const ret = &(allWorkItems[workStatus.currentWorkID++]);

	if (workStatus.currentWorkID >= MAX_NUM_JOBS)
	{
		if (!canImmediateFlushJobs)
		{
			MessageBoxA(NULL, "Error: Exhausted job pool. Cannot create more jobs.", "Error", MB_OK);
			__debugbreak();
		}
		else
		{
			SynchronizeThreads();
		}
	}

	return ret;
}

void IDirect3DDevice9Hook::CreateNewVertexShadeJob(VS_2_0_OutputRegisters* const * const outputRegs, const unsigned* const vertexIndices, const workerJobType jobWidth) const
{
#ifdef _DEBUG
	if (jobWidth >= VERTEX_SHADE_JOB_MAX)
	{
		__debugbreak();
	}
#endif
	const unsigned char numVertsPerJob = (1 << (2 * jobWidth) );

	slist_item* const newItem = GetNewWorkerJob(false);
	newItem->jobType = jobWidth;
	slist_item::_jobData::_vertexJobData& vertexJobData = newItem->jobData.vertexJobData;
#ifdef _DEBUG
	for (unsigned char x = 0; x < ARRAYSIZE(vertexJobData.outputRegs); ++x)
		vertexJobData.outputRegs[x] = NULL;
#endif
	for (unsigned char x = 0; x < numVertsPerJob; ++x)
		vertexJobData.outputRegs[x] = outputRegs[x];

#ifdef _DEBUG
	for (unsigned char x = 0; x < ARRAYSIZE(vertexJobData.vertexIndex); ++x)
		vertexJobData.vertexIndex[x] = 0xFFFFFFFF;
#endif
	for (unsigned char x = 0; x < numVertsPerJob; ++x)
		vertexJobData.vertexIndex[x] = vertexIndices[x];

	++workStatus.numJobs;
}

void IDirect3DDevice9Hook::CreateNewPixelShadeJob(const unsigned x, const unsigned y, const int barycentricA, const int barycentricB, const int barycentricC, const primitivePixelJobData* const primitiveData) const
{
	slist_item* const newItem = GetNewWorkerJob(true);
	newItem->jobType = pixelShadeJob;
	slist_item::_jobData::_pixelJobData& pixelJobData = newItem->jobData.pixelJobData;
#ifdef _DEBUG
	if (!primitiveData)
	{
		__debugbreak();
	}
#endif
	pixelJobData.primitiveData = primitiveData;
	pixelJobData.x = x;
	pixelJobData.y = y;
	pixelJobData.barycentricA = barycentricA;
	pixelJobData.barycentricB = barycentricB;
	pixelJobData.barycentricC = barycentricC;

	++workStatus.numJobs;
}
#endif // #ifdef MULTITHREAD_SHADING

const primitivePixelJobData* const IDirect3DDevice9Hook::GetNewPrimitiveJobData(const void* const v0, const void* const v1, const void* const v2, const float barycentricNormalizeFactor, const UINT primitiveID, const bool VFace,
	const UINT vertex0index, const UINT vertex1index, const UINT vertex2index) const
{
#ifdef _DEBUG
	if (primitiveID >= ARRAYSIZE(allPrimitiveJobData) )
	{
		__debugbreak();
	}
#endif

	primitivePixelJobData& newPrimitiveData = allPrimitiveJobData[primitiveID];

	if (currentDrawCallData.pixelData.useShaderVerts)
	{
		primitivePixelJobData::_pixelShadeVertexData::_shadeFromShader& thisShadeFromShader = newPrimitiveData.pixelShadeVertexData.shadeFromShader;
		thisShadeFromShader.v0 = static_cast<const VS_2_0_OutputRegisters* const>(v0);
		thisShadeFromShader.v1 = static_cast<const VS_2_0_OutputRegisters* const>(v1);
		thisShadeFromShader.v2 = static_cast<const VS_2_0_OutputRegisters* const>(v2);
	}
	else
	{
		primitivePixelJobData::_pixelShadeVertexData::_shadeFromStream& thisShadeFromStream = newPrimitiveData.pixelShadeVertexData.shadeFromStream;
		thisShadeFromStream.v0 = static_cast<CONST BYTE* const>(v0);
		thisShadeFromStream.v1 = static_cast<CONST BYTE* const>(v1);
		thisShadeFromStream.v2 = static_cast<CONST BYTE* const>(v2);
	}
	newPrimitiveData.vertex0index = vertex0index;
	newPrimitiveData.vertex1index = vertex1index;
	newPrimitiveData.vertex2index = vertex2index;
	newPrimitiveData.barycentricNormalizeFactor = barycentricNormalizeFactor;
	newPrimitiveData.primitiveID = primitiveID;
	newPrimitiveData.VFace = VFace;

	return &newPrimitiveData;
}

static std::vector<unsigned> alreadyShadedVerts;

struct vertJobCollector
{
	VS_2_0_OutputRegisters* outputRegs;
	UINT vertexIndex;
};

static std::vector<vertJobCollector> vertJobsToShade;

static inline const UINT GetNumVertsUsed(const D3DPRIMITIVETYPE PrimitiveType, const UINT PrimitiveCount)
{
	switch (PrimitiveType)
	{
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Unknown primitive type specified");
#else
		__assume(0);
#endif
		// Intentional fallthrough
	case D3DPT_POINTLIST    :
		return PrimitiveCount;
	case D3DPT_LINELIST     :
		return PrimitiveCount * 2;
	case D3DPT_LINESTRIP    :
		return PrimitiveCount + 1;
	case D3DPT_TRIANGLELIST :
		return PrimitiveCount * 3;
	case D3DPT_TRIANGLESTRIP:
		return PrimitiveCount + 2;
	case D3DPT_TRIANGLEFAN  :
		return PrimitiveCount + 2;
	}
}

template <const bool useVertexBuffer, const bool useIndexBuffer>
void IDirect3DDevice9Hook::ProcessVerticesToBuffer(const IDirect3DVertexDeclaration9Hook* const decl, const DeclarationSemanticMapping& mapping, std::vector<VS_2_0_OutputRegisters>& outputVerts, const BYTE* const indexBuffer, const D3DFORMAT indexFormat, 
	const D3DPRIMITIVETYPE PrimitiveType, const INT BaseVertexIndex, const UINT MinVertexIndex, const UINT startIndex, const UINT primCount, const void* const vertStreamBytes, const unsigned short vertStreamStride) const
{
	const unsigned numOutputVerts = GetNumVertsUsed(PrimitiveType, primCount);
#ifdef _DEBUG
	if (numOutputVerts < 3)
	{
		DbgBreakPrint("Error: Not enough verts processed to make a triangle");
	}
#endif

#ifdef _DEBUG
	outputVerts.clear();
#endif
	outputVerts.resize(numOutputVerts);

	frameStats.numVertsProcessed += numOutputVerts;

	VS_2_0_OutputRegisters* outputBufferPtr = &outputVerts.front();

	vertJobsToShade.clear();

	const bool anyUserClipPlanesEnabled = currentDrawCallData.vertexData.userClipPlanesEnabled;

	if (useIndexBuffer)
	{
		alreadyShadedVerts.clear();

		switch (indexFormat)
		{
		case D3DFMT_INDEX16:
		{
			const unsigned short* const bufferShorts = (const unsigned short* const)indexBuffer;
			for (unsigned x = 0; x < numOutputVerts; ++x)
			{
				// Uhhhh, this isn't correct except for point-lists, line-lists, and triangle-lists:
				const unsigned short index = bufferShorts[x + startIndex] + BaseVertexIndex;

				if (index >= alreadyShadedVerts.size() )
					alreadyShadedVerts.resize(index + 1, 0xFFFFFFFF);

				if (alreadyShadedVerts[index] != 0xFFFFFFFF)
				{
#ifdef MULTITHREAD_SHADING
					// Do fix-ups after the loop, but only shade vertices once
					outputBufferPtr++;
#else
					*outputBufferPtr++ = outputVerts[alreadyShadedVerts[index] ];
#endif
					++frameStats.numVertsReused;
				}
				else
				{
#ifdef MULTITHREAD_SHADING
					vertJobCollector newJob;
					newJob.outputRegs = outputBufferPtr++;
					newJob.vertexIndex = index;
					vertJobsToShade.push_back(newJob);
#else

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
					LARGE_INTEGER vertexShadeStartTime;
					QueryPerformanceCounter(&vertexShadeStartTime);
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

					if (anyUserClipPlanesEnabled)
						ProcessVertexToBuffer<true>(mapping, &deviceMainVShaderEngine, outputBufferPtr++, index);
					else
						ProcessVertexToBuffer<false>(mapping, &deviceMainVShaderEngine, outputBufferPtr++, index);

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
					LARGE_INTEGER vertexShadeEndTime;
					QueryPerformanceCounter(&vertexShadeEndTime);

					totalVertexShadeTicks += (vertexShadeEndTime.QuadPart - vertexShadeStartTime.QuadPart);
					++numVertexShadeTasks;
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

#endif
					alreadyShadedVerts[index] = x;
				}
			}
		}
			break;
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Undefined index buffer format");
#endif
		case D3DFMT_INDEX32:
		{
			const unsigned* const bufferLongs = (const unsigned* const)indexBuffer;
			for (unsigned x = 0; x < numOutputVerts; ++x)
			{
				// Uhhhh, this isn't correct except for point-lists, line-lists, and triangle-lists:
				const unsigned index = bufferLongs[x + startIndex] + BaseVertexIndex;

				if (index >= alreadyShadedVerts.size() )
					alreadyShadedVerts.resize(index + 1, 0xFFFFFFFF);

				if (alreadyShadedVerts[index] != 0xFFFFFFFF)
				{
#ifdef MULTITHREAD_SHADING
					// Do fix-ups after the loop, but only shade vertices once
					outputBufferPtr++;
#else // #ifdef MULTITHREAD_SHADING
					*outputBufferPtr++ = outputVerts[alreadyShadedVerts[index] ];
#endif // #ifdef MULTITHREAD_SHADING
					++frameStats.numVertsReused;
				}
				else
				{
#ifdef MULTITHREAD_SHADING
					vertJobCollector newJob;
					newJob.outputRegs = outputBufferPtr++;
					newJob.vertexIndex = index;
					vertJobsToShade.push_back(newJob);
#else // #ifdef MULTITHREAD_SHADING

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
					LARGE_INTEGER vertexShadeStartTime;
					QueryPerformanceCounter(&vertexShadeStartTime);
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
					if (anyUserClipPlanesEnabled)
						ProcessVertexToBuffer<true>(mapping, &deviceMainVShaderEngine, outputBufferPtr++, index);
					else
						ProcessVertexToBuffer<false>(mapping, &deviceMainVShaderEngine, outputBufferPtr++, index);

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
					LARGE_INTEGER vertexShadeEndTime;
					QueryPerformanceCounter(&vertexShadeEndTime);

					totalVertexShadeTicks += (vertexShadeEndTime.QuadPart - vertexShadeStartTime.QuadPart);
					++numVertexShadeTasks;
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

#endif // #ifdef MULTITHREAD_SHADING
					alreadyShadedVerts[index] = x;
				}
			}
		}
			break;
		}
	}
	else
	{
		for (unsigned x = 0; x < numOutputVerts; ++x)
		{
			// Uhhhh, this isn't correct except for point-lists, line-lists, and triangle-lists:
#ifdef MULTITHREAD_SHADING
			vertJobCollector newJob;
			newJob.outputRegs = outputBufferPtr++;
			newJob.vertexIndex = x + BaseVertexIndex;
			vertJobsToShade.push_back(newJob);
#else

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
					LARGE_INTEGER vertexShadeStartTime;
					QueryPerformanceCounter(&vertexShadeStartTime);
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

			if (anyUserClipPlanesEnabled)
				ProcessVertexToBuffer<true>(mapping, &deviceMainVShaderEngine, outputBufferPtr++, x + BaseVertexIndex);
			else
				ProcessVertexToBuffer<false>(mapping, &deviceMainVShaderEngine, outputBufferPtr++, x + BaseVertexIndex);

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
					LARGE_INTEGER vertexShadeEndTime;
					QueryPerformanceCounter(&vertexShadeEndTime);

					totalVertexShadeTicks += (vertexShadeEndTime.QuadPart - vertexShadeStartTime.QuadPart);
					++numVertexShadeTasks;
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

#endif
		}
	}

	//numWorkItems = numOutputVerts;
	//InterlockedExchange(&numWorkItems, numOutputVerts);
	//PulseEvent(notifyJobsBeginEvent); // TODO: Replace PulseEvent with something safer
	/*SetEvent(notifyJobsBeginEvent);
	ResetEvent(notifyJobsBeginEvent);
	WaitForSingleObject(notifyAllJobsCompletedEvent, INFINITE);
	ResetEvent(notifyAllJobsCompletedEvent);
	if (numWorkItems != 0)
	{
		__debugbreak();
	}*/
#ifdef MULTITHREAD_SHADING
	const unsigned numNewJobs = vertJobsToShade.size();

#ifdef RUN_SHADERS_IN_WARPS
	if (!deviceMainVShaderEngine.GetShaderInfo()->usesDynamicBranching && !deviceMainVShaderEngine.GetShaderInfo()->usesInstructionPredication)
	{
		for (unsigned x = 0; x < numNewJobs;)
		{
			const unsigned jobsRemaining = numNewJobs - x;
			/*if (jobsRemaining >= 64)
			{
				UINT vertexIndices[64];
				const vertJobCollector& thisNewJob = vertJobsToShade[x];
				for (unsigned char y = 0; y < 64; ++y)
					vertexIndices[y] = (&thisNewJob)[y].vertexIndex;
				CreateNewVertexShadeJob(thisNewJob.outputRegs, vertexIndices, vertexShade64Job);
				x += 64;
			}
			else if (jobsRemaining >= 16)
			{
				UINT vertexIndices[16];
				const vertJobCollector& thisNewJob = vertJobsToShade[x];
				for (unsigned char y = 0; y < 16; ++y)
					vertexIndices[y] = (&thisNewJob)[y].vertexIndex;
				CreateNewVertexShadeJob(thisNewJob.outputRegs, vertexIndices, vertexShade16Job);
				x += 16;
			}
			else */if (jobsRemaining >= 4)
			{
				UINT vertexIndices[4];
				VS_2_0_OutputRegisters* outputRegs[4];
				const vertJobCollector& thisNewJob = vertJobsToShade[x];
				for (unsigned char y = 0; y < 4; ++y)
				{
					vertexIndices[y] = (&thisNewJob)[y].vertexIndex;
					outputRegs[y] = (&thisNewJob)[y].outputRegs;
				}
				CreateNewVertexShadeJob(outputRegs, vertexIndices, vertexShade4Job);
				x += 4;
			}
			else
			{
				const vertJobCollector& thisNewJob = vertJobsToShade[x];
				CreateNewVertexShadeJob(&thisNewJob.outputRegs, &thisNewJob.vertexIndex, vertexShade1Job);
				++x;
			}
		}
	}
	else // We don't currently support branching in our warp-jobs
#endif // #ifdef RUN_SHADERS_IN_WARPS
	{
		for (unsigned x = 0; x < numNewJobs; ++x)
		{
			const vertJobCollector& thisNewJob = vertJobsToShade[x];
			CreateNewVertexShadeJob(&thisNewJob.outputRegs, &thisNewJob.vertexIndex, vertexShade1Job);
		}
	}
	//CloseThreadpoolCleanupGroupMembers(cleanup, FALSE, NULL);
	//RefreshThreadpoolWork();
	SynchronizeThreads();

	// This is the "fixup phase" that's necessary to populate all of the vertex copies through here
	// This avoids shading re-used vertices more than once and saves *a ton* of processing power! :)
	if (useIndexBuffer)
	{
		switch (indexFormat)
		{
		case D3DFMT_INDEX16:
		{
			const unsigned short* const bufferShorts = (const unsigned short* const)indexBuffer;
			for (unsigned x = 0; x < numOutputVerts; ++x)
			{
				// Uhhhh, this isn't correct except for point-lists, line-lists, and triangle-lists:
				const unsigned short index = bufferShorts[x + startIndex] + BaseVertexIndex;

				if (alreadyShadedVerts[index] != x)
				{
					outputVerts[x] = outputVerts[alreadyShadedVerts[index] ];
				}
			}
		}
			break;
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Undefined index buffer format");
#endif
		case D3DFMT_INDEX32:
		{
			const unsigned* const bufferLongs = (const unsigned* const)indexBuffer;
			for (unsigned x = 0; x < numOutputVerts; ++x)
			{
				// Uhhhh, this isn't correct except for point-lists, line-lists, and triangle-lists:
				const unsigned index = bufferLongs[x + startIndex] + BaseVertexIndex;

				if (alreadyShadedVerts[index] != x)
				{
					outputVerts[x] = outputVerts[alreadyShadedVerts[index] ];
				}
			}
		}
			break;
		}
	}
#endif

#ifdef DEBUG_VERTEX_OUT_POSITIONS
	for (unsigned x = 0; x < outputVerts.size(); ++x)
	{
		const D3DXVECTOR4& finalVertPos = currentState.currentVertexShader->GetPosition(outputVerts[x]);
		char buffer[256] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
		sprintf(buffer, "Vert %u final pos: (%f, %f, %f)\n", x, finalVertPos.x, finalVertPos.y, finalVertPos.z);
#pragma warning(pop)
		OutputDebugString(buffer);
	}
#endif
}

template <const bool useVertexBuffer, const bool useIndexBuffer>
void IDirect3DDevice9Hook::ProcessVerticesToBuffer(const IDirect3DVertexDeclaration9Hook* const decl, const DeclarationSemanticMapping& mapping, std::vector<VS_2_0_OutputRegisters>& outputVerts, const IDirect3DIndexBuffer9Hook* const indexBuffer, 
	const D3DPRIMITIVETYPE PrimitiveType, const INT BaseVertexIndex, const UINT MinVertexIndex, const UINT startIndex, const UINT primCount, const void* const vertStreamBytes, const unsigned short vertStreamStride) const
{
	if (useIndexBuffer)
	{
		ProcessVerticesToBuffer<useVertexBuffer, useIndexBuffer>(decl, mapping, outputVerts, indexBuffer->GetBufferBytes(), indexBuffer->GetFormat(), PrimitiveType, BaseVertexIndex, MinVertexIndex, startIndex, primCount, vertStreamBytes, vertStreamStride);
	}
	else
	{
		ProcessVerticesToBuffer<useVertexBuffer, useIndexBuffer>(decl, mapping, outputVerts, NULL, D3DFMT_UNKNOWN, PrimitiveType, BaseVertexIndex, MinVertexIndex, startIndex, primCount, vertStreamBytes, vertStreamStride);
	}
}

void IDirect3DDevice9Hook::InitVertexShader(const DeviceState& deviceState, const ShaderInfo& vertexShaderInfo) const
{
	deviceMainVShaderEngine.Init(deviceState, vertexShaderInfo, &vsDrawCallCB);

#ifdef MULTITHREAD_SHADING
	for (unsigned x = 0; x < ARRAYSIZE(threadItem); ++x)
	{
		threadItem[x].threadVS_2_0 = deviceMainVShaderEngine;
	}
#endif
}

void IDirect3DDevice9Hook::InitPixelShader(const DeviceState& deviceState, const ShaderInfo& pixelShaderInfo) const
{
	deviceMainPShaderEngine.Init(deviceState, pixelShaderInfo, &psDrawCallCB);

#ifdef MULTITHREAD_SHADING
	for (unsigned x = 0; x < ARRAYSIZE(threadItem); ++x)
	{
		threadItem[x].threadPS_2_0 = deviceMainPShaderEngine;
	}
#endif
}

COM_DECLSPEC_NOTHROW void IDirect3DDevice9Hook::SetupCurrentDrawCallVertexData(const DeclarationSemanticMapping& mapping)
{
	currentDrawCallData.vertexData.userClipPlanesEnabled = currentState.currentRenderStates.renderStatesUnion.namedStates.clipPlaneEnable > 0;
	currentDrawCallData.vertexData.mapping = &mapping;
}

COM_DECLSPEC_NOTHROW void IDirect3DDevice9Hook::SetupCurrentDrawCallPixelData(const bool useShaderVerts, const void* const vs_to_ps_mapping) const
{
	currentDrawCallData.pixelData.useShaderVerts = useShaderVerts;
	if (useShaderVerts)
	{
		currentDrawCallData.pixelData.offsetIntoVertexForOPosition_Bytes = currentState.currentVertexShader->GetOPosOffset_Bytes();
		currentDrawCallData.pixelData.vs_to_ps_mappings.vs_psMapping = static_cast<const VStoPSMapping* const>(vs_to_ps_mapping);
	}
	else
	{
		currentDrawCallData.pixelData.offsetIntoVertexForOPosition_Bytes = 0;// currentState.currentVertexDecl->GetStream0Float4PositionTOffset();
		currentDrawCallData.pixelData.vs_to_ps_mappings.vertexDeclMapping = static_cast<const DeclarationSemanticMapping* const>(vs_to_ps_mapping);
	}
}

// Counts the number of 32-bit DWORD's for each of the D3DDECLTYPE's
static unsigned char typeSize_DWORDs[MAXD3DDECLTYPE] =
{
	1,// D3DDECLTYPE_FLOAT1    =  0,  // 1D float expanded to (value, 0., 0., 1.)
    2,// D3DDECLTYPE_FLOAT2    =  1,  // 2D float expanded to (value, value, 0., 1.)
    3,// D3DDECLTYPE_FLOAT3    =  2,  // 3D float expanded to (value, value, value, 1.)
    4,// D3DDECLTYPE_FLOAT4    =  3,  // 4D float
    1,// D3DDECLTYPE_D3DCOLOR  =  4,  // 4D packed unsigned bytes mapped to 0. to 1. range
									// Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
    1,// D3DDECLTYPE_UBYTE4    =  5,  // 4D unsigned byte
    1,// D3DDECLTYPE_SHORT2    =  6,  // 2D signed short expanded to (value, value, 0., 1.)
    2,// D3DDECLTYPE_SHORT4    =  7,  // 4D signed short

	// The following types are valid only with vertex shaders >= 2.0
    1,// D3DDECLTYPE_UBYTE4N   =  8,  // Each of 4 bytes is normalized by dividing to 255.0
    1,// D3DDECLTYPE_SHORT2N   =  9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
    2,// D3DDECLTYPE_SHORT4N   = 10,  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
    1,// D3DDECLTYPE_USHORT2N  = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
    2,// D3DDECLTYPE_USHORT4N  = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
    1,// D3DDECLTYPE_UDEC3     = 13,  // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
    1,// D3DDECLTYPE_DEC3N     = 14,  // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
    1,// D3DDECLTYPE_FLOAT16_2 = 15,  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
    2 // D3DDECLTYPE_FLOAT16_4 = 16,  // Four 16-bit floating point values
};

static inline void ComputeCachedStreamEnd(StreamDataTypeEndPointers& thisStreamEnd, const BYTE* const streamBegin, const unsigned streamLenBytes)
{
	thisStreamEnd.streamEndAbsolute = streamBegin + streamLenBytes;

	for (unsigned x = 0; x < ARRAYSIZE(thisStreamEnd.dataTypeStreamEnds); ++x)
	{
		const D3DDECLTYPE thisType = (const D3DDECLTYPE)x;
		if (streamBegin)
			thisStreamEnd.dataTypeStreamEnds[thisType] = thisStreamEnd.streamEndAbsolute - (typeSize_DWORDs[thisType] * sizeof(const DWORD) );
		else
			thisStreamEnd.dataTypeStreamEnds[thisType] = NULL;
	}
}

void IDirect3DDevice9Hook::RecomputeCachedStreamEndsIfDirty()
{
	if (!currentState.currentVertexDecl)
	{
		__debugbreak(); // Should never be calling this function if we don't have a vertex decl
	}

	const std::vector<DebuggableD3DVERTEXELEMENT9>& elements = currentState.currentVertexDecl->GetElementsInternal();
	const unsigned numElements = elements.size() - 1; // Minus one here because we want to ignore the D3DDECL_END element
	for (unsigned x = 0; x < numElements; ++x)
	{
		const DebuggableD3DVERTEXELEMENT9& thisElement = elements[x];
		StreamDataTypeEndPointers& thisStreamEnd = currentState.currentStreamEnds[thisElement.Stream];

		// Only recompute for dirty streams:
		if (thisStreamEnd.dirty)
		{
			const IDirect3DVertexBuffer9Hook* const currentStreamVB = currentState.currentStreams[thisElement.Stream].vertexBuffer;
			if (currentStreamVB)
				ComputeCachedStreamEnd(thisStreamEnd, currentStreamVB->GetInternalDataBuffer(), currentStreamVB->GetInternalLength_Bytes() );
			else
				ComputeCachedStreamEnd(thisStreamEnd, NULL, 0);

			// Clear the dirty flag when we have computed all of the pointers
			thisStreamEnd.dirty = false;
		}
	}
}

void IDirect3DDevice9Hook::RecomputeCachedStreamEndsForUP(const BYTE* const stream0Data, const unsigned numVertices, const USHORT vertexStride)
{
	if (!currentState.currentVertexDecl)
	{
		__debugbreak(); // Should never be calling this function if we don't have a vertex decl
	}

	ComputeCachedStreamEnd(currentState.currentStreamEnds[0], stream0Data, numVertices * vertexStride);
}

// Returns true for "should draw", or false for "should skip"
const bool IDirect3DDevice9Hook::TotalDrawCallSkipTest(void) const
{
#ifdef ENABLE_END_TO_SKIP_DRAWS
	if ( (GetAsyncKeyState(VK_END) & 0x8000) )
	{
		// Skip this draw call if END is held down
		return false;
	}
#endif // #ifdef ENABLE_END_TO_SKIP_DRAWS

	bool DepthWriteEnabled = false;
	if (currentState.currentDepthStencil != NULL)
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zWriteEnable)
			DepthWriteEnabled = true;

	if (DepthWriteEnabled)
	{
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zFunc == D3DCMP_NEVER)
			return false; // TODO: Check for stencil enable and stencil zFail here
	}

	if (currentState.currentRenderStates.renderStatesUnion.namedStates.colorWriteEnable == 0x00)
		return false;

	if (!DepthWriteEnabled)
	{
		if (currentState.currentPixelShader != NULL)
		{
			const ShaderInfo& pixelShaderInfo = currentState.currentPixelShader->GetShaderInfo();
			bool allUnbound = true;
			for (unsigned x = 0; x < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++x)
			{
				if (pixelShaderInfo.usedMRTMask & (1 << x) )
				{
					if (currentState.currentRenderTargets[x])
					{
						allUnbound = false;
						break;
					}
				}
			}
			if (allUnbound)
				return false;
		}
		else
		{
			if (!currentState.currentRenderTargets[0])
				return false;
		}
	}

	if (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaTestEnable)
	{
		switch (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaFunc)
		{
		case D3DCMP_NEVER:
			return false;
		case D3DCMP_ALWAYS:
		case D3DCMP_EQUAL:
		case D3DCMP_NOTEQUAL:
		case D3DCMP_LESSEQUAL:
		case D3DCMP_GREATEREQUAL:
			break;
		case D3DCMP_LESS:
			if (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaRef == 0x00)
				return false;
			break;
		case D3DCMP_GREATER:
			if (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaRef == 0xFF)
				return false;
			break;
		}
	}

	if (currentState.currentRenderStates.renderStatesUnion.namedStates.scissorTestEnable)
	{
		const RECT& thisScissorRect = currentState.currentScissorRect.scissorRect;
		if (thisScissorRect.right <= thisScissorRect.left)
			return false;
		if (thisScissorRect.bottom <= thisScissorRect.top)
			return false;
	}

	return true;
}

// Returns true if the currently set pipeline can do early-Z testing, or false if it cannot (false if depth isn't enabled, or no depth buffer is bound, or a weird Z-test is set up, or the pixel shader outputs depth)
const bool IDirect3DDevice9Hook::CurrentPipelineCanEarlyZTest(void) const
{
#ifndef DISALLOW_EARLY_Z_TESTING
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.zEnable != D3DZB_FALSE)
	{
		// Currently only less and lessequal are supported. In the future, greater and greaterequal can also be supported
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zFunc == D3DCMP_LESS || 
			currentState.currentRenderStates.renderStatesUnion.namedStates.zFunc == D3DCMP_LESSEQUAL)
		{
			if (currentState.currentDepthStencil != NULL)
			{
				if (currentState.currentPixelShader == NULL)
					return true;
				else
					return !currentState.currentPixelShader->GetShaderInfo().psWritesDepth;
			}
		}
	}
#endif // #ifndef DISALLOW_EARLY_Z_TESTING
	return false;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DrawPrimitive(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
	if (!currentState.currentVertexDecl)
		return D3DERR_INVALIDCALL;

	if (PrimitiveCount == 0)
		return D3DERR_INVALIDCALL;

	if (PrimitiveType > D3DPT_TRIANGLEFAN || PrimitiveType < D3DPT_POINTLIST)
		return D3DERR_INVALIDCALL;

	HRESULT ret = S_OK;

	if (!TotalDrawCallSkipTest() )
		return ret;

	bool usePassthroughVertexShader = false;
	bool usePassthroughPixelShader = false;

	if (!currentState.currentPixelShader)
	{
		usePassthroughPixelShader = true;

		IDirect3DPixelShader9Hook* FixedFunctionPixelShader = NULL;
		FixedFunctionStateToPixelShader(currentState, &FixedFunctionPixelShader, this);
		SetPixelShader(FixedFunctionPixelShader);
		SetFixedFunctionPixelShaderState(currentState, this);
	}

	RecomputeCachedStreamEndsIfDirty();

	// This is the case if we have a D3DUSAGE_POSITIONT with usageindex 0
	if (SkipVertexProcessing() )
	{
		if (CurrentPipelineCanEarlyZTest() )
			DrawPrimitiveUBPretransformedSkipVS<false, unsigned, true>(PrimitiveType, 0, 0, PrimitiveCount);
		else
			DrawPrimitiveUBPretransformedSkipVS<false, unsigned, false>(PrimitiveType, 0, 0, PrimitiveCount);

		if (usePassthroughPixelShader)
			SetPixelShader(NULL);
		return ret;
	}

	if (!currentState.currentVertexShader)
	{
		usePassthroughVertexShader = true;

		IDirect3DVertexShader9Hook* FixedFunctionVertexShader = NULL;
		FixedFunctionStateToVertexShader(currentState, &FixedFunctionVertexShader, this);
		SetVertexShader(FixedFunctionVertexShader);
		SetFixedFunctionVertexShaderState(currentState, this);
	}

	processedVertexBuffer->clear();
	if (!currentState.currentVertexDecl)
	{
		DbgBreakPrint("Error: NULL vertex decl");
	}
	DeclarationSemanticMapping vertexDeclMapping;
	vertexDeclMapping.ClearSemanticMapping();
	vertexDeclMapping.ComputeMappingVS(currentState.currentVertexDecl, currentState.currentVertexShader);
	InitVertexShader(currentState, currentState.currentVertexShader->GetShaderInfo() );

	SetupCurrentDrawCallVertexData(vertexDeclMapping);

#ifdef _DEBUG
	if (currentState.currentVertexShader && !currentState.currentVertexShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached vertex shader detected");
	}
#endif

	ProcessVerticesToBuffer<true, false>(currentState.currentVertexDecl, vertexDeclMapping, *processedVertexBuffer, NULL, PrimitiveType, 0, 0, StartVertex, PrimitiveCount, NULL, 0);

#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}
#endif

	if (CurrentPipelineCanEarlyZTest() )
		DrawPrimitiveUB<true>(PrimitiveType, PrimitiveCount, *processedVertexBuffer);
	else
		DrawPrimitiveUB<false>(PrimitiveType, PrimitiveCount, *processedVertexBuffer);

	if (usePassthroughVertexShader)
		SetVertexShader(NULL);
	if (usePassthroughPixelShader)
		SetPixelShader(NULL);

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DrawIndexedPrimitive(THIS_ D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	if (!currentState.currentVertexDecl)
		return D3DERR_INVALIDCALL;

	if (NumVertices == 0)
		return D3DERR_INVALIDCALL;

	if (primCount == 0)
		return D3DERR_INVALIDCALL;

	if (PrimitiveType > D3DPT_TRIANGLEFAN || PrimitiveType < D3DPT_POINTLIST)
		return D3DERR_INVALIDCALL;

	if (currentState.currentTextures[0])
	{
#ifdef WITH_SURFACE_HASHING
		const surfaceHash& currentHash = currentState.currentTextures[0]->GetUnderlyingSurfaces()[0]->GetSurfaceHash();
		if (currentHash.format == D3DFMT_DXT5 && currentHash.sizeBytes == 0x4000 && currentHash.hashVal == 0xD957D29D)
		{
			//__debugbreak();
		}
#endif
	}

	if (currentState.currentIndexBuffer == NULL)
		return D3DERR_INVALIDCALL;

	if (!TotalDrawCallSkipTest() )
		return S_OK;

	RecomputeCachedStreamEndsIfDirty();

	bool usePassthroughVertexShader = false;
	bool usePassthroughPixelShader = false;

	if (!currentState.currentPixelShader)
	{
		usePassthroughPixelShader = true;

		IDirect3DPixelShader9Hook* FixedFunctionPixelShader = NULL;
		FixedFunctionStateToPixelShader(currentState, &FixedFunctionPixelShader, this);
		SetPixelShader(FixedFunctionPixelShader);
		SetFixedFunctionPixelShaderState(currentState, this);
	}

	// This is the case if we have a D3DUSAGE_POSITIONT with usageindex 0
	if (SkipVertexProcessing() )
	{
		switch (currentState.currentIndexBuffer->GetFormat() )
		{
		case D3DFMT_INDEX16:
			if (CurrentPipelineCanEarlyZTest() )
				DrawPrimitiveUBPretransformedSkipVS<true, unsigned short, true>(PrimitiveType, BaseVertexIndex, startIndex, primCount);
			else
				DrawPrimitiveUBPretransformedSkipVS<true, unsigned short, false>(PrimitiveType, BaseVertexIndex, startIndex, primCount);
			break;
		default:
#ifdef _DEBUG
		{
			__debugbreak(); // Should never be here
		}
#endif
		case D3DFMT_INDEX32:
			if (CurrentPipelineCanEarlyZTest() )
				DrawPrimitiveUBPretransformedSkipVS<true, unsigned, true>(PrimitiveType, BaseVertexIndex, startIndex, primCount);
			else
				DrawPrimitiveUBPretransformedSkipVS<true, unsigned, false>(PrimitiveType, BaseVertexIndex, startIndex, primCount);
			break;
		}

		if (usePassthroughPixelShader)
			SetPixelShader(NULL);
		return S_OK;
	}

	if (!currentState.currentVertexShader)
	{
		usePassthroughVertexShader = true;

		IDirect3DVertexShader9Hook* FixedFunctionVertexShader = NULL;
		FixedFunctionStateToVertexShader(currentState, &FixedFunctionVertexShader, this);
		SetVertexShader(FixedFunctionVertexShader);
		SetFixedFunctionVertexShaderState(currentState, this);
	}

	HRESULT ret = S_OK;

	processedVertexBuffer->clear();

	if (!currentState.currentVertexDecl)
	{
		DbgBreakPrint("Error: NULL vertex decl");
	}
	DeclarationSemanticMapping vertexDeclMapping;
	vertexDeclMapping.ClearSemanticMapping();
	vertexDeclMapping.ComputeMappingVS(currentState.currentVertexDecl, currentState.currentVertexShader);
	InitVertexShader(currentState, currentState.currentVertexShader->GetShaderInfo() );

	SetupCurrentDrawCallVertexData(vertexDeclMapping);

#ifdef _DEBUG
	if (currentState.currentVertexShader && !currentState.currentVertexShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached vertex shader detected");
	}
#endif

	ProcessVerticesToBuffer<true, true>(currentState.currentVertexDecl, vertexDeclMapping, *processedVertexBuffer, currentState.currentIndexBuffer, PrimitiveType, BaseVertexIndex, MinVertexIndex, startIndex, primCount, NULL, 0);

#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}
#endif

	if (CurrentPipelineCanEarlyZTest() )
		DrawPrimitiveUB<true>(PrimitiveType, primCount, *processedVertexBuffer);
	else
		DrawPrimitiveUB<false>(PrimitiveType, primCount, *processedVertexBuffer);

	if (usePassthroughVertexShader)
		SetVertexShader(NULL);
	if (usePassthroughPixelShader)
		SetPixelShader(NULL);

	return ret;
}

static inline const int computeEdgeSidedness(const int ax, const int ay, const int bx, const int by, const int cx, const int cy)
{
	return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

template <const unsigned char writeMask>
void IDirect3DDevice9Hook::LoadSrcBlend(D3DXVECTOR4& srcBlend, const D3DBLEND blendMode, const D3DXVECTOR4& srcColor, const D3DXVECTOR4& dstColor) const
{
	switch (blendMode)
	{
	case D3DBLEND_ZERO           :
		if (writeMask & 0x1)
			srcBlend.x = 0.0f;
		if (writeMask & 0x2)
			srcBlend.y = 0.0f;
		if (writeMask & 0x4)
			srcBlend.z = 0.0f;
		if (writeMask & 0x8)
			srcBlend.w = 0.0f;
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid D3DBLEND type passed to blending unit");
#else
		__assume(0);
#endif
	case D3DBLEND_ONE            :
		if (writeMask & 0x1)
			srcBlend.x = 1.0f;
		if (writeMask & 0x2)
			srcBlend.y = 1.0f;
		if (writeMask & 0x4)
			srcBlend.z = 1.0f;
		if (writeMask & 0x8)
			srcBlend.w = 1.0f;
		break;
	case D3DBLEND_INVSRCCOLOR2   :
#ifdef _DEBUG
		DbgBreakPrint("Error: D3DBLEND_INVSRCCOLOR2 is not yet supported!"); // Not yet supported!
#else
		__assume(0);
#endif
	case D3DBLEND_SRCCOLOR2      :
#ifdef _DEBUG
		DbgBreakPrint("Error: D3DBLEND_SRCCOLOR2 is not yet supported!"); // Not yet supported!
#else
		__assume(0);
#endif
	case D3DBLEND_SRCCOLOR       :
		if (writeMask & 0x1)
			srcBlend.x = srcColor.x;
		if (writeMask & 0x2)
			srcBlend.y = srcColor.y;
		if (writeMask & 0x4)
			srcBlend.z = srcColor.z;
		if (writeMask & 0x8)
			srcBlend.w = srcColor.w;
		break;
	case D3DBLEND_INVSRCCOLOR    :
		if (writeMask & 0x1)
			srcBlend.x = 1.0f - srcColor.x;
		if (writeMask & 0x2)
			srcBlend.y = 1.0f - srcColor.y;
		if (writeMask & 0x4)
			srcBlend.z = 1.0f - srcColor.z;
		if (writeMask & 0x8)
			srcBlend.w = 1.0f - srcColor.w;
		break;
	case D3DBLEND_BOTHSRCALPHA   :
	case D3DBLEND_SRCALPHA       :
		if (writeMask & 0x1)
			srcBlend.x = srcColor.w;
		if (writeMask & 0x2)
			srcBlend.y = srcColor.w;
		if (writeMask & 0x4)
			srcBlend.z = srcColor.w;
		if (writeMask & 0x8)
			srcBlend.w = srcColor.w;
		break;
	case D3DBLEND_BOTHINVSRCALPHA:
	case D3DBLEND_INVSRCALPHA    :
		if (writeMask & 0x1)
			srcBlend.x = 1.0f - srcColor.w;
		if (writeMask & 0x2)
			srcBlend.y = 1.0f - srcColor.w;
		if (writeMask & 0x4)
			srcBlend.z = 1.0f - srcColor.w;
		if (writeMask & 0x8)
			srcBlend.w = 1.0f - srcColor.w;
		break;
	case D3DBLEND_DESTALPHA      :
		if (writeMask & 0x1)
			srcBlend.x = dstColor.w;
		if (writeMask & 0x2)
			srcBlend.y = dstColor.w;
		if (writeMask & 0x4)
			srcBlend.z = dstColor.w;
		if (writeMask & 0x8)
			srcBlend.w = dstColor.w;
		break;
	case D3DBLEND_INVDESTALPHA   :
		if (writeMask & 0x1)
			srcBlend.x = 1.0f - dstColor.w;
		if (writeMask & 0x2)
			srcBlend.y = 1.0f - dstColor.w;
		if (writeMask & 0x4)
			srcBlend.z = 1.0f - dstColor.w;
		if (writeMask & 0x8)
			srcBlend.w = 1.0f - dstColor.w;
		break;
	case D3DBLEND_DESTCOLOR      :
		if (writeMask & 0x1)
			srcBlend.x = dstColor.x;
		if (writeMask & 0x2)
			srcBlend.y = dstColor.y;
		if (writeMask & 0x4)
			srcBlend.z = dstColor.z;
		if (writeMask & 0x8)
			srcBlend.w = dstColor.w;
		break;
	case D3DBLEND_INVDESTCOLOR   :
		if (writeMask & 0x1)
			srcBlend.x = 1.0f - dstColor.x;
		if (writeMask & 0x2)
			srcBlend.y = 1.0f - dstColor.y;
		if (writeMask & 0x4)
			srcBlend.z = 1.0f - dstColor.z;
		if (writeMask & 0x8)
			srcBlend.w = 1.0f - dstColor.w;
		break;
	case D3DBLEND_SRCALPHASAT    :
	{
		const float as = srcColor.w;
		const float invad = 1.0f - dstColor.w;
		const float f = as < invad ? as : invad;
		if (writeMask & 0x1)
			srcBlend.x = f;
		if (writeMask & 0x2)
			srcBlend.y = f;
		if (writeMask & 0x4)
			srcBlend.z = f;
		if (writeMask & 0x8)
			srcBlend.w = 1.0f;
	}
		break;
	case D3DBLEND_BLENDFACTOR    :
		if (writeMask & 0x1)
			srcBlend.x = currentState.currentRenderStates.cachedBlendFactor.x;
		if (writeMask & 0x2)
			srcBlend.y = currentState.currentRenderStates.cachedBlendFactor.y;
		if (writeMask & 0x4)
			srcBlend.z = currentState.currentRenderStates.cachedBlendFactor.z;
		if (writeMask & 0x8)
			srcBlend.w = currentState.currentRenderStates.cachedBlendFactor.w;
		break;
	case D3DBLEND_INVBLENDFACTOR :
		if (writeMask & 0x1)
			srcBlend.x = currentState.currentRenderStates.cachedInvBlendFactor.x;
		if (writeMask & 0x2)
			srcBlend.y = currentState.currentRenderStates.cachedInvBlendFactor.y;
		if (writeMask & 0x4)
			srcBlend.z = currentState.currentRenderStates.cachedInvBlendFactor.z;
		if (writeMask & 0x8)
			srcBlend.w = currentState.currentRenderStates.cachedInvBlendFactor.w;
		break;
	}
}

template <const unsigned char writeMask>
void IDirect3DDevice9Hook::LoadDstBlend(D3DXVECTOR4& dstBlend, const D3DBLEND blendMode, const D3DXVECTOR4& srcColor, const D3DXVECTOR4& dstColor) const
{
	switch (blendMode)
	{
	case D3DBLEND_ZERO           :
		if (writeMask & 0x1)
			dstBlend.x = 0.0f;
		if (writeMask & 0x2)
			dstBlend.y = 0.0f;
		if (writeMask & 0x4)
			dstBlend.z = 0.0f;
		if (writeMask & 0x8)
			dstBlend.w = 0.0f;
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid D3DBLEND passed to blending unit");
#else
		__assume(0);
#endif
	case D3DBLEND_ONE            :
		if (writeMask & 0x1)
			dstBlend.x = 1.0f;
		if (writeMask & 0x2)
			dstBlend.y = 1.0f;
		if (writeMask & 0x4)
			dstBlend.z = 1.0f;
		if (writeMask & 0x8)
			dstBlend.w = 1.0f;
		break;
	case D3DBLEND_SRCCOLOR       :
		if (writeMask & 0x1)
			dstBlend.x = srcColor.x;
		if (writeMask & 0x2)
			dstBlend.y = srcColor.y;
		if (writeMask & 0x4)
			dstBlend.z = srcColor.z;
		if (writeMask & 0x8)
			dstBlend.w = srcColor.w;
		break;
	case D3DBLEND_INVSRCCOLOR    :
		if (writeMask & 0x1)
			dstBlend.x = 1.0f - srcColor.x;
		if (writeMask & 0x2)
			dstBlend.y = 1.0f - srcColor.y;
		if (writeMask & 0x4)
			dstBlend.z = 1.0f - srcColor.z;
		if (writeMask & 0x8)
			dstBlend.w = 1.0f - srcColor.w;
		break;
	case D3DBLEND_BOTHINVSRCALPHA:
	case D3DBLEND_SRCALPHA       :
		if (writeMask & 0x1)
			dstBlend.x = srcColor.w;
		if (writeMask & 0x2)
			dstBlend.y = srcColor.w;
		if (writeMask & 0x4)
			dstBlend.z = srcColor.w;
		if (writeMask & 0x8)
			dstBlend.w = srcColor.w;
		break;
	case D3DBLEND_BOTHSRCALPHA   :
	case D3DBLEND_INVSRCALPHA    :
		if (writeMask & 0x1)
			dstBlend.x = 1.0f - srcColor.w;
		if (writeMask & 0x2)
			dstBlend.y = 1.0f - srcColor.w;
		if (writeMask & 0x4)
			dstBlend.z = 1.0f - srcColor.w;
		if (writeMask & 0x8)
			dstBlend.w = 1.0f - srcColor.w;
		break;
	case D3DBLEND_DESTALPHA      :
		if (writeMask & 0x1)
			dstBlend.x = dstColor.w;
		if (writeMask & 0x2)
			dstBlend.y = dstColor.w;
		if (writeMask & 0x4)
			dstBlend.z = dstColor.w;
		if (writeMask & 0x8)
			dstBlend.w = dstColor.w;
		break;
	case D3DBLEND_INVDESTALPHA   :
		if (writeMask & 0x1)
			dstBlend.x = 1.0f - dstColor.w;
		if (writeMask & 0x2)
			dstBlend.y = 1.0f - dstColor.w;
		if (writeMask & 0x4)
			dstBlend.z = 1.0f - dstColor.w;
		if (writeMask & 0x8)
			dstBlend.w = 1.0f - dstColor.w;
		break;
	case D3DBLEND_INVSRCCOLOR2   :
#ifdef _DEBUG
		DbgBreakPrint("Error: D3DBLEND_INVSRCCOLOR2 is not yet supported!"); // Not yet supported!
#else
		__assume(0);
#endif
	case D3DBLEND_SRCCOLOR2      :
#ifdef _DEBUG
		DbgBreakPrint("Error: D3DBLEND_SRCCOLOR2 is not yet supported!"); // Not yet supported!
#else
		__assume(0);
#endif
	case D3DBLEND_DESTCOLOR      :
		if (writeMask & 0x1)
			dstBlend.x = dstColor.x;
		if (writeMask & 0x2)
			dstBlend.y = dstColor.y;
		if (writeMask & 0x4)
			dstBlend.z = dstColor.z;
		if (writeMask & 0x8)
			dstBlend.w = dstColor.w;
		break;
	case D3DBLEND_INVDESTCOLOR   :
		if (writeMask & 0x1)
			dstBlend.x = 1.0f - dstColor.x;
		if (writeMask & 0x2)
			dstBlend.y = 1.0f - dstColor.y;
		if (writeMask & 0x4)
			dstBlend.z = 1.0f - dstColor.z;
		if (writeMask & 0x8)
			dstBlend.w = 1.0f - dstColor.w;
		break;
	case D3DBLEND_SRCALPHASAT    :
	{
		const float as = srcColor.w;
		const float invad = 1.0f - dstColor.w;
		const float f = as < invad ? as : invad;
		if (writeMask & 0x1)
			dstBlend.x = f;
		if (writeMask & 0x2)
			dstBlend.y = f;
		if (writeMask & 0x4)
			dstBlend.z = f;
		if (writeMask & 0x8)
			dstBlend.w = 1.0f;
	}
		break;
	case D3DBLEND_BLENDFACTOR    :
		if (writeMask & 0x1)
			dstBlend.x = currentState.currentRenderStates.cachedBlendFactor.x;
		if (writeMask & 0x2)
			dstBlend.y = currentState.currentRenderStates.cachedBlendFactor.y;
		if (writeMask & 0x4)
			dstBlend.z = currentState.currentRenderStates.cachedBlendFactor.z;
		if (writeMask & 0x8)
			dstBlend.w = currentState.currentRenderStates.cachedBlendFactor.w;
		break;
	case D3DBLEND_INVBLENDFACTOR :
		if (writeMask & 0x1)
			dstBlend.x = currentState.currentRenderStates.cachedInvBlendFactor.x;
		if (writeMask & 0x2)
			dstBlend.y = currentState.currentRenderStates.cachedInvBlendFactor.y;
		if (writeMask & 0x4)
			dstBlend.z = currentState.currentRenderStates.cachedInvBlendFactor.z;
		if (writeMask & 0x8)
			dstBlend.w = currentState.currentRenderStates.cachedInvBlendFactor.w;
		break;
	}
}

template <const unsigned char writeMask>
void IDirect3DDevice9Hook::AlphaBlend(D3DXVECTOR4& outVec, const D3DBLENDOP blendOp, const D3DXVECTOR4& srcBlend, const D3DXVECTOR4& dstBlend, const D3DXVECTOR4& srcColor, const D3DXVECTOR4& dstColor) const
{
	float4 combinedSrc, combinedDst;
	if (writeMask & 0x1)
	{
		mul(combinedSrc.x, srcBlend.x, srcColor.x);
		mul(combinedDst.x, dstBlend.x, dstColor.x);
	}
	if (writeMask & 0x2)
	{
		mul(combinedSrc.y, srcBlend.y, srcColor.y);
		mul(combinedDst.y, dstBlend.y, dstColor.y);
	}
	if (writeMask & 0x4)
	{
		mul(combinedSrc.z, srcBlend.z, srcColor.z);
		mul(combinedDst.z, dstBlend.z, dstColor.z);
	}
	if (writeMask & 0x8)
	{
		mul(combinedSrc.w, srcBlend.w, srcColor.w);
		mul(combinedDst.w, dstBlend.w, dstColor.w);
	}

	switch (blendOp)
	{
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid D3DBLENDOP passed to alpha blending unit");
#else
		__assume(0);
#endif
	case D3DBLENDOP_ADD        :
		if (writeMask & 0x1)
			outVec.x = combinedSrc.x + combinedDst.x;
		if (writeMask & 0x2)
			outVec.y = combinedSrc.y + combinedDst.y;
		if (writeMask & 0x4)
			outVec.z = combinedSrc.z + combinedDst.z;
		if (writeMask & 0x8)
			outVec.w = combinedSrc.w + combinedDst.w;
		break;
	case D3DBLENDOP_SUBTRACT   :
		if (writeMask & 0x1)
			outVec.x = combinedSrc.x - combinedDst.x;
		if (writeMask & 0x2)
			outVec.y = combinedSrc.y - combinedDst.y;
		if (writeMask & 0x4)
			outVec.z = combinedSrc.z - combinedDst.z;
		if (writeMask & 0x8)
			outVec.w = combinedSrc.w - combinedDst.w;
		break;
	case D3DBLENDOP_REVSUBTRACT:
		if (writeMask & 0x1)
			outVec.x = combinedDst.x - combinedSrc.x;
		if (writeMask & 0x2)
			outVec.y = combinedDst.y - combinedSrc.y;
		if (writeMask & 0x4)
			outVec.z = combinedDst.z - combinedSrc.z;
		if (writeMask & 0x8)
			outVec.w = combinedDst.w - combinedSrc.w;
		break;
	case D3DBLENDOP_MIN        :
		if (writeMask & 0x1)
			outVec.x = combinedSrc.x < combinedDst.x ? combinedSrc.x : combinedDst.x;
		if (writeMask & 0x2)
			outVec.y = combinedSrc.y < combinedDst.y ? combinedSrc.y : combinedDst.y;
		if (writeMask & 0x4)
			outVec.z = combinedSrc.z < combinedDst.z ? combinedSrc.z : combinedDst.z;
		if (writeMask & 0x8)
			outVec.w = combinedSrc.w < combinedDst.w ? combinedSrc.w : combinedDst.w;
		break;
	case D3DBLENDOP_MAX        :
		if (writeMask & 0x1)
			outVec.x = combinedSrc.x > combinedDst.x ? combinedSrc.x : combinedDst.x;
		if (writeMask & 0x2)
			outVec.y = combinedSrc.y > combinedDst.y ? combinedSrc.y : combinedDst.y;
		if (writeMask & 0x4)
			outVec.z = combinedSrc.z > combinedDst.z ? combinedSrc.z : combinedDst.z;
		if (writeMask & 0x8)
			outVec.w = combinedSrc.w > combinedDst.w ? combinedSrc.w : combinedDst.w;
		break;
	}
}

// Apply a 4x4 dithering algorithm when writing to surfaces with lower precision than 8 bits per pixel:
static inline void DitherColor(const unsigned x, const unsigned y, D3DXVECTOR4& ditheredColor, const IDirect3DSurface9Hook* const surface)
{
	static const float uniform2bit[4][4] =
	{
		{ (0.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 4.0f },
		{ (12.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 4.0f },
		{ (3.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 4.0f },
		{ (15.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 4.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 4.0f }
	};

	static const float uniform3bit[4][4] =
	{
		{ (0.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 8.0f },
		{ (12.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 8.0f },
		{ (3.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 8.0f },
		{ (15.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 8.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 8.0f }
	};

	static const float uniform4bit[4][4] =
	{
		{ (0.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 16.0f },
		{ (12.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 16.0f },
		{ (3.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 16.0f },
		{ (15.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 16.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 16.0f }
	};

	static const float uniform5bit[4][4] =
	{
		{ (0.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 32.0f },
		{ (12.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 32.0f },
		{ (3.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 32.0f },
		{ (15.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 32.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 32.0f }
	};

	static const float uniform6bit[4][4] =
	{
		{ (0.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (8.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (2.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (10.0f / 16.0f - 8.0f / 16.0f) / 64.0f },
		{ (12.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (4.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (14.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (6.0f / 16.0f - 8.0f / 16.0f) / 64.0f },
		{ (3.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (11.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (1.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (9.0f / 16.0f - 8.0f / 16.0f) / 64.0f },
		{ (15.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (7.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (13.0f / 16.0f - 8.0f / 16.0f) / 64.0f, (5.0f / 16.0f - 8.0f / 16.0f) / 64.0f }
	};

	const unsigned xMod = x & 0x3;
	const unsigned yMod = y & 0x3;

	switch (surface->GetInternalFormat() )
	{
	default:
		return;
	case D3DFMT_R5G6B5               :
		ditheredColor.x += uniform5bit[xMod][yMod];
		ditheredColor.y += uniform6bit[xMod][yMod];
		ditheredColor.z += uniform5bit[xMod][yMod];
		break;
	case D3DFMT_A1R5G5B5             : // Don't dither the 1 bit, it won't look good
	case D3DFMT_X1R5G5B5             :
		ditheredColor.x += uniform5bit[xMod][yMod];
		ditheredColor.y += uniform6bit[xMod][yMod];
		ditheredColor.z += uniform5bit[xMod][yMod];
		break;
	case D3DFMT_A4R4G4B4             :
		ditheredColor.x += uniform4bit[xMod][yMod];
		ditheredColor.y += uniform4bit[xMod][yMod];
		ditheredColor.z += uniform4bit[xMod][yMod];
		ditheredColor.w += uniform4bit[xMod][yMod];
		break;
	case D3DFMT_A8R3G3B2             :
		ditheredColor.x += uniform3bit[xMod][yMod];
		ditheredColor.y += uniform3bit[xMod][yMod];
		ditheredColor.z += uniform2bit[xMod][yMod];
		break;
	case D3DFMT_X4R4G4B4             :
		ditheredColor.x += uniform4bit[xMod][yMod];
		ditheredColor.y += uniform4bit[xMod][yMod];
		ditheredColor.z += uniform4bit[xMod][yMod];
		break;
	case D3DFMT_A4L4                 :
		ditheredColor.x += uniform4bit[xMod][yMod];
		ditheredColor.y += uniform4bit[xMod][yMod];
		ditheredColor.z += uniform4bit[xMod][yMod];
		ditheredColor.w += uniform4bit[xMod][yMod];
		break;
	}
}

template <const unsigned char writeMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const
{
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaBlendEnable)
	{
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.separateAlphaBlendEnable) // Have to run alpha blending twice for separate alpha
		{
			// Alpha blend skip:
			if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_SRCALPHA && 
				currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_INVSRCALPHA) // TODO: Check the for the less common Min/Max/Reversesubtract blend ops
			{
				if (value.w == 0.0f)
					return;
			}
			// Additive blend skip
			else if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_ONE &&
				currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_ONE) // TODO: Check for the less common Min/Max/Reversesubtract blend ops
			{
				if (value.x == 0.0f && value.y == 0.0f && value.z == 0.0f)
					return;
			}

			D3DXVECTOR4 dstColor, finalColor;
			outSurface->GetPixelVec<writeMask, false>(x, y, dstColor);

			D3DXVECTOR4 srcBlendColor, dstBlendColor;
			LoadSrcBlend<writeMask & 0x7>(srcBlendColor, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend, value, dstColor);
			LoadDstBlend<writeMask & 0x7>(dstBlendColor, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend, value, dstColor);
			AlphaBlend<writeMask & 0x7>(finalColor, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOp, srcBlendColor, dstBlendColor, value, dstColor);

			if (writeMask & 0x8)
			{
				D3DXVECTOR4 srcBlendAlpha, dstBlendAlpha, finalAlpha;
				LoadSrcBlend<writeMask & 0x8>(srcBlendAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlendAlpha, value, dstColor);
				LoadDstBlend<writeMask & 0x8>(dstBlendAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlendAlpha, value, dstColor);
				AlphaBlend<writeMask & 0x8>(finalAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOpAlpha, srcBlendAlpha, dstBlendAlpha, value, dstColor);
				finalColor.w = finalAlpha.w;
			}

			if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
				DitherColor(x, y, finalColor, outSurface);

			outSurface->SetPixelVec<writeMask>(x, y, finalColor);
		}
		else // Simple alpha blending without separate alpha
		{
			// Alpha blend skip:
			if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_SRCALPHA && 
				currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_INVSRCALPHA) // TODO: Check the for the less common Min/Max/Reversesubtract blend ops
			{
				if (value.w == 0.0f)
					return;
			}
			// Additive blend skip
			else if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_ONE &&
				currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_ONE) // TODO: Check for the less common Min/Max/Reversesubtract blend ops
			{
				if (value.x == 0.0f && value.y == 0.0f && value.z == 0.0f)
					return;
			}
			__declspec(align(16) ) D3DXVECTOR4 dstColor;
			outSurface->GetPixelVec<writeMask, false>(x, y, dstColor);

			__declspec(align(16) ) D3DXVECTOR4 srcBlend;
			__declspec(align(16) ) D3DXVECTOR4 dstBlend;
			LoadSrcBlend<writeMask>(srcBlend, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend, value, dstColor);
			LoadDstBlend<writeMask>(dstBlend, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend, value, dstColor);
			AlphaBlend<writeMask>(dstColor, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOp, srcBlend, dstBlend, value, dstColor);

			if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
				DitherColor(x, y, dstColor, outSurface);

			outSurface->SetPixelVec<writeMask>(x, y, dstColor);
		}
	}
	else // Super simple - no alpha blending at all!
	{
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
		{
			D3DXVECTOR4 ditheredColor = value;
			DitherColor(x, y, ditheredColor, outSurface);
			outSurface->SetPixelVec<writeMask>(x, y, ditheredColor);
		}
		else
			outSurface->SetPixelVec<writeMask>(x, y, value);
	}
}

void IDirect3DDevice9Hook::RenderOutput(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const
{
	// Increment pixels written stat (even if write mask is 0)
	++frameStats.numPixelsWritten;

	switch (currentState.currentRenderStates.renderStatesUnion.namedStates.colorWriteEnable)
	{
	case 0:
#ifdef _DEBUG
		__debugbreak(); // Should never be here
#else
		__assume(0);
#endif
		return;
	case 1:
		ROPBlendWriteMask<1>(outSurface, x, y, value);
		break;
	case 2:
		ROPBlendWriteMask<2>(outSurface, x, y, value);
		break;
	case 3:
		ROPBlendWriteMask<3>(outSurface, x, y, value);
		break;
	case 4:
		ROPBlendWriteMask<4>(outSurface, x, y, value);
		break;
	case 5:
		ROPBlendWriteMask<5>(outSurface, x, y, value);
		break;
	case 6:
		ROPBlendWriteMask<6>(outSurface, x, y, value);
		break;
	case 7:
		ROPBlendWriteMask<7>(outSurface, x, y, value);
		break;
	case 8:
		ROPBlendWriteMask<8>(outSurface, x, y, value);
		break;
	case 9:
		ROPBlendWriteMask<9>(outSurface, x, y, value);
		break;
	case 10:
		ROPBlendWriteMask<10>(outSurface, x, y, value);
		break;
	case 11:
		ROPBlendWriteMask<11>(outSurface, x, y, value);
		break;
	case 12:
		ROPBlendWriteMask<12>(outSurface, x, y, value);
		break;
	case 13:
		ROPBlendWriteMask<13>(outSurface, x, y, value);
		break;
	case 14:
		ROPBlendWriteMask<14>(outSurface, x, y, value);
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Unexpected write mask!");
#else
		__assume(0);
#endif
	case 15:
		ROPBlendWriteMask<15>(outSurface, x, y, value);
		break;
	}
}

// Must be called before shading a pixel to reset the pixel shader state machine!
void IDirect3DDevice9Hook::PreShadePixel(const unsigned x, const unsigned y, PShaderEngine* const pixelShader, PS_2_0_OutputRegisters* const pixelOutput) const
{
	pixelOutput->pixelStatus = normalWrite;
	pixelShader->Reset(x, y, pixelOutput);
}

// Handles running the pixel shader and interpolating input for this pixel from a vertex declaration + raw vertex stream
void IDirect3DDevice9Hook::ShadePixelFromStream(PShaderEngine* const pixelEngine, const DeclarationSemanticMapping& vertexDeclMapping, const unsigned x, const unsigned y, const D3DXVECTOR3& barycentricInterpolants, const UINT offsetBytesToOPosition, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2) const
{
	// Perform Z-clipping (clip the pixel if it's outside of the [0.0, 1.0] range):
	float invZ0, invZ1, invZ2;
	const float pixelDepth = InterpolatePixelDepth(barycentricInterpolants, offsetBytesToOPosition, v0, v1, v2, invZ0, invZ1, invZ2);
	if (pixelDepth < 0.0f)
		return;
	else if (pixelDepth > 1.0f)
		return;

	PS_2_0_OutputRegisters pixelOutput;

	// Very important to reset the state machine back to its original settings!
	PreShadePixel(x, y, pixelEngine, &pixelOutput);

	if (currentState.currentDepthStencil)
	{
		// Return "depth fail/stencil fail" for all texels outside of the bounds of the depth/stencil buffer
		// This can happen as the depth/stencil buffer can be a different size than the current render targets
		if (!currentState.currentDepthStencil->IsTexelValid(x, y) )
			return;

		if (!StencilTestNoWrite(x, y) )
		{
			// Fail the stencil test!
			pixelOutput.pixelStatus = stencilFail;
			ShadePixel(x, y, pixelEngine);
			return;
		}

		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zEnable)
		{
			const unsigned bufferDepth = currentState.currentDepthStencil->GetRawDepth(x, y);
			if (!DepthTest(pixelDepth, bufferDepth, currentState.currentRenderStates.renderStatesUnion.namedStates.zFunc, currentState.currentDepthStencil->GetInternalFormat() ) )
			{
				// Fail the depth test!
				pixelOutput.pixelStatus = ZFail;
				ShadePixel(x, y, pixelEngine);
				return;
			}
			pixelOutput.oDepth = pixelDepth;
		}
	}

	InterpolateStreamIntoRegisters(pixelEngine, vertexDeclMapping, barycentricInterpolants, v0, v1, v2, invZ0, invZ1, invZ2, pixelDepth);

	ShadePixel(x, y, pixelEngine);
}

// This is a non-perspective-correct attribute interpolation:
static inline void InterpolateVertexAttribute(const D3DXVECTOR3& floatBarycentrics, const D3DXVECTOR4& attr0, const D3DXVECTOR4& attr1, const D3DXVECTOR4& attr2, D3DXVECTOR4& outAttr)
{
	outAttr.x = attr0.x * floatBarycentrics.x + attr1.x * floatBarycentrics.y + attr2.x * floatBarycentrics.z;
	outAttr.y = attr0.y * floatBarycentrics.x + attr1.y * floatBarycentrics.y + attr2.y * floatBarycentrics.z;
	outAttr.z = attr0.z * floatBarycentrics.x + attr1.z * floatBarycentrics.y + attr2.z * floatBarycentrics.z;
	outAttr.w = attr0.w * floatBarycentrics.x + attr1.w * floatBarycentrics.y + attr2.w * floatBarycentrics.z;
}

// This is a perspective-correct attribute interpolation:
static inline void InterpolateVertexAttribute_PerspectiveCorrect(const D3DXVECTOR3& floatBarycentrics, const D3DXVECTOR4& attr0, const D3DXVECTOR4& attr1, const D3DXVECTOR4& attr2, const float invZ0, const float invZ1, const float invZ2, const float pixelZ, D3DXVECTOR4& outAttr)
{
	outAttr.x = pixelZ * (attr0.x * invZ0 * floatBarycentrics.x + attr1.x * invZ1 * floatBarycentrics.y + attr2.x * invZ2 * floatBarycentrics.z);
	outAttr.y = pixelZ * (attr0.y * invZ0 * floatBarycentrics.x + attr1.y * invZ1 * floatBarycentrics.y + attr2.y * invZ2 * floatBarycentrics.z);
	outAttr.z = pixelZ * (attr0.z * invZ0 * floatBarycentrics.x + attr1.z * invZ1 * floatBarycentrics.y + attr2.z * invZ2 * floatBarycentrics.z);
	outAttr.w = pixelZ * (attr0.w * invZ0 * floatBarycentrics.x + attr1.w * invZ1 * floatBarycentrics.y + attr2.w * invZ2 * floatBarycentrics.z);
}

// Handles interpolating pixel shader input registers from a vertex declaration + raw vertex stream
void IDirect3DDevice9Hook::InterpolateStreamIntoRegisters(PShaderEngine* const pixelShader, const DeclarationSemanticMapping& vertexDeclMapping, const D3DXVECTOR3& barycentricInterpolants, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, const float invZ0, const float invZ1, const float invZ2, const float pixelZ) const
{
	const ShaderInfo& pixelShaderInfo = currentState.currentPixelShader->GetShaderInfo();
	if (pixelShaderInfo.shaderMajorVersion == 1)
	{
		for (unsigned char v = 0; v < D3DMCS_COLOR2; ++v)
		{
			if (pixelShaderInfo.inputRegistersUsedBitmask & (1 << v) )
			{
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_2_0_inputs.v[v]);

				const DebuggableD3DVERTEXELEMENT9* const foundColorValue = vertexDeclMapping.vals[D3DDECLUSAGE_COLOR][v];
				if (foundColorValue)
				{
					D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = foundColorValue->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + foundColorValue->Offset) );

					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
					{
						// Color interpolator registers:
						D3DXVECTOR4 cf1, cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + foundColorValue->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + foundColorValue->Offset) );

						InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
					}
					else
					{
						interpolatedFloatValue = cf0;
					}
				}
				else
				{
					// It seems like this is correct behavior (color usage elements that are unbound from either the VS or pretransformed vertex data are read into the PS as (1,1,1,1) whereas all other usages are (0,0,0,0) ), but I can't find documentation for this
					interpolatedFloatValue = staticColorWhiteOpaque;
				}
			}
		}
		for (unsigned char t = 0; t < 6; ++t)
		{
			if (pixelShaderInfo.inputRegistersUsedBitmask & (1 << (t + D3DMCS_COLOR2) ) )
			{
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_2_0_inputs.t[t]);

				const DebuggableD3DVERTEXELEMENT9* const foundTexcoordValue = vertexDeclMapping.vals[D3DDECLUSAGE_TEXCOORD][t];
				if (foundTexcoordValue)
				{
					D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = foundTexcoordValue->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + foundTexcoordValue->Offset) );

					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
					{
						// Texcoord interpolator registers:
						D3DXVECTOR4 cf1, cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + foundTexcoordValue->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + foundTexcoordValue->Offset) );

						InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
					}
					else
					{
						interpolatedFloatValue = cf0;
					}
				}
				else
				{
					interpolatedFloatValue = staticColorBlackTranslucent;
				}
			}
		}
	}
	else if (pixelShaderInfo.shaderMajorVersion == 2)
	{
		const unsigned numDeclaredRegisters = pixelShaderInfo.declaredRegisters.size();
		for (unsigned x = 0; x < numDeclaredRegisters; ++x)
		{
			const DeclaredRegister& reg = pixelShaderInfo.declaredRegisters[x];
			if (reg.registerType == D3DSPR_INPUT)
			{
				// Color interpolator registers:
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]);
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDeclMapping.vals[D3DDECLUSAGE_COLOR][reg.usageIndex];
				if (!element)
				{
					// It seems like this is correct behavior (color usage elements that are unbound from either the VS or pretransformed vertex data are read into the PS as (1,1,1,1) whereas all other usages are (0,0,0,0) ), but I can't find documentation for this
					if (reg.usageType == D3DDECLUSAGE_COLOR)
					{
						interpolatedFloatValue = staticColorWhiteOpaque;
					}
					else
					{
						interpolatedFloatValue = staticColorBlackTranslucent;
					}
				}
				else
				{
					D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );

					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Color interpolator registers:
						D3DXVECTOR4 cf1, cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
					}
					else
					{
						interpolatedFloatValue = cf0;
					}
				}
			}
			else if (reg.registerType == D3DSPR_TEXTURE)
			{
				// Texcoord interpolator registers:
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDeclMapping.vals[D3DDECLUSAGE_TEXCOORD][reg.usageIndex];
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]);
				if (!element)
				{
					// It seems like this is correct behavior (color usage elements that are unbound from either the VS or pretransformed vertex data are read into the PS as (1,1,1,1) whereas all other usages are (0,0,0,0) ), but I can't find documentation for this
					if (reg.usageType == D3DDECLUSAGE_COLOR)
					{
						interpolatedFloatValue = staticColorWhiteOpaque;
					}
					else
					{
						interpolatedFloatValue = staticColorBlackTranslucent;
					}
				}
				else
				{
					D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );

					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Texcoord interpolator registers:
						D3DXVECTOR4 cf1, cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
					}
					else
					{
						interpolatedFloatValue = cf0;
					}
				}
			}
		}
	}
	else if (pixelShaderInfo.shaderMajorVersion == 3)
	{
		const unsigned numDeclaredRegisters = pixelShaderInfo.declaredRegisters.size();
		for (unsigned x = 0; x < numDeclaredRegisters; ++x)
		{
			const DeclaredRegister& reg = pixelShaderInfo.declaredRegisters[x];
			if (reg.registerType == D3DSPR_INPUT)
			{
				// Interpolator registers:
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]);
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDeclMapping.vals[reg.usageType][reg.usageIndex];
				if (!element)
				{
					// It seems like this is correct behavior (color usage elements that are unbound from either the VS or pretransformed vertex data are read into the PS as (1,1,1,1) whereas all other usages are (0,0,0,0) ), but I can't find documentation for this
					if (reg.usageType == D3DDECLUSAGE_COLOR)
					{
						interpolatedFloatValue = staticColorWhiteOpaque;
					}
					else
					{
						interpolatedFloatValue = staticColorBlackTranslucent;
					}
				}
				else
				{
					D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;

					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );
					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Interpolator registers:
						D3DXVECTOR4 cf1, cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						// TODO: Implement PS_3_0 interpreters too
						InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
					}
					else
					{
						interpolatedFloatValue = cf0;
					}
				}
			}
		}
	}
#ifdef _DEBUG
	else
	{
		DbgBreakPrint("Error: Unknown pixel shader version specified (not 1, 2, or 3)");
	}
#endif
}

const float IDirect3DDevice9Hook::InterpolatePixelDepth(const D3DXVECTOR3& barycentricInterpolants, const UINT byteOffsetToOPosition, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, float& invZ0, float& invZ1, float& invZ2) const
{
	// TODO: This assumes that the PositionT element is a D3DDECLTYPE_FLOAT4 (which is the default, and the case in the vast majority of situations)
	const D3DXVECTOR4& xyzRhw0 = *(const D3DXVECTOR4* const)(v0 + byteOffsetToOPosition);
	const D3DXVECTOR4& xyzRhw1 = *(const D3DXVECTOR4* const)(v1 + byteOffsetToOPosition);
	const D3DXVECTOR4& xyzRhw2 = *(const D3DXVECTOR4* const)(v2 + byteOffsetToOPosition);

	// Apply perspective-correct Z interpolation:
	const float localInvZ0 = xyzRhw0.z == 0.0f ? (16777216.0f) : 1.0f / xyzRhw0.z;
	const float localInvZ1 = xyzRhw1.z == 0.0f ? (16777216.0f) : 1.0f / xyzRhw1.z;
	const float localInvZ2 = xyzRhw2.z == 0.0f ? (16777216.0f) : 1.0f / xyzRhw2.z;

	const float invInterpolatedDepth = (localInvZ0 * barycentricInterpolants.x) + (localInvZ1 * barycentricInterpolants.y) + (localInvZ2 * barycentricInterpolants.z);

	invZ0 = localInvZ0;
	invZ1 = localInvZ1;
	invZ2 = localInvZ2;

	return 1.0f / invInterpolatedDepth;
}

// Handles interpolating pixel shader input registers from vertex shader output registers
// TODO: Like InterpolateStreamIntoRegisters, have this function fill with (1,1,1,1) for input color usage registers or (0,0,0,0) for other usages if the vertex shader doesn't write to the corresponding output registers
void IDirect3DDevice9Hook::InterpolateShaderIntoRegisters(PShaderEngine* const pixelShader, const VStoPSMapping& vs_psMapping, const D3DXVECTOR3& barycentricInterpolants, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, const float invZ0, const float invZ1, const float invZ2, const float pixelZ) const
{
	const ShaderInfo& pixelShaderInfo = currentState.currentPixelShader->GetShaderInfo();
	if (pixelShaderInfo.shaderMajorVersion == 1)
	{
		// ps_1_* doesn't use input declarations, so we need to use another method to figure out which input registers we need interpolated
		for (unsigned char v = 0; v < D3DMCS_COLOR2; ++v)
		{
			if (pixelShaderInfo.inputRegistersUsedBitmask & (1 << v) )
			{
				const unsigned char vsRegisterIndex = v;
				const D3DXVECTOR4& cf0 = *(const D3DXVECTOR4* const)&(v0.vs_interpolated_outputs.vs_2_0_outputs.oD[vsRegisterIndex]);
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_2_0_inputs.v[v]);
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
				{
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_2_0_outputs.oD[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_2_0_outputs.oD[vsRegisterIndex]);
					InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
				}
				else
				{
					interpolatedFloatValue = cf0;
				}
			}
		}
		for (unsigned char t = 0; t < 6; ++t)
		{
			if (pixelShaderInfo.inputRegistersUsedBitmask & (1 << (t + D3DMCS_COLOR2) ) )
			{
				const unsigned char vsRegisterIndex = t;
				const D3DXVECTOR4& cf0 = *(const D3DXVECTOR4* const)&(v0.vs_interpolated_outputs.vs_2_0_outputs.oT[vsRegisterIndex]);
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_2_0_inputs.t[t]);
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
				{
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_2_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_2_0_outputs.oT[vsRegisterIndex]);
					InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
				}
				else
				{
					interpolatedFloatValue = cf0;
				}
			}
		}
	}
	else if (pixelShaderInfo.shaderMajorVersion == 2)
	{
		const unsigned numDeclaredRegisters = pixelShaderInfo.declaredRegisters.size();
		for (unsigned x = 0; x < numDeclaredRegisters; ++x)
		{
			const DeclaredRegister& reg = pixelShaderInfo.declaredRegisters[x];
			if (reg.registerType == D3DSPR_INPUT)
			{
				// Color interpolator registers:
				const unsigned vsRegisterIndex = vs_psMapping.psInputRegistersUnion.ps_2_0_registers.colors[reg.registerIndex];
				const D3DXVECTOR4& cf0 = *(const D3DXVECTOR4* const)&(v0.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);

				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]);

				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Color interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
				}
				else
				{
					interpolatedFloatValue = cf0;
				}
			}
			else if (reg.registerType == D3DSPR_TEXTURE)
			{
				// Texcoord interpolator registers:
				const unsigned vsRegisterIndex = vs_psMapping.psInputRegistersUnion.ps_2_0_registers.texCoords[reg.registerIndex];
				const D3DXVECTOR4& cf0 = *(const D3DXVECTOR4* const)&(v0.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]);

				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Texcoord interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
				}
				else
				{
					interpolatedFloatValue = cf0;
				}
			}
		}
	}
	else if (pixelShaderInfo.shaderMajorVersion == 3)
	{
		const unsigned numDeclaredRegisters = pixelShaderInfo.declaredRegisters.size();
		for (unsigned x = 0; x < numDeclaredRegisters; ++x)
		{
			const DeclaredRegister& reg = pixelShaderInfo.declaredRegisters[x];
			if (reg.registerType == D3DSPR_INPUT)
			{
				// Interpolator registers:
				const unsigned vsRegisterIndex = vs_psMapping.psInputRegistersUnion.ps_3_0_registers.inputs[reg.registerIndex];
				const D3DXVECTOR4& cf0 = *(const D3DXVECTOR4* const)&(v0.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters.ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]);

				if (reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);

					// TODO: Implement PS_3_0 interpreters too
					InterpolateVertexAttribute_PerspectiveCorrect(barycentricInterpolants, cf0, cf1, cf2, invZ0, invZ1, invZ2, pixelZ, interpolatedFloatValue);
				}
				else
				{
					interpolatedFloatValue = cf0;
				}
			}
		}
	}
#ifdef _DEBUG
	else
	{
		DbgBreakPrint("Error: Unknown pixel shader version specified (not 1, 2, or 3)");
	}
#endif
}

void IDirect3DDevice9Hook::ShadePixel(const unsigned x, const unsigned y, PShaderEngine* const pixelShader) const
{
	++frameStats.numPixelsShaded;

	if (pixelShader->outputRegisters->pixelStatus == normalWrite)
	{
		// Perform pixel shading:
#ifndef FORCE_INTERPRETED_PIXEL_SHADER
		if (currentState.currentPixelShader->jitShaderMain)
		{
			// Execute JIT pixel shader engine:
			currentState.currentPixelShader->jitShaderMain(*pixelShader);
		}
		else
#endif
		{
			// Execute interpreted pixel shader engine:
			pixelShader->InterpreterExecutePixel();
		}
	}

	switch (pixelShader->outputRegisters->pixelStatus)
	{
	case normalWrite:
	{
		// Alpha testing:
		// This MSDN page says that alpha testing only happens against the alpha value from oC0: https://docs.microsoft.com/en-us/windows/desktop/direct3d9/multiple-render-targets
		if (!AlphaTest(*(const D3DXVECTOR4* const)&(pixelShader->outputRegisters->oC[0]) ) )
		{
			++frameStats.numAlphaTestFailPixels;
			return;
		}

		const unsigned xCoord = x >> SUBPIXEL_ACCURACY_BITS;
		const unsigned yCoord = y >> SUBPIXEL_ACCURACY_BITS;
		for (unsigned rt = 0; rt < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++rt)
		{
			IDirect3DSurface9Hook* const currentRenderTarget = currentState.currentRenderTargets[rt];
			if (!currentRenderTarget)
				continue;

			RenderOutput(currentRenderTarget, xCoord, yCoord, *(const D3DXVECTOR4* const)&(pixelShader->outputRegisters->oC[rt]) );
		}

		if (currentState.currentDepthStencil)
		{
			if (currentState.currentRenderStates.renderStatesUnion.namedStates.zWriteEnable)
			{
				const float depthBias = currentState.currentRenderStates.renderStatesUnion.namedStates.depthBias;
				currentState.currentDepthStencil->SetDepth(xCoord, yCoord, pixelShader->outputRegisters->oDepth + depthBias);
			}
			
			if (currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
			{
				StencilPassOperation(x, y);
			}
		}
	}
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Unknown pixelstatus!");
#else
		__assume(0);
#endif
	case discard:
		// Pixel discarded (TEXKILL) case, don't write out anything!
		++frameStats.numPixelsTexkilled;
		break;
	case stencilFail:
		if (currentState.currentDepthStencil && currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
		{
			const unsigned xCoord = x >> SUBPIXEL_ACCURACY_BITS;
			const unsigned yCoord = y >> SUBPIXEL_ACCURACY_BITS;
			StencilFailOperation(xCoord, yCoord);
		}
		break;
	case ZFail:
		if (currentState.currentDepthStencil && currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
		{
			const unsigned xCoord = x >> SUBPIXEL_ACCURACY_BITS;
			const unsigned yCoord = y >> SUBPIXEL_ACCURACY_BITS;
			StencilZFailOperation(xCoord, yCoord);
		}
		break;
	}
}

void IDirect3DDevice9Hook::StencilOperation(const unsigned x, const unsigned y, const D3DSTENCILOP stencilOp) const
{
	const DWORD stencilWriteMask = currentState.currentRenderStates.renderStatesUnion.namedStates.stencilWriteMask;
	switch (stencilOp)
	{
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid D3DSTENCILOP specified for stenciling");
#endif
	case D3DSTENCILOP_KEEP   :
		return;
	case D3DSTENCILOP_ZERO   :
		currentState.currentDepthStencil->SetStencil(x, y, 0);
		break;
	case D3DSTENCILOP_REPLACE:
		currentState.currentDepthStencil->SetStencil(x, y, currentState.currentRenderStates.renderStatesUnion.namedStates.stencilRef & stencilWriteMask);
		break;
	case D3DSTENCILOP_INCRSAT:
	{
		DWORD currentStencil = currentState.currentDepthStencil->GetStencil(x, y);
		++currentStencil;
		const DWORD formatMask = GetStencilFormatMask(currentState.currentDepthStencil->GetInternalFormat() );
		if (currentStencil > formatMask)
			currentStencil = formatMask;
		currentState.currentDepthStencil->SetStencil(x, y, currentStencil & stencilWriteMask);
	}
		break;
	case D3DSTENCILOP_DECRSAT:
	{
		DWORD currentStencil = currentState.currentDepthStencil->GetStencil(x, y);
		if (currentStencil > 0)
			--currentStencil;
		currentState.currentDepthStencil->SetStencil(x, y, currentStencil & stencilWriteMask);
	}
		break;
	case D3DSTENCILOP_INVERT :
	{
		DWORD currentStencil = currentState.currentDepthStencil->GetStencil(x, y);
		currentStencil = ~currentStencil;
		currentState.currentDepthStencil->SetStencil(x, y, currentStencil & stencilWriteMask);
	}
		break;
	case D3DSTENCILOP_INCR   :
	{
		DWORD currentStencil = currentState.currentDepthStencil->GetStencil(x, y);
		++currentStencil;
		currentState.currentDepthStencil->SetStencil(x, y, currentStencil & stencilWriteMask);
	}
		break;
	case D3DSTENCILOP_DECR   :
	{
		DWORD currentStencil = currentState.currentDepthStencil->GetStencil(x, y);
		--currentStencil;
		currentState.currentDepthStencil->SetStencil(x, y, currentStencil & stencilWriteMask);
	}
		break;
	}
}

void IDirect3DDevice9Hook::StencilFailOperation(const unsigned x, const unsigned y) const
{
	StencilOperation(x, y, currentState.currentRenderStates.renderStatesUnion.namedStates.stencilFail);
}

void IDirect3DDevice9Hook::StencilZFailOperation(const unsigned x, const unsigned y) const
{
	StencilOperation(x, y, currentState.currentRenderStates.renderStatesUnion.namedStates.stencilZFail);
}

void IDirect3DDevice9Hook::StencilPassOperation(const unsigned x, const unsigned y) const
{
	StencilOperation(x, y, currentState.currentRenderStates.renderStatesUnion.namedStates.stencilPass);
}

// true = "pass" (draw the pixel), false = "fail" (discard the pixel for all render targets and also discard depth/stencil writes for this pixel)
const bool IDirect3DDevice9Hook::AlphaTest(const D3DXVECTOR4& outColor) const
{
	// Alpha testing:
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaTestEnable)
	{
		switch (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaFunc)
		{
		case D3DCMP_NEVER       :
			return false;
		case D3DCMP_LESS        :
		{
			if (!(outColor.w < currentState.currentRenderStates.cachedAlphaRefFloat) )
				return false;
		}
			break;
		case D3DCMP_EQUAL       :
		{
			if (!(outColor.w == currentState.currentRenderStates.cachedAlphaRefFloat) )
				return false;
		}
			break;
		case D3DCMP_LESSEQUAL   :
		{
			if (!(outColor.w <= currentState.currentRenderStates.cachedAlphaRefFloat) )
				return false;
		}
			break;
		case D3DCMP_GREATER     :
		{
			if (!(outColor.w > currentState.currentRenderStates.cachedAlphaRefFloat) )
				return false;
		}
			break;
		case D3DCMP_NOTEQUAL    :
		{
			if (!(outColor.w != currentState.currentRenderStates.cachedAlphaRefFloat) )
				return false;
		}
			break;
		case D3DCMP_GREATEREQUAL:
		{
			if (!(outColor.w >= currentState.currentRenderStates.cachedAlphaRefFloat) )
				return false;
		}
			break;
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Invalid D3DCMP function passed to Alpha Test");
#endif
		case D3DCMP_ALWAYS      :
			return true;
		}
	}

	return true;
}

// true = "pass" (draw the pixel), false = "fail" (discard the pixel)
const bool IDirect3DDevice9Hook::StencilTestNoWrite(const unsigned x, const unsigned y) const
{
	if (!currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
		return true;

	if (!currentState.currentDepthStencil)
		return true;

	const DWORD rawDestStencilVal = currentState.currentDepthStencil->GetStencil(x, y);
	const DWORD maskedDestStencilVal = currentState.currentRenderStates.renderStatesUnion.namedStates.stencilMask & rawDestStencilVal;

	const DWORD referenceStencilVal = currentState.currentRenderStates.renderStatesUnion.namedStates.stencilRef;
	const DWORD maskedRefStencilVal = currentState.currentRenderStates.renderStatesUnion.namedStates.stencilMask & referenceStencilVal;

	switch (currentState.currentRenderStates.renderStatesUnion.namedStates.stencilFunc)
	{
	case D3DCMP_NEVER       :
		return false;
	case D3DCMP_LESS        :
		return maskedRefStencilVal < maskedDestStencilVal;
	case D3DCMP_EQUAL       :
		return maskedRefStencilVal == maskedDestStencilVal;
	case D3DCMP_LESSEQUAL   :
		return maskedRefStencilVal <= maskedDestStencilVal;
	case D3DCMP_GREATER     :
		return maskedRefStencilVal > maskedDestStencilVal;
	case D3DCMP_NOTEQUAL    :
		return maskedRefStencilVal != maskedDestStencilVal;
	case D3DCMP_GREATEREQUAL:
		return maskedRefStencilVal >= maskedDestStencilVal;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid D3DCMP function passed to stencil test");
#endif
	case D3DCMP_ALWAYS      :
		return true;
	}
}

// Handles running the pixel shader from a processed vertex shader
void IDirect3DDevice9Hook::ShadePixelFromShader(PShaderEngine* const pixelEngine, const VStoPSMapping& vs_psMapping, const unsigned x, const unsigned y, const D3DXVECTOR3& barycentricInterpolants, const UINT byteOffsetToOPosition, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2) const
{
	// Perform Z-clipping (clip the pixel if it's outside of the [0.0, 1.0] range):
	float invZ0, invZ1, invZ2;
	const float pixelDepth = InterpolatePixelDepth(barycentricInterpolants, byteOffsetToOPosition, (CONST BYTE* const)&v0, (CONST BYTE* const)&v1, (CONST BYTE* const)&v2, invZ0, invZ1, invZ2);
	if (pixelDepth < 0.0f)
		return;
	else if (pixelDepth > 1.0f)
		return;

	PS_2_0_OutputRegisters pixelOutput;

	// Very important to reset the state machine back to its original settings!
	PreShadePixel(x, y, pixelEngine, &pixelOutput);

	if (currentState.currentDepthStencil)
	{
		// Return "depth fail/stencil fail" for all texels outside of the bounds of the depth/stencil buffer
		// This can happen as the depth/stencil buffer can be a different size than the current render targets
		if (!currentState.currentDepthStencil->IsTexelValid(x, y) )
			return;

		if (!StencilTestNoWrite(x, y) )
		{
			// Fail the stencil test!
			pixelOutput.pixelStatus = stencilFail;
			ShadePixel(x, y, pixelEngine);
			return;
		}

		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zEnable)
		{
			const unsigned bufferDepth = currentState.currentDepthStencil->GetRawDepth(x, y);
			if (!DepthTest(pixelDepth, bufferDepth, currentState.currentRenderStates.renderStatesUnion.namedStates.zFunc, currentState.currentDepthStencil->GetInternalFormat() ) )
			{
				// Fail the depth test!
				pixelOutput.pixelStatus = ZFail;
				ShadePixel(x, y, pixelEngine);
				return;
			}
			pixelOutput.oDepth = pixelDepth;
		}
	}

	InterpolateShaderIntoRegisters(pixelEngine, vs_psMapping, barycentricInterpolants, v0, v1, v2, invZ0, invZ1, invZ2, pixelDepth);

	ShadePixel(x, y, pixelEngine);
}

static inline const bool isTopLeftEdge(const int2& v0, const int2& v1)
{
	//return (v1.y < v0.y) | (v0.x < v1.x);
	const int dx = v1.x - v0.x;
	const int dy = v1.y - v0.y;
	return ( (dy < 0) || ( (dy == 0) && (dx < 0) ) );
}

// Assumes pre-transformed vertices from a vertex declaration + raw vertex stream
void IDirect3DDevice9Hook::RasterizeLineFromStream(const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0, CONST BYTE* const v1) const
{
#ifdef ENABLE_END_TO_SKIP_DRAWS
	if ( (GetAsyncKeyState(VK_END) & 0x8000) )
	{
		// Skip rendering if END is held down
		return;
	}
#endif

	// Do nothing, line rasterization is not yet implemented
}

// Assumes pre-transformed vertex from a vertex declaration + raw vertex stream
void IDirect3DDevice9Hook::RasterizePointFromStream(const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0) const
{
#ifdef ENABLE_END_TO_SKIP_DRAWS
	if ( (GetAsyncKeyState(VK_END) & 0x8000) )
	{
		// Skip rendering if END is held down
		return;
	}
#endif

	// Do nothing, point rasterization is not yet implemented
}

// Assumes pre-transformed vertices from a vertex declaration + raw vertex stream
template <const bool rasterizerUsesEarlyZTest>
void IDirect3DDevice9Hook::RasterizeTriangleFromStream(const DeclarationSemanticMapping& vertexDeclMapping, CONST D3DXVECTOR4* const v0, CONST D3DXVECTOR4* const v1, CONST D3DXVECTOR4* const v2, 
	const float fWidth, const float fHeight, const UINT primitiveID, const UINT vertex0index, const UINT vertex1index, const UINT vertex2index) const
{
	const D3DXVECTOR4& pos0 = *v0;
	const D3DXVECTOR4& pos1 = *v1;
	const D3DXVECTOR4& pos2 = *v2;

	// Near plane culling:
	// TODO: Replace with real triangle clipping!
	if (pos0.z < 0.0f || pos0.z > 1.0f)
		return;
	if (pos1.z < 0.0f || pos1.z > 1.0f)
		return;
	if (pos2.z < 0.0f || pos2.z > 1.0f)
		return;

	// Compute screenspace bounds for this triangle:
	D3DXVECTOR2 topleft(pos0.x, pos0.y);
	D3DXVECTOR2 botright(topleft);
	if (pos1.x < topleft.x)
		topleft.x = pos1.x;
	if (pos2.x < topleft.x)
		topleft.x = pos2.x;
	if (pos1.y < topleft.y)
		topleft.y = pos1.y;
	if (pos2.y < topleft.y)
		topleft.y = pos2.y;
	if (pos1.x > botright.x)
		botright.x = pos1.x;
	if (pos2.x > botright.x)
		botright.x = pos2.x;
	if (pos1.y > botright.y)
		botright.y = pos1.y;
	if (pos2.y > botright.y)
		botright.y = pos2.y;

	// Cull triangles that intersect the guard band:
	// TODO: Replace with real triangle clipping!
	if (topleft.x < -8192.0f || topleft.x > 8192.0f)
		return;
	if (topleft.y < -8192.0f || topleft.y > 8192.0f)
		return;
	if (botright.x < -8192.0f || botright.x > 8192.0f)
		return;
	if (botright.y < -8192.0f || botright.y > 8192.0f)
		return;

	// Clip screenspace bounds to the screen size:
	if (topleft.x < 0)
		topleft.x = 0;
	if (topleft.y < 0)
		topleft.y = 0;
	if (botright.x > fWidth)
		botright.x = fWidth;
	if (botright.y > fHeight)
		botright.y = fHeight;

	// Clip to the scissor rect, if enabled
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.scissorTestEnable)
	{
		if (topleft.x < currentState.currentScissorRect.fleft)
			topleft.x = currentState.currentScissorRect.fleft;
		if (topleft.y < currentState.currentScissorRect.ftop)
			topleft.y = currentState.currentScissorRect.ftop;
		if (botright.x > currentState.currentScissorRect.fright)
			botright.x = currentState.currentScissorRect.fright;
		if (botright.y > currentState.currentScissorRect.fbottom)
			botright.y = currentState.currentScissorRect.fbottom;
	}

	topleft.x *= SUBPIXEL_ACCURACY_BIASMULTF;
	topleft.y *= SUBPIXEL_ACCURACY_BIASMULTF;
	botright.x *= SUBPIXEL_ACCURACY_BIASMULTF;
	botright.y *= SUBPIXEL_ACCURACY_BIASMULTF;

	int2 i0, i1, i2;
	i0.x = (const int)(pos0.x * SUBPIXEL_ACCURACY_BIASMULTF);
	i0.y = (const int)(pos0.y * SUBPIXEL_ACCURACY_BIASMULTF);
	i1.x = (const int)(pos1.x * SUBPIXEL_ACCURACY_BIASMULTF);
	i1.y = (const int)(pos1.y * SUBPIXEL_ACCURACY_BIASMULTF);
	i2.x = (const int)(pos2.x * SUBPIXEL_ACCURACY_BIASMULTF);
	i2.y = (const int)(pos2.y * SUBPIXEL_ACCURACY_BIASMULTF);

	const int xMin = (const int)topleft.x;
	const int yMin = (const int)topleft.y;
	const int xMax = (const int)botright.x;
	const int yMax = (const int)botright.y;

	// Early out on zero-area triangles
	if (!(yMin <= yMax && xMin < xMax) )
		return;

	// Cull zero or negative-area triangles (with our default triangle winding being CW, this will also cull CCW triangles):
	const int maxNumPixels = (yMax - yMin) * (xMax - xMin);
	if (maxNumPixels < 1)
		return;

	const int twiceTriangleArea = computeEdgeSidedness(i0.x, i0.y, i1.x, i1.y, i2.x, i2.y);
	const float barycentricNormalizeFactor = 1.0f / twiceTriangleArea;

	const int barycentricXDelta0 = i0.y - i1.y;
	const int barycentricXDelta1 = i1.y - i2.y;
	const int barycentricXDelta2 = i2.y - i0.y;

	const int barycentricYDelta0 = i1.x - i0.x;
	const int barycentricYDelta1 = i2.x - i1.x;
	const int barycentricYDelta2 = i0.x - i2.x;

	// Correct for top-left rule. Source: https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
	const char topleftEdgeBias0 = isTopLeftEdge(i1, i2) ? 0 : -1;
	const char topleftEdgeBias1 = isTopLeftEdge(i2, i0) ? 0 : -1;
	const char topleftEdgeBias2 = isTopLeftEdge(i0, i1) ? 0 : -1;

	int row0 = computeEdgeSidedness(i1.x, i1.y, i2.x, i2.y, xMin, yMin) + topleftEdgeBias0;
	int row1 = computeEdgeSidedness(i2.x, i2.y, i0.x, i0.y, xMin, yMin) + topleftEdgeBias1;
	int row2 = computeEdgeSidedness(i0.x, i0.y, i1.x, i1.y, xMin, yMin) + topleftEdgeBias2;

	unsigned earlyZTestDepthValue;
	const IDirect3DSurface9Hook* depthStencil;
	if (rasterizerUsesEarlyZTest)
	{
		depthStencil = currentState.currentDepthStencil;

		// TODO: Don't assume less-than test for Z CMPFUNC
		float minDepthValue = pos0.z < pos1.z ? pos0.z : pos1.z;
		minDepthValue = minDepthValue < pos2.z ? minDepthValue : pos2.z;

		earlyZTestDepthValue = depthStencil->GetRawDepthValueFromFloatDepth(minDepthValue);
	}

	const primitivePixelJobData* const primitiveData = GetNewPrimitiveJobData(v0, v1, v2, barycentricNormalizeFactor, primitiveID, twiceTriangleArea > 0, vertex0index, vertex1index, vertex2index);
	for (int y = yMin; y <= yMax; y += SUBPIXEL_ACCURACY_BIASMULT)
	{
		// Reset at the next row:
		int currentBarycentric0 = row0;
		int currentBarycentric1 = row1;
		int currentBarycentric2 = row2;

		for (int x = xMin; x < xMax; x += SUBPIXEL_ACCURACY_BIASMULT)
		{
			// Is our test-pixel inside all three triangle edges?
			if ( (currentBarycentric0 | currentBarycentric1 | currentBarycentric2) >= 0)
			{
				if (rasterizerUsesEarlyZTest)
				{
					const unsigned compareRawDepth = depthStencil->GetRawDepth(x, y);

					// TODO: Don't assume less-than test for Z CMPFUNC
					if (compareRawDepth < earlyZTestDepthValue)
					{
						currentBarycentric0 += barycentricXDelta1;
						currentBarycentric1 += barycentricXDelta2;
						currentBarycentric2 += barycentricXDelta0;
						continue;
					}
				}
#ifdef MULTITHREAD_SHADING
				CreateNewPixelShadeJob(x, y, currentBarycentric0 - topleftEdgeBias0, currentBarycentric1 - topleftEdgeBias1, currentBarycentric2 - topleftEdgeBias2, primitiveData);
#else
				ShadePixelFromStream(&deviceMainPShaderEngine, vertexDeclMapping, x, y, 
					D3DXVECTOR3( (currentBarycentric0 - topleftEdgeBias0) * barycentricNormalizeFactor, 
						(currentBarycentric1 - topleftEdgeBias1) * barycentricNormalizeFactor, 
						(currentBarycentric2 - topleftEdgeBias2) * barycentricNormalizeFactor), 
					currentDrawCallData.pixelData.offsetIntoVertexForOPosition_Bytes, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2);
#endif
			}

			currentBarycentric0 += barycentricXDelta1;
			currentBarycentric1 += barycentricXDelta2;
			currentBarycentric2 += barycentricXDelta0;
		}

		row0 += barycentricYDelta1;
		row1 += barycentricYDelta2;
		row2 += barycentricYDelta0;
	}
}

// Assumes pre-transformed vertices from a processed vertex shader
void IDirect3DDevice9Hook::RasterizeLineFromShader(const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1) const
{
#ifdef ENABLE_END_TO_SKIP_DRAWS
	if ( (GetAsyncKeyState(VK_END) & 0x8000) )
	{
		// Skip rendering if END is held down
		return;
	}
#endif

	// Do nothing, line rasterization is not yet implemented
}

// Assumes pre-transformed vertex from a processed vertex shader
void IDirect3DDevice9Hook::RasterizePointFromShader(const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0) const
{
#ifdef ENABLE_END_TO_SKIP_DRAWS
	if ( (GetAsyncKeyState(VK_END) & 0x8000) )
	{
		// Skip rendering if END is held down
		return;
	}
#endif

	// Do nothing, point rasterization is not yet implemented
}

// Assumes pre-transformed vertices from a processed vertex shader
template <const bool rasterizerUsesEarlyZTest>
void IDirect3DDevice9Hook::RasterizeTriangleFromShader(const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, 
	const float fWidth, const float fHeight, const UINT primitiveID, const UINT vertex0index, const UINT vertex1index, const UINT vertex2index) const
{
	const D3DXVECTOR4& pos0 = currentState.currentVertexShader->GetPosition(v0);
	const D3DXVECTOR4& pos1 = currentState.currentVertexShader->GetPosition(v1);
	const D3DXVECTOR4& pos2 = currentState.currentVertexShader->GetPosition(v2);

	// Near plane culling:
	// TODO: Replace with real triangle clipping!
	if (pos0.z < 0.0f || pos0.z > 1.0f)
		return;
	if (pos1.z < 0.0f || pos1.z > 1.0f)
		return;
	if (pos2.z < 0.0f || pos2.z > 1.0f)
		return;

	// Compute screenspace bounds for this triangle:
	D3DXVECTOR2 topleft(pos0.x, pos0.y);
	D3DXVECTOR2 botright(topleft);
	if (pos1.x < topleft.x)
		topleft.x = pos1.x;
	if (pos2.x < topleft.x)
		topleft.x = pos2.x;
	if (pos1.y < topleft.y)
		topleft.y = pos1.y;
	if (pos2.y < topleft.y)
		topleft.y = pos2.y;
	if (pos1.x > botright.x)
		botright.x = pos1.x;
	if (pos2.x > botright.x)
		botright.x = pos2.x;
	if (pos1.y > botright.y)
		botright.y = pos1.y;
	if (pos2.y > botright.y)
		botright.y = pos2.y;

	// Cull triangles that intersect the guard band:
	// TODO: Replace with real triangle clipping!
	if (topleft.x < -8192.0f || topleft.x > 8192.0f)
		return;
	if (topleft.y < -8192.0f || topleft.y > 8192.0f)
		return;
	if (botright.x < -8192.0f || botright.x > 8192.0f)
		return;
	if (botright.y < -8192.0f || botright.y > 8192.0f)
		return;

	// Clip screenspace bounds to the screen size:
	if (topleft.x < 0)
		topleft.x = 0;
	if (topleft.y < 0)
		topleft.y = 0;
	if (botright.x > fWidth)
		botright.x = fWidth;
	if (botright.y > fHeight)
		botright.y = fHeight;

	// Clip to the scissor rect, if enabled
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.scissorTestEnable)
	{
		if (topleft.x < currentState.currentScissorRect.fleft)
			topleft.x = currentState.currentScissorRect.fleft;
		if (topleft.y < currentState.currentScissorRect.ftop)
			topleft.y = currentState.currentScissorRect.ftop;
		if (botright.x > currentState.currentScissorRect.fright)
			botright.x = currentState.currentScissorRect.fright;
		if (botright.y > currentState.currentScissorRect.fbottom)
			botright.y = currentState.currentScissorRect.fbottom;
	}

	topleft.x *= SUBPIXEL_ACCURACY_BIASMULTF;
	topleft.y *= SUBPIXEL_ACCURACY_BIASMULTF;
	botright.x *= SUBPIXEL_ACCURACY_BIASMULTF;
	botright.y *= SUBPIXEL_ACCURACY_BIASMULTF;

	int2 i0, i1, i2;
	i0.x = (const int)(pos0.x * SUBPIXEL_ACCURACY_BIASMULTF);
	i0.y = (const int)(pos0.y * SUBPIXEL_ACCURACY_BIASMULTF);
	i1.x = (const int)(pos1.x * SUBPIXEL_ACCURACY_BIASMULTF);
	i1.y = (const int)(pos1.y * SUBPIXEL_ACCURACY_BIASMULTF);
	i2.x = (const int)(pos2.x * SUBPIXEL_ACCURACY_BIASMULTF);
	i2.y = (const int)(pos2.y * SUBPIXEL_ACCURACY_BIASMULTF);

	const int xMin = (const int)topleft.x;
	const int yMin = (const int)topleft.y;
	const int xMax = (const int)botright.x;
	const int yMax = (const int)botright.y;

	// Early out on zero area triangles
	if (!(yMin <= yMax && xMin < xMax) )
		return;

	// Cull zero or negative-area triangles (with our default triangle winding being CW, this will also cull CCW triangles):
	const int maxNumPixels = (yMax - yMin) * (xMax - xMin);
	if (maxNumPixels < 1)
		return;

	const int twiceTriangleArea = computeEdgeSidedness(i0.x, i0.y, i1.x, i1.y, i2.x, i2.y);
	const float barycentricNormalizeFactor = 1.0f / twiceTriangleArea;

	const int barycentricXDelta0 = i0.y - i1.y;
	const int barycentricXDelta1 = i1.y - i2.y;
	const int barycentricXDelta2 = i2.y - i0.y;

	const int barycentricYDelta0 = i1.x - i0.x;
	const int barycentricYDelta1 = i2.x - i1.x;
	const int barycentricYDelta2 = i0.x - i2.x;

	// Correct for top-left rule. Source: https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
	const char topleftEdgeBias0 = isTopLeftEdge(i1, i2) ? 0 : -1;
	const char topleftEdgeBias1 = isTopLeftEdge(i2, i0) ? 0 : -1;
	const char topleftEdgeBias2 = isTopLeftEdge(i0, i1) ? 0 : -1;

	int row0 = computeEdgeSidedness(i1.x, i1.y, i2.x, i2.y, xMin, yMin) + topleftEdgeBias0;
	int row1 = computeEdgeSidedness(i2.x, i2.y, i0.x, i0.y, xMin, yMin) + topleftEdgeBias1;
	int row2 = computeEdgeSidedness(i0.x, i0.y, i1.x, i1.y, xMin, yMin) + topleftEdgeBias2;

	unsigned earlyZTestDepthValue;
	const IDirect3DSurface9Hook* depthStencil;
	if (rasterizerUsesEarlyZTest)
	{
		depthStencil = currentState.currentDepthStencil;

		// TODO: Don't assume less-than test for Z CMPFUNC
		float minDepthValue = pos0.z < pos1.z ? pos0.z : pos1.z;
		minDepthValue = minDepthValue < pos2.z ? minDepthValue : pos2.z;

		earlyZTestDepthValue = depthStencil->GetRawDepthValueFromFloatDepth(minDepthValue);
	}

	const primitivePixelJobData* const primitiveData = GetNewPrimitiveJobData(&v0, &v1, &v2, barycentricNormalizeFactor, primitiveID, twiceTriangleArea > 0, vertex0index, vertex1index, vertex2index);
	for (int y = yMin; y <= yMax; y += SUBPIXEL_ACCURACY_BIASMULT)
	{
		// Reset at the next row:
		int currentBarycentric0 = row0;
		int currentBarycentric1 = row1;
		int currentBarycentric2 = row2;

		for (int x = xMin; x < xMax; x += SUBPIXEL_ACCURACY_BIASMULT)
		{
			// Is our test-pixel inside all three triangle edges?
			if ( (currentBarycentric0 | currentBarycentric1 | currentBarycentric2) >= 0)
			{
				if (rasterizerUsesEarlyZTest)
				{
					const unsigned compareDepth = depthStencil->GetRawDepth(x, y);

					// TODO: Don't assume less-than test for Z CMPFUNC
					if (compareDepth < earlyZTestDepthValue)
					{
						currentBarycentric0 += barycentricXDelta1;
						currentBarycentric1 += barycentricXDelta2;
						currentBarycentric2 += barycentricXDelta0;
						continue;
					}
				}
#ifdef MULTITHREAD_SHADING
				CreateNewPixelShadeJob(x, y, currentBarycentric0 - topleftEdgeBias0, currentBarycentric1 - topleftEdgeBias1, currentBarycentric2 - topleftEdgeBias2, primitiveData);
#else // #ifdef MULTITHREAD_SHADING

#ifdef PROFILE_AVERAGE_PIXEL_SHADE_TIMES
				LARGE_INTEGER pixelStartTime;
				QueryPerformanceCounter(&pixelStartTime);
#endif // PROFILE_AVERAGE_PIXEL_SHADE_TIMES
				ShadePixelFromShader(&deviceMainPShaderEngine, vs_psMapping, x, y, 
					D3DXVECTOR3( (currentBarycentric0 - topleftEdgeBias0) * barycentricNormalizeFactor, 
						(currentBarycentric1 - topleftEdgeBias1) * barycentricNormalizeFactor, 
						(currentBarycentric2 - topleftEdgeBias2) * barycentricNormalizeFactor), 
					currentDrawCallData.pixelData.offsetIntoVertexForOPosition_Bytes, v0, v1, v2);

#ifdef PROFILE_AVERAGE_PIXEL_SHADE_TIMES
				LARGE_INTEGER pixelEndTime;
				QueryPerformanceCounter(&pixelEndTime);

				totalPixelShadeTicks += (pixelEndTime.QuadPart - pixelStartTime.QuadPart);
				++numPixelShadeTasks;
#endif // PROFILE_AVERAGE_PIXEL_SHADE_TIMES

#endif // #ifdef MULTITHREAD_SHADING
			}

			currentBarycentric0 += barycentricXDelta1;
			currentBarycentric1 += barycentricXDelta2;
			currentBarycentric2 += barycentricXDelta0;
		}

		row0 += barycentricYDelta1;
		row1 += barycentricYDelta2;
		row2 += barycentricYDelta0;
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DrawPrimitiveUP(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	HRESULT ret;

	if (PrimitiveCount == 0)
		return D3DERR_INVALIDCALL;

	if (!pVertexStreamZeroData)
		return D3DERR_INVALIDCALL;

	if (PrimitiveType > D3DPT_TRIANGLEFAN || PrimitiveType < D3DPT_POINTLIST)
		return D3DERR_INVALIDCALL;

	if (!currentState.currentVertexDecl)
	{
#ifdef _DEBUG
		DbgBreakPrint("Error: Vertex decl is NULL");
#endif
		return D3DERR_INVALIDCALL;
	}

#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}
#endif

	if (VertexStreamZeroStride > 0xFFFF)
		return D3DERR_INVALIDCALL;

	if (!TotalDrawCallSkipTest() )
		return S_OK;

	const unsigned numInputVerts = GetNumVertsUsed(PrimitiveType, PrimitiveCount);

	const unsigned short shortVertexStreamZeroStride = (const unsigned short)VertexStreamZeroStride;
	currentState.currentSoftUPStream.vertexBuffer->SoftUPSetInternalPointer( (const BYTE* const)pVertexStreamZeroData);
	currentState.currentSoftUPStream.streamOffset = 0;
	currentState.currentSoftUPStream.streamStride = shortVertexStreamZeroStride;
	currentState.currentSoftUPStream.streamDividerFrequency = D3DSTREAMSOURCE_INDEXEDDATA;

	SwapWithCopy(currentState.currentSoftUPStream, currentState.currentStreams[0]);
	SwapWithCopy(currentState.currentSoftUPStreamEnd, currentState.currentStreamEnds[0]);

	// Recompute after the swap since this function operates on currentState.currentStreamEnds[0] expecting it to already be the currentSoftUPStreamEnd
	RecomputeCachedStreamEndsForUP( (const BYTE* const)pVertexStreamZeroData, numInputVerts, shortVertexStreamZeroStride);

	ret = DrawPrimitive(PrimitiveType, 0, PrimitiveCount);

	// Undo swap after our draw call is completed
	SwapWithCopy(currentState.currentSoftUPStreamEnd, currentState.currentStreamEnds[0]);
	SwapWithCopy(currentState.currentSoftUPStream, currentState.currentStreams[0]);

	currentState.currentSoftUPStream.vertexBuffer->SoftUPResetInternalPointer();
	currentState.currentSoftUPStream.streamStride = 0;

	// Following any IDirect3DDevice9::DrawPrimitiveUP call, the stream 0 settings, referenced by IDirect3DDevice9::GetStreamSource, are set to NULL.
	// Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb174372(v=vs.85).aspx
	if (currentState.currentStreams[0].vertexBuffer != NULL)
	{
		SetStreamSource(0, NULL, 0, 0);
	}

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DrawIndexedPrimitiveUP(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	HRESULT ret;
#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}
#endif

	if (PrimitiveCount == 0)
		return D3DERR_INVALIDCALL;

	if (NumVertices == 0)
		return D3DERR_INVALIDCALL;

	if (MinVertexIndex >= NumVertices)
		return D3DERR_INVALIDCALL;

	if (!pIndexData)
		return D3DERR_INVALIDCALL;

	if (IndexDataFormat < D3DFMT_INDEX16 || IndexDataFormat > D3DFMT_INDEX32)
		return D3DERR_INVALIDCALL;

	if (!pVertexStreamZeroData)
		return D3DERR_INVALIDCALL;

	if (PrimitiveType > D3DPT_TRIANGLEFAN || PrimitiveType < D3DPT_POINTLIST)
		return D3DERR_INVALIDCALL;

	if (VertexStreamZeroStride > 0xFFFF)
		return D3DERR_INVALIDCALL;

	if (!TotalDrawCallSkipTest() )
		return S_OK;

	const unsigned short shortVertexStreamZeroStride = (const unsigned short)VertexStreamZeroStride;

	currentState.currentSoftUPStream.vertexBuffer->SoftUPSetInternalPointer( (const BYTE* const)pVertexStreamZeroData);
	currentState.currentSoftUPStream.streamOffset = 0;
	currentState.currentSoftUPStream.streamStride = shortVertexStreamZeroStride;
	currentState.currentSoftUPStream.streamDividerFrequency = D3DSTREAMSOURCE_INDEXEDDATA;

	currentState.currentSoftUPIndexBuffer->SoftUPSetInternalPointer(pIndexData, IndexDataFormat);

	SwapWithCopy(currentState.currentSoftUPStream, currentState.currentStreams[0]);
	SwapWithCopy(currentState.currentSoftUPStreamEnd, currentState.currentStreamEnds[0]);
	SwapWithCopy(currentState.currentSoftUPIndexBuffer, currentState.currentIndexBuffer);

	// Recompute after the swap since this function operates on currentState.currentStreamEnds[0] expecting it to already be the currentSoftUPStreamEnd
	RecomputeCachedStreamEndsForUP( (const BYTE* const)pVertexStreamZeroData, NumVertices, shortVertexStreamZeroStride);

	ret = DrawIndexedPrimitive(PrimitiveType, 0, MinVertexIndex, NumVertices, 0, PrimitiveCount);

	// Undo swap after our draw call is completed
	SwapWithCopy(currentState.currentSoftUPIndexBuffer, currentState.currentIndexBuffer);
	SwapWithCopy(currentState.currentSoftUPStreamEnd, currentState.currentStreamEnds[0]);
	SwapWithCopy(currentState.currentSoftUPStream, currentState.currentStreams[0]);

	currentState.currentSoftUPStream.vertexBuffer->SoftUPResetInternalPointer();
	currentState.currentSoftUPStream.streamStride = 0;
	currentState.currentSoftUPIndexBuffer->SoftUPResetInternalPointer();

	// Following any IDirect3DDevice9::DrawIndexedPrimitiveUP call, the stream 0 settings, referenced by IDirect3DDevice9::GetStreamSource, are set to NULL.
	// Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb174370(v=vs.85).aspx
	if (currentState.currentStreams[0].vertexBuffer != NULL)
	{
		SetStreamSource(0, NULL, 0, 0);
	}

	// Also, the index buffer setting for IDirect3DDevice9::SetIndices is set to NULL.
	// Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb174370(v=vs.85).aspx
	if (currentState.currentIndexBuffer != NULL)
	{
		SetIndices(NULL);
	}

	return ret;
}

const bool IDirect3DDevice9Hook::ShouldCullEntireTriangle(const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2)
{
	const unsigned short combinedOutClipCodes = v0.vertexClip.clipCodesCombined & v1.vertexClip.clipCodesCombined & v2.vertexClip.clipCodesCombined;
	return combinedOutClipCodes != 0x0000;
}

const bool IDirect3DDevice9Hook::ShouldCullEntireLine(const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1)
{
	const unsigned short combinedOutClipCodes = v0.vertexClip.clipCodesCombined & v1.vertexClip.clipCodesCombined;
	return combinedOutClipCodes != 0x0000;
}

const bool IDirect3DDevice9Hook::ShouldCullEntirePoint(const VS_2_0_OutputRegisters& v0)
{
	return v0.vertexClip.clipCodesCombined != 0x0000;
}

template <const bool useIndexBuffer, typename indexFormat, const bool rasterizerUsesEarlyZTest>
void IDirect3DDevice9Hook::DrawPrimitiveUBPretransformedSkipVS(const D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT startIndex, UINT primCount) const
{
#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}

	if (primCount == 0)
	{
		DbgBreakPrint("Error: Can't render with 0 primitives!");
	}
#endif

	const float fWidth = currentState.cachedViewport.fWidth;
	const float fHeight = currentState.cachedViewport.fHeight;

	DeclarationSemanticMapping vertexDeclMapping;
	vertexDeclMapping.ClearSemanticMapping();
	vertexDeclMapping.ComputeMappingPS(currentState.currentVertexDecl, currentState.currentPixelShader);
	InitPixelShader(currentState, currentState.currentPixelShader->GetShaderInfo() );

	SetupCurrentDrawCallPixelData(false, &vertexDeclMapping);

	const DebuggableD3DVERTEXELEMENT9* const positionT0 = currentState.currentVertexDecl->GetPositionT0Element();
	if (!positionT0)
	{
#ifdef _DEBUG
		__debugbreak(); // We should never be here...
#endif
	}
	const BYTE* const positionT0stream = currentState.currentStreams[positionT0->Stream].vertexBuffer->GetInternalDataBuffer() + currentState.currentStreams[positionT0->Stream].streamOffset + positionT0->Offset;
	const unsigned short positionT0streamStride = currentState.currentStreams[positionT0->Stream].streamStride;

	const indexFormat* indices = NULL;
	if (useIndexBuffer)
		indices = ( (const indexFormat* const)currentState.currentIndexBuffer->GetBufferBytes() ) + startIndex;

	switch (PrimitiveType)
	{
	case D3DPT_TRIANGLELIST:
	{
		for (unsigned primitiveID = 0; primitiveID < primCount; ++primitiveID)
		{
			const indexFormat i0 = useIndexBuffer ? (indices[primitiveID * 3] + BaseVertexIndex) : (primitiveID * 3);
			const indexFormat i1 = useIndexBuffer ? (indices[primitiveID * 3 + 1] + BaseVertexIndex) : (primitiveID * 3 + 1);
			const indexFormat i2 = useIndexBuffer ? (indices[primitiveID * 3 + 2] + BaseVertexIndex) : (primitiveID * 3 + 2);

			// Skip degenerate triangles
			if (useIndexBuffer)
				if (i0 == i1 || i1 == i2 || i0 == i2)
					continue;

			const BYTE* const v0 = positionT0stream + i0 * positionT0streamStride;
			const BYTE* const v1 = positionT0stream + i1 * positionT0streamStride;
			const BYTE* const v2 = positionT0stream + i2 * positionT0streamStride;
			switch (currentState.currentRenderStates.renderStatesUnion.namedStates.cullmode)
			{
			case D3DCULL_CCW:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				break;
			case D3DCULL_CW:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID, i0, i2, i1);
				break;
			case D3DCULL_NONE:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID + primCount /*hack*/, i0, i2, i1);
				break;
			}
		}
	}
		break;
	case D3DPT_TRIANGLESTRIP:
	{
		const unsigned vertCount = primCount + 2;
		for (unsigned x = 2; x < vertCount; ++x)
		{
			const UINT primitiveID = x - 2;

			const indexFormat i2 = useIndexBuffer ? (indices[x] + BaseVertexIndex) : (primitiveID + 2);
			const indexFormat i1 = useIndexBuffer ? (indices[x - 1] + BaseVertexIndex) : (primitiveID + 1);
			const indexFormat i0 = useIndexBuffer ? (indices[primitiveID] + BaseVertexIndex) : (primitiveID);

			// Skip degenerate triangles
			if (useIndexBuffer)
				if (i0 == i1 || i1 == i2 || i0 == i2)
					continue;

			const BYTE* v2 = positionT0stream + i2 * positionT0streamStride;
			const BYTE* v1 = positionT0stream + i1 * positionT0streamStride;
			const BYTE* const v0 = positionT0stream + i0 * positionT0streamStride;
			if (x & 0x1)
				SwapWithCopy(v1, v2); // This alternating winding is necessary to keep triangles from flipping CW to CCW and back on every other triangle in the strip: https://msdn.microsoft.com/en-us/library/windows/desktop/bb206274(v=vs.85).aspx
			switch (currentState.currentRenderStates.renderStatesUnion.namedStates.cullmode)
			{
			case D3DCULL_CCW:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				break;
			case D3DCULL_CW:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID, i0, i2, i1);
				break;
			case D3DCULL_NONE:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID + primCount /*hack*/, i0, i2, i1);
				break;
			}
		}
	}
		break;
	case D3DPT_TRIANGLEFAN:
	{
		const unsigned vertCount = primCount + 2;
		for (unsigned x = 2; x < vertCount; ++x)
		{
			const UINT primitiveID = x - 2;

			const indexFormat i2 = useIndexBuffer ? (indices[0] + BaseVertexIndex) : (0);
			const indexFormat i1 = useIndexBuffer ? (indices[x] + BaseVertexIndex) : (x);
			const indexFormat i0 = useIndexBuffer ? (indices[x - 1] + BaseVertexIndex) : (x - 1);

			// Skip degenerate triangles
			if (useIndexBuffer)
				if (i0 == i1 || i1 == i2 || i0 == i2)
					continue;

			const BYTE* const v0 = positionT0stream + i0 * positionT0streamStride;
			const BYTE* const v1 = positionT0stream + i1 * positionT0streamStride;
			const BYTE* const v2 = positionT0stream + i2 * positionT0streamStride;

			switch (currentState.currentRenderStates.renderStatesUnion.namedStates.cullmode)
			{
			case D3DCULL_CCW:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				break;
			case D3DCULL_CW:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID, i0, i2, i1);
				break;
			case D3DCULL_NONE:
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangleFromStream<rasterizerUsesEarlyZTest>(vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID + primCount /*TODO: Hack*/, i0, i2, i1);
				break;
			}
		}
	}
		break;
	default:
		// Not yet supported!
		DbgBreakPrint("Error: Not yet supported: Only triangles are currently supported for DrawPrimitiveUBPretransformedSkipVS");
		break;
	}

#ifdef MULTITHREAD_SHADING
	SynchronizeThreads();
#endif
}

template <const bool rasterizerUsesEarlyZTest>
void IDirect3DDevice9Hook::DrawPrimitiveUB(const D3DPRIMITIVETYPE PrimitiveType, const UINT PrimitiveCount, const std::vector<VS_2_0_OutputRegisters>& processedVerts) const
{
#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}

	if (processedVerts.empty() )
	{
		__debugbreak();
	}

	if (PrimitiveCount == 0)
	{
		DbgBreakPrint("Error: Can't render with 0 primitives!");
	}
#endif

	const VS_2_0_OutputRegisters* const processedVertsBuffer = &processedVerts.front();

	VStoPSMapping vStoPSMapping;
	vStoPSMapping.ClearSemanticMapping();
	vStoPSMapping.ComputeMappingVSToPS(currentState.currentVertexShader, currentState.currentPixelShader);

	InitPixelShader(currentState, currentState.currentPixelShader->GetShaderInfo() );
	SetupCurrentDrawCallPixelData(true, &vStoPSMapping);

	const float fWidth = currentState.cachedViewport.fWidth;
	const float fHeight = currentState.cachedViewport.fHeight;

	switch (PrimitiveType)
	{
	case D3DPT_TRIANGLELIST:
	{
#ifdef _DEBUG
		const unsigned vertCount = PrimitiveCount * 3;
		if (vertCount > processedVerts.size() )
		{
			__debugbreak();
		}
#endif
		for (unsigned x = 0; x < PrimitiveCount; ++x)
		{
			const unsigned i0 = x * 3;
			const unsigned i1 = i0 + 1;
			const unsigned i2 = i0 + 2;

			// Skip degenerate triangles
			if (i0 == i1 || i1 == i2 || i0 == i2)
				continue;

			if (ShouldCullEntireTriangle(processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2]) )
				continue;

			switch (currentState.currentRenderStates.renderStatesUnion.namedStates.cullmode)
			{
			case D3DCULL_CCW:
				RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2], fWidth, fHeight, x, i0, i1, i2);
				break;
			case D3DCULL_CW:
				RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i2], processedVertsBuffer[i1], fWidth, fHeight, x, i0, i2, i1);
				break;
			case D3DCULL_NONE:
				RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2], fWidth, fHeight, x, i0, i1, i2);
				RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i2], processedVertsBuffer[i1], fWidth, fHeight, x + PrimitiveCount /*TODO: Hack*/, i0, i2, i1);
				break;
			}
		}
	}
		break;
	case D3DPT_TRIANGLESTRIP:
	{
		const unsigned vertCount = PrimitiveCount + 2;
#ifdef _DEBUG
		if (vertCount > processedVerts.size() )
		{
			__debugbreak();
		}
#endif
		for (unsigned x = 2; x < vertCount; ++x)
		{
			const UINT primitiveID = x - 2;

			const unsigned i2 = x;
			const unsigned i1 = x - 1;
			const unsigned i0 = primitiveID;

			// Skip degenerate triangles
			if (i0 == i1 || i1 == i2 || i0 == i2)
				continue;

			if (ShouldCullEntireTriangle(processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2]) )
				continue;

			if (x & 0x1) // This alternating winding is necessary to keep triangles from flipping CW to CCW and back on every other triangle in the strip: https://msdn.microsoft.com/en-us/library/windows/desktop/bb206274(v=vs.85).aspx
			{
				switch (currentState.currentRenderStates.renderStatesUnion.namedStates.cullmode)
				{
				case D3DCULL_CCW:
					RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i2], processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
					break;
				case D3DCULL_CW:
					RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
					break;
				case D3DCULL_NONE:
					RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i2], processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
					RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2], fWidth, fHeight, primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i1, i2);
					break;
				}
			}
			else
			{
				switch (currentState.currentRenderStates.renderStatesUnion.namedStates.cullmode)
				{
				case D3DCULL_CCW:
					RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
					break;
				case D3DCULL_CW:
					RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i2], processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
					break;
				case D3DCULL_NONE:
					RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
					RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i2], processedVertsBuffer[i1], fWidth, fHeight, primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i2, i1);
					break;
				}
			}
		}
	}
		break;
	case D3DPT_TRIANGLEFAN:
	{
		const unsigned vertCount = PrimitiveCount + 2;
#ifdef _DEBUG
		if (vertCount > processedVerts.size() )
		{
			__debugbreak();
		}
#endif
		for (unsigned x = 2; x < vertCount; ++x)
		{
			const UINT primitiveID = x - 2;

			const unsigned i2 = 0;
			const unsigned i1 = x;
			const unsigned i0 = x - 1;

			// Skip degenerate triangles
			if (i0 == i1 || i1 == i2 || i0 == i2)
				continue;

			if (ShouldCullEntireTriangle(processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2]) )
				continue;

			switch (currentState.currentRenderStates.renderStatesUnion.namedStates.cullmode)
			{
			case D3DCULL_CCW:
				RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
				break;
			case D3DCULL_CW:
				RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i2], processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
				break;
			case D3DCULL_NONE:
				RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1], processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangleFromShader<rasterizerUsesEarlyZTest>(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i2], processedVertsBuffer[i1], fWidth, fHeight, primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i2, i1);
				break;
			}
		}
	}
		break;
	case D3DPT_LINELIST:
	{
#ifdef _DEBUG
		const unsigned vertCount = PrimitiveCount * 2;
		if (vertCount > processedVerts.size() )
		{
			__debugbreak();
		}
#endif
		for (unsigned x = 0; x < PrimitiveCount; ++x)
		{
			const unsigned i0 = x * 2;
			const unsigned i1 = i0 + 1;

			RasterizeLineFromShader(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1]);
		}
	}
		break;
	case D3DPT_LINESTRIP:
	{
		const unsigned vertCount = PrimitiveCount + 1;
#ifdef _DEBUG
		if (vertCount > processedVerts.size() )
		{
			__debugbreak();
		}
#endif

		for (unsigned x = 1; x < vertCount; ++x)
		{
			const unsigned i1 = x;
			const unsigned i0 = x - 1;

			RasterizeLineFromShader(vStoPSMapping, processedVertsBuffer[i0], processedVertsBuffer[i1]);
		}
	}
		break;
	case D3DPT_POINTLIST:
	{
#ifdef _DEBUG
		const unsigned vertCount = PrimitiveCount;
		if (vertCount > processedVerts.size() )
		{
			__debugbreak();
		}
#endif

		for (unsigned x = 0; x < PrimitiveCount; ++x)
		{
			RasterizePointFromShader(vStoPSMapping, processedVertsBuffer[x]);
		}
	}
		break;
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
		break;
	}

#ifdef MULTITHREAD_SHADING
	//CloseThreadpoolCleanupGroupMembers(cleanup, FALSE, NULL);
	//RefreshThreadpoolWork();
	SynchronizeThreads();
#endif
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::ProcessVertices(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags)
{
#ifdef _DEBUG
	// Don't support this?
	DbgBreakPrint("Error: ProcessVertices() is not yet supported");
#endif
	IDirect3DVertexBuffer9Hook* hookPtr = dynamic_cast<IDirect3DVertexBuffer9Hook*>(pDestBuffer);
#ifdef _DEBUG
	if (hookPtr)
#endif
		pDestBuffer = hookPtr->GetUnderlyingVertexBuffer();
#ifdef _DEBUG
	else if (pDestBuffer != NULL)
	{
		DbgBreakPrint("Error: Destination vertex buffer is not hooked!");
	}
#endif
	HRESULT ret = d3d9dev->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DrawRectPatch(THIS_ UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
	// Nope! Not gonna support tessellation in D3D9
	DbgBreakPrint("Error: Tessellation is not yet supported");

	HRESULT ret = d3d9dev->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DrawTriPatch(THIS_ UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
	// Nope! Not gonna support tessellation in D3D9
	DbgBreakPrint("Error: Tessellation is not yet supported");

	HRESULT ret = d3d9dev->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DeletePatch(THIS_ UINT Handle)
{
	// Nope! Not gonna support tessellation in D3D9
	DbgBreakPrint("Error: Tessellation is not yet supported");

	HRESULT ret = d3d9dev->DeletePatch(Handle);
	return ret;
}

const bool IDirect3DDevice9Hook::SkipVertexProcessing(void) const
{
	if (IsUsingFVF() )
	{
		return currentState.currentFVF.namedFVF.positionTypeLow == dbgD3DFVF_XYZRHW;
	}
	else
	{
		if (!currentState.currentVertexDecl)
			return true;
		return currentState.currentVertexDecl->SkipVertexProcessing();
	}
}

void IDirect3DDevice9Hook::InitializeState(const D3DPRESENT_PARAMETERS& d3dpp, const D3DDEVTYPE devType, const DWORD createFlags, const HWND focusWindow)
{
	// Init the viewport:
	currentState.cachedViewport.viewport.X = currentState.cachedViewport.viewport.Y = 0;
	currentState.cachedViewport.viewport.MinZ = 0.0f;
	currentState.cachedViewport.viewport.MaxZ = 1.0f;
	currentState.cachedViewport.viewport.Width = d3dpp.BackBufferWidth;
	currentState.cachedViewport.viewport.Height = d3dpp.BackBufferHeight;
	currentState.cachedViewport.RecomputeCache();

	// Init the scissor rect:
	currentState.currentScissorRect.scissorRect.left = currentState.currentScissorRect.scissorRect.top = 0;
	currentState.currentScissorRect.scissorRect.right = d3dpp.BackBufferWidth;
	currentState.currentScissorRect.scissorRect.bottom = d3dpp.BackBufferHeight;
	currentState.currentScissorRect.RecomputeScissorRect();

	// Very important to set these initial flags for the device:
	initialDevType = devType;
	initialCreateFlags = createFlags;
	initialCreateFocusWindow = focusWindow;
	initialCreateDeviceWindow = d3dpp.hDeviceWindow;

	// Mixed-mode vertex processing starts in hardware mode by default
	if (initialCreateFlags & (D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_HARDWARE_VERTEXPROCESSING) )
		currentState.currentSwvpEnabled = FALSE;
	else
		currentState.currentSwvpEnabled = TRUE;

	// Reset the implicit swap chain for this device:
	delete implicitSwapChain;
	implicitSwapChain = NULL;

	// Clear and then init the shader engines:
	deviceMainVShaderEngine.GlobalInit(&vsDrawCallCB);
	deviceMainPShaderEngine.GlobalInit(&psDrawCallCB);

#ifdef DUMP_TEXTURES_ON_FIRST_SET
	IDirect3DSurface9Hook::InitDumpSurfaces();
#endif

	LPDIRECT3DSWAPCHAIN9 realSwapChain = NULL;
#ifdef _DEBUG
	HRESULT swapChainHR = 
#endif
		d3d9dev->GetSwapChain(0, &realSwapChain);
#ifdef _DEBUG
	if (FAILED(swapChainHR) || !realSwapChain)
	{
		DbgBreakPrint("Error: Failed to retrieve real (unhooked) implicit swap chain");
	}
#endif

	LPDIRECT3DSURFACE9 realBackBuffer = NULL;
#ifdef _DEBUG
	HRESULT backBufferHR = 
#endif
		d3d9dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &realBackBuffer);
#ifdef _DEBUG
	if (FAILED(backBufferHR) || !realBackBuffer)
	{
		DbgBreakPrint("Error: Failed to retrieve real (unhooked) implicit back buffer");
	}
#endif

	IDirect3DSurface9Hook* backbufferSurfaceHook = new IDirect3DSurface9Hook(realBackBuffer, this);
	backbufferSurfaceHook->CreateDeviceImplicitSurface(d3dpp);
	backbufferSurfaceHook->AddRef();

	implicitSwapChain = new IDirect3DSwapChain9Hook(realSwapChain, this);
	implicitSwapChain->InitializeSwapChain(d3dpp, backbufferSurfaceHook);
	implicitSwapChain->AddRef();

	// Initialize all of our render targets:
	currentState.currentRenderTargets[0] = backbufferSurfaceHook;
	for (unsigned x = 1; x < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++x)
		currentState.currentRenderTargets[x] = NULL;

	if (d3dpp.EnableAutoDepthStencil)
	{
		if (d3dpp.AutoDepthStencilFormat != D3DFMT_UNKNOWN)
		{
			LPDIRECT3DSURFACE9 realAutoDepthStencil = NULL;
#ifdef _DEBUG
			HRESULT getDepthStencilHR = 
#endif
			d3d9dev->GetDepthStencilSurface(&realAutoDepthStencil);
#ifdef _DEBUG
			if (FAILED(getDepthStencilHR) || !realAutoDepthStencil)
			{
				DbgBreakPrint("Error: Failed to retrieve real (unhooked) implicit depth-stencil surface");
			}
#endif
			IDirect3DSurface9Hook* autoDepthStencilSurfaceHook = new IDirect3DSurface9Hook(realAutoDepthStencil, this);
			autoDepthStencilSurfaceHook->CreateDeviceImplicitDepthStencil(d3dpp);
			
			currentState.currentDepthStencil = autoDepthStencilSurfaceHook;
		}
	}
	else
	{
		// The default value for this render state is D3DZB_TRUE if a depth stencil was created along with the swap chain by setting the EnableAutoDepthStencil member of the D3DPRESENT_PARAMETERS structure to TRUE, and D3DZB_FALSE otherwise.
		// Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb172599(v=vs.85).aspx
		currentState.currentRenderStates.renderStatesUnion.namedStates.zEnable = D3DZB_FALSE;
	}

#ifdef MULTITHREAD_SHADING
	MAX_NUM_JOBS = d3dpp.BackBufferWidth * d3dpp.BackBufferHeight * NUM_JOBS_PER_PIXEL;
	if (MAX_NUM_JOBS < 1024 * 1024)
		MAX_NUM_JOBS = 1024 * 1024; // We need to be able to shade 1024x1024 vertices in one draw call
	allWorkItems = (slist_item*)malloc(sizeof(slist_item) * MAX_NUM_JOBS);
	if (!allWorkItems)
	{
		__debugbreak();
	}

	// This is needed to init the threads to the proper state:
	SynchronizeThreads();
#endif // MULTITHREAD_SHADING

	if (hConsoleHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hConsoleHandle);
		hConsoleHandle = INVALID_HANDLE_VALUE;
	}
	hConsoleHandle = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (overlayFontTexture != NULL)
	{
		overlayFontTexture->Release();
		overlayFontTexture = NULL;
	}

#pragma warning(push)
#pragma warning(disable:4302) // warning C4302: 'type cast': truncation from 'LPSTR' to 'WORD'
	HRSRC bitmapResource = FindResourceA(hLThisDLL, MAKEINTRESOURCEA(IDB_PNG1), "PNG");
#pragma warning(pop)
	if (bitmapResource)
	{
		HGLOBAL loadedResource = LoadResource(hLThisDLL, bitmapResource);
		if (loadedResource)
		{
			const unsigned resourceSize = SizeofResource(hLThisDLL, bitmapResource);
			if (resourceSize > 0)
			{
				const void* const resourceBytes = LockResource(loadedResource);
				if (resourceBytes)
				{
					LPDIRECT3DTEXTURE9 tempRealTexture = NULL;
					if (SUCCEEDED(D3DXCreateTextureFromFileInMemory(this, resourceBytes, resourceSize, &tempRealTexture) ) )
					{
						D3DSURFACE_DESC surfDesc = {};
						tempRealTexture->GetLevelDesc(0, &surfDesc);
						IDirect3DTexture9Hook* hookRet = new IDirect3DTexture9Hook(tempRealTexture, this);
						hookRet->CreateTexture(surfDesc.Width, surfDesc.Height, tempRealTexture->GetLevelCount(), (const DebuggableUsage)0, surfDesc.Format, D3DPOOL_MANAGED);
						overlayFontTexture = hookRet;
					}
				}
			}
		}
	}
}

IDirect3DDevice9Hook::IDirect3DDevice9Hook(LPDIRECT3DDEVICE9 _d3d9dev, IDirect3D9Hook* _parentHook) : d3d9dev(_d3d9dev), parentHook(_parentHook), refCount(1), initialDevType(D3DDEVTYPE_HAL), initialCreateFlags(D3DCREATE_HARDWARE_VERTEXPROCESSING),
	enableDialogs(FALSE), sceneBegun(FALSE), implicitSwapChain(NULL), hConsoleHandle(INVALID_HANDLE_VALUE), overlayFontTexture(NULL), numPixelsPassedZTest(0), initialCreateFocusWindow(NULL), initialCreateDeviceWindow(NULL)
{
#ifdef _DEBUG
	m_FirstMember = false;
#endif

	memset(&deviceCS, 0, sizeof(deviceCS) );
	InitializeCriticalSection(&deviceCS);

#ifdef _DEBUG
	memcpy(&CreationParameters, &d3d9dev->CreationParameters, (char*)&m_FirstMember - (char*)&CreationParameters);
#endif

#ifdef MULTITHREAD_SHADING
	InitThreadQueue();
#endif

#ifndef NO_CACHING_FVF_VERT_DECLS
	FVFToVertDeclCache = new std::map<DWORD, IDirect3DVertexDeclaration9Hook*>;
#endif

	currentState.currentSoftUPStream.vertexBuffer = new IDirect3DVertexBuffer9Hook(NULL, this);
	currentState.currentSoftUPStream.vertexBuffer->MarkSoftBufferUP(true);

	currentState.currentSoftUPIndexBuffer = new IDirect3DIndexBuffer9Hook(NULL, this);
	currentState.currentSoftUPIndexBuffer->MarkSoftBufferUP(true);

	processedVertexBuffer = new std::vector<VS_2_0_OutputRegisters>;
}

/*virtual*/ IDirect3DDevice9Hook::~IDirect3DDevice9Hook()
{
	if (overlayFontTexture != NULL)
	{
		overlayFontTexture->Release();
		overlayFontTexture = NULL;
	}

	delete currentState.currentSoftUPStream.vertexBuffer;
	currentState.currentSoftUPStream.vertexBuffer = NULL;
	delete currentState.currentSoftUPIndexBuffer;
	currentState.currentSoftUPIndexBuffer = NULL;

#ifndef NO_CACHING_FVF_VERT_DECLS
	delete FVFToVertDeclCache;
	FVFToVertDeclCache = NULL;
#endif

	delete processedVertexBuffer;
	processedVertexBuffer = NULL;

	DeleteCriticalSection(&deviceCS);
	memset(&deviceCS, 0, sizeof(deviceCS) );

#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
	memset(this, 0x00000000, sizeof(*this) );
#endif
}

// Returns true if the current vertex FVF or decl has a COLOR0 component, or false otherwise
const bool DeviceState::CurrentStateHasInputVertexColor0(void) const
{
	switch (declTarget)
	{
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Should never be here!");
#else
		__assume(0);
#endif
	case targetFVF:
		return currentFVF.namedFVF.hasDiffuse; // Diffuse color is always COLOR0
	case targetVertexDecl:
		if (currentVertexDecl)
			return currentVertexDecl->GetHasCOLOR0();
		return false;
	}
}

// Returns true if the current vertex FVF or decl has a COLOR1 component, or false otherwise
const bool DeviceState::CurrentStateHasInputVertexColor1(void) const
{
	switch (declTarget)
	{
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Should never be here!");
#else
		__assume(0);
#endif
	case targetFVF:
		return currentFVF.namedFVF.hasSpecular; // Specular color is always COLOR1
	case targetVertexDecl:
		if (currentVertexDecl)
			return currentVertexDecl->GetHasCOLOR1();
		return false;
	}
}

void IDirect3DDevice9Hook::ModifyPresentParameters(D3DPRESENT_PARAMETERS& modifiedParams)
{
	modifiedParams.Windowed = TRUE; // Force Windowed mode
	modifiedParams.FullScreen_RefreshRateInHz = 0;
	//modifiedParams.BackBufferFormat = D3DFMT_UNKNOWN;
	modifiedParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; // For performance, you're gonna want this enabled
//#ifdef _DEBUG
	modifiedParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	modifiedParams.MultiSampleQuality = 0;
	modifiedParams.Flags &= (~D3DPRESENTFLAG_DEVICECLIP); // Remove the DEVICECLIP flag so that there aren't weird artifacts when rendering into a window that spans multiple monitors
//#endif
}
