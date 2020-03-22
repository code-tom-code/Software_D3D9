#pragma once

#include "PShaderEngine.h"
#include "ShaderAnalysis.h"
#include "IDirect3DDevice9Hook.h"
#include "IDirect3DPixelShader9Hook.h"
#include "IDirect3DTexture9Hook.h"

void PShaderEngine::WriteDstParameter(const DWORD rawDstBytecode, const D3DXVECTOR4& value)
{
	D3DXVECTOR4& out = ResolveDstParameter(rawDstBytecode);
	const dstParameterToken& dstParameter = *(const dstParameterToken* const)&rawDstBytecode;
	const unsigned writeMask = dstParameter.GetWriteMask();
	if (dstParameter.GetResultModifierUnshifted() & D3DSPDM_SATURATE) // Saturate
	{
		switch (writeMask)
		{
		case 0:
#ifdef _DEBUG
			DbgBreakPrint("Error: Zero write mask"); // Should never be here
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
			DbgBreakPrint("Error: Out of range write mask");
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
			DbgBreakPrint("Error: Zero write mask"); // Should never be here
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
			DbgBreakPrint("Error: Out of range write mask!");
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

D3DXVECTOR4& PShaderEngine::ResolveDstParameter(const DWORD rawDstBytecode)
{
	const dstParameterToken& dstParameter = *(const dstParameterToken* const)&rawDstBytecode;
	const unsigned index = dstParameter.GetRegisterIndex();
	switch (dstParameter.GetRegisterType() )
	{
	case D3DSPR_TEMP       :
#ifdef _DEBUG
		if (index >= ARRAYSIZE(PS_2_0_RuntimeRegisters::r) )
		{
			__debugbreak(); // Out of bounds register index
		}
#endif
		return runtimeRegisters[0].r[index];
	case D3DSPR_INPUT      :
#ifdef _DEBUG
		__debugbreak(); // input registers can't be dst registers
#endif
		return *(D3DXVECTOR4* const)&(inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[index]);
	case D3DSPR_CONST      :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst parameters
#endif
		return const_cast<D3DXVECTOR4&>(constantRegisters->c[index]);
	case D3DSPR_TEXTURE       : // Also known as D3DSPR_ADDR (VS)
#ifdef _DEBUG
		if (shaderInfo->shaderMajorVersion > 1)
		{
			__debugbreak(); // Input texcoord registers can't be dst parameters since ps_2_0
		}
#endif
		return *(D3DXVECTOR4* const)&(inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[index]);
	case D3DSPR_RASTOUT    :
	case D3DSPR_ATTROUT    :
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
	case D3DSPR_COLOROUT   :
#ifdef _DEBUG
		if (index >= ARRAYSIZE(PS_2_0_OutputRegisters::oC) )
		{
			__debugbreak(); // Out of bounds register index
		}
#endif
		return *(D3DXVECTOR4* const)&(outputRegisters[0].oC[index]);
	case D3DSPR_CONSTINT   :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#endif
		return *(D3DXVECTOR4* const)&constantRegisters->i[index];
	case D3DSPR_DEPTHOUT   :
		return *(D3DXVECTOR4* const)&outputRegisters[0].oDepth;
	case D3DSPR_SAMPLER    :
#ifdef _DEBUG
		DbgBreakPrint("Error: Sampler register cannot be destination parameter");
#endif
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
#ifdef _DEBUG
		DbgBreakPrint("Error: Constant registers beyond 1024 are not supported, also const registers can't be used as dst registers!");
#endif
	case D3DSPR_CONSTBOOL  :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst parameters
#endif
		return *(D3DXVECTOR4* const)&constantRegisters->b[index];
	case D3DSPR_LOOP       :
#ifdef _DEBUG
		DbgBreakPrint("Error: Loop register cannot be destination parameter");
#endif
	case D3DSPR_TEMPFLOAT16:
#ifdef _DEBUG
		DbgBreakPrint("Error: TempFloat16 register cannot be destination parameter");
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_MISCTYPE   : // VPOS and VFACE
#ifdef _DEBUG
		__debugbreak(); // Special input registers can't be dst parameters
#endif
		return *(&(miscRegisters[0].vPos) + index);
	case D3DSPR_LABEL      :
#ifdef _DEBUG
		DbgBreakPrint("Error: Label register cannot be destination parameter");
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_PREDICATE  :
		return *(D3DXVECTOR4* const)&runtimeRegisters[0].p0;
	default:
#ifdef _DEBUG
		{
			DbgBreakPrint("Error: Undefined destination register type");
		}
#else
			__assume(0);
#endif
	}
	return *(D3DXVECTOR4* const)NULL;
}

const float PShaderEngine::ResolveSrcReplicateSwizzle(const DWORD rawSrcBytecode, const D3DXVECTOR4& registerData) const
{
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)&rawSrcBytecode;
	const float* const fRegisterData = (const float* const)&registerData;
	return fRegisterData[srcParameter.GetChannelSwizzle()];
}

const D3DXVECTOR4 PShaderEngine::ResolveSrcRegister(const DWORD rawSrcBytecode) const
{
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)&rawSrcBytecode;
	const D3DXVECTOR4& sourceParam = ResolveSrcParameter(rawSrcBytecode);
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
		DbgBreakPrint("Error: Undefined register source modifier");
#endif
	case D3DSPSM_NONE   :
		return ret;
	case D3DSPSM_NEG    :
		ret *= -1.0f;
		break;
	case D3DSPSM_BIAS   :
		ret.x -= 0.5f;
		ret.y -= 0.5f;
		ret.z -= 0.5f;
		ret.w -= 0.5f;
		break;
	case D3DSPSM_BIASNEG:
		ret.x -= 0.5f;
		ret.y -= 0.5f;
		ret.z -= 0.5f;
		ret.w -= 0.5f;
		ret *= -1.0f;
		break;
	case D3DSPSM_SIGN   :
		ret.x -= 0.5f;
		ret.y -= 0.5f;
		ret.z -= 0.5f;
		ret.w -= 0.5f;
		ret *= 2.0f;
		break;
	case D3DSPSM_SIGNNEG:
		ret.x -= 0.5f;
		ret.y -= 0.5f;
		ret.z -= 0.5f;
		ret.w -= 0.5f;
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

const D3DXVECTOR4& PShaderEngine::ResolveSrcParameter(const DWORD rawSrcBytecode) const
{
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)&rawSrcBytecode;
	const unsigned index = srcParameter.GetRegisterIndex();
	switch (srcParameter.GetRegisterType() )
	{
	case D3DSPR_TEMP       :
		return runtimeRegisters[0].r[index];
	case D3DSPR_INPUT      :
		return *(const D3DXVECTOR4* const)&(inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.v[index]);
	case D3DSPR_CONST      :
		return constantRegisters->c[index];
	case D3DSPR_TEXTURE       : // Also known as D3DSPR_ADDR (VS)
		return *(const D3DXVECTOR4* const)&(inputRegisters[0].ps_interpolated_inputs.ps_2_0_inputs.t[index]);
	case D3DSPR_RASTOUT    :
#ifdef _DEBUG
		DbgBreakPrint("Error: RASTOUT register is not a valid Source parameter");
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_ATTROUT    :
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
	case D3DSPR_COLOROUT   :
		return *(const D3DXVECTOR4* const)&(outputRegisters[0].oC[index]);
	case D3DSPR_CONSTINT   :
		return *(const D3DXVECTOR4* const)&constantRegisters->i[index];
	case D3DSPR_DEPTHOUT   :
		return *(const D3DXVECTOR4* const)&outputRegisters[0].oDepth;
	case D3DSPR_SAMPLER    :
		return *(const D3DXVECTOR4* const)&constantRegisters->s[index];
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
#ifdef _DEBUG
		DbgBreakPrint("Error: Constant indices beyond 1024 are not supported");
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_CONSTBOOL  :
		return *(const D3DXVECTOR4* const)&constantRegisters->b[index];
	case D3DSPR_LOOP       :
#ifdef _DEBUG
		DbgBreakPrint("Error: Loop register is not a valid source parameter");
#endif
	case D3DSPR_TEMPFLOAT16:
#ifdef _DEBUG
		DbgBreakPrint("Error: TempFloat16 register is not a valid source parameter");
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_MISCTYPE   : // VPOS and VFACE
		return *(&(miscRegisters[0].vPos) + index);
	case D3DSPR_LABEL      :
#ifdef _DEBUG
		DbgBreakPrint("Error: Label register is not a valid source parameter");
#else
		return runtimeRegisters[0].r[0];
#endif
	case D3DSPR_PREDICATE  :
		return *(const D3DXVECTOR4* const)&runtimeRegisters[0].p0;
	default:
#ifdef _DEBUG
		{
			DbgBreakPrint("Error: Undefined source parameter register type");
		}
#else
			__assume(0);
#endif
	}
	return *(const D3DXVECTOR4* const)NULL;
}

// Called once at device reset time
void PShaderEngine::GlobalInit(const PS_2_0_ConstantsBuffer* const _constantRegisters)
{
	memset(this, 0, sizeof(*this) );

	GlobalInitTex2DFunctionTable();

	miscRegisters[0].vFace.x = 1.0f;
	miscRegisters[1].vFace.x = 1.0f;
	miscRegisters[2].vFace.x = 1.0f;
	miscRegisters[3].vFace.x = 1.0f;

	constantRegisters = _constantRegisters;
}

void PShaderEngine::Init(const DeviceState& deviceState, const ShaderInfo& _shaderInfo, PS_2_0_ConstantsBuffer* const mutableConstantRegisters)
{
	shaderInfo = &_shaderInfo;

#ifdef _DEBUG
	if (!deviceState.currentPixelShader)
	{
		DbgBreakPrint("Error: Cannot run pixel shader with SetPixelShader(NULL)");
	}
#endif
	instructionPtr = _shaderInfo.firstInstructionToken;

	const DeviceState_ShaderRegisters& globalPixelShaderConstants = deviceState.pixelShaderRegisters;

	InitGlobalConstants(globalPixelShaderConstants, mutableConstantRegisters);

	// Local constants ("immediate constants") always take precedence over global constants: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205596(v=vs.85).aspx
	InitLocalConstants(_shaderInfo, mutableConstantRegisters);

	InitSamplers(deviceState, mutableConstantRegisters);
}

void PShaderEngine::InitGlobalConstants(const DeviceState_ShaderRegisters& globalPixelShaderConstants, PS_2_0_ConstantsBuffer* const mutableConstantRegisters)
{
	memcpy(&mutableConstantRegisters->c, &globalPixelShaderConstants.floats, sizeof(D3DXVECTOR4) * ARRAYSIZE(PS_2_0_ConstantsBuffer::c) );
	memcpy(&mutableConstantRegisters->b, &globalPixelShaderConstants.bools, sizeof(BOOL) * ARRAYSIZE(PS_2_0_ConstantsBuffer::b) );
	memcpy(&mutableConstantRegisters->i, &globalPixelShaderConstants.ints, sizeof(int4) * ARRAYSIZE(PS_2_0_ConstantsBuffer::i) );
}

void PShaderEngine::InitLocalConstants(const ShaderInfo& pixelShaderInfo, PS_2_0_ConstantsBuffer* const mutableConstantRegisters)
{
	// Floats
	{
		const unsigned numFloatConsts = pixelShaderInfo.initialConstantValues.size();
		for (unsigned x = 0; x < numFloatConsts; ++x)
		{
			const InitialConstantValue& constF = pixelShaderInfo.initialConstantValues[x];
			mutableConstantRegisters->c[constF.constantRegisterIndex] = constF.initialValue;
		}
	}

	// Bools
	{
		const unsigned numBoolConsts = pixelShaderInfo.initialConstantValuesB.size();
		for (unsigned x = 0; x < numBoolConsts; ++x)
		{
			const InitialConstantValueB& constB = pixelShaderInfo.initialConstantValuesB[x];
			mutableConstantRegisters->b[constB.constantRegisterIndex] = constB.initialValue;
		}
	}

	// Ints
	{
		const unsigned numIntConsts = pixelShaderInfo.initialConstantValuesI.size();
		for (unsigned x = 0; x < numIntConsts; ++x)
		{
			const InitialConstantValueI& constI = pixelShaderInfo.initialConstantValuesI[x];
			mutableConstantRegisters->i[constI.constantRegisterIndex] = constI.initialValue;
		}
	}
}

void PShaderEngine::InitSamplers(const DeviceState& deviceState, PS_2_0_ConstantsBuffer* const mutableConstantRegisters)
{
	for (unsigned x = 0; x < ARRAYSIZE(PS_2_0_ConstantsBuffer::s); ++x)
	{
		sampler& thisSampler = mutableConstantRegisters->s[x];
		thisSampler.texture = deviceState.currentTextures[x];
		thisSampler.samplerState = deviceState.currentSamplerStates[x];
	}
}

// Called once for every pixel to reset the state of the interpreter to its default
void PShaderEngine::Reset(const unsigned x, const unsigned y)
{
	instructionPtr = shaderInfo->firstInstructionToken;

	miscRegisters[0].vPos.x = (const float)x;
	miscRegisters[0].vPos.y = (const float)y;

	outputRegisters[0].pixelStatus = normalWrite;

#ifdef _DEBUG
	// These values match the default GPR values in PIX
	for (unsigned registerIndex = 0; registerIndex < ARRAYSIZE(runtimeRegisters[0].r); ++registerIndex)
	{
		D3DXVECTOR4& vec = runtimeRegisters[0].r[registerIndex];
		vec.x = 1.0f;
		vec.y = 1.0f;
		vec.z = 1.0f;
		vec.w = 1.0f;
	}
#endif
}

// Called once for every quad of pixels to reset the state of the interpreter to its default
void PShaderEngine::Reset4(const __m128i x4, const __m128i y4)
{
	instructionPtr = shaderInfo->firstInstructionToken;

	const __m128 x4f = _mm_cvtepi32_ps(x4);
	const __m128 y4f = _mm_cvtepi32_ps(y4);

	miscRegisters[0].vPos.x = x4f.m128_f32[0];
	miscRegisters[1].vPos.x = x4f.m128_f32[1];
	miscRegisters[2].vPos.x = x4f.m128_f32[2];
	miscRegisters[3].vPos.x = x4f.m128_f32[3];
	miscRegisters[0].vPos.y = y4f.m128_f32[0];
	miscRegisters[1].vPos.y = y4f.m128_f32[1];
	miscRegisters[2].vPos.y = y4f.m128_f32[2];
	miscRegisters[3].vPos.y = y4f.m128_f32[3];

	outputRegisters[0].pixelStatus = normalWrite;
	outputRegisters[1].pixelStatus = normalWrite;
	outputRegisters[2].pixelStatus = normalWrite;
	outputRegisters[3].pixelStatus = normalWrite;

#ifdef _DEBUG
	// These values match the default GPR values in PIX
	for (unsigned pixelIndex = 0; pixelIndex < 4; ++pixelIndex)
	{
		for (unsigned registerIndex = 0; registerIndex < ARRAYSIZE(runtimeRegisters[pixelIndex].r); ++registerIndex)
		{
			D3DXVECTOR4& vec = runtimeRegisters[pixelIndex].r[registerIndex];
			vec.x = 1.0f;
			vec.y = 1.0f;
			vec.z = 1.0f;
			vec.w = 1.0f;
		}
	}
#endif
}

const shaderStatus PShaderEngine::InterpreterExecStep(void)
{
	const instructionToken rawInstructionToken = *(const instructionToken* const)instructionPtr++;
	const D3DSHADER_INSTRUCTION_OPCODE_TYPE currentOpcode = (const D3DSHADER_INSTRUCTION_OPCODE_TYPE)(rawInstructionToken.opcode);
	switch (currentOpcode)
	{
	case D3DSIO_NOP         :
		nop();
		break;
	case D3DSIO_MOV         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		WriteDstParameter(dstParam, src0);
	}
		break;
	case D3DSIO_ADD         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		add(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SUB         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		sub(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_MAD         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		mad(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_MUL         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		mul(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_RCP         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src = ResolveSrcRegister(*instructionPtr);
		const float f = ResolveSrcReplicateSwizzle(*instructionPtr++, src);
		D3DXVECTOR4 dst;
		rcp(dst, f);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_RSQ         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src = ResolveSrcRegister(*instructionPtr);
		const float f = ResolveSrcReplicateSwizzle(*instructionPtr++, src);
		D3DXVECTOR4 dst;
		rsq(dst, f);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DP3         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		dp3(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DP4         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		dp4(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_MIN         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		min(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_MAX         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		max(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SLT         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		slt(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SGE         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		sge(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_EXPP        :
	case D3DSIO_EXP         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src = ResolveSrcRegister(*instructionPtr);
		const float f = ResolveSrcReplicateSwizzle(*instructionPtr++, src);
		D3DXVECTOR4 dst;
		exp(dst, f);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_LOGP        :
	case D3DSIO_LOG         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src = ResolveSrcRegister(*instructionPtr);
		const float f = ResolveSrcReplicateSwizzle(*instructionPtr++, src);
		D3DXVECTOR4 dst;
		log(dst, f);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_LIT         : // Technically this instruction is only available to vertex shaders, but I don't see the harm in executing it if one does show up in a pixel shader for some reason
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		lit(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;

		// Technically this instruction is only available to vertex shaders, but I don't see the harm in executing it if one does show up in a pixel shader for some reason
	case D3DSIO_DST         : // TODO: Validate correctness of this (the docs seem incomplete and misleading)
	{
		const DWORD destParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dest;
		dst(dest, src0, src1);
		WriteDstParameter(destParam, dest);
	}
		break;
	case D3DSIO_LRP         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		lrp(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_FRC         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		frc(dst, src);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_M4x4        :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = *(&src1 + 1);
		const D3DXVECTOR4 src3 = *(&src1 + 2);
		const D3DXVECTOR4 src4 = *(&src1 + 3);
		D3DXVECTOR4 dst;
		m4x4(dst, src0, src1, src2, src3, src4);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_M4x3        :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = *(&src1 + 1);
		const D3DXVECTOR4 src3 = *(&src1 + 2);
		D3DXVECTOR4 dst;
		m4x3(dst, src0, src1, src2, src3);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_M3x4        :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = *(&src1 + 1);
		const D3DXVECTOR4 src3 = *(&src1 + 2);
		const D3DXVECTOR4 src4 = *(&src1 + 3);
		D3DXVECTOR4 dst;
		m3x4(dst, src0, src1, src2, src3, src4);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_M3x3        :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = *(&src1 + 1);
		const D3DXVECTOR4 src3 = *(&src1 + 2);
		D3DXVECTOR4 dst;
		m3x3(dst, src0, src1, src2, src3);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_M3x2        :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = *(&src1 + 1);
		D3DXVECTOR4 dst;
		m3x2(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_CALL        :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: CALL Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_CALLNZ      :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: CALLNZ Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_LOOP        :
	{
		const DWORD aLDst = *instructionPtr++;
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: LOOP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_RET         :
		DbgBreakPrint("Error: RET Shader function not yet implemented!"); // Not yet implemented!
		return normalCompletion;
	case D3DSIO_ENDLOOP     :
		DbgBreakPrint("Error: ENDLOOP Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_LABEL       :
	{
		// src0 contains the label index for this label
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		
		// Do nothing
	}
		break;
	case D3DSIO_DCL         :
		DbgBreakPrint("Error: DCL Should not be encountered during normal shader execution!");
		break;
	case D3DSIO_POW         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr);
		const float f0 = ResolveSrcReplicateSwizzle(*instructionPtr++, src0);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr);
		const float f1 = ResolveSrcReplicateSwizzle(*instructionPtr++, src1);
		D3DXVECTOR4 dst;
		pow(dst, f0, f1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_CRS         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		crs(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SGN         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1_unused = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2_unused = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		sgn(dst, src0, src1_unused, src2_unused);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_ABS         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		abs(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_NRM         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		nrm(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SINCOS      :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr);
		const srcParameterToken& srcParameter = *(const srcParameterToken* const)instructionPtr;
		const unsigned swizzleX = srcParameter.GetChannelSwizzleXYZW() & 0x3;
		const unsigned swizzleY = (srcParameter.GetChannelSwizzleXYZW() >> 2) & 0x3;
		const float f = ResolveSrcReplicateSwizzle(*instructionPtr++, src0);
		if (shaderInfo->shaderMajorVersion < 3) // Shader model 2 has these extra registers, but all the shader model 3+ don't have them
		{
			const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
			const D3DXVECTOR4 src2 = ResolveSrcRegister(*instructionPtr++);
		}
		D3DXVECTOR4 dst;
		if (swizzleX && swizzleY)
			sincos_sc(dst, f);
		else if (swizzleX)
			sincos_c(dst, f);
		else if (swizzleY)
			sincos_s(dst, f);
#ifdef _DEBUG
		else
		{
			// Whyyyyyyyyyyyyyyyy?
			DbgBreakPrint("Error: SINCOS function swizzle is undefined");
		}
#endif
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_REP         :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: REP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_ENDREP      :
		DbgBreakPrint("Error: ENDREP Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_IF          :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: IF Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_IFC         :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: IFC Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_ELSE        :
		DbgBreakPrint("Error: ELSE Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_ENDIF       :
		DbgBreakPrint("Error: ENDIF Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_BREAK       :
		DbgBreakPrint("Error: BREAK Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_BREAKC      :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: BREAKC Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_MOVA        : // This is a vs-only instruction
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: MOVA Instruction not available to pixel shaders!");
	}
		break;
	case D3DSIO_DEFB        :
		DbgBreakPrint("Error: DEFB Should not be encountered during normal shader execution!");
		break;
	case D3DSIO_DEFI        :
		DbgBreakPrint("Error: DEFI Should not be encountered during normal shader execution!");
		break;		
	case D3DSIO_TEXCOORD    :
	{
		const DWORD dstParam = *instructionPtr++;
		DbgBreakPrint("Error: TEXCOORD Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXKILL     :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4& dstValue = ResolveDstParameter(dstParam);
		const int4* const i4Value = (const int4* const)&dstValue;
		if ( (i4Value->x | i4Value->y | i4Value->z) < 0)
		{
			outputRegisters[0].pixelStatus = discard;
			return texkillStatus;
		}
		else
		{
			;
		}
	}
		break;
	case D3DSIO_TEX         : // Standard texture sampling with tex2D(), this is the same instruction as tex3Dproj/tex2Dproj/texldp and tex3Dbias/tex2Dbias/texldb
		// This instruction has three different source register counts (zero for ps_1_0 thru ps_1_3, one for ps_1_4, and two for shader model 2 and up)
	{
		const DWORD dstParam = *instructionPtr++;
		D3DXVECTOR4 src0;
		if (shaderInfo->shaderMajorVersion > 1 ||
			(shaderInfo->shaderMajorVersion == 1 && shaderInfo->shaderMinorVersion > 3) )
		{
			src0 = ResolveSrcRegister(*instructionPtr++);
		}
		const sampler* samplerPtr;
		if (shaderInfo->shaderMajorVersion > 1)
		{
			const D3DXVECTOR4& src1 = ResolveSrcParameter(*instructionPtr++);
			samplerPtr = (const sampler* const)&src1;
		}
		else
		{
			const dstParameterToken* const dstToken = (const dstParameterToken* const)&dstParam;

			if (shaderInfo->shaderMinorVersion < 4)
			{
				srcParameterToken newSrc0TexcoordToken;
				newSrc0TexcoordToken.internalRawToken = 0x00000000;
				newSrc0TexcoordToken.srcParameter.registerIndex = dstToken->GetRegisterIndex();
				newSrc0TexcoordToken.srcParameter.registerType_lowBits = D3DSPR_TEXTURE;
				newSrc0TexcoordToken.srcParameter.instructionParameterBit = parameterTokenMarker;
				newSrc0TexcoordToken.srcParameter.sourceSwizzle.fullSwizzle = _NoSwizzleXYZW;
				src0 = ResolveSrcRegister(newSrc0TexcoordToken.internalRawToken);
			}

			srcParameterToken newSrc1SamplerToken;
			newSrc1SamplerToken.internalRawToken = 0x00000000;
			newSrc1SamplerToken.srcParameter.registerIndex = dstToken->GetRegisterIndex();
			newSrc1SamplerToken.srcParameter.registerType_lowBits = D3DSPR_SAMPLER & 0x7;
			newSrc1SamplerToken.srcParameter.registerType_highBits = (D3DSPR_SAMPLER >> 3);
			newSrc1SamplerToken.srcParameter.instructionParameterBit = parameterTokenMarker;
			newSrc1SamplerToken.srcParameter.sourceSwizzle.fullSwizzle = _NoSwizzleXYZW;
			const D3DXVECTOR4& src1 = ResolveSrcParameter(newSrc1SamplerToken.internalRawToken);
			samplerPtr = (const sampler* const)&src1;
		}
		D3DXVECTOR4 dst;
		if (rawInstructionToken.opcodeControls == OpCtrl_TexLd_Bias) // tex2Dbias/texldb variant
		{
			// TODO: Calculate gradient on texcoords and pass along for mip-level computation
			const D3DXVECTOR4 grad_ddx_ddy(0.0f, 0.0f, 0.0f, 0.0f);
			tex2Dgrad<0xF, true>(dst, src0, grad_ddx_ddy, grad_ddx_ddy, samplerPtr);

			// TODO: src1 can have a swizzle that gets applied after the texture sample but before the write mask happens
		}
		else
		{
			if (rawInstructionToken.opcodeControls == OpCtrl_TexLd_Project) // tex2Dproj or texldp instruction variant has the texcoord divided by the texcoord.w component
			{
				const float invW = 1.0f / src0.w;
				src0.x *= invW;
				src0.y *= invW;
				src0.z *= invW;
				src0.w = 1.0f;
			}

			// TODO: Calculate gradient on texcoords and pass along for mip-level computation
			const D3DXVECTOR4 grad_ddx_ddy(0.0f, 0.0f, 0.0f, 0.0f);
			tex2Dgrad<0xF, false>(dst, src0, grad_ddx_ddy, grad_ddx_ddy, samplerPtr);

			// TODO: src1 can have a swizzle that gets applied after the texture sample but before the write mask happens
		}

		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_TEXBEM      :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXBEML     :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXBEML Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXREG2AR   :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXREG2AR Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXREG2GB   :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXREG2GB Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXM3x2PAD  :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXM3x2PAD Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXM3x2TEX  :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXM3x2TEX Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXM3x3PAD  :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXM3x3PAD Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXM3x3TEX  :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXM3x3TEX Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_RESERVED0   :
		DbgBreakPrint("Error: RESERVED0 Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_TEXM3x3SPEC :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXM3x3SPEC Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXM3x3VSPEC:
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXM3x3VSPEC Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_CND         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		cnd(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DEF         :
		DbgBreakPrint("Error: DEF Should not be encountered during normal shader execution!");
		break;
	case D3DSIO_TEXREG2RGB  :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXREG2RGB Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXDP3TEX   : // This is pretty complicated: https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/texdp3tex---ps
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		srcParameterToken srcDst;
		srcDst.internalRawToken = 0x00000000;
		srcDst.srcParameter.instructionParameterBit = parameterTokenMarker;
		srcDst.srcParameter.registerIndex = dstParam.GetRegisterIndex();
		srcDst.srcParameter.registerType_lowBits = D3DSPR_TEXTURE;
		srcDst.srcParameter.sourceSwizzle.fullSwizzle = _NoSwizzleXYZW;
#ifdef _DEBUG
		if (dstParam.GetRegisterType() != D3DSPR_TEXTURE)
		{
			__debugbreak(); // Unexpected src/dst register type
		}
#endif
		const D3DXVECTOR4 texCoord = ResolveSrcRegister(srcDst.internalRawToken);

		srcParameterToken samplerSrc = srcDst;
		samplerSrc.srcParameter.registerType_lowBits = (D3DSPR_SAMPLER & 0x7);
		samplerSrc.srcParameter.registerType_highBits = ( (D3DSPR_SAMPLER >> 3) & 0x3);
		const D3DXVECTOR4& samplerTemp = ResolveSrcParameter(samplerSrc.internalRawToken);
		const sampler* const samplerPtr = (const sampler* const)&samplerTemp;

		// TODO: Calculate gradient on texcoords and pass along for mip-level computation
		const D3DXVECTOR4 grad_ddx_ddy(0.0f, 0.0f, 0.0f, 0.0f);
		float rampTexcoord;
		dp3(rampTexcoord, texCoord, src0);

		D3DXVECTOR4 tempDst;
		D3DXVECTOR4 rampTexcoord4(rampTexcoord, 0.0f, 0.0f, 0.0f);
		tex2Dgrad<0xF, false>(tempDst, rampTexcoord4, grad_ddx_ddy, grad_ddx_ddy, samplerPtr);

		WriteDstParameter(dstParam.internalRawToken, tempDst);
	}
		break;
	case D3DSIO_TEXM3x2DEPTH:
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXM3x2DEPTH Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXDP3      :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXDP3 Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXM3x3     :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: TEXM3x3 Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXDEPTH    :
	{
		const DWORD dstParam = *instructionPtr++;
		DbgBreakPrint("Error: TEXDEPTH Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_CMP         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		cmp(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_BEM         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: BEM Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_DP2ADD      :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;
		dp2add(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DSX         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;

		// Derivative is always zero for one-pixel warps (need at least a 2x2 quad warp to compute this)
		dsx(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DSY         :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		D3DXVECTOR4 dst;

		// Derivative is always zero for one-pixel warps (need at least a 2x2 quad warp to compute this)
		dsy(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_TEXLDD      : // tex2Dgrad()
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4& src1 = ResolveSrcParameter(*instructionPtr++);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src3 = ResolveSrcRegister(*instructionPtr++);
		const sampler* const samplerPtr = (const sampler* const)&src1;
		D3DXVECTOR4 dst;
		tex2Dgrad<0xF, false>(dst, src0, src2, src3, samplerPtr);

		// TODO: src1 can have a swizzle that gets applied after the texture sample but before the write mask happens

		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SETP        :
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: SETP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXLDL      : // tex2Dlod()
	{
		const DWORD dstParam = *instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		const D3DXVECTOR4& src1 = ResolveSrcParameter(*instructionPtr++);
		const sampler* const samplerPtr = (const sampler* const)&src1;
		D3DXVECTOR4 dst;
		tex2Dlod<0xF>(dst, src0, samplerPtr);

		// TODO: src1 can have a swizzle that gets applied after the texture sample but before the write mask happens

		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_BREAKP      :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(*instructionPtr++);
		DbgBreakPrint("Error: BREAKP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;		
	case D3DSIO_PHASE       :
		// Just do nothing, treat the whole shader as if it were a ps_2_0+ shader instead of a ps_1_4 shader
		break;
	case D3DSIO_COMMENT     :
	{
		const unsigned commentLengthDWORDs = (instructionPtr[-1] & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
		instructionPtr += commentLengthDWORDs; // Skip the comment block
	}
		break;
	case D3DSIO_END         :
		// We're done! We reached the end of the shader!
		outputRegisters[0].pixelStatus = normalWrite;
		return normalCompletion;
	default:
#ifdef _DEBUG
	{
		DbgBreakPrint("Error: Undefined shader opcode");
	}
#else
		__assume(0);
#endif
	}

	// Keep executing the shader
	return shouldContinueStatus;
}

void PShaderEngine::InterpreterExecutePixel(void)
{
	// Execute the shader:
	shaderStatus currentStatus = shouldContinueStatus;
	while (currentStatus == shouldContinueStatus)
	{
		currentStatus = InterpreterExecStep();
	}

	switch (currentStatus)
	{
	case shouldContinueStatus:
#ifdef _DEBUG
		DbgBreakPrint("Error: Pixel shader exited without finishing");
#endif
		return;
	case errorStatus:
#ifdef _DEBUG
		DbgBreakPrint("Error: Pixel shader exited with error");
#endif
		return;
	case texkillStatus:
		return;
	default:
#ifdef _DEBUG
		DbgBreakPrint("Error: Pixel shader exited with unknown status");
#endif
	case normalCompletion:
		// Do ps_1_* output register fixup (because ps_1_* outputs with register r0 instead of into one of the oC[N] registers):
		if (shaderInfo->shaderMajorVersion < 2)
		{
			outputRegisters[0].oC[0] = *(const float4* const)&runtimeRegisters[0].r[0];
		}
		return;
	}
}
