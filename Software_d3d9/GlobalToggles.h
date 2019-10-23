#pragma once

// Uncomment to disable fixed function shader caching
// #define NO_CACHING_FIXED_FUNCTION_SHADERS 1

// Uncomment to disable FVF vertex decl caching
// #define NO_CACHING_FVF_VERT_DECLS 1

// Uncomment this line for debugging vertex shader engines:
// Note that if the JIT pixel shader fails to compile, the renderer will still fall back to using the interpreter pixel shader engine regardless of this setting
// #define FORCE_INTERPRETED_VERTEX_SHADER 1

// Uncomment this line for debugging pixel shader engines:
// Note that if the JIT pixel shader fails to compile, the renderer will still fall back to using the interpreter pixel shader engine regardless of this setting
// #define FORCE_INTERPRETED_PIXEL_SHADER 1

// Comment out to disable
// #define DUMP_TEXTURES_ON_FIRST_SET 1

// Comment out to disable
// #define COMPUTE_SURFACE_HASHES_FOR_DEBUGGING 1

// Commment this out to enable "fully correct" gamma correction calculations
#define USE_CHEAP_GAMMA_APPROXIMATION 1

// Comment out to disable
#ifdef _DEBUG
// #define DEBUG_VERTEX_OUT_POSITIONS 1
#endif

// Uncomment to enable showing the shader compilation window when compiling (does not play nicely with fullscreen windows)
// #define DEBUG_SHOW_SHADERCOMPILE_WINDOW 1

// These are useful for debugging: Forcing windowed mode, and forcing no VSync
#ifdef _DEBUG
	#define OVERRIDE_FORCE_WINDOWED_MODE 1
#endif
#ifdef _DEBUG
	#define OVERRIDE_FORCE_NO_VSYNC 1
#endif
#define OVERRIDE_HIDE_CURSOR 1

// If this is not defined, all shaders will run in solo threads rather than warps (usually of at least 2x2 pixels or vertices)
#define RUN_SHADERS_IN_WARPS 1

// Defines for different kinds of parallel libraries
#define PARALLELLIB_CONCRT 1
#define PARALLELLIB_TBB 2

// Use this define to set which parallel library to use (ConcRT or TBB are currently supported):
#define PARALLEL_LIBRARY PARALLELLIB_CONCRT

// Comment this line out to disable multithread shading
#define MULTITHREAD_SHADING 1

// Comment these out to disable shader execution profiling
#ifndef MULTITHREAD_SHADING
	//#define PROFILE_AVERAGE_VERTEX_SHADE_TIMES 1
	//#define PROFILE_AVERAGE_PIXEL_SHADE_TIMES 1
#endif

// If defined, the software renderer will improve the system's scheduler resolution (which is useful when running many threads in the job system).
// This increase only lasts as long as the process is running, and the default scheduler resolution will be applied when the process terminates.
#ifdef MULTITHREAD_SHADING
	#define INCREASE_SYSTEM_SCHEDULER_RESOLUTION 1
#endif

#ifdef MULTITHREAD_SHADING
	#define TRIANGLEJOBS 1
	#define PIXELJOBS 2
	#define TRIANGLEJOBS_OR_PIXELJOBS PIXELJOBS
#endif

// Comment out to disable
//#define INDEX_BUFFER_MAGIC_COOKIE 1

// Comment out to disable
//#define VERTEX_BUFFER_MAGIC_COOKIE 1

// Comment out to disable
//#define SURFACE_MAGIC_COOKIE 1

// Comment out to disable printing when D3D hook objects are fully released (deleted)
#define DEBUGPRINT_D3DHOOKOBJECT_FULLRELEASES 1

// Comment out to disable memset'ing a D3DHook object after it's destructed
#define WIPE_ON_DESTRUCT_D3DHOOKOBJECT 1

// Comment out to use a more efficient allocator for surface alloc. Leave uncommented for a safer way to make sure reads and writes don't go past the end of the surface
#ifndef SURFACE_MAGIC_COOKIE
	//#define SURFACE_ALLOC_PAGE_NOACCESS 1
#endif

// If defined, this will wipe surfaces when D3DLOCK_DISCARD is specified during a LockRect operation
#ifdef _DEBUG
	#define SURFACE_ENFORCE_DISCARD_ON_LOCK 1
#endif

// Comment out to use a more efficient allocator for vertex buffer alloc. Leave uncommented for a safer way to make sure reads and writes don't go past the end of the buffer
#ifndef VERTEX_BUFFER_MAGIC_COOKIE
	//#define VERTEX_BUFFER_ALLOC_PAGE_NOACCESS 1
#endif

// If defined, this will force vertex buffer data to be made read-only after Unlock() is called
#ifdef VERTEX_BUFFER_ALLOC_PAGE_NOACCESS
	//#define VERTEX_BUFFER_ENFORCE_READONLY_WHILE_UNLOCKED 1
#endif

// If defined, this will wipe vertex buffers when D3DLOCK_DISCARD is specified during a Lock operation
#ifdef _DEBUG
	#define VERTEX_BUFFER_ENFORCE_DISCARD_ON_LOCK 1
#endif

// Comment out to use a more efficient allocator for index buffer alloc. Leave uncommented for a safer way to make sure reads and writes don't go past the end of the buffer
#ifndef INDEX_BUFFER_MAGIC_COOKIE
	//#define INDEX_BUFFER_ALLOC_PAGE_NOACCESS 1
#endif

// If defined, this will force index buffer data to be made read-only after Unlock() is called
#ifdef INDEX_BUFFER_ALLOC_PAGE_NOACCESS
	//#define INDEX_BUFFER_ENFORCE_READONLY_WHILE_UNLOCKED 1
#endif

// If defined, this will wipe index buffers when D3DLOCK_DISCARD is specified during a Lock operation
#ifdef _DEBUG
	#define INDEX_BUFFER_ENFORCE_DISCARD_ON_LOCK 1
#endif

// Uncomment this to enable holding the "END" key to skip draw calls (very useful in debug mode where draw calls can take a very long time to complete)
#define ENABLE_END_TO_SKIP_DRAWS 1

// Comment this out to allow the early Z testing optimization
//#define DISALLOW_EARLY_Z_TESTING 1
