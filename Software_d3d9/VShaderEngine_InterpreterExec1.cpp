#pragma once

#include "VShaderEngine.h"
#include "IDirect3DDevice9Hook.h"
#include "IDirect3DVertexShader9Hook.h"

const bool VShaderEngine::InterpreterExecStep1()
{
#ifdef _DEBUG
	if (!instructionPtr)
	{
		__debugbreak();
	}
#endif

	const instructionToken rawInstructionToken = *(const instructionToken* const)instructionPtr++;
	const D3DSHADER_INSTRUCTION_OPCODE_TYPE currentInstructionOpcode = (const D3DSHADER_INSTRUCTION_OPCODE_TYPE)(rawInstructionToken.opcode);
	switch (currentInstructionOpcode)
	{
	case D3DSIO_NOP         :
		nop();
		break;
	case D3DSIO_MOV         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		WriteDstParameter(dstParam, src0);
	}
		break;
	case D3DSIO_ADD         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		add(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SUB         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		sub(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_MAD         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		mad(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_MUL         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		mul(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_RCP         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD srcBytecode = *instructionPtr;
		const D3DXVECTOR4 src = ResolveSrcRegister(instructionPtr);
		const float f = ResolveSrcReplicateSwizzle(srcBytecode, src);
		D3DXVECTOR4 dst;
		rcp(dst, f);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_RSQ         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD srcBytecode = *instructionPtr;
		const D3DXVECTOR4 src = ResolveSrcRegister(instructionPtr);
		const float f = ResolveSrcReplicateSwizzle(srcBytecode, src);
		D3DXVECTOR4 dst;
		rsq(dst, f);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DP3         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		dp3(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DP4         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		dp4(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_MIN         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		min(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_MAX         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		max(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SLT         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		slt(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SGE         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		sge(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_EXPP        :
	case D3DSIO_EXP         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD srcBytecode = *instructionPtr;
		const D3DXVECTOR4 src = ResolveSrcRegister(instructionPtr);
		const float f = ResolveSrcReplicateSwizzle(srcBytecode, src);
		D3DXVECTOR4 dst;
		exp(dst, f);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_LOGP        :
	case D3DSIO_LOG         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD srcBytecode = *instructionPtr;
		const D3DXVECTOR4 src = ResolveSrcRegister(instructionPtr);
		const float f = ResolveSrcReplicateSwizzle(srcBytecode, src);
		D3DXVECTOR4 dst;
		log(dst, f);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_LIT         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		lit(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DST         : // TODO: Validate correctness of this (the docs seem incomplete and misleading)
	{
		const dstParameterToken destParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dest;
		dst(dest, src0, src1);
		WriteDstParameter(destParam, dest);
	}
		break;
	case D3DSIO_LRP         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		lrp(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_FRC         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		frc(dst, src);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_M4x4        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
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
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2 = *(&src1 + 1);
		const D3DXVECTOR4 src3 = *(&src1 + 2);
		D3DXVECTOR4 dst;
		m4x3(dst, src0, src1, src2, src3);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_M3x4        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
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
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2 = *(&src1 + 1);
		const D3DXVECTOR4 src3 = *(&src1 + 2);
		D3DXVECTOR4 dst;
		m3x3(dst, src0, src1, src2, src3);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_M3x2        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2 = *(&src1 + 1);
		D3DXVECTOR4 dst;
		m3x2(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_CALL        :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: CALL Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_CALLNZ      :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: CALLNZ Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_LOOP        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: LOOP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_RET         :
		DbgBreakPrint("Error: RET Shader function not yet implemented!"); // Not yet implemented!
		return false;
	case D3DSIO_ENDLOOP     :
		DbgBreakPrint("Error: ENDLOOP Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_LABEL       :
	{
		// src0 contains the label index for this label
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		
		// Do nothing
	}
		break;
	case D3DSIO_DCL         :
		DbgBreakPrint("Error: DCL Should not be encountered during normal shader execution!");
		break;
	case D3DSIO_POW         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD src0Bytecode = *instructionPtr;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const float f0 = ResolveSrcReplicateSwizzle(src0Bytecode, src0);
		const DWORD src1Bytecode = *instructionPtr;
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const float f1 = ResolveSrcReplicateSwizzle(src1Bytecode, src1);
		D3DXVECTOR4 dst;
		pow(dst, f0, f1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_CRS         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		crs(dst, src0, src1);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SGN         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1_unused = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2_unused = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		sgn(dst, src0, src1_unused, src2_unused);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_ABS         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		abs(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_NRM         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		nrm(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_SINCOS      :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD src0Bytecode = *instructionPtr;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const srcParameterToken& srcParameter = *(const srcParameterToken* const)instructionPtr;
		const unsigned swizzleX = srcParameter.GetChannelSwizzleXYZW() & 0x3;
		const unsigned swizzleY = (srcParameter.GetChannelSwizzleXYZW() >> 2) & 0x3;
		const float f = ResolveSrcReplicateSwizzle(src0Bytecode, src0);
		if (shaderInfo->shaderMajorVersion < 3) // Shader model 2 has these extra registers, but all the shader model 3+ don't have them
		{
			const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
			const D3DXVECTOR4 src2 = ResolveSrcRegister(instructionPtr);
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
			__debugbreak();
		}
#endif
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_REP         :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: REP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_ENDREP      :
		DbgBreakPrint("Error: ENDREP Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_IF          :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: IF Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_IFC         :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
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
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: BREAKC Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_MOVA        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
#ifdef _DEBUG
		if (dstParam.GetRegisterType() != D3DSPR_ADDR)
		{
			__debugbreak(); // This instruction is only allowed to move into the address register (a0 register)
		}
#endif
		WriteDstParameter(dstParam, src0);
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
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		DbgBreakPrint("Error: TEXCOORD Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_TEXKILL     :
		DbgBreakPrint("Error: Should never hit TEXKILL inside a vertex shader!");
		__debugbreak();
		break;
	case D3DSIO_TEXBEM      :
		DbgBreakPrint("Error: TEXBEM Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXBEML     :
		DbgBreakPrint("Error: TEXBEML Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXREG2AR   :
		DbgBreakPrint("Error: TEXREG2AR Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXREG2GB   :
		DbgBreakPrint("Error: TEXREG2GB Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXM3x2PAD  :
		DbgBreakPrint("Error: TEXM3x2PAD Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXM3x2TEX  :
		DbgBreakPrint("Error: TEXM3x2TEX Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXM3x3PAD  :
		DbgBreakPrint("Error: TEXM3x3PAD Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXM3x3TEX  :
		DbgBreakPrint("Error: TEXM3x3TEX Shader function not available in vertex shaders!");
		break;
	case D3DSIO_RESERVED0   :
		DbgBreakPrint("Error: RESERVED0 Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXM3x3SPEC :
		DbgBreakPrint("Error: TEXM3x3SPEC Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXM3x3VSPEC:
		DbgBreakPrint("Error: TEXM3x3VSPEC Shader function not available in vertex shaders!");
		break;
	case D3DSIO_CND         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		cnd(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DEF         :
		DbgBreakPrint("Error: DEF Should not be encountered during normal shader execution!"); // Not yet implemented!
		break;
	case D3DSIO_TEXREG2RGB  :
		DbgBreakPrint("Error: TEXREG2RGB Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXDP3TEX   :
		DbgBreakPrint("Error: TEXDP3TEX Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXM3x2DEPTH:
		DbgBreakPrint("Error: TEXM3x2DEPTH Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXDP3      :
		DbgBreakPrint("Error: TEXDP3 Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXM3x3     :
		DbgBreakPrint("Error: TEXM3x3 Shader function not available in vertex shaders!");
		break;
	case D3DSIO_TEXDEPTH    :
		DbgBreakPrint("Error: TEXDEPTH Shader function not available in vertex shaders!");
		break;
	case D3DSIO_CMP         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		cmp(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_BEM         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: BEM Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_DP2ADD      :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src2 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;
		dp2add(dst, src0, src1, src2);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DSX         : // Technically these derivative/gradient instructions aren't supposed to be runnable by vertex shaders, but it won't break anything if we do
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;

		// Derivative is always zero for one-vertex warps (need at least a 2x2 quad warp to compute this)
		dsx(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_DSY         : // Technically these derivative/gradient instructions aren't supposed to be runnable by vertex shaders, but it won't break anything if we do
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		D3DXVECTOR4 dst;

		// Derivative is always zero for one-vertex warps (need at least a 2x2 quad warp to compute this)
		dsy(dst, src0);
		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_TEXLDD      : // tex2Dgrad()
		DbgBreakPrint("Error: TEXLDD Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_SETP        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const D3DXVECTOR4 src1 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: SETP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;

		// Technically tex2D() is not allowed in vertex shaders, however I don't see anything wrong with promoting it to tex2Dlod and just letting the shader run
	case D3DSIO_TEX         : // Standard texture sampling with tex2D() for shader model 2 and up
	case D3DSIO_TEXLDL      : // tex2Dlod()
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		const srcParameterToken& src1Param = *(const srcParameterToken* const)instructionPtr++;
		const D3DXVECTOR4& src1 = ResolveSrcParameter(src1Param);
		const sampler* const samplerPtr = (const sampler* const)&src1;
		D3DXVECTOR4 dst;

		tex2Dlod<0xF>(dst, src0, samplerPtr);

		// TODO: src1 can have a swizzle that gets applied after the texture sample but before the write mask happens

		WriteDstParameter(dstParam, dst);
	}
		break;
	case D3DSIO_BREAKP      :
	{
		const D3DXVECTOR4 src0 = ResolveSrcRegister(instructionPtr);
		DbgBreakPrint("Error: BREAKP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;		
	case D3DSIO_PHASE       :
		DbgBreakPrint("Error: Should never hit PHASE instruction in vertex shader!");
		break;
	case D3DSIO_COMMENT     :
	{
		const unsigned commentLengthDWORDs = (instructionPtr[-1] & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
		instructionPtr += commentLengthDWORDs; // Skip the comment block
	}
		break;
	case D3DSIO_END         :
		// We're done! We reached the end of the shader!
		return false;
	default:
#ifdef _DEBUG
	{
		__debugbreak();
	}
#else
		__assume(0);
#endif
	}

	return true;
}
