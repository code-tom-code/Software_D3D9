#pragma once

#include "ShaderAnalysis.h"

static const char* const shaderEntrypointName = "ShaderMain";
static const char* const shaderJITTempDirectory = 
#ifdef _DEBUG
	"shaderjitd";
#else
	"shaderjit";
#endif

// Not multithread-safe!
const char* const ConstructShaderJITName(const ShaderInfo& shaderInfo);

const bool JITNewShader(const ShaderInfo& shaderInfo, const char* const shaderFilename);

static inline void AppendString(std::vector<char>& cppfile, const char* const str)
{
	const unsigned len = strlen(str);

	for (unsigned x = 0; x < len; ++x)
		cppfile.push_back(str[x]);
}

// Internal functions, do not call:
void LoadPrefixFileInternal(std::vector<char>& cppfile);
const bool JITCPPFileInternal(const ShaderInfo& shaderInfo, const char* const shaderFilename);
