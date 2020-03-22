#pragma once

#include "d3d9include.h"
#include "ShaderEngineBase.h"

struct DeviceState;
struct DeviceState_ShaderRegisters;
struct ShaderInfo;

#define MAX_NUM_VS_CONSTANTS 4096

struct VS_2_0_ConstantsBuffer
{
	D3DXVECTOR4 c[MAX_NUM_VS_CONSTANTS]; // Constant registers
	BOOL b[MAX_NUM_CONST_BOOL_REGISTERS]; // Constant boolean registers
	int4 i[MAX_NUM_CONST_INT_REGISTERS]; // Constant int registers
	sampler s[4]; // Samplers
};

struct VS_2_0_InputRegisters
{
	__declspec(align(16) ) D3DXVECTOR4 v[VS_MAX_INPUT_REGISTERS]; // Input registers from vertex stream
};

struct VS_2_0_RuntimeRegisters
{
	D3DXVECTOR4 r[MAX_NUM_TEMP_REGISTERS]; // Temporary GPR
	int4 a; // Address register (one-channel register in vs_1_1 but four-channel in vs_2_0 and up). Sometimes called "a0".
	int4 p; // Predicate register
	int aL; // Loop Counter Register (only exists in vs_2_0 and up)
};

struct vertexClipStruct
{
	// Device clip planes (6 bits):
	unsigned short leftClip : 1;
	unsigned short rightClip : 1;
	unsigned short topClip : 1;
	unsigned short bottomClip : 1;
	unsigned short frontClip : 1;
	unsigned short backClip : 1;

	// User clipping planes (6 bits):
	unsigned short userClip0 : 1;
	unsigned short userClip1 : 1;
	unsigned short userClip2 : 1;
	unsigned short userClip3 : 1;
	unsigned short userClip4 : 1;
	unsigned short userClip5 : 1;

	// 2D Guard-band clip planes (4 bits):
	unsigned short GBLeftClip : 1;
	unsigned short GBRightClip : 1;
	unsigned short GBTopClip : 1;
	unsigned short GBBottomClip : 1;
};
static_assert(sizeof(vertexClipStruct) == sizeof(unsigned short), "Error!");

// Be careful about reordering elements within this struct, see outPositionRegisterIndex for more info
__declspec(align(16) ) struct VS_2_0_OutputRegisters
{
	VS_2_0_OutputRegisters()
	{
#ifdef _DEBUG
		memset(this, 0, sizeof(*this) );
#endif
	}

	__declspec(align(16) )  D3DXVECTOR4 oPos; // Output position register for pre-vs_3_0 (very important)

	// TODO: MSDN says that the fog and point-size registers are float1's, not float4's. Is this right, or does HLSL allow storing float4's in these registers?
	__declspec(align(16) )  D3DXVECTOR4 oFog; // Output fog register
	__declspec(align(16) )  D3DXVECTOR4 oPts; // Output point-size register
	union _vs_interpolated_outputs
	{
		struct _vs_2_0_outputs
		{
			__declspec(align(16) )  D3DCOLORVALUE oD[D3DMCS_COLOR2]; // Diffuse and Specular color output registers
			__declspec(align(16) )  D3DCOLORVALUE oT[D3DDP_MAXTEXCOORD]; // Output texcoord registers
		} vs_2_0_outputs;

		struct _vs_3_0_outputs
		{
			__declspec(align(16) )  D3DCOLORVALUE oT[10]; // Output texcoord registers
		} vs_3_0_outputs;
	} vs_interpolated_outputs;
	union vertexClipUnion
	{
		unsigned short clipCodesCombined;
		vertexClipStruct clipCodesNamed;
	} vertexClip;
};

__declspec(align(16) ) class VShaderEngine : public ShaderEngineBase
{
public:
	VShaderEngine() : constantRegisters(NULL)
	{
		outputRegisters[0] = NULL;
		outputRegisters[1] = NULL;
		outputRegisters[2] = NULL;
		outputRegisters[3] = NULL;
	}

	~VShaderEngine()
	{
		constantRegisters = NULL;
		for (unsigned x = 0; x < ARRAYSIZE(outputRegisters); ++x)
			outputRegisters[x] = NULL;
	}

	inline VShaderEngine& operator=(const VShaderEngine& rhs)
	{
		shaderInfo = rhs.shaderInfo;
		constantRegisters = rhs.constantRegisters;
		return *this;
	}

	inline void DeepCopy(const VShaderEngine& rhs)
	{
		memcpy(this, &rhs, sizeof(*this) );
	}

	// Called once for each shader instruction
	const bool InterpreterExecStep1();

	// Called once for each shader instruction, for 4 verts at a time
	const bool InterpreterExecStep4();

	// Called once at device reset time
	void GlobalInit(const VS_2_0_ConstantsBuffer* const _constantRegisters);

	// Called once at the beginning of processing a draw-call:
	void Init(const DeviceState& deviceState, const ShaderInfo& _shaderInfo, VS_2_0_ConstantsBuffer* const mutableConstantRegisters);

	// Called once for every vertex or vertex warp to reset the state of the interpreter to its default
	void Reset(VS_2_0_OutputRegisters* const * const outputVerts, const unsigned char numVerts);

	const VS_2_0_ConstantsBuffer* constantRegisters;
	__declspec(align(16) ) VS_2_0_InputRegisters inputRegisters[4];
	__declspec(align(16) ) VS_2_0_RuntimeRegisters runtimeRegisters[4];
	VS_2_0_OutputRegisters* outputRegisters[4];

	D3DXVECTOR4& ResolveDstParameter(const dstParameterToken dstParameter);
	void ResolveDstParameter4(const dstParameterToken dstParameter, D3DXVECTOR4* (&outDstRegisters)[4]);
	const D3DXVECTOR4& ResolveSrcParameter(const srcParameterToken srcParameter) const;
	void ResolveSrcParameter4(const srcParameterToken srcParameter, const D3DXVECTOR4* (&outSrcRegisters)[4]) const;
	const float ResolveSrcReplicateSwizzle(const DWORD rawSrcBytecode, const D3DXVECTOR4& registerData) const;
	void ResolveSrcReplicateSwizzle4(const DWORD rawSrcBytecode, const D3DXVECTOR4 (&registerData)[4], float (&outReplicateValue)[4]) const;

	INTRINSIC_INLINE const D3DXVECTOR4& ResolveSrcAddressIfValid(const void* const addressPtr) const
	{
		if (addressPtr >= &constantRegisters->c[0] && addressPtr <= &constantRegisters->c[(MAX_NUM_VS_CONSTANTS - 1)])
			return *(const D3DXVECTOR4* const)addressPtr;
		else if (addressPtr >= &runtimeRegisters[0].r[0] && addressPtr <= &runtimeRegisters[0].r[(MAX_NUM_TEMP_REGISTERS - 1)])
			return *(const D3DXVECTOR4* const)addressPtr;
		else if (addressPtr >= &inputRegisters[0].v[0] && addressPtr <= &inputRegisters[0].v[VS_MAX_INPUT_REGISTERS - 1])
			return *(const D3DXVECTOR4* const)addressPtr;
		else
			// When reading outside of the constants buffer or the runtime registers the
			// constant (0,0,0,0) is always returned
			return ZEROVEC;
	}

	INTRINSIC_INLINE void ResolveSrcAddressIfValid4(const void* const (&addressPtr4)[4], const D3DXVECTOR4* (&outSrcRegisters)[4]) const
	{
		if (addressPtr4[0] >= &constantRegisters->c[0] && addressPtr4[0] <= &constantRegisters->c[(MAX_NUM_VS_CONSTANTS - 1)])
			outSrcRegisters[0] = (const D3DXVECTOR4* const)addressPtr4[0];
		else if (addressPtr4[0] >= &runtimeRegisters[0].r[0] && addressPtr4[0] <= &runtimeRegisters[0].r[(MAX_NUM_TEMP_REGISTERS - 1)])
			outSrcRegisters[0] = (const D3DXVECTOR4* const)addressPtr4[0];
		else if (addressPtr4[0] >= &inputRegisters[0].v[0] && addressPtr4[0] <= &inputRegisters[0].v[VS_MAX_INPUT_REGISTERS - 1])
			outSrcRegisters[0] = (const D3DXVECTOR4* const)addressPtr4[0];
		else
			outSrcRegisters[0] = &ZEROVEC; // When reading outside of the constants buffer or the runtime registers the constant (0,0,0,0) is always returned

		if (addressPtr4[1] >= &constantRegisters->c[0] && addressPtr4[1] <= &constantRegisters->c[(MAX_NUM_VS_CONSTANTS - 1)])
			outSrcRegisters[1] = (const D3DXVECTOR4* const)addressPtr4[1];
		else if (addressPtr4[1] >= &runtimeRegisters[1].r[0] && addressPtr4[1] <= &runtimeRegisters[1].r[(MAX_NUM_TEMP_REGISTERS - 1)])
			outSrcRegisters[1] = (const D3DXVECTOR4* const)addressPtr4[0];
		else if (addressPtr4[1] >= &inputRegisters[1].v[0] && addressPtr4[1] <= &inputRegisters[1].v[VS_MAX_INPUT_REGISTERS - 1])
			outSrcRegisters[1] = (const D3DXVECTOR4* const)addressPtr4[1];
		else
			outSrcRegisters[1] = &ZEROVEC; // When reading outside of the constants buffer or the runtime registers the constant (0,0,0,0) is always returned

		if (addressPtr4[2] >= &constantRegisters->c[0] && addressPtr4[2] <= &constantRegisters->c[(MAX_NUM_VS_CONSTANTS - 1)])
			outSrcRegisters[2] = (const D3DXVECTOR4* const)addressPtr4[2];
		else if (addressPtr4[2] >= &runtimeRegisters[2].r[0] && addressPtr4[2] <= &runtimeRegisters[2].r[(MAX_NUM_TEMP_REGISTERS - 1)])
			outSrcRegisters[2] = (const D3DXVECTOR4* const)addressPtr4[2];
		else if (addressPtr4[2] >= &inputRegisters[2].v[0] && addressPtr4[2] <= &inputRegisters[2].v[VS_MAX_INPUT_REGISTERS - 1])
			outSrcRegisters[2] = (const D3DXVECTOR4* const)addressPtr4[2];
		else
			outSrcRegisters[2] = &ZEROVEC; // When reading outside of the constants buffer or the runtime registers the constant (0,0,0,0) is always returned

		if (addressPtr4[3] >= &constantRegisters->c[0] && addressPtr4[3] <= &constantRegisters->c[(MAX_NUM_VS_CONSTANTS - 1)])
			outSrcRegisters[3] = (const D3DXVECTOR4* const)addressPtr4[3];
		else if (addressPtr4[3] >= &runtimeRegisters[3].r[0] && addressPtr4[3] <= &runtimeRegisters[3].r[(MAX_NUM_TEMP_REGISTERS - 1)])
			outSrcRegisters[3] = (const D3DXVECTOR4* const)addressPtr4[3];
		else if (addressPtr4[3] >= &inputRegisters[3].v[0] && addressPtr4[3] <= &inputRegisters[3].v[VS_MAX_INPUT_REGISTERS - 1])
			outSrcRegisters[3] = (const D3DXVECTOR4* const)addressPtr4[3];
		else
			outSrcRegisters[3] = &ZEROVEC; // When reading outside of the constants buffer or the runtime registers the constant (0,0,0,0) is always returned
	}

	const INTRINSIC_INLINE D3DXVECTOR4& GetSrcRegisterFromAddress(const float4& baseRegister, const int addressOffset)
	{
		const float4* const targetRegister = &baseRegister + addressOffset;
		return ResolveSrcAddressIfValid(targetRegister);
	}

protected:
	void InitGlobalConstants(const DeviceState_ShaderRegisters& globalVertexShaderConstants, VS_2_0_ConstantsBuffer* const mutableConstantRegisters);
	void InitLocalConstants(const ShaderInfo& vertexShaderInfo, VS_2_0_ConstantsBuffer* const mutableConstantRegisters);
	void InitSamplers(const DeviceState& deviceState, VS_2_0_ConstantsBuffer* const mutableConstantRegisters);

	template <const bool isVS3_0>
	D3DXVECTOR4& ResolveDstParameterVS(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned index);

	template <const bool isVS3_0>
	void ResolveDstParameterVS4(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned index, D3DXVECTOR4* (&outDstRegisters)[4]);

	template <const bool isVS3_0>
	const D3DXVECTOR4& ResolveSrcParameterVS(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned index) const;

	template <const bool isVS3_0>
	void ResolveSrcParameterVS4(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned index, const D3DXVECTOR4* (&outSrcRegisters)[4]) const;

	// TODO: vs_3_0 introduced the concept of dst relative addressing (for output registers), we need to account for this somehow
	void WriteDstParameter(const dstParameterToken dstParameter, const D3DXVECTOR4& value);
	void WriteDstParameter4(const dstParameterToken dstParameter, const D3DXVECTOR4 (&value)[4]);

	const D3DXVECTOR4 ResolveSrcRegister(const DWORD*& ptrSrcBytecode) const;
	void ResolveSrcRegister4(const DWORD*& ptrSrcBytecode, D3DXVECTOR4 (&outSrcRegisters)[4]) const;
};
