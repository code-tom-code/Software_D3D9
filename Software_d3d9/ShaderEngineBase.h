#pragma once

#include "ShaderBase.h"
#include "ShaderIntrinsicFunctions.h"

struct ShaderInfo;

static const D3DXVECTOR4 ZEROVEC(0.0f, 0.0f, 0.0f, 0.0f);

__declspec(align(16) ) class ShaderEngineBase
{
public:
	ShaderEngineBase() : instructionPtr(NULL), shaderInfo(NULL)
	{
	}

	~ShaderEngineBase()
	{
		instructionPtr = NULL;
		shaderInfo = NULL;
	}

	typedef void (__fastcall * tex2DMip0Sig)(float4& outVal, const float4& texCoord, const sampler* const samplerPtr);
	tex2DMip0Sig tex2DMip0ptrs[15];

	typedef void (__fastcall * tex2DLoDSig)(float4& outVal, const float4& texCoordAndLoD, const sampler* const samplerPtr);
	tex2DLoDSig tex2DLoDptrs[15];

	typedef void (__fastcall * tex2DGradSig)(float4& outVal, const float4& texCoord, const float4& texDdx, const float4& texDdy, const sampler* const samplerPtr);
	tex2DGradSig tex2DGradPtrs[15];

	typedef void (__fastcall * tex2DGradBiasSig)(float4& outVal, const float4& texCoord, const float4& texDdx, const float4& texDdy, const sampler* const samplerPtr);
	tex2DGradBiasSig tex2DGradBiasPtrs[15];

	void GlobalInitTex2DFunctionTable()
	{
		tex2DMip0ptrs[0] = (const tex2DMip0Sig)&tex2Dmip0<1>;
		tex2DMip0ptrs[1] = (const tex2DMip0Sig)&tex2Dmip0<2>;
		tex2DMip0ptrs[2] = (const tex2DMip0Sig)&tex2Dmip0<3>;
		tex2DMip0ptrs[3] = (const tex2DMip0Sig)&tex2Dmip0<4>;
		tex2DMip0ptrs[4] = (const tex2DMip0Sig)&tex2Dmip0<5>;
		tex2DMip0ptrs[5] = (const tex2DMip0Sig)&tex2Dmip0<6>;
		tex2DMip0ptrs[6] = (const tex2DMip0Sig)&tex2Dmip0<7>;
		tex2DMip0ptrs[7] = (const tex2DMip0Sig)&tex2Dmip0<8>;
		tex2DMip0ptrs[8] = (const tex2DMip0Sig)&tex2Dmip0<9>;
		tex2DMip0ptrs[9] = (const tex2DMip0Sig)&tex2Dmip0<10>;
		tex2DMip0ptrs[10] = (const tex2DMip0Sig)&tex2Dmip0<11>;
		tex2DMip0ptrs[11] = (const tex2DMip0Sig)&tex2Dmip0<12>;
		tex2DMip0ptrs[12] = (const tex2DMip0Sig)&tex2Dmip0<13>;
		tex2DMip0ptrs[13] = (const tex2DMip0Sig)&tex2Dmip0<14>;
		tex2DMip0ptrs[14] = (const tex2DMip0Sig)&tex2Dmip0<15>;

		tex2DLoDptrs[0] = (const tex2DLoDSig)&tex2Dlod<1>;
		tex2DLoDptrs[1] = (const tex2DLoDSig)&tex2Dlod<2>;
		tex2DLoDptrs[2] = (const tex2DLoDSig)&tex2Dlod<3>;
		tex2DLoDptrs[3] = (const tex2DLoDSig)&tex2Dlod<4>;
		tex2DLoDptrs[4] = (const tex2DLoDSig)&tex2Dlod<5>;
		tex2DLoDptrs[5] = (const tex2DLoDSig)&tex2Dlod<6>;
		tex2DLoDptrs[6] = (const tex2DLoDSig)&tex2Dlod<7>;
		tex2DLoDptrs[7] = (const tex2DLoDSig)&tex2Dlod<8>;
		tex2DLoDptrs[8] = (const tex2DLoDSig)&tex2Dlod<9>;
		tex2DLoDptrs[9] = (const tex2DLoDSig)&tex2Dlod<10>;
		tex2DLoDptrs[10] = (const tex2DLoDSig)&tex2Dlod<11>;
		tex2DLoDptrs[11] = (const tex2DLoDSig)&tex2Dlod<12>;
		tex2DLoDptrs[12] = (const tex2DLoDSig)&tex2Dlod<13>;
		tex2DLoDptrs[13] = (const tex2DLoDSig)&tex2Dlod<14>;
		tex2DLoDptrs[14] = (const tex2DLoDSig)&tex2Dlod<15>;

		tex2DGradPtrs[0] = (const tex2DGradSig)&tex2Dgrad<1, false>;
		tex2DGradPtrs[1] = (const tex2DGradSig)&tex2Dgrad<2, false>;
		tex2DGradPtrs[2] = (const tex2DGradSig)&tex2Dgrad<3, false>;
		tex2DGradPtrs[3] = (const tex2DGradSig)&tex2Dgrad<4, false>;
		tex2DGradPtrs[4] = (const tex2DGradSig)&tex2Dgrad<5, false>;
		tex2DGradPtrs[5] = (const tex2DGradSig)&tex2Dgrad<6, false>;
		tex2DGradPtrs[6] = (const tex2DGradSig)&tex2Dgrad<7, false>;
		tex2DGradPtrs[7] = (const tex2DGradSig)&tex2Dgrad<8, false>;
		tex2DGradPtrs[8] = (const tex2DGradSig)&tex2Dgrad<9, false>;
		tex2DGradPtrs[9] = (const tex2DGradSig)&tex2Dgrad<10, false>;
		tex2DGradPtrs[10] = (const tex2DGradSig)&tex2Dgrad<11, false>;
		tex2DGradPtrs[11] = (const tex2DGradSig)&tex2Dgrad<12, false>;
		tex2DGradPtrs[12] = (const tex2DGradSig)&tex2Dgrad<13, false>;
		tex2DGradPtrs[13] = (const tex2DGradSig)&tex2Dgrad<14, false>;
		tex2DGradPtrs[14] = (const tex2DGradSig)&tex2Dgrad<15, false>;

		tex2DGradBiasPtrs[0] = (const tex2DGradBiasSig)&tex2Dgrad<1, true>;
		tex2DGradBiasPtrs[1] = (const tex2DGradBiasSig)&tex2Dgrad<2, true>;
		tex2DGradBiasPtrs[2] = (const tex2DGradBiasSig)&tex2Dgrad<3, true>;
		tex2DGradBiasPtrs[3] = (const tex2DGradBiasSig)&tex2Dgrad<4, true>;
		tex2DGradBiasPtrs[4] = (const tex2DGradBiasSig)&tex2Dgrad<5, true>;
		tex2DGradBiasPtrs[5] = (const tex2DGradBiasSig)&tex2Dgrad<6, true>;
		tex2DGradBiasPtrs[6] = (const tex2DGradBiasSig)&tex2Dgrad<7, true>;
		tex2DGradBiasPtrs[7] = (const tex2DGradBiasSig)&tex2Dgrad<8, true>;
		tex2DGradBiasPtrs[8] = (const tex2DGradBiasSig)&tex2Dgrad<9, true>;
		tex2DGradBiasPtrs[9] = (const tex2DGradBiasSig)&tex2Dgrad<10, true>;
		tex2DGradBiasPtrs[10] = (const tex2DGradBiasSig)&tex2Dgrad<11, true>;
		tex2DGradBiasPtrs[11] = (const tex2DGradBiasSig)&tex2Dgrad<12, true>;
		tex2DGradBiasPtrs[12] = (const tex2DGradBiasSig)&tex2Dgrad<13, true>;
		tex2DGradBiasPtrs[13] = (const tex2DGradBiasSig)&tex2Dgrad<14, true>;
		tex2DGradBiasPtrs[14] = (const tex2DGradBiasSig)&tex2Dgrad<15, true>;
	}

	// Public accessor for ShaderInfo
	inline const ShaderInfo* const GetShaderInfo() const
	{
		return shaderInfo;
	}	

protected:
	const DWORD* instructionPtr;

	const ShaderInfo* shaderInfo;
};
