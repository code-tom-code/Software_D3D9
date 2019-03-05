#pragma once

#include "VShaderEngine.h"
#include "IDirect3DDevice9Hook.h"
#include "IDirect3DVertexShader9Hook.h"

#ifdef RUN_SHADERS_IN_WARPS

static const D3DXVECTOR4 negHalfVec(-0.5f, -0.5f, -0.5f, -0.5f);

void VShaderEngine::ResolveSrcReplicateSwizzle4(const DWORD rawSrcBytecode, const D3DXVECTOR4 (&registerData)[4], float (&outReplicateValue)[4]) const
{
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)&rawSrcBytecode;
	const float* const fRegisterData4[4] = 
	{
		&registerData[0].x,
		&registerData[1].x,
		&registerData[2].x,
		&registerData[3].x
	};
	const unsigned char replicateSwizzleChannel = srcParameter.GetChannelSwizzle();
	outReplicateValue[0] = fRegisterData4[0][replicateSwizzleChannel];
	outReplicateValue[1] = fRegisterData4[1][replicateSwizzleChannel];
	outReplicateValue[2] = fRegisterData4[2][replicateSwizzleChannel];
	outReplicateValue[3] = fRegisterData4[3][replicateSwizzleChannel];
}

// TODO: vs_3_0 introduced the concept of dst relative addressing (for output registers), we need to account for this somehow
void VShaderEngine::WriteDstParameter4(const dstParameterToken dstParameter, const D3DXVECTOR4 (&value4)[4])
{
	D3DXVECTOR4* out4[4];
	ResolveDstParameter4(dstParameter, out4);
	const unsigned writeMask = dstParameter.GetWriteMask();
	if (dstParameter.GetResultModifierUnshifted() & D3DSPDM_SATURATE) // Saturate
	{
		switch (writeMask)
		{
		case 0:
#ifdef _DEBUG
			__debugbreak(); // Should never be here
#endif
			break;
		case 0x1:
			out4[0]->x = saturate(value4[0].x);
			out4[1]->x = saturate(value4[1].x);
			out4[2]->x = saturate(value4[2].x);
			out4[3]->x = saturate(value4[3].x);
			break;
		case 0x3:
			out4[0]->x = saturate(value4[0].x);
			out4[1]->x = saturate(value4[1].x);
			out4[2]->x = saturate(value4[2].x);
			out4[3]->x = saturate(value4[3].x);
			// Intentional fallthrough
		case 0x2:
			out4[0]->y = saturate(value4[0].y);
			out4[1]->y = saturate(value4[1].y);
			out4[2]->y = saturate(value4[2].y);
			out4[3]->y = saturate(value4[3].y);
			break;
		case 0x7:
			out4[0]->x = saturate(value4[0].x);
			out4[1]->x = saturate(value4[1].x);
			out4[2]->x = saturate(value4[2].x);
			out4[3]->x = saturate(value4[3].x);
			// Intentional fallthrough
		case 0x6:
			out4[0]->y = saturate(value4[0].y);
			out4[1]->y = saturate(value4[1].y);
			out4[2]->y = saturate(value4[2].y);
			out4[3]->y = saturate(value4[3].y);
			// Intentional fallthrough
		case 0x4:
			out4[0]->z = saturate(value4[0].z);
			out4[1]->z = saturate(value4[1].z);
			out4[2]->z = saturate(value4[2].z);
			out4[3]->z = saturate(value4[3].z);
			break;
		case 0x5:
			out4[0]->x = saturate(value4[0].x);
			out4[1]->x = saturate(value4[1].x);
			out4[2]->x = saturate(value4[2].x);
			out4[3]->x = saturate(value4[3].x);
			out4[0]->z = saturate(value4[0].z);
			out4[1]->z = saturate(value4[1].z);
			out4[2]->z = saturate(value4[2].z);
			out4[3]->z = saturate(value4[3].z);
			break;
		default:
#ifdef _DEBUG
			__debugbreak();
#endif
		case 0xF: // The most common case, all bits are set!
			out4[0]->x = saturate(value4[0].x);
			out4[1]->x = saturate(value4[1].x);
			out4[2]->x = saturate(value4[2].x);
			out4[3]->x = saturate(value4[3].x);
			// Intentional fallthrough
		case 0xE: // 14
			out4[0]->y = saturate(value4[0].y);
			out4[1]->y = saturate(value4[1].y);
			out4[2]->y = saturate(value4[2].y);
			out4[3]->y = saturate(value4[3].y);
			// Intentional fallthrough
		case 0xC: // 12
			out4[0]->z = saturate(value4[0].z);
			out4[1]->z = saturate(value4[1].z);
			out4[2]->z = saturate(value4[2].z);
			out4[3]->z = saturate(value4[3].z);
			// Intentional fallthrough
		case 0x8:
			out4[0]->w = saturate(value4[0].w);
			out4[1]->w = saturate(value4[1].w);
			out4[2]->w = saturate(value4[2].w);
			out4[3]->w = saturate(value4[3].w);
			break;
		case 0x9:
			out4[0]->x = saturate(value4[0].x);
			out4[1]->x = saturate(value4[1].x);
			out4[2]->x = saturate(value4[2].x);
			out4[3]->x = saturate(value4[3].x);
			out4[0]->w = saturate(value4[0].w);
			out4[1]->w = saturate(value4[1].w);
			out4[2]->w = saturate(value4[2].w);
			out4[3]->w = saturate(value4[3].w);
			break;
		case 0xA: // 10
			out4[0]->w = saturate(value4[0].y);
			out4[1]->w = saturate(value4[1].y);
			out4[2]->w = saturate(value4[2].y);
			out4[3]->w = saturate(value4[3].y);
			break;
		case 0xB: // 11
			out4[0]->x = saturate(value4[0].x);
			out4[1]->x = saturate(value4[1].x);
			out4[2]->x = saturate(value4[2].x);
			out4[3]->x = saturate(value4[3].x);
			out4[0]->y = saturate(value4[0].y);
			out4[1]->y = saturate(value4[1].y);
			out4[2]->y = saturate(value4[2].y);
			out4[3]->y = saturate(value4[3].y);
			out4[0]->w = saturate(value4[0].w);
			out4[1]->w = saturate(value4[1].w);
			out4[2]->w = saturate(value4[2].w);
			out4[3]->w = saturate(value4[3].w);
			break;
		case 0xD: // 13
			out4[0]->x = saturate(value4[0].x);
			out4[1]->x = saturate(value4[1].x);
			out4[2]->x = saturate(value4[2].x);
			out4[3]->x = saturate(value4[3].x);
			out4[0]->z = saturate(value4[0].z);
			out4[1]->z = saturate(value4[1].z);
			out4[2]->z = saturate(value4[2].z);
			out4[3]->z = saturate(value4[3].z);
			out4[0]->w = saturate(value4[0].w);
			out4[1]->w = saturate(value4[1].w);
			out4[2]->w = saturate(value4[2].w);
			out4[3]->w = saturate(value4[3].w);
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
			out4[0]->x = value4[0].x;
			out4[1]->x = value4[1].x;
			out4[2]->x = value4[2].x;
			out4[3]->x = value4[3].x;
			break;
		case 0x3:
			out4[0]->x = value4[0].x;
			out4[1]->x = value4[1].x;
			out4[2]->x = value4[2].x;
			out4[3]->x = value4[3].x;
			// Intentional fallthrough
		case 0x2:
			out4[0]->y = value4[0].y;
			out4[1]->y = value4[1].y;
			out4[2]->y = value4[2].y;
			out4[3]->y = value4[3].y;
			break;
		case 0x7:
			out4[0]->x = value4[0].x;
			out4[1]->x = value4[1].x;
			out4[2]->x = value4[2].x;
			out4[3]->x = value4[3].x;
			// Intentional fallthrough
		case 0x6:
			out4[0]->y = value4[0].y;
			out4[1]->y = value4[1].y;
			out4[2]->y = value4[2].y;
			out4[3]->y = value4[3].y;
			// Intentional fallthrough
		case 0x4:
			out4[0]->z = value4[0].z;
			out4[1]->z = value4[1].z;
			out4[2]->z = value4[2].z;
			out4[3]->z = value4[3].z;
			break;
		case 0x5:
			out4[0]->x = value4[0].x;
			out4[1]->x = value4[1].x;
			out4[2]->x = value4[2].x;
			out4[3]->x = value4[3].x;
			out4[0]->z = value4[0].z;
			out4[1]->z = value4[1].z;
			out4[2]->z = value4[2].z;
			out4[3]->z = value4[3].z;
			break;
		default:
#ifdef _DEBUG
			__debugbreak();
#endif
		case 0xF: // The most common case, all bits are set!
			out4[0]->x = value4[0].x;
			out4[1]->x = value4[1].x;
			out4[2]->x = value4[2].x;
			out4[3]->x = value4[3].x;
			// Intentional fallthrough
		case 0xE: // 14
			out4[0]->y = value4[0].y;
			out4[1]->y = value4[1].y;
			out4[2]->y = value4[2].y;
			out4[3]->y = value4[3].y;
			// Intentional fallthrough
		case 0xC: // 12
			out4[0]->z = value4[0].z;
			out4[1]->z = value4[1].z;
			out4[2]->z = value4[2].z;
			out4[3]->z = value4[3].z;
			// Intentional fallthrough
		case 0x8:
			out4[0]->w = value4[0].w;
			out4[1]->w = value4[1].w;
			out4[2]->w = value4[2].w;
			out4[3]->w = value4[3].w;
			break;
		case 0x9:
			out4[0]->x = value4[0].x;
			out4[1]->x = value4[1].x;
			out4[2]->x = value4[2].x;
			out4[3]->x = value4[3].x;
			out4[0]->w = value4[0].w;
			out4[1]->w = value4[1].w;
			out4[2]->w = value4[2].w;
			out4[3]->w = value4[3].w;
			break;
		case 0xA: // 10
			out4[0]->y = value4[0].y;
			out4[1]->y = value4[1].y;
			out4[2]->y = value4[2].y;
			out4[3]->y = value4[3].y;
			out4[0]->w = value4[0].w;
			out4[1]->w = value4[1].w;
			out4[2]->w = value4[2].w;
			out4[3]->w = value4[3].w;
			break;
		case 0xB: // 11
			out4[0]->x = value4[0].x;
			out4[1]->x = value4[1].x;
			out4[2]->x = value4[2].x;
			out4[3]->x = value4[3].x;
			out4[0]->y = value4[0].y;
			out4[1]->y = value4[1].y;
			out4[2]->y = value4[2].y;
			out4[3]->y = value4[3].y;
			out4[0]->w = value4[0].w;
			out4[1]->w = value4[1].w;
			out4[2]->w = value4[2].w;
			out4[3]->w = value4[3].w;
			break;
		case 0xD: // 13
			out4[0]->x = value4[0].x;
			out4[1]->x = value4[1].x;
			out4[2]->x = value4[2].x;
			out4[3]->x = value4[3].x;
			out4[0]->z = value4[0].z;
			out4[1]->z = value4[1].z;
			out4[2]->z = value4[2].z;
			out4[3]->z = value4[3].z;
			out4[0]->w = value4[0].w;
			out4[1]->w = value4[1].w;
			out4[2]->w = value4[2].w;
			out4[3]->w = value4[3].w;
			break;
		}
	}
}

template <const bool isVS3_0>
void VShaderEngine::ResolveDstParameterVS4(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned index, D3DXVECTOR4* (&outDstRegisters)[4])
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
		outDstRegisters[0] = &runtimeRegisters[0].r[index];
		outDstRegisters[1] = &runtimeRegisters[1].r[index];
		outDstRegisters[2] = &runtimeRegisters[2].r[index];
		outDstRegisters[3] = &runtimeRegisters[3].r[index];
		return;
	case D3DSPR_INPUT      :
#ifdef _DEBUG
		__debugbreak(); // input registers can't be dst registers
#endif
		outDstRegisters[0] = &inputRegisters[0].v[index];
		outDstRegisters[1] = &inputRegisters[1].v[index];
		outDstRegisters[2] = &inputRegisters[2].v[index];
		outDstRegisters[3] = &inputRegisters[3].v[index];
		return;
	case D3DSPR_CONST      :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#endif
		outDstRegisters[0] = const_cast<D3DXVECTOR4* const>(&constantRegisters->c[index]);
		outDstRegisters[1] = const_cast<D3DXVECTOR4* const>(&constantRegisters->c[index]);
		outDstRegisters[2] = const_cast<D3DXVECTOR4* const>(&constantRegisters->c[index]);
		outDstRegisters[3] = const_cast<D3DXVECTOR4* const>(&constantRegisters->c[index]);
		return;
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		outDstRegisters[0] = (D3DXVECTOR4* const)&runtimeRegisters[0].a;
		outDstRegisters[1] = (D3DXVECTOR4* const)&runtimeRegisters[1].a;
		outDstRegisters[2] = (D3DXVECTOR4* const)&runtimeRegisters[2].a;
		outDstRegisters[3] = (D3DXVECTOR4* const)&runtimeRegisters[3].a;
		return;
	case D3DSPR_RASTOUT    :
#ifdef _DEBUG
		if (index >= 3)
		{
			__debugbreak(); // Out of bounds register index!
		}
#endif
		outDstRegisters[0] = &(outputRegisters[0]->oPos) + index;
		outDstRegisters[1] = &(outputRegisters[1]->oPos) + index;
		outDstRegisters[2] = &(outputRegisters[2]->oPos) + index;
		outDstRegisters[3] = &(outputRegisters[3]->oPos) + index;
		return;
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		if (isVS3_0)
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_3_0_outputs::oT) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			outDstRegisters[0] = (D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outDstRegisters[1] = (D3DXVECTOR4* const)&outputRegisters[1]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outDstRegisters[2] = (D3DXVECTOR4* const)&outputRegisters[2]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outDstRegisters[3] = (D3DXVECTOR4* const)&outputRegisters[3]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
		}
		else
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_2_0_outputs::oT) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			outDstRegisters[0] = (D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
			outDstRegisters[1] = (D3DXVECTOR4* const)&outputRegisters[1]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
			outDstRegisters[2] = (D3DXVECTOR4* const)&outputRegisters[2]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
			outDstRegisters[3] = (D3DXVECTOR4* const)&outputRegisters[3]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
		}
		return;
	case D3DSPR_CONSTINT   :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#endif
		outDstRegisters[0] = (D3DXVECTOR4* const)&constantRegisters->i[index];
		outDstRegisters[1] = (D3DXVECTOR4* const)&constantRegisters->i[index];
		outDstRegisters[2] = (D3DXVECTOR4* const)&constantRegisters->i[index];
		outDstRegisters[3] = (D3DXVECTOR4* const)&constantRegisters->i[index];
		return;
	case D3DSPR_COLOROUT   :
		if (isVS3_0)
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_3_0_outputs::oT) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			outDstRegisters[0] = (D3DXVECTOR4* const)&(outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[index]);
			outDstRegisters[1] = (D3DXVECTOR4* const)&(outputRegisters[1]->vs_interpolated_outputs.vs_3_0_outputs.oT[index]);
			outDstRegisters[2] = (D3DXVECTOR4* const)&(outputRegisters[2]->vs_interpolated_outputs.vs_3_0_outputs.oT[index]);
			outDstRegisters[3] = (D3DXVECTOR4* const)&(outputRegisters[3]->vs_interpolated_outputs.vs_3_0_outputs.oT[index]);
		}
		else
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_2_0_outputs::oD) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			outDstRegisters[0] = (D3DXVECTOR4* const)&(outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[index]);
			outDstRegisters[1] = (D3DXVECTOR4* const)&(outputRegisters[1]->vs_interpolated_outputs.vs_2_0_outputs.oD[index]);
			outDstRegisters[2] = (D3DXVECTOR4* const)&(outputRegisters[2]->vs_interpolated_outputs.vs_2_0_outputs.oD[index]);
			outDstRegisters[3] = (D3DXVECTOR4* const)&(outputRegisters[3]->vs_interpolated_outputs.vs_2_0_outputs.oD[index]);
		}
		return;
	case D3DSPR_ATTROUT    :
		if (isVS3_0)
		{
			outDstRegisters[0] = &outputRegisters[0]->oFog;
			outDstRegisters[1] = &outputRegisters[1]->oFog;
			outDstRegisters[2] = &outputRegisters[2]->oFog;
			outDstRegisters[3] = &outputRegisters[3]->oFog;
		}
		else
		{
#ifdef _DEBUG
			if (index >= ARRAYSIZE(VS_2_0_OutputRegisters::_vs_interpolated_outputs::_vs_2_0_outputs::oD) )
			{
				__debugbreak(); // Out of bounds register index!
			}
#endif
			outDstRegisters[0] = (D3DXVECTOR4* const)&(outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[index]);
			outDstRegisters[1] = (D3DXVECTOR4* const)&(outputRegisters[1]->vs_interpolated_outputs.vs_2_0_outputs.oD[index]);
			outDstRegisters[2] = (D3DXVECTOR4* const)&(outputRegisters[2]->vs_interpolated_outputs.vs_2_0_outputs.oD[index]);
			outDstRegisters[3] = (D3DXVECTOR4* const)&(outputRegisters[3]->vs_interpolated_outputs.vs_2_0_outputs.oD[index]);
		}
		return;
	case D3DSPR_DEPTHOUT   :
#ifdef _DEBUG
		__debugbreak(); // This should only be used from the PShader
#else
		outDstRegisters[0] = &runtimeRegisters[0].r[0];
		outDstRegisters[1] = &runtimeRegisters[1].r[0];
		outDstRegisters[2] = &runtimeRegisters[2].r[0];
		outDstRegisters[3] = &runtimeRegisters[3].r[0];
		return;
#endif
	case D3DSPR_SAMPLER    :
#ifdef _DEBUG
		__debugbreak(); // Sampler registers can't be dst registers - also sampler registers can't be used before vs_3_0
#else
		outDstRegisters[0] = &runtimeRegisters[0].r[0];
		outDstRegisters[1] = &runtimeRegisters[1].r[0];
		outDstRegisters[2] = &runtimeRegisters[2].r[0];
		outDstRegisters[3] = &runtimeRegisters[3].r[0];
		return;
#endif
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#else
		outDstRegisters[0] = &runtimeRegisters[0].r[0];
		outDstRegisters[1] = &runtimeRegisters[1].r[0];
		outDstRegisters[2] = &runtimeRegisters[2].r[0];
		outDstRegisters[3] = &runtimeRegisters[3].r[0];
		return;
#endif
	case D3DSPR_CONSTBOOL  :
#ifdef _DEBUG
		__debugbreak(); // Const registers can't be dst registers
#endif
		outDstRegisters[0] = (D3DXVECTOR4* const)&constantRegisters->b[index];
		outDstRegisters[1] = (D3DXVECTOR4* const)&constantRegisters->b[index];
		outDstRegisters[2] = (D3DXVECTOR4* const)&constantRegisters->b[index];
		outDstRegisters[3] = (D3DXVECTOR4* const)&constantRegisters->b[index];
		return;
	case D3DSPR_LOOP       :
		outDstRegisters[0] = (D3DXVECTOR4* const)&runtimeRegisters[0].aL;
		outDstRegisters[1] = (D3DXVECTOR4* const)&runtimeRegisters[1].aL;
		outDstRegisters[2] = (D3DXVECTOR4* const)&runtimeRegisters[2].aL;
		outDstRegisters[3] = (D3DXVECTOR4* const)&runtimeRegisters[3].aL;
		return;
	case D3DSPR_TEMPFLOAT16:
#ifdef _DEBUG
		__debugbreak();
#else
		outDstRegisters[0] = &runtimeRegisters[0].r[0];
		outDstRegisters[1] = &runtimeRegisters[1].r[0];
		outDstRegisters[2] = &runtimeRegisters[2].r[0];
		outDstRegisters[3] = &runtimeRegisters[3].r[0];
		return;
#endif
	case D3DSPR_MISCTYPE   :
#ifdef _DEBUG
		__debugbreak();
#endif
		outDstRegisters[0] = &runtimeRegisters[0].r[index];
		outDstRegisters[1] = &runtimeRegisters[1].r[index];
		outDstRegisters[2] = &runtimeRegisters[2].r[index];
		outDstRegisters[3] = &runtimeRegisters[3].r[index];
		return;
	case D3DSPR_LABEL      :
#ifdef _DEBUG
		__debugbreak(); // Labels can't be dst registers
#else
		outDstRegisters[0] = &runtimeRegisters[0].r[0];
		outDstRegisters[1] = &runtimeRegisters[1].r[0];
		outDstRegisters[2] = &runtimeRegisters[2].r[0];
		outDstRegisters[3] = &runtimeRegisters[3].r[0];
		return;
#endif
	case D3DSPR_PREDICATE  :
		outDstRegisters[0] = (D3DXVECTOR4* const)&runtimeRegisters[0].p;
		outDstRegisters[1] = (D3DXVECTOR4* const)&runtimeRegisters[1].p;
		outDstRegisters[2] = (D3DXVECTOR4* const)&runtimeRegisters[2].p;
		outDstRegisters[3] = (D3DXVECTOR4* const)&runtimeRegisters[3].p;
		return;
	default:
#ifdef _DEBUG
		{
			__debugbreak(); // Should never be here (invalid SPR type)
		}
#else
			__assume(0);
#endif
	}

	{
		__debugbreak();
	}
	outDstRegisters[0] = NULL;
	outDstRegisters[1] = NULL;
	outDstRegisters[2] = NULL;
	outDstRegisters[3] = NULL;
}

void VShaderEngine::ResolveDstParameter4(const dstParameterToken dstParameter, D3DXVECTOR4* (&outDstRegisters)[4])
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
		ResolveDstParameterVS4<false>(dstParameter.GetRegisterType(), index, outDstRegisters);
		break;
	case 3:
		ResolveDstParameterVS4<true>(dstParameter.GetRegisterType(), index, outDstRegisters);
		break;
	}
}

template <const bool isVS3_0>
void VShaderEngine::ResolveSrcParameterVS4(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned index, const D3DXVECTOR4* (&outSrcRegisters)[4]) const
{
	switch (registerType)
	{
	case D3DSPR_TEMP       :
		outSrcRegisters[0] = &runtimeRegisters[0].r[index];
		outSrcRegisters[1] = &runtimeRegisters[1].r[index];
		outSrcRegisters[2] = &runtimeRegisters[2].r[index];
		outSrcRegisters[3] = &runtimeRegisters[3].r[index];
		return;
	case D3DSPR_INPUT      :
		outSrcRegisters[0] = &inputRegisters[0].v[index];
		outSrcRegisters[1] = &inputRegisters[1].v[index];
		outSrcRegisters[2] = &inputRegisters[2].v[index];
		outSrcRegisters[3] = &inputRegisters[3].v[index];
		return;
	case D3DSPR_CONST      :
		outSrcRegisters[0] = &constantRegisters->c[index];
		outSrcRegisters[1] = &constantRegisters->c[index];
		outSrcRegisters[2] = &constantRegisters->c[index];
		outSrcRegisters[3] = &constantRegisters->c[index];
		return;
	case D3DSPR_ADDR       : // Also known as D3DSPR_TEXTURE (PS)
		outSrcRegisters[0] = (const D3DXVECTOR4* const)&runtimeRegisters[0].a;
		outSrcRegisters[1] = (const D3DXVECTOR4* const)&runtimeRegisters[1].a;
		outSrcRegisters[2] = (const D3DXVECTOR4* const)&runtimeRegisters[2].a;
		outSrcRegisters[3] = (const D3DXVECTOR4* const)&runtimeRegisters[3].a;
		return;
	case D3DSPR_RASTOUT    :
		if (isVS3_0)
		{
#ifdef _DEBUG
			__debugbreak();
#endif
			outSrcRegisters[0] = &runtimeRegisters[0].r[0];
			outSrcRegisters[1] = &runtimeRegisters[1].r[0];
			outSrcRegisters[2] = &runtimeRegisters[2].r[0];
			outSrcRegisters[3] = &runtimeRegisters[3].r[0];
			return;
		}
		else
		{
			outSrcRegisters[0] = (&outputRegisters[0]->oPos) + index;
			outSrcRegisters[1] = (&outputRegisters[1]->oPos) + index;
			outSrcRegisters[2] = (&outputRegisters[2]->oPos) + index;
			outSrcRegisters[3] = (&outputRegisters[3]->oPos) + index;
			return;
		}
	case D3DSPR_TEXCRDOUT  : // Also known as D3DSPR_OUTPUT
		if (isVS3_0)
		{
			outSrcRegisters[0] = (const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outSrcRegisters[1] = (const D3DXVECTOR4* const)&outputRegisters[1]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outSrcRegisters[2] = (const D3DXVECTOR4* const)&outputRegisters[2]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outSrcRegisters[3] = (const D3DXVECTOR4* const)&outputRegisters[3]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			return;
		}
		else
		{
			outSrcRegisters[0] = (const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
			outSrcRegisters[1] = (const D3DXVECTOR4* const)&outputRegisters[1]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
			outSrcRegisters[2] = (const D3DXVECTOR4* const)&outputRegisters[2]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
			outSrcRegisters[3] = (const D3DXVECTOR4* const)&outputRegisters[3]->vs_interpolated_outputs.vs_2_0_outputs.oT[index];
			return;
		}
	case D3DSPR_CONSTINT   :
		outSrcRegisters[0] = (const D3DXVECTOR4* const)&constantRegisters->i[index];
		outSrcRegisters[1] = (const D3DXVECTOR4* const)&constantRegisters->i[index];
		outSrcRegisters[2] = (const D3DXVECTOR4* const)&constantRegisters->i[index];
		outSrcRegisters[3] = (const D3DXVECTOR4* const)&constantRegisters->i[index];
		return;
	case D3DSPR_COLOROUT   :
		if (isVS3_0)
		{
			outSrcRegisters[0] = (const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outSrcRegisters[1] = (const D3DXVECTOR4* const)&outputRegisters[1]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outSrcRegisters[2] = (const D3DXVECTOR4* const)&outputRegisters[2]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			outSrcRegisters[3] = (const D3DXVECTOR4* const)&outputRegisters[3]->vs_interpolated_outputs.vs_3_0_outputs.oT[index];
			return;
		}
		else
		{
			outSrcRegisters[0] = (const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
			outSrcRegisters[1] = (const D3DXVECTOR4* const)&outputRegisters[1]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
			outSrcRegisters[2] = (const D3DXVECTOR4* const)&outputRegisters[2]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
			outSrcRegisters[3] = (const D3DXVECTOR4* const)&outputRegisters[3]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
			return;
		}
	case D3DSPR_ATTROUT    :
		if (isVS3_0)
		{
			outSrcRegisters[0] = &outputRegisters[0]->oFog;
			outSrcRegisters[1] = &outputRegisters[1]->oFog;
			outSrcRegisters[2] = &outputRegisters[2]->oFog;
			outSrcRegisters[3] = &outputRegisters[3]->oFog;
			return;
		}
		else
		{
			outSrcRegisters[0] = (const D3DXVECTOR4* const)&outputRegisters[0]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
			outSrcRegisters[1] = (const D3DXVECTOR4* const)&outputRegisters[1]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
			outSrcRegisters[2] = (const D3DXVECTOR4* const)&outputRegisters[2]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
			outSrcRegisters[3] = (const D3DXVECTOR4* const)&outputRegisters[3]->vs_interpolated_outputs.vs_2_0_outputs.oD[index];
			return;
		}
	case D3DSPR_DEPTHOUT   :
#ifdef _DEBUG
		__debugbreak();
#else
		outSrcRegisters[0] = &runtimeRegisters[0].r[0];
		outSrcRegisters[1] = &runtimeRegisters[1].r[0];
		outSrcRegisters[2] = &runtimeRegisters[2].r[0];
		outSrcRegisters[3] = &runtimeRegisters[3].r[0];
		return;
#endif
	case D3DSPR_SAMPLER    :
#ifdef _DEBUG
		__debugbreak();
#else
		outSrcRegisters[0] = &runtimeRegisters[0].r[0];
		outSrcRegisters[1] = &runtimeRegisters[1].r[0];
		outSrcRegisters[2] = &runtimeRegisters[2].r[0];
		outSrcRegisters[3] = &runtimeRegisters[3].r[0];
#endif
		return;
	case D3DSPR_CONST2     :
	case D3DSPR_CONST3     :
	case D3DSPR_CONST4     :
#ifdef _DEBUG
		__debugbreak();
#else
		outSrcRegisters[0] = &runtimeRegisters[0].r[0];
		outSrcRegisters[1] = &runtimeRegisters[1].r[0];
		outSrcRegisters[2] = &runtimeRegisters[2].r[0];
		outSrcRegisters[3] = &runtimeRegisters[3].r[0];
#endif
		return;
	case D3DSPR_CONSTBOOL  :
		outSrcRegisters[0] = (const D3DXVECTOR4* const)&constantRegisters->b[index];
		outSrcRegisters[1] = (const D3DXVECTOR4* const)&constantRegisters->b[index];
		outSrcRegisters[2] = (const D3DXVECTOR4* const)&constantRegisters->b[index];
		outSrcRegisters[3] = (const D3DXVECTOR4* const)&constantRegisters->b[index];
		return;
	case D3DSPR_LOOP       :
		outSrcRegisters[0] = (const D3DXVECTOR4* const)&runtimeRegisters[0].aL;
		outSrcRegisters[1] = (const D3DXVECTOR4* const)&runtimeRegisters[1].aL;
		outSrcRegisters[2] = (const D3DXVECTOR4* const)&runtimeRegisters[2].aL;
		outSrcRegisters[3] = (const D3DXVECTOR4* const)&runtimeRegisters[3].aL;
		return;
	case D3DSPR_TEMPFLOAT16:
#ifdef _DEBUG
		__debugbreak();
#else
		outSrcRegisters[0] = &runtimeRegisters[0].r[0];
		outSrcRegisters[1] = &runtimeRegisters[1].r[0];
		outSrcRegisters[2] = &runtimeRegisters[2].r[0];
		outSrcRegisters[3] = &runtimeRegisters[3].r[0];
#endif
		return;
	case D3DSPR_MISCTYPE   :
#ifdef _DEBUG
		__debugbreak();
#endif
		outSrcRegisters[0] = &runtimeRegisters[0].r[index];
		outSrcRegisters[1] = &runtimeRegisters[1].r[index];
		outSrcRegisters[2] = &runtimeRegisters[2].r[index];
		outSrcRegisters[3] = &runtimeRegisters[3].r[index];
		return;
	case D3DSPR_LABEL      :
#ifdef _DEBUG
		__debugbreak(); // Uhhhhh, is this debugbreak correct? Aren't Labels allowed to be source parameters for the CALL and CALLNZ instructions?
#else
		outSrcRegisters[0] = &runtimeRegisters[0].r[0];
		outSrcRegisters[1] = &runtimeRegisters[1].r[0];
		outSrcRegisters[2] = &runtimeRegisters[2].r[0];
		outSrcRegisters[3] = &runtimeRegisters[3].r[0];
#endif
		return;
	case D3DSPR_PREDICATE  :
		outSrcRegisters[0] = (const D3DXVECTOR4* const)&runtimeRegisters[0].p;
		outSrcRegisters[1] = (const D3DXVECTOR4* const)&runtimeRegisters[1].p;
		outSrcRegisters[2] = (const D3DXVECTOR4* const)&runtimeRegisters[2].p;
		outSrcRegisters[3] = (const D3DXVECTOR4* const)&runtimeRegisters[3].p;
		return;
	default:
#ifdef _DEBUG
		{
			__debugbreak();
		}
#else
			__assume(0);
#endif
	}

	{
		__debugbreak();
	}

	outSrcRegisters[0] = NULL;
	outSrcRegisters[1] = NULL;
	outSrcRegisters[2] = NULL;
	outSrcRegisters[3] = NULL;
}

void VShaderEngine::ResolveSrcParameter4(const srcParameterToken srcParameter, const D3DXVECTOR4* (&outSrcRegisters)[4]) const
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
		ResolveSrcParameterVS4<false>(srcParameter.GetRegisterType(), index, outSrcRegisters);
		return;
	case 3:
		ResolveSrcParameterVS4<true>(srcParameter.GetRegisterType(), index, outSrcRegisters);
		return;
	}
}

// Source parameter token: https://msdn.microsoft.com/en-us/library/windows/hardware/ff569716(v=vs.85).aspx
void VShaderEngine::ResolveSrcRegister4(const DWORD*& ptrSrcBytecode, D3DXVECTOR4 (&outSrcRegisters)[4]) const
{
	const DWORD rawSrcBytecode = *ptrSrcBytecode;
	const srcParameterToken& srcParameter = *(const srcParameterToken* const)&rawSrcBytecode;

	// Advance the instruction pointer (very important):
	++ptrSrcBytecode;

	const D3DXVECTOR4* sourceParamPtr4[4] = { NULL };

	switch (srcParameter.GetRelativeAddressingType() )
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#else
		__assume(0);
#endif
	case D3DSHADER_ADDRMODE_ABSOLUTE:
		ResolveSrcParameter4(srcParameter, sourceParamPtr4);
		break;
	case D3DSHADER_ADDRMODE_RELATIVE:
	{
		int addressOffsetRegisters4[4] = { 0 };

		const D3DXVECTOR4* originalSourceParamPtr4[4] = { NULL };
		ResolveSrcParameter4(srcParameter, originalSourceParamPtr4);

		union
		{
			const int4* addressRegister4i[4];
			const D3DXVECTOR4* addressRegister4f[4];
		} address4;

		// If SM2 or up, then relative addressing can use either the address register (a) or the loop
		// counter register (aL), so it needs another DWORD to know which to use. Otherwise (if vs_1_1) then
		// there's no extra relative addressing token and reading from a.x (the first component of the address
		// register) is assumed.
		if (shaderInfo->shaderMajorVersion > 1)
		{
			// Shader relative addressing: https://msdn.microsoft.com/en-us/library/windows/hardware/ff569708(v=vs.85).aspx
			const srcParameterToken& relativeAddressingSrcToken = *(const srcParameterToken* const)ptrSrcBytecode++;

			ResolveSrcParameter4(relativeAddressingSrcToken, address4.addressRegister4f);

			const unsigned int addressRegisterSwizzle = relativeAddressingSrcToken.GetChannelSwizzleXYZW();
			switch (addressRegisterSwizzle)
			{
			default:
#ifdef _DEBUG
				__debugbreak(); // Not sure what kinda swizzle we have here, but it's not a replicate-swizzle...
#endif
			case D3DSP_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT: // "No swizzle" in this case means "implied .xxxx" for the address register
			case D3DSP_REPLICATERED >> D3DVS_SWIZZLE_SHIFT:
				addressOffsetRegisters4[0] = RoundToNearest(address4.addressRegister4f[0]->x);
				addressOffsetRegisters4[1] = RoundToNearest(address4.addressRegister4f[1]->x);
				addressOffsetRegisters4[2] = RoundToNearest(address4.addressRegister4f[2]->x);
				addressOffsetRegisters4[3] = RoundToNearest(address4.addressRegister4f[3]->x);
				break;
			case D3DSP_REPLICATEGREEN >> D3DVS_SWIZZLE_SHIFT:
				addressOffsetRegisters4[0] = RoundToNearest(address4.addressRegister4f[0]->y);
				addressOffsetRegisters4[1] = RoundToNearest(address4.addressRegister4f[1]->y);
				addressOffsetRegisters4[2] = RoundToNearest(address4.addressRegister4f[2]->y);
				addressOffsetRegisters4[3] = RoundToNearest(address4.addressRegister4f[3]->y);
				break;
			case D3DSP_REPLICATEBLUE >> D3DVS_SWIZZLE_SHIFT:
				addressOffsetRegisters4[0] = RoundToNearest(address4.addressRegister4f[0]->z);
				addressOffsetRegisters4[1] = RoundToNearest(address4.addressRegister4f[1]->z);
				addressOffsetRegisters4[2] = RoundToNearest(address4.addressRegister4f[2]->z);
				addressOffsetRegisters4[3] = RoundToNearest(address4.addressRegister4f[3]->z);
				break;
			case D3DSP_REPLICATEALPHA >> D3DVS_SWIZZLE_SHIFT:
				addressOffsetRegisters4[0] = RoundToNearest(address4.addressRegister4f[0]->w);
				addressOffsetRegisters4[1] = RoundToNearest(address4.addressRegister4f[1]->w);
				addressOffsetRegisters4[2] = RoundToNearest(address4.addressRegister4f[2]->w);
				addressOffsetRegisters4[3] = RoundToNearest(address4.addressRegister4f[3]->w);
				break;
			}
		}
		else // Shader model 1 case:
		{
			ResolveSrcParameterVS4<false>(D3DSPR_ADDR, 0, address4.addressRegister4f);
			addressOffsetRegisters4[0] = RoundToNearest(address4.addressRegister4f[0]->x);
			addressOffsetRegisters4[1] = RoundToNearest(address4.addressRegister4f[1]->x);
			addressOffsetRegisters4[2] = RoundToNearest(address4.addressRegister4f[2]->x);
			addressOffsetRegisters4[3] = RoundToNearest(address4.addressRegister4f[3]->x);
		}

		const void* const relativeRegister4[4] = 
		{
			originalSourceParamPtr4[0] + addressOffsetRegisters4[0],
			originalSourceParamPtr4[1] + addressOffsetRegisters4[1],
			originalSourceParamPtr4[2] + addressOffsetRegisters4[2],
			originalSourceParamPtr4[3] + addressOffsetRegisters4[3]
		};
		ResolveSrcAddressIfValid4(relativeRegister4, sourceParamPtr4);
	}

		break;
	}

	const unsigned char sourceSwizzle = srcParameter.GetSwizzle();

	D3DXVECTOR4 ret4[4];

	// Handle source swizzles (if any are present):
	if (sourceSwizzle == (D3DVS_NOSWIZZLE >> D3DVS_SWIZZLE_SHIFT) )
	{
		ret4[0] = *sourceParamPtr4[0];
		ret4[1] = *sourceParamPtr4[1];
		ret4[2] = *sourceParamPtr4[2];
		ret4[3] = *sourceParamPtr4[3];
	}
	else
	{
		const float* const fltParams4[4] = 
		{
			&sourceParamPtr4[0]->x,
			&sourceParamPtr4[1]->x,
			&sourceParamPtr4[2]->x,
			&sourceParamPtr4[3]->x
		};

		// R channel
		const unsigned char rSwizzleSelect = sourceSwizzle & 0x3;
		ret4[0].x = fltParams4[0][rSwizzleSelect];
		ret4[1].x = fltParams4[1][rSwizzleSelect];
		ret4[2].x = fltParams4[2][rSwizzleSelect];
		ret4[3].x = fltParams4[3][rSwizzleSelect];

		// G channel
		const unsigned char gSwizzleSelect = (sourceSwizzle >> 2) & 0x3;
		ret4[0].y = fltParams4[0][gSwizzleSelect];
		ret4[1].y = fltParams4[1][gSwizzleSelect];
		ret4[2].y = fltParams4[2][gSwizzleSelect];
		ret4[3].y = fltParams4[3][gSwizzleSelect];

		// B channel
		const unsigned char bSwizzleSelect = (sourceSwizzle >> 4) & 0x3;
		ret4[0].z = fltParams4[0][bSwizzleSelect];
		ret4[1].z = fltParams4[1][bSwizzleSelect];
		ret4[2].z = fltParams4[2][bSwizzleSelect];
		ret4[3].z = fltParams4[3][bSwizzleSelect];

		// A channel
		const unsigned char aSwizzleSelect = (sourceSwizzle >> 6) & 0x3;
		ret4[0].w = fltParams4[0][aSwizzleSelect];
		ret4[1].w = fltParams4[1][aSwizzleSelect];
		ret4[2].w = fltParams4[2][aSwizzleSelect];
		ret4[3].w = fltParams4[3][aSwizzleSelect];
	}

	switch (srcParameter.GetSourceModifiersUnshifted() )
	{
	default:
#ifdef _DEBUG
		__debugbreak();
#endif
	case D3DSPSM_NONE   :
		outSrcRegisters[0] = ret4[0];
		outSrcRegisters[1] = ret4[1];
		outSrcRegisters[2] = ret4[2];
		outSrcRegisters[3] = ret4[3];
		return;
	case D3DSPSM_NEG    :
		outSrcRegisters[0] = ret4[0] * -1.0f;
		outSrcRegisters[1] = ret4[1] * -1.0f;
		outSrcRegisters[2] = ret4[2] * -1.0f;
		outSrcRegisters[3] = ret4[3] * -1.0f;
		return;
	case D3DSPSM_BIAS   :
	{
		outSrcRegisters[0] = ret4[0] + negHalfVec;
		outSrcRegisters[1] = ret4[1] + negHalfVec;
		outSrcRegisters[2] = ret4[2] + negHalfVec;
		outSrcRegisters[3] = ret4[3] + negHalfVec;
	}
		return;
	case D3DSPSM_BIASNEG:
	{
		ret4[0] += negHalfVec;
		ret4[1] += negHalfVec;
		ret4[2] += negHalfVec;
		ret4[3] += negHalfVec;
		outSrcRegisters[0] = ret4[0] * -1.0f;
		outSrcRegisters[1] = ret4[1] * -1.0f;
		outSrcRegisters[2] = ret4[2] * -1.0f;
		outSrcRegisters[3] = ret4[3] * -1.0f;
	}
		return;
	case D3DSPSM_SIGN   :
	{
		ret4[0] += negHalfVec;
		ret4[1] += negHalfVec;
		ret4[2] += negHalfVec;
		ret4[3] += negHalfVec;
		outSrcRegisters[0] = ret4[0] * 2.0f;
		outSrcRegisters[1] = ret4[1] * 2.0f;
		outSrcRegisters[2] = ret4[2] * 2.0f;
		outSrcRegisters[3] = ret4[3] * 2.0f;
	}
		return;
	case D3DSPSM_SIGNNEG:
	{
		ret4[0] += negHalfVec;
		ret4[1] += negHalfVec;
		ret4[2] += negHalfVec;
		ret4[3] += negHalfVec;
		outSrcRegisters[0] = ret4[0] * -2.0f;
		outSrcRegisters[1] = ret4[1] * -2.0f;
		outSrcRegisters[2] = ret4[2] * -2.0f;
		outSrcRegisters[3] = ret4[3] * -2.0f;
	}
		return;
	case D3DSPSM_COMP   :
	{
		const D3DXVECTOR4 oneVec(1.0f, 1.0f, 1.0f, 1.0f);
		outSrcRegisters[0] = oneVec - ret4[0];
		outSrcRegisters[1] = oneVec - ret4[1];
		outSrcRegisters[2] = oneVec - ret4[2];
		outSrcRegisters[3] = oneVec - ret4[3];
	}
		return;
	case D3DSPSM_X2     :
	{
		outSrcRegisters[0] = ret4[0] * 2.0f;
		outSrcRegisters[1] = ret4[1] * 2.0f;
		outSrcRegisters[2] = ret4[2] * 2.0f;
		outSrcRegisters[3] = ret4[3] * 2.0f;
	}
		return;
	case D3DSPSM_X2NEG  :
	{
		outSrcRegisters[0] = ret4[0] * -2.0f;
		outSrcRegisters[1] = ret4[1] * -2.0f;
		outSrcRegisters[2] = ret4[2] * -2.0f;
		outSrcRegisters[3] = ret4[3] * -2.0f;
	}
		return;
	case D3DSPSM_DZ     :
	{
		const float invZ4[4] = 
		{
			1.0f / ret4[0].z,
			1.0f / ret4[1].z,
			1.0f / ret4[2].z,
			1.0f / ret4[3].z
		};

		outSrcRegisters[0].x = ret4[0].x * invZ4[0];
		outSrcRegisters[1].x = ret4[1].x * invZ4[1];
		outSrcRegisters[2].x = ret4[2].x * invZ4[2];
		outSrcRegisters[3].x = ret4[3].x * invZ4[3];

		outSrcRegisters[0].y = ret4[0].y * invZ4[0];
		outSrcRegisters[1].y = ret4[1].y * invZ4[1];
		outSrcRegisters[2].y = ret4[2].y * invZ4[2];
		outSrcRegisters[3].y = ret4[3].y * invZ4[3];

		outSrcRegisters[0].z = ret4[0].z;
		outSrcRegisters[1].z = ret4[1].z;
		outSrcRegisters[2].z = ret4[2].z;
		outSrcRegisters[3].z = ret4[3].z;

		outSrcRegisters[0].w = ret4[0].w;
		outSrcRegisters[1].w = ret4[1].w;
		outSrcRegisters[2].w = ret4[2].w;
		outSrcRegisters[3].w = ret4[3].w;
	}
		return;
	case D3DSPSM_DW     :
	{
		const float invW4[4] = 
		{
			1.0f / ret4[0].w,
			1.0f / ret4[1].w,
			1.0f / ret4[2].w,
			1.0f / ret4[3].w
		};

		outSrcRegisters[0].x = ret4[0].x * invW4[0];
		outSrcRegisters[1].x = ret4[1].x * invW4[1];
		outSrcRegisters[2].x = ret4[2].x * invW4[2];
		outSrcRegisters[3].x = ret4[3].x * invW4[3];

		outSrcRegisters[0].y = ret4[0].y * invW4[0];
		outSrcRegisters[1].y = ret4[1].y * invW4[1];
		outSrcRegisters[2].y = ret4[2].y * invW4[2];
		outSrcRegisters[3].y = ret4[3].y * invW4[3];

		outSrcRegisters[0].z = ret4[0].z;
		outSrcRegisters[1].z = ret4[1].z;
		outSrcRegisters[2].z = ret4[2].z;
		outSrcRegisters[3].z = ret4[3].z;

		outSrcRegisters[0].w = ret4[0].w;
		outSrcRegisters[1].w = ret4[1].w;
		outSrcRegisters[2].w = ret4[2].w;
		outSrcRegisters[3].w = ret4[3].w;
	}
		return;
	case D3DSPSM_ABS    :
	{
		outSrcRegisters[0].x = fabsf(ret4[0].x);
		outSrcRegisters[1].x = fabsf(ret4[1].x);
		outSrcRegisters[2].x = fabsf(ret4[2].x);
		outSrcRegisters[3].x = fabsf(ret4[3].x);

		outSrcRegisters[0].y = fabsf(ret4[0].y);
		outSrcRegisters[1].y = fabsf(ret4[1].y);
		outSrcRegisters[2].y = fabsf(ret4[2].y);
		outSrcRegisters[3].y = fabsf(ret4[3].y);

		outSrcRegisters[0].z = fabsf(ret4[0].z);
		outSrcRegisters[1].z = fabsf(ret4[1].z);
		outSrcRegisters[2].z = fabsf(ret4[2].z);
		outSrcRegisters[3].z = fabsf(ret4[3].z);

		outSrcRegisters[0].w = fabsf(ret4[0].w);
		outSrcRegisters[1].w = fabsf(ret4[1].w);
		outSrcRegisters[2].w = fabsf(ret4[2].w);
		outSrcRegisters[3].w = fabsf(ret4[3].w);
	}
		return;
	case D3DSPSM_ABSNEG :
	{
		outSrcRegisters[0].x = -fabsf(ret4[0].x);
		outSrcRegisters[1].x = -fabsf(ret4[1].x);
		outSrcRegisters[2].x = -fabsf(ret4[2].x);
		outSrcRegisters[3].x = -fabsf(ret4[3].x);

		outSrcRegisters[0].y = -fabsf(ret4[0].y);
		outSrcRegisters[1].y = -fabsf(ret4[1].y);
		outSrcRegisters[2].y = -fabsf(ret4[2].y);
		outSrcRegisters[3].y = -fabsf(ret4[3].y);

		outSrcRegisters[0].z = -fabsf(ret4[0].z);
		outSrcRegisters[1].z = -fabsf(ret4[1].z);
		outSrcRegisters[2].z = -fabsf(ret4[2].z);
		outSrcRegisters[3].z = -fabsf(ret4[3].z);

		outSrcRegisters[0].w = -fabsf(ret4[0].w);
		outSrcRegisters[1].w = -fabsf(ret4[1].w);
		outSrcRegisters[2].w = -fabsf(ret4[2].w);
		outSrcRegisters[3].w = -fabsf(ret4[3].w);
	}
		return;
	case D3DSPSM_NOT    :
	{
		BOOL* const boolPtr4[4] = 
		{
			(BOOL* const)&ret4[0].x,
			(BOOL* const)&ret4[1].x,
			(BOOL* const)&ret4[2].x,
			(BOOL* const)&ret4[3].x
		};

		*boolPtr4[0] = !*boolPtr4[0];
		*boolPtr4[1] = !*boolPtr4[1];
		*boolPtr4[2] = !*boolPtr4[2];
		*boolPtr4[3] = !*boolPtr4[3];

		outSrcRegisters[0] = ret4[0];
		outSrcRegisters[1] = ret4[1];
		outSrcRegisters[2] = ret4[2];
		outSrcRegisters[3] = ret4[3];
	}
		return;
	}
}

const bool VShaderEngine::InterpreterExecStep4()
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
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		WriteDstParameter4(dstParam, src0);
	}
		break;
	case D3DSIO_ADD         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		add4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_SUB         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		sub4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_MAD         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		D3DXVECTOR4 src2[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		ResolveSrcRegister4(instructionPtr, src2);
		D3DXVECTOR4 dst[4];
		mad4(dst, src0, src1, src2);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_MUL         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		mul4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_RCP         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD srcBytecode = *instructionPtr;
		D3DXVECTOR4 src[4];
		ResolveSrcRegister4(instructionPtr, src);
		float f4[4];
		ResolveSrcReplicateSwizzle4(srcBytecode, src, f4);
		D3DXVECTOR4 dst[4];
		rcp4(dst, f4);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_RSQ         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD srcBytecode = *instructionPtr;
		D3DXVECTOR4 src[4];
		ResolveSrcRegister4(instructionPtr, src);
		float f4[4];
		ResolveSrcReplicateSwizzle4(srcBytecode, src, f4);
		D3DXVECTOR4 dst[4];
		rsq4(dst, f4);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_DP3         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		dp3_4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_DP4         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		dp4_4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_MIN         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		min4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_MAX         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		max4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_SLT         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		slt4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_SGE         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		sge4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_EXPP        :
	case D3DSIO_EXP         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD srcBytecode = *instructionPtr;
		D3DXVECTOR4 src[4];
		ResolveSrcRegister4(instructionPtr, src);
		float f4[4];
		ResolveSrcReplicateSwizzle4(srcBytecode, src, f4);
		D3DXVECTOR4 dst[4];
		exp4(dst, f4);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_LOGP        :
	case D3DSIO_LOG         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD srcBytecode = *instructionPtr;
		D3DXVECTOR4 src[4];
		ResolveSrcRegister4(instructionPtr, src);
		float f4[4];
		ResolveSrcReplicateSwizzle4(srcBytecode, src, f4);
		D3DXVECTOR4 dst[4];
		log4(dst, f4);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_LIT         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 dst[4];
		lit4(dst, src0);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_DST         : // TODO: Validate correctness of this (the docs seem incomplete and misleading)
	{
		const dstParameterToken destParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dest[4];
		dst4(dest, src0, src1);
		WriteDstParameter4(destParam, dest);
	}
		break;
	case D3DSIO_LRP         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		D3DXVECTOR4 src2[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		ResolveSrcRegister4(instructionPtr, src2);
		D3DXVECTOR4 dst[4];
		lrp4(dst, src0, src1, src2);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_FRC         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src[4];
		ResolveSrcRegister4(instructionPtr, src);
		D3DXVECTOR4 dst[4];
		frc4(dst, src);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_M4x4        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 src2[4]; // *(&src1 + 1);
		D3DXVECTOR4 src3[4]; // *(&src1 + 2);
		D3DXVECTOR4 src4[4]; // *(&src1 + 3);
		D3DXVECTOR4 dst[4];
		m4x4_4(dst, src0, src1, src2, src3, src4);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_M4x3        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 src2[4]; // *(&src1 + 1);
		D3DXVECTOR4 src3[4]; // *(&src1 + 2);
		D3DXVECTOR4 dst[4];
		m4x3_4(dst, src0, src1, src2, src3);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_M3x4        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 src2[4]; // *(&src1 + 1);
		D3DXVECTOR4 src3[4]; // *(&src1 + 2);
		D3DXVECTOR4 src4[4]; // *(&src1 + 3);
		D3DXVECTOR4 dst[4];
		m3x4_4(dst, src0, src1, src2, src3, src4);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_M3x3        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 src2[4]; // *(&src1 + 1);
		D3DXVECTOR4 src3[4]; // *(&src1 + 2);
		D3DXVECTOR4 dst[4];
		m3x3_4(dst, src0, src1, src2, src3);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_M3x2        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 src2[4]; // *(&src1 + 1);
		D3DXVECTOR4 dst[4];
		m3x2_4(dst, src0, src1, src2);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_CALL        :
	{
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		DbgBreakPrint("Error: CALL Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_CALLNZ      :
	{
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		DbgBreakPrint("Error: CALLNZ Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_LOOP        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
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
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		float f0[4];
		ResolveSrcReplicateSwizzle4(src0Bytecode, src0, f0);
		const DWORD src1Bytecode = *instructionPtr;
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		float f1[4];
		ResolveSrcReplicateSwizzle4(src1Bytecode, src1, f1);
		D3DXVECTOR4 dst[4];
		pow4(dst, f0, f1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_CRS         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 dst[4];
		crs4(dst, src0, src1);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_SGN         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src_unused[4];
		ResolveSrcRegister4(instructionPtr, src_unused); // Skip unused src1
		ResolveSrcRegister4(instructionPtr, src_unused); // Skip unused src2
		D3DXVECTOR4 dst[4];
		sgn4(dst, src0, src_unused, src_unused);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_ABS         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 dst[4];
		abs4(dst, src0);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_NRM         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 dst[4];
		nrm4(dst, src0);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_SINCOS      :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		const DWORD src0Bytecode = *instructionPtr;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		const srcParameterToken& srcParameter = *(const srcParameterToken* const)instructionPtr;
		const unsigned swizzleX = srcParameter.GetChannelSwizzleXYZW() & 0x3;
		const unsigned swizzleY = (srcParameter.GetChannelSwizzleXYZW() >> 2) & 0x3;
		float f[4];
		ResolveSrcReplicateSwizzle4(src0Bytecode, src0, f);
		if (shaderInfo->shaderMajorVersion < 3) // Shader model 2 has these extra registers, but all the shader model 3+ don't have them
		{
			D3DXVECTOR4 src_unused[4];
			ResolveSrcRegister4(instructionPtr, src_unused); // src1 is unused, so just skip it
			ResolveSrcRegister4(instructionPtr, src_unused); // src2 is unused, so just skip it
		}
		D3DXVECTOR4 dst[4];
		if (swizzleX && swizzleY)
			sincos_sc4(dst, f);
		else if (swizzleX)
			sincos_c4(dst, f);
		else if (swizzleY)
			sincos_s4(dst, f);
#ifdef _DEBUG
		else
		{
			// Whyyyyyyyyyyyyyyyy?
			__debugbreak();
		}
#endif
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_REP         :
	{
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		DbgBreakPrint("Error: REP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_ENDREP      :
		DbgBreakPrint("Error: ENDREP Shader function not yet implemented!"); // Not yet implemented!
		break;
	case D3DSIO_IF          :
	{
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		DbgBreakPrint("Error: IF Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_IFC         :
	{
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
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
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		DbgBreakPrint("Error: BREAKC Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_MOVA        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
#ifdef _DEBUG
		if (dstParam.GetRegisterType() != D3DSPR_ADDR)
		{
			__debugbreak(); // This instruction is only allowed to move into the address register (a0 register)
		}
#endif
		WriteDstParameter4(dstParam, src0);
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
		D3DXVECTOR4 src0[4];
		D3DXVECTOR4 src1[4];
		D3DXVECTOR4 src2[4];
		ResolveSrcRegister4(instructionPtr, src0);
		ResolveSrcRegister4(instructionPtr, src1);
		ResolveSrcRegister4(instructionPtr, src2);
		D3DXVECTOR4 dst[4];
		cnd4(dst, src0, src1, src2);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_DEF         :
		DbgBreakPrint("Error: DEF Should not be encountered during normal shader execution!");
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
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 src2[4];
		ResolveSrcRegister4(instructionPtr, src2);
		D3DXVECTOR4 dst[4];
		cmp4(dst, src0, src1, src2);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_BEM         :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		DbgBreakPrint("Error: BEM Shader function not yet implemented!"); // Not yet implemented!
	}
		break;
	case D3DSIO_DP2ADD      :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		D3DXVECTOR4 src2[4];
		ResolveSrcRegister4(instructionPtr, src2);
		D3DXVECTOR4 dst[4];
		dp2add4(dst, src0, src1, src2);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_DSX         : // Technically these derivative/gradient instructions aren't supposed to be runnable by vertex shaders, but it won't break anything if we do
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 dst[4];
		dsx(dst, src0);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_DSY         : // Technically these derivative/gradient instructions aren't supposed to be runnable by vertex shaders, but it won't break anything if we do
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 dst[4];
		dsy(dst, src0);
		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_TEXLDD      : // tex2Dgrad()
		DbgBreakPrint("Error: TEXLDD Shader function not available in vertex shader!");
		break;
	case D3DSIO_SETP        :
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		D3DXVECTOR4 src1[4];
		ResolveSrcRegister4(instructionPtr, src1);
		DbgBreakPrint("Error: SETP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;

		// Technically tex2D() is not allowed in vertex shaders, however I don't see anything wrong with promoting it to tex2Dlod and just letting the shader run
	case D3DSIO_TEX         : // Standard texture sampling with tex2D() for shader model 2 and up
	case D3DSIO_TEXLDL      : // tex2Dlod()
	{
		const dstParameterToken dstParam = *(const dstParameterToken* const)instructionPtr++;
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);

		// Since sampler registers can't be Relative Address'd, and since they come from the
		// constants table, we can do this once rather than four times for the same sampler:
		const srcParameterToken& src1Param = *(const srcParameterToken* const)instructionPtr++;
		const D3DXVECTOR4& src1 = ResolveSrcParameter(src1Param);
		const sampler* const samplerPtr = (const sampler* const)&src1;

		D3DXVECTOR4 dst[4];

		tex2Dlod4<0xF>(dst, src0, samplerPtr);

		// TODO: src1 can have a swizzle that gets applied after the texture sample but before the write mask happens

		WriteDstParameter4(dstParam, dst);
	}
		break;
	case D3DSIO_BREAKP      :
	{
		D3DXVECTOR4 src0[4];
		ResolveSrcRegister4(instructionPtr, src0);
		DbgBreakPrint("Error: BREAKP Shader function not yet implemented!"); // Not yet implemented!
	}
		break;		
	case D3DSIO_PHASE       :
		DbgBreakPrint("Error: Should never hit PHASE instruction in vertex shader!"); // Not yet implemented!
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

#endif // #ifdef RUN_SHADERS_IN_WARPS
