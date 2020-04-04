#pragma once

#include "ShaderJIT.h"
#include "IDirect3DDevice9Hook.h"
#include "ShaderJIT_HLSL_to_CPP_Tables.h"

#pragma warning(push)
#pragma warning(disable:4996) // We use sprintf a lot in this file and don't want to keep having to manually escape every call-site, so this whole file is 4996-disabled

static const unsigned NumSourceParams[customOpcode + 1] = 
{
	0, // justOpcode,
	1, // srcOnly,
	2, // srcSrcOnly,
	0, // dstOnly,
	1, // srcDst,
	2, // srcSrcDst,
	3, // srcSrcSrcDst,
	4, // srcSrcSrcSrcDst,
	0, // customOpcode
};

template <typename T>
static inline const bool IsConstantGlobal(const unsigned constIndex, const std::vector<T>& immediateRegisters)
{
	const unsigned numImmediateRegs = immediateRegisters.size();
	for (unsigned x = 0; x < numImmediateRegs; ++x)
	{
		const InitialConstantValueBase& constValue = immediateRegisters[x];
		if (constValue.constantRegisterIndex == constIndex)
			return false;
	}

	return true;
}

static inline const unsigned GetNumSourceParams(const opcodeDisplayType opcodeType)
{
	return NumSourceParams[opcodeType];
}

static inline const bool GetOpcodeAllowChannelSplitting(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_BREAKP)
		return opcodeAllowChannelSplitting[opcode];
	else
		return false;
}

static inline const bool GetOpcodeAllChannelsBroadcastSameVal(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_BREAKP)
		return opcodeAllChannelsBroadcastSameVal[opcode];
	else
		return false;
}

static inline const bool GetOpcodeImplementedForJIT(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode)
{
	if (opcode <= D3DSIO_BREAKP)
		return opcodeImplementedForJIT[opcode];
	else
		switch (opcode)
		{
		default:
#ifdef _DEBUG
		{
			__debugbreak(); // Invalid opcode detected
		}
#endif
			return false;
		case D3DSIO_PHASE:
		case D3DSIO_COMMENT:
		case D3DSIO_END:
			return true;
		}
}

static const inline bool NeedTempDestCopy(const dstParameterToken& dst, const srcParameterToken& src)
{
	if (dst.GetRegisterType() == src.GetRegisterType() )
	{
		return dst.GetRegisterIndex() == src.GetRegisterIndex();
	}

	return false;
}

static const inline bool NeedTempDestCopy(const dstParameterToken& dst, const srcParameterToken& src0, const srcParameterToken& src1)
{
	return NeedTempDestCopy(dst, src0) || NeedTempDestCopy(dst, src1);
}

static const inline bool NeedTempDestCopy(const dstParameterToken& dst, const srcParameterToken& src0, const srcParameterToken& src1, const srcParameterToken& src2)
{
	return NeedTempDestCopy(dst, src0, src1) || NeedTempDestCopy(dst, src2);
}

static const inline bool NeedTempDestCopy(const dstParameterToken& dst, const srcParameterToken& src0, const srcParameterToken& src1, const srcParameterToken& src2, const srcParameterToken& src3)
{
	return NeedTempDestCopy(dst, src0, src1, src2) || NeedTempDestCopy(dst, src3);
}

// This function *does not* advance the shader bytecode stream!
static inline void ResolveSrcParameterDisasm(const ShaderInfo& shaderInfo, const DWORD* bytecode, std::vector<char>& disasmline)
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
		AppendString(disasmline, "-");
		break;
	case D3DSPSM_ABS    :
		AppendString(disasmline, "abs(");
		break;
	case D3DSPSM_ABSNEG :
		AppendString(disasmline, "-abs(");
		break;
	case D3DSPSM_NOT    :
		AppendString(disasmline, "!");
		break;
	default:
		DbgBreakPrint("Error: Unknown source modifier");
		break;
	}

	unsigned index = srcParameter.GetRegisterIndex();

	const D3DSHADER_PARAM_REGISTER_TYPE registerType = srcParameter.GetRegisterType();
	switch (registerType)
	{
	case D3DSPR_TEMP       :
		AppendString(disasmline, "r");
		break;
	case D3DSPR_INPUT      :
		AppendString(disasmline, "v");
		break;
	case D3DSPR_CONST      :
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
		AppendString(disasmline, "c");
		break;
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		if (shaderInfo.isPixelShader)
		{
			if (shaderInfo.shaderMajorVersion == 3)
				AppendString(disasmline, "v"); // Texcoord
			else
				AppendString(disasmline, "t"); // Texcoord
		}
		else
		{
			AppendString(disasmline, "a"); // Address
		}
		break;
	case D3DSPR_RASTOUT    :
		switch (index)
		{
		default:
			DbgBreakPrint("Error: Unknown RASTOUT register index");
		case D3DSRO_POSITION:
			AppendString(disasmline, "oPos");
			break;
		case D3DSRO_FOG:
			AppendString(disasmline, "oFog");
			break;
		case D3DSRO_POINT_SIZE:
			AppendString(disasmline, "oPts");
			break;
		}
		break;
	case D3DSPR_ATTROUT    :
		AppendString(disasmline, "oD");
		break;
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		AppendString(disasmline, "oT");
		break;
	case D3DSPR_CONSTINT   :
		AppendString(disasmline, "i");
		break;
	case D3DSPR_COLOROUT   :
		AppendString(disasmline, "oC");
		break;
	case D3DSPR_DEPTHOUT   :
		AppendString(disasmline, "oDepth");
		break;
	case D3DSPR_SAMPLER    :
		AppendString(disasmline, "s");
		break;
	case D3DSPR_CONSTBOOL  :
		AppendString(disasmline, "b");
		break;
	case D3DSPR_LOOP       :
		AppendString(disasmline, "aL");
		break;
	case D3DSPR_TEMPFLOAT16:
		AppendString(disasmline, "r");
		break;
	case D3DSPR_MISCTYPE   :
		switch (index)
		{
		default:
			DbgBreakPrint("Unknown MISCTYPE register");
		case D3DSMO_POSITION:
			AppendString(disasmline, "VPOS");
			break;
		case D3DSMO_FACE:
			AppendString(disasmline, "VFACE");
			break;
		}
		break;
	case D3DSPR_LABEL      :
		AppendString(disasmline, "LABEL");
		break;
	case D3DSPR_PREDICATE  :
		AppendString(disasmline, "p");
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
	case D3DSPR_CONST      :
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
	char shaderIndexBuffer[16] = {0};
	sprintf(shaderIndexBuffer, "%u", index);
	AppendString(disasmline, shaderIndexBuffer);

	switch (sourceMod)
	{
	default:
	case D3DSPSM_NONE   :
	case D3DSPSM_NOT    :
	case D3DSPSM_NEG    :
		break;
	case D3DSPSM_BIAS   :
	case D3DSPSM_BIASNEG:
		AppendString(disasmline, "_bias");
		break;
	case D3DSPSM_SIGN   :
	case D3DSPSM_SIGNNEG:
		AppendString(disasmline, "_bx2");
		break;
	case D3DSPSM_COMP   :
		AppendString(disasmline, "_complement");
		break;
	case D3DSPSM_X2     :
	case D3DSPSM_X2NEG  :
		AppendString(disasmline, "_x2");
		break;
	case D3DSPSM_DZ     :
		AppendString(disasmline, "_dz");
		break;
	case D3DSPSM_DW     :
		AppendString(disasmline, "_dw");
		break;
	case D3DSPSM_ABS    :
	case D3DSPSM_ABSNEG :
		AppendString(disasmline, ")");
		break;
	}

	if (srcParameter.GetRelativeAddressingType() == D3DSHADER_ADDRMODE_RELATIVE)
	{
		DbgBreakPrint("Relative addressing not yet implemented!");

		// Super important to do this, otherwise the bytecode stream gets broken!
		++bytecode;
	}

	const unsigned channelSwizzle = srcParameter.GetChannelSwizzleXYZW();
	switch (channelSwizzle)
	{
	case D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT: // .xyzw
		break; // Don't print .xyzw all the time
	case D3DSP_REPLICATERED >> D3DVS_SWIZZLE_SHIFT: // .x
		AppendString(disasmline, ".x");
		break;
	case D3DSP_REPLICATEGREEN >> D3DVS_SWIZZLE_SHIFT: // .y
		AppendString(disasmline, ".y");
		break;
	case D3DSP_REPLICATEBLUE >> D3DVS_SWIZZLE_SHIFT: // .z
		AppendString(disasmline, ".z");
		break;
	case D3DSP_REPLICATEALPHA >> D3DVS_SWIZZLE_SHIFT: // .w
		AppendString(disasmline, ".w");
		break;
	default:
	{
		static const char* const channelName[4] =
		{
			"x",
			"y",
			"z",
			"w"
		};

		char sourceSwizzleBuffer[8] = {0};
		sprintf(sourceSwizzleBuffer, ".%s", channelName[channelSwizzle & 0x3]);
		AppendString(disasmline, sourceSwizzleBuffer);
		sprintf(sourceSwizzleBuffer, "%s", channelName[(channelSwizzle >> 2) & 0x3]);
		AppendString(disasmline, sourceSwizzleBuffer);
		sprintf(sourceSwizzleBuffer, "%s", channelName[(channelSwizzle >> 4) & 0x3]);
		AppendString(disasmline, sourceSwizzleBuffer);
		sprintf(sourceSwizzleBuffer, "%s", channelName[(channelSwizzle >> 6) & 0x3]);
		AppendString(disasmline, sourceSwizzleBuffer);
	}
		break;
	}

	// Super important to advance the bytecode stream
	++bytecode;
}

// This function *does* advance the global shader bytecode stream
static inline void ResolveSrcParameterSimpleSwizzleNoSourceMods(const ShaderInfo& shaderInfo, const DWORD*& bytecode, std::vector<char>& jitline)
{
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)bytecode;

	const D3DSHADER_PARAM_SRCMOD_TYPE sourceMod = srcParameter.GetSourceModifiersUnshifted();
	switch (sourceMod)
	{
	case D3DSPSM_NONE   :
		break;
	default:
		DbgBreakPrint("Error: Function cannot handle source modifiers");
		break;
	}

	unsigned index = srcParameter.GetRegisterIndex();

	const D3DSHADER_PARAM_REGISTER_TYPE registerType = srcParameter.GetRegisterType();
	switch (registerType)
	{
	case D3DSPR_TEMP       :
		AppendString(jitline, "r");
		break;
	case D3DSPR_INPUT      :
		AppendString(jitline, "v");
		break;
	case D3DSPR_CONST      :
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
		AppendString(jitline, "c");
		break;
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		if (shaderInfo.isPixelShader)
		{
			if (shaderInfo.shaderMajorVersion == 3)
				AppendString(jitline, "v"); // Texcoord
			else
				AppendString(jitline, "t"); // Texcoord
		}
		else
		{
			AppendString(jitline, "a"); // Address
		}
		break;
	case D3DSPR_RASTOUT    :
		switch (index)
		{
		default:
			DbgBreakPrint("Error: Unknown RASTOUT register index");
		case D3DSRO_POSITION:
			AppendString(jitline, "oPos");
			break;
		case D3DSRO_FOG:
			AppendString(jitline, "oFog");
			break;
		case D3DSRO_POINT_SIZE:
			AppendString(jitline, "oPts");
			break;
		}
		break;
	case D3DSPR_ATTROUT    :
		AppendString(jitline, "oD");
		break;
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		AppendString(jitline, "oT");
		break;
	case D3DSPR_CONSTINT   :
		AppendString(jitline, "i");
		break;
	case D3DSPR_COLOROUT   :
		AppendString(jitline, "oC");
		break;
	case D3DSPR_DEPTHOUT   :
		AppendString(jitline, "oDepth");
		break;
	case D3DSPR_SAMPLER    :
		AppendString(jitline, "s");
		break;
	case D3DSPR_CONSTBOOL  :
		AppendString(jitline, "b");
		break;
	case D3DSPR_LOOP       :
		AppendString(jitline, "aL");
		break;
	case D3DSPR_TEMPFLOAT16:
		AppendString(jitline, "r");
		break;
	case D3DSPR_MISCTYPE   :
		switch (index)
		{
		default:
			DbgBreakPrint("Unknown MISCTYPE register");
		case D3DSMO_POSITION:
			AppendString(jitline, "vpos");
			break;
		case D3DSMO_FACE:
			AppendString(jitline, "vface");
			break;
		}
		break;
	case D3DSPR_LABEL      :
		AppendString(jitline, "LABEL");
		break;
	case D3DSPR_PREDICATE  :
		AppendString(jitline, "p");
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
	case D3DSPR_CONST      :
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
	char shaderIndexBuffer[16] = {0};
	sprintf(shaderIndexBuffer, "%u", index);
	AppendString(jitline, shaderIndexBuffer);

	if (srcParameter.GetRelativeAddressingType() == D3DSHADER_ADDRMODE_RELATIVE)
	{
		DbgBreakPrint("Relative addressing not yet implemented!");

		// Super important to do this, otherwise the bytecode stream gets broken!
		++bytecode;
	}

	const unsigned channelSwizzle = srcParameter.GetChannelSwizzleXYZW();
	switch (channelSwizzle)
	{
	case D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT: // .xyzw
		break; // Don't print .xyzw all the time
	case D3DSP_REPLICATERED >> D3DVS_SWIZZLE_SHIFT: // .x
		AppendString(jitline, ".x");
		break;
	case D3DSP_REPLICATEGREEN >> D3DVS_SWIZZLE_SHIFT: // .y
		AppendString(jitline, ".y");
		break;
	case D3DSP_REPLICATEBLUE >> D3DVS_SWIZZLE_SHIFT: // .z
		AppendString(jitline, ".z");
		break;
	case D3DSP_REPLICATEALPHA >> D3DVS_SWIZZLE_SHIFT: // .w
		AppendString(jitline, ".w");
		break;
	default:
		DbgBreakPrint("Error: Non-trivial source register swizzle detected");
		break;
	}

	// Super important to advance the bytecode stream
	++bytecode;
}

template <const unsigned swizzleIndex>
static inline void PrintSourceRegNameAndSourceSwizzle(const srcParameterToken& sourceToken, char* const outBuffer, const ShaderInfo& shaderInfo, const srcParameterToken* const addressToken = NULL)
{
	unsigned index = sourceToken.GetRegisterIndex();

	const D3DSHADER_PARAM_REGISTER_TYPE registerType = sourceToken.GetRegisterType();
	const char* registerBaseString = NULL;
	switch (registerType)
	{
	case D3DSPR_TEMP       :
		registerBaseString = "r";
		break;
	case D3DSPR_INPUT      :
		registerBaseString = "v";
		break;
	case D3DSPR_CONST      :
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
		registerBaseString = "c";
		break;
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		if (shaderInfo.isPixelShader)
		{
			if (shaderInfo.shaderMajorVersion == 3)
				registerBaseString = "v"; // Texcoord
			else
				registerBaseString = "t"; // Texcoord
		}
		else
		{
			registerBaseString = "a"; // Address
		}
		break;
	case D3DSPR_RASTOUT    :
		switch (index)
		{
		default:
			DbgBreakPrint("Error: Unknown RASTOUT register index");
		case D3DSRO_POSITION:
			registerBaseString = "oPos";
			break;
		case D3DSRO_FOG:
			registerBaseString = "oFog";
			break;
		case D3DSRO_POINT_SIZE:
			registerBaseString = "oPts";
			break;
		}
		break;
	case D3DSPR_ATTROUT    :
		registerBaseString = "oD";
		break;
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		registerBaseString = "oT";
		break;
	case D3DSPR_CONSTINT   :
		registerBaseString = "i";
		break;
	case D3DSPR_COLOROUT   :
		registerBaseString = "oC";
		break;
	case D3DSPR_DEPTHOUT   :
		registerBaseString = "oDepth";
		break;
	case D3DSPR_SAMPLER    :
		registerBaseString = "s";
		break;
	case D3DSPR_CONSTBOOL  :
		registerBaseString = "b";
		break;
	case D3DSPR_LOOP       :
		registerBaseString = "aL";
		break;
	case D3DSPR_TEMPFLOAT16:
		registerBaseString = "r";
		break;
	case D3DSPR_MISCTYPE   :
		switch (index)
		{
		default:
			DbgBreakPrint("Unknown MISCTYPE register");
		case D3DSMO_POSITION:
			registerBaseString = "vpos";
			break;
		case D3DSMO_FACE:
			registerBaseString = "vface";
			break;
		}
		break;
	case D3DSPR_LABEL      :
		registerBaseString = "LABEL";
		break;
	case D3DSPR_PREDICATE  :
		registerBaseString = "p";
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
	case D3DSPR_CONST      :
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

	PrintSourceSwizzle<swizzleIndex>(sourceToken, outBuffer, registerBaseString, index);

	if (addressToken)
	{
		std::string outCopy = outBuffer;
		char addressRegisterAndSwizzle[32];
		switch (addressToken->GetChannelSwizzleXYZW() )
		{
		default:
		case D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT: // .xyzw
		case D3DSP_REPLICATERED >> D3DVS_SWIZZLE_SHIFT: // .x
			PrintSourceRegNameAndSourceSwizzle<0>(*addressToken, addressRegisterAndSwizzle, shaderInfo);
			break;
		case D3DSP_REPLICATEGREEN >> D3DVS_SWIZZLE_SHIFT: // .y
			PrintSourceRegNameAndSourceSwizzle<1>(*addressToken, addressRegisterAndSwizzle, shaderInfo);
			break;
		case D3DSP_REPLICATEBLUE >> D3DVS_SWIZZLE_SHIFT: // .z
			PrintSourceRegNameAndSourceSwizzle<2>(*addressToken, addressRegisterAndSwizzle, shaderInfo);
			break;
		case D3DSP_REPLICATEALPHA >> D3DVS_SWIZZLE_SHIFT: // .w
			PrintSourceRegNameAndSourceSwizzle<3>(*addressToken, addressRegisterAndSwizzle, shaderInfo);
			break;
		}
		sprintf(outBuffer, "vs.GetSrcRegisterFromAddress(%s, %s)", outCopy.c_str(), addressRegisterAndSwizzle);
	}
}

static inline void PrintRegisterName(const parameterToken& parameterToken, char* const outBuffer, const ShaderInfo& shaderInfo, const srcParameterToken* const addressToken = NULL)
{
	unsigned index = parameterToken.GetRegisterIndex();

	const D3DSHADER_PARAM_REGISTER_TYPE registerType = parameterToken.GetRegisterType();
	const char* registerBaseString = NULL;
	switch (registerType)
	{
	case D3DSPR_TEMP       :
		registerBaseString = "r";
		break;
	case D3DSPR_INPUT      :
		registerBaseString = "v";
		break;
	case D3DSPR_CONST      :
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
		registerBaseString = "c";
		break;
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		if (shaderInfo.isPixelShader)
		{
			if (shaderInfo.shaderMajorVersion == 3)
				registerBaseString = "v"; // Texcoord
			else
				registerBaseString = "t"; // Texcoord
		}
		else
		{
			registerBaseString = "a"; // Address
		}
		break;
	case D3DSPR_RASTOUT    :
		switch (index)
		{
		default:
			DbgBreakPrint("Error: Unknown RASTOUT register index");
		case D3DSRO_POSITION:
			registerBaseString = "oPos";
			break;
		case D3DSRO_FOG:
			registerBaseString = "oFog";
			break;
		case D3DSRO_POINT_SIZE:
			registerBaseString = "oPts";
			break;
		}
		break;
	case D3DSPR_ATTROUT    :
		registerBaseString = "oD";
		break;
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		registerBaseString = "oT";
		break;
	case D3DSPR_CONSTINT   :
		registerBaseString = "i";
		break;
	case D3DSPR_COLOROUT   :
		registerBaseString = "oC";
		break;
	case D3DSPR_DEPTHOUT   :
		registerBaseString = "oDepth";
		break;
	case D3DSPR_SAMPLER    :
		registerBaseString = "s";
		break;
	case D3DSPR_CONSTBOOL  :
		registerBaseString = "b";
		break;
	case D3DSPR_LOOP       :
		registerBaseString = "aL";
		break;
	case D3DSPR_TEMPFLOAT16:
		registerBaseString = "r";
		break;
	case D3DSPR_MISCTYPE   :
		switch (index)
		{
		default:
			DbgBreakPrint("Unknown MISCTYPE register");
		case D3DSMO_POSITION:
			registerBaseString = "vpos";
			break;
		case D3DSMO_FACE:
			registerBaseString = "vface";
			break;
		}
		break;
	case D3DSPR_LABEL      :
		registerBaseString = "LABEL";
		break;
	case D3DSPR_PREDICATE  :
		registerBaseString = "p";
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
	case D3DSPR_CONST      :
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
	sprintf(outBuffer, "%s%u", registerBaseString, index);

	if (addressToken)
	{
		std::string outCopy = outBuffer;
		char addressRegisterAndSwizzle[32];
		switch (addressToken->GetChannelSwizzleXYZW() )
		{
		default:
		case D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT: // .xyzw
		case D3DSP_REPLICATERED >> D3DVS_SWIZZLE_SHIFT: // .x
			PrintSourceRegNameAndSourceSwizzle<0>(*addressToken, addressRegisterAndSwizzle, shaderInfo);
			break;
		case D3DSP_REPLICATEGREEN >> D3DVS_SWIZZLE_SHIFT: // .y
			PrintSourceRegNameAndSourceSwizzle<1>(*addressToken, addressRegisterAndSwizzle, shaderInfo);
			break;
		case D3DSP_REPLICATEBLUE >> D3DVS_SWIZZLE_SHIFT: // .z
			PrintSourceRegNameAndSourceSwizzle<2>(*addressToken, addressRegisterAndSwizzle, shaderInfo);
			break;
		case D3DSP_REPLICATEALPHA >> D3DVS_SWIZZLE_SHIFT: // .w
			PrintSourceRegNameAndSourceSwizzle<3>(*addressToken, addressRegisterAndSwizzle, shaderInfo);
			break;
		}
		sprintf(outBuffer, "vs.GetSrcRegisterFromAddress(%s, %s)", outCopy.c_str(), addressRegisterAndSwizzle);
	}
}

static inline void AdvanceSrcParameter(const ShaderInfo& shaderInfo, const DWORD*& shaderMemory, const srcParameterToken*& outSrcParam, const srcParameterToken*& outAddressParam)
{
	outSrcParam = (const srcParameterToken* const)shaderMemory;
	++shaderMemory;

	// Shader relative addressing: https://msdn.microsoft.com/en-us/library/windows/hardware/ff569708(v=vs.85).aspx
	switch(outSrcParam->GetRelativeAddressingType() )
	{
	default:
	case D3DSHADER_ADDRMODE_ABSOLUTE:
		outAddressParam = NULL;
		return;
	case D3DSHADER_ADDRMODE_RELATIVE:
		if (shaderInfo.shaderMajorVersion == 1 && !shaderInfo.isPixelShader) // vs_1_1 has implicit relative addressing of a0.x (the only relative addressable register and component)
		{
			// Bit 31 is 0x1 and this is of type "Address Register" and the swizzle is an X-broadcast (*.xxxx)
			static const DWORD IMPLICIT_SRC_PARAMETER_TOKEN_A0_X = (0x80000000) | (D3DSPR_ADDR << 28);

			outAddressParam = (const srcParameterToken* const)&IMPLICIT_SRC_PARAMETER_TOKEN_A0_X;
		}
		else // Explicit relative address specified in the next DWORD token:
		{
			outAddressParam = (const srcParameterToken* const)shaderMemory;
			++shaderMemory;
		}
		break;
	}
}


static inline const bool ParseCustomOpcode(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode, const DWORD*& shaderMemory, const ShaderInfo& shaderInfo, std::vector<char>& shaderbody)
{
	char disasm[1024] = {0};
	sprintf(disasm, "\t// %s ", ShaderInfo::GetOpcodeString(opcode) );
	AppendString(shaderbody, disasm);

	switch (opcode)
	{
	default:
		__debugbreak(); // Unhandled custom opcode detected!
		break;
	case D3DSIO_LABEL:
	{
		const srcParameterToken& srcParameter = *(const srcParameterToken* const)shaderMemory++;
		sprintf(disasm, "LABEL%u", srcParameter.GetRegisterIndex() );
	}
		break;
	case D3DSIO_DCL:
	{
		const DWORD dwordToken = *shaderMemory++;
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;
		UNREFERENCED_PARAMETER(destParameter);
	}
		break;
	case D3DSIO_SINCOS:
	{
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;
		const srcParameterToken* srcParameters[3] = {NULL};
		const srcParameterToken* srcAddrParameters[3] = {NULL};

		unsigned numSrcTokens;
		if (shaderInfo.shaderMajorVersion < 3) // Shader model 2 has these extra registers, but all the shader model 3+ don't have them
			numSrcTokens = 3;
		else
			numSrcTokens = 1;

		for (unsigned x = 0; x < numSrcTokens; ++x)
			AdvanceSrcParameter(shaderInfo, shaderMemory, srcParameters[x], srcAddrParameters[x]);

		UNREFERENCED_PARAMETER(destParameter);

		DbgBreakPrint("Error: SINCOS is not yet implemented in the JIT system!");
	}
		break;
	case D3DSIO_DEFB:
	{
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;
		const BOOL dword0 = *shaderMemory++;

		sprintf(disasm, "b%u = %s", destParameter.GetRegisterIndex(), dword0 ? "TRUE" : "FALSE");
		AppendString(shaderbody, disasm);
	}
		break;
	case D3DSIO_DEFI:
	{
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;
		const DWORD dword0 = *shaderMemory++;
		int4 iVar;
		iVar.x = *(const int* const)&dword0;
		const DWORD dword1 = *shaderMemory++;
		iVar.y = *(const int* const)&dword1;
		const DWORD dword2 = *shaderMemory++;
		iVar.z = *(const int* const)&dword2;
		const DWORD dword3 = *shaderMemory++;
		iVar.w = *(const int* const)&dword3;

		sprintf(disasm, "i%u = int4(%i, %i, %i, %i)", destParameter.GetRegisterIndex(), iVar.x, iVar.y, iVar.z, iVar.w);
		AppendString(shaderbody, disasm);
	}
		break;
	case D3DSIO_DEF:
	{
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;
		const DWORD dword0 = *shaderMemory++;
		float4 fVar;
		fVar.x = *(const float* const)&dword0;
		const DWORD dword1 = *shaderMemory++;
		fVar.y = *(const float* const)&dword1;
		const DWORD dword2 = *shaderMemory++;
		fVar.z = *(const float* const)&dword2;
		const DWORD dword3 = *shaderMemory++;
		fVar.w = *(const float* const)&dword3;

		sprintf(disasm, "c%u = float4(%f, %f, %f, %f)", destParameter.GetRegisterIndex(), fVar.x, fVar.y, fVar.z, fVar.w);
		AppendString(shaderbody, disasm);
	}
		break;
	case D3DSIO_TEX:
	{
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;
		srcParameterToken src0; // This is the texcoord source parameter
		srcParameterToken src1; // This is the sampler source parameter
		if (shaderInfo.shaderMajorVersion < 2)
		{
			if (shaderInfo.shaderMinorVersion < 4)
			{
				// This is the ps_1_0 thru ps_1_3 case: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/tex---ps
				// Just T dst

				// src0 is the texcoord source parameter:
				src0.internalRawToken = 0x00000000;
				src0.srcParameter.instructionParameterBit = parameterTokenMarker;
				src0.srcParameter.registerIndex = destParameter.GetRegisterIndex();
				src0.srcParameter.registerType_lowBits = destParameter.GetRegisterType();
				src0.srcParameter.registerType_highBits = (destParameter.GetRegisterType() >> 3);
				src0.srcParameter.sourceSwizzle.fullSwizzle = _NoSwizzleXYZW;
			}
			else
			{
				// This is the ps_1_4 case: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/texld---ps-1-4
				// tex R dst, T/R src0
				src0 = *(const srcParameterToken* const)shaderMemory++;
			}

			// src1 is the sampler source parameter:
			src1.internalRawToken = 0x00000000;
			src1.srcParameter.instructionParameterBit = parameterTokenMarker;
			src1.srcParameter.registerIndex = destParameter.GetRegisterIndex();
			src1.srcParameter.registerType_lowBits = (D3DSPR_SAMPLER & 0x7);
			src1.srcParameter.registerType_highBits = ( (D3DSPR_SAMPLER >> 3) & 0x3);
			src1.srcParameter.sourceSwizzle.fullSwizzle = _NoSwizzleXYZW;
		}
		else
		{
			// This is the shader model 2+ case: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/texld---ps-2-0
			// tex R dst, T/R/V src0, S src1
			src0 = *(const srcParameterToken* const)shaderMemory++;
			src1 = *(const srcParameterToken* const)shaderMemory++;
		}

		// TODO: Support tex2Dbias (D3DSIO_TEX), tex2Dproj (D3DSIO_TEX), tex2Dgrad (D3DSIO_TEXLDD) and tex2Dod (D3DSIO_TEXLDL) too
		char destRegisterName[8] = {0};
		char src0RegisterName[8] = {0};
		char src1RegisterName[8] = {0};
		PrintRegisterName(destParameter, destRegisterName, shaderInfo);
		PrintRegisterName(src0, src0RegisterName, shaderInfo);
		PrintRegisterName(src1, src1RegisterName, shaderInfo);

		char texStringSource[64] = {0};
		sprintf(texStringSource, "%s, %s, %s\n", destRegisterName, src0RegisterName, src1RegisterName);
		AppendString(shaderbody, texStringSource);

		const unsigned writeMask = destParameter.GetWriteMask();
		sprintf(texStringSource, "\t(*%cs.tex2DMip0ptrs[%u])", shaderInfo.isPixelShader ? 'p' : 'v', writeMask - 1);
		AppendString(shaderbody, texStringSource);

		// Technically we don't have to worry about dest register aliasing with source registers for this instruction, because the
		// texld instruction internally makes a modifiable copy of the texcoord for coord wrapping/clamping/etc. behavior
		sprintf(texStringSource, "(%s, %s, %s);\n", destRegisterName, src0RegisterName, src1RegisterName);
		AppendString(shaderbody, texStringSource);
	}
		break;
	case D3DSIO_PHASE:
		break;
	case D3DSIO_COMMENT:
	{
		const DWORD commentToken = shaderMemory[-1];
		const unsigned numTokensToSkip = (commentToken & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
		shaderMemory += numTokensToSkip;
	}
		break;
	case D3DSIO_END:
		shaderbody.push_back('\n');
		return true;
	}

	shaderbody.push_back('\n');

	return false;
}

static inline void PrintSourceModifier(const srcParameterToken& sourceParameterToken, char* const outBuffer, const char* const sourceRegName)
{
	switch (sourceParameterToken.GetSourceModifiersUnshifted() )
	{
	default:
		DbgBreakPrint("Error: Unknown source register modifier!");
	case D3DSPSM_NONE   :
		sprintf(outBuffer, "%s", sourceRegName);
		break;
	case D3DSPSM_NEG    :
		sprintf(outBuffer, "-%s", sourceRegName);
		break;
	case D3DSPSM_BIAS   :
		sprintf(outBuffer, "-0.5f + %s", sourceRegName);
		break;
	case D3DSPSM_BIASNEG:
		sprintf(outBuffer, "(-0.5f + %s) * -1.0f", sourceRegName);
		break;
	case D3DSPSM_SIGN   :
		sprintf(outBuffer, "(-0.5f + %s) * 2.0f", sourceRegName);
		break;
	case D3DSPSM_SIGNNEG:
		sprintf(outBuffer, "(-0.5f + %s) * -2.0f", sourceRegName);
		break;
	case D3DSPSM_COMP   :
		sprintf(outBuffer, "1.0f - %s", sourceRegName);
		break;
	case D3DSPSM_X2     :
		sprintf(outBuffer, "2.0f * %s", sourceRegName);
		break;
	case D3DSPSM_X2NEG  :
		sprintf(outBuffer, "-2.0f * %s", sourceRegName);
		break;
	case D3DSPSM_DZ     :
		DbgBreakPrint("Error: Source modifier not yet supported");
		break;
	case D3DSPSM_DW     :
		DbgBreakPrint("Error: Source modifier not yet supported");
		break;
	case D3DSPSM_ABS    :
		sprintf(outBuffer, "fabsf(%s)", sourceRegName);
		break;
	case D3DSPSM_ABSNEG :
		sprintf(outBuffer, "-1.0f * fabsf(%s)", sourceRegName);
		break;
	case D3DSPSM_NOT    :
		sprintf(outBuffer, "!%s", sourceRegName);
		break;
	}
}

// These strings are for disassembly only (not for shader-code)
static const char* writeMaskDisasmStrings[16] = 
{
	".0", // 0
	".x", // 1
	".y", // 2
	".xy", // 3
	".z", // 4
	".xz", // 5
	".yz", // 6
	".xyz", // 7
	".w", // 8
	".xw", // 9
	".yw", // 10
	".xyw", // 11
	".zw", // 12
	".xzw", // 13
	".yzw", // 14
	"", // 15
};

template <const unsigned swizzleIndex>
static inline void PrintSourceSwizzle(const srcParameterToken& sourceToken, char* const outBuffer, const char* const baseString, const unsigned regIndex)
{
	static const char* const swizzleChannels[4] = 
	{
		".x",
		".y",
		".z",
		".w"
	};

	const unsigned swizzleX = sourceToken.GetChannelSwizzleXYZW() & 0x3;
	const unsigned swizzleY = (sourceToken.GetChannelSwizzleXYZW() >> 2) & 0x3;
	const unsigned swizzleZ = (sourceToken.GetChannelSwizzleXYZW() >> 4) & 0x3;
	const unsigned swizzleW = (sourceToken.GetChannelSwizzleXYZW() >> 6) & 0x3;

	switch (swizzleIndex)
	{
	default:
		DbgBreakPrint("Should never be here!");
	case 0: // x
		sprintf(outBuffer, "%s%u%s", baseString, regIndex, swizzleChannels[swizzleX]);
		break;
	case 1: // y
		sprintf(outBuffer, "%s%u%s", baseString, regIndex, swizzleChannels[swizzleY]);
		break;
	case 2: // z
		sprintf(outBuffer, "%s%u%s", baseString, regIndex, swizzleChannels[swizzleZ]);
		break;
	case 3: // w
		sprintf(outBuffer, "%s%u%s", baseString, regIndex, swizzleChannels[swizzleW]);
		break;
	}
}

template <const unsigned swizzleIndex>
static inline void PrintSourceSwizzle(const srcParameterToken& sourceToken, char* const outBuffer, const char* const baseString)
{
	static const char* const swizzleChannels[4] = 
	{
		".x",
		".y",
		".z",
		".w"
	};

	const unsigned swizzleX = sourceToken.GetChannelSwizzleXYZW() & 0x3;
	const unsigned swizzleY = (sourceToken.GetChannelSwizzleXYZW() >> 2) & 0x3;
	const unsigned swizzleZ = (sourceToken.GetChannelSwizzleXYZW() >> 4) & 0x3;
	const unsigned swizzleW = (sourceToken.GetChannelSwizzleXYZW() >> 6) & 0x3;

	switch (swizzleIndex)
	{
	default:
		DbgBreakPrint("Should never be here!");
	case 0: // x
		sprintf(outBuffer, "%s%s", baseString, swizzleChannels[swizzleX]);
		break;
	case 1: // y
		sprintf(outBuffer, "%s%s", baseString, swizzleChannels[swizzleY]);
		break;
	case 2: // z
		sprintf(outBuffer, "%s%s", baseString, swizzleChannels[swizzleZ]);
		break;
	case 3: // w
		sprintf(outBuffer, "%s%s", baseString, swizzleChannels[swizzleW]);
		break;
	}
}

template <const unsigned swizzleIndex>
static inline void PrintSourceRegNameAndSourceSwizzleOrTempReg(const srcParameterToken& sourceToken, const dstParameterToken& destToken, char* const outBuffer, const ShaderInfo& shaderInfo, const srcParameterToken* const addressToken = NULL)
{
	if (NeedTempDestCopy(destToken, sourceToken) )
		PrintSourceSwizzle<swizzleIndex>(sourceToken, outBuffer, "tempDst");
	else
		PrintSourceRegNameAndSourceSwizzle<swizzleIndex>(sourceToken, outBuffer, shaderInfo, addressToken);
}
// returns true if done parsing, or false to continue parsing the shader
static inline const bool ParseOpcode(const DWORD*& shaderMemory, const ShaderInfo& shaderInfo, std::vector<char>& shaderbody, bool& usedTempDestRegister)
{
	// Increment the token pointer to the first parameter or to the next instruction (if the opcode has no parameters)
	const instructionToken rawOpcodeToken = *(const instructionToken* const)shaderMemory++;
	const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode = (const D3DSHADER_INSTRUCTION_OPCODE_TYPE)(rawOpcodeToken.opcode);

	char disasm[1024] = {0};
	char buffer[1024] = {0};

	if (!GetOpcodeImplementedForJIT(opcode) )
	{
		char notImplementedMessage[128] = {0};
		sprintf(notImplementedMessage, "\t//--- Warning: Opcode \"%s\" not yet implemented for JIT! ---\n", ShaderInfo::GetOpcodeString(opcode) );
		AppendString(shaderbody, notImplementedMessage);
	}

	if (!GetOpcodeAllowChannelSplitting(opcode) )
	{
		const opcodeDisplayType opcodeType = ShaderInfo::GetOpcodeDisplayType(opcode);
		if (opcodeType == customOpcode)
			return ParseCustomOpcode(opcode, shaderMemory, shaderInfo, shaderbody);

		sprintf(disasm, "\t// %s\n", ShaderInfo::GetOpcodeString(opcode) );
		AppendString(shaderbody, disasm);
		AppendString(shaderbody, "\t{\n");

		char destRegisterName[16] = {0};
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;
		const unsigned writeMask = destParameter.GetWriteMask();
		PrintRegisterName(destParameter, destRegisterName, shaderInfo);

		const srcParameterToken* srcParameters[4] = {NULL};
		const srcParameterToken* srcAddrParameters[4] = {NULL};
		const unsigned numSrcParameters = GetNumSourceParams(opcodeType);
		for (unsigned x = 0; x < numSrcParameters; ++x)
			AdvanceSrcParameter(shaderInfo, shaderMemory, srcParameters[x], srcAddrParameters[x]);

		const bool hasWriteMask = writeMask != 0xF;
		if (hasWriteMask)
		{
			AppendString(shaderbody, "\t\tfloat4 unmaskedRet;\n");
		}

		for (unsigned x = 0; x < numSrcParameters; ++x)
		{
			if (srcParameters[x]->GetChannelSwizzleXYZW() == (D3DSP_NOSWIZZLE >> D3DSP_SWIZZLE_SHIFT) )
				continue;

			sprintf(buffer, "\t\tfloat4 swizzledSource%u;\n", x);
			AppendString(shaderbody, buffer);

			char srcRegisterName[16] = {0};
			PrintRegisterName(*srcParameters[x], srcRegisterName, shaderInfo);

			PrintSourceSwizzle<0>(*srcParameters[x], disasm, srcRegisterName);
			sprintf(buffer, "\t\tswizzledSource%u.x = %s;\n", x, disasm);
			AppendString(shaderbody, buffer);

			PrintSourceSwizzle<1>(*srcParameters[x], disasm, srcRegisterName);
			sprintf(buffer, "\t\tswizzledSource%u.y = %s;\n", x, disasm);
			AppendString(shaderbody, buffer);

			PrintSourceSwizzle<2>(*srcParameters[x], disasm, srcRegisterName);
			sprintf(buffer, "\t\tswizzledSource%u.z = %s;\n", x, disasm);
			AppendString(shaderbody, buffer);

			PrintSourceSwizzle<3>(*srcParameters[x], disasm, srcRegisterName);
			sprintf(buffer, "\t\tswizzledSource%u.w = %s;\n", x, disasm);
			AppendString(shaderbody, buffer);
		}

		const char* functionStringSource = NULL;

		// TODO: Support tex2Dbias (D3DSIO_TEX), tex2Dproj (D3DSIO_TEX), tex2Dgrad (D3DSIO_TEXLDD) and tex2Dod (D3DSIO_TEXLDL) too
		if (opcode == D3DSIO_TEX)
		{
			static char texStringSource[64] = {0};
			if (shaderInfo.isPixelShader)
				sprintf(texStringSource, "(*ps.tex2DMip0ptrs[%u])", writeMask - 1);
			else
				sprintf(texStringSource, "(*vs.tex2DMip0ptrs[%u])", writeMask - 1);
			functionStringSource = texStringSource;
		}
		else
		{
			functionStringSource = ShaderInfo::GetOpcodeFunctionString(opcode);
		}

		sprintf(buffer, "\t\t%s(%s, ", functionStringSource, hasWriteMask ? "unmaskedRet" : destRegisterName);
		AppendString(shaderbody, buffer);

		for (unsigned x = 0; x < numSrcParameters; ++x)
		{
			if (x > 0)
				AppendString(shaderbody, ", ");

			if (srcParameters[x]->GetChannelSwizzleXYZW() == (D3DSP_NOSWIZZLE >> D3DSP_SWIZZLE_SHIFT) )
			{
				PrintRegisterName(*srcParameters[x], buffer, shaderInfo, srcAddrParameters[x]);
				AppendString(shaderbody, buffer);
			}
			else
			{
				sprintf(buffer, "swizzledSource%u", x);
				AppendString(shaderbody, buffer);
			}
		}

		AppendString(shaderbody, ");\n");

		if (hasWriteMask)
		{
			if (writeMask & 0x1)
			{
				sprintf(buffer, "\t\t%s.x = unmaskedRet.x;\n", destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x2)
			{
				sprintf(buffer, "\t\t%s.y = unmaskedRet.y;\n", destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x4)
			{
				sprintf(buffer, "\t\t%s.z = unmaskedRet.z;\n", destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x8)
			{
				sprintf(buffer, "\t\t%s.w = unmaskedRet.w;\n", destRegisterName);
				AppendString(shaderbody, buffer);
			}
		}

		if (destParameter.GetResultModifierUnshifted() & D3DSPDM_SATURATE)
		{
			if (writeMask & 0x1)
			{
				sprintf(buffer, "\t%s.x = saturate(%s.x);\n", destRegisterName, destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x2)
			{
				sprintf(buffer, "\t%s.y = saturate(%s.y);\n", destRegisterName, destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x4)
			{
				sprintf(buffer, "\t%s.z = saturate(%s.z);\n", destRegisterName, destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x8)
			{
				sprintf(buffer, "\t%s.w = saturate(%s.w);\n", destRegisterName, destRegisterName);
				AppendString(shaderbody, buffer);
			}
		}

		AppendString(shaderbody, "\t}\n\n");

		return false;
	}

	const opcodeDisplayType opcodeType = ShaderInfo::GetOpcodeDisplayType(opcode);

	unsigned numSourceRegisters;
	switch (opcodeType)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case justOpcode:
	case customOpcode:
	case dstOnly:
		numSourceRegisters = 0;
		break;
	case srcOnly:
	case srcDst:
		numSourceRegisters = 1;
		break;
	case srcSrcOnly:
	case srcSrcDst:
		numSourceRegisters = 2;
		break;
	case srcSrcSrcDst:
		numSourceRegisters = 3;
		break;
	case srcSrcSrcSrcDst:
		numSourceRegisters = 4;
		break;
	}

	switch (opcodeType)
	{
	case customOpcode:
		return ParseCustomOpcode(opcode, shaderMemory, shaderInfo, shaderbody);
	case justOpcode:
		sprintf(disasm, "\t// %s\n", ShaderInfo::GetOpcodeString(opcode) );
		AppendString(shaderbody, disasm);
		sprintf(buffer, "\t%s(%cs);\n\n", ShaderInfo::GetOpcodeFunctionString(opcode), shaderInfo.isPixelShader ? 'p' : 'v');
		AppendString(shaderbody, buffer);
		break;
	case srcOnly:
	{
		std::vector<char> sourceDisasmLine;
		ResolveSrcParameterDisasm(shaderInfo, shaderMemory, sourceDisasmLine);
		sprintf(disasm, "\t// %s %s\n", ShaderInfo::GetOpcodeString(opcode), &(sourceDisasmLine.front() ) );
		AppendString(shaderbody, disasm);
		std::vector<char> jitLine;
		ResolveSrcParameterSimpleSwizzleNoSourceMods(shaderInfo, shaderMemory, jitLine);
		sprintf(buffer, "\t%s(%cs, %s);\n\n", ShaderInfo::GetOpcodeFunctionString(opcode), shaderInfo.isPixelShader ? 'p' : 'v', &(jitLine.front() ) );
		AppendString(shaderbody, buffer);
	}
		break;
	case srcSrcOnly:
	{
		std::vector<char> sourceDisasmLine;
		std::vector<char> jitLine;
		ResolveSrcParameterDisasm(shaderInfo, shaderMemory, sourceDisasmLine);
		sourceDisasmLine.push_back(' ');
		ResolveSrcParameterSimpleSwizzleNoSourceMods(shaderInfo, shaderMemory, jitLine);
		ResolveSrcParameterDisasm(shaderInfo, shaderMemory, sourceDisasmLine);
		ResolveSrcParameterSimpleSwizzleNoSourceMods(shaderInfo, shaderMemory, jitLine);
		sprintf(disasm, "\t// %s %s\n", ShaderInfo::GetOpcodeString(opcode), &(sourceDisasmLine.front() ) );
		AppendString(shaderbody, disasm);
		sprintf(buffer, "\t%s(%cs, %s);\n\n", ShaderInfo::GetOpcodeFunctionString(opcode), shaderInfo.isPixelShader ? 'p' : 'v', &(jitLine.front() ) );
		AppendString(shaderbody, buffer);
	}
		break;
	case dstOnly:
	{
		std::vector<char> jitLine;
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;

		if (opcode == D3DSIO_TEXKILL)
		{
			char registerName[16] = {0};
			const unsigned registerIndex = destParameter.GetRegisterIndex();
			switch (destParameter.GetRegisterType() )
			{
			default:
				DbgBreakPrint("Error: Unexpected register type encountered for TEXKILL instruction");
			case D3DSPR_TEMP: // Temporary (GPR) register
				sprintf(registerName, "r%u", registerIndex);
				break;
			case D3DSPR_TEXTURE: // Texcoord register
				if (shaderInfo.shaderMajorVersion == 3)
					sprintf(registerName, "v%u", registerIndex);
				else
					sprintf(registerName, "t%u", registerIndex);
				break;
			}

			const unsigned writeMask = destParameter.GetWriteMask();

			sprintf(disasm, "\t// %s %s%s\n", ShaderInfo::GetOpcodeString(opcode), registerName, writeMaskDisasmStrings[writeMask]);
			AppendString(shaderbody, disasm);

			if (writeMask & 0x1) // x
			{
				sprintf(buffer, "\tif (%s.x < 0.0f)\n"
					"\t{\n"
					"\t\tps.outputRegisters[0].pixelStatus = discard;\n"
					"\t\treturn;\n"
					"\t}\n", registerName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x2) // y
			{
				sprintf(buffer, "\tif (%s.y < 0.0f)\n"
					"\t{\n"
					"\t\tps.outputRegisters[0].pixelStatus = discard;\n"
					"\t\treturn;\n"
					"\t}\n", registerName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x4) // z
			{
				sprintf(buffer, "\tif (%s.z < 0.0f)\n"
					"\t{\n"
					"\t\tps.outputRegisters[0].pixelStatus = discard;\n"
					"\t\treturn;\n"
					"\t}\n", registerName);
				AppendString(shaderbody, buffer);
			}

			AppendString(shaderbody, "\n");
		}
	}
		break;
	case srcDst:
	case srcSrcDst:
	case srcSrcSrcDst:
	case srcSrcSrcSrcDst: // Only ever used with D3DSIO_TEXLDD
	{
		char destRegisterName[16] = {0};
		char srcModifierStrings[4][32] = {0};
		char srcRegisterNames[4][32] = {0};
		/*char src0ModifierString[32] = {0};
		char src0RegisterName[16] = {0};
		char src1ModifierString[32] = {0};
		char src1RegisterName[16] = {0};
		char src2ModifierString[32] = {0};
		char src2RegisterName[16] = {0};
		char src3ModifierString[32] = {0};
		char src3RegisterName[16] = {0};*/
		const dstParameterToken& destParameter = *(const dstParameterToken* const)shaderMemory++;
		const unsigned writeMask = destParameter.GetWriteMask();
		PrintRegisterName(destParameter, destRegisterName, shaderInfo);
		const srcParameterToken* srcParameters[4] = {NULL};
		const srcParameterToken* srcAddrParameters[4] = {NULL};
		for (unsigned x = 0; x < numSourceRegisters; ++x)
			AdvanceSrcParameter(shaderInfo, shaderMemory, srcParameters[x], srcAddrParameters[x]);
		for (unsigned x = 0; x < numSourceRegisters; ++x)
			PrintRegisterName(*srcParameters[x], srcRegisterNames[x], shaderInfo);
		/*PrintRegisterName(*srcParameters[0], src0RegisterName, shaderInfo);
		PrintRegisterName(*srcParameters[1], src1RegisterName, shaderInfo);
		PrintRegisterName(*srcParameters[2], src2RegisterName, shaderInfo);
		PrintRegisterName(*srcParameters[3], src3RegisterName, shaderInfo);*/
		static const char* const disasmStrings[4] =
		{
			"\t// %s %s%s, %s\n",
			"\t// %s %s%s, %s, %s\n",
			"\t// %s %s%s, %s, %s, %s\n",
			"\t// %s %s%s, %s, %s, %s, %s\n",
		};
		//sprintf(disasm, "\t// %s %s%s, %s, %s, %s, %s\n", ShaderInfo::GetOpcodeString(opcode), destRegisterName, writeMaskDisasmStrings[writeMask], src0RegisterName, src1RegisterName, src2RegisterName, src3RegisterName);
		sprintf(disasm, disasmStrings[numSourceRegisters - 1], ShaderInfo::GetOpcodeString(opcode), destRegisterName, writeMaskDisasmStrings[writeMask], srcRegisterNames[0], srcRegisterNames[1], srcRegisterNames[2], srcRegisterNames[3]);
		AppendString(shaderbody, disasm);

		bool needsTempDestCopy = false;
		switch (numSourceRegisters)
		{
		case 1:
			if (NeedTempDestCopy(destParameter, *srcParameters[0]) )
				needsTempDestCopy = true;
			break;
		case 2:
			if (NeedTempDestCopy(destParameter, *srcParameters[0], *srcParameters[1]) )
				needsTempDestCopy = true;
			break;
		case 3:
			if (NeedTempDestCopy(destParameter, *srcParameters[0], *srcParameters[1], *srcParameters[2]) )
				needsTempDestCopy = true;
			break;
		case 4:
			if (NeedTempDestCopy(destParameter, *srcParameters[0], *srcParameters[1], *srcParameters[2], *srcParameters[3]) )
				needsTempDestCopy = true;
			break;
		}

		if (needsTempDestCopy)
		{
			if (!usedTempDestRegister)
			{
				usedTempDestRegister = true;
				AppendString(shaderbody, "\tfloat4 tempDst;\n");
			}
			sprintf(buffer, "\ttempDst = %s; // Need to save dst register as a temporary because it's aliased to a source register\n", destRegisterName);
			AppendString(shaderbody, buffer);
		}

		static const char* const writeMaskStrings[4] =
		{
			"\t%s(%s.%c, %s);\n",
			"\t%s(%s.%c, %s, %s);\n",
			"\t%s(%s.%c, %s, %s, %s);\n",
			"\t%s(%s.%c, %s, %s, %s, %s);\n",
		};

		if (writeMask & 0x1)
		{
			for (unsigned x = 0; x < numSourceRegisters; ++x)
			{
				PrintSourceRegNameAndSourceSwizzleOrTempReg<0>(*srcParameters[x], destParameter, srcRegisterNames[x], shaderInfo, srcAddrParameters[x]);
				PrintSourceModifier(*srcParameters[x], srcModifierStrings[x], srcRegisterNames[x]);
			}
			/*PrintSourceRegNameAndSourceSwizzleOrTempReg<0>(*srcParameters[0], destParameter, src0RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[0], src0ModifierString, src0RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<0>(*srcParameters[1], destParameter, src1RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[1], src1ModifierString, src1RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<0>(*srcParameters[2], destParameter, src2RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[2], src2ModifierString, src2RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<0>(*srcParameters[3], destParameter, src3RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[3], src3ModifierString, src3RegisterName);*/
			sprintf(buffer, writeMaskStrings[numSourceRegisters - 1], ShaderInfo::GetOpcodeFunctionString(opcode), destRegisterName, 'x', srcModifierStrings[0], srcModifierStrings[1], srcModifierStrings[2], srcModifierStrings[3]);
			AppendString(shaderbody, buffer);
		}
		if (writeMask & 0x2)
		{
			for (unsigned x = 0; x < numSourceRegisters; ++x)
			{
				PrintSourceRegNameAndSourceSwizzleOrTempReg<1>(*srcParameters[x], destParameter, srcRegisterNames[x], shaderInfo, srcAddrParameters[x]);
				PrintSourceModifier(*srcParameters[x], srcModifierStrings[x], srcRegisterNames[x]);
			}
			/*PrintSourceRegNameAndSourceSwizzleOrTempReg<1>(*srcParameters[0], destParameter, src0RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[0], src0ModifierString, src0RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<1>(*srcParameters[1], destParameter, src1RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[1], src1ModifierString, src1RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<1>(*srcParameters[2], destParameter, src2RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[2], src2ModifierString, src2RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<1>(*srcParameters[3], destParameter, src3RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[3], src3ModifierString, src3RegisterName);*/
			//sprintf(buffer, "\t%s(%s.y, %s, %s, %s, %s);\n", ShaderInfo::GetOpcodeFunctionString(opcode), destRegisterName, src0ModifierString, src1ModifierString, src2ModifierString, src3ModifierString);
			sprintf(buffer, writeMaskStrings[numSourceRegisters - 1], ShaderInfo::GetOpcodeFunctionString(opcode), destRegisterName, 'y', srcModifierStrings[0], srcModifierStrings[1], srcModifierStrings[2], srcModifierStrings[3]);
			AppendString(shaderbody, buffer);
		}
		if (writeMask & 0x4)
		{
			for (unsigned x = 0; x < numSourceRegisters; ++x)
			{
				PrintSourceRegNameAndSourceSwizzleOrTempReg<2>(*srcParameters[x], destParameter, srcRegisterNames[x], shaderInfo, srcAddrParameters[x]);
				PrintSourceModifier(*srcParameters[x], srcModifierStrings[x], srcRegisterNames[x]);
			}
			/*PrintSourceRegNameAndSourceSwizzleOrTempReg<2>(*srcParameters[0], destParameter, src0RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[0], src0ModifierString, src0RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<2>(*srcParameters[1], destParameter, src1RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[1], src1ModifierString, src1RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<2>(*srcParameters[2], destParameter, src2RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[2], src2ModifierString, src2RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<2>(*srcParameters[3], destParameter, src3RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[3], src3ModifierString, src3RegisterName);
			sprintf(buffer, "\t%s(%s.z, %s, %s, %s, %);\n", ShaderInfo::GetOpcodeFunctionString(opcode), destRegisterName, src0ModifierString, src1ModifierString, src2ModifierString, src3ModifierString);*/
			sprintf(buffer, writeMaskStrings[numSourceRegisters - 1], ShaderInfo::GetOpcodeFunctionString(opcode), destRegisterName, 'z', srcModifierStrings[0], srcModifierStrings[1], srcModifierStrings[2], srcModifierStrings[3]);
			AppendString(shaderbody, buffer);
		}
		if (writeMask & 0x8)
		{
			for (unsigned x = 0; x < numSourceRegisters; ++x)
			{
				PrintSourceRegNameAndSourceSwizzleOrTempReg<3>(*srcParameters[x], destParameter, srcRegisterNames[x], shaderInfo, srcAddrParameters[x]);
				PrintSourceModifier(*srcParameters[x], srcModifierStrings[x], srcRegisterNames[x]);
			}
			/*PrintSourceRegNameAndSourceSwizzleOrTempReg<3>(*srcParameters[0], destParameter, src0RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[0], src0ModifierString, src0RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<3>(*srcParameters[1], destParameter, src1RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[1], src1ModifierString, src1RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<3>(*srcParameters[2], destParameter, src2RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[2], src2ModifierString, src2RegisterName);
			PrintSourceRegNameAndSourceSwizzleOrTempReg<3>(*srcParameters[3], destParameter, src3RegisterName, shaderInfo);
			PrintSourceModifier(*srcParameters[3], src3ModifierString, src3RegisterName);
			sprintf(buffer, "\t%s(%s.w, %s, %s, %s, %s);\n", ShaderInfo::GetOpcodeFunctionString(opcode), destRegisterName, src0ModifierString, src1ModifierString, src2ModifierString, src3ModifierString);*/
			sprintf(buffer, writeMaskStrings[numSourceRegisters - 1], ShaderInfo::GetOpcodeFunctionString(opcode), destRegisterName, 'w', srcModifierStrings[0], srcModifierStrings[1], srcModifierStrings[2], srcModifierStrings[3]);
			AppendString(shaderbody, buffer);
		}

		if (destParameter.GetResultModifierUnshifted() & D3DSPDM_SATURATE)
		{
			if (writeMask & 0x1)
			{
				sprintf(buffer, "\t%s.x = saturate(%s.x);\n", destRegisterName, destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x2)
			{
				sprintf(buffer, "\t%s.y = saturate(%s.y);\n", destRegisterName, destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x4)
			{
				sprintf(buffer, "\t%s.z = saturate(%s.z);\n", destRegisterName, destRegisterName);
				AppendString(shaderbody, buffer);
			}
			if (writeMask & 0x8)
			{
				sprintf(buffer, "\t%s.w = saturate(%s.w);\n", destRegisterName, destRegisterName);
				AppendString(shaderbody, buffer);
			}
		}

		AppendString(shaderbody, "\n");
	}
		break;
	default:
		DbgBreakPrint("Error: Unknown shader parameters type");
	}

	return false;
}

static inline const bool ParseShaderInstructions(const ShaderInfo& shaderInfo, std::vector<char>& shaderbody)
{
	const DWORD* currentInstruction = shaderInfo.firstInstructionToken;
	bool doneParsing = false;
	bool usedTempDestRegister = false;
	while (!doneParsing)
	{
		doneParsing = ParseOpcode(currentInstruction, shaderInfo, shaderbody, usedTempDestRegister);
	}

	return true;
}

static inline const bool ShaderBodyJIT(const ShaderInfo& shaderInfo, std::vector<char>& cppfile)
{
	static std::vector<char> shaderbody;
	shaderbody.clear();

	AppendString(shaderbody, "\t// --- Begin shader body ---\n\n");

	if (!ParseShaderInstructions(shaderInfo, shaderbody) )
	{
		return false;
	}

	// ps_1_x shader fixup!
	if (shaderInfo.isPixelShader && shaderInfo.shaderMajorVersion < 2)
	{
		AppendString(shaderbody, "\n\t// ps_1_* output register fixup (because ps_1_* outputs with register r0 instead of into one of the oC[N] registers):\n");
		AppendString(shaderbody, "\tmov(oC0.x, r0.x);\n");
		AppendString(shaderbody, "\tmov(oC0.y, r0.y);\n");
		AppendString(shaderbody, "\tmov(oC0.z, r0.z);\n");
		AppendString(shaderbody, "\tmov(oC0.w, r0.w);\n\n");
	}

	AppendString(shaderbody, "\t// --- End shader body ---\n\n");

	cppfile.insert(cppfile.end(), shaderbody.begin(), shaderbody.end() );

	return true;
}

const bool JITCPPFileInternal(const ShaderInfo& shaderInfo, const char* const shaderFilename)
{
	char filename[MAX_PATH] = {0};
#pragma warning(push)
#pragma warning(disable:4996)
	// Looks like: "shaderjit\ps_3_0_len114_hash0xD9FF5963d.cpp"
	sprintf(filename, "%s\\%s.cpp", shaderJITTempDirectory, shaderFilename);
#pragma warning(pop)
	HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DbgBreakPrint("Error in CreateFile");
		return false;
	}

	static std::vector<char> cppfile;

	cppfile.clear();

	// Write prefix file
	LoadPrefixFileInternal(cppfile);

	// Append version as a comment:
	{
		char versionBuffer[32] = {0};
		sprintf(versionBuffer, "// Shader version: %cs_%u_%u\n", shaderInfo.isPixelShader ? 'p' : 'v', shaderInfo.shaderMajorVersion, shaderInfo.shaderMinorVersion);
		AppendString(cppfile, versionBuffer);
	}

	// Append version as three compile-time constants:
	{
		char versionBuffer[128] = {0};
		sprintf(versionBuffer, "static const bool IS_PIXEL_SHADER = %s;\n", shaderInfo.isPixelShader ? "true" : "false");
		AppendString(cppfile, versionBuffer);
		sprintf(versionBuffer, "static const unsigned SHADER_MAJOR_VER = %u;\n", shaderInfo.shaderMajorVersion);
		AppendString(cppfile, versionBuffer);
		sprintf(versionBuffer, "static const unsigned SHADER_MINOR_VER = %u;\n", shaderInfo.shaderMinorVersion);
		AppendString(cppfile, versionBuffer);
	}

#ifdef DISASM_SHADER
	if (shaderInfo.D3DXDisasmString != NULL)
	{
		AppendString(cppfile, "/* D3DX Disassembly:\n");
		AppendString(cppfile, shaderInfo.D3DXDisasmString);
		AppendString(cppfile, "*/\n");
	}
#endif // DISASM_SHADER

	if (!shaderInfo.fixedFunctionMacroDefines.empty() )
	{
		AppendString(cppfile, "/* Fixed function shader defines:\n\n");

		const unsigned numDefines = shaderInfo.fixedFunctionMacroDefines.size();
		for (unsigned x = 0; x < numDefines; ++x)
		{
			const D3DXMACRO& shaderDefine = shaderInfo.fixedFunctionMacroDefines[x];
			if (!shaderDefine.Name || !shaderDefine.Definition)
				break;

			AppendString(cppfile, "#define\t");
			AppendString(cppfile, shaderDefine.Name);
			AppendString(cppfile, "\t");
			AppendString(cppfile, shaderDefine.Definition);
			AppendString(cppfile, "\n");
		}

		AppendString(cppfile, "*/\n");
	}

	// Write global static constants for local constant register values
	const unsigned numFloatConsts = shaderInfo.initialConstantValues.size();
	for (unsigned x = 0; x < numFloatConsts; ++x)
	{
		const InitialConstantValue& floatConst = shaderInfo.initialConstantValues[x];
		char floatConstBuffer[128] = {0};
		sprintf(floatConstBuffer, "static const float4 c%u = {%ff, %ff, %ff, %ff};\n", floatConst.constantRegisterIndex, floatConst.initialValue.x, floatConst.initialValue.y, floatConst.initialValue.z, floatConst.initialValue.w);
		AppendString(cppfile, floatConstBuffer);
	}
	const unsigned numIntConsts = shaderInfo.initialConstantValuesI.size();
	for (unsigned x = 0; x < numIntConsts; ++x)
	{
		const InitialConstantValueI& intConst = shaderInfo.initialConstantValuesI[x];
		char intConstBuffer[128] = {0};
		sprintf(intConstBuffer, "static const int4 i%u(%i, %i, %i, %i);\n", intConst.constantRegisterIndex, intConst.initialValue.x, intConst.initialValue.y, intConst.initialValue.z, intConst.initialValue.w);
		AppendString(cppfile, intConstBuffer);
	}
	const unsigned numBoolConsts = shaderInfo.initialConstantValuesB.size();
	for (unsigned x = 0; x < numBoolConsts; ++x)
	{
		const InitialConstantValueB& boolConst = shaderInfo.initialConstantValuesB[x];
		char boolConstBuffer[128] = {0};
		sprintf(boolConstBuffer, "static const BOOL b%u = %s;\n", boolConst.constantRegisterIndex, boolConst.initialValue ? "TRUE" : "FALSE");
		AppendString(cppfile, boolConstBuffer);
	}

	// Write ShaderMain header (PSMain or VSMain)
	if (shaderInfo.isPixelShader)
		AppendString(cppfile, "__declspec(dllexport) void __fastcall PixelShaderMain(PShaderEngine& ps)\n{\n");
	else
		AppendString(cppfile, "__declspec(dllexport) void __fastcall VertexShaderMain(VShaderEngine& vs)\n{\n");

	// Paste all disassembly and shader stats in a multiline comment
	AppendString(cppfile, "\t/*\n");
	{
		char shaderStatsString[1024];
		shaderInfo.PrintShaderStatsToString(shaderStatsString);
		AppendString(cppfile, shaderStatsString);
	}
	AppendString(cppfile, shaderInfo.shaderDisasmBuffer);
	AppendString(cppfile, "\t*/\n");

	// Write shader inputs (v and t registers)
	const unsigned numDeclaredRegs = shaderInfo.declaredRegisters.size();
	for (unsigned x = 0; x < numDeclaredRegs; ++x)
	{
		const DeclaredRegister& thisReg = shaderInfo.declaredRegisters[x];
		if (thisReg.isOutputRegister)
			continue;

		char inputRegBuffer[256] = {0};

		if (shaderInfo.isPixelShader)
		{
			if (shaderInfo.shaderMajorVersion < 3)
			{
				switch (thisReg.registerType)
				{
				default:
					DbgBreakPrint("Error: Unknown register type!");
				case D3DSPR_TEXTURE:
				{
					bool textureRegCanBeConst = true;
					if (shaderInfo.shaderMajorVersion == 1 && shaderInfo.shaderMinorVersion < 4 && shaderInfo.numTexInstructions > 0)
						textureRegCanBeConst = false;
					sprintf(inputRegBuffer, "\t%sfloat4& %c%u = *(%sfloat4* const)&(ps.inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.%c[%u]);\n", 
						textureRegCanBeConst ? "const " : "", 't', thisReg.registerIndex, textureRegCanBeConst ? "const " : "", 't', thisReg.registerIndex);
				}
					break;
				case D3DSPR_SAMPLER:
					sprintf(inputRegBuffer, "\tconst sampler* const s%u = &(ps.constantRegisters->s[%u]);\n", thisReg.registerIndex, thisReg.registerIndex);
					break;
				case D3DSPR_INPUT:
					sprintf(inputRegBuffer, "\tconst float4& %c%u = *(const float4* const)&(ps.inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.%c[%u]);\n", 'v', thisReg.registerIndex, 'v', thisReg.registerIndex);
					break;
				}
			}
			else if (shaderInfo.shaderMajorVersion == 3)
			{
				switch (thisReg.registerType)
				{
				case D3DSPR_MISCTYPE:
					switch (thisReg.registerIndex)
					{
					default:
						DbgBreakPrint("Error: Unknown MISC register index!");
					case D3DSMO_POSITION:
						sprintf(inputRegBuffer, "\tconst float4& vpos%u = *(const float4* const)&(ps.miscRegisters[0].vPos);\n", thisReg.registerIndex); // Technically there's no such thing as VPOS1, VPOS2, etc., but this is for simplicity...
						break;
					case D3DSMO_FACE:
						sprintf(inputRegBuffer, "\tconst float4& vface%u = *(const float4* const)&(ps.miscRegisters[0].vFace);\n", thisReg.registerIndex);
						break;
					}
					break;
				case D3DSPR_SAMPLER:
					sprintf(inputRegBuffer, "\tconst sampler* const s%u = &(ps.constantRegisters->s[%u]);\n", thisReg.registerIndex, thisReg.registerIndex);
					break;
				default:
					DbgBreakPrint("Error: Unknown register type!");
				case D3DSPR_INPUT:
					sprintf(inputRegBuffer, "\tconst float4& v%u = *(const float4* const)&(ps.inputRegisters[0].ps_interpolated_inputs.ps_3_0_inputs.t[%u]);\n", thisReg.registerIndex, thisReg.registerIndex);
					break;
				}
			}
			else
			{
				DbgBreakPrint("Error: Unexpected pixel shader version (not 1, 2, or 3)");
			}
		}
		else
		{
			sprintf(inputRegBuffer, "\tconst float4& v%u = *(const float4* const)&(vs.inputRegisters[0].v[%u]);\n", thisReg.registerIndex, thisReg.registerIndex);
		}

		AppendString(cppfile, inputRegBuffer);
	}

	// ps_1_* shaders don't declare input texcoord registers (v and t registers), so we need to manually add these
	if (shaderInfo.isPixelShader && shaderInfo.shaderMajorVersion < 2)
	{
		bool textureRegCanBeConst = true;
		if (shaderInfo.shaderMinorVersion < 4 && shaderInfo.numTexInstructions > 0)
			textureRegCanBeConst = false;

		char textureRegBuffer[256] = {0};
		for (unsigned char v = 0; v < D3DMCS_COLOR2; ++v)
		{
			if (shaderInfo.inputRegistersUsedBitmask & (1 << v) )
			{
				sprintf(textureRegBuffer, "\tconst float4& %c%u = *(const float4* const)&(ps.inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.%c[%u]);\n", 
					'v', v, 'v', v);
				AppendString(cppfile, textureRegBuffer);
			}
		}
		for (unsigned char t = 0; t < 6; ++t)
		{
			if (shaderInfo.inputRegistersUsedBitmask & (1 << (t + D3DMCS_COLOR2) ) )
			{
				sprintf(textureRegBuffer, "\t%sfloat4& %c%u = *(%sfloat4* const)&(ps.inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.%c[%u]);\n", 
					textureRegCanBeConst ? "const " : "", 't', t, textureRegCanBeConst ? "const " : "", 't', t);
				AppendString(cppfile, textureRegBuffer);
			}
		}
	}

	// Declare address register (vertex shaders only):
	if (!shaderInfo.isPixelShader /*TODO: Only do this if the shader uses relative addressing*/)
		AppendString(cppfile, "\tint4& a0 = (vs.runtimeRegisters[0].a);\n");

	// Declare output registers:
	if (shaderInfo.isPixelShader)
	{
		for (unsigned x = 0; x < 4; ++x)
		{
			if (shaderInfo.usedMRTMask & (1 << x) )
			{
				char tempRegBuffer[64] = {0};
				sprintf(tempRegBuffer, "\tfloat4& oC%u = *(float4* const)&(ps.outputRegisters[0].oC[%u]);\n", x, x);
				AppendString(cppfile, tempRegBuffer);
			}
		}

		if (shaderInfo.psWritesDepth)
		{
			AppendString(cppfile, "\tfloat4& oDepth = *(float4* const)&(ps.outputRegisters[0].oDepth);\n");
		}
	}
	else
	{
		const unsigned numWrittenOutputs = shaderInfo.writtenOutputRegisters.size();
		for (unsigned x = 0; x < numWrittenOutputs; ++x)
		{
			char tempRegBuffer[128] = {0};
			const WrittenOutputRegister& thisReg = shaderInfo.writtenOutputRegisters[x];
			if (shaderInfo.shaderMajorVersion < 3)
			{
				switch (thisReg.registerType)
				{
				case D3DSPR_RASTOUT  :
					switch (thisReg.registerIndex)
					{
					default:
						DbgBreakPrint("Error: Unknown vertex shader RASTOUT register index");
					case D3DSRO_POSITION:
						sprintf(tempRegBuffer, "\tfloat4& oPos%u = *(float4* const)&(vs.outputRegisters[0]->oPos);\n", thisReg.registerIndex);
						AppendString(cppfile, tempRegBuffer);
						break;
					case D3DSRO_FOG:
						sprintf(tempRegBuffer, "\tfloat4& oFog%u = *(float4* const)&(vs.outputRegisters[0]->oFog);\n", thisReg.registerIndex);
						AppendString(cppfile, tempRegBuffer);
						break;
					case D3DSRO_POINT_SIZE:
						sprintf(tempRegBuffer, "\tfloat4& oPts%u = *(float4* const)&(vs.outputRegisters[0]->oPts);\n", thisReg.registerIndex);
						AppendString(cppfile, tempRegBuffer);
						break;
					}
					break;
				case D3DSPR_ATTROUT  :
					sprintf(tempRegBuffer, "\tfloat4& oD%u = *(float4* const)&(vs.outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[%u]);\n", thisReg.registerIndex, thisReg.registerIndex);
					AppendString(cppfile, tempRegBuffer);
					break;
				case D3DSPR_TEXCRDOUT:
					sprintf(tempRegBuffer, "\tfloat4& oT%u = *(float4* const)&(vs.outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oT[%u]);\n", thisReg.registerIndex, thisReg.registerIndex);
					AppendString(cppfile, tempRegBuffer);
					break;
				default:
					break;
				}
			}
			else
			{
				sprintf(tempRegBuffer, "\tfloat4& oT%u = *(float4* const)&(vs.outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[%u]);\n", thisReg.registerIndex, thisReg.registerIndex);
				AppendString(cppfile, tempRegBuffer);
			}
		}
	}

	// Declare used non-immediate float const registers
	const unsigned numUsedConstF = shaderInfo.usedConstantsF.size();
	for (unsigned x = 0; x < numUsedConstF; ++x)
	{
		const unsigned usedConstF = shaderInfo.usedConstantsF[x];
		if (IsConstantGlobal(usedConstF, shaderInfo.initialConstantValues) )
		{
			char tempRegBuffer[96] = {0};
			sprintf(tempRegBuffer, "\tconst float4& c%u = *(const float4* const)&(%cs.constantRegisters->c[%u]);\n", usedConstF, shaderInfo.isPixelShader ? 'p' : 'v', usedConstF);
			AppendString(cppfile, tempRegBuffer);
		}
	}

	// Declare used non-immediate int const registers
	const unsigned numUsedConstI = shaderInfo.usedConstantsI.size();
	for (unsigned x = 0; x < numUsedConstI; ++x)
	{
		const unsigned usedConstI = shaderInfo.usedConstantsI[x];
		if (IsConstantGlobal(usedConstI, shaderInfo.initialConstantValuesI) )
		{
			char tempRegBuffer[96] = {0};
			sprintf(tempRegBuffer, "\tconst int4& i%u = %cs.constantRegisters->i[%u];\n", usedConstI, shaderInfo.isPixelShader ? 'p' : 'v', usedConstI);
			AppendString(cppfile, tempRegBuffer);
		}
	}

	// Declare used non-immediate bool const registers
	const unsigned numUsedConstB = shaderInfo.usedConstantsB.size();
	for (unsigned x = 0; x < numUsedConstB; ++x)
	{
		const unsigned usedConstB = shaderInfo.usedConstantsB[x];
		if (IsConstantGlobal(usedConstB, shaderInfo.initialConstantValuesB) )
		{
			char tempRegBuffer[96] = {0};
			sprintf(tempRegBuffer, "\tconst BOOL& b%u = %cs.constantRegisters->b[%u];\n", usedConstB, shaderInfo.isPixelShader ? 'p' : 'v', usedConstB);
			AppendString(cppfile, tempRegBuffer);
		}
	}

	// Declare used temp. registers (r registers)
	for (unsigned i = 0; i < 32; ++i)
	{
		if (shaderInfo.tempRegistersUsedBitmask & (1 << i) )
		{
			char tempRegBuffer[32] = {0};
			sprintf(tempRegBuffer, "\tfloat4 r%u;\n", i);
			AppendString(cppfile, tempRegBuffer);
		}
	}
	
	// Actual shader JIT work goes here
	if (!ShaderBodyJIT(shaderInfo, cppfile) )
	{
		DbgBreakPrint("Error in shader JIT main process");
		return false;
	}
	
	// Append suffix (return, closing braces)
	if (shaderInfo.isPixelShader)
	{
		AppendString(cppfile, "\tps.outputRegisters[0].pixelStatus = normalWrite;\n");
	}
	AppendString(cppfile, "} // end shadermain\n"); // Close the function brace
	AppendString(cppfile, "\n} // end extern \"C\"\n"); // Close the extern "C" brace

	// Write the file
	DWORD numBytesWritten = 0;
	if (!WriteFile(hFile, &(cppfile.front() ), cppfile.size(), &numBytesWritten, NULL) )
	{
		DbgBreakPrint("Error in WriteFile");
	}

	if (cppfile.size() != numBytesWritten)
	{
		DbgBreakPrint("Error: Num bytes written doesn't match for cpp file");
	}

	if (!CloseHandle(hFile) )
	{
		DbgBreakPrint("Error in CloseHandle");
		return false;
	}

	return true;
}

#pragma warning(pop)
