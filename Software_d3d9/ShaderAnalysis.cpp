#pragma once

#include "ShaderAnalysis.h"
#include <intrin.h> // for popcnt()

static inline void dprintf(ShaderInfo& shaderInfo, const char* const formatString, ...)
{
	char buffer[1024] = {0};

	va_list args;
	va_start(args, formatString);
#pragma warning(push)
#pragma warning(disable:4996)
	const int len = vsprintf(buffer, formatString, args);
#pragma warning(pop)
	va_end(args);

	if (len > 0)
	{
		shaderInfo.shaderDisasmText.insert(shaderInfo.shaderDisasmText.end(), buffer, buffer + len);
	}
}

void ShaderInfo::PrintShaderStatsToString(char (&outBuffer)[1024]) const
{
#pragma warning(push)
#pragma warning(disable:4996)
	sprintf(outBuffer, "Shader Version: %cs_%u_%u\n"
		"Shader Length (tokens): %u\n"
		"Instructions (ALU/TEX): %u/%u\n"
		"Temp registers used (r registers): %u/%u\n"
		"Input registers used (v registers): %u/%u\n"
		"Float constant registers used (c registers): %u\n"
		"Integer constant registers used (i registers): %u/%u\n"
		"Bool constant registers used (b registers): %u/%u\n"
		"Uses dynamic branching: %s\n"
		"Uses function calls: %s\n"
		"Uses int to float conversions: %s\n"
		"Uses TEXKILL instructions: %s\n"
		"Uses gradient instructions: %s\n"
		"Uses instruction predication: %s\n"
		"Uses scalar instruction co-issuing: %s\n"
		"Uses custom depth output: %s\n"
		"Uses vertex texture fetch (VTF): %s\n"
		"Uses multiple render targets (MRT): %s\n"
		"Has dependent texture fetches: %s\n",
		isPixelShader ? 'p' : 'v', shaderMajorVersion, shaderMinorVersion,
		shaderLengthDWORDs,
		numArithInstructions, numTexInstructions,
		numUniqueTempRegistersUsed, MAX_NUM_TEMP_REGISTERS,
		numUniqueInputRegistersUsed, isPixelShader ? ( (shaderMajorVersion < 2) ? 8 : 10) : 16,
		usedConstantsF.size(),
		usedConstantsI.size(), MAX_NUM_CONST_INT_REGISTERS,
		usedConstantsB.size(), MAX_NUM_CONST_BOOL_REGISTERS,
		usesDynamicBranching ? "TRUE" : "FALSE",
		usesFunctionCalls ? "TRUE" : "FALSE",
		usesIntToFloatConversions ? "TRUE" : "FALSE",
		usesTexkill ? "TRUE" : "FALSE",
		usesGradientInstructions ? "TRUE" : "FALSE",
		usesInstructionPredication ? "TRUE" : "FALSE",
		usesInstructionCoIssue ? "TRUE" : "FALSE",
		psWritesDepth ? "TRUE" : "FALSE",
		usesVertTextureFetch ? "TRUE" : "FALSE",
		usesMRT ? "TRUE" : "FALSE",
		hasDependentTextureFetches ? "TRUE" : "FALSE");
#pragma warning(pop)
}

// Note that ResolveDstParameter<print> also increments the bytecode stream pointer given to it
template <const bool print, const bool modifyRegister>
static inline void ResolveDstParameter(const DWORD*& bytecode, ShaderInfo& shaderInfo, unsigned long& uniqueTempRegistersUsed, outputRegisterWriteTracker& outRegisterwriteTracker)
{
	const dstParameterToken& dstParameter = *(const dstParameterToken* const)bytecode;
	const D3DSHADER_PARAM_REGISTER_TYPE registerType = dstParameter.GetRegisterType();
	unsigned index = dstParameter.GetRegisterIndex();

	const bool usesRelativeAddressing = dstParameter.GetRelativeAddressingType() == D3DSHADER_ADDRMODE_RELATIVE;

	const unsigned registerWriteMask = dstParameter.GetWriteMask();

	switch (registerType)
	{
	case D3DSPR_TEMP       :
		if (print) dprintf(shaderInfo, "r");
		uniqueTempRegistersUsed |= (1 << index);
		break;
	case D3DSPR_INPUT      :
		if (print) dprintf(shaderInfo, "v");
		break;
	case D3DSPR_CONST      :
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
		if (print) dprintf(shaderInfo, "c");
		break;
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		if (print)
		{
			shaderInfo.isPixelShader ? dprintf(shaderInfo, "t") : dprintf(shaderInfo, "a");
		}
		break;
	case D3DSPR_RASTOUT    :
		if (modifyRegister)
			outRegisterwriteTracker.SetAddOrSkip(registerType, index, (const BYTE)registerWriteMask, dstParameter, (const instructionToken* const)(bytecode - 1), usesRelativeAddressing ? (const dstParameterToken* const)(bytecode + 1) : NULL);

		if (shaderInfo.isPixelShader)
		{
			if (print) dprintf(shaderInfo, "RASTOUT");
		}
		else
		{
			switch (index)
			{
			case D3DSRO_POSITION:
				if (print) dprintf(shaderInfo, "oPos");
				break;
			case D3DSRO_FOG:
				if (print) dprintf(shaderInfo, "oFog");
				break;
			case D3DSRO_POINT_SIZE:
				if (print) dprintf(shaderInfo, "oPSize");
				break;
			default:
				if (print) dprintf(shaderInfo, "Unknown RASTOUT index specified (%u)", index);
				break;
			}
		}
		break;
	case D3DSPR_ATTROUT    :
		if (modifyRegister)
			outRegisterwriteTracker.SetAddOrSkip(registerType, index, (const BYTE)registerWriteMask, dstParameter, (const instructionToken* const)(bytecode - 1), usesRelativeAddressing ? (const dstParameterToken* const)(bytecode + 1) : NULL);

		if (print) dprintf(shaderInfo, "ATTROUT");
		break;
	case D3DSPR_OUTPUT  : // Also known as D3DSPR_TEXCRDOUT
		if (modifyRegister)
			outRegisterwriteTracker.SetAddOrSkip(registerType, index, (const BYTE)registerWriteMask, dstParameter, (const instructionToken* const)(bytecode - 1), usesRelativeAddressing ? (const dstParameterToken* const)(bytecode + 1) : NULL);

		if (print) dprintf(shaderInfo, "o");
		break;
	case D3DSPR_CONSTINT   :
		if (print) dprintf(shaderInfo, "i");
		break;
	case D3DSPR_COLOROUT   :
		if (modifyRegister)
			outRegisterwriteTracker.SetAddOrSkip(registerType, index, (const BYTE)registerWriteMask, dstParameter, (const instructionToken* const)(bytecode - 1), usesRelativeAddressing ? (const dstParameterToken* const)(bytecode + 1) : NULL);

		if (print) dprintf(shaderInfo, "oC");
		if (shaderInfo.isPixelShader)
		{
			shaderInfo.usedMRTMask |= (1 << index);
		}
		else
		{
			// Only pixel shaders can write to the oC# registers
			shaderInfo.parsingErrorDetected = true;
		}
		break;
	case D3DSPR_DEPTHOUT   :
		if (modifyRegister)
			outRegisterwriteTracker.SetAddOrSkip(registerType, index, (const BYTE)registerWriteMask, dstParameter, (const instructionToken* const)(bytecode - 1), usesRelativeAddressing ? (const dstParameterToken* const)(bytecode + 1) : NULL);

		if (print) dprintf(shaderInfo, "oDepth");
		if (shaderInfo.isPixelShader)
			shaderInfo.psWritesDepth = true;
		break;
	case D3DSPR_SAMPLER    :
		if (print) dprintf(shaderInfo, "s");
		break;
	case D3DSPR_CONSTBOOL  :
		if (print) dprintf(shaderInfo, "b");
		break;
	case D3DSPR_LOOP       :
		if (print) dprintf(shaderInfo, "aL");
		break;
	case D3DSPR_TEMPFLOAT16:
		if (print) dprintf(shaderInfo, "r");
		break;
	case D3DSPR_MISCTYPE   :
		if (print) dprintf(shaderInfo, "MISC");
		break;
	case D3DSPR_LABEL      :
		if (print) dprintf(shaderInfo, "LABEL");
		break;
	case D3DSPR_PREDICATE  :
		if (print) dprintf(shaderInfo, "p");
		break;
	default:
#ifdef _DEBUG
		{
			DbgBreakPrint("Error: Unknown shader register");
		}
#endif
		break;
	}

	// Print register index:
	switch (registerType)
	{
	case D3DSPR_CONST	   :
		break;
	case D3DSPR_CONST2     :
		index += 2048;
		break;
	case D3DSPR_CONST3     :
		index += 4096;
		break;
	case D3DSPR_CONST4     :
		index += 6144;
		break;
	default:
		break;
	}
	if (print) dprintf(shaderInfo, "%u", index);

	const unsigned resultModifier = dstParameter.GetResultModifierUnshifted();
	if (resultModifier & D3DSPDM_SATURATE)
		if (print) dprintf(shaderInfo, "_sat");
	if (resultModifier & D3DSPDM_PARTIALPRECISION)
		if (print) dprintf(shaderInfo, "_pp");
	if (resultModifier & D3DSPDM_MSAMPCENTROID)
		if (print) dprintf(shaderInfo, "_centroid");

	if (usesRelativeAddressing)
	{
		const dstParameterToken& relativeDstParameter = *(const dstParameterToken* const)(bytecode + 1);
		if (print) dprintf(shaderInfo, "[");
		switch (relativeDstParameter.GetRegisterType() )
		{
		case D3DSPR_ADDR:
			if (print) dprintf(shaderInfo, "a");
			break;
		case D3DSPR_LOOP:
			if (print) dprintf(shaderInfo, "aL");
			break;
		default:
#ifdef _DEBUG
			__debugbreak();
#endif
			if (print) dprintf(shaderInfo, "Error: Invalid relative offset register type specified (%u)!", relativeDstParameter.GetRegisterType() );
			break;
		}
		if (print) dprintf(shaderInfo, "%u", relativeDstParameter.GetRegisterIndex() );

		const unsigned relativeRegisterWriteMask = relativeDstParameter.GetWriteMask();
		if (relativeRegisterWriteMask != 0xF) // Omit displaying ".xyzw" after every register because it gets annoying!
		{
			bool writeMaskDotApplied = false;
			if (relativeRegisterWriteMask & 0x1) // x
			{
				if (print) dprintf(shaderInfo, ".x");
				writeMaskDotApplied = true;
			}
			if (relativeRegisterWriteMask & 0x2) // y
			{
				if (!writeMaskDotApplied)
				{
					if (print) dprintf(shaderInfo, ".y");
				}
				else
				{
					if (print) dprintf(shaderInfo, "y");
				}
				writeMaskDotApplied = true;
			}
			if (relativeRegisterWriteMask & 0x4) // z
			{
				if (!writeMaskDotApplied)
				{
					if (print) dprintf(shaderInfo, ".z");
				}
				else
				{
					if (print) dprintf(shaderInfo, "z");
				}
				writeMaskDotApplied = true;
			}
			if (relativeRegisterWriteMask & 0x8) // w
			{
				if (!writeMaskDotApplied)
				{
					if (print) dprintf(shaderInfo, ".w");
				}
				else
				{
					if (print) dprintf(shaderInfo, "w");
				}
			}
		}

		if (print) dprintf(shaderInfo, "]");

		// Super important to do this or else the byte-stream gets broken after the relative addressing token!
		++bytecode;
	}

	// Handle write masks:
	if (registerWriteMask != 0xF) // Omit displaying ".xyzw" after every register because it gets annoying!
	{
		bool writeMaskDotApplied = false;
		if (registerWriteMask & 0x1) // x
		{
			if (print) dprintf(shaderInfo, ".x");
			writeMaskDotApplied = true;
		}
		if (registerWriteMask & 0x2) // y
		{
			if (!writeMaskDotApplied)
			{
				if (print) dprintf(shaderInfo, ".y");
			}
			else
			{
				if (print) dprintf(shaderInfo, "y");
			}
			writeMaskDotApplied = true;
		}
		if (registerWriteMask & 0x4) // z
		{
			if (!writeMaskDotApplied)
			{
				if (print) dprintf(shaderInfo, ".z");
			}
			else
			{
				if (print) dprintf(shaderInfo, "z");
			}
			writeMaskDotApplied = true;
		}
		if (registerWriteMask & 0x8) // w
		{
			if (!writeMaskDotApplied)
			{
				if (print) dprintf(shaderInfo, ".w");
			}
			else
			{
				if (print) dprintf(shaderInfo, "w");
			}
		}
	}

	const unsigned shiftScale = dstParameter.GetResultShiftScale();
	if (shiftScale != 0)
	{
		if (shiftScale > 0x7)
		{
			// Multiply
			if (print) dprintf(shaderInfo, "_x%u", shiftScale & 0x7);
		}
		else
		{
			// Divide
			if (print) dprintf(shaderInfo, "_d%u", shiftScale & 0x7);
		}
	}

	// Super important to do this!
	++bytecode;
}

static inline void AddIfNotPresent(std::vector<unsigned>& vals, const unsigned val)
{
	const unsigned numVals = vals.size();
	for (unsigned x = 0; x < numVals; ++x)
	{
		if (vals[x] == val)
			return;
	}

	vals.push_back(val);
}

// Note that ResolveSrcParameter<print> also increments the bytecode stream pointer given to it
template <const bool print>
static inline void ResolveSrcParameter(ShaderInfo& shaderInfo, const DWORD*& bytecode, const bool isPixelShader, unsigned long& uniqueTempRegistersUsed, unsigned short& uniqueInputRegistersUsed)
{
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)bytecode;

	const D3DSHADER_PARAM_SRCMOD_TYPE sourceMod = srcParameter.GetSourceModifiersUnshifted();
	switch (sourceMod)
	{
	case D3DSPSM_NONE   :
	case D3DSPSM_BIAS   :
	case D3DSPSM_SIGN   :
	case D3DSPSM_COMP   :
	case D3DSPSM_X2     :
	case D3DSPSM_DW     :
	case D3DSPSM_DZ     :
		break;
	case D3DSPSM_NEG    :
	case D3DSPSM_BIASNEG:
	case D3DSPSM_SIGNNEG:
	case D3DSPSM_X2NEG  :
		if (print) dprintf(shaderInfo, "-");
		break;
	case D3DSPSM_ABS    :
		if (print) dprintf(shaderInfo, "abs(");
		break;
	case D3DSPSM_ABSNEG :
		if (print) dprintf(shaderInfo, "-abs(");
		break;
	case D3DSPSM_NOT    :
		if (print) dprintf(shaderInfo, "!");
		break;
	default:
		if (print) dprintf(shaderInfo, " Error: Unknown source modifier (%u) ", sourceMod >> D3DSP_SRCMOD_SHIFT);
		break;
	}

	unsigned index = srcParameter.GetRegisterIndex();

	const D3DSHADER_PARAM_REGISTER_TYPE registerType = srcParameter.GetRegisterType();
	switch (registerType)
	{
	case D3DSPR_TEMP       :
		if (print) dprintf(shaderInfo, "r");
		uniqueTempRegistersUsed |= (1 << index);
		break;
	case D3DSPR_INPUT      :
		if (print) dprintf(shaderInfo, "v");
		uniqueInputRegistersUsed |= (1 << index);
		break;
	case D3DSPR_CONST      :
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
		if (print) dprintf(shaderInfo, "c");
		break;
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		if (print)
		{
			isPixelShader ? dprintf(shaderInfo, "t") : dprintf(shaderInfo, "a");
		}
		if (shaderInfo.isPixelShader)
		{
			if (shaderInfo.shaderMajorVersion < 3)
				uniqueInputRegistersUsed |= (1 << (index + D3DMCS_COLOR2) ); // +2 for the Diffuse and Specular color registers
			else
				uniqueInputRegistersUsed |= (1 << index);
		}
		break;
	case D3DSPR_RASTOUT    :
		if (print) dprintf(shaderInfo, "RASTOUT");
		break;
	case D3DSPR_ATTROUT    :
		if (print) dprintf(shaderInfo, "ATTROUT");
		break;
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		if (print) dprintf(shaderInfo, "oT");
		break;
	case D3DSPR_CONSTINT   :
		if (print) dprintf(shaderInfo, "i");
		break;
	case D3DSPR_COLOROUT   :
		if (print) dprintf(shaderInfo, "oC");
		break;
	case D3DSPR_DEPTHOUT   :
		if (print) dprintf(shaderInfo, "oDepth");
		break;
	case D3DSPR_SAMPLER    :
		if (print) dprintf(shaderInfo, "s");
		break;
	case D3DSPR_CONSTBOOL  :
		if (print) dprintf(shaderInfo, "b");
		break;
	case D3DSPR_LOOP       :
		if (print) dprintf(shaderInfo, "aL");
		break;
	case D3DSPR_TEMPFLOAT16:
		if (print) dprintf(shaderInfo, "r");
		break;
	case D3DSPR_MISCTYPE   :
		if (print) dprintf(shaderInfo, "MISC");
		break;
	case D3DSPR_LABEL      :
		if (print) dprintf(shaderInfo, "LABEL");
		break;
	case D3DSPR_PREDICATE  :
		if (print) dprintf(shaderInfo, "p");
		break;
	default:
#ifdef _DEBUG
		{
			DbgBreakPrint("Error: Unknown shader register");
		}
#endif
		break;
	}

	// Print register index:
	switch (registerType)
	{
	case D3DSPR_CONST:
		break;
	case D3DSPR_CONST2     :
		index += 2048;
		break;
	case D3DSPR_CONST3     :
		index += 4096;
		break;
	case D3DSPR_CONST4     :
		index += 6144;
		break;
	default:
		break;
	}
	if (print) dprintf(shaderInfo, "%u", index);

	// Populate the "used constants" shader info:
	switch (registerType)
	{
	case D3DSPR_CONST:
	case D3DSPR_CONST2:
	case D3DSPR_CONST3:
	case D3DSPR_CONST4:
		AddIfNotPresent(shaderInfo.usedConstantsF, index);
		break;
	case D3DSPR_CONSTINT:
		AddIfNotPresent(shaderInfo.usedConstantsI, index);
		break;
	case D3DSPR_CONSTBOOL:
		AddIfNotPresent(shaderInfo.usedConstantsB, index);
		break;
	default:
		break;
	}

	switch (sourceMod)
	{
	default:
	case D3DSPSM_NONE   :
	case D3DSPSM_NOT    :
	case D3DSPSM_NEG    :
		break;
	case D3DSPSM_BIAS   :
	case D3DSPSM_BIASNEG:
		if (print) dprintf(shaderInfo, "_bias");
		break;
	case D3DSPSM_SIGN   :
	case D3DSPSM_SIGNNEG:
		if (print) dprintf(shaderInfo, "_bx2");
		break;
	case D3DSPSM_COMP   :
		if (print) dprintf(shaderInfo, "_complement");
		break;
	case D3DSPSM_X2     :
	case D3DSPSM_X2NEG  :
		if (print) dprintf(shaderInfo, "_x2");
		break;
	case D3DSPSM_DZ     :
		if (print) dprintf(shaderInfo, "_dz");
		break;
	case D3DSPSM_DW     :
		if (print) dprintf(shaderInfo, "_dw");
		break;
	case D3DSPSM_ABS    :
	case D3DSPSM_ABSNEG :
		if (print) dprintf(shaderInfo, ")");
		break;
	}

	if (srcParameter.GetRelativeAddressingType() == D3DSHADER_ADDRMODE_RELATIVE)
	{
		// Hack: If we're doing vs_1_* then the only register and only mask that can be used for relative addressing is the a0.x
		if (shaderInfo.shaderMajorVersion == 1)
		{
			if (print) dprintf(shaderInfo, "[a0.x]");

			// Do *not* advance the tokenstream in this special case!
		}
		else
		{
			const srcParameterToken& relativeSrcParameter = *(const srcParameterToken* const)(bytecode + 1);
			if (print) dprintf(shaderInfo, "[");
			const D3DSHADER_PARAM_REGISTER_TYPE relativeRegisterType = relativeSrcParameter.GetRegisterType();
			switch (relativeRegisterType)
			{
			case D3DSPR_ADDR:
				if (print) dprintf(shaderInfo, "a");
				break;
			case D3DSPR_LOOP:
				if (print) dprintf(shaderInfo, "aL");
				break;
			default:
#ifdef _DEBUG
				__debugbreak();
#endif
				if (print) dprintf(shaderInfo, "Error: Invalid relative offset register type specified (%u)!", relativeRegisterType);
				break;
			}
			if (print) dprintf(shaderInfo, "%u", relativeSrcParameter.GetRegisterIndex() );

			const unsigned channelSwizzle = relativeSrcParameter.GetChannelSwizzleXYZW();
			switch (channelSwizzle)
			{
			case D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT: // .xyzw
				break; // Don't print .xyzw all the time
			case D3DSP_REPLICATERED >> D3DVS_SWIZZLE_SHIFT: // .x
				if (print) dprintf(shaderInfo, ".x");
				break;
			case D3DSP_REPLICATEGREEN >> D3DVS_SWIZZLE_SHIFT: // .y
				if (print) dprintf(shaderInfo, ".y");
				break;
			case D3DSP_REPLICATEBLUE >> D3DVS_SWIZZLE_SHIFT: // .z
				if (print) dprintf(shaderInfo, ".z");
				break;
			case D3DSP_REPLICATEALPHA >> D3DVS_SWIZZLE_SHIFT: // .w
				if (print) dprintf(shaderInfo, ".w");
				break;
			default:
			{
				if (print)
				{
					static const char* const channelName[4] =
					{
						"x",
						"y",
						"z",
						"w"
					};

					dprintf(shaderInfo, ".%s", channelName[channelSwizzle & 0x3]);
					dprintf(shaderInfo, "%s", channelName[(channelSwizzle >> 2) & 0x3]);
					dprintf(shaderInfo, "%s", channelName[(channelSwizzle >> 4) & 0x3]);
					dprintf(shaderInfo, "%s", channelName[(channelSwizzle >> 6) & 0x3]);
				}
			}
				break;
			}

			if (print) dprintf(shaderInfo, "]");

			// Super important to do this, otherwise the bytecode stream gets broken after the relative addressing token!
			++bytecode;
		}
	}

	const unsigned channelSwizzle = srcParameter.GetChannelSwizzleXYZW();
	switch (channelSwizzle)
	{
	case D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT: // .xyzw
		break; // Don't print .xyzw all the time
	case D3DSP_REPLICATERED >> D3DVS_SWIZZLE_SHIFT: // .x
		if (print) dprintf(shaderInfo, ".x");
		break;
	case D3DSP_REPLICATEGREEN >> D3DVS_SWIZZLE_SHIFT: // .y
		if (print) dprintf(shaderInfo, ".y");
		break;
	case D3DSP_REPLICATEBLUE >> D3DVS_SWIZZLE_SHIFT: // .z
		if (print) dprintf(shaderInfo, ".z");
		break;
	case D3DSP_REPLICATEALPHA >> D3DVS_SWIZZLE_SHIFT: // .w
		if (print) dprintf(shaderInfo, ".w");
		break;
	default:
	{
		if (print)
		{
			static const char* const channelName[4] =
			{
				"x",
				"y",
				"z",
				"w"
			};

			dprintf(shaderInfo, ".%s", channelName[channelSwizzle & 0x3]);
			dprintf(shaderInfo, "%s", channelName[(channelSwizzle >> 2) & 0x3]);
			dprintf(shaderInfo, "%s", channelName[(channelSwizzle >> 4) & 0x3]);
			dprintf(shaderInfo, "%s", channelName[(channelSwizzle >> 6) & 0x3]);
		}
	}
		break;
	}

	// Super important to advance the bytecode stream
	++bytecode;
}

static const char* const opcodeToString[D3DSHADER_INSTRUCTION_OPCODE_TYPE::D3DSIO_BREAKP + 1] =
{
	"NOP", //D3DSIO_NOP          = 0,
    "MOV", //D3DSIO_MOV          ,
    "ADD", //D3DSIO_ADD          ,
    "SUB", //D3DSIO_SUB          ,
    "MAD", //D3DSIO_MAD          ,
    "MUL", //D3DSIO_MUL          ,
    "RCP", //D3DSIO_RCP          ,
    "RSQ", //D3DSIO_RSQ          ,
	"DP3", //D3DSIO_DP3          ,
    "DP4", //D3DSIO_DP4          ,
    "MIN", //D3DSIO_MIN          ,
    "MAX", //D3DSIO_MAX          ,
    "SLT", //D3DSIO_SLT          ,
    "SGE", //D3DSIO_SGE          ,
    "EXP", //D3DSIO_EXP          ,
    "LOG", //D3DSIO_LOG          ,
    "LIT", //D3DSIO_LIT          ,
    "DST", //D3DSIO_DST          ,
	"LRP", //D3DSIO_LRP          ,
    "FRC", //D3DSIO_FRC          ,
    "M4x4", //D3DSIO_M4x4         ,
    "M4x3", //D3DSIO_M4x3         ,
    "M3x4", //D3DSIO_M3x4         ,
    "M3x3", //D3DSIO_M3x3         ,
    "M3x2", //D3DSIO_M3x2         ,
    "CALL", //D3DSIO_CALL         ,
    "CALLNZ", //D3DSIO_CALLNZ       ,
    "LOOP", //D3DSIO_LOOP         ,
    "RET", //D3DSIO_RET          ,
    "ENDLOOP", //D3DSIO_ENDLOOP      ,
    "LABEL", //D3DSIO_LABEL        ,
    "DCL", //D3DSIO_DCL          ,
    "POW", //D3DSIO_POW          ,
    "CRS", //D3DSIO_CRS          ,
    "SGN", //D3DSIO_SGN          ,
    "ABS", //D3DSIO_ABS          ,
	"NRM", //D3DSIO_NRM          ,
    "SINCOS", //D3DSIO_SINCOS       ,
    "REP", //D3DSIO_REP          ,
    "ENDREP", //D3DSIO_ENDREP       ,
    "IF", //D3DSIO_IF           ,
    "IF_COMP", //D3DSIO_IFC          ,
    "ELSE", //D3DSIO_ELSE         ,
    "ENDIF", //D3DSIO_ENDIF        ,
    "BREAK", //D3DSIO_BREAK        ,
    "BREAK_COMP", //D3DSIO_BREAKC       ,
    "MOVA", //D3DSIO_MOVA         ,
    "DEFB", //D3DSIO_DEFB         ,
    "DEFI", //D3DSIO_DEFI         ,

	"UNK49",
	"UNK50",
	"UNK51",
	"UNK52",
	"UNK53",
	"UNK54",
	"UNK55",
	"UNK56",
	"UNK57",
	"UNK58",
	"UNK59",
	"UNK60",
	"UNK61",
	"UNK62",
	"UNK63",

    "TEXCOORD", //D3DSIO_TEXCOORD     = 64,
    "TEXKILL", //D3DSIO_TEXKILL      ,
    "TEX", //D3DSIO_TEX          ,
    "TEXBEM", //D3DSIO_TEXBEM       ,
    "TEXBEML", //D3DSIO_TEXBEML      ,
    "TEXREG2AR", //D3DSIO_TEXREG2AR    ,
    "TEXREG2GB", //D3DSIO_TEXREG2GB    ,
    "TEXM3x2PAD", //D3DSIO_TEXM3x2PAD   ,
    "TEXM3x2TEX", //D3DSIO_TEXM3x2TEX   ,
    "TEXM3x3PAD", //D3DSIO_TEXM3x3PAD   ,
    "TEXM3x3TEX", //D3DSIO_TEXM3x3TEX   ,
    "RESERVED0", //D3DSIO_RESERVED0    ,
    "TEXM3x3SPEC", //D3DSIO_TEXM3x3SPEC  ,
    "TEXM3x3VSPEC", //D3DSIO_TEXM3x3VSPEC ,
    "EXPP", //D3DSIO_EXPP         ,
    "LOGP", //D3DSIO_LOGP         ,
    "CND", //D3DSIO_CND          ,
    "DEF", //D3DSIO_DEF          ,
    "TEXREG2RGB", //D3DSIO_TEXREG2RGB   ,
    "TEXDP3TEX", //D3DSIO_TEXDP3TEX    ,
    "TEXM3x2DEPTH", //D3DSIO_TEXM3x2DEPTH ,
	"TEXDP3", //D3DSIO_TEXDP3       ,
    "TEXM3x3", //D3DSIO_TEXM3x3      ,
    "TEXDEPTH", //D3DSIO_TEXDEPTH     ,
    "CMP", //D3DSIO_CMP          ,
    "BEM", //D3DSIO_BEM          ,
    "DP2ADD", //D3DSIO_DP2ADD       ,
    "DSX", //D3DSIO_DSX          ,
    "DSY", //D3DSIO_DSY          ,
    "TEXLDD", //D3DSIO_TEXLDD       ,
    "SETP", //D3DSIO_SETP         ,
    "TEXLDL", //D3DSIO_TEXLDL       ,
    "BREAK_PRED", //D3DSIO_BREAKP       ,
};

static const char* const endOpcodes[] =
{
	"PHASE",
	"COMMENT",
	"END"
};

// Returns the opcode as a string suitable for diassembly
const char* const ShaderInfo::GetOpcodeString(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_BREAKP)
		return opcodeToString[opcode];
	else if (opcode >= D3DSIO_PHASE && opcode <= D3DSIO_END)
		return endOpcodes[opcode - D3DSIO_PHASE];
	else
		return "UNKNOWN_OPCODE";
}

static const char* const opcodeToFunction[D3DSHADER_INSTRUCTION_OPCODE_TYPE::D3DSIO_BREAKP + 1] =
{
	"nop", //D3DSIO_NOP          = 0,
    "mov", //D3DSIO_MOV          ,
    "add", //D3DSIO_ADD          ,
    "sub", //D3DSIO_SUB          ,
    "mad", //D3DSIO_MAD          ,
    "mul", //D3DSIO_MUL          ,
    "rcp", //D3DSIO_RCP          ,
    "rsq", //D3DSIO_RSQ          ,
	"dp3", //D3DSIO_DP3          ,
    "dp4", //D3DSIO_DP4          ,
    "min", //D3DSIO_MIN          ,
    "max", //D3DSIO_MAX          ,
    "slt", //D3DSIO_SLT          ,
    "sge", //D3DSIO_SGE          ,
    "exp", //D3DSIO_EXP          ,
    "log", //D3DSIO_LOG          ,
    "lit", //D3DSIO_LIT          ,
    "dst", //D3DSIO_DST          ,
	"lrp", //D3DSIO_LRP          ,
    "frc", //D3DSIO_FRC          ,
    "m4x4", //D3DSIO_M4x4         ,
    "m4x3", //D3DSIO_M4x3         ,
    "m3x4", //D3DSIO_M3x4         ,
    "m3x3", //D3DSIO_M3x3         ,
    "m3x2", //D3DSIO_M3x2         ,
    "call", //D3DSIO_CALL         ,
    "callnz", //D3DSIO_CALLNZ       ,
    "loop", //D3DSIO_LOOP         ,
    "ret", //D3DSIO_RET          ,
    "endloop", //D3DSIO_ENDLOOP      ,
    "label", //D3DSIO_LABEL        ,
    "dcl", //D3DSIO_DCL          ,
    "pow", //D3DSIO_POW          ,
    "crs", //D3DSIO_CRS          ,
    "sgn", //D3DSIO_SGN          ,
    "abs", //D3DSIO_ABS          ,
	"nrm", //D3DSIO_NRM          ,
    "sincos", //D3DSIO_SINCOS       ,
    "rep", //D3DSIO_REP          ,
    "endrep", //D3DSIO_ENDREP       ,
    "iffunc", //D3DSIO_IF           ,
    "if_comp", //D3DSIO_IFC          ,
    "elsefunc", //D3DSIO_ELSE         ,
    "endif", //D3DSIO_ENDIF        ,
    "breakfunc", //D3DSIO_BREAK        ,
    "break_comp", //D3DSIO_BREAKC       ,
    "mova", //D3DSIO_MOVA         ,
    "defb", //D3DSIO_DEFB         ,
    "defi", //D3DSIO_DEFI         ,

	"unk49",
	"unk50",
	"unk51",
	"unk52",
	"unk53",
	"unk54",
	"unk55",
	"unk56",
	"unk57",
	"unk58",
	"unk59",
	"unk60",
	"unk61",
	"unk62",
	"unk63",

    "texcoord", //D3DSIO_TEXCOORD     = 64,
    "texkill", //D3DSIO_TEXKILL      ,
    "tex", //D3DSIO_TEX          ,
    "texbem", //D3DSIO_TEXBEM       ,
    "texbeml", //D3DSIO_TEXBEML      ,
    "texreg2ar", //D3DSIO_TEXREG2AR    ,
    "texreg2gb", //D3DSIO_TEXREG2GB    ,
    "texm3x2pad", //D3DSIO_TEXM3x2PAD   ,
    "texm3x2tex", //D3DSIO_TEXM3x2TEX   ,
    "texm3x3pad", //D3DSIO_TEXM3x3PAD   ,
    "texm3x3tex", //D3DSIO_TEXM3x3TEX   ,
    "reserved0", //D3DSIO_RESERVED0    ,
    "texm3x3spec", //D3DSIO_TEXM3x3SPEC  ,
    "texm3x3vspec", //D3DSIO_TEXM3x3VSPEC ,
    "exp", //D3DSIO_EXPP         ,
    "log", //D3DSIO_LOGP         ,
    "cnd", //D3DSIO_CND          ,
    "def", //D3DSIO_DEF          ,
    "texreg2rgb", //D3DSIO_TEXREG2RGB   ,
    "texdp3tex", //D3DSIO_TEXDP3TEX    ,
    "texm3x2depth", //D3DSIO_TEXM3x2DEPTH ,
	"texdp3", //D3DSIO_TEXDP3       ,
    "texm3x3", //D3DSIO_TEXM3x3      ,
    "texdepth", //D3DSIO_TEXDEPTH     ,
    "cmp", //D3DSIO_CMP          ,
    "bem", //D3DSIO_BEM          ,
    "dp2add", //D3DSIO_DP2ADD       ,
    "dsx", //D3DSIO_DSX          ,
    "dsy", //D3DSIO_DSY          ,
    "texldd", //D3DSIO_TEXLDD       ,
    "setp", //D3DSIO_SETP         ,
    "texldl", //D3DSIO_TEXLDL       ,
    "break_pred", //D3DSIO_BREAKP       ,
};

// Returns the name of the function that this opcode maps to
const char* const ShaderInfo::GetOpcodeFunctionString(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_BREAKP)
		return opcodeToFunction[opcode];
	else
		return "function_not_found";
}

static const opcodeDisplayType opcodeTypes[D3DSHADER_INSTRUCTION_OPCODE_TYPE::D3DSIO_BREAKP + 1] =
{
	justOpcode, //D3DSIO_NOP          = 0,
    srcDst, //D3DSIO_MOV          ,
	srcSrcDst, //D3DSIO_ADD          ,
    srcSrcDst, //D3DSIO_SUB          ,
    srcSrcSrcDst, //D3DSIO_MAD          ,
    srcSrcDst, //D3DSIO_MUL          ,
    srcDst, //D3DSIO_RCP          ,
    srcDst, //D3DSIO_RSQ          ,
	srcSrcDst, //D3DSIO_DP3          ,
    srcSrcDst, //D3DSIO_DP4          ,
    srcSrcDst, //D3DSIO_MIN          ,
    srcSrcDst, //D3DSIO_MAX          ,
    srcSrcDst, //D3DSIO_SLT          ,
    srcSrcDst, //D3DSIO_SGE          ,
    srcDst, //D3DSIO_EXP          ,
    srcDst, //D3DSIO_LOG          ,
    srcDst, //D3DSIO_LIT          ,
    srcSrcDst, //D3DSIO_DST          ,
	srcSrcSrcDst, //D3DSIO_LRP          ,
    srcDst, //D3DSIO_FRC          ,
    srcSrcDst, //D3DSIO_M4x4         ,
    srcSrcDst, //D3DSIO_M4x3         ,
    srcSrcDst, //D3DSIO_M3x4         ,
    srcSrcDst, //D3DSIO_M3x3         ,
    srcSrcDst, //D3DSIO_M3x2         ,
    srcOnly, //D3DSIO_CALL         ,
    srcSrcOnly, //D3DSIO_CALLNZ       ,
    srcSrcOnly, //D3DSIO_LOOP         ,
    justOpcode, //D3DSIO_RET          ,
    justOpcode, //D3DSIO_ENDLOOP      ,
    customOpcode, //D3DSIO_LABEL        ,
    customOpcode, //D3DSIO_DCL          ,
    srcSrcDst, //D3DSIO_POW          ,
    srcSrcDst, //D3DSIO_CRS          ,
    srcSrcSrcDst, //D3DSIO_SGN          ,
    srcDst, //D3DSIO_ABS          ,
	srcDst, //D3DSIO_NRM          ,
    customOpcode, //D3DSIO_SINCOS       ,
    srcOnly, //D3DSIO_REP          ,
    justOpcode, //D3DSIO_ENDREP       ,
    srcOnly, //D3DSIO_IF           ,
    srcSrcOnly, //D3DSIO_IFC          ,
    justOpcode, //D3DSIO_ELSE         ,
    justOpcode, //D3DSIO_ENDIF        ,
    justOpcode, //D3DSIO_BREAK        ,
    srcSrcOnly, //D3DSIO_BREAKC       ,
    srcDst, //D3DSIO_MOVA         ,
    customOpcode, //D3DSIO_DEFB         ,
    customOpcode, //D3DSIO_DEFI         ,

	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,
	justOpcode,

    dstOnly, //D3DSIO_TEXCOORD     = 64,
    dstOnly, //D3DSIO_TEXKILL      ,
    customOpcode, //D3DSIO_TEX          ,
    srcDst, //D3DSIO_TEXBEM       ,
    srcDst, //D3DSIO_TEXBEML      ,
    srcDst, //D3DSIO_TEXREG2AR    ,
    srcDst, //D3DSIO_TEXREG2GB    ,
    srcDst, //D3DSIO_TEXM3x2PAD   ,
    srcDst, //D3DSIO_TEXM3x2TEX   ,
    srcDst, //D3DSIO_TEXM3x3PAD   ,
    srcDst, //D3DSIO_TEXM3x3TEX   ,
    justOpcode, //D3DSIO_RESERVED0    ,
    srcSrcDst, //D3DSIO_TEXM3x3SPEC  ,
    srcDst, //D3DSIO_TEXM3x3VSPEC ,
    srcDst, //D3DSIO_EXPP         ,
    srcDst, //D3DSIO_LOGP         ,
    srcSrcSrcDst, //D3DSIO_CND          ,
    customOpcode, //D3DSIO_DEF          ,
    srcDst, //D3DSIO_TEXREG2RGB   ,
    srcDst, //D3DSIO_TEXDP3TEX    ,
    srcDst, //D3DSIO_TEXM3x2DEPTH ,
	srcDst, //D3DSIO_TEXDP3       ,
    srcDst, //D3DSIO_TEXM3x3      ,
    dstOnly, //D3DSIO_TEXDEPTH     ,
    srcSrcSrcDst, //D3DSIO_CMP          ,
    srcSrcDst, //D3DSIO_BEM          ,
    srcSrcSrcDst, //D3DSIO_DP2ADD       ,
    srcDst, //D3DSIO_DSX          ,
    srcDst, //D3DSIO_DSY          ,
    srcSrcSrcSrcDst, //D3DSIO_TEXLDD       ,
    srcSrcDst, //D3DSIO_SETP         ,
    srcSrcDst, //D3DSIO_TEXLDL       ,
    srcOnly, //D3DSIO_BREAKP       ,
};

static const bool isOpcodeTexInstruction[D3DSHADER_INSTRUCTION_OPCODE_TYPE::D3DSIO_BREAKP + 1] =
{
	false, //D3DSIO_NOP          = 0,
    false, //D3DSIO_MOV          ,
	false, //D3DSIO_ADD          ,
    false, //D3DSIO_SUB          ,
    false, //D3DSIO_MAD          ,
    false, //D3DSIO_MUL          ,
    false, //D3DSIO_RCP          ,
    false, //D3DSIO_RSQ          ,
	false, //D3DSIO_DP3          ,
    false, //D3DSIO_DP4          ,
    false, //D3DSIO_MIN          ,
    false, //D3DSIO_MAX          ,
    false, //D3DSIO_SLT          ,
    false, //D3DSIO_SGE          ,
    false, //D3DSIO_EXP          ,
    false, //D3DSIO_LOG          ,
    false, //D3DSIO_LIT          ,
    false, //D3DSIO_DST          ,
	false, //D3DSIO_LRP          ,
    false, //D3DSIO_FRC          ,
    false, //D3DSIO_M4x4         ,
    false, //D3DSIO_M4x3         ,
    false, //D3DSIO_M3x4         ,
    false, //D3DSIO_M3x3         ,
    false, //D3DSIO_M3x2         ,
    false, //D3DSIO_CALL         ,
    false, //D3DSIO_CALLNZ       ,
    false, //D3DSIO_LOOP         ,
    false, //D3DSIO_RET          ,
    false, //D3DSIO_ENDLOOP      ,
    false, //D3DSIO_LABEL        ,
    false, //D3DSIO_DCL          ,
    false, //D3DSIO_POW          ,
    false, //D3DSIO_CRS          ,
    false, //D3DSIO_SGN          ,
    false, //D3DSIO_ABS          ,
	false, //D3DSIO_NRM          ,
    false, //D3DSIO_SINCOS       ,
    false, //D3DSIO_REP          ,
    false, //D3DSIO_ENDREP       ,
    false, //D3DSIO_IF           ,
    false, //D3DSIO_IFC          ,
    false, //D3DSIO_ELSE         ,
    false, //D3DSIO_ENDIF        ,
    false, //D3DSIO_BREAK        ,
    false, //D3DSIO_BREAKC       ,
    false, //D3DSIO_MOVA         ,
    false, //D3DSIO_DEFB         ,
    false, //D3DSIO_DEFI         ,

	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,

    false, //D3DSIO_TEXCOORD     = 64,
    false, //D3DSIO_TEXKILL      ,
    true, //D3DSIO_TEX          ,
    true, //D3DSIO_TEXBEM       ,
    true, //D3DSIO_TEXBEML      ,
    true, //D3DSIO_TEXREG2AR    ,
    true, //D3DSIO_TEXREG2GB    ,
    false, //D3DSIO_TEXM3x2PAD   ,
    true, //D3DSIO_TEXM3x2TEX   ,
    false, //D3DSIO_TEXM3x3PAD   ,
    true, //D3DSIO_TEXM3x3TEX   ,
    false, //D3DSIO_RESERVED0    ,
    true, //D3DSIO_TEXM3x3SPEC  ,
    true, //D3DSIO_TEXM3x3VSPEC ,
    false, //D3DSIO_EXPP         ,
    false, //D3DSIO_LOGP         ,
    false, //D3DSIO_CND          ,
    false, //D3DSIO_DEF          ,
    true, //D3DSIO_TEXREG2RGB   ,
    true, //D3DSIO_TEXDP3TEX    ,
    false, //D3DSIO_TEXM3x2DEPTH ,
	false, //D3DSIO_TEXDP3       ,
    false, //D3DSIO_TEXM3x3      ,
    false, //D3DSIO_TEXDEPTH     ,
    false, //D3DSIO_CMP          ,
    false, //D3DSIO_BEM          ,
    false, //D3DSIO_DP2ADD       ,
    false, //D3DSIO_DSX          ,
    false, //D3DSIO_DSY          ,
    true, //D3DSIO_TEXLDD       ,
    false, //D3DSIO_SETP         ,
    true, //D3DSIO_TEXLDL       ,
    false, //D3DSIO_BREAKP       ,
};

static const bool isDynamicBranchingInstruction[D3DSHADER_INSTRUCTION_OPCODE_TYPE::D3DSIO_BREAKP + 1] =
{
	false, //D3DSIO_NOP          = 0,
    false, //D3DSIO_MOV          ,
	false, //D3DSIO_ADD          ,
    false, //D3DSIO_SUB          ,
    false, //D3DSIO_MAD          ,
    false, //D3DSIO_MUL          ,
    false, //D3DSIO_RCP          ,
    false, //D3DSIO_RSQ          ,
	false, //D3DSIO_DP3          ,
    false, //D3DSIO_DP4          ,
    false, //D3DSIO_MIN          ,
    false, //D3DSIO_MAX          ,
    false, //D3DSIO_SLT          ,
    false, //D3DSIO_SGE          ,
    false, //D3DSIO_EXP          ,
    false, //D3DSIO_LOG          ,
    false, //D3DSIO_LIT          ,
    false, //D3DSIO_DST          ,
	false, //D3DSIO_LRP          ,
    false, //D3DSIO_FRC          ,
    false, //D3DSIO_M4x4         ,
    false, //D3DSIO_M4x3         ,
    false, //D3DSIO_M3x4         ,
    false, //D3DSIO_M3x3         ,
    false, //D3DSIO_M3x2         ,
    false, //D3DSIO_CALL         ,
    true, //D3DSIO_CALLNZ       ,
    false, //D3DSIO_LOOP         ,
    false, //D3DSIO_RET          ,
    false, //D3DSIO_ENDLOOP      ,
    false, //D3DSIO_LABEL        ,
    false, //D3DSIO_DCL          ,
    false, //D3DSIO_POW          ,
    false, //D3DSIO_CRS          ,
    false, //D3DSIO_SGN          ,
    false, //D3DSIO_ABS          ,
	false, //D3DSIO_NRM          ,
    false, //D3DSIO_SINCOS       ,
    false, //D3DSIO_REP          ,
    false, //D3DSIO_ENDREP       ,
    false, //D3DSIO_IF           ,
    true, //D3DSIO_IFC          ,
    false, //D3DSIO_ELSE         ,
    false, //D3DSIO_ENDIF        ,
    false, //D3DSIO_BREAK        ,
    true, //D3DSIO_BREAKC       ,
    false, //D3DSIO_MOVA         ,
    false, //D3DSIO_DEFB         ,
    false, //D3DSIO_DEFI         ,

	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,
	false,

    false, //D3DSIO_TEXCOORD     = 64,
    false, //D3DSIO_TEXKILL      ,
    false, //D3DSIO_TEX          ,
    false, //D3DSIO_TEXBEM       ,
    false, //D3DSIO_TEXBEML      ,
    false, //D3DSIO_TEXREG2AR    ,
    false, //D3DSIO_TEXREG2GB    ,
    false, //D3DSIO_TEXM3x2PAD   ,
    false, //D3DSIO_TEXM3x2TEX   ,
    false, //D3DSIO_TEXM3x3PAD   ,
    false, //D3DSIO_TEXM3x3TEX   ,
    false, //D3DSIO_RESERVED0    ,
    false, //D3DSIO_TEXM3x3SPEC  ,
    false, //D3DSIO_TEXM3x3VSPEC ,
    false, //D3DSIO_EXPP         ,
    false, //D3DSIO_LOGP         ,
    false, //D3DSIO_CND          ,
    false, //D3DSIO_DEF          ,
    false, //D3DSIO_TEXREG2RGB   ,
    false, //D3DSIO_TEXDP3TEX    ,
    false, //D3DSIO_TEXM3x2DEPTH ,
	false, //D3DSIO_TEXDP3       ,
    false, //D3DSIO_TEXM3x3      ,
    false, //D3DSIO_TEXDEPTH     ,
    false, //D3DSIO_CMP          ,
    false, //D3DSIO_BEM          ,
    false, //D3DSIO_DP2ADD       ,
    false, //D3DSIO_DSX          ,
    false, //D3DSIO_DSY          ,
    false, //D3DSIO_TEXLDD       ,
    false, //D3DSIO_SETP         ,
    false, //D3DSIO_TEXLDL       ,
    true, //D3DSIO_BREAKP       ,
};

static const int instructionTabIndents[D3DSHADER_INSTRUCTION_OPCODE_TYPE::D3DSIO_BREAKP + 1] =
{
	0, //D3DSIO_NOP          = 0,
    0, //D3DSIO_MOV          ,
	0, //D3DSIO_ADD          ,
    0, //D3DSIO_SUB          ,
    0, //D3DSIO_MAD          ,
    0, //D3DSIO_MUL          ,
    0, //D3DSIO_RCP          ,
    0, //D3DSIO_RSQ          ,
	0, //D3DSIO_DP3          ,
    0, //D3DSIO_DP4          ,
    0, //D3DSIO_MIN          ,
    0, //D3DSIO_MAX          ,
    0, //D3DSIO_SLT          ,
    0, //D3DSIO_SGE          ,
    0, //D3DSIO_EXP          ,
    0, //D3DSIO_LOG          ,
    0, //D3DSIO_LIT          ,
    0, //D3DSIO_DST          ,
	0, //D3DSIO_LRP          ,
    0, //D3DSIO_FRC          ,
    0, //D3DSIO_M4x4         ,
    0, //D3DSIO_M4x3         ,
    0, //D3DSIO_M3x4         ,
    0, //D3DSIO_M3x3         ,
    0, //D3DSIO_M3x2         ,
    0, //D3DSIO_CALL         ,
    0, //D3DSIO_CALLNZ       ,
    1, //D3DSIO_LOOP         ,
    0, //D3DSIO_RET          ,
    -1, //D3DSIO_ENDLOOP      ,
    0, //D3DSIO_LABEL        ,
    0, //D3DSIO_DCL          ,
    0, //D3DSIO_POW          ,
    0, //D3DSIO_CRS          ,
    0, //D3DSIO_SGN          ,
    0, //D3DSIO_ABS          ,
	0, //D3DSIO_NRM          ,
    0, //D3DSIO_SINCOS       ,
    1, //D3DSIO_REP          ,
    -1, //D3DSIO_ENDREP       ,
    1, //D3DSIO_IF           ,
    1, //D3DSIO_IFC          ,
    0, //D3DSIO_ELSE         ,
    -1, //D3DSIO_ENDIF        ,
    0, //D3DSIO_BREAK        ,
    0, //D3DSIO_BREAKC       ,
    0, //D3DSIO_MOVA         ,
    0, //D3DSIO_DEFB         ,
    0, //D3DSIO_DEFI         ,

	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,

    0, //D3DSIO_TEXCOORD     = 64,
    0, //D3DSIO_TEXKILL      ,
    0, //D3DSIO_TEX          ,
    0, //D3DSIO_TEXBEM       ,
    0, //D3DSIO_TEXBEML      ,
    0, //D3DSIO_TEXREG2AR    ,
    0, //D3DSIO_TEXREG2GB    ,
    0, //D3DSIO_TEXM3x2PAD   ,
    0, //D3DSIO_TEXM3x2TEX   ,
    0, //D3DSIO_TEXM3x3PAD   ,
    0, //D3DSIO_TEXM3x3TEX   ,
    0, //D3DSIO_RESERVED0    ,
    0, //D3DSIO_TEXM3x3SPEC  ,
    0, //D3DSIO_TEXM3x3VSPEC ,
    0, //D3DSIO_EXPP         ,
    0, //D3DSIO_LOGP         ,
    0, //D3DSIO_CND          ,
    0, //D3DSIO_DEF          ,
    0, //D3DSIO_TEXREG2RGB   ,
    0, //D3DSIO_TEXDP3TEX    ,
    0, //D3DSIO_TEXM3x2DEPTH ,
	0, //D3DSIO_TEXDP3       ,
    0, //D3DSIO_TEXM3x3      ,
    0, //D3DSIO_TEXDEPTH     ,
    0, //D3DSIO_CMP          ,
    0, //D3DSIO_BEM          ,
    0, //D3DSIO_DP2ADD       ,
    0, //D3DSIO_DSX          ,
    0, //D3DSIO_DSY          ,
    0, //D3DSIO_TEXLDD       ,
    0, //D3DSIO_SETP         ,
    0, //D3DSIO_TEXLDL       ,
    0, //D3DSIO_BREAKP       ,
};

const opcodeDisplayType ShaderInfo::GetOpcodeDisplayType(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_DEFI)
		return opcodeTypes[opcode];
	else if (opcode >= D3DSIO_TEXCOORD && opcode <= D3DSIO_BREAKP)
		return opcodeTypes[opcode];
	else
		return customOpcode;
}

struct ShaderComment_DBUG
{
	DWORD magic; // Should always be 'DBUG'
	DWORD size; // Should always be 0x28
	DWORD unknown0; // Seems to be 0x0290 (656 in dec)
	DWORD zero0;
	DWORD one0;
	DWORD unknown1; // Seems to be 0xA4 (164 in dec)
	DWORD unknown2; // Seems to be 0x19 (25 in dec)
	DWORD unknown3; // Seems to be 0xA8 (168 in dec)
	DWORD unknown4; // Seems to be 0x04
	DWORD unknown5; // Seems to be 0x0240 (576 in dec)
	DWORD unknown6; // Seems to be 0x0170 (368 in dec)
	char filename[1];
};

struct ShaderComment_CTAB
{
	DWORD magic; // Should always be 'CTAB'
	DWORD size; // Should always be 0x1C
	DWORD unknown0; // Seems to be 0x97 (151 dec)
	signed short unknown1; // Seems to be 0xFFFE (-2 in signed int)
	unsigned short unknown2; // Seems to be 0x200 (512 in dec)
	DWORD unknown3; // Seems to be 0x3 (the number of named global constants?)
	DWORD unknown4; // Seems to be 0x1C again
	DWORD unknown5; // Seems to be 0x105 (261 dec)
	DWORD unknown6; // Seems to be 0x90 (144 dec)
	DWORD unknown7; // Seems to be 0x58 (88 dec)
	unsigned short unknown8; // Seems to be 0x2
	unsigned short unknown9; // Seems to be 0x8
	unsigned short unknown10; // Seems to be 0x100 (256 dec)
	unsigned short unknown11; // Seems to be 0x2200 (8704 dec)
	unsigned short unknown12; // Seems to be 0x6400 (25600 dec)
	unsigned short zero0;
};

static const char* const usageStrings[] =
{
	"_POSITION",
	"_BLENDWEIGHT", 
	"_BLENDINDICES",
	"_NORMAL",      
	"_PSIZE",       
	"_TEXCOORD",    
	"_TANGENT",     
	"_BINORMAL",    
	"_TESSFACTOR",  
	"_POSITIONT",   
	"_COLOR",       
	"_FOG",         
	"_DEPTH",       
	"_SAMPLE"
};

template <const bool print>
static inline const bool ParseCustomOpcode(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode, const DWORD*& shaderMemory, ShaderInfo& shaderInfo, unsigned long& uniqueTempRegistersUsed, unsigned short& uniqueInputRegistersUsed, outputRegisterWriteTracker& outRegisterwriteTracker)
{
	if (print) dprintf(shaderInfo, "%s ", ShaderInfo::GetOpcodeString(opcode) );

	switch (opcode)
	{
	case D3DSIO_LABEL:
	{
		ShaderLabel newLabel;
		const srcParameterToken& srcParameter = *(const srcParameterToken* const)shaderMemory;
		newLabel.labelIndex = srcParameter.GetRegisterIndex();
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed); // Note that ResolveSrcParameter<print> also increments the bytecode stream pointer given to it
		newLabel.firstPostLabelToken = shaderMemory;
		shaderInfo.labels.push_back(newLabel);
	}
		break;
	case D3DSIO_DCL:
	{
		const DWORD dwordToken = *shaderMemory++;
		const dstParameterToken& dstParameter = *(const dstParameterToken* const)shaderMemory++;
		const D3DSHADER_PARAM_REGISTER_TYPE registerType = dstParameter.GetRegisterType();

		const D3DDECLUSAGE usage = (const D3DDECLUSAGE)( (dwordToken & D3DSP_DCL_USAGE_MASK) >> D3DSP_DCL_USAGE_SHIFT);
		const unsigned usageIndex = (dwordToken & D3DSP_DCL_USAGEINDEX_MASK) >> D3DSP_DCL_USAGEINDEX_SHIFT;

		DeclaredRegister newRegisterDcl;
		newRegisterDcl.isOutputRegister = registerType == D3DSPR_OUTPUT;
		newRegisterDcl.registerIndex = dstParameter.GetRegisterIndex();
		newRegisterDcl.registerType = registerType;
		newRegisterDcl.writeMask = dstParameter.GetWriteMask();
		newRegisterDcl.usageType = usage;
		newRegisterDcl.usageIndex = usageIndex;
		newRegisterDcl.miscInfo.uintData = 0;

		// Decode the DWORD token depending on the type of the Dst parameter
		switch (registerType)
		{
		case D3DSPR_SAMPLER:
		{
			const D3DSAMPLER_TEXTURE_TYPE textureType = (const D3DSAMPLER_TEXTURE_TYPE)(dwordToken & D3DSP_TEXTURETYPE_MASK);
			switch (textureType)
			{
			default:
				if (print) dprintf(shaderInfo, "Unknown sampler type (%u)\n", textureType >> D3DSP_TEXTURETYPE_SHIFT);
				break;
			case D3DSTT_UNKNOWN:
				if (print) dprintf(shaderInfo, "Unknown sampler type\n");
				break;
			case D3DSTT_2D     :
				if (print) dprintf(shaderInfo, "_2d");
				break;
			case D3DSTT_CUBE   :
				if (print) dprintf(shaderInfo, "_cube");
				break;
			case D3DSTT_VOLUME :
				if (print) dprintf(shaderInfo, "_volume");
				break;
			}
			newRegisterDcl.miscInfo.textureType = textureType;
			break;
		}
		case D3DSPR_TEXTURE:
		{
			// Nothing special here!
		}
			break;

		case D3DSPR_INPUT:
		{
			if (print) dprintf(shaderInfo, "%s", usageStrings[usage]);
			if (print) dprintf(shaderInfo, "%u ", usageIndex);
		}
			break;
		case D3DSPR_OUTPUT:
		{
			const DWORD* temp = &dstParameter.internalRawToken;
			ResolveDstParameter<print, false>(temp, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		}
			break;
		case D3DSPR_MISCTYPE: // Either VPOS (0) or FACE (1)
		{
			const D3DSHADER_MISCTYPE_OFFSETS miscRegisterType = (const D3DSHADER_MISCTYPE_OFFSETS)dstParameter.GetRegisterIndex();
			switch (miscRegisterType)
			{
			case D3DSMO_POSITION:
				if (print) dprintf(shaderInfo, "_VPOS");
				break;
			case D3DSMO_FACE:
				if (print) dprintf(shaderInfo, "_FACE");
				break;
			default:
				if (print) dprintf(shaderInfo, "Error: Unknown MISC register file index (%u)", miscRegisterType);
				break;
			}

			newRegisterDcl.miscInfo.miscInputType = miscRegisterType;
		}
			break;
		default:
			break;
		}

		shaderInfo.declaredRegisters.push_back(newRegisterDcl);

		if (print) dprintf(shaderInfo, " ");

		// Print the register afterwards:
		{
			const DWORD* temp = &dstParameter.internalRawToken;
			ResolveDstParameter<print, false>(temp, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		}

		// Print the usage for this decl after that (this isn't what D3DXDisasm does, but it really helps with debugging shaders to have it shown somewhere)
		if (print)
		{
			dprintf(shaderInfo, " : %s%u", usageStrings[usage], usageIndex);
		}
	}
		break;
	case D3DSIO_SINCOS:
		ResolveDstParameter<print, true>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (shaderInfo.shaderMajorVersion < 3) // Shader model 2 has these extra registers, but all the shader model 3+ don't have them
		{
			if (print) dprintf(shaderInfo, ", ");
			ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
			if (print) dprintf(shaderInfo, ", ");
			ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		}
		break;
	case D3DSIO_DEFB:
	{
		InitialConstantValueB newConstantB;
		const dstParameterToken& dstParameter = *(const dstParameterToken* const)shaderMemory;
		newConstantB.constantRegisterIndex = dstParameter.GetRegisterIndex();
		ResolveDstParameter<print, false>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		const DWORD dwordData = *shaderMemory++;
		const BOOL bData = (const BOOL)(dwordData);
		if (bData)
		{
			if (print) dprintf(shaderInfo, ", TRUE");
			newConstantB.initialValue = TRUE;
		}
		else
		{
			if (print) dprintf(shaderInfo, ", FALSE");
			newConstantB.initialValue = FALSE;
		}

		shaderInfo.initialConstantValuesB.push_back(newConstantB);
	}
		break;
	case D3DSIO_DEFI:
	{
		InitialConstantValueI newConstantI;
		const dstParameterToken& dstParameter = *(const dstParameterToken* const)shaderMemory;
		newConstantI.constantRegisterIndex = dstParameter.GetRegisterIndex();
		ResolveDstParameter<print, false>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		const DWORD dword0 = *shaderMemory++;
		newConstantI.initialValue.x = *(const int* const)&dword0;
		const DWORD dword1 = *shaderMemory++;
		newConstantI.initialValue.y = *(const int* const)&dword1;
		const DWORD dword2 = *shaderMemory++;
		newConstantI.initialValue.z = *(const int* const)&dword2;
		const DWORD dword3 = *shaderMemory++;
		newConstantI.initialValue.w = *(const int* const)&dword3;

		if (print) dprintf(shaderInfo, ", (%i, %i, %i, %i)", newConstantI.initialValue.x, newConstantI.initialValue.y, newConstantI.initialValue.z, newConstantI.initialValue.w);

		shaderInfo.initialConstantValuesI.push_back(newConstantI);
	}
		break;
	case D3DSIO_DEF:
	{
		InitialConstantValue newConstant;
		const dstParameterToken& dstParameter = *(const dstParameterToken* const)shaderMemory;
		newConstant.constantRegisterIndex = dstParameter.GetRegisterIndex();
		ResolveDstParameter<print, false>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		const DWORD dword0 = *shaderMemory++;
		newConstant.initialValue.x = *(const float* const)&dword0;
		const DWORD dword1 = *shaderMemory++;
		newConstant.initialValue.y = *(const float* const)&dword1;
		const DWORD dword2 = *shaderMemory++;
		newConstant.initialValue.z = *(const float* const)&dword2;
		const DWORD dword3 = *shaderMemory++;
		newConstant.initialValue.w = *(const float* const)&dword3;

		if (print) dprintf(shaderInfo, ", (%f, %f, %f, %f)", newConstant.initialValue.x, newConstant.initialValue.y, newConstant.initialValue.z, newConstant.initialValue.w);

		shaderInfo.initialConstantValues.push_back(newConstant);
	}
		break;
	case D3DSIO_TEX: // TEX means a whole lot of things. It can be tex from ps_1_0 thru ps_1_3, it can be texld from ps_1_4, or it can be texld for ps_1_4 and up.
		// Depending on the opcode controls, this can either be "plain texld/tex2D" (no controls), or texldp/tex2Dproj or texldb/tex2Dbias
		// Depending on the sampler source register, this can also be either tex1D, tex2D, tex3D, or texCUBE
	{
		ResolveDstParameter<print, true>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		if (shaderInfo.shaderMajorVersion < 2)
		{
			if (shaderInfo.shaderMinorVersion < 4)
			{
				// This is the ps_1_0 thru ps_1_3 case: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/tex---ps
				// Just T dst
			}
			else
			{
				// This is the ps_1_4 case: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/texld---ps-1-4
				// tex R dst, T/R src0
				if (print) dprintf(shaderInfo, ", ");
				ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
			}
		}
		else
		{
			// This is the ps_2_0+ case: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/texld---ps-2-0
			// tex R dst, T/R/V src0, S src1
			if (print) dprintf(shaderInfo, ", ");
			ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);

			if (print) dprintf(shaderInfo, ", ");
			ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		}
	}
		break;
	case D3DSIO_PHASE:
		break;
	case D3DSIO_COMMENT:
	{
		const DWORD commentToken = shaderMemory[-1];
		const unsigned numTokensToSkip = (commentToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;

		ShaderComment newComment;
		newComment.commentLengthTokens = numTokensToSkip;
		newComment.commentText = (const char* const)shaderMemory;
		shaderInfo.comments.push_back(newComment);

		switch (*shaderMemory)
		{
		case 'GUBD': // DBUG
		{
			const ShaderComment_DBUG* const dbug = (const ShaderComment_DBUG* const)shaderMemory;
			if (dbug->magic == 'GUBD')
			{
				if (dbug->size == 0x28)
				{
					if (print) dprintf(shaderInfo, "Shader compiled from: \"%s\"\n", dbug->filename);
				}
				else
				{
					if (print) dprintf(shaderInfo, "Error: Unexpected DBUG struct size detected! (Got %u, expected %u)\n", dbug->size, 0x28);
				}
			}
			else
			{
				if (print) dprintf(shaderInfo, "Error: Invalid DBUG struct magic value detected!\n");
			}
		}
			break;
		case 'BATC': // CTAB
		{
			const ShaderComment_CTAB* const ctab = (const ShaderComment_CTAB* const)shaderMemory;
			if (ctab->magic == 'BATC')
			{
				; // 
			}
			else
			{
				if (print) dprintf(shaderInfo, "Error: Invalid CTAB struct magic value detected!\n");
			}
		}
			break;
		}
		
		shaderMemory += numTokensToSkip;
	}
		break;
	case D3DSIO_END:
		if (print) dprintf(shaderInfo, "\n");
		shaderInfo.shaderLengthDWORDs = shaderMemory - shaderInfo.initialBytecodeToken;
		return true;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Unknown shader opcode");
#endif
		if (print) dprintf(shaderInfo, "Unknown opcode encountered (value 0x%08X)\n", opcode);
		break;
	}

	if (print) dprintf(shaderInfo, "\n");

	return false;
}

// Returns true if this opcode is a TEX instruction, or false otherwise
const bool ShaderInfo::IsOpcodeTexInstruction(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_BREAKP)
		return isOpcodeTexInstruction[opcode];
	else
		return false;
}

static inline const bool IsDynamicBranchingInstruction(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_BREAKP)
		return isDynamicBranchingInstruction[opcode];
	else
		return false;
}

static inline const int GetTabIndentModifier(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_BREAKP)
		return instructionTabIndents[opcode];
	else
		return 0;
}

template <const bool print>
static inline const bool ParseOpcode(const DWORD*& shaderMemory, ShaderInfo& shaderInfo, unsigned long& uniqueTempRegistersUsed, unsigned short& uniqueInputRegistersUsed, int& tabIndents, outputRegisterWriteTracker& outRegisterwriteTracker)
{
	const instructionToken rawInstructionToken = *(const instructionToken* const)shaderMemory++;
	const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode = (const D3DSHADER_INSTRUCTION_OPCODE_TYPE)(rawInstructionToken.opcode);

	const bool opcodeIsTexInstruction = ShaderInfo::IsOpcodeTexInstruction(opcode);
	unsigned* const instructionCount = &(shaderInfo.numArithInstructions);
	++instructionCount[opcodeIsTexInstruction];

	const bool opcodeIsDynamicBranchInstruction = IsDynamicBranchingInstruction(opcode);
	if (opcodeIsDynamicBranchInstruction)
		shaderInfo.usesDynamicBranching = true;

	if (opcode == D3DSIO_TEXKILL)
		shaderInfo.usesTexkill = true;

	if (rawInstructionToken.predicated)
		shaderInfo.usesInstructionPredication = true;

	if (rawInstructionToken.coIssue)
		shaderInfo.usesInstructionCoIssue = true;

	if (opcode == D3DSIO_TEXDEPTH || opcode == D3DSIO_TEXM3x2DEPTH)
		shaderInfo.psWritesDepth = true;

	switch (opcode)
	{
	case D3DSIO_CALL:
	case D3DSIO_CALLNZ:
		shaderInfo.usesFunctionCalls = true;
		break;
	default:
		break;
	}

	if (!shaderInfo.firstInstructionToken)
	{
		switch (opcode)
		{
			// Skippable instructions:
		case D3DSIO_NOP:
		case D3DSIO_DCL:
		case D3DSIO_DEF:
		case D3DSIO_PHASE:
		case D3DSIO_COMMENT:
		case D3DSIO_END:
			break;
			// First non-skippable instruction:
		default:
			shaderInfo.firstInstructionToken = shaderMemory - 1;
			break;
		}
	}

	switch (opcode)
	{
	case D3DSIO_DSX:
	case D3DSIO_DSY:
		if (!shaderInfo.isPixelShader)
			shaderInfo.parsingErrorDetected = true; // ddx and ddy are only available for pixel shaders!
		shaderInfo.usesGradientInstructions = true;
		break;
	default:
		break;
	}

	// Handle tabs:
	if (print)
	{
		for (int x = 0; x < tabIndents; ++x)
			dprintf(shaderInfo, "\t");

		tabIndents += GetTabIndentModifier(opcode);
#ifdef _DEBUG
		if (tabIndents < 0)
		{
			DbgBreakPrint("Error: Tab indents negative");
		}
#endif
	}

	if (opcode == D3DSIO_MOV)
	{
		// Handle int->float conversion checks:
		const srcParameterToken& srcParameter = *(const srcParameterToken* const)(shaderMemory + 1);
		switch (srcParameter.GetRegisterType() )
		{
		case D3DSPR_CONSTINT:
		case D3DSPR_CONSTBOOL:
			shaderInfo.usesIntToFloatConversions = true;
			break;
		default:
			break;
		}

		// Handle MRT detection:
		if (shaderInfo.isPixelShader)
		{
			const dstParameterToken& dstParameter = *(const dstParameterToken* const)shaderMemory;
			if (dstParameter.GetRegisterType() == D3DSPR_COLOROUT)
			{
				const unsigned outputRegisterIndex = dstParameter.GetRegisterIndex();
				shaderInfo.usedMRTMask |= (1 << outputRegisterIndex);
			}
		}
	}

	if (rawInstructionToken.coIssue)
		if (print) dprintf(shaderInfo, "+ CO_");

	switch (ShaderInfo::GetOpcodeDisplayType(opcode) )
	{
	case customOpcode:
		return ParseCustomOpcode<print>(opcode, shaderMemory, shaderInfo, uniqueTempRegistersUsed, uniqueInputRegistersUsed, outRegisterwriteTracker);
	case justOpcode:
		if (print) dprintf(shaderInfo, "%s\n", ShaderInfo::GetOpcodeString(opcode) );
		break;
	case srcOnly:
		if (print) dprintf(shaderInfo, "%s ", ShaderInfo::GetOpcodeString(opcode) );
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, "\n");
		break;
	case srcSrcOnly:
		if (print) dprintf(shaderInfo, "%s ", ShaderInfo::GetOpcodeString(opcode) );
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, "\n");
		break;
	case dstOnly:
		if (print) dprintf(shaderInfo, "%s ", ShaderInfo::GetOpcodeString(opcode) );
		ResolveDstParameter<print, true>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		if (print) dprintf(shaderInfo, "\n");
		break;
	case srcDst:
		if (print) dprintf(shaderInfo, "%s ", ShaderInfo::GetOpcodeString(opcode) );
		ResolveDstParameter<print, true>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, "\n");
		break;
	case srcSrcDst:
		if (print) dprintf(shaderInfo, "%s ", ShaderInfo::GetOpcodeString(opcode) );
		ResolveDstParameter<print, true>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, "\n");
		break;
	case srcSrcSrcDst:
		if (print) dprintf(shaderInfo, "%s ", ShaderInfo::GetOpcodeString(opcode) );
		ResolveDstParameter<print, true>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, "\n");
		break;
	case srcSrcSrcSrcDst: // Only ever used with D3DSIO_TEXLDD
		if (print) dprintf(shaderInfo, "%s ", ShaderInfo::GetOpcodeString(opcode) );
		ResolveDstParameter<print, true>(shaderMemory, shaderInfo, uniqueTempRegistersUsed, outRegisterwriteTracker);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, ", ");
		ResolveSrcParameter<print>(shaderInfo, shaderMemory, shaderInfo.isPixelShader, uniqueTempRegistersUsed, uniqueInputRegistersUsed);
		if (print) dprintf(shaderInfo, "\n");
		break;
	default:
		DbgBreakPrint("Error: Unknown shader parameters type");
		break;
	}
	return false;
}

template <const bool print>
static inline void ShaderAnalysisMain(const DWORD* shaderMemory, ShaderInfo& ret
#ifdef _DEBUG
	, const char* const filename
#endif
	)
{
	ret.initialBytecodeToken = shaderMemory;

	if (!shaderMemory)
	{
		ret.parsingErrorDetected = true;
		return;
	}

#ifdef DISASM_SHADER
	D3DXDisassembleShader(shaderMemory, FALSE, NULL, &ret.D3DXDisasm);
	if (ret.D3DXDisasm)
		ret.D3DXDisasmString = (const char* const)ret.D3DXDisasm->GetBufferPointer();
#endif // DISASM_SHADER

	outputRegisterWriteTracker outRegisterwriteTracker;

	const versionToken* const version = (const versionToken* const)shaderMemory;
	switch (version->pixelOrVertexShader)
	{
	case (verToken_vertexShader): // Vertex shader
		ret.isPixelShader = false;
		break;
	case (verToken_PixelShader): // Pixel shader
		ret.isPixelShader = true;
		break;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Shader is neither a vertex shader nor a pixel shader");
#endif
		ret.parsingErrorDetected = true;
		return;
	}

	ret.shaderMajorVersion = version->majorVersion;
	ret.shaderMinorVersion = version->minorVersion;

	if (ret.shaderMajorVersion < 1 || ret.shaderMajorVersion > 3)
	{
		ret.parsingErrorDetected = true;
#ifdef _DEBUG
		DbgBreakPrint("Error: Shader model is out of bounds for D3D9!");
#endif
		return;
	}

	if (ret.shaderMinorVersion > 4)
	{
		ret.parsingErrorDetected = true;
#ifdef _DEBUG
		DbgBreakPrint("Error: Shader model minor version greater than 4");
#endif
		return;
	}

	unsigned long uniqueTempRegistersUsed = 0x00000000;
	unsigned short uniqueInputRegistersUsed = 0x0000;

	// Move forwards one token to the first instruction/definition/declaration:
	++shaderMemory;

	int tabIndents;
	if (print)
		tabIndents = 0;

	bool doneParsing = false;
	while (!doneParsing)
	{
		doneParsing = ParseOpcode<print>(shaderMemory, ret, uniqueTempRegistersUsed, uniqueInputRegistersUsed, tabIndents, outRegisterwriteTracker);
	}

	outRegisterwriteTracker.AddOutputRegisterListingToShaderInfo(ret);

	for (unsigned x = 0; x < ret.shaderLengthDWORDs; ++x)
	{
		ret.shaderBytecodeHash = _rotl(ret.shaderBytecodeHash, 1);
		const DWORD newToken = ret.initialBytecodeToken[x];
		ret.shaderBytecodeHash ^= newToken;
	}

#ifdef DISASM_SHADER
	ret.shaderDisasmText.push_back('\0');
	ret.shaderDisasmBuffer = &ret.shaderDisasmText.front();
#endif

	ret.tempRegistersUsedBitmask = uniqueTempRegistersUsed;
	ret.numUniqueTempRegistersUsed = __popcnt(uniqueTempRegistersUsed);

	ret.inputRegistersUsedBitmask = uniqueInputRegistersUsed;
	ret.numUniqueInputRegistersUsed = (const unsigned short)(__popcnt(uniqueInputRegistersUsed) );

	if (ret.isPixelShader && ret.shaderMajorVersion == 1)
		ret.usedMRTMask |= 0x1; // ps_1_* shaders leave their color output in r0 instead of moving it into one of the oC[N] registers (which don't exist for version 1.x pixel shaders because MRT didn't exist back then)

	if (ret.numTexInstructions > 0 && !ret.isPixelShader)
		ret.usesVertTextureFetch = true;

	// It's an error for pixel shaders to not write out any color-channel data:
	if (ret.isPixelShader && ret.usedMRTMask == 0)
		ret.parsingErrorDetected = true;

	if (ret.usedMRTMask > 1)
		ret.usesMRT = true;

	return;
}

void DisasmAndAnalyzeShader(const DWORD* shaderMemory, ShaderInfo& shaderInfoOut
#ifdef _DEBUG
	, const char* const filename
#endif
	)
{
	ShaderAnalysisMain<true>(shaderMemory, shaderInfoOut
#ifdef _DEBUG
		, filename
#endif
		);

#ifdef DISASM_SHADER
	printf("%s\n", &shaderInfoOut.shaderDisasmText.front() );
#endif
}

void AnalyzeShader(const DWORD* shaderMemory, ShaderInfo& shaderInfoOut
#ifdef _DEBUG
	, const char* const filename
#endif
	)
{
	ShaderAnalysisMain<
#ifdef DISASM_SHADER
		true
#else
		false
#endif
	>(shaderMemory, shaderInfoOut
#ifdef _DEBUG
		, filename
#endif
		);
}

void outputRegisterWriteTracker::SetAddOrSkip(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned registerIndex, const BYTE writeMask, const dstParameterToken& dstParameter, const instructionToken* const _writeInstruction, const dstParameterToken* const optionalRelativeRegister/* = NULL*/)
{
	outputRegisterWriteState& thisRegWriteState = writtenRegisters[registerType][registerIndex];
	if (thisRegWriteState.writeMaskFinalState == 0x00)
	{
		outputRegisterIdentifier registerIdentifier;
		registerIdentifier.outputRegisterType = (const BYTE)registerType;
		registerIdentifier.outputRegisterNumber = registerIndex;
		registerIdentifiers.push_back(registerIdentifier);
	}

	outputRegisterWriteOperation newWriteOperation;
	newWriteOperation.writeInstruction = _writeInstruction;
	newWriteOperation.registerParameterToken = &dstParameter;
	newWriteOperation.optionalRelativeAddressingDestParameterToken = optionalRelativeRegister;
	newWriteOperation.instructionSourceParameterTokens = (const srcParameterToken* const)(&dstParameter + 1);
	newWriteOperation.operationWriteMask = writeMask;
	thisRegWriteState.writeOperations.push_back(newWriteOperation);

	thisRegWriteState.writeMaskFinalState |= writeMask;
}

void outputRegisterWriteTracker::AddOutputRegisterListingToShaderInfo(ShaderInfo& outShaderInfo)
{
	const unsigned numWrittenOutputRegisters = registerIdentifiers.size();
	for (unsigned x = 0; x < numWrittenOutputRegisters; ++x)
	{
		const outputRegisterIdentifier& registerIdentifier = registerIdentifiers[x];

		outputRegisterWriteState registerWriteState = writtenRegisters[registerIdentifier.outputRegisterType][registerIdentifier.outputRegisterNumber];
		WrittenOutputRegister newRegister;
		newRegister.registerType = (const D3DSHADER_PARAM_REGISTER_TYPE)(registerIdentifier.outputRegisterType);
		newRegister.registerIndex = registerIdentifier.outputRegisterNumber;
		newRegister.componentChannelsWritten = registerWriteState.writeMaskFinalState;
		newRegister.writeOperationsHistory.swap(registerWriteState.writeOperations); // Swap is really fast!
		outShaderInfo.writtenOutputRegisters.push_back(newRegister);
	}
}
