#pragma once

#include "d3d9include.h"
#include "ShaderBase.h"

#include <vector> // for std::vector

#include <stdio.h> // for printf

// Uncomment to disable
#define DISASM_SHADER 1

struct ShaderInfo;

struct InitialConstantValueBase
{
	unsigned constantRegisterIndex;
};

struct InitialConstantValue : public InitialConstantValueBase
{
	D3DXVECTOR4 initialValue;
};

struct InitialConstantValueI : public InitialConstantValueBase
{
	int4 initialValue;
};

struct InitialConstantValueB : public InitialConstantValueBase
{
	BOOL initialValue;
};

enum opcodeDisplayType
{
	justOpcode = 0,
	srcOnly,
	srcSrcOnly,
	dstOnly,
	srcDst,
	srcSrcDst,
	srcSrcSrcDst,
	srcSrcSrcSrcDst, // Only ever used with D3DSIO_TEXLDD
	customOpcode
};

// Thanks for all the documentation at: https://msdn.microsoft.com/en-us/library/windows/hardware/ff549176(v=vs.85).aspx
struct DeclaredRegister
{
	D3DSHADER_PARAM_REGISTER_TYPE registerType; // Sampler, input (from vertex decl), output, etc.
	D3DDECLUSAGE usageType; // If this is an input register for a VS, what is the usage that's coming in from the vertex declaration?
	unsigned usageIndex; // If this has a valid usage value, this is its usage index (POSITION0, COLOR2, TEXCOORD4, etc.)
	unsigned registerIndex;
	bool isOutputRegister; // true = output, false = input (output declarations are optional in SM3.0)
	unsigned char writeMask : 4; // Which channels are present on this register (x, y, z, and/or w)?
	union
	{
		D3DSAMPLER_TEXTURE_TYPE textureType; // Used only if this is a sampler
		D3DSHADER_MISCTYPE_OFFSETS miscInputType; // Used only if this is a misc. register
		D3DVS_RASTOUT_OFFSETS rasterOutType; // Used only if this is a rasterizer output register
		DWORD uintData;
	} miscInfo;
};

struct ShaderLabel
{
	unsigned labelIndex;
	const DWORD* firstPostLabelToken;
};

struct ShaderComment
{
	unsigned commentLengthTokens; // Length of the comment, in tokens (not including the comment token itself)
	const char* commentText; // Points to the beginning of the comment text
};

struct outputRegisterWriteTracker
{
	outputRegisterWriteTracker()
	{
	}

	struct outputRegisterIdentifier
	{
		BYTE outputRegisterType;
		BYTE outputRegisterNumber;
	};

	struct outputRegisterWriteOperation
	{
		outputRegisterWriteOperation() : writeInstruction(NULL), registerParameterToken(NULL), optionalRelativeAddressingDestParameterToken(NULL), instructionSourceParameterTokens(NULL), operationWriteMask(0xF)
		{
		}

		const instructionToken* writeInstruction;
		const dstParameterToken* registerParameterToken;
		const dstParameterToken* optionalRelativeAddressingDestParameterToken;
		const srcParameterToken* instructionSourceParameterTokens;
		BYTE operationWriteMask;
	};

	struct outputRegisterWriteState
	{
		outputRegisterWriteState() : writeMaskFinalState(0x00)
		{
		}

		std::vector<outputRegisterWriteOperation> writeOperations;
		BYTE writeMaskFinalState;
	};

	void SetAddOrSkip(const D3DSHADER_PARAM_REGISTER_TYPE registerType, const unsigned registerIndex, const BYTE writeMask, const dstParameterToken& dstParameter, const instructionToken* const writeInstruction, const dstParameterToken* const optionalRelativeRegister = NULL);

	void AddOutputRegisterListingToShaderInfo(ShaderInfo& outShaderInfo);

	static const unsigned MAX_SHADER_REGISTER_TYPES = D3DSPR_PREDICATE + 1;

	// 12 comes from vs_3_0 which supports 12 vertex shader output registers (o0 thru o11)
	static const unsigned MAX_SHADER_OUTPUT_REGISTERS = 12;

	outputRegisterWriteState writtenRegisters[MAX_SHADER_REGISTER_TYPES][MAX_SHADER_OUTPUT_REGISTERS];

	// This vector stores all of the output register identifiers in the order that they were found in the shader:
	std::vector<outputRegisterIdentifier> registerIdentifiers;
};

struct WrittenOutputRegister
{
	WrittenOutputRegister() : registerType(D3DSPR_TEMP), registerIndex(0), componentChannelsWritten(0x00)
	{
	}

	D3DSHADER_PARAM_REGISTER_TYPE registerType;
	unsigned registerIndex;
	BYTE componentChannelsWritten; /* For D3D8 shaders this is okay to be anything nonzero, but for D3D9 SM1.1 or SM2.0 shader validation it must be always 0xF (as it's
	illegal to not write all channels in an output register until SM3.0 when partial writes to output registers were allowed again) */

	// A history (in the order of which writes occurred in the shader) of all write operations to this shader output register
	std::vector<outputRegisterWriteTracker::outputRegisterWriteOperation> writeOperationsHistory;
};

struct ShaderInfo
{
	ShaderInfo() : parsingErrorDetected(false), isPixelShader(false), shaderMajorVersion(0), shaderMinorVersion(0), 
		shaderLengthDWORDs(0), numArithInstructions(0), numTexInstructions(0), tempRegistersUsedBitmask(0), numUniqueTempRegistersUsed(0),
		usesDynamicBranching(false), usesFunctionCalls(false), usesVertTextureFetch(false), hasDependentTextureFetches(false), usesMRT(false), firstInstructionToken(NULL),
		usesIntToFloatConversions(false), usesTexkill(false), usesGradientInstructions(false), initialBytecodeToken(NULL), usedMRTMask(0), psWritesDepth(false), 
		usesInstructionPredication(false), usesInstructionCoIssue(false), shaderBytecodeHash(0), inputRegistersUsedBitmask(0), numUniqueInputRegistersUsed(0)
#ifdef DISASM_SHADER
		, shaderDisasmBuffer(NULL), D3DXDisasm(NULL), D3DXDisasmString(NULL)
#endif
	{
		initialConstantValues.clear();
		initialConstantValuesI.clear();
		initialConstantValuesB.clear();
		usedConstantsF.clear();
		usedConstantsI.clear();
		usedConstantsB.clear();
		declaredRegisters.clear();
		writtenOutputRegisters.clear();
		labels.clear();
		comments.clear();
#ifdef DISASM_SHADER
		shaderDisasmBuffer = NULL;
		D3DXDisasmString = NULL;
		if (D3DXDisasm)
		{
			D3DXDisasm->Release();
			D3DXDisasm = NULL;
		}
		shaderDisasmText.clear();
#endif
		fixedFunctionMacroDefines.clear();
		
		initialBytecodeToken = NULL;
		firstInstructionToken = NULL;
	}

	~ShaderInfo()
	{
#ifdef DISASM_SHADER
		shaderDisasmBuffer = NULL;
		D3DXDisasmString = NULL;
		if (D3DXDisasm)
		{
			D3DXDisasm->Release();
			D3DXDisasm = NULL;
		}
		shaderDisasmText.clear();
#endif
	}

	// Disallow copying of ShaderInfos!
	ShaderInfo(const ShaderInfo&) = delete;
	ShaderInfo(ShaderInfo&&) = delete;
	void operator=(const ShaderInfo&) = delete;

	bool parsingErrorDetected; // Could not parse the entire shader - ShaderInfo may be wrong!
	bool isPixelShader; // true = pixel shader, false = vertex shader
	unsigned short shaderMajorVersion;
	unsigned short shaderMinorVersion;

#ifdef DISASM_SHADER
	const char* shaderDisasmBuffer;
	LPD3DXBUFFER D3DXDisasm;
	const char* D3DXDisasmString;
#endif

	void PrintShaderStatsToString(char (&outBuffer)[1024]) const;

	unsigned shaderLengthDWORDs; // The number of DWORD tokens (including the first version token) in the bytecode
	unsigned numArithInstructions;
	unsigned numTexInstructions; // This field must be directly after numArithInstructions for reasons
	unsigned tempRegistersUsedBitmask; // A bitmask containing which of the (up to 32) temporary registers (r#) are used in this shader
	unsigned numUniqueTempRegistersUsed; // Count of how many 1 bits are in the tempRegistersUsedBitmask
	unsigned short inputRegistersUsedBitmask; // A bitmask containing which of the (up to 16) input registers (v#) are used in this shader
	unsigned short numUniqueInputRegistersUsed; // Count of how many 1 bits are in the inputRegistersUsedBitmask
	bool usesDynamicBranching; // Specifically this refers to dynamic branching that can cause warp divergence
	bool usesFunctionCalls; // Does this shader have any function calls?
	bool usesIntToFloatConversions; // Checks to see if you're reading from integer constants into floating-point registers (this is slow)
	bool usesTexkill; // Only for pixel shaders, noted for disabling fast-Z pass
	bool usesGradientInstructions; // Only for pixel shaders
	bool usesInstructionPredication; // Whether any instructions use Predication (ie., read from the Predication register)
	bool usesInstructionCoIssue; // Whether any instructions use Co-Issuing (executing scalar alpha-channel instructions simultaneously with vector RGB-channel instructions)
	bool psWritesDepth; // Only for pixel shaders, does this pixel shader write out depth (disables fast-Z pass)

	// Only relevant for VS_3_0 shaders:
	bool usesVertTextureFetch;

	// Pointer to the start of the shader bytecode (this is going to always point to the VERSION token)
	const DWORD* initialBytecodeToken;

	// Pointer to the first instruction (after the VERSION token, and also skipping any COMMENT blocks that are immediately adjacent to it)
	const DWORD* firstInstructionToken;

	// Initial constant register values (defines)
	std::vector<InitialConstantValue> initialConstantValues;
	std::vector<InitialConstantValueI> initialConstantValuesI;
	std::vector<InitialConstantValueB> initialConstantValuesB;

	// Which constants registers are accessed during this shader (overlaps with initial constant values):
	std::vector<unsigned> usedConstantsF;
	std::vector<unsigned> usedConstantsI;
	std::vector<unsigned> usedConstantsB;

	// declared inputs and samplers
	std::vector<DeclaredRegister> declaredRegisters;

	// Which output registers are actually written to
	std::vector<WrittenOutputRegister> writtenOutputRegisters;

	// Labels (used as targets for calls):
	std::vector<ShaderLabel> labels;

	// Comments
	std::vector<ShaderComment> comments;

#ifdef DISASM_SHADER
	std::vector<char> shaderDisasmText;
#endif

	// Used with fixed-function shaders only:
	std::vector<D3DXMACRO> fixedFunctionMacroDefines;

	unsigned shaderBytecodeHash;

	// Only relevant for pixel shaders:
	unsigned char usedMRTMask; // Which render targets does this shader output to (as a bit-mask)?
	bool usesMRT; // Does this pixel shader use MRT (multiple render targets) as output?
	bool hasDependentTextureFetches; // Not yet implemented (always false)

	static const opcodeDisplayType GetOpcodeDisplayType(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode);

	// Returns the opcode as a string suitable for diassembly
	static const char* const GetOpcodeString(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode);

	// Returns the name of the function that this opcode maps to
	static const char* const GetOpcodeFunctionString(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode);

	// Returns true if this opcode is a TEX instruction, or false otherwise
	static const bool IsOpcodeTexInstruction(const D3DSHADER_INSTRUCTION_OPCODE_TYPE opcode);
};

void DisasmAndAnalyzeShader(const DWORD* shaderMemory, ShaderInfo& shaderInfoOut
#ifdef _DEBUG
	, const char* const filename
#endif
	);

void AnalyzeShader(const DWORD* shaderMemory, ShaderInfo& shaderInfoOut
#ifdef _DEBUG
	, const char* const filename
#endif
	);
