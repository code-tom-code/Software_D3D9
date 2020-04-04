#pragma once

#include "GlobalToggles.h"
#include "DebuggableEnums.h"
#include <intrin.h> // For intrinsics

#pragma pack(push)

// The maximum number of temp (GPR or "r") registers in any shader (vs, ps, 1, 2, and 3)
#define MAX_NUM_TEMP_REGISTERS 32

// The maximum number of constant integer ("i") registers
#define MAX_NUM_CONST_INT_REGISTERS 16

// The maximum number of constant bool ("b") registers
#define MAX_NUM_CONST_BOOL_REGISTERS 16

// These defines are used for gradient instructions (ddx/ddy or dsx/dsy)
#define GRADIENT_QUAD_TOPLEFT_INDEX 0
#define GRADIENT_QUAD_TOPRIGHT_INDEX 1
#define GRADIENT_QUAD_BOTLEFT_INDEX 2
#define GRADIENT_QUAD_BOTRIGHT_INDEX 3

#define MAX_USER_CLIP_PLANES_SUPPORTED 6

// All versions of vertex shaders can use up to 16 input registers
#define VS_MAX_INPUT_REGISTERS 16

// The intention of this class is to have a float4 class with no constructor overhead. Do not add a constructor to this struct that does anything!
struct float4
{
	float x, y, z, w;
};

// The intention of this class is to have an int4 class with no constructor overhead. Do not add a constructor to this struct that does anything!
struct int4
{
	int4()
	{
	}

	int4(const int _x, const int _y, const int _z, const int _w) : x(_x), y(_y), z(_z), w(_w)
	{
	}

	int x, y, z, w;
};

enum versionTokenShaderType : unsigned short
{
	verToken_vertexShader = (D3DVS_VERSION(0, 0) >> 16),
	verToken_PixelShader = (D3DPS_VERSION(0, 0) >> 16)
};

struct versionToken
{
	unsigned char minorVersion;
	unsigned char majorVersion;
	versionTokenShaderType pixelOrVertexShader;
};
static_assert(sizeof(versionToken) == sizeof(DWORD), "Error!");

enum eOpcodeControls : unsigned char
{
	OpCtrl_Reserved0 = D3DSPC_RESERVED0,

	// These comparison opcode controls are used in the "C" conditional variants to various instructions (break_comp with D3DSIO_BREAKC, if_comp with D3DSIO_IFC, and set_comp with D3DSIO_SETP)
	OpCtrl_Compare_Greater = D3DSPC_GT,
	OpCtrl_Compare_Equals = D3DSPC_EQ,
	OpCtrl_Compare_GreaterEquals = D3DSPC_GE,
	OpCtrl_Compare_Less = D3DSPC_LT,
	OpCtrl_Compare_NotEquals = D3DSPC_NE,
	OpCtrl_Compare_LessEquals = D3DSPC_LE,
	OpCtrl_Compare_Reserved = D3DSPC_RESERVED1,

	// One or zero of these texld opcode controls are used with the the D3DSIO_TEX opcode to make it either a texld, texldp, or texldb instruction
	// Note that the values of these enums overlap with those of the comparison opcode controls
	OpCtrl_TexLd_Project = (D3DSI_TEXLD_PROJECT >> D3DSP_OPCODESPECIFICCONTROL_SHIFT),
	OpCtrl_TexLd_Bias = (D3DSI_TEXLD_BIAS >> D3DSP_OPCODESPECIFICCONTROL_SHIFT)
};

// [31] Always 0x0 (false) for instruction tokens and always 0x1 (true) for parameter tokens
enum instructionParameterMarkerBit : unsigned char
{
	instructionTokenMarker = 0x0,
	parameterTokenMarker = 0x1
};

enum relativeAddressingEnableBit : unsigned short
{
	absoluteAddressing = (D3DSHADER_ADDRMODE_ABSOLUTE >> D3DSHADER_ADDRESSMODE_SHIFT),
	relativeAddressing = (D3DSHADER_ADDRMODE_RELATIVE >> D3DSHADER_ADDRESSMODE_SHIFT)
};

// Instruction Token: https://msdn.microsoft.com/en-us/library/windows/hardware/ff567721(v=vs.85).aspx
struct instructionToken
{
	DEBUGGABLE_D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode; // [15:00] Bits 0 through 15 indicate an operation code. D3DSIO_* is an example of an operation code, where * represents the instruction.
	eOpcodeControls opcodeControls; // [23:16] Bits 16 through 23 indicate specific controls related to the operation code.
	unsigned char instructionLengthDWORDs : 4; /* [27:24] For pixel and vertex shader versions earlier than 2_0, bits 24 through 27 are reserved and set to 0x0.
										 For pixel and vertex shader versions 2_0 and later, bits 24 through 27 specify the size in DWORDs of the 
										 instruction excluding the instruction token itself (that is, the number of tokens that comprise the 
										 instruction excluding the instruction token). */
	bool predicated : 1; /* [28] For pixel and vertex shader versions earlier than 2_0, bit 28 is reserved and set to 0x0.
								  For pixel and vertex shader versions 2_0 and later, bit 28 indicates whether the instruction is predicated (that is, 
								  contains an extra predicate source token at the end of the shader code. If this 
								  bit is set to 0x1, the instruction is predicated. */
	unsigned char reservedZero : 1; // [29] Reserved. This value is set to 0x0.
	bool coIssue : 1; /* [30] For pixel shader versions earlier than 2_0, bit 30 is the co-issue bit.
							   If set to 1, execute this instruction with previous instructions; otherwise, execute separately.
							   For pixel shader version 2_0 and later and all vertex shader versions, bit 30 is reserved and set to 0x0. */
	instructionParameterMarkerBit instructionOrParameterMarkerBit : 1; // [31] Always 0x0 (false) for instruction tokens and always 0x1 (true) for parameter tokens
};
static_assert(sizeof(instructionToken) == sizeof(DWORD), "Error!");

struct parameterToken
{
	inline const unsigned GetRegisterIndex(void) const
	{
		return internalRawToken & D3DSP_REGNUM_MASK;
	}

	inline const D3DSHADER_PARAM_REGISTER_TYPE GetRegisterType(void) const
	{
		const unsigned regType1 = (internalRawToken & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT;
		const unsigned regType2 = ( (internalRawToken & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2) ;// >> 3;
		return (const D3DSHADER_PARAM_REGISTER_TYPE)(regType1 | regType2);
	}

	inline const D3DSHADER_ADDRESSMODE_TYPE GetRelativeAddressingType(void) const
	{
		return (const D3DSHADER_ADDRESSMODE_TYPE)( (internalRawToken & D3DSHADER_ADDRESSMODE_MASK) );
	}

	union
	{
		// This is the raw DWORD token
		DWORD internalRawToken;

		struct debuggableSrcParameter
		{
			unsigned short registerIndex : 11; // [10:00] Bits 0 through 10 indicate the register number (offset in register file).
			unsigned short registerType_highBits : 2; // [12:11] Bits 11 and 12 are the fourth and fifth bits [3,4] for indicating the register type.
			relativeAddressingEnableBit relativeAddressingEnable : 1; /* [13] For a pixel shader (PS) versions earlier than 3_0, bit 13 is reserved and set to 0x0.
														 For pixel shader (PS) version 3_0 and later and all versions of vertex shader (VS),
														 bit 13 indicates whether relative addressing mode is used.
														 If set to 1, relative addressing applies.*/
			unsigned short reserved_mustBeZero : 2; // [15:14] Reserved for all versions of PS and VS. This value is set to 0x0.
			struct
			{
				union
				{
					struct
					{
						debuggableSwizzleChannel xSwizzle : 2; // [17:16] Channel X swizzle
						debuggableSwizzleChannel ySwizzle : 2; // [19:18] Channel Y swizzle
						debuggableSwizzleChannel zSwizzle : 2; // [21:20] Channel Z swizzle
						debuggableSwizzleChannel wSwizzle : 2; // [23:22] Channel W swizzle
					} channelSwizzles;

					debuggableFullSwizzle fullSwizzle; // [23:16] XYZW swizzle
				};
			} sourceSwizzle; /* [23:16] Bits 16 through 23 indicate channel swizzle.
							 All arithmetic operations are performed in four (X,Y,Z,W) parallel channels.
							 Swizzle specifies which source component participates in a channel of operation.
							 For more information about swizzle, see the latest DirectX SDK documentation. */
			debuggableSourceModifierType sourceModifier : 4; /* [27:24] Bits 24 through 27 indicate the source modifier.
															 This 4-bit value indicates the source modifier types. */
			unsigned char registerType_lowBits : 3; // [30:28] Bits 28 through 30 are the first three bits [0,1,2] for indicating the register type
			instructionParameterMarkerBit instructionParameterBit : 1; // [31] Bit 31 is 0x1. This must always be 0x1 for parameter tokens
		} srcParameter;

		struct debuggableDstParameter
		{
			unsigned short registerIndex : 11; // [10:00] Bits 0 through 10 indicate the register number (offset in register file).
			unsigned short registerType_highBits : 2; // [12:11] Bits 11 and 12 are the fourth and fifth bits [3,4] for indicating the register type.
			relativeAddressingEnableBit relativeAddressingEnable : 1; /* [13] For vertex shader (VS) version 3_0 and later,
														 bit 13 indicates whether relative addressing mode is used.
														 If set to 1, relative addressing applies.
														 For all pixel shader (PS) versions (ps_*) and vertex shader versions earlier than 3_0, bit 13 is reserved and set to 0x0. */
			unsigned short reserved_mustBeZero : 2; // [15:14] Reserved. This value is set to 0x0.
			union
			{
				struct
				{
					writeMaskType writeMask : 4;
					writeMaskType /*resultModifier*/ : 4;
				} writeMaskStruct;

				struct
				{
					resultModifierType /*writeMask*/ : 4;
					resultModifierType resultModifier : 4;
				} resultModifierStruct;

				struct
				{
					unsigned char writeMask : 4; // [19:16] Write mask.
					unsigned char resultModifier : 4; // [23:20] Bits 20 through 23 indicate the result modifier. Multiple result modifiers can be used (result modifier types can be ORed together).
				} writeMaskAndResultModiferRawStruct;
			};
			signed char resultShiftScale : 4; /* [27:24] For PS versions earlier than 2_0, bits 24 through 27 specify the result shift scale (signed shift).
												For PS version 2_0 and later and VS, these bits are reserved and set to 0x0. */
			unsigned char registerType_lowBits : 3; // [30:28] Bits 28 through 30 are the first three bits [0,1,2] for indicating the register type.
			instructionParameterMarkerBit instructionParameterBit : 1; // [31] Bit 31 is 0x1. This must always be 0x1 for parameter tokens
		} dstParameter;
	};
};
static_assert(sizeof(parameterToken) == sizeof(DWORD), "Error!");

// Source: https://msdn.microsoft.com/en-us/library/windows/hardware/ff569716(v=vs.85).aspx
struct srcParameterToken : public parameterToken
{
	inline const unsigned GetSourceModifiers(void) const
	{
		return ( (internalRawToken & D3DSP_SRCMOD_MASK) >> D3DSP_SRCMOD_SHIFT);
	}

	inline const D3DSHADER_PARAM_SRCMOD_TYPE GetSourceModifiersUnshifted(void) const
	{
		return (const D3DSHADER_PARAM_SRCMOD_TYPE)(internalRawToken & D3DSP_SRCMOD_MASK);
	}

	// Returns 0, 1, 2, or 3 to indicate an X, Y, Z, or W swizzle respectively
	inline const unsigned char GetChannelSwizzle(void) const
	{
		return (const unsigned char)( ( (internalRawToken & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT) & 0x3);
	}

	inline const unsigned GetChannelSwizzleXYZW(void) const
	{
		return ( (internalRawToken & D3DSP_SWIZZLE_MASK) >> D3DSP_SWIZZLE_SHIFT);
	}

	inline const unsigned char GetSwizzle(void) const
	{
		return (const unsigned char)( (internalRawToken & D3DVS_SWIZZLE_MASK) >> D3DVS_SWIZZLE_SHIFT);
	}
};
static_assert(sizeof(srcParameterToken) == sizeof(DWORD), "Error!");

// Source: https://msdn.microsoft.com/en-us/library/windows/hardware/ff552738(v=vs.85).aspx
struct dstParameterToken : public parameterToken
{
	inline const unsigned GetWriteMask(void) const
	{
		return (internalRawToken & D3DSP_WRITEMASK_ALL) >> 16;
	}

	inline const unsigned GetResultShiftScale(void) const
	{
		return (internalRawToken & D3DSP_DSTSHIFT_MASK) >> D3DSP_DSTSHIFT_SHIFT;
	}

	inline const unsigned GetResultModifier(void) const
	{
		return (internalRawToken & D3DSP_DSTMOD_MASK) >> D3DSP_DSTMOD_SHIFT;
	}

	inline const unsigned GetResultModifierUnshifted(void) const
	{
		return (internalRawToken & D3DSP_DSTMOD_MASK);
	}
};
static_assert(sizeof(dstParameterToken) == sizeof(DWORD), "Error!");

#pragma pack(pop)

struct SamplerState
{
	SamplerState()
	{
		ClearSamplerStateToDefaults();
	}

	~SamplerState()
	{
		ClearSamplerStateToDefaults();
	}

	inline void ClearSamplerStateToDefaults()
	{
		static const DWORD badSamplerStateValue = 0xBAADCAFE;
		for (unsigned x = 0; x <= D3DSAMP_DMAPOFFSET; ++x)
			stateUnion.state[x] = badSamplerStateValue;

		stateUnion.namedStates.addressU = D3DTADDRESS_WRAP;
		stateUnion.namedStates.addressV = D3DTADDRESS_WRAP;
		stateUnion.namedStates.addressW = D3DTADDRESS_WRAP;
		stateUnion.namedStates.borderColor = D3DCOLOR_ARGB(0, 0, 0, 0);
		stateUnion.namedStates.magFilter = D3DTEXF_POINT;
		stateUnion.namedStates.minFilter = D3DTEXF_POINT;
		stateUnion.namedStates.mipFilter = D3DTEXF_NONE;
		stateUnion.namedStates.mipMapLoDBias = 0.0f;
		stateUnion.namedStates.maxMipLevel = 0;
		stateUnion.namedStates.maxAnisotropy = 1;
		stateUnion.namedStates.sRGBTexture = FALSE; // Not sure if this is a BOOL or a float (see variable definition for more info)
		stateUnion.namedStates.elementIndex = 0;
		stateUnion.namedStates.dMapOffset = 0;

		cachedFloatMaxMipLevel = 0.0f;
	}

	union _stateUnion
	{
		struct _namedStates
		{
			DWORD zeroState; // This is an invalid sampler state
			D3DTEXTUREADDRESS addressU;
			D3DTEXTUREADDRESS addressV;
			D3DTEXTUREADDRESS addressW;
			D3DCOLOR borderColor;
			D3DTEXTUREFILTERTYPE magFilter;
			D3DTEXTUREFILTERTYPE minFilter;
			D3DTEXTUREFILTERTYPE mipFilter;
			float mipMapLoDBias;
			DWORD maxMipLevel;
			DWORD maxAnisotropy;
			BOOL sRGBTexture; // NVidia cg docs (http://http.developer.nvidia.com/Cg/SRGBTexture.html) make this sound like a float, but D3D9 docs (https://msdn.microsoft.com/en-us/library/windows/desktop/bb172602(v=vs.85).aspx) make it sound like a BOOL. Since we're trying to emulate D3D9 here, we'll use the BOOL definition.
			DWORD elementIndex;
			DWORD dMapOffset; // "(DWORD) - Offset (in vertices) in a pre-sampled displacement map". Source: https://msdn.microsoft.com/en-us/library/windows/desktop/bb219748(v=vs.85).aspx
		} namedStates;
		DWORD state[D3DSAMP_DMAPOFFSET + 1];
	} stateUnion;
	static_assert(sizeof(_stateUnion) == sizeof(DWORD) * (D3DSAMP_DMAPOFFSET + 1), "Error: Unexpected union size!");

	float cachedFloatMaxMipLevel;
};

struct sampler
{
	SamplerState samplerState;
	class IDirect3DTexture9Hook* texture;
};
