#pragma once

#include "VShaderEngine.h"
#include "IDirect3DDevice9Hook.h"
#include "IDirect3DVertexShader9Hook.h"

static const D3DXVECTOR4 negHalfVec(-0.5f, -0.5f, -0.5f, -0.5f);

template <const bool isVS3_0>
D3DXVECTOR4& VShaderEngine::ResolveDstParameterVS(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned index)
{
	switch (registerType)
	{
	case D3DSPR_TEMP       :
#ifdef _DEBUG
		if (index >= ARRAYSIZE(VS_2_0_RuntimeRegisters::r) )
		{
			__debugbreak(); // Out of bounds register index!
		}
#endif
		return runtimeRegisters[0].r[index];
	case D3DSPR_INPUT      :
#ifdef _DEBUG
		__debugbreak(); // input registers can't be dst registers
#endif
		return inputRegisters[0].v[index];
	case D3DSPR_CONST      :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#endif
		return const_cast<D3DXVECTOR4&>(constantRegisters->c[index]);
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		return *(D3DXVECTOR4* const)&runtimeRegisters[0].a;
	case D3DSPR_RASTOUT    :
#ifdef _DEBUG
		if (isVS3_0)
		{
			__debugbreak();
		}
		if (index >= 3)
		{
			__debugbreak(); // Out of bounds register index!
		}
#endif
		return (&outputRegisters[0]->oPos)[index];
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		if (isVS3_0)
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_3_0_outputs::oT) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			return *(D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
		}
		else
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_2_0_outputs::oT) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			return *(D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
		}
	case D3DSPR_CONSTINT   :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#endif
		return *(D3DXVECTOR4* const)&constantRegisters->i[index];
	case D3DSPR_COLOROUT   :
		if (isVS3_0)
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_3_0_outputs::oT) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			return *(D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
		}
		else
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_2_0_outputs::oD) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			return *(D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
		}
	case D3DSPR_ATTROUT    :
		if (isVS3_0)
			return outputRegisters[0]->oFog;
		else
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_2_0_outputs::oD) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			return *(D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
		}
	case D3DSPR_DEPTHOUT   :
#ifdef _DEBUG
		__debugbreak(); // Only usable by the PShader
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_SAMPLER    :
#ifdef _DEBUG
		__debugbreak(); // Sampler registers can't be dst registers - also sampler registers can't be used before vs_3_0
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_CONSTBOOL  :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#endif
		return *(D3DXVECTOR4* const)&constantRegisters->b[index];
	case D3DSPR_LOOP       :
		return *(D3DXVECTOR4* const)&runtimeRegisters[0].aL;
	case D3DSPR_TEMPFLOAT16:
#ifdef _DEBUG
		__debugbreak();
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_MISCTYPE   :
#ifdef _DEBUG
		__debugbreak();
#endif
		return runtimeRegisters[0].r[index];
	case D3DSPR_LABEL      :
#ifdef _DEBUG
		__debugbreak(); // Labels can't be dst registers
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_PREDICATE  :
		return *(D3DXVECTOR4* const)&runtimeRegisters[0].p;
	default:
#ifdef _DEBUG
		{
			__debugbreak(); // Should never be here (invalid SPR type)
		}
#else
			__assume(0);
#endif
	}
	return *(D3DXVECTOR4* const)NULL;
}

// TODO: vs_3_0 introduced the concept of dst relative addressing (for output registers), we need to account for this somehow
void VShaderEngine::WriteDstParameter(const dstParameterToken dstParameter, const D3DXVECTOR4& value)
{
	D3DXVECTOR4& out = ResolveDstParameter(dstParameter);
	const unsigned writeMask = dstParameter.GetWriteMask();
	if (dstParameter.GetResultModifierUnshifted() & D3DSPDM_SATURATE) // Saturate
	{
		switch (writeMask)
		{
		case 0:
#ifdef _DEBUG
			__debugbreak(); // Should never be hereWriteDstParameter4
#endif
			break;
		case 0x1:
			out.x = saturate(value.x);
			break;
		case 0x3:
			out.x = saturate(value.x);
			// Intentional fallthrough
		case 0x2:
			out.y = saturate(value.y);
			break;
		case 0x7:
			out.x = saturate(value.x);
			// Intentional fallthrough
		case 0x6:
			out.y = saturate(value.y);
			// Intentional fallthrough
		case 0x4:
			out.z = saturate(value.z);
			break;
		case 0x5:
			out.x = saturate(value.x);
			out.z = saturate(value.z);
			break;
		default:
#ifdef _DEBUG
			__debugbreak();
#endif
		case 0xF: // The most common case, all bits are set!
			out.x = saturate(value.x);
			// Intentional fallthrough
		case 0xE: // 14
			out.y = saturate(value.y);
			// Intentional fallthrough
		case 0xC: // 12
			out.z = saturate(value.z);
			// Intentional fallthrough
		case 0x8:
			out.w = saturate(value.w);
			break;
		case 0x9:
			out.x = saturate(value.x);
			out.w = saturate(value.w);
			break;
		case 0xA: // 10
			out.y = saturate(value.y);
			out.w = saturate(value.w);
			break;
		case 0xB: // 11
			out.x = saturate(value.x);
			out.y = saturate(value.y);
			out.w = saturate(value.w);
			break;
		case 0xD: // 13
			out.x = saturate(value.x);
			out.z = saturate(value.z);
			out.w = saturate(value.w);
			break;
		}
	}
	else // No saturates
	{
		switch (writeMask)
		{
		case 0:
#ifdef _DEBUG
			__debugbreak(); // Should never be here
#endif
			break;
		case 0x1:
			out.x = value.x;
			break;
		case 0x3:
			out.x = value.x;
			// Intentional fallthrough
		case 0x2:
			out.y = value.y;
			break;
		case 0x7:
			out.x = value.x;
			// Intentional fallthrough
		case 0x6:
			out.y = value.y;
			// Intentional fallthrough
		case 0x4:
			out.z = value.z;
			break;
		case 0x5:
			out.x = value.x;
			out.z = value.z;
			break;
		default:
#ifdef _DEBUG
			__debugbreak();
#endif
		case 0xF: // The most common case, all bits are set!
			out.x = value.x;
			// Intentional fallthrough
		case 0xE: // 14
			out.y = value.y;
			// Intentional fallthrough
		case 0xC: // 12
			out.z = value.z;
			// Intentional fallthrough
		case 0x8:
			out.w = value.w;
			break;
		case 0x9:
			out.x = value.x;
			out.w = value.w;
			break;
		case 0xA: // 10
			out.y = value.y;
			out.w = value.w;
			break;
		case 0xB: // 11
			out.x = value.x;
			out.y = value.y;
			out.w = value.w;
			break;
		case 0xD: // 13
			out.x = value.x;
			out.z = value.z;
			out.w = value.w;
			break;
		}
	}
}

D3DXVECTOR4& VShaderEngine::ResolveDstParameter(const dstParameterToken dstParameter)
{
	const unsigned index = dstParameter.GetRegisterIndex();
	switch (shaderInfo->shaderMajorVersion)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case 1:
	case 2:
		return ResolveDstParameterVS<false>(dstParameter.GetRegisterType(), index);
	case 3:
		return ResolveDstParameterVS<true>(dstParameter.GetRegisterType(), index);
	}
}

const float VShaderEngine::ResolveSrcReplicateSwizzle(const DWORD rawSrcBytecode, const D3DXVECTOR4& registerData) const
{
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)&rawSrcBytecode;
	const float* const fRegisterData = (const float* const)&registerData;
	return fRegisterData[srcParameter.GetChannelSwizzle()];
}

// Source parameter token: https://msdn.microsoft.com/en-us/library/windows/hardware/ff569716(v=vs.85).aspx
const D3DXVECTOR4 VShaderEngine::ResolveSrcRegister(const DWORD*& ptrSrcBytecode) const
{
	const DWORD rawSrcBytecode = *ptrSrcBytecode;
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)&rawSrcBytecode;

	// Advance the instruction pointer (very important):
	++ptrSrcBytecode;

	const D3DXVECTOR4* sourceParamPtr = NULL;

	switch (srcParameter.GetRelativeAddressingType() )
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case D3DSHADER_ADDRMODE_ABSOLUTE:
		sourceParamPtr = &ResolveSrcParameter(srcParameter);
		break;
	case D3DSHADER_ADDRMODE_RELATIVE:
	{
		int addressOffsetRegisters = 0;

		const D3DXVECTOR4* const originalSourceParamPtr = &ResolveSrcParameter(srcParameter);

		// If SM2 or up, then relative addressing can use either the address register (a) or the loop
		// counter register (aL), so it needs another DWORD to know which to use. Otherwise (if vs_1_1) then
		// there's no extra relative addressing token and reading from a.x (the first component of the address
		// register) is assumed.
		if (shaderInfo->shaderMajorVersion > 1)
		{
			// Shader relative addressing: https://msdn.microsoft.com/en-us/library/windows/hardware/ff569708(v=vs.85).aspx
			const srcParameterToken& relativeAddressingSrcToken = *(const srcParameterToken* const)ptrSrcBytecode++;

			const int4& addressRegister = *(const int4* const)&ResolveSrcParameter(relativeAddressingSrcToken);
			const unsigned int addressRegisterSwizzle = relativeAddressingSrcToken.GetChannelSwizzleXYZW();
			switch (addressRegisterSwizzle)
			{
			default:
			case D3DSP_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT:
			case D3DSP_REPLICATERED >> D3DVS_SWIZZLE_SHIFT:
				addressOffsetRegisters = RoundToNearest(*(const float* const)&addressRegister.x);
				break;
			case D3DSP_REPLICATEGREEN >> D3DVS_SWIZZLE_SHIFT:
				addressOffsetRegisters = RoundToNearest(*(const float* const)&addressRegister.y);
				break;
			case D3DSP_REPLICATEBLUE >> D3DVS_SWIZZLE_SHIFT:
				addressOffsetRegisters = RoundToNearest(*(const float* const)&addressRegister.z);
				break;
			case D3DSP_REPLICATEALPHA >> D3DVS_SWIZZLE_SHIFT:
				addressOffsetRegisters = RoundToNearest(*(const float* const)&addressRegister.w);
				break;
			}
		}
		else
		{
			const int4& addressRegister = *(const int4* const)&ResolveSrcParameterVS<false>(D3DSPR_ADDR, 0);
			addressOffsetRegisters = RoundToNearest(*(const float* const)&addressRegister.x);
		}

		const D3DXVECTOR4* const relativeRegister = originalSourceParamPtr + addressOffsetRegisters;
		sourceParamPtr = &ResolveSrcAddressIfValid(relativeRegister);
	}

		break;
	}

	const D3DXVECTOR4& sourceParam = *sourceParamPtr;
	const unsigned char sourceSwizzle = srcParameter.GetSwizzle();

	D3DXVECTOR4 ret;

	if (sourceSwizzle == (D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) )
	{
		ret = sourceParam;
	}
	else
	{
		const float* const fltParams = &sourceParam.x;
		ret.x = fltParams[sourceSwizzle & 0x3]; // R channel
		ret.y = fltParams[(sourceSwizzle >> 2) & 0x3]; // G channel
		ret.z = fltParams[(sourceSwizzle >> 4) & 0x3]; // B channel
		ret.w = fltParams[(sourceSwizzle >> 6) & 0x3]; // A channel
	}

	switch (srcParameter.GetSourceModifiersUnshifted() )
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#endif
	case D3DSPSM_NONE   :
		return ret;
	case D3DSPSM_NEG    :
		ret *= -1.0f;
		break;
	case D3DSPSM_BIAS   :
		ret += negHalfVec;
		break;
	case D3DSPSM_BIASNEG:
		ret += negHalfVec;
		ret *= -1.0f;
		break;
	case D3DSPSM_SIGN   :
		ret += negHalfVec;
		ret *= 2.0f;
		break;
	case D3DSPSM_SIGNNEG:
		ret += negHalfVec;
		ret *= -2.0f;
		break;
	case D3DSPSM_COMP   :
		ret.x = 1.0f - ret.x;
		ret.y = 1.0f - ret.y;
		ret.z = 1.0f - ret.z;
		ret.w = 1.0f - ret.w;
		break;
	case D3DSPSM_X2     :
		ret *= 2.0f;
		break;
	case D3DSPSM_X2NEG  :
		ret *= -2.0f;
		break;
	case D3DSPSM_DZ     :
	{
		const float invZ = 1.0f / ret.z;
		ret.x *= invZ;
		ret.y *= invZ;
	}
		break;
	case D3DSPSM_DW     :
	{
		const float invW = 1.0f / ret.w;
		ret.x *= invW;
		ret.y *= invW;
	}
		break;
	case D3DSPSM_ABS    :
		ret.x = fabsf(ret.x);
		ret.y = fabsf(ret.y);
		ret.z = fabsf(ret.z);
		ret.w = fabsf(ret.w);
		break;
	case D3DSPSM_ABSNEG :
		ret.x = -fabsf(ret.x);
		ret.y = -fabsf(ret.y);
		ret.z = -fabsf(ret.z);
		ret.w = -fabsf(ret.w);
		break;
	case D3DSPSM_NOT    :
	{
		BOOL* const boolPtr = (BOOL* const)&ret.x;
		*boolPtr = !*boolPtr;
	}
		break;
	}

	return ret;
}

template <const bool isVS3_0>
const D3DXVECTOR4& VShaderEngine::ResolveSrcParameterVS(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned index) const
{
	switch (registerType)
	{
	case D3DSPR_TEMP       :
		return runtimeRegisters[0].r[index];
	case D3DSPR_INPUT      :
		return inputRegisters[0].v[index];
	case D3DSPR_CONST      :
		return constantRegisters->c[index];
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		return *(const D3DXVECTOR4* const)&runtimeRegisters[0].a;
	case D3DSPR_RASTOUT    :
		if (isVS3_0)
		{
#ifdef _DEBUG
			__debugbreak();
#endif
			return runtimeRegisters[0].r[0];
		}
		else
			return (&outputRegisters[0]->oPos)[index];
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		if (isVS3_0)
			return *(const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
		else
			return *(const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
	case D3DSPR_CONSTINT   :
		return *(const D3DXVECTOR4* const)&constantRegisters->i[index];
	case D3DSPR_COLOROUT   :
		if (isVS3_0)
			return *(const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
		else
			return *(const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
	case D3DSPR_ATTROUT    :
		if (isVS3_0)
			return outputRegisters[0]->oFog;
		else
			return *(const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
	case D3DSPR_DEPTHOUT   :
#ifdef _DEBUG
		__debugbreak();
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_SAMPLER    :
#ifdef _DEBUG
		__debugbreak();
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
#ifdef _DEBUG
		__debugbreak();
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_CONSTBOOL  :
		return *(const D3DXVECTOR4* const)&constantRegisters->b[index];
	case D3DSPR_LOOP       :
		return *(const D3DXVECTOR4* const)&runtimeRegisters[0].aL;
	case D3DSPR_TEMPFLOAT16:
#ifdef _DEBUG
		__debugbreak();
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_MISCTYPE   :
#ifdef _DEBUG
		__debugbreak();
#endif
		return runtimeRegisters[0].r[index];
	case D3DSPR_LABEL      :
#ifdef _DEBUG
		__debugbreak(); // Uhhhhh, is this debugbreak correct? Aren't Labels allowed to be source parameters for the CALL and CALLNZ instructions?
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_PREDICATE  :
		return *(const D3DXVECTOR4* const)&runtimeRegisters[0].p;
	default:
#ifdef _DEBUG
		{
			__debugbreak();
		}
#else
			__assume(0);
#endif
	}
	return *(const D3DXVECTOR4* const)NULL;
}

const D3DXVECTOR4& VShaderEngine::ResolveSrcParameter(const srcParameterToken srcParameter) const 
{
	const unsigned index = srcParameter.GetRegisterIndex();
	switch (shaderInfo->shaderMajorVersion)
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case 1:
	case 2:
		return ResolveSrcParameterVS<false>(srcParameter.GetRegisterType(), index);
	case 3:
		return ResolveSrcParameterVS<true>(srcParameter.GetRegisterType(), index);
	}
	return *(const D3DXVECTOR4* const)NULL;
}

// Called once at device reset time
void VShaderEngine::GlobalInit(const VS_2_0_ConstantsBuffer* const _constantRegisters)
{
	memset(this, 0, sizeof(*this) );

	GlobalInitTex2DFunctionTable();

	constantRegisters = _constantRegisters;
}

void VShaderEngine::Init(const DeviceState& deviceState, const ShaderInfo& _shaderInfo, VS_2_0_ConstantsBuffer* const mutableConstantRegisters)
{
	shaderInfo = &_shaderInfo;

#ifdef _DEBUG
	if (!deviceState.currentVertexShader)
	{
		__debugbreak();
	}
#endif
	instructionPtr = _shaderInfo.firstInstructionToken;

	const DeviceState_ShaderRegisters& globalVertexShaderConstants = deviceState.vertexShaderRegisters;

	InitGlobalConstants(globalVertexShaderConstants, mutableConstantRegisters);

	// Local constants ("immediate constants") always take precedence over global constants: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205596(v=vs.85).aspx
	InitLocalConstants(_shaderInfo, mutableConstantRegisters);

	InitSamplers(deviceState, mutableConstantRegisters);
}

void VShaderEngine::InitGlobalConstants(const DeviceState_ShaderRegisters& globalVertexShaderConstants, VS_2_0_ConstantsBuffer* const mutableConstantRegisters)
{
	memcpy(&mutableConstantRegisters->c, &globalVertexShaderConstants.floats, sizeof(D3DXVECTOR4) * ARRAYSIZE(VS_2_0_ConstantsBuffer::c) );
	memcpy(&mutableConstantRegisters->b, &globalVertexShaderConstants.bools, sizeof(BOOL) * ARRAYSIZE(VS_2_0_ConstantsBuffer::b) );
	memcpy(&mutableConstantRegisters->i, &globalVertexShaderConstants.ints, sizeof(int4) * ARRAYSIZE(VS_2_0_ConstantsBuffer::i) );
}

void VShaderEngine::InitLocalConstants(const ShaderInfo& vertexShaderInfo, VS_2_0_ConstantsBuffer* const mutableConstantRegisters)
{
	// Floats
	{
		const unsigned numFloatConsts = vertexShaderInfo.initialConstantValues.size();
		for (unsigned x = 0; x < numFloatConsts; ++x)
		{
			const InitialConstantValue& constF = vertexShaderInfo.initialConstantValues[x];
			mutableConstantRegisters->c[constF.constantRegisterIndex] = constF.initialValue;
		}
	}

	// Bools
	{
		const unsigned numBoolConsts = vertexShaderInfo.initialConstantValuesB.size();
		for (unsigned x = 0; x < numBoolConsts; ++x)
		{
			const InitialConstantValueB& constB = vertexShaderInfo.initialConstantValuesB[x];
			mutableConstantRegisters->b[constB.constantRegisterIndex] = constB.initialValue;
		}
	}

	// Ints
	{
		const unsigned numIntConsts = vertexShaderInfo.initialConstantValuesI.size();
		for (unsigned x = 0; x < numIntConsts; ++x)
		{
			const InitialConstantValueI& constI = vertexShaderInfo.initialConstantValuesI[x];
			mutableConstantRegisters->i[constI.constantRegisterIndex] = constI.initialValue;
		}
	}
}

void VShaderEngine::InitSamplers(const DeviceState& deviceState, VS_2_0_ConstantsBuffer* const mutableConstantRegisters)
{
	for (unsigned x = 0; x < ARRAYSIZE(VS_2_0_ConstantsBuffer::s); ++x)
	{
		sampler& thisSampler = mutableConstantRegisters->s[x];
		thisSampler.texture = deviceState.currentTextures[x + D3DVERTEXTEXTURESAMPLER0];
		thisSampler.samplerState = deviceState.currentSamplerStates[x + D3DVERTEXTEXTURESAMPLER0];
	}
}

// Called once for every vertex or vertex warp to reset the state of the interpreter to its default
void VShaderEngine::Reset(VS_2_0_OutputRegisters* const * const outputVerts, const unsigned char numVerts)
{
	instructionPtr = shaderInfo->firstInstructionToken;
#ifdef _DEBUG
	for (unsigned char x = 0; x < ARRAYSIZE(outputRegisters); ++x)
		outputRegisters[x] = NULL;
#endif
	for (unsigned char x = 0; x < numVerts; ++x)
		outputRegisters[x] = outputVerts[x];
}
