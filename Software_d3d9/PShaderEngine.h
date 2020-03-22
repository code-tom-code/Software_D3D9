#pragma once

#include "d3d9include.h"
#include "ShaderEngineBase.h"

class IDirect3DTexture9Hook;
struct DeviceState;
struct DeviceState_ShaderRegisters;
struct ShaderInfo;

#define MAX_NUM_PS_CONSTANTS 4096
#define MAX_NUM_PS_INPUTS 10

__declspec(align(16) ) struct PS_2_0_ConstantsBuffer
{
	D3DXVECTOR4 c[MAX_NUM_PS_CONSTANTS]; // Constant registers
	BOOL b[MAX_NUM_CONST_BOOL_REGISTERS]; // Constant boolean registers
	int4 i[MAX_NUM_CONST_INT_REGISTERS]; // Constant int registers
	sampler s[16]; // Samplers
};

__declspec(align(16) ) struct PS_2_0_InputRegisters
{
	union _ps_interpolated_inputs
	{
		struct _ps_2_0_inputs
		{
			__declspec(align(16) ) D3DCOLORVALUE v[D3DMCS_COLOR2]; // Input vertex color registers (0 = diffuse, 1 = specular)
			__declspec(align(16) ) D3DCOLORVALUE t[D3DDP_MAXTEXCOORD]; // Texcoord registers
		} ps_2_0_inputs;

		struct _ps_3_0_inputs
		{
			__declspec(align(16) ) D3DCOLORVALUE t[MAX_NUM_PS_INPUTS]; // Input interpolator registers
		} ps_3_0_inputs;
	} ps_interpolated_inputs;
};

__declspec(align(16) ) struct PS_2_0_RuntimeRegisters
{
	D3DXVECTOR4 r[MAX_NUM_TEMP_REGISTERS]; // Temporary GPR
	int4 p0; // Predicate register
	int aL; // Loop Counter Register (only exists in ps_3_0 and up)
};

enum pixelOutputStatus : unsigned // This needs to be a 32-bit enum so that we can use gather4 on it
{
	normalWrite,
	discard, // TEXKILL
	stencilFail,
	ZFail
};

__declspec(align(16) ) struct PS_2_0_OutputRegisters
{
	float4 oC[4]; // Output color registers (very important)
	float oDepth; // Output depth register
	pixelOutputStatus pixelStatus; // Was this pixel discarded or did it finish (or was it Z or Stencil-culled)?
};

__declspec(align(16) ) struct PS_2_0_MiscRegisters
{
	D3DXVECTOR4 vPos;
	D3DXVECTOR4 vFace;
};

enum shaderStatus : unsigned char
{
	shouldContinueStatus = 0,
	errorStatus = 1,
	texkillStatus = 2,
	normalCompletion = 3
};

__declspec(align(16) ) class PShaderEngine : public ShaderEngineBase
{
public:
	PShaderEngine() : constantRegisters(NULL)
	{
	}

	~PShaderEngine()
	{
		constantRegisters = NULL;
	}

	inline PShaderEngine& operator=(const PShaderEngine& rhs)
	{
		shaderInfo = rhs.shaderInfo;
		constantRegisters = rhs.constantRegisters;
		return *this;
	}

	inline void DeepCopy(const PShaderEngine& rhs)
	{
		memcpy(this, &rhs, sizeof(*this) );
	}

	// Called once for each shader instruction
	const shaderStatus InterpreterExecStep();

	// Called once at device reset time
	void GlobalInit(const PS_2_0_ConstantsBuffer* const _constantRegisters);

	// Called once at the beginning of processing a draw-call:
	void Init(const DeviceState& deviceState, const ShaderInfo& _shaderInfo, PS_2_0_ConstantsBuffer* const mutableConstantRegisters);

	// Called once for every pixel to reset the state of the interpreter to its default
	void Reset(const unsigned x, const unsigned y);

	// Called once for every quad of pixels to reset the state of the interpreter to its default
	void Reset4(const __m128i x4, const __m128i y4);

	void InterpreterExecutePixel(void);

	const PS_2_0_ConstantsBuffer* constantRegisters;
	__declspec(align(16) ) PS_2_0_InputRegisters inputRegisters[4];
	__declspec(align(16) ) PS_2_0_RuntimeRegisters runtimeRegisters[4];
	__declspec(align(16) ) PS_2_0_OutputRegisters outputRegisters[4];
	__declspec(align(16) ) PS_2_0_MiscRegisters miscRegisters[4];

	const INTRINSIC_INLINE D3DXVECTOR4& ResolveSrcAddressIfValid(const void* const addressPtr) const
	{
		if (addressPtr >= &constantRegisters->c[0] && addressPtr <= &constantRegisters->c[(MAX_NUM_PS_CONSTANTS - 1)])
			return *(const D3DXVECTOR4* const)addressPtr;
		else if (addressPtr >= &runtimeRegisters[0].r[0] && addressPtr <= &runtimeRegisters[0].r[(MAX_NUM_TEMP_REGISTERS - 1)])
			return *(const D3DXVECTOR4* const)addressPtr;
		else if (addressPtr >= &inputRegisters[0].ps_interpolated_inputs.ps_3_0_inputs.t[0] && addressPtr <= &inputRegisters[0].ps_interpolated_inputs.ps_3_0_inputs.t[MAX_NUM_PS_INPUTS - 1])
			return *(const D3DXVECTOR4* const)addressPtr;
		else
			// When reading outside of the constants buffer or the runtime registers the
			// constant (0,0,0,0) is always returned
			return ZEROVEC;
	}

	const INTRINSIC_INLINE D3DXVECTOR4& GetSrcRegisterFromAddress(const float4& baseRegister, const int addressOffset)
	{
		const float4* const targetRegister = &baseRegister + addressOffset;
		return ResolveSrcAddressIfValid(targetRegister);
	}

protected:
	void InitGlobalConstants(const DeviceState_ShaderRegisters& globalPixelShaderConstants, PS_2_0_ConstantsBuffer* const mutableConstantRegisters);
	void InitLocalConstants(const ShaderInfo& pixelShaderInfo, PS_2_0_ConstantsBuffer* const mutableConstantRegisters);
	void InitSamplers(const DeviceState& deviceState, PS_2_0_ConstantsBuffer* const mutableConstantRegisters);

	void WriteDstParameter(const DWORD rawDstBytecode, const D3DXVECTOR4& value);
	D3DXVECTOR4& ResolveDstParameter(const DWORD rawDstBytecode);
	const float ResolveSrcReplicateSwizzle(const DWORD rawSrcBytecode, const D3DXVECTOR4& registerData) const;
	const D3DXVECTOR4& ResolveSrcParameter(const DWORD rawSrcBytecode) const;
	const D3DXVECTOR4 ResolveSrcRegister(const DWORD rawSrcBytecode) const;
};
