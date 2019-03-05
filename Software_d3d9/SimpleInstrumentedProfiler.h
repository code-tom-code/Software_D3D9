#pragma once

#define ENABLE_SIMPLE_PROFILER 0

#if ENABLE_SIMPLE_PROFILER
	// TODO: Don't hardcode this path
	#include "C:\\Users\\Tom\\Documents\\Visual Studio 2017\\Projects\\SimpleHFileProfiler\\SimpleHFileProfiler\\SimpleProfiler.h"
#else
	#define SIMPLE_DYNAMIC_STRING_SCOPE(dynamicStringScopeName) dynamicStringScopeName
	#define SIMPLE_DYNAMIC_STRING_SCOPE_CONDITIONAL(dynamicStringScopeName, conditional) dynamicStringScopeName; (conditional)
	#define SIMPLE_NAME_SCOPE(scopeName) scopeName
	#define SIMPLE_NAME_SCOPE_CONDITIONAL(scopeName, conditional) scopeName; (conditional)
	#define SIMPLE_FUNC_SCOPE()
	#define SIMPLE_FUNC_SCOPE_CONDITIONAL(conditional) (conditional)
	#define SIMPLE_FRAME_END_MARKER()
#endif
