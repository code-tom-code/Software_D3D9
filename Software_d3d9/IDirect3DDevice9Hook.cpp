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
#include "DitherTables.h"

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
static const unsigned SUBPIXEL_ACCURACY_BIASMULT2 = 2 * SUBPIXEL_ACCURACY_BIASMULT;
static const int SUBPIXEL_MAX_VALUE = MAXINT >> SUBPIXEL_ACCURACY_BITS;
static const int SUBPIXEL_MIN_VALUE = MININT >> SUBPIXEL_ACCURACY_BITS;
static const float SUBPIXEL_MAX_VALUEF = 8191.0f;//(const float)SUBPIXEL_MAX_VALUE;
static const float SUBPIXEL_MIN_VALUEF = -8191.0f;//(const float)SUBPIXEL_MIN_VALUE;
static const float SUBPIXEL_ACCURACY_BIASMULTF = (const float)SUBPIXEL_ACCURACY_BIASMULT;
static const __m128 SUBPIXEL_ACCURACY_BIASMULT_SPLATTEDF = { SUBPIXEL_ACCURACY_BIASMULTF, SUBPIXEL_ACCURACY_BIASMULTF, SUBPIXEL_ACCURACY_BIASMULTF, SUBPIXEL_ACCURACY_BIASMULTF };
static const D3DXVECTOR4 zeroVec(0.0f, 0.0f, 0.0f, 0.0f);
static const D3DXVECTOR4 vertShaderInputRegisterDefault(0.0f, 0.0f, 0.0f, 1.0f);

static const unsigned twoVecBytes[4] = { 0x2, 0x2, 0x2, 0x2 };
static const __m128i twoVec = *(const __m128i* const)twoVecBytes;
static const unsigned intBoundsQuadAlignVecBytes[4] = { ~0x1, ~0x1, ~0x0, ~0x0 };
static const __m128i intBoundsQuadAlignVec = *(const __m128i* const)intBoundsQuadAlignVecBytes;

static const D3DXVECTOR4 staticColorWhiteOpaque(1.0f, 1.0f, 1.0f, 1.0f);
static const D3DXVECTOR4 staticColorBlackTranslucent(0.0f, 0.0f, 0.0f, 0.0f);
static const __m128 guardBandMin = { -8192.0f, -8192.0f, -8192.0f, -8192.0f };
static const __m128 guardBandMax = { 8192.0f, 8192.0f, 8192.0f, 8192.0f };

static const unsigned sevenVecBytes[4] = { 0x7, 0x7, 0x7, 0x7 };
static const __m128i sevenVec = *(const __m128i* const)sevenVecBytes;
static const unsigned x4quadOffsetBytes[4] = { 0, 1, 0, 1 };
static const __m128i x4quadOffset = *(const __m128i* const)x4quadOffsetBytes;
static const unsigned y4quadOffsetBytes[4] = { 0, 0, 1, 1 };
static const __m128i y4quadOffset = *(const __m128i* const)y4quadOffsetBytes;

static const __m128 zeroMaskVec = { 0.0f, 0.0f, 0.0f, 0.0f };
static const __m128i zeroMaskVecI = { 0 };
static const unsigned oneMaskVecBytes[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
static const __m128i oneMaskVec = *(const __m128i* const)oneMaskVecBytes;
static_assert(sizeof(oneMaskVecBytes) == sizeof(oneMaskVec), "Error! Unexpected vector size");

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
} threadItem [NUM_THREADS * 4 + 1] = {0};

// This is volatile and aligned because it will be used with Interlocked operations
static volatile long __declspec(align(16) ) tlsThreadNumber = 0;

// Threadpool implementation was slower than single-threaded implementation in some cases
//static TP_CALLBACK_ENVIRON mainThreadpoolCallbackEnv = {0};
//static PTP_POOL mainThreadpool = NULL;
//static PTP_CLEANUP_GROUP mainThreadpoolCleanupGroup = NULL;

static_assert(sizeof(_threadItem) > 64, "Error: False sharing may occur if thread struct is smaller than a cache line!");

struct int3
{
	int a, b, c;
};

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
#if TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
		struct _pixelJobData
		{
			const primitivePixelJobData* primitiveData;
			int3 barycentricCoords[4];
			unsigned x[4];
			unsigned y[4];
		} pixelJobData;
#endif // #if TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS

#if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
		struct _triangleRasterizeJobData
		{
			UINT primitiveID;
			UINT vertIndex0, vertIndex1, vertIndex2;
			union _triangleRasterizeVerticesUnion
			{
				struct _triangleRasterizeFromStream
				{
					const D3DXVECTOR4* v0;
					const D3DXVECTOR4* v1;
					const D3DXVECTOR4* v2;
				} triangleRasterizeFromStream;

				struct _triangleRasterizeFromShader
				{
					const VS_2_0_OutputRegisters* v0;
					const VS_2_0_OutputRegisters* v1;
					const VS_2_0_OutputRegisters* v2;
				} triangleRasterizeFromShader;
			} rasterVertices;
		} triangleRasterizeJobData;
#endif // #if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
	} jobData;
};
static_assert(sizeof(slist_item) % 16 == 0, "Error, bad struct alignment!");

static slist_item* allWorkItems = NULL;
static DWORD tlsIndex = TLS_OUT_OF_INDEXES;

static inline void VertexShadeJob1(slist_item& job, _threadItem* const myPtr)
{
	SIMPLE_FUNC_SCOPE();
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
	SIMPLE_FUNC_SCOPE();
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

#if TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
static inline void PixelShadeJob1(slist_item& job, _threadItem* const myPtr)
{
	const IDirect3DDevice9Hook* const devHook = myPtr->devHook;
	const drawCallPixelJobData& drawCallData = devHook->currentDrawCallData.pixelData;
	const slist_item::_jobData::_pixelJobData& pixelJobData = job.jobData.pixelJobData;
	const primitivePixelJobData* const primitiveData = pixelJobData.primitiveData;

	SIMPLE_FUNC_SCOPE_CONDITIONAL(primitiveData->primitiveID == 0 && 
		abs( (const int)(320 - pixelJobData.x[0]) ) < 5 && 
		abs( (const int)(240 - pixelJobData.y[0]) ) < 5);

	const __m128 barycentricNormalizeFactor = _mm_set1_ps(primitiveData->barycentricNormalizeFactor);
	const __m128i barycentricCoordsVector = _mm_load_si128( (const __m128i* const)&pixelJobData.barycentricCoords[0]);
	const __m128 barycentricCoordsVectorF = _mm_mul_ps(_mm_cvtepi32_ps(barycentricCoordsVector), barycentricNormalizeFactor);
	const __m128 invZ = _mm_load_ps( (const float* const)&(primitiveData->invZ) );

	const primitivePixelJobData::_pixelShadeVertexData::_shadeFromAgnostic& verts = primitiveData->pixelShadeVertexData.shadeFromAgnostic;
	if (drawCallData.useShaderVerts)
		devHook->SetupPixel<true>(&myPtr->threadPS_2_0, drawCallData.vs_to_ps_mappings.sourceAgnosticMapping, pixelJobData.x[0], pixelJobData.y[0], 
			barycentricCoordsVectorF, drawCallData.offsetIntoVertexForOPosition_Bytes, verts.v0, verts.v1, verts.v2, invZ);
	else
		devHook->SetupPixel<false>(&myPtr->threadPS_2_0, drawCallData.vs_to_ps_mappings.sourceAgnosticMapping, pixelJobData.x[0], pixelJobData.y[0], 
			barycentricCoordsVectorF, drawCallData.offsetIntoVertexForOPosition_Bytes, verts.v0, verts.v1, verts.v2, invZ);
}

static inline void PixelShadeJob4(slist_item& job, _threadItem* const myPtr)
{
	const IDirect3DDevice9Hook* const devHook = myPtr->devHook;
	const drawCallPixelJobData& drawCallData = devHook->currentDrawCallData.pixelData;
	const slist_item::_jobData::_pixelJobData& pixelJobData = job.jobData.pixelJobData;
	const primitivePixelJobData* const primitiveData = pixelJobData.primitiveData;

	SIMPLE_FUNC_SCOPE_CONDITIONAL(primitiveData->primitiveID == 0 && 
		abs( (const int)(320 - pixelJobData.x[0]) ) < 5 && 
		abs( (const int)(240 - pixelJobData.y[0]) ) < 5);

	const __m128i x4 = *(const __m128i* const)(pixelJobData.x);
	const __m128i y4 = *(const __m128i* const)(pixelJobData.y);

	const __m128 barycentricNormalizeFactor = _mm_set1_ps(primitiveData->barycentricNormalizeFactor);
	const __m128i barycentricCoordIntVal4[4] =
	{
		*(const __m128i* const)(&pixelJobData.barycentricCoords[0]),
		*(const __m128i* const)(&pixelJobData.barycentricCoords[1]),
		*(const __m128i* const)(&pixelJobData.barycentricCoords[2]),
		*(const __m128i* const)(&pixelJobData.barycentricCoords[3])
	};
	const __m128 barycentricCoords4[4] =
	{
		_mm_mul_ps(_mm_cvtepi32_ps(barycentricCoordIntVal4[0]), barycentricNormalizeFactor),
		_mm_mul_ps(_mm_cvtepi32_ps(barycentricCoordIntVal4[1]), barycentricNormalizeFactor),
		_mm_mul_ps(_mm_cvtepi32_ps(barycentricCoordIntVal4[2]), barycentricNormalizeFactor),
		_mm_mul_ps(_mm_cvtepi32_ps(barycentricCoordIntVal4[3]), barycentricNormalizeFactor)
	};

	const __m128 invZ = _mm_load_ps( (const float* const)&(primitiveData->invZ) );

	const primitivePixelJobData::_pixelShadeVertexData::_shadeFromAgnostic& verts = primitiveData->pixelShadeVertexData.shadeFromAgnostic;
	if (drawCallData.useShaderVerts)
		devHook->SetupPixel4<true>(&myPtr->threadPS_2_0, drawCallData.vs_to_ps_mappings.sourceAgnosticMapping, x4, y4, barycentricCoords4, 
			drawCallData.offsetIntoVertexForOPosition_Bytes, verts.v0, verts.v1, verts.v2, invZ);
	else
		devHook->SetupPixel4<false>(&myPtr->threadPS_2_0, drawCallData.vs_to_ps_mappings.sourceAgnosticMapping, x4, y4, barycentricCoords4, 
			drawCallData.offsetIntoVertexForOPosition_Bytes, verts.v0, verts.v1, verts.v2, invZ);
}
#endif // #if TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS

#if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
static inline void TriangleRasterJob1(slist_item& job, _threadItem* const myPtr)
{
	const IDirect3DDevice9Hook* const devHook = myPtr->devHook;
	const drawCallTriangleRasterizeJobsData& drawCallData = devHook->currentDrawCallData.triangleRasterizeData;
	const slist_item::_jobData::_triangleRasterizeJobData& triangleRasterizeJobData = job.jobData.triangleRasterizeJobData;

	if (drawCallData.rasterizeTriangleFromShader)
	{
		const slist_item::_jobData::_triangleRasterizeJobData::_triangleRasterizeVerticesUnion::_triangleRasterizeFromShader& rasterFromShader = triangleRasterizeJobData.rasterVertices.triangleRasterizeFromShader;
		if (drawCallData.rasterizerUsesEarlyZTest)
			devHook->RasterizeTriangle<true, true>(&myPtr->threadPS_2_0, drawCallData.vStoPSMapping, rasterFromShader.v0, rasterFromShader.v1, rasterFromShader.v2,
				drawCallData.fWidth, drawCallData.fHeight, triangleRasterizeJobData.primitiveID, triangleRasterizeJobData.vertIndex0, triangleRasterizeJobData.vertIndex1, triangleRasterizeJobData.vertIndex2);
		else
			devHook->RasterizeTriangle<false, true>(&myPtr->threadPS_2_0, drawCallData.vStoPSMapping, rasterFromShader.v0, rasterFromShader.v1, rasterFromShader.v2,
				drawCallData.fWidth, drawCallData.fHeight, triangleRasterizeJobData.primitiveID, triangleRasterizeJobData.vertIndex0, triangleRasterizeJobData.vertIndex1, triangleRasterizeJobData.vertIndex2);
	}
	else
	{
		const slist_item::_jobData::_triangleRasterizeJobData::_triangleRasterizeVerticesUnion::_triangleRasterizeFromStream& rasterFromStream = triangleRasterizeJobData.rasterVertices.triangleRasterizeFromStream;
		if (drawCallData.rasterizerUsesEarlyZTest)
			devHook->RasterizeTriangle<true, false>(&myPtr->threadPS_2_0, drawCallData.vertexDeclMapping, rasterFromStream.v0, rasterFromStream.v1, rasterFromStream.v2,
				drawCallData.fWidth, drawCallData.fHeight, triangleRasterizeJobData.primitiveID, triangleRasterizeJobData.vertIndex0, triangleRasterizeJobData.vertIndex1, triangleRasterizeJobData.vertIndex2);
		else
			devHook->RasterizeTriangle<false, false>(&myPtr->threadPS_2_0, drawCallData.vertexDeclMapping, rasterFromStream.v0, rasterFromStream.v1, rasterFromStream.v2,
				drawCallData.fWidth, drawCallData.fHeight, triangleRasterizeJobData.primitiveID, triangleRasterizeJobData.vertIndex0, triangleRasterizeJobData.vertIndex1, triangleRasterizeJobData.vertIndex2);
	}
}
#endif // #if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS

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

#if TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
		case pixelShade1Job:
			PixelShadeJob1(*item, myPtr);
			break;
#ifdef RUN_SHADERS_IN_WARPS
		case pixelShade4Job:
			PixelShadeJob4(*item, myPtr);
			break;
#endif // #ifdef RUN_SHADERS_IN_WARPS

#endif // #if TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS

#if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
		case triangleRasterizeJob:
			TriangleRasterJob1(*item, myPtr);
			break;
#endif // #if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
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

template <const unsigned numJobsToRunSinglethreaded>
static inline void SynchronizeThreads()
{
	// WorkUntilNoMoreWork(&threadItem[NUM_THREADS]);
	//workStatus.numJobs = workStatus.currentWorkID;

	if (workStatus.numJobs == 0)
		return;

	//SetEvent(moreWorkReadyEvent);

	const long cacheNumJobs = workStatus.numJobs;
	if (cacheNumJobs <= numJobsToRunSinglethreaded) // Don't actually use multiple threads for incredibly small jobs, it's just a waste of scheduling time overall and it makes debugging more confusing than it has to be
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

// Returns 0xFF if the pixel "passes" the depth test (should be written) and 0x00 if the pixel "fails" the depth test (should be discarded)
static const __m128 pixelDepthScaleD15 = { 32768.0f, 32768.0f, 32768.0f, 32768.0f };
static const __m128 pixelDepthScaleD16 = { 65536.0f, 65536.0f, 65536.0f, 65536.0f };
static const __m128 pixelDepthScaleD24 = { 16777216.0f, 16777216.0f, 16777216.0f, 16777216.0f };
static const __m128 pixelDepthScaleD32 = { 4294967296.0f, 4294967296.0f, 4294967296.0f, 4294967296.0f };
static inline const __m128i DepthTest4(const __m128 pixelDepth4, const __m128i bufferDepth4, const D3DCMPFUNC comparison, const D3DFORMAT depthFmt)
{
	__m128i quantizedPixelDepth4;
	switch (depthFmt)
	{
	case D3DFMT_D15S1:
	{
		const __m128 scaledDepth4 = _mm_mul_ps(pixelDepthScaleD15, pixelDepth4);
		quantizedPixelDepth4 = _mm_cvtps_epi32(scaledDepth4);
	}
		break;
	case D3DFMT_D16:
	case D3DFMT_D16_LOCKABLE:
	{
		const __m128 scaledDepth4 = _mm_mul_ps(pixelDepthScaleD16, pixelDepth4);
		quantizedPixelDepth4 = _mm_cvtps_epi32(scaledDepth4);
	}
		break;
	default:
		DbgBreakPrint("Error: Unknown Depth buffer format!");
	case D3DFMT_D24FS8:
	case D3DFMT_D24S8:
	case D3DFMT_D24X4S4:
	case D3DFMT_D24X8:
	{
		const __m128 scaledDepth4 = _mm_mul_ps(pixelDepthScaleD24, pixelDepth4);
		quantizedPixelDepth4 = _mm_cvtps_epi32(scaledDepth4);
	}
		break;
	case D3DFMT_D32:
	case D3DFMT_D32_LOCKABLE:
	{
		const __m128 scaledDepth4 = _mm_mul_ps(pixelDepthScaleD32, pixelDepth4);
		quantizedPixelDepth4 = _mm_cvtps_epi32(scaledDepth4); // Technically this isn't correct and this should instead be using _mm_cvtps_epu32, but that requires AVX512 for some reason
	}
		break;
	case D3DFMT_D32F_LOCKABLE:
	{
		quantizedPixelDepth4 = *(const __m128i* const)&pixelDepth4;
	}
		break;
	}

	switch (comparison)
	{
	case D3DCMP_NEVER       :
		return zeroMaskVecI;
	case D3DCMP_LESS        :
		return _mm_cmplt_epi32(quantizedPixelDepth4, bufferDepth4);
	case D3DCMP_EQUAL       :
		return _mm_cmpeq_epi32(quantizedPixelDepth4, bufferDepth4);
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Undefined D3DCMP specified for DepthTest");
#endif
	case D3DCMP_LESSEQUAL   : // Implemented as !>
		return _mm_xor_si128(_mm_cmpgt_epi32(quantizedPixelDepth4, bufferDepth4), oneMaskVec);
	case D3DCMP_GREATER     :
		return _mm_cmpgt_epi32(quantizedPixelDepth4, bufferDepth4);
	case D3DCMP_NOTEQUAL    : // Implemented as !=
		return _mm_xor_si128(_mm_cmpeq_epi32(quantizedPixelDepth4, bufferDepth4), oneMaskVec);
	case D3DCMP_GREATEREQUAL: // Implemented as !<
		return _mm_xor_si128(_mm_cmplt_epi32(quantizedPixelDepth4, bufferDepth4), oneMaskVec);
	case D3DCMP_ALWAYS      :
		return oneMaskVec;
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
	depthBiasSplatted = _mm_set1_ps(0.0f);
	alphaRefSplatted = _mm_set1_ps(0.0f);
	simplifiedAlphaBlendMode = noAlphaBlending;
	alphaBlendNeedsDestRead = true;
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
#ifdef OVERRIDE_HIDE_CURSOR
	if (bShow == FALSE)
		return TRUE;
#endif
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
#ifdef _DEBUG
	OutputDebugStringA("IDirect3DDevice9::Reset called!\n");
#endif

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

#ifdef ENABLE_END_TO_SKIP_DRAWS
static DWORD lastCheckedTicks = 0x00000000;
static bool lastCheckedIsHoldingDownEnd = false;

static inline void ResetHoldingDownEndToSkip()
{
	--lastCheckedTicks;
}

// Skip this draw call if END is held down
static inline const bool IsHoldingEndToSkipDrawCalls(void)
{
	// Since GetTickCount() has a 1ms resolution (at best), this limits us to only calling this function at most once per millisecond
	const DWORD currentTime = GetTickCount();
	if (currentTime != lastCheckedTicks)
	{
		if ( (GetAsyncKeyState(VK_END) & 0x8000) )
			lastCheckedIsHoldingDownEnd = true;
		else
			lastCheckedIsHoldingDownEnd = false;

		lastCheckedTicks = currentTime;
	}

	return lastCheckedIsHoldingDownEnd;
}
#endif // #ifdef ENABLE_END_TO_SKIP_DRAWS

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::Present(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	SIMPLE_FUNC_SCOPE();

#ifdef ENABLE_END_TO_SKIP_DRAWS
	ResetHoldingDownEndToSkip();
#endif // ENABLE_END_TO_SKIP_DRAWS

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

	SIMPLE_FRAME_END_MARKER();

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

	// TODO: If Scissor Test is enabled (D3DRS_SCISSORTEST), then clip input rects against scissor rect. Why?
	// Because in D3D9 (but not 10 and up), the Clear() call *is* affected by scissor testing.
	// Source: https://docs.microsoft.com/en-us/windows/desktop/direct3d9/scissor-test

	// TODO: Clip the clear rect against the viewport rect (in the case that the viewport rect is smaller than the render target)

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
		SIMPLE_NAME_SCOPE("Clear Target");

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
		SIMPLE_NAME_SCOPE("Clear Z Buffer");

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
		SIMPLE_NAME_SCOPE("Clear Stencil Buffer");

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
template <const bool canImmediateFlushJobs>
static inline slist_item* const GetNewWorkerJob(void)
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
			SynchronizeThreads<4>();
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

	slist_item* const newItem = GetNewWorkerJob<false>();
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

#if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
void IDirect3DDevice9Hook::CreateNewTriangleRasterJob(const UINT primitiveID, const UINT vertID0, const UINT vertID1, const UINT vertID2, const bool rasterizeFromShader, const void* const vert0, const void* const vert1, const void* const vert2) const
{
	slist_item* const newItem = GetNewWorkerJob<false>();
	newItem->jobType = triangleRasterizeJob;
	slist_item::_jobData::_triangleRasterizeJobData& triangleRasterizeJobData = newItem->jobData.triangleRasterizeJobData;

	triangleRasterizeJobData.primitiveID = primitiveID;
	triangleRasterizeJobData.vertIndex0 = vertID0;
	triangleRasterizeJobData.vertIndex1 = vertID1;
	triangleRasterizeJobData.vertIndex2 = vertID2;

	if (rasterizeFromShader)
	{
		slist_item::_jobData::_triangleRasterizeJobData::_triangleRasterizeVerticesUnion::_triangleRasterizeFromShader& rasterizeFromShaderVerts = triangleRasterizeJobData.rasterVertices.triangleRasterizeFromShader;
		rasterizeFromShaderVerts.v0 = (const VS_2_0_OutputRegisters* const)vert0;
		rasterizeFromShaderVerts.v1 = (const VS_2_0_OutputRegisters* const)vert1;
		rasterizeFromShaderVerts.v2 = (const VS_2_0_OutputRegisters* const)vert2;
	}
	else
	{
		slist_item::_jobData::_triangleRasterizeJobData::_triangleRasterizeVerticesUnion::_triangleRasterizeFromStream& rasterizeFromStreamVerts = triangleRasterizeJobData.rasterVertices.triangleRasterizeFromStream;
		rasterizeFromStreamVerts.v0 = (const D3DXVECTOR4* const)vert0;
		rasterizeFromStreamVerts.v1 = (const D3DXVECTOR4* const)vert1;
		rasterizeFromStreamVerts.v2 = (const D3DXVECTOR4* const)vert2;
	}

	++workStatus.numJobs;
}
#endif // #if TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS

#endif // #ifdef MULTITHREAD_SHADING

void IDirect3DDevice9Hook::CreateNewPixelShadeJob(const unsigned x, const unsigned y, const __m128i barycentricAdjusted, const primitivePixelJobData* const primitiveData) const
{
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
	slist_item* const newItem = GetNewWorkerJob<true>();
	newItem->jobType = pixelShade1Job;
	slist_item::_jobData::_pixelJobData& pixelJobData = newItem->jobData.pixelJobData;
#ifdef _DEBUG
	if (!primitiveData)
	{
		__debugbreak();
	}
#endif
	pixelJobData.primitiveData = primitiveData;
	pixelJobData.x[0] = x;
	pixelJobData.y[0] = y;
	pixelJobData.barycentricCoords[0].a = barycentricAdjusted.m128i_i32[0];
	pixelJobData.barycentricCoords[0].b = barycentricAdjusted.m128i_i32[1];
	pixelJobData.barycentricCoords[0].c = barycentricAdjusted.m128i_i32[2];

	++workStatus.numJobs;
#else // #if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
	const __m128 barycentricFactors = _mm_mul_ps(_mm_cvtepi32_ps(barycentricAdjusted), _mm_set1_ps(primitiveData->barycentricNormalizeFactor) );
	const __m128 invZ = _mm_load_ps( (const float* const)&(primitiveData->invZ) );

#ifdef PROFILE_AVERAGE_PIXEL_SHADE_TIMES
	LARGE_INTEGER pixelStartTime;
	QueryPerformanceCounter(&pixelStartTime);
#endif // PROFILE_AVERAGE_PIXEL_SHADE_TIMES
	if (currentDrawCallData.pixelData.useShaderVerts)
		SetupPixel<true>(&deviceMainPShaderEngine, currentDrawCallData.pixelData.vs_to_ps_mappings.sourceAgnosticMapping, 
			x, y, barycentricFactors, currentDrawCallData.pixelData.offsetIntoVertexForOPosition_Bytes, 
			primitiveData->pixelShadeVertexData.shadeFromAgnostic.v0, primitiveData->pixelShadeVertexData.shadeFromAgnostic.v1, primitiveData->pixelShadeVertexData.shadeFromAgnostic.v2, invZ);
	else
		SetupPixel<false>(&deviceMainPShaderEngine, currentDrawCallData.pixelData.vs_to_ps_mappings.sourceAgnosticMapping, 
			x, y, barycentricFactors, currentDrawCallData.pixelData.offsetIntoVertexForOPosition_Bytes, 
			primitiveData->pixelShadeVertexData.shadeFromAgnostic.v0, primitiveData->pixelShadeVertexData.shadeFromAgnostic.v1, primitiveData->pixelShadeVertexData.shadeFromAgnostic.v2, invZ);
#ifdef PROFILE_AVERAGE_PIXEL_SHADE_TIMES
	LARGE_INTEGER pixelEndTime;
	QueryPerformanceCounter(&pixelEndTime);

	totalPixelShadeTicks += (pixelEndTime.QuadPart - pixelStartTime.QuadPart);
	++numPixelShadeTasks;
#endif // PROFILE_AVERAGE_PIXEL_SHADE_TIMES

#endif // #if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
}

#ifdef RUN_SHADERS_IN_WARPS
void IDirect3DDevice9Hook::CreateNewPixelShadeJob4(const __m128i x4, const __m128i y4, const __m128i (&barycentricsAdjusted4)[4], const primitivePixelJobData* const primitiveData) const
{
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
	slist_item* const newItem = GetNewWorkerJob<true>();
	newItem->jobType = pixelShade4Job;
	slist_item::_jobData::_pixelJobData& pixelJobData = newItem->jobData.pixelJobData;
#ifdef _DEBUG
	if (!primitiveData)
	{
		__debugbreak();
	}
#endif
	pixelJobData.primitiveData = primitiveData;
	pixelJobData.x[0] = x4.m128i_i32[0];
	pixelJobData.x[1] = x4.m128i_i32[1];
	pixelJobData.x[2] = x4.m128i_i32[0];
	pixelJobData.x[3] = x4.m128i_i32[1];
	pixelJobData.y[0] = y4.m128i_i32[0];
	pixelJobData.y[1] = y4.m128i_i32[0];
	pixelJobData.y[2] = y4.m128i_i32[2];
	pixelJobData.y[3] = y4.m128i_i32[2];
	pixelJobData.barycentricCoords[0].a = barycentricsAdjusted4[0].m128i_i32[0];
	pixelJobData.barycentricCoords[0].b = barycentricsAdjusted4[0].m128i_i32[1];
	pixelJobData.barycentricCoords[0].c = barycentricsAdjusted4[0].m128i_i32[2];
	pixelJobData.barycentricCoords[1].a = barycentricsAdjusted4[1].m128i_i32[0];
	pixelJobData.barycentricCoords[1].b = barycentricsAdjusted4[1].m128i_i32[1];
	pixelJobData.barycentricCoords[1].c = barycentricsAdjusted4[1].m128i_i32[2];
	pixelJobData.barycentricCoords[2].a = barycentricsAdjusted4[2].m128i_i32[0];
	pixelJobData.barycentricCoords[2].b = barycentricsAdjusted4[2].m128i_i32[1];
	pixelJobData.barycentricCoords[2].c = barycentricsAdjusted4[2].m128i_i32[2];
	pixelJobData.barycentricCoords[3].a = barycentricsAdjusted4[3].m128i_i32[0];
	pixelJobData.barycentricCoords[3].b = barycentricsAdjusted4[3].m128i_i32[1];
	pixelJobData.barycentricCoords[3].c = barycentricsAdjusted4[3].m128i_i32[2];

	++workStatus.numJobs;
#else // #if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
	const __m128 barycentricNormalizeFactor = _mm_set1_ps(primitiveData->barycentricNormalizeFactor);
	const __m128 barycentricCoords4[4] =
	{
		_mm_mul_ps(_mm_cvtepi32_ps(barycentricsAdjusted4[0]), barycentricNormalizeFactor),
		_mm_mul_ps(_mm_cvtepi32_ps(barycentricsAdjusted4[1]), barycentricNormalizeFactor),
		_mm_mul_ps(_mm_cvtepi32_ps(barycentricsAdjusted4[2]), barycentricNormalizeFactor),
		_mm_mul_ps(_mm_cvtepi32_ps(barycentricsAdjusted4[3]), barycentricNormalizeFactor)
	};
	const __m128 invZ = _mm_load_ps( (const float* const)&(primitiveData->invZ) );

	const primitivePixelJobData::_pixelShadeVertexData::_shadeFromAgnostic& verts = primitiveData->pixelShadeVertexData.shadeFromAgnostic;
	if (currentDrawCallData.pixelData.useShaderVerts)
		SetupPixel4<true>(&deviceMainPShaderEngine, currentDrawCallData.pixelData.vs_to_ps_mappings.sourceAgnosticMapping, x4, y4, barycentricCoords4, 
			currentDrawCallData.pixelData.offsetIntoVertexForOPosition_Bytes, verts.v0, verts.v1, verts.v2, invZ);
	else
		SetupPixel4<false>(&deviceMainPShaderEngine, currentDrawCallData.pixelData.vs_to_ps_mappings.sourceAgnosticMapping, x4, y4, barycentricCoords4, 
			currentDrawCallData.pixelData.offsetIntoVertexForOPosition_Bytes, verts.v0, verts.v1, verts.v2, invZ);
#endif // #if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
}
#endif // RUN_SHADERS_IN_WARPS

template <const bool shadeFromShader>
const primitivePixelJobData* const IDirect3DDevice9Hook::GetNewPrimitiveJobData(const void* const v0, const void* const v1, const void* const v2, const float barycentricNormalizeFactor, const UINT primitiveID, const bool VFace,
	const UINT vertex0index, const UINT vertex1index, const UINT vertex2index, const __m128 p0, const __m128 p1, const __m128 p2) const
{
#ifdef _DEBUG
	if (primitiveID >= ARRAYSIZE(allPrimitiveJobData) )
	{
		__debugbreak();
	}
#endif

	primitivePixelJobData& newPrimitiveData = allPrimitiveJobData[primitiveID];

	__m128 localInvZ;
	localInvZ.m128_f32[0] = p0.m128_f32[2];
	localInvZ.m128_f32[1] = p1.m128_f32[2];
	localInvZ.m128_f32[2] = p2.m128_f32[2];

	const __m128 zeroVec = _mm_setzero_ps();
	const __m128 localZ = _mm_div_ps(oneVec, localInvZ);
	const __m128 selectMask = _mm_cmpeq_ps(zeroVec, localInvZ); // Members that equal zero will be set to 0xFF, members that don't will be set to 0x00
	const __m128 localInvZSelected = _mm_blendv_ps(localZ, maxDepth24Bit, selectMask);
	newPrimitiveData.invZ = D3DXVECTOR3(localInvZSelected.m128_f32[0], localInvZSelected.m128_f32[1], localInvZSelected.m128_f32[2]);

	if (shadeFromShader)
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

template <const bool useVertexBuffer, const D3DFORMAT indexFormat>
void IDirect3DDevice9Hook::ProcessVerticesToBufferInner(const IDirect3DVertexDeclaration9Hook* const decl, const DeclarationSemanticMapping& mapping, const BYTE* const indexBuffer,
	const D3DPRIMITIVETYPE PrimitiveType, const INT BaseVertexIndex, const UINT MinVertexIndex, const UINT startIndex, const UINT primCount, const void* const vertStreamBytes, const unsigned short vertStreamStride) const
{
	// How many processed verts does this draw call output?
	const unsigned numOutputVerts = GetNumVertsUsed(PrimitiveType, primCount);

#ifdef _DEBUG
	if (numOutputVerts < 3)
	{
		DbgBreakPrint("Error: Not enough verts processed to make a triangle");
	}
#endif

	processedVertsUsed = 0;
	if (numOutputVerts >= processVertsAllocated)
	{
		_mm_free(processedVertexBuffer);
		processedVertexBuffer = NULL;
		processedVertexBuffer = (VS_2_0_OutputRegisters* const)_mm_malloc(numOutputVerts * sizeof(VS_2_0_OutputRegisters), 16);
		processVertsAllocated = numOutputVerts;
	}
	processedVertsUsed = numOutputVerts;

	frameStats.numVertsProcessed += numOutputVerts;

	VS_2_0_OutputRegisters* const startOutputBuffer = processedVertexBuffer;
	VS_2_0_OutputRegisters* outputBufferPtr = processedVertexBuffer;

#ifdef _DEBUG
	if ( ( ( (const size_t)outputBufferPtr) & 0xF) != 0)
	{
		__debugbreak(); // Oh noes, our registers aren't aligned and this will break SSE instructions!
	}
#endif

	vertJobsToShade.clear();

	const bool anyUserClipPlanesEnabled = currentDrawCallData.vertexData.userClipPlanesEnabled;

	unsigned short* alreadyShadedVerts16Ptr
#ifdef _DEBUG
		= NULL
#endif
		;
	unsigned* alreadyShadedVerts32Ptr
#ifdef _DEBUG
		= NULL
#endif
		;

	if (indexFormat != D3DFMT_UNKNOWN)
	{
		switch (indexFormat)
		{
		case D3DFMT_INDEX16:
		{
			alreadyShadedVerts16->clear();

			const unsigned short* const bufferShorts = (const unsigned short* const)indexBuffer;
			for (unsigned x = 0; x < numOutputVerts; ++x)
			{
				// Uhhhh, this isn't correct except for point-lists, line-lists, and triangle-lists:
				const unsigned short index = bufferShorts[x + startIndex] + BaseVertexIndex;

				if (index >= alreadyShadedVerts16->size() )
				{
					alreadyShadedVerts16->resize(index + 1, 0xFFFF);
					alreadyShadedVerts16Ptr = &alreadyShadedVerts16->front();
				}

				if (alreadyShadedVerts16Ptr[index] != 0xFFFF)
				{
					// Do fix-ups after the loop, but only shade vertices once
					++frameStats.numVertsReused;
				}
				else
				{
					vertJobCollector newJob;
					newJob.outputRegs = outputBufferPtr;
					newJob.vertexIndex = index;
					vertJobsToShade.push_back(newJob);

					alreadyShadedVerts16Ptr[index] = x;
				}

				// Increment the registers pointer
				++outputBufferPtr;
			}
		}
			break;
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Undefined index buffer format");
#else
			__assume(0);
#endif
		case D3DFMT_INDEX32:
		{
			alreadyShadedVerts32->clear();

			const unsigned* const bufferLongs = (const unsigned* const)indexBuffer;
			for (unsigned x = 0; x < numOutputVerts; ++x)
			{
				// Uhhhh, this isn't correct except for point-lists, line-lists, and triangle-lists:
				const unsigned index = bufferLongs[x + startIndex] + BaseVertexIndex;

				if (index >= alreadyShadedVerts32->size() )
				{
					alreadyShadedVerts32->resize(index + 1, 0xFFFFFFFF);
					alreadyShadedVerts32Ptr = &alreadyShadedVerts32->front();
				}

				if (alreadyShadedVerts32Ptr[index] != 0xFFFFFFFF)
				{
					// Do fix-ups after the loop, but only shade vertices once
					++frameStats.numVertsReused;
				}
				else
				{
					vertJobCollector newJob;
					newJob.outputRegs = outputBufferPtr;
					newJob.vertexIndex = index;
					vertJobsToShade.push_back(newJob);

					alreadyShadedVerts32Ptr[index] = x;
				}

				// Increment the registers pointer
				++outputBufferPtr;
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
			vertJobCollector newJob;
			newJob.outputRegs = outputBufferPtr;
			newJob.vertexIndex = x + BaseVertexIndex;
			vertJobsToShade.push_back(newJob);

			// Increment the registers pointer
			++outputBufferPtr;
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
#ifdef _DEBUG
					if ( ( ( (const size_t)outputRegs[y]) & 0xF) != 0)
					{
						__debugbreak(); // Oh noes, our registers aren't aligned and this will break SSE instructions!
					}
#endif
				}
#ifdef MULTITHREAD_SHADING
				CreateNewVertexShadeJob(outputRegs, vertexIndices, vertexShade4Job);
#else // #ifdef MULTITHREAD_SHADING

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
				LARGE_INTEGER vertexShadeStartTime;
				QueryPerformanceCounter(&vertexShadeStartTime);
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

				if (anyUserClipPlanesEnabled)
					ProcessVertexToBuffer4<true>(mapping, &deviceMainVShaderEngine, outputRegs, vertexIndices);
				else
					ProcessVertexToBuffer4<false>(mapping, &deviceMainVShaderEngine, outputRegs, vertexIndices);

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
				LARGE_INTEGER vertexShadeEndTime;
				QueryPerformanceCounter(&vertexShadeEndTime);

				totalVertexShadeTicks += (vertexShadeEndTime.QuadPart - vertexShadeStartTime.QuadPart);
				numVertexShadeTasks += 4;
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

#endif // #ifdef MULTITHREAD_SHADING
				x += 4;
			}
			else
			{
				const vertJobCollector& thisNewJob = vertJobsToShade[x];
#ifdef MULTITHREAD_SHADING
				CreateNewVertexShadeJob(&thisNewJob.outputRegs, &thisNewJob.vertexIndex, vertexShade1Job);
#else // #ifdef MULTITHREAD_SHADING

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
				LARGE_INTEGER vertexShadeStartTime;
				QueryPerformanceCounter(&vertexShadeStartTime);
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

				if (anyUserClipPlanesEnabled)
					ProcessVertexToBuffer<true>(mapping, &deviceMainVShaderEngine, thisNewJob.outputRegs, thisNewJob.vertexIndex);
				else
					ProcessVertexToBuffer<false>(mapping, &deviceMainVShaderEngine, thisNewJob.outputRegs, thisNewJob.vertexIndex);

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
				LARGE_INTEGER vertexShadeEndTime;
				QueryPerformanceCounter(&vertexShadeEndTime);

				totalVertexShadeTicks += (vertexShadeEndTime.QuadPart - vertexShadeStartTime.QuadPart);
				++numVertexShadeTasks;
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

#endif // #ifdef MULTITHREAD_SHADING
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
#ifdef MULTITHREAD_SHADING
			CreateNewVertexShadeJob(&thisNewJob.outputRegs, &thisNewJob.vertexIndex, vertexShade1Job);
#else // #ifdef MULTITHREAD_SHADING

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
			LARGE_INTEGER vertexShadeStartTime;
			QueryPerformanceCounter(&vertexShadeStartTime);
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

			if (anyUserClipPlanesEnabled)
				ProcessVertexToBuffer<true>(mapping, &deviceMainVShaderEngine, thisNewJob.outputRegs, thisNewJob.vertexIndex);
			else
				ProcessVertexToBuffer<false>(mapping, &deviceMainVShaderEngine, thisNewJob.outputRegs, thisNewJob.vertexIndex);

#ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES
			LARGE_INTEGER vertexShadeEndTime;
			QueryPerformanceCounter(&vertexShadeEndTime);

			totalVertexShadeTicks += (vertexShadeEndTime.QuadPart - vertexShadeStartTime.QuadPart);
			++numVertexShadeTasks;
#endif // #ifdef PROFILE_AVERAGE_VERTEX_SHADE_TIMES

#endif // #ifdef MULTITHREAD_SHADING
		}
	}

#ifdef MULTITHREAD_SHADING
	//CloseThreadpoolCleanupGroupMembers(cleanup, FALSE, NULL);
	//RefreshThreadpoolWork();
	{
		SynchronizeThreads<4>();
	}
#endif // #ifdef MULTITHREAD_SHADING

	// This is the "fixup phase" that's necessary to populate all of the vertex copies through here
	// This avoids shading re-used vertices more than once and saves *a ton* of processing power in not reshading! :)

	// TODO: Avoiding the "fixup phase" of processvertices would save time (roughly 1/3rd of the total cost of ProcessVerticesToBufferInner), at the added complexity
	// to DrawPrimitiveUB(), which would then need to know how to walk index buffers. Doing this would also save on memory bandwidth.
	{
		if (indexFormat != D3DFMT_UNKNOWN)
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

					if (alreadyShadedVerts16Ptr[index] != x)
					{
						startOutputBuffer[x] = startOutputBuffer[alreadyShadedVerts16Ptr[index] ];
					}
				}
			}
				break;
			default:
#ifdef _DEBUG
				DbgBreakPrint("Error: Undefined index buffer format");
#else
				__assume(0);
#endif
			case D3DFMT_INDEX32:
			{
				const unsigned* const bufferLongs = (const unsigned* const)indexBuffer;
				for (unsigned x = 0; x < numOutputVerts; ++x)
				{
					// Uhhhh, this isn't correct except for point-lists, line-lists, and triangle-lists:
					const unsigned index = bufferLongs[x + startIndex] + BaseVertexIndex;

					if (alreadyShadedVerts32Ptr[index] != x)
					{
						startOutputBuffer[x] = startOutputBuffer[alreadyShadedVerts32Ptr[index] ];
					}
				}
			}
				break;
			}
		}
	}

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
void IDirect3DDevice9Hook::ProcessVerticesToBuffer(const IDirect3DVertexDeclaration9Hook* const decl, const DeclarationSemanticMapping& mapping, const IDirect3DIndexBuffer9Hook* const indexBuffer, 
	const D3DPRIMITIVETYPE PrimitiveType, const INT BaseVertexIndex, const UINT MinVertexIndex, const UINT startIndex, const UINT primCount, const void* const vertStreamBytes, const unsigned short vertStreamStride) const
{
	SIMPLE_FUNC_SCOPE();

	if (useIndexBuffer)
	{
		switch (indexBuffer->GetFormat() )
		{
		default:
#ifdef _DEBUG
			__debugbreak(); // Should never be here!
#else
			__assume(0);
#endif
		case D3DFMT_INDEX16:
			ProcessVerticesToBufferInner<useVertexBuffer, D3DFMT_INDEX16>(decl, mapping, indexBuffer->GetBufferBytes(), PrimitiveType, BaseVertexIndex, MinVertexIndex, startIndex, primCount, vertStreamBytes, vertStreamStride);
			break;
		case D3DFMT_INDEX32:
			ProcessVerticesToBufferInner<useVertexBuffer, D3DFMT_INDEX32>(decl, mapping, indexBuffer->GetBufferBytes(), PrimitiveType, BaseVertexIndex, MinVertexIndex, startIndex, primCount, vertStreamBytes, vertStreamStride);
			break;
		}
	}
	else
	{
		ProcessVerticesToBufferInner<useVertexBuffer, D3DFMT_UNKNOWN>(decl, mapping, NULL, PrimitiveType, BaseVertexIndex, MinVertexIndex, startIndex, primCount, vertStreamBytes, vertStreamStride);
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

COM_DECLSPEC_NOTHROW void IDirect3DDevice9Hook::SetupCurrentDrawCallTriangleRasterizeData(const float fWidth, const float fHeight, const bool rasterizerUsesEarlyZTest, const bool rasterizeTriangleFromShader, const void* const interpolantDeclInfo) const
{
	currentDrawCallData.triangleRasterizeData.fWidth = fWidth;
	currentDrawCallData.triangleRasterizeData.fHeight = fHeight;
	if (rasterizeTriangleFromShader)
		currentDrawCallData.triangleRasterizeData.vStoPSMapping = (const VStoPSMapping* const)interpolantDeclInfo;
	else
		currentDrawCallData.triangleRasterizeData.vertexDeclMapping = (const DeclarationSemanticMapping* const)interpolantDeclInfo;
	currentDrawCallData.triangleRasterizeData.rasterizerUsesEarlyZTest = rasterizerUsesEarlyZTest;
	currentDrawCallData.triangleRasterizeData.rasterizeTriangleFromShader = rasterizeTriangleFromShader;
}

// Counts the number of 32-bit DWORD's for each of the D3DDECLTYPE's
static unsigned char typeSize_DWORDs[MAXD3DDECLTYPE] =
{
	1 * sizeof(DWORD),// D3DDECLTYPE_FLOAT1    =  0,  // 1D float expanded to (value, 0., 0., 1.)
    2 * sizeof(DWORD),// D3DDECLTYPE_FLOAT2    =  1,  // 2D float expanded to (value, value, 0., 1.)
    3 * sizeof(DWORD),// D3DDECLTYPE_FLOAT3    =  2,  // 3D float expanded to (value, value, value, 1.)
    4 * sizeof(DWORD),// D3DDECLTYPE_FLOAT4    =  3,  // 4D float
    1 * sizeof(DWORD),// D3DDECLTYPE_D3DCOLOR  =  4,  // 4D packed unsigned bytes mapped to 0. to 1. range
									// Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
    1 * sizeof(DWORD),// D3DDECLTYPE_UBYTE4    =  5,  // 4D unsigned byte
    1 * sizeof(DWORD),// D3DDECLTYPE_SHORT2    =  6,  // 2D signed short expanded to (value, value, 0., 1.)
    2 * sizeof(DWORD),// D3DDECLTYPE_SHORT4    =  7,  // 4D signed short

	// The following types are valid only with vertex shaders >= 2.0
    1 * sizeof(DWORD),// D3DDECLTYPE_UBYTE4N   =  8,  // Each of 4 bytes is normalized by dividing to 255.0
    1 * sizeof(DWORD),// D3DDECLTYPE_SHORT2N   =  9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
    2 * sizeof(DWORD),// D3DDECLTYPE_SHORT4N   = 10,  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
    1 * sizeof(DWORD),// D3DDECLTYPE_USHORT2N  = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
    2 * sizeof(DWORD),// D3DDECLTYPE_USHORT4N  = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
    1 * sizeof(DWORD),// D3DDECLTYPE_UDEC3     = 13,  // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
    1 * sizeof(DWORD),// D3DDECLTYPE_DEC3N     = 14,  // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
    1 * sizeof(DWORD),// D3DDECLTYPE_FLOAT16_2 = 15,  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
    2 * sizeof(DWORD) // D3DDECLTYPE_FLOAT16_4 = 16,  // Four 16-bit floating point values
};

static inline void ComputeCachedStreamEnd(StreamDataTypeEndPointers& thisStreamEnd, const BYTE* const streamBegin, const unsigned streamLenBytes)
{
	if (streamBegin != NULL && streamLenBytes != 0)
	{
		thisStreamEnd.streamEndAbsolute = streamBegin + streamLenBytes;
		for (unsigned x = 0; x < ARRAYSIZE(thisStreamEnd.dataTypeStreamEnds); ++x)
		{
			const D3DDECLTYPE thisType = (const D3DDECLTYPE)x;
			thisStreamEnd.dataTypeStreamEnds[thisType] = thisStreamEnd.streamEndAbsolute - typeSize_DWORDs[thisType];
		}
	}
	else
	{
		thisStreamEnd.streamEndAbsolute = NULL;
		for (unsigned x = 0; x < ARRAYSIZE(thisStreamEnd.dataTypeStreamEnds); ++x)
		{
			const D3DDECLTYPE thisType = (const D3DDECLTYPE)x;
			thisStreamEnd.dataTypeStreamEnds[thisType] = NULL;
		}
	}
}

void IDirect3DDevice9Hook::RecomputeCachedStreamEndsIfDirty()
{
	SIMPLE_FUNC_SCOPE();

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
	SIMPLE_FUNC_SCOPE();

#ifdef ENABLE_END_TO_SKIP_DRAWS
	// Skip this draw call if END is held down
	if (IsHoldingEndToSkipDrawCalls() )
		return false;
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

	if (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaBlendEnable && !currentState.currentRenderStates.renderStatesUnion.namedStates.separateAlphaBlendEnable)
	{
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_ZERO && currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_ONE)
			return false;
	}

	if (currentState.currentRenderStates.renderStatesUnion.namedStates.scissorTestEnable)
	{
		const RECT& thisScissorRect = currentState.currentScissorRect.scissorRect;
		if (thisScissorRect.right <= thisScissorRect.left)
			return false;
		if (thisScissorRect.bottom <= thisScissorRect.top)
			return false;
	}

	// Skip the entire draw call if we're doing dumb stuff (like rendering a fullscreen quad that is entirely alpha-transparent while alpha blending is enabled using the fixed function pipeline. Looking at you, Morrowind.exe):
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaBlendEnable && !currentState.currentPixelShader && !currentState.currentVertexShader)
	{
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.lighting && 
			currentState.currentRenderStates.renderStatesUnion.namedStates.ambientMaterialSource == D3DMCS_MATERIAL && currentState.currentMaterial.Ambient.a == 0.0f &&
			currentState.currentRenderStates.renderStatesUnion.namedStates.diffuseMaterialSource == D3DMCS_MATERIAL && currentState.currentMaterial.Diffuse.a == 0.0f)
		{
			// FFPS selects vertex color for alpha channel
			if (currentState.currentStageStates[0].stageStateUnion.namedStates.alphaOp == D3DTOP_MODULATE && 
				(currentState.currentStageStates[0].stageStateUnion.namedStates.alphaArg1 == D3DTA_DIFFUSE || currentState.currentStageStates[0].stageStateUnion.namedStates.alphaArg2 == D3DTA_DIFFUSE || 
				currentState.currentStageStates[0].stageStateUnion.namedStates.alphaArg1 == D3DTA_CURRENT || currentState.currentStageStates[0].stageStateUnion.namedStates.alphaArg2 == D3DTA_CURRENT) )
				return false;
			else if (currentState.currentStageStates[0].stageStateUnion.namedStates.alphaOp == D3DTOP_SELECTARG1 && (currentState.currentStageStates[0].stageStateUnion.namedStates.alphaArg1 == D3DTA_DIFFUSE || currentState.currentStageStates[0].stageStateUnion.namedStates.alphaArg1 == D3DTA_CURRENT) )
				return false;
			else if (currentState.currentStageStates[0].stageStateUnion.namedStates.alphaOp == D3DTOP_SELECTARG2 && (currentState.currentStageStates[0].stageStateUnion.namedStates.alphaArg2 == D3DTA_DIFFUSE || currentState.currentStageStates[0].stageStateUnion.namedStates.alphaArg2 == D3DTA_CURRENT) )
				return false;
		}
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
	SIMPLE_FUNC_SCOPE();

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

	processedVertsUsed = 0;
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

	ProcessVerticesToBuffer<true, false>(currentState.currentVertexDecl, vertexDeclMapping, NULL, PrimitiveType, 0, 0, StartVertex, PrimitiveCount, NULL, 0);

#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}
#endif

	if (CurrentPipelineCanEarlyZTest() )
		DrawPrimitiveUB<true>(PrimitiveType, PrimitiveCount);
	else
		DrawPrimitiveUB<false>(PrimitiveType, PrimitiveCount);

	if (usePassthroughVertexShader)
		SetVertexShader(NULL);
	if (usePassthroughPixelShader)
		SetPixelShader(NULL);

	return ret;
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DrawIndexedPrimitive(THIS_ D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	SIMPLE_FUNC_SCOPE();

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

	processedVertsUsed = 0;

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

	ProcessVerticesToBuffer<true, true>(currentState.currentVertexDecl, vertexDeclMapping, currentState.currentIndexBuffer, PrimitiveType, BaseVertexIndex, MinVertexIndex, startIndex, primCount, NULL, 0);

#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}
#endif

	if (CurrentPipelineCanEarlyZTest() )
		DrawPrimitiveUB<true>(PrimitiveType, primCount);
	else
		DrawPrimitiveUB<false>(PrimitiveType, primCount);

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

template <const unsigned char channelWriteMask>
void IDirect3DDevice9Hook::LoadBlend(D3DXVECTOR4& outBlend, const D3DBLEND blendMode, const D3DXVECTOR4& srcColor, const D3DXVECTOR4& dstColor) const
{
	// Simple + fast mode:
	if ( (channelWriteMask & 0x7) == 0x7)
	{
		switch (blendMode)
		{
		case D3DBLEND_ZERO:
			// Do nothing. This case is handled in AlphaBlend().
			return;
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Invalid D3DBLEND type passed to blending unit");
#else
			__assume(0);
#endif
		case D3DBLEND_ONE            :
			// Do nothing. This case is handled in AlphaBlend().
			return;
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
			outBlend = srcColor;
			return;
		case D3DBLEND_INVSRCCOLOR    :
			*(__m128* const)&outBlend = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&srcColor);
			return;
		case D3DBLEND_BOTHSRCALPHA   :
		case D3DBLEND_SRCALPHA       :
			*(__m128* const)&outBlend = _mm_load1_ps(&srcColor.w);
			return;
		case D3DBLEND_BOTHINVSRCALPHA:
		case D3DBLEND_INVSRCALPHA    :
			*(__m128* const)&outBlend = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&srcColor.w) );
			return;
		case D3DBLEND_DESTALPHA      :
			*(__m128* const)&outBlend = _mm_load1_ps(&dstColor.w);
			return;
		case D3DBLEND_INVDESTALPHA   :
			*(__m128* const)&outBlend = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&dstColor.w) );
			return;
		case D3DBLEND_DESTCOLOR      :
			outBlend = dstColor;
			return;
		case D3DBLEND_INVDESTCOLOR:
			*(__m128* const)&outBlend = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&dstColor);
			return;
		case D3DBLEND_SRCALPHASAT    :
		{
			// TODO: Low-priorty SIMDify this (nobody uses this blend state anyway)
			const float as = srcColor.w;
			const float invad = 1.0f - dstColor.w;
			const float f = as < invad ? as : invad;
			if (channelWriteMask & 0x1)
				outBlend.x = f;
			if (channelWriteMask & 0x2)
				outBlend.y = f;
			if (channelWriteMask & 0x4)
				outBlend.z = f;
			if (channelWriteMask & 0x8)
				outBlend.w = 1.0f;
			return;
		}
		case D3DBLEND_BLENDFACTOR    :
			outBlend = currentState.currentRenderStates.cachedBlendFactor;
			return;
		case D3DBLEND_INVBLENDFACTOR:
			outBlend = currentState.currentRenderStates.cachedInvBlendFactor;
			return;
		}
		return;
	}

	switch (blendMode)
	{
	case D3DBLEND_ZERO           :
		// Do nothing. This case is handled in AlphaBlend().
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid D3DBLEND type passed to blending unit");
#else
		__assume(0);
#endif
	case D3DBLEND_ONE            :
		// Do nothing. This case is handled in AlphaBlend().
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
		if (channelWriteMask & 0x1)
			outBlend.x = srcColor.x;
		if (channelWriteMask & 0x2)
			outBlend.y = srcColor.y;
		if (channelWriteMask & 0x4)
			outBlend.z = srcColor.z;
		if (channelWriteMask & 0x8)
			outBlend.w = srcColor.w;
		break;
	case D3DBLEND_INVSRCCOLOR    :
		if (channelWriteMask & 0x1)
			outBlend.x = 1.0f - srcColor.x;
		if (channelWriteMask & 0x2)
			outBlend.y = 1.0f - srcColor.y;
		if (channelWriteMask & 0x4)
			outBlend.z = 1.0f - srcColor.z;
		if (channelWriteMask & 0x8)
			outBlend.w = 1.0f - srcColor.w;
		break;
	case D3DBLEND_BOTHSRCALPHA   :
	case D3DBLEND_SRCALPHA       :
		if (channelWriteMask & 0x1)
			outBlend.x = srcColor.w;
		if (channelWriteMask & 0x2)
			outBlend.y = srcColor.w;
		if (channelWriteMask & 0x4)
			outBlend.z = srcColor.w;
		if (channelWriteMask & 0x8)
			outBlend.w = srcColor.w;
		break;
	case D3DBLEND_BOTHINVSRCALPHA:
	case D3DBLEND_INVSRCALPHA    :
		if (channelWriteMask & 0x1)
			outBlend.x = 1.0f - srcColor.w;
		if (channelWriteMask & 0x2)
			outBlend.y = 1.0f - srcColor.w;
		if (channelWriteMask & 0x4)
			outBlend.z = 1.0f - srcColor.w;
		if (channelWriteMask & 0x8)
			outBlend.w = 1.0f - srcColor.w;
		break;
	case D3DBLEND_DESTALPHA      :
		if (channelWriteMask & 0x1)
			outBlend.x = dstColor.w;
		if (channelWriteMask & 0x2)
			outBlend.y = dstColor.w;
		if (channelWriteMask & 0x4)
			outBlend.z = dstColor.w;
		if (channelWriteMask & 0x8)
			outBlend.w = dstColor.w;
		break;
	case D3DBLEND_INVDESTALPHA   :
		if (channelWriteMask & 0x1)
			outBlend.x = 1.0f - dstColor.w;
		if (channelWriteMask & 0x2)
			outBlend.y = 1.0f - dstColor.w;
		if (channelWriteMask & 0x4)
			outBlend.z = 1.0f - dstColor.w;
		if (channelWriteMask & 0x8)
			outBlend.w = 1.0f - dstColor.w;
		break;
	case D3DBLEND_DESTCOLOR      :
		if (channelWriteMask & 0x1)
			outBlend.x = dstColor.x;
		if (channelWriteMask & 0x2)
			outBlend.y = dstColor.y;
		if (channelWriteMask & 0x4)
			outBlend.z = dstColor.z;
		if (channelWriteMask & 0x8)
			outBlend.w = dstColor.w;
		break;
	case D3DBLEND_INVDESTCOLOR   :
		if (channelWriteMask & 0x1)
			outBlend.x = 1.0f - dstColor.x;
		if (channelWriteMask & 0x2)
			outBlend.y = 1.0f - dstColor.y;
		if (channelWriteMask & 0x4)
			outBlend.z = 1.0f - dstColor.z;
		if (channelWriteMask & 0x8)
			outBlend.w = 1.0f - dstColor.w;
		break;
	case D3DBLEND_SRCALPHASAT    :
	{
		const float as = srcColor.w;
		const float invad = 1.0f - dstColor.w;
		const float f = as < invad ? as : invad;
		if (channelWriteMask & 0x1)
			outBlend.x = f;
		if (channelWriteMask & 0x2)
			outBlend.y = f;
		if (channelWriteMask & 0x4)
			outBlend.z = f;
		if (channelWriteMask & 0x8)
			outBlend.w = 1.0f;
	}
		break;
	case D3DBLEND_BLENDFACTOR    :
		if (channelWriteMask & 0x1)
			outBlend.x = currentState.currentRenderStates.cachedBlendFactor.x;
		if (channelWriteMask & 0x2)
			outBlend.y = currentState.currentRenderStates.cachedBlendFactor.y;
		if (channelWriteMask & 0x4)
			outBlend.z = currentState.currentRenderStates.cachedBlendFactor.z;
		if (channelWriteMask & 0x8)
			outBlend.w = currentState.currentRenderStates.cachedBlendFactor.w;
		break;
	case D3DBLEND_INVBLENDFACTOR :
		if (channelWriteMask & 0x1)
			outBlend.x = currentState.currentRenderStates.cachedInvBlendFactor.x;
		if (channelWriteMask & 0x2)
			outBlend.y = currentState.currentRenderStates.cachedInvBlendFactor.y;
		if (channelWriteMask & 0x4)
			outBlend.z = currentState.currentRenderStates.cachedInvBlendFactor.z;
		if (channelWriteMask & 0x8)
			outBlend.w = currentState.currentRenderStates.cachedInvBlendFactor.w;
		break;
	}
}

template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::LoadBlend4(D3DXVECTOR4 (&outBlend)[4], const D3DBLEND blendMode, const D3DXVECTOR4 (&srcColor)[4], const D3DXVECTOR4 (&dstColor)[4]) const
{
	if (pixelWriteMask == 0x0)
	{
#ifdef _DEBUG
		__debugbreak(); // Don't call this function if you aren't going to write anything out!
#endif
		return;
	}

	// Simple + fast mode:
	if ( (channelWriteMask & 0x7) == 0x7)
	{
		switch (blendMode)
		{
		case D3DBLEND_ZERO:
			// Do nothing. This case is handled in AlphaBlend4().
			return;
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Invalid D3DBLEND type passed to blending unit");
#else
			__assume(0);
#endif
		case D3DBLEND_ONE            :
			// Do nothing. This case is handled in AlphaBlend4().
			return;
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
			if (pixelWriteMask & 0x1) outBlend[0] = srcColor[0];
			if (pixelWriteMask & 0x2) outBlend[1] = srcColor[1];
			if (pixelWriteMask & 0x4) outBlend[2] = srcColor[2];
			if (pixelWriteMask & 0x8) outBlend[3] = srcColor[3];
			return;
		case D3DBLEND_INVSRCCOLOR    :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outBlend[0]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(srcColor[0]) );
			if (pixelWriteMask & 0x2) *(__m128* const)&(outBlend[1]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(srcColor[1]) );
			if (pixelWriteMask & 0x4) *(__m128* const)&(outBlend[2]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(srcColor[2]) );
			if (pixelWriteMask & 0x8) *(__m128* const)&(outBlend[3]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(srcColor[3]) );
			return;
		case D3DBLEND_BOTHSRCALPHA   :
		case D3DBLEND_SRCALPHA       :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outBlend[0]) = _mm_load1_ps(&(srcColor[0].w) );
			if (pixelWriteMask & 0x2) *(__m128* const)&(outBlend[1]) = _mm_load1_ps(&(srcColor[1].w) );
			if (pixelWriteMask & 0x4) *(__m128* const)&(outBlend[2]) = _mm_load1_ps(&(srcColor[2].w) );
			if (pixelWriteMask & 0x8) *(__m128* const)&(outBlend[3]) = _mm_load1_ps(&(srcColor[3].w) );
			return;
		case D3DBLEND_BOTHINVSRCALPHA:
		case D3DBLEND_INVSRCALPHA    :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outBlend[0]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&(srcColor[0].w) ) );
			if (pixelWriteMask & 0x2) *(__m128* const)&(outBlend[1]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&(srcColor[1].w) ) );
			if (pixelWriteMask & 0x4) *(__m128* const)&(outBlend[2]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&(srcColor[2].w) ) );
			if (pixelWriteMask & 0x8) *(__m128* const)&(outBlend[3]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&(srcColor[3].w) ) );
			return;
		case D3DBLEND_DESTALPHA      :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outBlend[0]) = _mm_load1_ps(&(dstColor[0].w) );
			if (pixelWriteMask & 0x2) *(__m128* const)&(outBlend[1]) = _mm_load1_ps(&(dstColor[1].w) );
			if (pixelWriteMask & 0x4) *(__m128* const)&(outBlend[2]) = _mm_load1_ps(&(dstColor[2].w) );
			if (pixelWriteMask & 0x8) *(__m128* const)&(outBlend[3]) = _mm_load1_ps(&(dstColor[3].w) );
			return;
		case D3DBLEND_INVDESTALPHA   :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outBlend[0]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&(dstColor[0].w) ) );
			if (pixelWriteMask & 0x2) *(__m128* const)&(outBlend[1]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&(dstColor[1].w) ) );
			if (pixelWriteMask & 0x4) *(__m128* const)&(outBlend[2]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&(dstColor[2].w) ) );
			if (pixelWriteMask & 0x8) *(__m128* const)&(outBlend[3]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, _mm_load1_ps(&(dstColor[3].w) ) );
			return;
		case D3DBLEND_DESTCOLOR      :
			if (pixelWriteMask & 0x1) outBlend[0] = dstColor[0];
			if (pixelWriteMask & 0x2) outBlend[1] = dstColor[1];
			if (pixelWriteMask & 0x4) outBlend[2] = dstColor[2];
			if (pixelWriteMask & 0x8) outBlend[3] = dstColor[3];
			return;
		case D3DBLEND_INVDESTCOLOR:
			if (pixelWriteMask & 0x1) *(__m128* const)&(outBlend[0]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(dstColor[0]) );
			if (pixelWriteMask & 0x2) *(__m128* const)&(outBlend[1]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(dstColor[1]) );
			if (pixelWriteMask & 0x4) *(__m128* const)&(outBlend[2]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(dstColor[2]) );
			if (pixelWriteMask & 0x8) *(__m128* const)&(outBlend[3]) = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(dstColor[3]) );
			return;
		case D3DBLEND_SRCALPHASAT    :
		{
			// TODO: Low-priorty SIMDify this (nobody uses this blend state anyway)
			for (unsigned x = 0; x < 4; ++x)
			{
				if (pixelWriteMask & (1 << x) )
				{
					const float as = srcColor[x].w;
					const float invad = 1.0f - dstColor[x].w;
					const float f = as < invad ? as : invad;
					if (channelWriteMask & 0x1)
						outBlend[x].x = f;
					if (channelWriteMask & 0x2)
						outBlend[x].y = f;
					if (channelWriteMask & 0x4)
						outBlend[x].z = f;
					if (channelWriteMask & 0x8)
						outBlend[x].w = 1.0f;
				}
			}
			return;
		}
		case D3DBLEND_BLENDFACTOR    :
		{
			const __m128 blendFactor = *(const __m128* const)&(currentState.currentRenderStates.cachedBlendFactor);
			if (pixelWriteMask & 0x1) *(__m128* const)&(outBlend[0]) = blendFactor;
			if (pixelWriteMask & 0x2) *(__m128* const)&(outBlend[1]) = blendFactor;
			if (pixelWriteMask & 0x4) *(__m128* const)&(outBlend[2]) = blendFactor;
			if (pixelWriteMask & 0x8) *(__m128* const)&(outBlend[3]) = blendFactor;
		}
			return;
		case D3DBLEND_INVBLENDFACTOR:
		{
			const __m128 invBlendFactor = *(const __m128* const)&(currentState.currentRenderStates.cachedInvBlendFactor);
			if (pixelWriteMask & 0x1) *(__m128* const)&(outBlend[0]) = invBlendFactor;
			if (pixelWriteMask & 0x2) *(__m128* const)&(outBlend[1]) = invBlendFactor;
			if (pixelWriteMask & 0x4) *(__m128* const)&(outBlend[2]) = invBlendFactor;
			if (pixelWriteMask & 0x8) *(__m128* const)&(outBlend[3]) = invBlendFactor;
		}
			return;
		}
		return;
	}

	switch (blendMode)
	{
	case D3DBLEND_ZERO           :
		// Do nothing. This case is handled in AlphaBlend4().
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Invalid D3DBLEND type passed to blending unit");
#else
		__assume(0);
#endif
	case D3DBLEND_ONE            :
		// Do nothing. This case is handled in AlphaBlend4().
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
		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = srcColor[0].x;
			if (channelWriteMask & 0x2)	outBlend[0].y = srcColor[0].y;
			if (channelWriteMask & 0x4)	outBlend[0].z = srcColor[0].z;
			if (channelWriteMask & 0x8)	outBlend[0].w = srcColor[0].w;
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = srcColor[1].x;
			if (channelWriteMask & 0x2)	outBlend[1].y = srcColor[1].y;
			if (channelWriteMask & 0x4)	outBlend[1].z = srcColor[1].z;
			if (channelWriteMask & 0x8)	outBlend[1].w = srcColor[1].w;
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = srcColor[2].x;
			if (channelWriteMask & 0x2)	outBlend[2].y = srcColor[2].y;
			if (channelWriteMask & 0x4)	outBlend[2].z = srcColor[2].z;
			if (channelWriteMask & 0x8)	outBlend[2].w = srcColor[2].w;
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = srcColor[3].x;
			if (channelWriteMask & 0x2)	outBlend[3].y = srcColor[3].y;
			if (channelWriteMask & 0x4)	outBlend[3].z = srcColor[3].z;
			if (channelWriteMask & 0x8)	outBlend[3].w = srcColor[3].w;
		}
		break;
	case D3DBLEND_INVSRCCOLOR    :
	{
		__m128 invSrcColor4[4];
		if (pixelWriteMask & 0x1) invSrcColor4[0] = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(srcColor[0]) );
		if (pixelWriteMask & 0x2) invSrcColor4[1] = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(srcColor[1]) );
		if (pixelWriteMask & 0x4) invSrcColor4[2] = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(srcColor[2]) );
		if (pixelWriteMask & 0x8) invSrcColor4[3] = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(srcColor[3]) );

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = invSrcColor4[0].m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[0].y = invSrcColor4[0].m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[0].z = invSrcColor4[0].m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[0].w = invSrcColor4[0].m128_f32[3];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = invSrcColor4[1].m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[1].y = invSrcColor4[1].m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[1].z = invSrcColor4[1].m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[1].w = invSrcColor4[1].m128_f32[3];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = invSrcColor4[2].m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[2].y = invSrcColor4[2].m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[2].z = invSrcColor4[2].m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[2].w = invSrcColor4[2].m128_f32[3];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = invSrcColor4[3].m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[3].y = invSrcColor4[3].m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[3].z = invSrcColor4[3].m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[3].w = invSrcColor4[3].m128_f32[3];
		}
	}
		break;
	case D3DBLEND_BOTHSRCALPHA   :
	case D3DBLEND_SRCALPHA       :
	{
		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = srcColor[0].w;
			if (channelWriteMask & 0x2)	outBlend[0].y = srcColor[0].w;
			if (channelWriteMask & 0x4)	outBlend[0].z = srcColor[0].w;
			if (channelWriteMask & 0x8)	outBlend[0].w = srcColor[0].w;
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = srcColor[1].w;
			if (channelWriteMask & 0x2)	outBlend[1].y = srcColor[1].w;
			if (channelWriteMask & 0x4)	outBlend[1].z = srcColor[1].w;
			if (channelWriteMask & 0x8)	outBlend[1].w = srcColor[1].w;
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = srcColor[2].w;
			if (channelWriteMask & 0x2)	outBlend[2].y = srcColor[2].w;
			if (channelWriteMask & 0x4)	outBlend[2].z = srcColor[2].w;
			if (channelWriteMask & 0x8)	outBlend[2].w = srcColor[2].w;
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = srcColor[3].w;
			if (channelWriteMask & 0x2)	outBlend[3].y = srcColor[3].w;
			if (channelWriteMask & 0x4)	outBlend[3].z = srcColor[3].w;
			if (channelWriteMask & 0x8)	outBlend[3].w = srcColor[3].w;
		}
	}
		break;
	case D3DBLEND_BOTHINVSRCALPHA:
	case D3DBLEND_INVSRCALPHA    :
	{
		__m128 alphaVec;
		if (pixelWriteMask & 0x1) alphaVec.m128_f32[0] = srcColor[0].w;
		if (pixelWriteMask & 0x2) alphaVec.m128_f32[1] = srcColor[1].w;
		if (pixelWriteMask & 0x4) alphaVec.m128_f32[2] = srcColor[2].w;
		if (pixelWriteMask & 0x8) alphaVec.m128_f32[3] = srcColor[3].w;
		const __m128 invAlphaVec = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, alphaVec);

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = invAlphaVec.m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[0].y = invAlphaVec.m128_f32[0];
			if (channelWriteMask & 0x4)	outBlend[0].z = invAlphaVec.m128_f32[0];
			if (channelWriteMask & 0x8)	outBlend[0].w = invAlphaVec.m128_f32[0];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = invAlphaVec.m128_f32[1];
			if (channelWriteMask & 0x2)	outBlend[1].y = invAlphaVec.m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[1].z = invAlphaVec.m128_f32[1];
			if (channelWriteMask & 0x8)	outBlend[1].w = invAlphaVec.m128_f32[1];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = invAlphaVec.m128_f32[2];
			if (channelWriteMask & 0x2)	outBlend[2].y = invAlphaVec.m128_f32[2];
			if (channelWriteMask & 0x4)	outBlend[2].z = invAlphaVec.m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[2].w = invAlphaVec.m128_f32[2];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = invAlphaVec.m128_f32[3];
			if (channelWriteMask & 0x2)	outBlend[3].y = invAlphaVec.m128_f32[3];
			if (channelWriteMask & 0x4)	outBlend[3].z = invAlphaVec.m128_f32[3];
			if (channelWriteMask & 0x8)	outBlend[3].w = invAlphaVec.m128_f32[3];
		}
	}
		break;
	case D3DBLEND_DESTALPHA      :
		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = dstColor[0].w;
			if (channelWriteMask & 0x2)	outBlend[0].y = dstColor[0].w;
			if (channelWriteMask & 0x4)	outBlend[0].z = dstColor[0].w;
			if (channelWriteMask & 0x8)	outBlend[0].w = dstColor[0].w;
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = dstColor[1].w;
			if (channelWriteMask & 0x2)	outBlend[1].y = dstColor[1].w;
			if (channelWriteMask & 0x4)	outBlend[1].z = dstColor[1].w;
			if (channelWriteMask & 0x8)	outBlend[1].w = dstColor[1].w;
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = dstColor[2].w;
			if (channelWriteMask & 0x2)	outBlend[2].y = dstColor[2].w;
			if (channelWriteMask & 0x4)	outBlend[2].z = dstColor[2].w;
			if (channelWriteMask & 0x8)	outBlend[2].w = dstColor[2].w;
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = dstColor[3].w;
			if (channelWriteMask & 0x2)	outBlend[3].y = dstColor[3].w;
			if (channelWriteMask & 0x4)	outBlend[3].z = dstColor[3].w;
			if (channelWriteMask & 0x8)	outBlend[3].w = dstColor[3].w;
		}
		break;
	case D3DBLEND_INVDESTALPHA   :
	{
		__m128 alphaVec;
		if (pixelWriteMask & 0x1) alphaVec.m128_f32[0] = dstColor[0].w;
		if (pixelWriteMask & 0x2) alphaVec.m128_f32[1] = dstColor[1].w;
		if (pixelWriteMask & 0x4) alphaVec.m128_f32[2] = dstColor[2].w;
		if (pixelWriteMask & 0x8) alphaVec.m128_f32[3] = dstColor[3].w;
		const __m128 invAlphaVec = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, alphaVec);

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = invAlphaVec.m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[0].y = invAlphaVec.m128_f32[0];
			if (channelWriteMask & 0x4)	outBlend[0].z = invAlphaVec.m128_f32[0];
			if (channelWriteMask & 0x8)	outBlend[0].w = invAlphaVec.m128_f32[0];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = invAlphaVec.m128_f32[1];
			if (channelWriteMask & 0x2)	outBlend[1].y = invAlphaVec.m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[1].z = invAlphaVec.m128_f32[1];
			if (channelWriteMask & 0x8)	outBlend[1].w = invAlphaVec.m128_f32[1];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = invAlphaVec.m128_f32[2];
			if (channelWriteMask & 0x2)	outBlend[2].y = invAlphaVec.m128_f32[2];
			if (channelWriteMask & 0x4)	outBlend[2].z = invAlphaVec.m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[2].w = invAlphaVec.m128_f32[2];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = invAlphaVec.m128_f32[3];
			if (channelWriteMask & 0x2)	outBlend[3].y = invAlphaVec.m128_f32[3];
			if (channelWriteMask & 0x4)	outBlend[3].z = invAlphaVec.m128_f32[3];
			if (channelWriteMask & 0x8)	outBlend[3].w = invAlphaVec.m128_f32[3];
		}
	}
		break;
	case D3DBLEND_DESTCOLOR      :
		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = dstColor[0].x;
			if (channelWriteMask & 0x2)	outBlend[0].y = dstColor[0].y;
			if (channelWriteMask & 0x4)	outBlend[0].z = dstColor[0].z;
			if (channelWriteMask & 0x8)	outBlend[0].w = dstColor[0].w;
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = dstColor[1].x;
			if (channelWriteMask & 0x2)	outBlend[1].y = dstColor[1].y;
			if (channelWriteMask & 0x4)	outBlend[1].z = dstColor[1].z;
			if (channelWriteMask & 0x8)	outBlend[1].w = dstColor[1].w;
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = dstColor[2].x;
			if (channelWriteMask & 0x2)	outBlend[2].y = dstColor[2].y;
			if (channelWriteMask & 0x4)	outBlend[2].z = dstColor[2].z;
			if (channelWriteMask & 0x8)	outBlend[2].w = dstColor[2].w;
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = dstColor[3].x;
			if (channelWriteMask & 0x2)	outBlend[3].y = dstColor[3].y;
			if (channelWriteMask & 0x4)	outBlend[3].z = dstColor[3].z;
			if (channelWriteMask & 0x8)	outBlend[3].w = dstColor[3].w;
		}
		break;
	case D3DBLEND_INVDESTCOLOR   :
	{
		__m128 invDstColor4[4];
		if (pixelWriteMask & 0x1) invDstColor4[0] = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(dstColor[0]) );
		if (pixelWriteMask & 0x2) invDstColor4[1] = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(dstColor[1]) );
		if (pixelWriteMask & 0x4) invDstColor4[2] = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(dstColor[2]) );
		if (pixelWriteMask & 0x8) invDstColor4[3] = _mm_sub_ps(*(const __m128* const)&staticColorWhiteOpaque, *(const __m128* const)&(dstColor[3]) );

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = invDstColor4[0].m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[0].y = invDstColor4[0].m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[0].z = invDstColor4[0].m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[0].w = invDstColor4[0].m128_f32[3];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = invDstColor4[1].m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[1].y = invDstColor4[1].m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[1].z = invDstColor4[1].m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[1].w = invDstColor4[1].m128_f32[3];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = invDstColor4[2].m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[2].y = invDstColor4[2].m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[2].z = invDstColor4[2].m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[2].w = invDstColor4[2].m128_f32[3];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = invDstColor4[3].m128_f32[0];
			if (channelWriteMask & 0x2)	outBlend[3].y = invDstColor4[3].m128_f32[1];
			if (channelWriteMask & 0x4)	outBlend[3].z = invDstColor4[3].m128_f32[2];
			if (channelWriteMask & 0x8)	outBlend[3].w = invDstColor4[3].m128_f32[3];
		}
	}
		break;
	case D3DBLEND_SRCALPHASAT    :
	{
		// TODO: Low-priorty SIMDify this (nobody uses this blend state anyway)
		for (unsigned x = 0; x < 4; ++x)
		{
			if (pixelWriteMask & (1 << x) )
			{
				const float as = srcColor[x].w;
				const float invad = 1.0f - dstColor[x].w;
				const float f = as < invad ? as : invad;
				if (channelWriteMask & 0x1)
					outBlend[x].x = f;
				if (channelWriteMask & 0x2)
					outBlend[x].y = f;
				if (channelWriteMask & 0x4)
					outBlend[x].z = f;
				if (channelWriteMask & 0x8)
					outBlend[x].w = 1.0f;
			}
		}
	}
		break;
	case D3DBLEND_BLENDFACTOR    :
		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = currentState.currentRenderStates.cachedBlendFactor.x;
			if (channelWriteMask & 0x2)	outBlend[0].y = currentState.currentRenderStates.cachedBlendFactor.y;
			if (channelWriteMask & 0x4)	outBlend[0].z = currentState.currentRenderStates.cachedBlendFactor.z;
			if (channelWriteMask & 0x8)	outBlend[0].w = currentState.currentRenderStates.cachedBlendFactor.w;
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = currentState.currentRenderStates.cachedBlendFactor.x;
			if (channelWriteMask & 0x2)	outBlend[1].y = currentState.currentRenderStates.cachedBlendFactor.y;
			if (channelWriteMask & 0x4)	outBlend[1].z = currentState.currentRenderStates.cachedBlendFactor.z;
			if (channelWriteMask & 0x8)	outBlend[1].w = currentState.currentRenderStates.cachedBlendFactor.w;
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = currentState.currentRenderStates.cachedBlendFactor.x;
			if (channelWriteMask & 0x2)	outBlend[2].y = currentState.currentRenderStates.cachedBlendFactor.y;
			if (channelWriteMask & 0x4)	outBlend[2].z = currentState.currentRenderStates.cachedBlendFactor.z;
			if (channelWriteMask & 0x8)	outBlend[2].w = currentState.currentRenderStates.cachedBlendFactor.w;
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = currentState.currentRenderStates.cachedBlendFactor.x;
			if (channelWriteMask & 0x2)	outBlend[3].y = currentState.currentRenderStates.cachedBlendFactor.y;
			if (channelWriteMask & 0x4)	outBlend[3].z = currentState.currentRenderStates.cachedBlendFactor.z;
			if (channelWriteMask & 0x8)	outBlend[3].w = currentState.currentRenderStates.cachedBlendFactor.w;
		}
		break;
	case D3DBLEND_INVBLENDFACTOR :
		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outBlend[0].x = currentState.currentRenderStates.cachedInvBlendFactor.x;
			if (channelWriteMask & 0x2)	outBlend[0].y = currentState.currentRenderStates.cachedInvBlendFactor.y;
			if (channelWriteMask & 0x4)	outBlend[0].z = currentState.currentRenderStates.cachedInvBlendFactor.z;
			if (channelWriteMask & 0x8)	outBlend[0].w = currentState.currentRenderStates.cachedInvBlendFactor.w;
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outBlend[1].x = currentState.currentRenderStates.cachedInvBlendFactor.x;
			if (channelWriteMask & 0x2)	outBlend[1].y = currentState.currentRenderStates.cachedInvBlendFactor.y;
			if (channelWriteMask & 0x4)	outBlend[1].z = currentState.currentRenderStates.cachedInvBlendFactor.z;
			if (channelWriteMask & 0x8)	outBlend[1].w = currentState.currentRenderStates.cachedInvBlendFactor.w;
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outBlend[2].x = currentState.currentRenderStates.cachedInvBlendFactor.x;
			if (channelWriteMask & 0x2)	outBlend[2].y = currentState.currentRenderStates.cachedInvBlendFactor.y;
			if (channelWriteMask & 0x4)	outBlend[2].z = currentState.currentRenderStates.cachedInvBlendFactor.z;
			if (channelWriteMask & 0x8)	outBlend[2].w = currentState.currentRenderStates.cachedInvBlendFactor.w;
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outBlend[3].x = currentState.currentRenderStates.cachedInvBlendFactor.x;
			if (channelWriteMask & 0x2)	outBlend[3].y = currentState.currentRenderStates.cachedInvBlendFactor.y;
			if (channelWriteMask & 0x4)	outBlend[3].z = currentState.currentRenderStates.cachedInvBlendFactor.z;
			if (channelWriteMask & 0x8)	outBlend[3].w = currentState.currentRenderStates.cachedInvBlendFactor.w;
		}
		break;
	}
}

template <const unsigned char channelWriteMask>
void IDirect3DDevice9Hook::AlphaBlend(D3DXVECTOR4& outVec, const D3DBLENDOP blendOp, const D3DXVECTOR4& srcBlend, const D3DXVECTOR4& dstBlend, const D3DXVECTOR4& srcColor, const D3DXVECTOR4& dstColor) const
{
	__m128 combinedSrc;
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_ZERO)
		combinedSrc = *(const __m128* const)&zeroVec;
	else if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_ONE)
		combinedSrc = *(const __m128* const)&srcColor;
	else
		combinedSrc = _mm_mul_ps(*(const __m128* const)&srcBlend, *(const __m128* const)&srcColor);

	__m128 combinedDst;
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_ZERO)
		combinedDst = *(const __m128* const)&zeroVec;
	else if (currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_ONE)
		combinedDst = *(const __m128* const)&dstColor;
	else
		combinedDst = _mm_mul_ps(*(const __m128* const)&dstBlend, *(const __m128* const)&dstColor);

	// Simple SIMD version
	if ( (channelWriteMask & 0x7) == 0x7)
	{
		switch (blendOp)
		{
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Invalid D3DBLENDOP passed to alpha blending unit");
#else
			__assume(0);
#endif
		case D3DBLENDOP_ADD        :
			*(__m128* const)&outVec = _mm_add_ps(combinedSrc, combinedDst);
			return;
		case D3DBLENDOP_SUBTRACT   :
			*(__m128* const)&outVec = _mm_sub_ps(combinedSrc, combinedDst);
			return;
		case D3DBLENDOP_REVSUBTRACT:
			*(__m128* const)&outVec = _mm_sub_ps(combinedDst, combinedSrc);
			return;
		case D3DBLENDOP_MIN        :
			*(__m128* const)&outVec = _mm_min_ps(combinedSrc, combinedDst);
			return;
		case D3DBLENDOP_MAX        :
			*(__m128* const)&outVec = _mm_max_ps(combinedSrc, combinedDst);
			return;
		}

		return;
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
	{
		const __m128 sum = _mm_add_ps(combinedSrc, combinedDst);
		if (channelWriteMask & 0x1)
			outVec.x = sum.m128_f32[0];
		if (channelWriteMask & 0x2)
			outVec.y = sum.m128_f32[1];
		if (channelWriteMask & 0x4)
			outVec.z = sum.m128_f32[2];
		if (channelWriteMask & 0x8)
			outVec.w = sum.m128_f32[3];
	}
		break;
	case D3DBLENDOP_SUBTRACT   :
	{
		const __m128 difference = _mm_sub_ps(combinedSrc, combinedDst);
		if (channelWriteMask & 0x1)
			outVec.x = difference.m128_f32[0];
		if (channelWriteMask & 0x2)
			outVec.y = difference.m128_f32[1];
		if (channelWriteMask & 0x4)
			outVec.z = difference.m128_f32[2];
		if (channelWriteMask & 0x8)
			outVec.w = difference.m128_f32[3];
	}
		break;
	case D3DBLENDOP_REVSUBTRACT:
	{
		const __m128 revDifference = _mm_sub_ps(combinedDst, combinedSrc);
		if (channelWriteMask & 0x1)
			outVec.x = revDifference.m128_f32[0];
		if (channelWriteMask & 0x2)
			outVec.y = revDifference.m128_f32[1];
		if (channelWriteMask & 0x4)
			outVec.z = revDifference.m128_f32[2];
		if (channelWriteMask & 0x8)
			outVec.w = revDifference.m128_f32[3];
	}
		break;
	case D3DBLENDOP_MIN        :
	{
		const __m128 vecMin = _mm_min_ps(combinedSrc, combinedDst);
		if (channelWriteMask & 0x1)
			outVec.x = vecMin.m128_f32[0];
		if (channelWriteMask & 0x2)
			outVec.y = vecMin.m128_f32[1];
		if (channelWriteMask & 0x4)
			outVec.z = vecMin.m128_f32[2];
		if (channelWriteMask & 0x8)
			outVec.w = vecMin.m128_f32[3];
	}
		break;
	case D3DBLENDOP_MAX        :
	{
		const __m128 vecMax = _mm_max_ps(combinedSrc, combinedDst);
		if (channelWriteMask & 0x1)
			outVec.x = vecMax.m128_f32[0];
		if (channelWriteMask & 0x2)
			outVec.y = vecMax.m128_f32[1];
		if (channelWriteMask & 0x4)
			outVec.z = vecMax.m128_f32[2];
		if (channelWriteMask & 0x8)
			outVec.w = vecMax.m128_f32[3];
	}
		break;
	}
}

template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::AlphaBlend4(D3DXVECTOR4 (&outVec)[4], const D3DBLENDOP blendOp, const D3DXVECTOR4 (&srcBlend)[4], const D3DXVECTOR4 (&dstBlend)[4], const D3DXVECTOR4 (&srcColor)[4], const D3DXVECTOR4 (&dstColor)[4]) const
{
	if (pixelWriteMask == 0x0)
	{
#ifdef _DEBUG
		__debugbreak(); // Don't call this function if you aren't going to write any pixels out!
#endif
		return;
	}

	__m128 combinedSrc[4];
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_ZERO)
	{
		if (pixelWriteMask & 0x1) combinedSrc[0] = *(const __m128* const)&zeroVec;
		if (pixelWriteMask & 0x2) combinedSrc[1] = *(const __m128* const)&zeroVec;
		if (pixelWriteMask & 0x4) combinedSrc[2] = *(const __m128* const)&zeroVec;
		if (pixelWriteMask & 0x8) combinedSrc[3] = *(const __m128* const)&zeroVec;
	}
	else if (currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend == D3DBLEND_ONE)
	{
		if (pixelWriteMask & 0x1) combinedSrc[0] = *(const __m128* const)&(srcColor[0]);
		if (pixelWriteMask & 0x2) combinedSrc[1] = *(const __m128* const)&(srcColor[1]);
		if (pixelWriteMask & 0x4) combinedSrc[2] = *(const __m128* const)&(srcColor[2]);
		if (pixelWriteMask & 0x8) combinedSrc[3] = *(const __m128* const)&(srcColor[3]);
	}
	else
	{
		if (pixelWriteMask & 0x1) combinedSrc[0] = _mm_mul_ps(*(const __m128* const)&(srcBlend[0]), *(const __m128* const)&(srcColor[0]) );
		if (pixelWriteMask & 0x2) combinedSrc[1] = _mm_mul_ps(*(const __m128* const)&(srcBlend[1]), *(const __m128* const)&(srcColor[1]) );
		if (pixelWriteMask & 0x4) combinedSrc[2] = _mm_mul_ps(*(const __m128* const)&(srcBlend[2]), *(const __m128* const)&(srcColor[2]) );
		if (pixelWriteMask & 0x8) combinedSrc[3] = _mm_mul_ps(*(const __m128* const)&(srcBlend[3]), *(const __m128* const)&(srcColor[3]) );
	}

	__m128 combinedDst[4];
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_ZERO)
	{
		if (pixelWriteMask & 0x1) combinedDst[0] = *(const __m128* const)&zeroVec;
		if (pixelWriteMask & 0x2) combinedDst[1] = *(const __m128* const)&zeroVec;
		if (pixelWriteMask & 0x4) combinedDst[2] = *(const __m128* const)&zeroVec;
		if (pixelWriteMask & 0x8) combinedDst[3] = *(const __m128* const)&zeroVec;
	}
	else if (currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend == D3DBLEND_ONE)
	{
		if (pixelWriteMask & 0x1) combinedDst[0] = *(const __m128* const)&(dstColor[0]);
		if (pixelWriteMask & 0x2) combinedDst[1] = *(const __m128* const)&(dstColor[1]);
		if (pixelWriteMask & 0x4) combinedDst[2] = *(const __m128* const)&(dstColor[2]);
		if (pixelWriteMask & 0x8) combinedDst[3] = *(const __m128* const)&(dstColor[3]);
	}
	else
	{
		if (pixelWriteMask & 0x1) combinedDst[0] = _mm_mul_ps(*(const __m128* const)&(dstBlend[0]), *(const __m128* const)&(dstColor[0]) );
		if (pixelWriteMask & 0x2) combinedDst[1] = _mm_mul_ps(*(const __m128* const)&(dstBlend[1]), *(const __m128* const)&(dstColor[1]) );		
		if (pixelWriteMask & 0x4) combinedDst[2] = _mm_mul_ps(*(const __m128* const)&(dstBlend[2]), *(const __m128* const)&(dstColor[2]) );
		if (pixelWriteMask & 0x8) combinedDst[3] = _mm_mul_ps(*(const __m128* const)&(dstBlend[3]), *(const __m128* const)&(dstColor[3]) );
	}

	// Simple SIMD version
	if ( (channelWriteMask & 0x7) == 0x7)
	{
		switch (blendOp)
		{
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Invalid D3DBLENDOP passed to alpha blending unit");
#else
			__assume(0);
#endif
		case D3DBLENDOP_ADD        :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outVec[0]) = _mm_add_ps(combinedSrc[0], combinedDst[0]);
			if (pixelWriteMask & 0x2) *(__m128* const)&(outVec[1]) = _mm_add_ps(combinedSrc[1], combinedDst[1]);
			if (pixelWriteMask & 0x4) *(__m128* const)&(outVec[2]) = _mm_add_ps(combinedSrc[2], combinedDst[2]);
			if (pixelWriteMask & 0x8) *(__m128* const)&(outVec[3]) = _mm_add_ps(combinedSrc[3], combinedDst[3]);
			return;
		case D3DBLENDOP_SUBTRACT   :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outVec[0]) = _mm_sub_ps(combinedSrc[0], combinedDst[0]);
			if (pixelWriteMask & 0x2) *(__m128* const)&(outVec[1]) = _mm_sub_ps(combinedSrc[1], combinedDst[1]);
			if (pixelWriteMask & 0x4) *(__m128* const)&(outVec[2]) = _mm_sub_ps(combinedSrc[2], combinedDst[2]);
			if (pixelWriteMask & 0x8) *(__m128* const)&(outVec[3]) = _mm_sub_ps(combinedSrc[3], combinedDst[3]);
			return;
		case D3DBLENDOP_REVSUBTRACT:
			if (pixelWriteMask& 0x1) *(__m128* const)&(outVec[0]) = _mm_sub_ps(combinedDst[0], combinedSrc[0]);
			if (pixelWriteMask& 0x2) *(__m128* const)&(outVec[1]) = _mm_sub_ps(combinedDst[1], combinedSrc[1]);
			if (pixelWriteMask& 0x4) *(__m128* const)&(outVec[2]) = _mm_sub_ps(combinedDst[2], combinedSrc[2]);
			if (pixelWriteMask& 0x8) *(__m128* const)&(outVec[3]) = _mm_sub_ps(combinedDst[3], combinedSrc[3]);
			return;
		case D3DBLENDOP_MIN        :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outVec[0]) = _mm_min_ps(combinedSrc[0], combinedDst[0]);
			if (pixelWriteMask & 0x2) *(__m128* const)&(outVec[1]) = _mm_min_ps(combinedSrc[1], combinedDst[1]);
			if (pixelWriteMask & 0x4) *(__m128* const)&(outVec[2]) = _mm_min_ps(combinedSrc[2], combinedDst[2]);
			if (pixelWriteMask & 0x8) *(__m128* const)&(outVec[3]) = _mm_min_ps(combinedSrc[3], combinedDst[3]);
			return;
		case D3DBLENDOP_MAX        :
			if (pixelWriteMask & 0x1) *(__m128* const)&(outVec[0]) = _mm_max_ps(combinedSrc[0], combinedDst[0]);
			if (pixelWriteMask & 0x2) *(__m128* const)&(outVec[1]) = _mm_max_ps(combinedSrc[1], combinedDst[1]);
			if (pixelWriteMask & 0x4) *(__m128* const)&(outVec[2]) = _mm_max_ps(combinedSrc[2], combinedDst[2]);
			if (pixelWriteMask & 0x8) *(__m128* const)&(outVec[3]) = _mm_max_ps(combinedSrc[3], combinedDst[3]);
			return;
		}

		return;
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
	{
		__m128 sum4[4];
		if (pixelWriteMask & 0x1) sum4[0] = _mm_add_ps(combinedSrc[0], combinedDst[0]);
		if (pixelWriteMask & 0x2) sum4[1] = _mm_add_ps(combinedSrc[1], combinedDst[1]);
		if (pixelWriteMask & 0x4) sum4[2] = _mm_add_ps(combinedSrc[2], combinedDst[2]);
		if (pixelWriteMask & 0x8) sum4[3] = _mm_add_ps(combinedSrc[3], combinedDst[3]);

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outVec[0].x = sum4[0].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[0].y = sum4[0].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[0].z = sum4[0].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[0].w = sum4[0].m128_f32[3];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outVec[1].x = sum4[1].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[1].y = sum4[1].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[1].z = sum4[1].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[1].w = sum4[1].m128_f32[3];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outVec[2].x = sum4[2].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[2].y = sum4[2].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[2].z = sum4[2].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[2].w = sum4[2].m128_f32[3];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outVec[3].x = sum4[3].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[3].y = sum4[3].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[3].z = sum4[3].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[3].w = sum4[3].m128_f32[3];
		}
	}
		break;
	case D3DBLENDOP_SUBTRACT   :
	{
		__m128 difference4[4];
		if (pixelWriteMask & 0x1) difference4[0] = _mm_sub_ps(combinedSrc[0], combinedDst[0]);
		if (pixelWriteMask & 0x2) difference4[1] = _mm_sub_ps(combinedSrc[1], combinedDst[1]);
		if (pixelWriteMask & 0x4) difference4[2] = _mm_sub_ps(combinedSrc[2], combinedDst[2]);
		if (pixelWriteMask & 0x8) difference4[3] = _mm_sub_ps(combinedSrc[3], combinedDst[3]);

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outVec[0].x = difference4[0].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[0].y = difference4[0].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[0].z = difference4[0].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[0].w = difference4[0].m128_f32[3];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outVec[1].x = difference4[1].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[1].y = difference4[1].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[1].z = difference4[1].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[1].w = difference4[1].m128_f32[3];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outVec[2].x = difference4[2].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[2].y = difference4[2].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[2].z = difference4[2].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[2].w = difference4[2].m128_f32[3];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outVec[3].x = difference4[3].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[3].y = difference4[3].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[3].z = difference4[3].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[3].w = difference4[3].m128_f32[3];
		}
	}
		break;
	case D3DBLENDOP_REVSUBTRACT:
	{
		__m128 revDifference4[4];
		if (pixelWriteMask & 0x1) revDifference4[0] = _mm_sub_ps(combinedDst[0], combinedSrc[0]);
		if (pixelWriteMask & 0x2) revDifference4[1] = _mm_sub_ps(combinedDst[1], combinedSrc[1]);
		if (pixelWriteMask & 0x4) revDifference4[2] = _mm_sub_ps(combinedDst[2], combinedSrc[2]);
		if (pixelWriteMask & 0x8) revDifference4[3] = _mm_sub_ps(combinedDst[3], combinedSrc[3]);

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outVec[0].x = revDifference4[0].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[0].y = revDifference4[0].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[0].z = revDifference4[0].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[0].w = revDifference4[0].m128_f32[3];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outVec[1].x = revDifference4[1].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[1].y = revDifference4[1].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[1].z = revDifference4[1].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[1].w = revDifference4[1].m128_f32[3];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outVec[2].x = revDifference4[2].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[2].y = revDifference4[2].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[2].z = revDifference4[2].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[2].w = revDifference4[2].m128_f32[3];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outVec[3].x = revDifference4[3].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[3].y = revDifference4[3].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[3].z = revDifference4[3].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[3].w = revDifference4[3].m128_f32[3];
		}
	}
		break;
	case D3DBLENDOP_MIN        :
	{
		__m128 minVec4[4];
		if (pixelWriteMask & 0x1) minVec4[0] = _mm_min_ps(combinedSrc[0], combinedDst[0]);
		if (pixelWriteMask & 0x2) minVec4[1] = _mm_min_ps(combinedSrc[1], combinedDst[1]);
		if (pixelWriteMask & 0x4) minVec4[2] = _mm_min_ps(combinedSrc[2], combinedDst[2]);
		if (pixelWriteMask & 0x8) minVec4[3] = _mm_min_ps(combinedSrc[3], combinedDst[3]);

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outVec[0].x = minVec4[0].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[0].y = minVec4[0].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[0].z = minVec4[0].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[0].w = minVec4[0].m128_f32[3];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outVec[1].x = minVec4[1].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[1].y = minVec4[1].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[1].z = minVec4[1].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[1].w = minVec4[1].m128_f32[3];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outVec[2].x = minVec4[2].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[2].y = minVec4[2].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[2].z = minVec4[2].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[2].w = minVec4[2].m128_f32[3];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outVec[3].x = minVec4[3].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[3].y = minVec4[3].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[3].z = minVec4[3].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[3].w = minVec4[3].m128_f32[3];
		}
	}
		break;
	case D3DBLENDOP_MAX        :
	{
		__m128 maxVec4[4];
		if (pixelWriteMask & 0x1) maxVec4[0] = _mm_max_ps(combinedSrc[0], combinedDst[0]);
		if (pixelWriteMask & 0x2) maxVec4[1] = _mm_max_ps(combinedSrc[1], combinedDst[1]);
		if (pixelWriteMask & 0x4) maxVec4[2] = _mm_max_ps(combinedSrc[2], combinedDst[2]);
		if (pixelWriteMask & 0x8) maxVec4[3] = _mm_max_ps(combinedSrc[3], combinedDst[3]);

		if (pixelWriteMask & 0x1)
		{
			if (channelWriteMask & 0x1)	outVec[0].x = maxVec4[0].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[0].y = maxVec4[0].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[0].z = maxVec4[0].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[0].w = maxVec4[0].m128_f32[3];
		}
		if (pixelWriteMask & 0x2)
		{
			if (channelWriteMask & 0x1)	outVec[1].x = maxVec4[1].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[1].y = maxVec4[1].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[1].z = maxVec4[1].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[1].w = maxVec4[1].m128_f32[3];
		}
		if (pixelWriteMask & 0x4)
		{
			if (channelWriteMask & 0x1)	outVec[2].x = maxVec4[2].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[2].y = maxVec4[2].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[2].z = maxVec4[2].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[2].w = maxVec4[2].m128_f32[3];
		}
		if (pixelWriteMask & 0x8)
		{
			if (channelWriteMask & 0x1)	outVec[3].x = maxVec4[3].m128_f32[0];
			if (channelWriteMask & 0x2)	outVec[3].y = maxVec4[3].m128_f32[1];
			if (channelWriteMask & 0x4)	outVec[3].z = maxVec4[3].m128_f32[2];
			if (channelWriteMask & 0x8)	outVec[3].w = maxVec4[3].m128_f32[3];
		}
	}
		break;
	}
}

// Apply a 4x4 dithering algorithm when writing to surfaces with lower precision than 8 bits per pixel:
static inline void DitherColor(const unsigned x, const unsigned y, D3DXVECTOR4& ditheredColor, const IDirect3DSurface9Hook* const surface)
{
	const unsigned xMod = x & 0x3;
	const unsigned yMod = y & 0x3;

	switch (surface->GetInternalFormat() )
	{
	default:
		return;
	case D3DFMT_R5G6B5               :
	case D3DFMT_A1R5G5B5             : // Don't dither the 1 bit, it won't look good
	case D3DFMT_X1R5G5B5             :
		ditheredColor.x += uniform5bit[xMod][yMod];
		ditheredColor.y += uniform6bit[xMod][yMod];
		ditheredColor.z += uniform5bit[xMod][yMod];
		break;
	case D3DFMT_A4R4G4B4             :
	case D3DFMT_A4L4                 :
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
	}
}

static const unsigned noAlphaMaskDWORDs[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 };
static const __m128 noAlphaMask = *(const __m128* const)noAlphaMaskDWORDs;
static const unsigned and3maskDWORDs[4] = { 0x3, 0x3, 0x3, 0x3 };
static const __m128i and3mask = *(const __m128i* const)and3maskDWORDs;
// Apply a 4x4 dithering algorithm when writing to surfaces with lower precision than 8 bits per pixel:
static inline void DitherColor4(const __m128i x4, const __m128i y4, D3DXVECTOR4 (&ditheredColor)[4], const IDirect3DSurface9Hook* const surface)
{
	const __m128i xMod4 = _mm_and_si128(x4, and3mask);
	const __m128i yMod4 = _mm_and_si128(y4, and3mask);

	const __m128i addressOffsetIndices = _mm_add_epi32(xMod4, _mm_slli_epi32(yMod4, 2) ); // Shift left by 2 (same as multiplying by 4, the array width)

	switch (surface->GetInternalFormat() )
	{
	default:
		return;
	case D3DFMT_R5G6B5               :
	case D3DFMT_A1R5G5B5             : // Don't dither the 1 bit, it won't look good
	case D3DFMT_X1R5G5B5             :
	{
		const __m128 uniform5bit4 = _mm_i32gather_ps(&uniform5bit[0][0], addressOffsetIndices, 4);
		const __m128 uniform6bit4 = _mm_i32gather_ps(&uniform6bit[0][0], addressOffsetIndices, 4);

		const __m128 blended5566[4] = 
		{
			_mm_shuffle_ps(uniform5bit4, uniform6bit4, _MM_SHUFFLE(0, 0, 0, 0) ),
			_mm_shuffle_ps(uniform5bit4, uniform6bit4, _MM_SHUFFLE(1, 1, 1, 1) ),
			_mm_shuffle_ps(uniform5bit4, uniform6bit4, _MM_SHUFFLE(2, 2, 2, 2) ),
			_mm_shuffle_ps(uniform5bit4, uniform6bit4, _MM_SHUFFLE(3, 3, 3, 3) )
		};

		const __m128 blended5655[4] = 
		{
			_mm_shuffle_ps(blended5566[0], blended5566[0], _MM_SHUFFLE(0, 2, 0, 0) ),
			_mm_shuffle_ps(blended5566[1], blended5566[1], _MM_SHUFFLE(0, 2, 0, 0) ),
			_mm_shuffle_ps(blended5566[2], blended5566[2], _MM_SHUFFLE(0, 2, 0, 0) ),
			_mm_shuffle_ps(blended5566[3], blended5566[3], _MM_SHUFFLE(0, 2, 0, 0) )
		};

		const __m128 blended5650[4] = 
		{
			_mm_and_ps(blended5655[0], noAlphaMask),
			_mm_and_ps(blended5655[1], noAlphaMask),
			_mm_and_ps(blended5655[2], noAlphaMask),
			_mm_and_ps(blended5655[3], noAlphaMask)
		};

		*(__m128* const)&(ditheredColor[0]) = _mm_add_ps(blended5650[0], *(const __m128* const)&(ditheredColor[0]) );
		*(__m128* const)&(ditheredColor[1]) = _mm_add_ps(blended5650[1], *(const __m128* const)&(ditheredColor[1]) );
		*(__m128* const)&(ditheredColor[2]) = _mm_add_ps(blended5650[2], *(const __m128* const)&(ditheredColor[2]) );
		*(__m128* const)&(ditheredColor[3]) = _mm_add_ps(blended5650[3], *(const __m128* const)&(ditheredColor[3]) );
	}
		break;
	case D3DFMT_A4R4G4B4             :
	case D3DFMT_A4L4                 :
	case D3DFMT_X4R4G4B4             :
	{
		const __m128 uniform4bit4 = _mm_i32gather_ps(&uniform4bit[0][0], addressOffsetIndices, 4);
		__m128 ditherVectors[4];
		ditherVectors[0] = _mm_shuffle_ps(uniform4bit4, uniform4bit4, _MM_SHUFFLE(0, 0, 0, 0) );
		ditherVectors[1] = _mm_shuffle_ps(uniform4bit4, uniform4bit4, _MM_SHUFFLE(1, 1, 1, 1) );
		ditherVectors[2] = _mm_shuffle_ps(uniform4bit4, uniform4bit4, _MM_SHUFFLE(2, 2, 2, 2) );
		ditherVectors[3] = _mm_shuffle_ps(uniform4bit4, uniform4bit4, _MM_SHUFFLE(3, 3, 3, 3) );

		*(__m128* const)&(ditheredColor[0]) = _mm_add_ps(ditherVectors[0], *(const __m128* const)&(ditheredColor[0]) );
		*(__m128* const)&(ditheredColor[1]) = _mm_add_ps(ditherVectors[1], *(const __m128* const)&(ditheredColor[1]) );
		*(__m128* const)&(ditheredColor[2]) = _mm_add_ps(ditherVectors[2], *(const __m128* const)&(ditheredColor[2]) );
		*(__m128* const)&(ditheredColor[3]) = _mm_add_ps(ditherVectors[3], *(const __m128* const)&(ditheredColor[3]) );
	}
		break;
	case D3DFMT_A8R3G3B2             :
	{
		const __m128 uniform3bit4 = _mm_i32gather_ps(&uniform3bit[0][0], addressOffsetIndices, 4);
		const __m128 uniform2bit4 = _mm_i32gather_ps(&uniform2bit[0][0], addressOffsetIndices, 4);

		const __m128 blended3322[4] = 
		{
			_mm_shuffle_ps(uniform3bit4, uniform2bit4, _MM_SHUFFLE(0, 0, 0, 0) ),
			_mm_shuffle_ps(uniform3bit4, uniform2bit4, _MM_SHUFFLE(1, 1, 1, 1) ),
			_mm_shuffle_ps(uniform3bit4, uniform2bit4, _MM_SHUFFLE(2, 2, 2, 2) ),
			_mm_shuffle_ps(uniform3bit4, uniform2bit4, _MM_SHUFFLE(3, 3, 3, 3) )
		};

		const __m128 blended3320[4] = 
		{
			_mm_and_ps(blended3322[0], noAlphaMask),
			_mm_and_ps(blended3322[1], noAlphaMask),
			_mm_and_ps(blended3322[2], noAlphaMask),
			_mm_and_ps(blended3322[3], noAlphaMask)
		};

		*(__m128* const)&(ditheredColor[0]) = _mm_add_ps(blended3320[0], *(const __m128* const)&(ditheredColor[0]) );
		*(__m128* const)&(ditheredColor[1]) = _mm_add_ps(blended3320[1], *(const __m128* const)&(ditheredColor[1]) );
		*(__m128* const)&(ditheredColor[2]) = _mm_add_ps(blended3320[2], *(const __m128* const)&(ditheredColor[2]) );
		*(__m128* const)&(ditheredColor[3]) = _mm_add_ps(blended3320[3], *(const __m128* const)&(ditheredColor[3]) );
	}
		break;
	}
}

template <const unsigned char channelWriteMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const
{
	if (currentState.currentRenderStates.simplifiedAlphaBlendMode > RenderStates::noAlphaBlending)
	{
		if (currentState.currentRenderStates.simplifiedAlphaBlendMode != RenderStates::otherAlphaBlending)
			ROPBlendWriteMask_AlphaBlendTest<channelWriteMask>(outSurface, x, y, value); // Alpha blend skip / Additive blend skip test
		else
			ROPBlendWriteMask_AlphaBlend<channelWriteMask>(outSurface, x, y, value);
	}
	else // Super simple - no alpha blending at all!
	{
		ROPBlendWriteMask_NoAlphaBlend<channelWriteMask>(outSurface, x, y, value);
	}
}

template <const unsigned char channelWriteMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask_AlphaBlendTest(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const
{
	switch (currentState.currentRenderStates.simplifiedAlphaBlendMode)
	{
	default:
	case RenderStates::otherAlphaBlending:
	case RenderStates::noAlphaBlending:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
		return;
	case RenderStates::alphaBlending:
	{
		if (value.w == 0.0f)
			return;
	}
		break;
	case RenderStates::additiveBlending:
	{
		// TODO: SIMDify this check
		if (value.x == 0.0f && value.y == 0.0f && value.z == 0.0f)
			return;
	}
		break;
	case RenderStates::multiplicativeBlending:
	{
		// TODO: SIMDify this check
		if (value.x == 1.0f && value.y == 1.0f && value.z == 1.0f)
			return;
	}
		break;
	}

	ROPBlendWriteMask_AlphaBlend<channelWriteMask>(outSurface, x, y, value);
}

template <const unsigned char channelWriteMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask_AlphaBlend(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const
{
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.separateAlphaBlendEnable) // Have to run alpha blending twice for separate alpha
	{
		// Read the dest color in from the rendertarget (read can be skipped if our alpha-blending mode is set up such that we know we'll never use the read data)
		D3DXVECTOR4 dstColor, finalColor;
		if (currentState.currentRenderStates.alphaBlendNeedsDestRead)
			outSurface->GetPixelVec<channelWriteMask, false>(x, y, dstColor);

		D3DXVECTOR4 srcBlendColor, dstBlendColor;
		LoadBlend<channelWriteMask & 0x7>(srcBlendColor, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend, value, dstColor);
		LoadBlend<channelWriteMask & 0x7>(dstBlendColor, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend, value, dstColor);
		AlphaBlend<channelWriteMask & 0x7>(finalColor, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOp, srcBlendColor, dstBlendColor, value, dstColor);

		if (channelWriteMask & 0x8)
		{
			D3DXVECTOR4 srcBlendAlpha, dstBlendAlpha, finalAlpha;
			LoadBlend<channelWriteMask & 0x8>(srcBlendAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlendAlpha, value, dstColor);
			LoadBlend<channelWriteMask & 0x8>(dstBlendAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlendAlpha, value, dstColor);
			AlphaBlend<channelWriteMask & 0x8>(finalAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOpAlpha, srcBlendAlpha, dstBlendAlpha, value, dstColor);
			finalColor.w = finalAlpha.w;
		}

		if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
			DitherColor(x, y, finalColor, outSurface);

		outSurface->SetPixelVec<channelWriteMask>(x, y, finalColor);
	}
	else // Simple alpha blending without separate alpha
	{
		// Read the dest color in from the rendertarget (read can be skipped if our alpha-blending mode is set up such that we know we'll never use the read data)
		__declspec(align(16) ) D3DXVECTOR4 dstColor;
		if (currentState.currentRenderStates.alphaBlendNeedsDestRead)
			outSurface->GetPixelVec<channelWriteMask, false>(x, y, dstColor);

		__declspec(align(16) ) D3DXVECTOR4 srcBlend;
		__declspec(align(16) ) D3DXVECTOR4 dstBlend;
		LoadBlend<channelWriteMask>(srcBlend, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend, value, dstColor);
		LoadBlend<channelWriteMask>(dstBlend, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend, value, dstColor);
		AlphaBlend<channelWriteMask>(dstColor, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOp, srcBlend, dstBlend, value, dstColor);

		if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
			DitherColor(x, y, dstColor, outSurface);

		outSurface->SetPixelVec<channelWriteMask>(x, y, dstColor);
	}
}

template <const unsigned char channelWriteMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask_NoAlphaBlend(IDirect3DSurface9Hook* const outSurface, const unsigned x, const unsigned y, const D3DXVECTOR4& value) const
{
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
	{
		D3DXVECTOR4 ditheredColor = value;
		DitherColor(x, y, ditheredColor, outSurface);
		outSurface->SetPixelVec<channelWriteMask>(x, y, ditheredColor);
	}
	else
		outSurface->SetPixelVec<channelWriteMask>(x, y, value);
}

template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask4(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const
{
	if (currentState.currentRenderStates.simplifiedAlphaBlendMode > RenderStates::noAlphaBlending)
	{
		if (currentState.currentRenderStates.simplifiedAlphaBlendMode != RenderStates::otherAlphaBlending)
			ROPBlendWriteMask4_AlphaBlendTest<channelWriteMask, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		else
			ROPBlendWriteMask4_AlphaBlend<channelWriteMask, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
	}
	else // Super simple - no alpha blending at all!
	{
		ROPBlendWriteMask4_NoAlphaBlend<channelWriteMask, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
	}
}

static_assert(sizeof(PS_2_0_OutputRegisters) % sizeof(float) == 0, "Error! Unexpected struct size!");
static const unsigned gatherAlphaValues4bytes[4] = { 0, sizeof(PS_2_0_OutputRegisters) / sizeof(float), 2 * sizeof(PS_2_0_OutputRegisters) / sizeof(float), 3 * sizeof(PS_2_0_OutputRegisters) / sizeof(float) };
static const __m128i gatherAlphaValues4 = *(const __m128i* const)gatherAlphaValues4bytes;
static const __m128i additiveBlendCompareByteShuffle = _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0);

template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask4_AlphaBlendTest(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const
{
	// Alpha blend skip:
	unsigned char failAlphaBlendMask = 0x00;
	switch (currentState.currentRenderStates.simplifiedAlphaBlendMode)
	{
	default:
	case RenderStates::otherAlphaBlending:
	case RenderStates::noAlphaBlending:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
		return;
	case RenderStates::alphaBlending:
	{
		const __declspec(align(16) ) __m128 alphaValue4 = _mm_i32gather_ps(&(outputRegisters[0].oC[RTIndex].w), gatherAlphaValues4, 4);
		failAlphaBlendMask = (const unsigned char)_mm_movemask_ps(_mm_cmpeq_ps(alphaValue4, *(const __m128* const)&zeroVec) );
	}
		break;
	case RenderStates::additiveBlending:
	{
		__declspec(align(16) ) __m128i masks;
		if (pixelWriteMask & 0x1)
			masks.m128i_u32[0] = _mm_movemask_ps(_mm_cmpeq_ps(*(const __m128* const)&(outputRegisters[0].oC[RTIndex]), *(const __m128* const)&zeroVec) );
		if (pixelWriteMask & 0x2)
			masks.m128i_u32[1] = _mm_movemask_ps(_mm_cmpeq_ps(*(const __m128* const)&(outputRegisters[1].oC[RTIndex]), *(const __m128* const)&zeroVec) );
		if (pixelWriteMask & 0x4)
			masks.m128i_u32[2] = _mm_movemask_ps(_mm_cmpeq_ps(*(const __m128* const)&(outputRegisters[2].oC[RTIndex]), *(const __m128* const)&zeroVec) );
		if (pixelWriteMask & 0x8)
			masks.m128i_u32[3] = _mm_movemask_ps(_mm_cmpeq_ps(*(const __m128* const)&(outputRegisters[3].oC[RTIndex]), *(const __m128* const)&zeroVec) );

		failAlphaBlendMask = (const unsigned char)_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(masks, sevenVec), sevenVec) ) );
	}
		break;
	case RenderStates::multiplicativeBlending:
	{
		__declspec(align(16) ) __m128i masks;
		if (pixelWriteMask & 0x1)
			masks.m128i_u32[0] = _mm_movemask_ps(_mm_cmpeq_ps(*(const __m128* const)&(outputRegisters[0].oC[RTIndex]), *(const __m128* const)&staticColorWhiteOpaque) );
		if (pixelWriteMask & 0x2)
			masks.m128i_u32[1] = _mm_movemask_ps(_mm_cmpeq_ps(*(const __m128* const)&(outputRegisters[1].oC[RTIndex]), *(const __m128* const)&staticColorWhiteOpaque) );
		if (pixelWriteMask & 0x4)
			masks.m128i_u32[2] = _mm_movemask_ps(_mm_cmpeq_ps(*(const __m128* const)&(outputRegisters[2].oC[RTIndex]), *(const __m128* const)&staticColorWhiteOpaque) );
		if (pixelWriteMask & 0x8)
			masks.m128i_u32[3] = _mm_movemask_ps(_mm_cmpeq_ps(*(const __m128* const)&(outputRegisters[3].oC[RTIndex]), *(const __m128* const)&staticColorWhiteOpaque) );

		failAlphaBlendMask = (const unsigned char)_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(masks, sevenVec), sevenVec) ) );
	}
		break;
	}

	const unsigned char newPixelWriteMask = pixelWriteMask & (~failAlphaBlendMask);
	switch (newPixelWriteMask)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case 0x0:
		return; // No pixels left, we're done!
	case 0x1:
		ROPBlendWriteMask_AlphaBlend<channelWriteMask>(outSurface, x4.m128i_u32[0], y4.m128i_u32[0], *(const D3DXVECTOR4* const)&(outputRegisters[0].oC[RTIndex]) );
		return;
	case 0x2:
		ROPBlendWriteMask_AlphaBlend<channelWriteMask>(outSurface, x4.m128i_u32[1], y4.m128i_u32[1], *(const D3DXVECTOR4* const)&(outputRegisters[1].oC[RTIndex]) );
		return;
	case 0x4:
		ROPBlendWriteMask_AlphaBlend<channelWriteMask>(outSurface, x4.m128i_u32[2], y4.m128i_u32[2], *(const D3DXVECTOR4* const)&(outputRegisters[2].oC[RTIndex]) );
		return;
	case 0x8:
		ROPBlendWriteMask_AlphaBlend<channelWriteMask>(outSurface, x4.m128i_u32[3], y4.m128i_u32[3], *(const D3DXVECTOR4* const)&(outputRegisters[3].oC[RTIndex]) );
		return;
	case 0x3:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0x3>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0x5:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0x5>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0x6:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0x6>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0x7:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0x7>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0x9:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0x9>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0xA:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0xA>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0xB:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0xB>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0xC:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0xC>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0xD:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0xD>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0xE:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0xE>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	case 0xF:
		ROPBlendWriteMask4_AlphaBlend<channelWriteMask, 0xF>(outSurface, x4, y4, outputRegisters, RTIndex);
		return;
	}
}

template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask4_AlphaBlend(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const
{
	// TODO: This kinda sucks. We should make a version of the blend functions that can blend pixels without copying them first
	__declspec(align(16) ) D3DXVECTOR4 colorCopy[4];
	if (pixelWriteMask & 0x1) colorCopy[0] = *(const D3DXVECTOR4* const)&(outputRegisters[0].oC[RTIndex]);
	if (pixelWriteMask & 0x2) colorCopy[1] = *(const D3DXVECTOR4* const)&(outputRegisters[1].oC[RTIndex]);
	if (pixelWriteMask & 0x4) colorCopy[2] = *(const D3DXVECTOR4* const)&(outputRegisters[2].oC[RTIndex]);
	if (pixelWriteMask & 0x8) colorCopy[3] = *(const D3DXVECTOR4* const)&(outputRegisters[3].oC[RTIndex]);

	if (currentState.currentRenderStates.renderStatesUnion.namedStates.separateAlphaBlendEnable) // Have to run alpha blending twice for separate alpha
	{
		__declspec(align(16) ) D3DXVECTOR4 dstColor[4];
		__declspec(align(16) ) D3DXVECTOR4 finalColor[4];

		// Read the dest color in from the rendertarget (read can be skipped if our alpha-blending mode is set up such that we know we'll never use the read data)
		if (currentState.currentRenderStates.alphaBlendNeedsDestRead)
			outSurface->GetPixelVec4<channelWriteMask, false, pixelWriteMask>(x4, y4, dstColor);

		__declspec(align(16) ) D3DXVECTOR4 srcBlendColor[4];
		__declspec(align(16) ) D3DXVECTOR4 dstBlendColor[4];
		LoadBlend4<channelWriteMask & 0x7, pixelWriteMask>(srcBlendColor, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend, colorCopy, dstColor);
		LoadBlend4<channelWriteMask & 0x7, pixelWriteMask>(dstBlendColor, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend, colorCopy, dstColor);
		AlphaBlend4<channelWriteMask & 0x7, pixelWriteMask>(finalColor, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOp, srcBlendColor, dstBlendColor, colorCopy, dstColor);

		if (channelWriteMask & 0x8)
		{
			__declspec(align(16) ) D3DXVECTOR4 srcBlendAlpha[4];
			__declspec(align(16) ) D3DXVECTOR4 dstBlendAlpha[4];
			__declspec(align(16) ) D3DXVECTOR4 finalAlpha[4];
			LoadBlend4<channelWriteMask & 0x8, pixelWriteMask>(srcBlendAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlendAlpha, colorCopy, dstColor);
			LoadBlend4<channelWriteMask & 0x8, pixelWriteMask>(dstBlendAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlendAlpha, colorCopy, dstColor);
			AlphaBlend4<channelWriteMask & 0x8, pixelWriteMask>(finalAlpha, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOpAlpha, srcBlendAlpha, dstBlendAlpha, colorCopy, dstColor);
			if (pixelWriteMask & 0x1) finalColor[0].w = finalAlpha[0].w;
			if (pixelWriteMask & 0x2) finalColor[1].w = finalAlpha[1].w;
			if (pixelWriteMask & 0x4) finalColor[2].w = finalAlpha[2].w;
			if (pixelWriteMask & 0x8) finalColor[3].w = finalAlpha[3].w;
		}

		if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
			DitherColor4(x4, y4, finalColor, outSurface);

		outSurface->SetPixelVec4<channelWriteMask, pixelWriteMask>(x4, y4, finalColor);
	}
	else // Simple alpha blending without separate alpha
	{
		// Read the dest color in from the rendertarget (read can be skipped if our alpha-blending mode is set up such that we know we'll never use the read data)
		__declspec(align(16) ) D3DXVECTOR4 dstColor[4];
		if (currentState.currentRenderStates.alphaBlendNeedsDestRead)
			outSurface->GetPixelVec4<channelWriteMask, false, pixelWriteMask>(x4, y4, dstColor);

		__declspec(align(16) ) D3DXVECTOR4 srcBlend[4];
		__declspec(align(16) ) D3DXVECTOR4 dstBlend[4];
		LoadBlend4<channelWriteMask, pixelWriteMask>(srcBlend, currentState.currentRenderStates.renderStatesUnion.namedStates.srcBlend, colorCopy, dstColor);
		LoadBlend4<channelWriteMask, pixelWriteMask>(dstBlend, currentState.currentRenderStates.renderStatesUnion.namedStates.destBlend, colorCopy, dstColor);
		AlphaBlend4<channelWriteMask, pixelWriteMask>(dstColor, currentState.currentRenderStates.renderStatesUnion.namedStates.blendOp, srcBlend, dstBlend, colorCopy, dstColor);

		if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
			DitherColor4(x4, y4, dstColor, outSurface);

		outSurface->SetPixelVec4<channelWriteMask, pixelWriteMask>(x4, y4, dstColor);
	}
}

template <const unsigned char channelWriteMask, const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::ROPBlendWriteMask4_NoAlphaBlend(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const
{
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.ditherEnable)
	{
		__declspec(align(16) ) D3DXVECTOR4 ditheredColor[4];
		if (pixelWriteMask & 0x1) ditheredColor[0] = *(const D3DXVECTOR4* const)&(outputRegisters[0].oC[RTIndex]);
		if (pixelWriteMask & 0x2) ditheredColor[1] = *(const D3DXVECTOR4* const)&(outputRegisters[1].oC[RTIndex]);
		if (pixelWriteMask & 0x4) ditheredColor[2] = *(const D3DXVECTOR4* const)&(outputRegisters[2].oC[RTIndex]);
		if (pixelWriteMask & 0x8) ditheredColor[3] = *(const D3DXVECTOR4* const)&(outputRegisters[3].oC[RTIndex]);
		DitherColor4(x4, y4, ditheredColor, outSurface);
		outSurface->SetPixelVec4<channelWriteMask, pixelWriteMask>(x4, y4, ditheredColor);
	}
	else
	{
		// TODO: This kinda sucks. We should make a version of SetPixelVec4 that can write out pixels without copying them first
		__declspec(align(16) ) D3DXVECTOR4 colorCopy[4];
		if (pixelWriteMask & 0x1) colorCopy[0] = *(const D3DXVECTOR4* const)&(outputRegisters[0].oC[RTIndex]);
		if (pixelWriteMask & 0x2) colorCopy[1] = *(const D3DXVECTOR4* const)&(outputRegisters[1].oC[RTIndex]);
		if (pixelWriteMask & 0x4) colorCopy[2] = *(const D3DXVECTOR4* const)&(outputRegisters[2].oC[RTIndex]);
		if (pixelWriteMask & 0x8) colorCopy[3] = *(const D3DXVECTOR4* const)&(outputRegisters[3].oC[RTIndex]);
		outSurface->SetPixelVec4<channelWriteMask, pixelWriteMask>(x4, y4, colorCopy);
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

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::RenderOutput4(IDirect3DSurface9Hook* const outSurface, const __m128i x4, const __m128i y4, const PS_2_0_OutputRegisters (&outputRegisters)[4], const unsigned char RTIndex) const
{
	// Increment pixels written stat (even if write mask is 0)
	switch (pixelWriteMask)
	{
	default:
	case 0x0:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
		return;
	case 0x1:
	case 0x2:
	case 0x4:
	case 0x8:
		++frameStats.numPixelsWritten;
		break;
	case 0x3:
	case 0x5:
	case 0x6:
	case 0x9:
	case 0xA:
	case 0xC:
		frameStats.numPixelsWritten += 2;
		break;
	case 0x7:
	case 0xB:
	case 0xD:
	case 0xE:
		frameStats.numPixelsWritten += 3;
		break;
	case 0xF:
		frameStats.numPixelsWritten += 4;
		break;
	}

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
		ROPBlendWriteMask4<1, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 2:
		ROPBlendWriteMask4<2, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 3:
		ROPBlendWriteMask4<3, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 4:
		ROPBlendWriteMask4<4, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 5:
		ROPBlendWriteMask4<5, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 6:
		ROPBlendWriteMask4<6, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 7:
		ROPBlendWriteMask4<7, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 8:
		ROPBlendWriteMask4<8, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 9:
		ROPBlendWriteMask4<9, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 10:
		ROPBlendWriteMask4<10, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 11:
		ROPBlendWriteMask4<11, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 12:
		ROPBlendWriteMask4<12, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 13:
		ROPBlendWriteMask4<13, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	case 14:
		ROPBlendWriteMask4<14, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Unexpected write mask!");
#else
		__assume(0);
#endif
	case 15:
		ROPBlendWriteMask4<15, pixelWriteMask>(outSurface, x4, y4, outputRegisters, RTIndex);
		break;
	}
}

// Must be called before shading a pixel to reset the pixel shader state machine!
void IDirect3DDevice9Hook::PreShadePixel(const unsigned x, const unsigned y, PShaderEngine* const pixelShader) const
{
	pixelShader->Reset(x, y);
}

// Must be called before shading a pixel to reset the pixel shader state machine!
void IDirect3DDevice9Hook::PreShadePixel4(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const
{
	pixelShader->Reset4(x4, y4);
}

// Handles pixel setup and depth and attribute interpolation before shading the pixel
template <const bool setupFromShader>
void IDirect3DDevice9Hook::SetupPixel(PShaderEngine* const pixelEngine, const void* const shaderOrStreamMapping, const unsigned x, const unsigned y, const __m128 barycentricInterpolants, 
	const UINT offsetBytesToOPosition, const void* const v0, const void* const v1, const void* const v2, const __m128 invZ) const
{
	const float pixelDepth = InterpolatePixelDepth(barycentricInterpolants, invZ);

	// Very important to reset the state machine back to its original settings!
	PreShadePixel(x, y, pixelEngine);

	if (currentState.currentDepthStencil)
	{
		if (!StencilTestNoWrite(x, y) )
		{
			// Fail the stencil test!
			ShadePixel_FailStencil(x, y);
			return;
		}

		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zEnable)
		{
			const unsigned bufferDepth = currentState.currentDepthStencil->GetRawDepth(x, y);
			if (!DepthTest(pixelDepth, bufferDepth, currentState.currentRenderStates.renderStatesUnion.namedStates.zFunc, currentState.currentDepthStencil->GetInternalFormat() ) )
			{
				// Fail the depth test!
				ShadePixel_FailDepth(x, y);
				return;
			}
			pixelEngine->outputRegisters[0].oDepth = pixelDepth;
		}
	}

	// Precompute some vectors that will be used for all of attribute interpolation
	const __m128 floatBarycentricsInvZ = _mm_mul_ps(invZ, barycentricInterpolants);
	const __m128 floatBarycentricsInvZ_X = _mm_permute_ps(floatBarycentricsInvZ, _MM_SHUFFLE(0, 0, 0, 0) );
	const __m128 floatBarycentricsInvZ_Y = _mm_permute_ps(floatBarycentricsInvZ, _MM_SHUFFLE(1, 1, 1, 1) );
	const __m128 floatBarycentricsInvZ_Z = _mm_permute_ps(floatBarycentricsInvZ, _MM_SHUFFLE(2, 2, 2, 2) );
	if (setupFromShader)
	{
		InterpolateShaderIntoRegisters(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping,  
			*(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, pixelDepth,
			floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z);
	}
	else
	{
		InterpolateStreamIntoRegisters(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, 
			(const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, pixelDepth,
			floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z);
	}

	ShadePixel_RunShader(x, y, pixelEngine);
}

// Handles pixel quad setup and depth and attribute interpolation before shading the pixel quad
template <const bool setupFromShader>
void IDirect3DDevice9Hook::SetupPixel4(PShaderEngine* const pixelEngine, const void* const shaderOrStreamMapping, const __m128i x4, const __m128i y4, const __m128 (&barycentricInterpolants)[4], 
	const UINT offsetBytesToOPosition, const void* const v0, const void* const v1, const void* const v2, const __m128 invZ) const
{
	__m128 pixelDepth4;
	InterpolatePixelDepth4(barycentricInterpolants, invZ, pixelDepth4);

	// Very important to reset the state machine back to its original settings!
	PreShadePixel4(x4, y4, pixelEngine);

	unsigned char pixelWriteMask = 0xF;
	if (currentState.currentDepthStencil)
	{
		// TODO: Make a StencilTestNoWrite4
		/*for (unsigned z = 0; z < 4; ++z)
		{
			if (!StencilTestNoWrite(x4.m128i_i32[z], y4.m128i_i32[z]) )
			{
				// Fail the stencil test!
				pixelOutput4[z].pixelStatus = stencilFail;
				ShadePixel(x, y, pixelEngine);
				return;
			}
		}*/

		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zEnable)
		{
			const __m128i bufferDepth4 = currentState.currentDepthStencil->GetRawDepth4(x4, y4);
			const __m128i depthTestResults = DepthTest4(pixelDepth4, bufferDepth4, currentState.currentRenderStates.renderStatesUnion.namedStates.zFunc, currentState.currentDepthStencil->GetInternalFormat() );
			const unsigned char maskBits = _mm_movemask_ps(_mm_cvtepi32_ps(depthTestResults) );
			if (maskBits == 0x0)
				return;
			pixelWriteMask = maskBits;
			pixelEngine->outputRegisters[0].oDepth = pixelDepth4.m128_f32[0];
			pixelEngine->outputRegisters[0].pixelStatus = (maskBits & 0x1) ? normalWrite : ZFail;
			pixelEngine->outputRegisters[1].oDepth = pixelDepth4.m128_f32[1];
			pixelEngine->outputRegisters[1].pixelStatus = (maskBits & 0x2) ? normalWrite : ZFail;
			pixelEngine->outputRegisters[2].oDepth = pixelDepth4.m128_f32[2];
			pixelEngine->outputRegisters[2].pixelStatus = (maskBits & 0x4) ? normalWrite : ZFail;
			pixelEngine->outputRegisters[3].oDepth = pixelDepth4.m128_f32[3];
			pixelEngine->outputRegisters[3].pixelStatus = (maskBits & 0x8) ? normalWrite : ZFail;
		}
	}

	// Precompute some vectors that will be used for all of attribute interpolation
	const __m128 floatBarycentricsInvZ4[4] = 
	{
		(pixelWriteMask & 0x1) ? _mm_mul_ps(invZ, barycentricInterpolants[0]) : _mm_setzero_ps(),
		(pixelWriteMask & 0x2) ? _mm_mul_ps(invZ, barycentricInterpolants[1]) : _mm_setzero_ps(),
		(pixelWriteMask & 0x4) ? _mm_mul_ps(invZ, barycentricInterpolants[2]) : _mm_setzero_ps(),
		(pixelWriteMask & 0x8) ? _mm_mul_ps(invZ, barycentricInterpolants[3]) : _mm_setzero_ps()
	};
	const __m128 floatBarycentricsInvZ_X4[4] = 
	{
		_mm_permute_ps(floatBarycentricsInvZ4[0], _MM_SHUFFLE(0, 0, 0, 0) ),
		_mm_permute_ps(floatBarycentricsInvZ4[1], _MM_SHUFFLE(0, 0, 0, 0) ),
		_mm_permute_ps(floatBarycentricsInvZ4[2], _MM_SHUFFLE(0, 0, 0, 0) ),
		_mm_permute_ps(floatBarycentricsInvZ4[3], _MM_SHUFFLE(0, 0, 0, 0) )
	};
	const __m128 floatBarycentricsInvZ_Y4[4] = 
	{
		_mm_permute_ps(floatBarycentricsInvZ4[0], _MM_SHUFFLE(1, 1, 1, 1) ),
		_mm_permute_ps(floatBarycentricsInvZ4[1], _MM_SHUFFLE(1, 1, 1, 1) ),
		_mm_permute_ps(floatBarycentricsInvZ4[2], _MM_SHUFFLE(1, 1, 1, 1) ),
		_mm_permute_ps(floatBarycentricsInvZ4[3], _MM_SHUFFLE(1, 1, 1, 1) )
	};
	const __m128 floatBarycentricsInvZ_Z4[4] = 
	{
		_mm_permute_ps(floatBarycentricsInvZ4[0], _MM_SHUFFLE(2, 2, 2, 2) ),
		_mm_permute_ps(floatBarycentricsInvZ4[1], _MM_SHUFFLE(2, 2, 2, 2) ),
		_mm_permute_ps(floatBarycentricsInvZ4[2], _MM_SHUFFLE(2, 2, 2, 2) ),
		_mm_permute_ps(floatBarycentricsInvZ4[3], _MM_SHUFFLE(2, 2, 2, 2) )
	};

	switch (pixelWriteMask)
	{
	case 0x1:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x1>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x1>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x1>(x4, y4, pixelEngine);
		break;
	case 0x2:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x2>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x2>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x2>(x4, y4, pixelEngine);
		break;
	case 0x3:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x3>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x3>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x3>(x4, y4, pixelEngine);
		break;
	case 0x4:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x4>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x4>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x4>(x4, y4, pixelEngine);
		break;
	case 0x5:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x5>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x5>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x5>(x4, y4, pixelEngine);
		break;
	case 0x6:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x6>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x6>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x6>(x4, y4, pixelEngine);
		break;
	case 0x7:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x7>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x7>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x7>(x4, y4, pixelEngine);
		break;
	case 0x8:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x8>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x8>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x8>(x4, y4, pixelEngine);
		break;
	case 0x9:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0x9>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0x9>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0x9>(x4, y4, pixelEngine);
		break;
	case 0xA:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0xA>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0xA>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0xA>(x4, y4, pixelEngine);
		break;
	case 0xB:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0xB>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0xB>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0xB>(x4, y4, pixelEngine);
		break;
	case 0xC:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0xC>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0xC>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0xC>(x4, y4, pixelEngine);
		break;
	case 0xD:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0xD>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0xD>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0xD>(x4, y4, pixelEngine);
		break;
	case 0xE:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0xE>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0xE>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0xE>(x4, y4, pixelEngine);
		break;
	default:
#ifdef _DEBUG
		__debugbreak(); // Should never be here!
#else
		__assume(0);
#endif
	case 0xF:
		if (setupFromShader)
			InterpolateShaderIntoRegisters4<0xF>(pixelEngine, *(const VStoPSMapping* const)shaderOrStreamMapping, *(const VS_2_0_OutputRegisters* const)v0, *(const VS_2_0_OutputRegisters* const)v1, *(const VS_2_0_OutputRegisters* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		else
			InterpolateStreamIntoRegisters4<0xF>(pixelEngine, *(const DeclarationSemanticMapping* const)shaderOrStreamMapping, (const BYTE* const)v0, (const BYTE* const)v1, (const BYTE* const)v2, 
				pixelDepth4, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4);
		ShadePixel4_RunShader<0xF>(x4, y4, pixelEngine);
		break;
	}
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
static inline void InterpolateVertexAttribute_PerspectiveCorrect(const D3DXVECTOR4& attr0, const D3DXVECTOR4& attr1, const D3DXVECTOR4& attr2, const __m128 floatBarycentricsInvZ_X, const __m128 floatBarycentricsInvZ_Y, const __m128 floatBarycentricsInvZ_Z, const float pixelZ, D3DXVECTOR4& outAttr)
{
	const __m128* const attr0ptr = (const __m128* const)&attr0;
	const __m128* const attr1ptr = (const __m128* const)&attr1;
	const __m128* const attr2ptr = (const __m128* const)&attr2;

	// I think that mul, mul, mul, add, add is the best we can do here since the attribute data comes in and needs a transpose, which prevents us from using dotproduct3
	const __m128 mulResultA = _mm_mul_ps(floatBarycentricsInvZ_X, *attr0ptr);
	const __m128 mulResultB = _mm_mul_ps(floatBarycentricsInvZ_Y, *attr1ptr);
	const __m128 mulResultC = _mm_mul_ps(floatBarycentricsInvZ_Z, *attr2ptr);

	const __m128 result = _mm_add_ps(_mm_add_ps(mulResultA, mulResultB), mulResultC);

	__m128* const outAttrPtr = (__m128* const)&outAttr;

	const __m128 pixelZ_float4 = _mm_load1_ps(&pixelZ);
	*outAttrPtr = _mm_mul_ps(pixelZ_float4, result);
}

// This is a perspective-correct attribute interpolation:
static inline void InterpolateVertexAttribute_PerspectiveCorrect4(const D3DXVECTOR4& attr0, const D3DXVECTOR4& attr1, const D3DXVECTOR4& attr2, 
	const __m128 (&invZSplatted_X4)[4], const __m128 (&invZSplatted_Y4)[4], const __m128 (&invZSplatted_Z4)[4], const __m128 pixelZ4, D3DXVECTOR4 (&outAttr4)[4])
{
	const __m128 attr0vec = *(const __m128* const)&attr0;
	const __m128 attr1vec = *(const __m128* const)&attr1;
	const __m128 attr2vec = *(const __m128* const)&attr2;

	// I think that mul, mul, mul, add, add is the best we can do here since the attribute data comes in and needs a transpose, which prevents us from using dotproduct3
	const __m128 mulResultA[4] = 
	{
		_mm_mul_ps(invZSplatted_X4[0], attr0vec),
		_mm_mul_ps(invZSplatted_X4[1], attr0vec),
		_mm_mul_ps(invZSplatted_X4[2], attr0vec),
		_mm_mul_ps(invZSplatted_X4[3], attr0vec),
	};
	const __m128 mulResultB[4] = 
	{
		_mm_mul_ps(invZSplatted_Y4[0], attr1vec),
		_mm_mul_ps(invZSplatted_Y4[1], attr1vec),
		_mm_mul_ps(invZSplatted_Y4[2], attr1vec),
		_mm_mul_ps(invZSplatted_Y4[3], attr1vec)
	};
	const __m128 mulResultC[4] = 
	{
		_mm_mul_ps(invZSplatted_Z4[0], attr2vec),
		_mm_mul_ps(invZSplatted_Z4[1], attr2vec),
		_mm_mul_ps(invZSplatted_Z4[2], attr2vec),
		_mm_mul_ps(invZSplatted_Z4[3], attr2vec)
	};
	const __m128 result[4] = 
	{
		_mm_add_ps(_mm_add_ps(mulResultA[0], mulResultB[0]), mulResultC[0]),
		_mm_add_ps(_mm_add_ps(mulResultA[1], mulResultB[1]), mulResultC[1]),
		_mm_add_ps(_mm_add_ps(mulResultA[2], mulResultB[2]), mulResultC[2]),
		_mm_add_ps(_mm_add_ps(mulResultA[3], mulResultB[3]), mulResultC[3])
	};

	const __m128 pixelZ_float4[4] = 
	{
		_mm_permute_ps(pixelZ4, _MM_SHUFFLE(0, 0, 0, 0) ),
		_mm_permute_ps(pixelZ4, _MM_SHUFFLE(1, 1, 1, 1) ),
		_mm_permute_ps(pixelZ4, _MM_SHUFFLE(2, 2, 2, 2) ),
		_mm_permute_ps(pixelZ4, _MM_SHUFFLE(3, 3, 3, 3) )
	};
	*(__m128* const)&(outAttr4[0]) = _mm_mul_ps(pixelZ_float4[0], result[0]);
	*(__m128* const)&(outAttr4[1]) = _mm_mul_ps(pixelZ_float4[1], result[1]);
	*(__m128* const)&(outAttr4[2]) = _mm_mul_ps(pixelZ_float4[2], result[2]);
	*(__m128* const)&(outAttr4[3]) = _mm_mul_ps(pixelZ_float4[3], result[3]);
}

// Handles interpolating pixel shader input registers from a vertex declaration + raw vertex stream
void IDirect3DDevice9Hook::InterpolateStreamIntoRegisters(PShaderEngine* const pixelShader, const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, 
	const float pixelZ, const __m128 floatBarycentricsInvZ_X, const __m128 floatBarycentricsInvZ_Y, const __m128 floatBarycentricsInvZ_Z) const
{
	const ShaderInfo& pixelShaderInfo = currentState.currentPixelShader->GetShaderInfo();
	if (pixelShaderInfo.shaderMajorVersion == 1)
	{
		for (unsigned char v = 0; v < D3DMCS_COLOR2; ++v)
		{
			if (pixelShaderInfo.inputRegistersUsedBitmask & (1 << v) )
			{
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[v]);

				const DebuggableD3DVERTEXELEMENT9* const foundColorValue = vertexDeclMapping.vals[D3DDECLUSAGE_COLOR][v];
				if (foundColorValue)
				{
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = foundColorValue->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + foundColorValue->Offset) );

					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
					{
						// Color interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + foundColorValue->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + foundColorValue->Offset) );

						InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[t]);

				const DebuggableD3DVERTEXELEMENT9* const foundTexcoordValue = vertexDeclMapping.vals[D3DDECLUSAGE_TEXCOORD][t];
				if (foundTexcoordValue)
				{
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = foundTexcoordValue->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + foundTexcoordValue->Offset) );

					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
					{
						// Texcoord interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + foundTexcoordValue->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + foundTexcoordValue->Offset) );

						InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]);
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
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );

					// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Color interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]);
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
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );

					// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Texcoord interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]);
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
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;

					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );

					// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						// TODO: Implement PS_3_0 interpreters too
						InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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

// Handles interpolating pixel shader input registers from a vertex declaration + raw vertex stream
template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::InterpolateStreamIntoRegisters4(PShaderEngine* const pixelShader, const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0, CONST BYTE* const v1, CONST BYTE* const v2, 
	const __m128 pixelZ4, const __m128 (&floatBarycentricsInvZ_X4)[4], const __m128 (&floatBarycentricsInvZ_Y4)[4], const __m128 (&floatBarycentricsInvZ_Z4)[4]) const
{
	const ShaderInfo& pixelShaderInfo = currentState.currentPixelShader->GetShaderInfo();
	if (pixelShaderInfo.shaderMajorVersion == 1)
	{
		for (unsigned char v = 0; v < D3DMCS_COLOR2; ++v)
		{
			if (pixelShaderInfo.inputRegistersUsedBitmask & (1 << v) )
			{
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[v]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_2_0_inputs.v[v]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_2_0_inputs.v[v]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_2_0_inputs.v[v])
				};

				const DebuggableD3DVERTEXELEMENT9* const foundColorValue = vertexDeclMapping.vals[D3DDECLUSAGE_COLOR][v];
				if (foundColorValue)
				{
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = foundColorValue->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + foundColorValue->Offset) );

					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
					{
						// Color interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + foundColorValue->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + foundColorValue->Offset) );

						D3DXVECTOR4 interpolatedValues4[4];
						InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
					}
					else
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
					}
				}
				else
				{
					// It seems like this is correct behavior (color usage elements that are unbound from either the VS or pretransformed vertex data are read into the PS as (1,1,1,1) whereas all other usages are (0,0,0,0) ), but I can't find documentation for this
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = staticColorWhiteOpaque;
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = staticColorWhiteOpaque;
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = staticColorWhiteOpaque;
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = staticColorWhiteOpaque;
				}
			}
		}
		for (unsigned char t = 0; t < 6; ++t)
		{
			if (pixelShaderInfo.inputRegistersUsedBitmask & (1 << (t + D3DMCS_COLOR2) ) )
			{
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[t]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_2_0_inputs.t[t]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_2_0_inputs.t[t]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_2_0_inputs.t[t])
				};

				const DebuggableD3DVERTEXELEMENT9* const foundTexcoordValue = vertexDeclMapping.vals[D3DDECLUSAGE_TEXCOORD][t];
				if (foundTexcoordValue)
				{
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = foundTexcoordValue->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + foundTexcoordValue->Offset) );

					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
					{
						// Texcoord interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + foundTexcoordValue->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + foundTexcoordValue->Offset) );

						D3DXVECTOR4 interpolatedValues4[4];
						InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
					}
					else
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
					}
				}
				else
				{
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = staticColorBlackTranslucent;
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = staticColorBlackTranslucent;
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = staticColorBlackTranslucent;
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = staticColorBlackTranslucent;
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
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex])
				};
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDeclMapping.vals[D3DDECLUSAGE_COLOR][reg.usageIndex];
				if (!element)
				{
					// It seems like this is correct behavior (color usage elements that are unbound from either the VS or pretransformed vertex data are read into the PS as (1,1,1,1) whereas all other usages are (0,0,0,0) ), but I can't find documentation for this
					if (reg.usageType == D3DDECLUSAGE_COLOR)
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = staticColorWhiteOpaque;
					}
					else
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = staticColorBlackTranslucent;
					}
				}
				else
				{
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );

					// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Color interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						D3DXVECTOR4 interpolatedValues4[4];
						InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
					}
					else
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
					}
				}
			}
			else if (reg.registerType == D3DSPR_TEXTURE)
			{
				// Texcoord interpolator registers:
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDeclMapping.vals[D3DDECLUSAGE_TEXCOORD][reg.usageIndex];
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex])
				};
				if (!element)
				{
					// It seems like this is correct behavior (color usage elements that are unbound from either the VS or pretransformed vertex data are read into the PS as (1,1,1,1) whereas all other usages are (0,0,0,0) ), but I can't find documentation for this
					if (reg.usageType == D3DDECLUSAGE_COLOR)
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = staticColorWhiteOpaque;
					}
					else
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = staticColorBlackTranslucent;
					}
				}
				else
				{
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;
					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );

					// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Texcoord interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						D3DXVECTOR4 interpolatedValues4[4];
						InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
					}
					else
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
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
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex])
				};
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDeclMapping.vals[reg.usageType][reg.usageIndex];
				if (!element)
				{
					// It seems like this is correct behavior (color usage elements that are unbound from either the VS or pretransformed vertex data are read into the PS as (1,1,1,1) whereas all other usages are (0,0,0,0) ), but I can't find documentation for this
					if (reg.usageType == D3DDECLUSAGE_COLOR)
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = staticColorWhiteOpaque;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = staticColorWhiteOpaque;
					}
					else
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = staticColorBlackTranslucent;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = staticColorBlackTranslucent;
					}
				}
				else
				{
					__declspec(align(16) ) D3DXVECTOR4 cf0;
					const D3DDECLTYPE registerLoadType = element->Type;

					LoadElementToRegister(cf0, registerLoadType, (v0 + element->Offset) );

					// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
					if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
					{
						// Interpolator registers:
						__declspec(align(16) ) D3DXVECTOR4 cf1;
						__declspec(align(16) ) D3DXVECTOR4 cf2;
						LoadElementToRegister(cf1, registerLoadType, (v1 + element->Offset) );
						LoadElementToRegister(cf2, registerLoadType, (v2 + element->Offset) );

						// TODO: Implement PS_3_0 interpreters too
						D3DXVECTOR4 interpolatedValues4[4];
						InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
					}
					else
					{
						if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
						if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
						if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
						if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
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

static const __m128 oneVec = { 1.0f, 1.0f, 1.0f, 1.0f };
static const __m128 maxDepth24Bit = { 16777216.0f, 16777216.0f, 16777216.0f, 16777216.0f };

const float IDirect3DDevice9Hook::InterpolatePixelDepth(const __m128 barycentricInterpolants, const __m128 invZ) const
{
	const __m128 invInterpolatedDepth = _mm_dp_ps(invZ, barycentricInterpolants, 0x7F);

	return _mm_div_ps(oneVec, invInterpolatedDepth).m128_f32[0];
}

void IDirect3DDevice9Hook::InterpolatePixelDepth4(const __m128 (&barycentricInterpolants4)[4], const __m128 invZ, __m128& outPixelDepth4) const
{
	const __m128 dotProdResult0 = _mm_dp_ps(invZ, barycentricInterpolants4[0], 0x7F);
	const __m128 dotProdResult1 = _mm_dp_ps(invZ, barycentricInterpolants4[1], 0x7F);
	const __m128 dotProdResult2 = _mm_dp_ps(invZ, barycentricInterpolants4[2], 0x7F);
	const __m128 dotProdResult3 = _mm_dp_ps(invZ, barycentricInterpolants4[3], 0x7F);

	const __m128 invInterpolatedDepth4 = 
	{
		dotProdResult0.m128_f32[0],
		dotProdResult1.m128_f32[0],
		dotProdResult2.m128_f32[0],
		dotProdResult3.m128_f32[0]
	};

	const __m128 pixelDepth4 = _mm_div_ps(oneVec, invInterpolatedDepth4);
	outPixelDepth4 = pixelDepth4;
}

// Handles interpolating pixel shader input registers from vertex shader output registers
// TODO: Like InterpolateStreamIntoRegisters, have this function fill with (1,1,1,1) for input color usage registers or (0,0,0,0) for other usages if the vertex shader doesn't write to the corresponding output registers
void IDirect3DDevice9Hook::InterpolateShaderIntoRegisters(PShaderEngine* const pixelShader, const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, 
	const float pixelZ, const __m128 floatBarycentricsInvZ_X, const __m128 floatBarycentricsInvZ_Y, const __m128 floatBarycentricsInvZ_Z) const
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
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[v]);
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
				{
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_2_0_outputs.oD[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_2_0_outputs.oD[vsRegisterIndex]);
					InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[t]);
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
				{
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_2_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_2_0_outputs.oT[vsRegisterIndex]);
					InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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

				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]);

				// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Color interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]);

				// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Texcoord interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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
				D3DXVECTOR4& interpolatedFloatValue = *(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]);

				// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
				if (reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);

					// TODO: Implement PS_3_0 interpreters too
					InterpolateVertexAttribute_PerspectiveCorrect(cf0, cf1, cf2, floatBarycentricsInvZ_X, floatBarycentricsInvZ_Y, floatBarycentricsInvZ_Z, pixelZ, interpolatedFloatValue);
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

// Handles interpolating pixel shader input registers from vertex shader output registers
// TODO: Like InterpolateStreamIntoRegisters, have this function fill with (1,1,1,1) for input color usage registers or (0,0,0,0) for other usages if the vertex shader doesn't write to the corresponding output registers
template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::InterpolateShaderIntoRegisters4(PShaderEngine* const pixelShader, const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1, const VS_2_0_OutputRegisters& v2, 
	const __m128 pixelZ4, const __m128 (&floatBarycentricsInvZ_X4)[4], const __m128 (&floatBarycentricsInvZ_Y4)[4], const __m128 (&floatBarycentricsInvZ_Z4)[4]) const
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
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[v]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_2_0_inputs.v[v]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_2_0_inputs.v[v]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_2_0_inputs.v[v])
				};
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
				{
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_2_0_outputs.oD[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_2_0_outputs.oD[vsRegisterIndex]);
					
					D3DXVECTOR4 interpolatedValues4[4];
					InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
				}
				else
				{
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
				}
			}
		}
		for (unsigned char t = 0; t < 6; ++t)
		{
			if (pixelShaderInfo.inputRegistersUsedBitmask & (1 << (t + D3DMCS_COLOR2) ) )
			{
				const unsigned char vsRegisterIndex = t;
				const D3DXVECTOR4& cf0 = *(const D3DXVECTOR4* const)&(v0.vs_interpolated_outputs.vs_2_0_outputs.oT[vsRegisterIndex]);
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[t]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_2_0_inputs.t[t]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_2_0_inputs.t[t]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_2_0_inputs.t[t])
				};
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT)
				{
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_2_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_2_0_outputs.oT[vsRegisterIndex]);
					
					D3DXVECTOR4 interpolatedValues4[4];
					InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
				}
				else
				{
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
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

				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_2_0_inputs.v[reg.registerIndex])
				};

				// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Color interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					
					D3DXVECTOR4 interpolatedValues4[4];
					InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
				}
				else
				{
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
				}
			}
			else if (reg.registerType == D3DSPR_TEXTURE)
			{
				// Texcoord interpolator registers:
				const unsigned vsRegisterIndex = vs_psMapping.psInputRegistersUnion.ps_2_0_registers.texCoords[reg.registerIndex];
				const D3DXVECTOR4& cf0 = *(const D3DXVECTOR4* const)&(v0.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_2_0_inputs.t[reg.registerIndex])
				};

				// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
				if (currentState.currentRenderStates.renderStatesUnion.namedStates.shadeMode != D3DSHADE_FLAT && reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Texcoord interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					
					D3DXVECTOR4 interpolatedValues4[4];
					InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
				}
				else
				{
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
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
				D3DXVECTOR4* const interpolatedOutRegisterValues[4] = 
				{
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[0].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[1].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[2].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex]),
					(D3DXVECTOR4* const)&(pixelShader->inputRegisters[3].ps_interpolated_inputs.ps_3_0_inputs.t[reg.registerIndex])
				};

				// TODO: Look into whether or not D3DDECLUSAGE_POSITION in pixel shaders disables perspective correction for that interpolated register or not
				if (reg.usageType != D3DDECLUSAGE_BLENDINDICES)
				{
					// Interpolator registers:
					const D3DXVECTOR4& cf1 = *(const D3DXVECTOR4* const)&(v1.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);
					const D3DXVECTOR4& cf2 = *(const D3DXVECTOR4* const)&(v2.vs_interpolated_outputs.vs_3_0_outputs.oT[vsRegisterIndex]);

					// TODO: Implement PS_3_0 interpreters too
					
					D3DXVECTOR4 interpolatedValues4[4];
					InterpolateVertexAttribute_PerspectiveCorrect4(cf0, cf1, cf2, floatBarycentricsInvZ_X4, floatBarycentricsInvZ_Y4, floatBarycentricsInvZ_Z4, pixelZ4, interpolatedValues4);
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = interpolatedValues4[0];
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = interpolatedValues4[1];
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = interpolatedValues4[2];
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = interpolatedValues4[3];
				}
				else
				{
					if (pixelWriteMask & 0x1) *interpolatedOutRegisterValues[0] = cf0;
					if (pixelWriteMask & 0x2) *interpolatedOutRegisterValues[1] = cf0;
					if (pixelWriteMask & 0x4) *interpolatedOutRegisterValues[2] = cf0;
					if (pixelWriteMask & 0x8) *interpolatedOutRegisterValues[3] = cf0;
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

void IDirect3DDevice9Hook::ShadePixel_RunShader(const unsigned x, const unsigned y, PShaderEngine* const pixelShader) const
{
	++frameStats.numPixelsShaded;

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

	if (currentState.currentPixelShader->GetShaderInfo().usesTexkill)
		PostShadePixel_DiscardTest(x, y, pixelShader->outputRegisters[0]);
	else
		PostShadePixel_AlphaTest(x, y, pixelShader->outputRegisters[0]);
}

void IDirect3DDevice9Hook::PostShadePixel_DiscardTest(const unsigned x, const unsigned y, const PS_2_0_OutputRegisters& pixelShaderOutput) const
{
	if (pixelShaderOutput.pixelStatus == discard)
	{
		PostShadePixel_Discard(x, y);
		return;
	}

	// Alpha testing:
	PostShadePixel_AlphaTest(x, y, pixelShaderOutput);
}

void IDirect3DDevice9Hook::PostShadePixel_AlphaTest(const unsigned x, const unsigned y, const PS_2_0_OutputRegisters& pixelShaderOutput) const
{
	// This MSDN page says that alpha testing only happens against the alpha value from oC0: https://docs.microsoft.com/en-us/windows/desktop/direct3d9/multiple-render-targets
	if (!AlphaTest(*(const D3DXVECTOR4* const)&(pixelShaderOutput.oC[0]) ) )
	{
		PostShadePixel_FailAlphaTest(x, y);
		return;
	}

	PostShadePixel_WriteOutput(x, y, pixelShaderOutput);
}

void IDirect3DDevice9Hook::PostShadePixel_WriteOutput(const unsigned x, const unsigned y, const PS_2_0_OutputRegisters& pixelShaderOutput) const
{
	PostShadePixel_WriteOutputColor(x, y, pixelShaderOutput);

	if (currentState.currentDepthStencil)
	{
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zWriteEnable)
		{
			// TODO: Re-run depth test in the case that this pixel shader writes out a custom depth
			PostShadePixel_WriteOutputDepth(x, y, pixelShaderOutput.oDepth);
		}
			
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
		{
			PostShadePixel_WriteOutputStencil(x, y);
		}
	}
}

void IDirect3DDevice9Hook::ShadePixel_FailStencil(const unsigned x, const unsigned y) const
{
	const unsigned xCoord = x >> SUBPIXEL_ACCURACY_BITS;
	const unsigned yCoord = y >> SUBPIXEL_ACCURACY_BITS;
	StencilFailOperation(xCoord, yCoord);
}

void IDirect3DDevice9Hook::ShadePixel_FailDepth(const unsigned x, const unsigned y) const
{
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
	{
		const unsigned xCoord = x >> SUBPIXEL_ACCURACY_BITS;
		const unsigned yCoord = y >> SUBPIXEL_ACCURACY_BITS;
		StencilZFailOperation(xCoord, yCoord);
	}
}

void IDirect3DDevice9Hook::PostShadePixel_FailAlphaTest(const unsigned x, const unsigned y) const
{
	++frameStats.numAlphaTestFailPixels;
}

void IDirect3DDevice9Hook::PostShadePixel_Discard(const unsigned x, const unsigned y) const
{
	++frameStats.numPixelsTexkilled;
}

void IDirect3DDevice9Hook::PostShadePixel_WriteOutputColor(const unsigned x, const unsigned y, const PS_2_0_OutputRegisters& pixelOutputColor) const
{
	const unsigned xCoord = x >> SUBPIXEL_ACCURACY_BITS;
	const unsigned yCoord = y >> SUBPIXEL_ACCURACY_BITS;
	for (unsigned char rt = 0; rt < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++rt)
	{
		IDirect3DSurface9Hook* const currentRenderTarget = currentState.currentRenderTargets[rt];
		if (!currentRenderTarget)
			continue;

		RenderOutput(currentRenderTarget, xCoord, yCoord, *(const D3DXVECTOR4* const)&(pixelOutputColor.oC[rt]) );
	}
}

void IDirect3DDevice9Hook::PostShadePixel_WriteOutputDepth(const unsigned x, const unsigned y, const float pixelOutputDepth) const
{
	const unsigned xCoord = x >> SUBPIXEL_ACCURACY_BITS;
	const unsigned yCoord = y >> SUBPIXEL_ACCURACY_BITS;
	const float depthBias = currentState.currentRenderStates.renderStatesUnion.namedStates.depthBias;
	currentState.currentDepthStencil->SetDepth(xCoord, yCoord, pixelOutputDepth + depthBias);
}

void IDirect3DDevice9Hook::PostShadePixel_WriteOutputStencil(const unsigned x, const unsigned y) const
{
	StencilPassOperation(x, y);
}

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::ShadePixel4_RunShader(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const
{
	frameStats.numPixelsShaded += 4;

	// Perform pixel shading:
	// TODO: Need to write a quad execution interpreter engine, and a quad JIT engine to replace this
	// Hack for now, need to preserve the 0th set of output registers or else they'll get clobbered by later
	// iterations of pixel shading
	__declspec(align(16) ) PS_2_0_OutputRegisters savedOutputRegs0;
	for (unsigned char z = 0; z < 4; ++z)
	{
		if (!(pixelWriteMask & (1 << z) ) )
			continue;

		if (z != 0)
			pixelShader->inputRegisters[0] = pixelShader->inputRegisters[z];

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
			pixelShader->Reset(x4.m128i_u32[z], y4.m128i_u32[z]); // HACK!
			pixelShader->InterpreterExecutePixel();
		}

		if (z != 0)
			pixelShader->outputRegisters[z] = pixelShader->outputRegisters[0];
		else
			savedOutputRegs0 = pixelShader->outputRegisters[0];
	}
	if (pixelWriteMask & 0x1)
		pixelShader->outputRegisters[0] = savedOutputRegs0;

	if (currentState.currentPixelShader->GetShaderInfo().usesTexkill)
		PostShadePixel4_DiscardTest<pixelWriteMask>(x4, y4, pixelShader);
	else
		PostShadePixel4_AlphaTest<pixelWriteMask>(x4, y4, pixelShader);
}

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::ShadePixel4_FailStencil(const __m128i x4, const __m128i y4) const
{
	for (unsigned char z = 0; z < 4; ++z)
	{
		if (pixelWriteMask & (1 << z) )
		{
			StencilFailOperation(x4.m128i_u32[z], y4.m128i_u32[z]);
		}
	}
}

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::ShadePixel4_FailDepth(const __m128i x4, const __m128i y4) const
{
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
	{
		for (unsigned char z = 0; z < 4; ++z)
		{
			if (pixelWriteMask & (1 << z) )
			{
				StencilZFailOperation(x4.m128i_u32[z], y4.m128i_u32[z]);
			}
		}
	}
}

static_assert(sizeof(PS_2_0_OutputRegisters) % sizeof(DWORD) == 0, "Error! Unexpected size slack!");
static const unsigned outputRegisterOffsetsGatherBytes[4] = { 0, sizeof(PS_2_0_OutputRegisters) / sizeof(DWORD), sizeof(PS_2_0_OutputRegisters) * 2 / sizeof(DWORD), sizeof(PS_2_0_OutputRegisters) * 3 / sizeof(DWORD) };
static const __m128i outputRegisterOffsetsGather = *(const __m128i* const)outputRegisterOffsetsGatherBytes;

static const __m128i pixelStatusDiscardCheck4 = _mm_set1_epi32(discard);

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::PostShadePixel4_DiscardTest(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const
{
	// Handle discard (texkill) here now, then call PostShadePixel4_Discard() for any discarded pixels
	const __m128i pixelStatus4 = _mm_i32gather_epi32( (const int* const)&(pixelShader->outputRegisters[0].pixelStatus), outputRegisterOffsetsGather, 4);
	const unsigned char pixelDiscardMask = pixelWriteMask & (const unsigned char)(_mm_movemask_ps(_mm_cvtepi32_ps(_mm_cmpeq_epi32(pixelStatus4, pixelStatusDiscardCheck4) ) ) );
	PostShadePixel4_Discard(pixelDiscardMask);

	const unsigned char pixelWriteMask_postDiscard = pixelWriteMask & (~pixelDiscardMask);
	switch (pixelWriteMask_postDiscard)
	{
	default:
#ifdef _DEBUG
		__debugbreak(); // Should never be here!
#else
		__assume(0);
#endif
	case 0x0:
		return; // No pixels left, we're done!
	case 0x1:
		PostShadePixel_AlphaTest(x4.m128i_u32[0], y4.m128i_u32[0], pixelShader->outputRegisters[0]);
		return;
	case 0x2:
		PostShadePixel_AlphaTest(x4.m128i_u32[1], y4.m128i_u32[1], pixelShader->outputRegisters[1]);
		return;
	case 0x4:
		PostShadePixel_AlphaTest(x4.m128i_u32[2], y4.m128i_u32[2], pixelShader->outputRegisters[2]);
		return;
	case 0x8:
		PostShadePixel_AlphaTest(x4.m128i_u32[3], y4.m128i_u32[3], pixelShader->outputRegisters[3]);
		return;
	case 0x3:
		PostShadePixel4_AlphaTest<0x3>(x4, y4, pixelShader);
		return;
	case 0x5:
		PostShadePixel4_AlphaTest<0x5>(x4, y4, pixelShader);
		return;
	case 0x6:
		PostShadePixel4_AlphaTest<0x6>(x4, y4, pixelShader);
		return;
	case 0x7:
		PostShadePixel4_AlphaTest<0x7>(x4, y4, pixelShader);
		return;
	case 0x9:
		PostShadePixel4_AlphaTest<0x9>(x4, y4, pixelShader);
		return;
	case 0xA:
		PostShadePixel4_AlphaTest<0xA>(x4, y4, pixelShader);
		return;
	case 0xB:
		PostShadePixel4_AlphaTest<0xB>(x4, y4, pixelShader);
		return;
	case 0xC:
		PostShadePixel4_AlphaTest<0xC>(x4, y4, pixelShader);
		return;
	case 0xD:
		PostShadePixel4_AlphaTest<0xD>(x4, y4, pixelShader);
		return;
	case 0xE:
		PostShadePixel4_AlphaTest<0xE>(x4, y4, pixelShader);
		return;
	case 0xF:
		PostShadePixel4_AlphaTest<0xF>(x4, y4, pixelShader);
		return;
	}
}

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::PostShadePixel4_AlphaTest(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const
{
	const unsigned char postAlphaTestWriteMask = pixelWriteMask & _mm_movemask_ps(AlphaTest4<0xF>(pixelShader->outputRegisters) );
	switch (postAlphaTestWriteMask)
	{
	default:
#ifdef _DEBUG
		__debugbreak(); // Should never be here!
#else
		__assume(0);
#endif
	case 0x0:
		return; // No more pixels left, we're done!
	case 0x1:
		PostShadePixel_WriteOutput(x4.m128i_u32[0], y4.m128i_u32[0], pixelShader->outputRegisters[0]);
		return;
	case 0x2:
		PostShadePixel_WriteOutput(x4.m128i_u32[1], y4.m128i_u32[1], pixelShader->outputRegisters[1]);
		return;
	case 0x4:
		PostShadePixel_WriteOutput(x4.m128i_u32[2], y4.m128i_u32[2], pixelShader->outputRegisters[2]);
		return;
	case 0x8:
		PostShadePixel_WriteOutput(x4.m128i_u32[3], y4.m128i_u32[3], pixelShader->outputRegisters[3]);
		return;
	case 0x3:
		PostShadePixel4_WriteOutput<0x3>(x4, y4, pixelShader);
		return;
	case 0x5:
		PostShadePixel4_WriteOutput<0x5>(x4, y4, pixelShader);
		return;
	case 0x6:
		PostShadePixel4_WriteOutput<0x6>(x4, y4, pixelShader);
		return;
	case 0x7:
		PostShadePixel4_WriteOutput<0x7>(x4, y4, pixelShader);
		return;
	case 0x9:
		PostShadePixel4_WriteOutput<0x9>(x4, y4, pixelShader);
		return;
	case 0xA:
		PostShadePixel4_WriteOutput<0xA>(x4, y4, pixelShader);
		return;
	case 0xB:
		PostShadePixel4_WriteOutput<0xB>(x4, y4, pixelShader);
		return;
	case 0xC:
		PostShadePixel4_WriteOutput<0xC>(x4, y4, pixelShader);
		return;
	case 0xD:
		PostShadePixel4_WriteOutput<0xD>(x4, y4, pixelShader);
		return;
	case 0xE:
		PostShadePixel4_WriteOutput<0xE>(x4, y4, pixelShader);
		return;
	case 0xF:
		PostShadePixel4_WriteOutput<0xF>(x4, y4, pixelShader);
		return;
	}
}

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::PostShadePixel4_WriteOutput(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const
{	
	PostShadePixel4_WriteOutputColor<pixelWriteMask>(x4, y4, pixelShader);

	if (currentState.currentDepthStencil)
	{
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.zWriteEnable)
		{
			PostShadePixel4_WriteOutputDepth<pixelWriteMask>(x4, y4, pixelShader);
		}
			
		if (currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
		{
			PostShadePixel4_WriteOutputStencil<pixelWriteMask>(x4, y4);
		}
	}
}

void IDirect3DDevice9Hook::PostShadePixel4_Discard(const unsigned char pixelDiscardMask) const
{
	switch (pixelDiscardMask)
	{
	default:
	case 0x0:
		return;
	case 0x1:
	case 0x2:
	case 0x4:
	case 0x8:
		++frameStats.numPixelsTexkilled;
		return;
	case 0x3:
	case 0x5:
	case 0x6:
	case 0x9:
	case 0xA:
	case 0xC:
		frameStats.numPixelsTexkilled += 2;
		return;
	case 0x7:
	case 0xB:
	case 0xD:
	case 0xE:
		frameStats.numPixelsTexkilled += 3;
		return;
	case 0xF:
		frameStats.numPixelsTexkilled += 4;
		return;
	}
}

void IDirect3DDevice9Hook::PostShadePixel4_FailAlphaTest(const unsigned char pixelsFailAlphaTestMask) const
{
	switch (pixelsFailAlphaTestMask)
	{
	default:
	case 0x0:
		return;
	case 0x1:
	case 0x2:
	case 0x4:
	case 0x8:
		++frameStats.numAlphaTestFailPixels;
		return;
	case 0x3:
	case 0x5:
	case 0x6:
	case 0x9:
	case 0xA:
	case 0xC:
		frameStats.numAlphaTestFailPixels += 2;
		return;
	case 0x7:
	case 0xB:
	case 0xD:
	case 0xE:
		frameStats.numAlphaTestFailPixels += 3;
		return;
	case 0xF:
		frameStats.numAlphaTestFailPixels += 4;
		return;
	}
}

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::PostShadePixel4_WriteOutputColor(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const
{
	switch (pixelWriteMask)
	{
	default:
	case 0x0:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
		return;
	case 0x1:
		PostShadePixel_WriteOutputColor(x4.m128i_u32[0], y4.m128i_u32[0], pixelShader->outputRegisters[0]);
		return;
	case 0x2:
		PostShadePixel_WriteOutputColor(x4.m128i_u32[1], y4.m128i_u32[1], pixelShader->outputRegisters[1]);
		return;
	case 0x4:
		PostShadePixel_WriteOutputColor(x4.m128i_u32[2], y4.m128i_u32[2], pixelShader->outputRegisters[2]);
		return;
	case 0x8:
		PostShadePixel_WriteOutputColor(x4.m128i_u32[3], y4.m128i_u32[3], pixelShader->outputRegisters[3]);
		return;
	case 0x3:
	case 0x5:
	case 0x6:
	case 0x7:
	case 0x9:
	case 0xA:
	case 0xB:
	case 0xC:
	case 0xD:
	case 0xE:
	case 0xF:
		break;
	}

	for (unsigned char rt = 0; rt < D3D_MAX_SIMULTANEOUS_RENDERTARGETS; ++rt)
	{
		IDirect3DSurface9Hook* const currentRenderTarget = currentState.currentRenderTargets[rt];
		if (!currentRenderTarget)
			continue;

		RenderOutput4<pixelWriteMask>(currentRenderTarget, x4, y4, pixelShader->outputRegisters, rt);
	}
}

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::PostShadePixel4_WriteOutputDepth(const __m128i x4, const __m128i y4, PShaderEngine* const pixelShader) const
{
	switch (pixelWriteMask)
	{
	default:
	case 0x0:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
		return;
	case 0x1:
		PostShadePixel_WriteOutputDepth(x4.m128i_u32[0], y4.m128i_u32[0], pixelShader->outputRegisters[0].oDepth);
		return;
	case 0x2:
		PostShadePixel_WriteOutputDepth(x4.m128i_u32[1], y4.m128i_u32[1], pixelShader->outputRegisters[1].oDepth);
		return;
	case 0x4:
		PostShadePixel_WriteOutputDepth(x4.m128i_u32[2], y4.m128i_u32[2], pixelShader->outputRegisters[2].oDepth);
		return;
	case 0x8:
		PostShadePixel_WriteOutputDepth(x4.m128i_u32[3], y4.m128i_u32[3], pixelShader->outputRegisters[3].oDepth);
		return;
	case 0x3:
	case 0x5:
	case 0x6:
	case 0x7:
	case 0x9:
	case 0xA:
	case 0xB:
	case 0xC:
	case 0xD:
	case 0xE:
	case 0xF:
		break;
	}

	__m128 depth4;
	if (pixelWriteMask & 0x1)
		depth4.m128_f32[0] = pixelShader->outputRegisters[0].oDepth;
	if (pixelWriteMask & 0x2)
		depth4.m128_f32[1] = pixelShader->outputRegisters[1].oDepth;
	if (pixelWriteMask & 0x4)
		depth4.m128_f32[2] = pixelShader->outputRegisters[2].oDepth;
	if (pixelWriteMask & 0x8)
		depth4.m128_f32[3] = pixelShader->outputRegisters[3].oDepth;
	depth4 = _mm_add_ps(depth4, currentState.currentRenderStates.depthBiasSplatted);
	currentState.currentDepthStencil->SetDepth4<pixelWriteMask>(x4, y4, depth4);
}

template <const unsigned char pixelWriteMask>
void IDirect3DDevice9Hook::PostShadePixel4_WriteOutputStencil(const __m128i x4, const __m128i y4) const
{
	if (pixelWriteMask == 0x0)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}
	switch (pixelWriteMask)
	{
	default:
	case 0x0:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
		return;
	case 0x1:
		PostShadePixel_WriteOutputStencil(x4.m128i_u32[0], y4.m128i_u32[0]);
		return;
	case 0x2:
		PostShadePixel_WriteOutputStencil(x4.m128i_u32[1], y4.m128i_u32[1]);
		return;
	case 0x4:
		PostShadePixel_WriteOutputStencil(x4.m128i_u32[2], y4.m128i_u32[2]);
		return;
	case 0x8:
		PostShadePixel_WriteOutputStencil(x4.m128i_u32[3], y4.m128i_u32[3]);
		return;
	case 0x3:
	case 0x5:
	case 0x6:
	case 0x7:
	case 0x9:
	case 0xA:
	case 0xB:
	case 0xC:
	case 0xD:
	case 0xE:
	case 0xF:
		break;
	}

	// TODO: Need to write a StencilOperation4 to replace this
	for (unsigned z = 0; z < 4; ++z)
	{
		if (pixelWriteMask & (1 << z) )
			PostShadePixel_WriteOutputStencil(x4.m128i_u32[z], y4.m128i_u32[z]);
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

/*void IDirect3DDevice9Hook::StencilFailOperation4(const __m128i x4, const __m128i y4) const
{
	StencilOperation4(x4, y4, currentState.currentRenderStates.renderStatesUnion.namedStates.stencilFail);
}

void IDirect3DDevice9Hook::StencilZFailOperation4(const __m128i x4, const __m128i y4) const
{
	StencilOperation4(x4, y4, currentState.currentRenderStates.renderStatesUnion.namedStates.stencilZFail);
}

void IDirect3DDevice9Hook::StencilPassOperation4(const __m128i x4, const __m128i y4) const
{
	StencilOperation4(x4, y4, currentState.currentRenderStates.renderStatesUnion.namedStates.stencilPass);
}*/

// true = "pass" (draw the pixel), false = "fail" (discard the pixel for all render targets and also discard depth/stencil writes for this pixel)
// This MSDN page says that alpha testing only happens against the alpha value from oC0: https://docs.microsoft.com/en-us/windows/desktop/direct3d9/multiple-render-targets
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
#else
			__assume(0);
#endif
		case D3DCMP_ALWAYS      :
			return true;
		}
	}

	return true;
}

// Returns a SSE vector mask (0xFF for "test pass" and 0x00 for "test fail")
template <const unsigned char pixelWriteMask>
const __m128 IDirect3DDevice9Hook::AlphaTest4(const PS_2_0_OutputRegisters (&outColor4)[4]) const
{
	// Alpha testing:
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaTestEnable)
	{
		switch (currentState.currentRenderStates.renderStatesUnion.namedStates.alphaFunc)
		{
		case D3DCMP_NEVER       :
			return *(const __m128* const)&zeroMaskVec;
		case D3DCMP_LESS        :
		{
			__m128 alphaVec;
			if (pixelWriteMask & 0x1) alphaVec.m128_f32[0] = outColor4[0].oC[0].w;
			if (pixelWriteMask & 0x2) alphaVec.m128_f32[1] = outColor4[1].oC[0].w;
			if (pixelWriteMask & 0x4) alphaVec.m128_f32[2] = outColor4[2].oC[0].w;
			if (pixelWriteMask & 0x8) alphaVec.m128_f32[3] = outColor4[3].oC[0].w;
			return _mm_cmplt_ps(alphaVec, currentState.currentRenderStates.alphaRefSplatted);
		}
		case D3DCMP_EQUAL       :
		{
			__m128 alphaVec;
			if (pixelWriteMask & 0x1) alphaVec.m128_f32[0] = outColor4[0].oC[0].w;
			if (pixelWriteMask & 0x2) alphaVec.m128_f32[1] = outColor4[1].oC[0].w;
			if (pixelWriteMask & 0x4) alphaVec.m128_f32[2] = outColor4[2].oC[0].w;
			if (pixelWriteMask & 0x8) alphaVec.m128_f32[3] = outColor4[3].oC[0].w;
			return _mm_cmpeq_ps(alphaVec, currentState.currentRenderStates.alphaRefSplatted);
		}
		case D3DCMP_LESSEQUAL   :
		{
			__m128 alphaVec;
			if (pixelWriteMask & 0x1) alphaVec.m128_f32[0] = outColor4[0].oC[0].w;
			if (pixelWriteMask & 0x2) alphaVec.m128_f32[1] = outColor4[1].oC[0].w;
			if (pixelWriteMask & 0x4) alphaVec.m128_f32[2] = outColor4[2].oC[0].w;
			if (pixelWriteMask & 0x8) alphaVec.m128_f32[3] = outColor4[3].oC[0].w;
			return _mm_cmple_ps(alphaVec, currentState.currentRenderStates.alphaRefSplatted);
		}
		case D3DCMP_GREATER     :
		{
			__m128 alphaVec;
			if (pixelWriteMask & 0x1) alphaVec.m128_f32[0] = outColor4[0].oC[0].w;
			if (pixelWriteMask & 0x2) alphaVec.m128_f32[1] = outColor4[1].oC[0].w;
			if (pixelWriteMask & 0x4) alphaVec.m128_f32[2] = outColor4[2].oC[0].w;
			if (pixelWriteMask & 0x8) alphaVec.m128_f32[3] = outColor4[3].oC[0].w;
			return _mm_cmpgt_ps(alphaVec, currentState.currentRenderStates.alphaRefSplatted);
		}
		case D3DCMP_NOTEQUAL    :
		{
			__m128 alphaVec;
			if (pixelWriteMask & 0x1) alphaVec.m128_f32[0] = outColor4[0].oC[0].w;
			if (pixelWriteMask & 0x2) alphaVec.m128_f32[1] = outColor4[1].oC[0].w;
			if (pixelWriteMask & 0x4) alphaVec.m128_f32[2] = outColor4[2].oC[0].w;
			if (pixelWriteMask & 0x8) alphaVec.m128_f32[3] = outColor4[3].oC[0].w;
			return _mm_cmpneq_ps(alphaVec, currentState.currentRenderStates.alphaRefSplatted);
		}
		case D3DCMP_GREATEREQUAL:
		{
			__m128 alphaVec;
			if (pixelWriteMask & 0x1) alphaVec.m128_f32[0] = outColor4[0].oC[0].w;
			if (pixelWriteMask & 0x2) alphaVec.m128_f32[1] = outColor4[1].oC[0].w;
			if (pixelWriteMask & 0x4) alphaVec.m128_f32[2] = outColor4[2].oC[0].w;
			if (pixelWriteMask & 0x8) alphaVec.m128_f32[3] = outColor4[3].oC[0].w;
			return _mm_cmpge_ps(alphaVec, currentState.currentRenderStates.alphaRefSplatted);
		}
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Invalid D3DCMP function passed to Alpha Test");
#else
			__assume(0);
#endif
		case D3DCMP_ALWAYS      :
			break;
		}
	}

	return *(const __m128* const)&oneMaskVec;
}

// true = "pass" (draw the pixel), false = "fail" (discard the pixel)
const bool IDirect3DDevice9Hook::StencilTestNoWrite(const unsigned x, const unsigned y) const
{
	if (!currentState.currentRenderStates.renderStatesUnion.namedStates.stencilEnable)
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

static inline const bool isTopLeftEdge(const int2& v0, const int2& v1)
{
	const int dx = v1.x - v0.x;
	const int dy = v1.y - v0.y;
	return ( (dy < 0) || ( (dy == 0) && (dx < 0) ) );
}

// Assumes pre-transformed vertices from a vertex declaration + raw vertex stream
void IDirect3DDevice9Hook::RasterizeLineFromStream(const DeclarationSemanticMapping& vertexDeclMapping, CONST BYTE* const v0, CONST BYTE* const v1) const
{
#ifdef ENABLE_END_TO_SKIP_DRAWS
	if (IsHoldingEndToSkipDrawCalls() )
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
	if (IsHoldingEndToSkipDrawCalls() )
	{
		// Skip rendering if END is held down
		return;
	}
#endif

	// Do nothing, point rasterization is not yet implemented
}

// Assumes pre-transformed vertices from a processed vertex shader
void IDirect3DDevice9Hook::RasterizeLineFromShader(const VStoPSMapping& vs_psMapping, const VS_2_0_OutputRegisters& v0, const VS_2_0_OutputRegisters& v1) const
{
#ifdef ENABLE_END_TO_SKIP_DRAWS
	if (IsHoldingEndToSkipDrawCalls() )
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
	if (IsHoldingEndToSkipDrawCalls() )
	{
		// Skip rendering if END is held down
		return;
	}
#endif

	// Do nothing, point rasterization is not yet implemented
}

template <const unsigned char integerBits, const unsigned char fractionBits, typename baseType = unsigned>
union TFixed
{
	struct _splitFormat
	{
		baseType fracPart : fractionBits;
		baseType intPart : integerBits;
	} splitFormat;

	baseType whole;

	TFixed<integerBits, fractionBits, baseType> operator+(const TFixed<integerBits, fractionBits, baseType>& other) const
	{
		TFixed<integerBits, fractionBits, baseType> ret;
		ret.whole = whole + other.whole;
		return ret;
	}

	TFixed<integerBits, fractionBits, baseType> operator-(const TFixed<integerBits, fractionBits, baseType>& other) const
	{
		TFixed<integerBits, fractionBits, baseType> ret;
		ret.whole = whole - other.whole;
		return ret;
	}

	TFixed<integerBits, fractionBits, baseType> operator*(const TFixed<integerBits, fractionBits, baseType>& other) const
	{
		TFixed<integerBits, fractionBits, baseType> ret;
		unsigned __int64 longWhole = whole;
		unsigned __int64 longOtherWhole = other.whole;
		unsigned __int64 longProduct = longWhole * longOtherWhole;
		unsigned __int64 deshiftedProduct = longProduct >> fractionBits;
		ret.whole = (const baseType)deshiftedProduct;
		return ret;
	}

	const bool operator==(const TFixed<integerBits, fractionBits, baseType>& other) const
	{
		return other.whole == whole;
	}
};

template <const unsigned char integerBits, const unsigned char fractionBits, typename baseType>
static const float ConvertFixedToFloat(TFixed<integerBits, fractionBits, baseType> fixed)
{
	float conv = 0.0f;

	baseType fracStorage = fixed.splitFormat.fracPart;
	for (unsigned x = 1; x < fractionBits; ++x)
	{
		if (fracStorage & (1 << (fractionBits - x) ) )
		{
			conv += (1.0f / (1 << x) );
		}
	}

	float intConv = (const float)(fixed.splitFormat.intPart);

	if (fixed.splitFormat.intPart & (1 << (integerBits - 1) ) )
	{
		// Negative number
		return ( (signed short)(fixed.splitFormat.intPart) + conv);
	}
	else
	{
		// Positive number
		return (fixed.splitFormat.intPart) + conv;
	}
}

template <const unsigned char integerBits, const unsigned char fractionBits, typename baseType>
static TFixed<integerBits, fractionBits, baseType> ConvertFloatToFixed(const float inFloat)
{
	const unsigned intPart = (const unsigned)inFloat;
	float fracPart = inFloat - intPart;

	TFixed<integerBits, fractionBits, baseType> ret;
	ret.whole = 0;

	ret.splitFormat.intPart = intPart;
	for (unsigned x = 0; x < fractionBits; ++x)
	{
		const float thisTestBit = 1.0f / (1 << (x + 1) );
		if (fracPart > thisTestBit)
		{
			ret.splitFormat.fracPart |= (1 << (fractionBits - (x + 1) ) );
			fracPart -= thisTestBit;
		}
	}

	return ret;
}

void NormalizeToRecipRange(const TFixed<16, 16> input, TFixed<16, 16>& outNormalizedFixed, int& normalizeFactor)
{
	if (input.splitFormat.intPart != 0)
	{
		unsigned long firstBitIndex = 0; // Starts at the LSB.
		_BitScanReverse(&firstBitIndex, input.splitFormat.intPart);
		normalizeFactor = firstBitIndex + 1;
		outNormalizedFixed.whole = input.whole >> normalizeFactor;
	}
	else if (input.whole == 0)
	{
		outNormalizedFixed.whole = -1;
		normalizeFactor = 0;
	}
	else
	{
		unsigned long firstBitIndex = 0; // Starts at the LSB, even though it scans backwards.
		_BitScanReverse(&firstBitIndex, input.splitFormat.fracPart);
		normalizeFactor = -(int)(16 - firstBitIndex - 1);
		outNormalizedFixed.whole = input.whole << -normalizeFactor;
	}
}

void UnNormalizeToRecipRange(const TFixed<16, 16> normalizedInput, TFixed<16, 16>& outRenormalized, const int normalizeFactor)
{
	if (normalizeFactor > 0)
	{
		outRenormalized.whole = normalizedInput.whole >> normalizeFactor;
	}
	else if (normalizeFactor == 0)
	{
		outRenormalized = normalizedInput;
	}
	else
	{
		outRenormalized.whole = normalizedInput.whole << -normalizeFactor;
	}
}

float FloatReciprocal(float input)
{
	const float approx0result = (48.0f / 17.0f) - (input * (32.0f / 17.0f) );
	float lastIterResult = approx0result;
	float currentIterResult;
	for (unsigned iters = 0; iters < 2; ++iters)
	{
		currentIterResult = lastIterResult * (2.0f - input * lastIterResult);
		lastIterResult = currentIterResult;
	}
	return currentIterResult;
}

static inline void TestSetMinMax(const TFixed<16, 16, unsigned>& inTestVal, signed short& minSet, signed short& maxSet)
{
	const signed short val = (signed short)(inTestVal.splitFormat.intPart);
	if (val < minSet)
		minSet = val;
	if (val > maxSet)
		maxSet = val;
}

// Newton-Raphson iteration using Q16.16 fixed-point numbers, with some help from this resource: http://www.cs.utsa.edu/~wagner/CS3343/newton/division.html
TFixed<16, 16> FixedReciprocal(TFixed<16, 16> input) // +0.xxxxxxxxdec = +0.xxxxhex, Q0.16 unsigned. Value between (0.5, 1.0)
{
	// Initial approximation is 48/17 - 32/17 * input
	TFixed<16, 16> approx0mul = ConvertFloatToFixed<16, 16, unsigned>(32.0f / 17.0f); // +1.8823529411764705882352941176471dec = +1.e1e1hex, Q1.16 unsigned

	TFixed<16, 16> approx0add = ConvertFloatToFixed<16, 16, unsigned>(48.0f / 17.0f); // +2.8235294117647058823529411764706dec = +2.d2d2hex, Q2.16 unsigned

	TFixed<16, 16> approx0mulresult = (input * approx0mul); // Q0.16 * Q1.16 = Q1.16 unsigned, value range between (0.941176 and 1.882353)

	TFixed<16, 16> approx0result = approx0add - approx0mulresult; // Q2.16 - Q1.16 = Q1.16, value range between (0.941176 and 1.882353 again)

	TFixed<16, 16> lastIterResult = approx0result;
	TFixed<16, 16> currentIterResult;

	// Each iteration computes: currentIterResult = lastIterResult * (2.0f - (input * lastIterResult) )
	// 5 iterations here is experimentally derived as being the maximum number of iterations before the result stabilizes
	for (unsigned iters = 0; iters < 5; ++iters)
	{
		TFixed<16, 16> intermedMul = input * lastIterResult; // Q0.16 * Q1.16 = Q1.16, value range for first iteration between (0.470588 and 1.882353)

		TFixed<16, 16, unsigned> fixedTwo = ConvertFloatToFixed<16, 16, unsigned>(2.0f); // Q2.16 unsigned (technically could be 2.0)
		TFixed<16, 16> intermedSubtract = fixedTwo - intermedMul; // Q2.16 - Q1.16 = Q1.16, value range for first iteration between (0.117647 and 1.529411)

		currentIterResult = lastIterResult * intermedSubtract; // Q1.16 * Q1.16 = Q1.16, value range for first iteration between (0.110727 and 2.878891) (don't worry this never actually goes over 1.9 in practice so it'll fit into a Q1.16)

		lastIterResult = currentIterResult;
	}

	return currentIterResult;
}

static inline const float BarycentricInverse(const int twiceTriangleArea)
{
	// Float version:
	//return 1.0f / twiceTriangleArea;

	// Fixed-point version:
	TFixed<16, 16> input = ConvertFloatToFixed<16, 16, unsigned>( (const float)twiceTriangleArea);
	TFixed<16, 16> normalizedInput;
	int normalizeFactor = 0;
	NormalizeToRecipRange(input, normalizedInput, normalizeFactor);
	TFixed<16, 16> recip = FixedReciprocal(normalizedInput);
	TFixed<16, 16, unsigned> output;
	UnNormalizeToRecipRange(recip, output, normalizeFactor);
	return ConvertFixedToFloat(output);
}

template <const bool rasterizerUsesEarlyZTest, const bool shadeFromShader>
void IDirect3DDevice9Hook::RasterizeTriangle(PShaderEngine* const pShaderEngine, const void* const mappingData, const void* const v0, const void* const v1, const void* const v2,
	const float fWidth, const float fHeight, const UINT primitiveID, const UINT vertex0index, const UINT vertex1index, const UINT vertex2index) const
{
	const D3DXVECTOR4& pos0 = shadeFromShader ? currentState.currentVertexShader->GetPosition(*(const VS_2_0_OutputRegisters* const)v0) : *(const D3DXVECTOR4* const)v0;
	const D3DXVECTOR4& pos1 = shadeFromShader ? currentState.currentVertexShader->GetPosition(*(const VS_2_0_OutputRegisters* const)v1) : *(const D3DXVECTOR4* const)v1;
	const D3DXVECTOR4& pos2 = shadeFromShader ? currentState.currentVertexShader->GetPosition(*(const VS_2_0_OutputRegisters* const)v2) : *(const D3DXVECTOR4* const)v2;

	const __m128 pos0vec = *(const __m128* const)&pos0;
	const __m128 pos1vec = *(const __m128* const)&pos1;
	const __m128 pos2vec = *(const __m128* const)&pos2;

	// Near plane culling:
	// TODO: Replace with real triangle clipping!
	const __m128 zCullVec = { pos0vec.m128_f32[2], pos1vec.m128_f32[2], pos2vec.m128_f32[2], 0.0f };
	// if (posN.z < 0.0f || posN.z > 1.0f) return;
	if (_mm_movemask_ps(_mm_or_ps(_mm_cmplt_ps(zCullVec, *(const __m128* const)&zeroVec), _mm_cmpgt_ps(zCullVec, *(const __m128* const)&staticColorWhiteOpaque) ) ) & 0x7)
		return;

	// Compute screenspace bounds for this triangle:
	__m128 topleft = _mm_min_ps(_mm_min_ps(pos0vec, pos1vec), pos2vec);
	__m128 botright = _mm_max_ps(_mm_max_ps(pos0vec, pos1vec), pos2vec);

	__m128 bounds = _mm_shuffle_ps(topleft, botright, _MM_SHUFFLE(1, 0, 1, 0) );

	// Cull triangles that intersect the guard band:
	// TODO: Replace with real triangle clipping!
	//if (_mm_movemask_ps(_mm_or_ps(_mm_cmplt_ps(bounds, guardBandMin), _mm_cmpgt_ps(bounds, guardBandMax) ) ) )
		//return;

	// Clip screenspace bounds to the screen size:
	topleft = _mm_max_ps(topleft, *(const __m128* const)&zeroVec);
	botright = _mm_min_ps(botright, _mm_set_ps(0.0f, 0.0f, fHeight, fWidth) );

	// Clip to the scissor rect, if enabled
	if (currentState.currentRenderStates.renderStatesUnion.namedStates.scissorTestEnable)
	{
		topleft = _mm_max_ps(topleft, currentState.currentScissorRect.topleftF);
		botright = _mm_min_ps(botright, currentState.currentScissorRect.botrightF);
	}

	// Clip to the depth buffer extents, if Z-testing is enabled
	const IDirect3DSurface9Hook* depthStencil;
	if (rasterizerUsesEarlyZTest)
	{
		depthStencil = currentState.currentDepthStencil;

		const __m128 depthExtents = depthStencil->GetInternalWidthHeightM2F();
		topleft = _mm_min_ps(topleft, depthExtents);
		botright = _mm_min_ps(botright, depthExtents);
	}

	bounds = _mm_shuffle_ps(topleft, botright, _MM_SHUFFLE(1, 0, 1, 0) );
	if (SUBPIXEL_ACCURACY_BIASMULT != 1)
		bounds = _mm_mul_ps(bounds, SUBPIXEL_ACCURACY_BIASMULT_SPLATTEDF);

	const __m128i intBounds = _mm_cvtps_epi32(bounds);

	// Align our triangle rasterization such that our quads line up with the render-target and depth-buffer's swizzles
	const __m128i intBoundsQuadAligned = _mm_and_si128(intBounds, intBoundsQuadAlignVec);
	const int xMin = intBoundsQuadAligned.m128i_i32[0];
	const int yMin = intBoundsQuadAligned.m128i_i32[1];
	const int xMax = intBoundsQuadAligned.m128i_i32[2];
	const int yMax = intBoundsQuadAligned.m128i_i32[3];

	// Early out on zero area triangles
	if (!(yMin <= yMax && xMin < xMax) )
		return;
	
	// Cull zero or negative-area triangles (with our default triangle winding being CW, this will also cull CCW triangles):
	const int maxNumPixels = (yMax - yMin) * (xMax - xMin);
	if (maxNumPixels < 1)
		return;

	const __m128i i0vec = (SUBPIXEL_ACCURACY_BIASMULT != 1) ? _mm_cvtps_epi32(_mm_mul_ps(pos0vec, SUBPIXEL_ACCURACY_BIASMULT_SPLATTEDF) ) : _mm_cvtps_epi32(pos0vec);
	const __m128i i1vec = (SUBPIXEL_ACCURACY_BIASMULT != 1) ? _mm_cvtps_epi32(_mm_mul_ps(pos1vec, SUBPIXEL_ACCURACY_BIASMULT_SPLATTEDF) ) : _mm_cvtps_epi32(pos1vec);
	const __m128i i2vec = (SUBPIXEL_ACCURACY_BIASMULT != 1) ? _mm_cvtps_epi32(_mm_mul_ps(pos2vec, SUBPIXEL_ACCURACY_BIASMULT_SPLATTEDF) ) : _mm_cvtps_epi32(pos2vec);
	int2 i0, i1, i2;
	i0.x = i0vec.m128i_i32[0];
	i0.y = i0vec.m128i_i32[1];
	i1.x = i1vec.m128i_i32[0];
	i1.y = i1vec.m128i_i32[1];
	i2.x = i2vec.m128i_i32[0];
	i2.y = i2vec.m128i_i32[1];

	// Cull backfacing triangles!
	const int twiceTriangleArea = computeEdgeSidedness(i0.x, i0.y, i1.x, i1.y, i2.x, i2.y);
	if (twiceTriangleArea <= 0)
		return;

	const float barycentricNormalizeFactor = 1.0f / twiceTriangleArea; // BarycentricInverse(twiceTriangleArea);
	//printf("TwiceArea: %i; Real: %f; Fixed: %f\n", twiceTriangleArea, 1.0f / twiceTriangleArea, barycentricNormalizeFactor);

	const __m128i y0011 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(i0vec), _mm_castsi128_ps(i1vec), _MM_SHUFFLE(1, 1, 1, 1) ) ); // Shuffles to be: i0.y, i0.y, i1.y, i1.y
	const __m128i y012 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(y0011), _mm_castsi128_ps(i2vec), _MM_SHUFFLE(0, 1, 2, 0) ) ); // Shuffles to be: i0.y, i1.y, i2.y, i2.x
	const __m128i y120 = _mm_shuffle_epi32(y012, _MM_SHUFFLE(0, 0, 2, 1) ); // Shuffles to be: i1.y, i2.y, i0.y, i0.y
	const __m128i barycentricXDelta = _mm_shuffle_epi32(_mm_sub_epi32(y012, y120), _MM_SHUFFLE(0, 0, 2, 1) ); // Shuffles to be barycentricXDelta1, barycentricXDelta2, barycentricXDelta0
	const __m128i barycentricXDelta2 = _mm_slli_epi32(barycentricXDelta, 1); // Times 2

	const __m128i x0011 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(i0vec), _mm_castsi128_ps(i1vec), _MM_SHUFFLE(0, 0, 0, 0) ) ); // Shuffles to be: i0.x, i0.x, i1.x, i1.x
	const __m128i x012 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(x0011), _mm_castsi128_ps(i2vec), _MM_SHUFFLE(0, 0, 2, 0) ) ); // Shuffles to be: i0.x, i1.x, i2.x, i2.x
	const __m128i x120 = _mm_shuffle_epi32(x012, _MM_SHUFFLE(0, 0, 2, 1) ); // Shuffles to be: i1.x, i2.x, i0.x, i0.x
	const __m128i barycentricYDelta = _mm_shuffle_epi32(_mm_sub_epi32(x120, x012), _MM_SHUFFLE(0, 0, 2, 1) ); // Shuffles to be barycentricYDelta1, barycentricYDelta2, barycentricYDelta0
	const __m128i barycentricYDelta2 = _mm_slli_epi32(barycentricYDelta, 1); // Times 2

	// Correct for top-left rule. Source: https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
	const char topleftEdgeBias0 = isTopLeftEdge(i1, i2) ? 0 : -1;
	const char topleftEdgeBias1 = isTopLeftEdge(i2, i0) ? 0 : -1;
	const char topleftEdgeBias2 = isTopLeftEdge(i0, i1) ? 0 : -1;

	const __m128i topleftEdgeBias = _mm_set_epi32(0, topleftEdgeBias2, topleftEdgeBias1, topleftEdgeBias0);

	const __m128 barycentricNormalizeFactorSplattedF = _mm_set1_ps(barycentricNormalizeFactor);
	__m128i rowReset = _mm_add_epi32(_mm_set_epi32(0, computeEdgeSidedness(i0.x, i0.y, i1.x, i1.y, xMin, yMin), computeEdgeSidedness(i2.x, i2.y, i0.x, i0.y, xMin, yMin), computeEdgeSidedness(i1.x, i1.y, i2.x, i2.y, xMin, yMin) ), topleftEdgeBias);

	__m128i earlyZTestDepthValue4;
	if (rasterizerUsesEarlyZTest)
	{
		// TODO: Don't assume less-than test for Z CMPFUNC
		float minDepthValue = pos0.z < pos1.z ? pos0.z : pos1.z;
		minDepthValue = minDepthValue < pos2.z ? minDepthValue : pos2.z;

		earlyZTestDepthValue4 = _mm_set1_epi32(depthStencil->GetRawDepthValueFromFloatDepth(minDepthValue) );
	}

	const primitivePixelJobData* const primitiveData = GetNewPrimitiveJobData<shadeFromShader>(v0, v1, v2, barycentricNormalizeFactor, primitiveID, twiceTriangleArea > 0, vertex0index, vertex1index, vertex2index,
		pos0vec, pos1vec, pos2vec);
	for (int y = yMin; y <= yMax; y += SUBPIXEL_ACCURACY_BIASMULT2)
	{
		// Reset at the next row:
		__m128i currentBarycentric[4];
		currentBarycentric[0] = rowReset;

		const __m128i y4 = _mm_add_epi32(_mm_set1_epi32(y), y4quadOffset);

		for (int x = xMin; x < xMax; x += SUBPIXEL_ACCURACY_BIASMULT2)
		{
			currentBarycentric[1] = _mm_add_epi32(currentBarycentric[0], barycentricXDelta);
			currentBarycentric[2] = _mm_add_epi32(currentBarycentric[0], barycentricYDelta);
			currentBarycentric[3] = _mm_add_epi32(currentBarycentric[2], barycentricXDelta);

			const __m128i masksVec = _mm_set_epi32(
				_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(currentBarycentric[3], oneMaskVec) ) ),
				_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(currentBarycentric[2], oneMaskVec) ) ),
				_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(currentBarycentric[1], oneMaskVec) ) ),
				_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpgt_epi32(currentBarycentric[0], oneMaskVec) ) ) );

			// At this stage, the mask determines which pixels are inside and which are outside of our triangle
			unsigned unifiedMask4 = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(masksVec, sevenVec), sevenVec) ) );
			const __m128i x4 = _mm_add_epi32(_mm_set1_epi32(x), x4quadOffset);
			if (rasterizerUsesEarlyZTest)
			{
				if (unifiedMask4 == 0xF) // All pixels inside triangle
				{
					const __m128i depth4 = depthStencil->GetRawDepth4(x4, y4);

					// TODO: Don't assume less-than test for Z CMPFUNC
					const unsigned depthMask = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmplt_epi32(earlyZTestDepthValue4, depth4) ) );
					unifiedMask4 &= depthMask;
				}
				else if  (unifiedMask4 > 0) // At least one pixel inside triangle
				{
					__m128i depth4;
					if (unifiedMask4 & 0x1)
						depth4.m128i_u32[0] = depthStencil->GetRawDepth(x4.m128i_i32[0], y4.m128i_i32[0]);
					if (unifiedMask4 & 0x2)
						depth4.m128i_u32[1] = depthStencil->GetRawDepth(x4.m128i_i32[1], y4.m128i_i32[1]);
					if (unifiedMask4 & 0x4)
						depth4.m128i_u32[2] = depthStencil->GetRawDepth(x4.m128i_i32[2], y4.m128i_i32[2]);
					if (unifiedMask4 & 0x8)
						depth4.m128i_u32[3] = depthStencil->GetRawDepth(x4.m128i_i32[3], y4.m128i_i32[3]);

					// TODO: Don't assume less-than test for Z CMPFUNC
					const unsigned depthMask = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmplt_epi32(earlyZTestDepthValue4, depth4) ) );
					unifiedMask4 &= depthMask;
				}
			}

			if (unifiedMask4 == 0xF) // All pixels inside triangle and pass early-Z
			{
				const __m128i barycentricsAdjusted4[4] =
				{
					_mm_sub_epi32(currentBarycentric[0], topleftEdgeBias),
					_mm_sub_epi32(currentBarycentric[1], topleftEdgeBias), 
					_mm_sub_epi32(currentBarycentric[2], topleftEdgeBias), 
					_mm_sub_epi32(currentBarycentric[3], topleftEdgeBias)
				};
				CreateNewPixelShadeJob4(x4, y4, barycentricsAdjusted4, primitiveData);
			}
			else if (unifiedMask4 > 0) // At least one pixel inside triangle and pass early-Z
			{
				if (unifiedMask4 & 0x1)
				{
					const __m128i barycentricAdjusted = _mm_sub_epi32(currentBarycentric[0], topleftEdgeBias);
					CreateNewPixelShadeJob(x4.m128i_i32[0], y4.m128i_i32[0], barycentricAdjusted, primitiveData);
				}
				if (unifiedMask4 & 0x2)
				{
					const __m128i barycentricAdjusted = _mm_sub_epi32(currentBarycentric[1], topleftEdgeBias);
					CreateNewPixelShadeJob(x4.m128i_i32[1], y4.m128i_i32[1], barycentricAdjusted, primitiveData);
				}
				if (unifiedMask4 & 0x4)
				{
					const __m128i barycentricAdjusted = _mm_sub_epi32(currentBarycentric[2], topleftEdgeBias);
					CreateNewPixelShadeJob(x4.m128i_i32[2], y4.m128i_i32[2], barycentricAdjusted, primitiveData);
				}
				if (unifiedMask4 & 0x8)
				{
					const __m128i barycentricAdjusted = _mm_sub_epi32(currentBarycentric[3], topleftEdgeBias);
					CreateNewPixelShadeJob(x4.m128i_i32[3], y4.m128i_i32[3], barycentricAdjusted, primitiveData);
				}
			}
			else
			{
				// No pixels inside triangle that passed early-Z, do nothing
			}

			currentBarycentric[0] = _mm_add_epi32(currentBarycentric[0], barycentricXDelta2);
		}

		rowReset = _mm_add_epi32(rowReset, barycentricYDelta2);
	}
}

COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE IDirect3DDevice9Hook::DrawPrimitiveUP(THIS_ D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	SIMPLE_FUNC_SCOPE();

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
	currentState.currentSoftUPStream.vertexBuffer->SoftUPSetInternalPointer( (const BYTE* const)pVertexStreamZeroData, numInputVerts * shortVertexStreamZeroStride);
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
	SIMPLE_FUNC_SCOPE();

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

	currentState.currentSoftUPStream.vertexBuffer->SoftUPSetInternalPointer( (const BYTE* const)pVertexStreamZeroData, NumVertices * shortVertexStreamZeroStride);
	currentState.currentSoftUPStream.streamOffset = 0;
	currentState.currentSoftUPStream.streamStride = shortVertexStreamZeroStride;
	currentState.currentSoftUPStream.streamDividerFrequency = D3DSTREAMSOURCE_INDEXEDDATA;

	currentState.currentSoftUPIndexBuffer->SoftUPSetInternalPointer(pIndexData, IndexDataFormat, PrimitiveType, PrimitiveCount);

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
	SIMPLE_FUNC_SCOPE();
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

	DeclarationSemanticMapping vertexDeclMapping;
	vertexDeclMapping.ClearSemanticMapping();
	vertexDeclMapping.ComputeMappingPS(currentState.currentVertexDecl, currentState.currentPixelShader);
	InitPixelShader(currentState, currentState.currentPixelShader->GetShaderInfo() );

	SetupCurrentDrawCallPixelData(false, &vertexDeclMapping);

	const float fWidth = currentState.cachedViewport.fWidth;
	const float fHeight = currentState.cachedViewport.fHeight;
	SetupCurrentDrawCallTriangleRasterizeData(fWidth, fHeight, rasterizerUsesEarlyZTest, false, &vertexDeclMapping);

	const DebuggableD3DVERTEXELEMENT9* const positionT0 = currentState.currentVertexDecl->GetPositionT0Element();
	if (!positionT0)
	{
#ifdef _DEBUG
		__debugbreak(); // We should never be here...
#endif
		return;
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
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, false, v0, v1, v2);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
#endif
				break;
			case D3DCULL_CW:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i2, i1, false, v0, v2, v1);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID, i0, i2, i1);
#endif
				break;
			case D3DCULL_NONE:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, false, v0, v1, v2);
				CreateNewTriangleRasterJob(primitiveID + primCount /*hack*/, i0, i2, i1, false, v0, v2, v1);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID + primCount /*hack*/, i0, i2, i1);
#endif
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
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, false, v0, v1, v2);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
#endif
				break;
			case D3DCULL_CW:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i2, i1, false, v0, v2, v1);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID, i0, i2, i1);
#endif
				break;
			case D3DCULL_NONE:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, false, v0, v1, v2);
				CreateNewTriangleRasterJob(primitiveID + primCount /*hack*/, i0, i2, i1, false, v0, v2, v1);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID + primCount /*hack*/, i0, i2, i1);
#endif
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
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, false, v0, v1, v2);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
#endif
				break;
			case D3DCULL_CW:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i2, i1, false, v0, v2, v1);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID, i0, i2, i1);
#endif
				break;
			case D3DCULL_NONE:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, false, v0, v1, v2);
				CreateNewTriangleRasterJob(primitiveID + primCount /*TODO: Hack*/, i0, i2, i1, false, v0, v2, v1);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v1, (const D3DXVECTOR4* const)v2, fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangle<rasterizerUsesEarlyZTest, false>(&deviceMainPShaderEngine, &vertexDeclMapping, (const D3DXVECTOR4* const)v0, (const D3DXVECTOR4* const)v2, (const D3DXVECTOR4* const)v1, fWidth, fHeight, primitiveID + primCount /*TODO: Hack*/, i0, i2, i1);
#endif
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
	SynchronizeThreads<
#if TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
		4
#else
		1
#endif
	>();
#endif
}

template <const bool rasterizerUsesEarlyZTest>
void IDirect3DDevice9Hook::DrawPrimitiveUB(const D3DPRIMITIVETYPE PrimitiveType, const UINT PrimitiveCount) const
{
	SIMPLE_FUNC_SCOPE();
#ifdef _DEBUG
	if (currentState.currentPixelShader && !currentState.currentPixelShader->jitShaderMain)
	{
		DbgPrint("Warning: Uncached pixel shader detected");
	}

	if (processedVertsUsed == 0)
	{
		__debugbreak();
	}

	if (PrimitiveCount == 0)
	{
		DbgBreakPrint("Error: Can't render with 0 primitives!");
	}
#endif

	const VS_2_0_OutputRegisters* const processedVertsBuffer = processedVertexBuffer;

	VStoPSMapping vStoPSMapping;
	vStoPSMapping.ClearSemanticMapping();
	vStoPSMapping.ComputeMappingVSToPS(currentState.currentVertexShader, currentState.currentPixelShader);

	InitPixelShader(currentState, currentState.currentPixelShader->GetShaderInfo() );
	SetupCurrentDrawCallPixelData(true, &vStoPSMapping);

	const float fWidth = currentState.cachedViewport.fWidth;
	const float fHeight = currentState.cachedViewport.fHeight;
	SetupCurrentDrawCallTriangleRasterizeData(fWidth, fHeight, rasterizerUsesEarlyZTest, true, &vStoPSMapping);

	switch (PrimitiveType)
	{
	case D3DPT_TRIANGLELIST:
	{
#ifdef _DEBUG
		const unsigned vertCount = PrimitiveCount * 3;
		if (vertCount > processedVertsUsed)
		{
			__debugbreak();
		}
#endif
		for (unsigned primitiveID = 0; primitiveID < PrimitiveCount; ++primitiveID)
		{
			const unsigned i0 = primitiveID * 3;
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
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, true, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2]);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
#endif
				break;
			case D3DCULL_CW:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i2, i1, true, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1]);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
#endif
				break;
			case D3DCULL_NONE:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, true, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2]);
				CreateNewTriangleRasterJob(primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i2, i1, true, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1]);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1], fWidth, fHeight, primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i2, i1);
#endif
				break;
			}
		}
	}
		break;
	case D3DPT_TRIANGLESTRIP:
	{
		const unsigned vertCount = PrimitiveCount + 2;
#ifdef _DEBUG
		if (vertCount > processedVertsUsed)
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
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
					CreateNewTriangleRasterJob(primitiveID, i0, i2, i1, true, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1]);
#else
					RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
#endif
					break;
				case D3DCULL_CW:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
					CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, true, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2]);
#else
					RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
#endif
					break;
				case D3DCULL_NONE:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
					CreateNewTriangleRasterJob(primitiveID, i0, i2, i1, true, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1]);
					CreateNewTriangleRasterJob(primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i1, i2, true, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2]);
#else
					RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
					RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2], fWidth, fHeight, primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i1, i2);
#endif
					break;
				}
			}
			else
			{
				switch (currentState.currentRenderStates.renderStatesUnion.namedStates.cullmode)
				{
				case D3DCULL_CCW:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
					CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, true, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2]);
#else
					RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
#endif
					break;
				case D3DCULL_CW:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
					CreateNewTriangleRasterJob(primitiveID, i0, i2, i1, true, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1]);
#else
					RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
#endif
					break;
				case D3DCULL_NONE:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
					CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, true, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2]);
					CreateNewTriangleRasterJob(primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i2, i1, true, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1]);
#else
					RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
					RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1], fWidth, fHeight, primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i2, i1);
#endif
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
		if (vertCount > processedVertsUsed)
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
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, true, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2]);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
#endif
				break;
			case D3DCULL_CW:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i2, i1, true, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1]);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1], fWidth, fHeight, primitiveID, i0, i2, i1);
#endif
				break;
			case D3DCULL_NONE:
#if defined(MULTITHREAD_SHADING) && TRIANGLEJOBS_OR_PIXELJOBS == TRIANGLEJOBS
				CreateNewTriangleRasterJob(primitiveID, i0, i1, i2, true, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2]);
				CreateNewTriangleRasterJob(primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i2, i1, true, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1]);
#else
				RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i1], &processedVertsBuffer[i2], fWidth, fHeight, primitiveID, i0, i1, i2);
				RasterizeTriangle<rasterizerUsesEarlyZTest, true>(&deviceMainPShaderEngine, &vStoPSMapping, &processedVertsBuffer[i0], &processedVertsBuffer[i2], &processedVertsBuffer[i1], fWidth, fHeight, primitiveID + PrimitiveCount /*TODO: Hack*/, i0, i2, i1);
#endif
				break;
			}
		}
	}
		break;
	case D3DPT_LINELIST:
	{
#ifdef _DEBUG
		const unsigned vertCount = PrimitiveCount * 2;
		if (vertCount > processedVertsUsed)
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
		if (vertCount > processedVertsUsed)
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
		if (vertCount > processedVertsUsed)
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
	{
		SIMPLE_NAME_SCOPE("SynchronizeThreads");
		SynchronizeThreads<
#if TRIANGLEJOBS_OR_PIXELJOBS == PIXELJOBS
			4
#else
			1
#endif
		>();
	}
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

#ifdef MULTITHREAD_SHADING
	for (unsigned x = 0; x < ARRAYSIZE(threadItem); ++x)
	{
		threadItem[x].threadVS_2_0.DeepCopy(deviceMainVShaderEngine);
		threadItem[x].threadPS_2_0.DeepCopy(deviceMainPShaderEngine);
	}
#endif // #ifdef MULTITHREAD_SHADING

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
	SynchronizeThreads<1>();
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
	enableDialogs(FALSE), sceneBegun(FALSE), implicitSwapChain(NULL), hConsoleHandle(INVALID_HANDLE_VALUE), overlayFontTexture(NULL), numPixelsPassedZTest(0), initialCreateFocusWindow(NULL), initialCreateDeviceWindow(NULL),
	processedVertexBuffer(NULL), processedVertsUsed(0), processVertsAllocated(0)
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

	alreadyShadedVerts32 = new std::vector<unsigned>();
	alreadyShadedVerts16 = new std::vector<unsigned short>();

	currentState.currentSoftUPStream.vertexBuffer = new IDirect3DVertexBuffer9Hook(NULL, this);
	currentState.currentSoftUPStream.vertexBuffer->MarkSoftBufferUP(true);

	currentState.currentSoftUPIndexBuffer = new IDirect3DIndexBuffer9Hook(NULL, this);
	currentState.currentSoftUPIndexBuffer->MarkSoftBufferUP(true);

	processedVertexBuffer = (VS_2_0_OutputRegisters* const)_mm_malloc(sizeof(VS_2_0_OutputRegisters), 16);
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

	delete alreadyShadedVerts32;
	alreadyShadedVerts32 = NULL;

	delete alreadyShadedVerts16;
	alreadyShadedVerts16 = NULL;

	if (processedVertexBuffer)
	{
		_mm_free(processedVertexBuffer);
		processedVertexBuffer = NULL;
	}

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
#ifdef OVERRIDE_FORCE_WINDOWED_MODE
	modifiedParams.Windowed = TRUE; // Force Windowed mode
	modifiedParams.FullScreen_RefreshRateInHz = 0;
	//modifiedParams.BackBufferFormat = D3DFMT_UNKNOWN;
#endif // #ifdef OVERRIDE_FORCE_WINDOWED_MODE

#ifdef OVERRIDE_FORCE_NO_VSYNC
	modifiedParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; // For performance, you're gonna want this enabled
#endif // #ifdef OVERRIDE_FORCE_NO_VSYNC

//#ifdef _DEBUG

	// We don't yet support multisampling
	modifiedParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	modifiedParams.MultiSampleQuality = 0;
	modifiedParams.Flags &= (~D3DPRESENTFLAG_DEVICECLIP); // Remove the DEVICECLIP flag so that there aren't weird artifacts when rendering into a window that spans multiple monitors
//#endif
}
