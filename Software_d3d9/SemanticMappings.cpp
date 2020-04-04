#pragma once

#include "SemanticMappings.h"
#include "IDirect3DVertexDeclaration9Hook.h"
#include "IDirect3DVertexShader9Hook.h"
#include "IDirect3DPixelShader9Hook.h"

// Computes a mapping between vertex streams and vertex shader inputs
void DeclarationSemanticMapping::ComputeMappingVS(const IDirect3DVertexDeclaration9Hook* const vertexDecl, const IDirect3DVertexShader9Hook* const vertexShader)
{
	const ShaderInfo& vertexShaderInfo = vertexShader->GetShaderInfo();
	const std::vector<DeclaredRegister>& declaredRegisters = vertexShaderInfo.declaredRegisters;
	const unsigned numDeclaredRegisters = declaredRegisters.size();
	for (unsigned x = 0; x < numDeclaredRegisters; ++x)
	{
		const DeclaredRegister& thisRegister = declaredRegisters[x];
		if (thisRegister.registerType == D3DSPR_INPUT)
		{
			const DebuggableD3DVERTEXELEMENT9* const element = vertexDecl->GetVertexElement(thisRegister.usageType, thisRegister.usageIndex);
			if (element)
			{
				// TODO: Make this work with multiple vertex streams
				vals[thisRegister.usageType][thisRegister.usageIndex] = element;
			}
		}
	}
}

// Computes a mapping between vertex shader outputs and the postransformed vertex declaration
void DeclarationSemanticMapping::ComputeMappingVStoPS(const IDirect3DVertexDeclaration9Hook* const vertexDecl, const IDirect3DVertexShader9Hook* const vertexShader)
{
	const ShaderInfo& vertexShaderInfo = vertexShader->GetShaderInfo();
	const std::vector<DeclaredRegister>& declaredRegisters = vertexShaderInfo.declaredRegisters;
	const unsigned numDeclaredRegisters = declaredRegisters.size();

	if (vertexShaderInfo.shaderMajorVersion == 3)
	{
		for (unsigned x = 0; x < numDeclaredRegisters; ++x)
		{
			const DeclaredRegister& thisRegister = declaredRegisters[x];
			if (thisRegister.registerType == D3DSPR_OUTPUT)
			{
				D3DDECLUSAGE outputUsage = thisRegister.usageType;
				if (outputUsage == D3DDECLUSAGE_POSITION)
					outputUsage = D3DDECLUSAGE_POSITIONT;
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDecl->GetVertexElement(outputUsage, thisRegister.usageIndex);
				if (element)
				{
					// TODO: Make this work with multiple vertex streams
					vals[outputUsage][thisRegister.usageIndex] = element;
				}
#ifdef _DEBUG
				else
				{
					// Could not find usage + index in the stream! Maybe this vertex shader is bound to the wrong vertex declaration?
					OutputDebugStringA("Error: vs_3_0 Element could not be found in stream. Maybe this vertex shader is bound to the wrong pixel shader?\n");
				}
#endif
			}
		}
	}
	else if (vertexShaderInfo.shaderMajorVersion == 2)
	{
		const unsigned numWrittenRegisters = vertexShaderInfo.writtenOutputRegisters.size();
		for (unsigned x = 0; x < numWrittenRegisters; ++x)
		{
			const WrittenOutputRegister& writtenRegister = vertexShaderInfo.writtenOutputRegisters[x];
			D3DDECLUSAGE outputUsage = D3DDECLUSAGE_TEXCOORD;
			unsigned registerIndex = writtenRegister.registerIndex;
			switch (writtenRegister.registerType)
			{
			case D3DSPR_RASTOUT:
				switch (writtenRegister.registerIndex)
				{
				default:
#ifdef _DEBUG
					DbgBreakPrint("Error: Shader RASTOUT register type is undefined");
#endif
				case D3DSRO_POSITION:
					outputUsage = D3DDECLUSAGE_POSITIONT;
					registerIndex = 0;
					break;
				case D3DSRO_FOG:
					outputUsage = D3DDECLUSAGE_FOG;
					registerIndex = 0;
					break;
				case D3DSRO_POINT_SIZE:
					outputUsage = D3DDECLUSAGE_PSIZE;
					registerIndex = 0;
					break;
				}
				break;
			default:
#ifdef _DEBUG
				DbgBreakPrint("Error: Shader register type is undefined");
#endif
			case D3DSPR_ATTROUT:
				// Uhhhhhhhhhhhhh, let's just pretend we're a texcoord like everything else!
			case D3DSPR_TEXCRDOUT:
				outputUsage = D3DDECLUSAGE_TEXCOORD;
				break;
			case D3DSPR_COLOROUT:
				outputUsage = D3DDECLUSAGE_COLOR;
				break;
			case D3DSPR_DEPTHOUT:
				outputUsage = D3DDECLUSAGE_DEPTH;
				break;
			}

			const DebuggableD3DVERTEXELEMENT9* const element = vertexDecl->GetVertexElementOutput(outputUsage, writtenRegister.registerIndex);
			if (element)
			{
				vals[outputUsage][writtenRegister.registerIndex] = element;
			}
#ifdef _DEBUG
			else
			{
				// Could not find usage + index in the stream! Maybe this vertex shader is bound to the wrong vertex declaration?
				OutputDebugStringA("Error: vs_2_0 Element could not be found in stream. Maybe this vertex shader is bound to the wrong pixel shader?\n");
			}
#endif
		}
	}
#ifdef _DEBUG
	else
	{
		DbgBreakPrint("Error: Unknown pixel shader version (not 2 or 3)");
	}
#endif
}

// Computes a mapping between vertex streams and pixel shader inputs (for POSITIONT decls)
void DeclarationSemanticMapping::ComputeMappingPS(const IDirect3DVertexDeclaration9Hook* const vertexDecl, const IDirect3DPixelShader9Hook* const pixelShader)
{
	const ShaderInfo& pixelShaderInfo = pixelShader->GetShaderInfo();
	const std::vector<DeclaredRegister>& declaredRegisters = pixelShaderInfo.declaredRegisters;
	const unsigned numDeclaredRegisters = declaredRegisters.size();
	for (unsigned x = 0; x < numDeclaredRegisters; ++x)
	{
		const DeclaredRegister& thisRegister = declaredRegisters[x];
		if (pixelShaderInfo.shaderMajorVersion == 3)
		{
			if (thisRegister.registerType == D3DSPR_INPUT)
			{
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDecl->GetVertexElement(thisRegister.usageType, thisRegister.usageIndex);
				if (element)
				{
					// TODO: Make this work with multiple vertex streams
					vals[thisRegister.usageType][thisRegister.usageIndex] = element;
				}
#ifdef _DEBUG
				else
				{
					// Could not find usage + index in the stream! Maybe this vertex shader is bound to the wrong vertex declaration?
					OutputDebugStringA("Error: ps_3_0 D3DSPR_INPUT element could not be found in stream. Maybe this pixel shader is bound to the wrong vertex declaration?\n");
				}
#endif
			}
		}
		else if (pixelShaderInfo.shaderMajorVersion == 2)
		{
			if (thisRegister.registerType == D3DSPR_INPUT) // color (diffuse, specular) register
			{
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDecl->GetVertexElement(D3DDECLUSAGE_COLOR, thisRegister.usageIndex);
				if (element)
				{
					// TODO: Make this work with multiple vertex streams
					vals[D3DDECLUSAGE_COLOR][thisRegister.usageIndex] = element;
				}
#ifdef _DEBUG
				else
				{
					// Could not find usage + index in the stream! Maybe this vertex shader is bound to the wrong vertex declaration?
					OutputDebugStringA("Error: ps_2_0 D3DSPR_INPUT element could not be found in stream. Maybe this pixel shader is bound to the wrong vertex declaration?\n");
				}
#endif
			}
			else if (thisRegister.registerType == D3DSPR_TEXTURE) // texcoord register
			{
				const DebuggableD3DVERTEXELEMENT9* const element = vertexDecl->GetVertexElement(D3DDECLUSAGE_TEXCOORD, thisRegister.usageIndex);
				if (element)
				{
					// TODO: Make this work with multiple vertex streams
					vals[D3DDECLUSAGE_TEXCOORD][thisRegister.usageIndex] = element;
				}
#ifdef _DEBUG
				else
				{
					// Could not find usage + index in the stream! Maybe this vertex shader is bound to the wrong vertex declaration?
					OutputDebugStringA("Error: ps_2_0 D3DSPR_TEXTURE element could not be found in stream. Maybe this pixel shader is bound to the wrong vertex declaration?\n");
				}
#endif
			}
		}
#ifdef _DEBUG
		else
		{
			// Don't support ps_1_4 or anything
			DbgBreakPrint("Error: Unknown pixel shader version specified (not 2 or 3)");
		}
#endif
	}
}

void VStoPSMapping::ComputeMappingVSToPS(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps)
{
#ifdef _DEBUG
	if (!vs)
	{
		DbgBreakPrint("Error: Vertex shader is NULL");
	}
	if (!ps)
	{
		DbgBreakPrint("Error: Pixel shader is NULL");
	}
#endif

	const ShaderInfo& vsShaderInfo = vs->GetShaderInfo();
	const ShaderInfo& psShaderInfo = ps->GetShaderInfo();

	if (vsShaderInfo.shaderMajorVersion == 1 || vsShaderInfo.shaderMajorVersion == 2) // vs_1 and vs_2
	{
		if (psShaderInfo.shaderMajorVersion == 1) // ps_1
			ComputeMappingVS2ToPS1(vs, ps);
		else if (psShaderInfo.shaderMajorVersion == 2) // ps_2
			ComputeMappingVS2ToPS2(vs, ps);
		else if (psShaderInfo.shaderMajorVersion == 3) // ps_3
			ComputeMappingVS2ToPS3(vs, ps);
#ifdef _DEBUG
		else
		{
			DbgBreakPrint("Error: Pixel shader version is unknown (not 1, 2, or 3)");
		}
#endif
	}
	else if (vsShaderInfo.shaderMajorVersion == 3) // vs_3
	{
		if (psShaderInfo.shaderMajorVersion == 1) // ps_1
			ComputeMappingVS3ToPS1(vs, ps);
		if (psShaderInfo.shaderMajorVersion == 2) // ps_2
			ComputeMappingVS3ToPS2(vs, ps);
		else if (psShaderInfo.shaderMajorVersion == 3) // ps_3
			ComputeMappingVS3ToPS3(vs, ps);
#ifdef _DEBUG
		else
		{
			DbgBreakPrint("Error: Pixel shader version is unknown (not 2 or 3)");
		}
#endif
	}
#ifdef _DEBUG
	else
	{
		DbgBreakPrint("Error: Vertex shader version is unknown (not 1, 2, or 3)");
	}
#endif
}

// VS1/VS2 to PS1:
// Directly go across -> 2colors + 6texcoords
void VStoPSMapping::ComputeMappingVS2ToPS1(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps)
{
	#ifdef _DEBUG
	if (!vs)
	{
		DbgBreakPrint("Error: Vertex shader is NULL");
	}
	if (!ps)
	{
		DbgBreakPrint("Error: Pixel shader is NULL");
	}
#endif

	// ps_1_* doesn't use input declarations
	const ShaderInfo& psShaderInfo = ps->GetShaderInfo();
	for (unsigned char t = 0; t < (D3DMCS_COLOR2 + 6); ++t)
	{
		if (psShaderInfo.inputRegistersUsedBitmask & (1 << t) )
		{
			psInputRegistersUnion.ps_2_0_registers.texCoords[t] = t;
		}
	}
}

// VS1/VS2 to PS2:
// Directly go across -> 2colors + 8texcoords
void VStoPSMapping::ComputeMappingVS2ToPS2(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps)
{
#ifdef _DEBUG
	if (!vs)
	{
		DbgBreakPrint("Error: Vertex shader is NULL");
	}
	if (!ps)
	{
		DbgBreakPrint("Error: Pixel shader is NULL");
	}
#endif

	const ShaderInfo& psShaderInfo = ps->GetShaderInfo();
	const std::vector<DeclaredRegister>& declaredRegs = psShaderInfo.declaredRegisters;
	for (unsigned x = 0; x < declaredRegs.size(); ++x)
	{
		const DeclaredRegister& reg = declaredRegs[x];
		switch (reg.registerType)
		{
		case D3DSPR_INPUT: // color (diffuse, specular) register
			psInputRegistersUnion.ps_2_0_registers.colors[reg.registerIndex] = reg.usageIndex;
			break;
		case D3DSPR_TEXTURE: // texcoord register
			psInputRegistersUnion.ps_2_0_registers.texCoords[reg.registerIndex] = reg.usageIndex + D3DMCS_COLOR2;
			break;
		default:
			break;
		}
	}
}

// VS1/VS2 to PS3:
// Use semantics to map from 2colors + 8texcoords to 10 PS input registers
void VStoPSMapping::ComputeMappingVS2ToPS3(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps)
{
#ifdef _DEBUG
	if (!vs)
	{
		DbgBreakPrint("Error: Vertex shader is NULL");
	}
	if (!ps)
	{
		DbgBreakPrint("Error: Pixel shader is NULL");
	}
#endif

	const ShaderInfo& psShaderInfo = ps->GetShaderInfo();
	const std::vector<DeclaredRegister>& psDeclaredRegs = psShaderInfo.declaredRegisters;
	const unsigned numPsDeclRegs = psDeclaredRegs.size();
	for (unsigned x = 0; x < numPsDeclRegs; ++x)
	{
		const DeclaredRegister& psReg = psDeclaredRegs[x];
		if (psReg.registerType != D3DSPR_INPUT) // Interpolator register
			continue;

		switch (psReg.usageType)
		{
		case D3DDECLUSAGE_COLOR:
			psInputRegistersUnion.ps_3_0_registers.inputs[psReg.registerIndex] = psReg.usageIndex;
			break;
		default:
#ifdef _DEBUG
			DbgBreakPrint("Error: Unexpected shader register usage type for PS_2_0");
#endif
		case D3DDECLUSAGE_TEXCOORD:
			psInputRegistersUnion.ps_3_0_registers.inputs[psReg.registerIndex] = psReg.usageIndex + D3DMCS_COLOR2;
			break;
		}
	}
}

// VS3 to PS2:
// Map from 12 VS output registers to 2colors + 6texcoords (anything that isn't a color goes into texcoords)
void VStoPSMapping::ComputeMappingVS3ToPS1(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps)
{
	#ifdef _DEBUG
	if (!vs)
	{
		DbgBreakPrint("Error: Vertex shader is NULL");
	}
	if (!ps)
	{
		DbgBreakPrint("Error: Pixel shader is NULL");
	}
#endif

	const ShaderInfo& vsShaderInfo = vs->GetShaderInfo();
	const std::vector<DeclaredRegister>& vsDeclaredRegs = vsShaderInfo.declaredRegisters;
	const unsigned numVsDeclRegs = vsDeclaredRegs.size();
	const ShaderInfo& psShaderInfo = ps->GetShaderInfo();
	for (unsigned char t = 0; t < 8; ++t)
	{
		psInputRegistersUnion.ps_3_0_registers.inputs[t] = 0;
		if (psShaderInfo.inputRegistersUsedBitmask & (1 << t) )
		{
			for (unsigned y = 0; y < numVsDeclRegs; ++y)
			{
				const DeclaredRegister& vsReg = vsDeclaredRegs[y];
				if (!vsReg.isOutputRegister)
					continue;
				if (vsReg.usageType == D3DDECLUSAGE_COLOR && vsReg.usageIndex == t)
					psInputRegistersUnion.ps_3_0_registers.inputs[t] = vsReg.registerIndex;
				else if (vsReg.usageType == D3DDECLUSAGE_TEXCOORD && vsReg.usageIndex == t - D3DMCS_COLOR2)
					psInputRegistersUnion.ps_3_0_registers.inputs[t] = vsReg.registerIndex + D3DMCS_COLOR2;
			}
		}
	}
	const std::vector<DeclaredRegister>& psDeclaredRegs = psShaderInfo.declaredRegisters;
	const unsigned numPsDeclRegs = psDeclaredRegs.size();
	for (unsigned x = 0; x < numPsDeclRegs; ++x)
	{
		const DeclaredRegister& psReg = psDeclaredRegs[x];
		switch (psReg.registerType)
		{
		case D3DSPR_INPUT: // color (diffuse, specular) register
			psInputRegistersUnion.ps_2_0_registers.colors[psReg.registerIndex] = 0;
			for (unsigned y = 0; y < numVsDeclRegs; ++y)
			{
				const DeclaredRegister& vsReg = vsDeclaredRegs[y];
				if (!vsReg.isOutputRegister)
					continue;
				if (vsReg.usageType == D3DDECLUSAGE_COLOR && vsReg.usageIndex == psReg.usageIndex)
				{
					psInputRegistersUnion.ps_2_0_registers.colors[psReg.registerIndex] = vsReg.registerIndex;
					break;
				}
			}
			break;
		case D3DSPR_TEXTURE: // texcoord register
			psInputRegistersUnion.ps_2_0_registers.texCoords[psReg.registerIndex] = 0;
			for (unsigned y = 0; y < numVsDeclRegs; ++y)
			{
				const DeclaredRegister& vsReg = vsDeclaredRegs[y];
				if (!vsReg.isOutputRegister)
					continue;
				if (vsReg.usageType == D3DDECLUSAGE_TEXCOORD && vsReg.usageIndex == psReg.usageIndex)
				{
					psInputRegistersUnion.ps_2_0_registers.colors[psReg.registerIndex] = vsReg.registerIndex;
					break;
				}
			}
			break;
		default:
			break;
		}
	}
}

// VS3 to PS2:
// Use semantics to map from 12 VS output registers to 2colors + 8texcoords (anything that isn't a color goes into texcoords)
void VStoPSMapping::ComputeMappingVS3ToPS2(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps)
{
#ifdef _DEBUG
	if (!vs)
	{
		DbgBreakPrint("Error: Vertex shader is NULL");
	}
	if (!ps)
	{
		DbgBreakPrint("Error: Pixel shader is NULL");
	}
#endif

	const ShaderInfo& vsShaderInfo = vs->GetShaderInfo();
	const std::vector<DeclaredRegister>& vsDeclaredRegs = vsShaderInfo.declaredRegisters;
	const unsigned numVsDeclRegs = vsDeclaredRegs.size();
	const ShaderInfo& psShaderInfo = ps->GetShaderInfo();
	const std::vector<DeclaredRegister>& psDeclaredRegs = psShaderInfo.declaredRegisters;
	const unsigned numPsDeclRegs = psDeclaredRegs.size();
	for (unsigned x = 0; x < numPsDeclRegs; ++x)
	{
		const DeclaredRegister& psReg = psDeclaredRegs[x];
		switch (psReg.registerType)
		{
		case D3DSPR_INPUT: // color (diffuse, specular) register
			psInputRegistersUnion.ps_2_0_registers.colors[psReg.registerIndex] = 0;
			for (unsigned y = 0; y < numVsDeclRegs; ++y)
			{
				const DeclaredRegister& vsReg = vsDeclaredRegs[y];
				if (!vsReg.isOutputRegister)
					continue;
				if (vsReg.usageType == D3DDECLUSAGE_COLOR && vsReg.usageIndex == psReg.usageIndex)
				{
					psInputRegistersUnion.ps_2_0_registers.colors[psReg.registerIndex] = vsReg.registerIndex;
					break;
				}
			}
			break;
		case D3DSPR_TEXTURE: // texcoord register
			psInputRegistersUnion.ps_2_0_registers.texCoords[psReg.registerIndex] = 0;
			for (unsigned y = 0; y < numVsDeclRegs; ++y)
			{
				const DeclaredRegister& vsReg = vsDeclaredRegs[y];
				if (!vsReg.isOutputRegister)
					continue;
				if (vsReg.usageType == D3DDECLUSAGE_TEXCOORD && vsReg.usageIndex == psReg.usageIndex)
				{
					psInputRegistersUnion.ps_2_0_registers.colors[psReg.registerIndex] = vsReg.registerIndex;
					break;
				}
			}
			break;
		default:
			break;
		}
	}
}

// VS3 to PS3:
// Map across non-rasterizer VS outputs (no POSITIONT, no FOG, no PSIZE, etc.)
void VStoPSMapping::ComputeMappingVS3ToPS3(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps)
{
#ifdef _DEBUG
	if (!vs)
	{
		DbgBreakPrint("Error: Vertex shader is NULL");
	}
	if (!ps)
	{
		DbgBreakPrint("Error: Pixel shader is NULL");
	}
#endif

	const ShaderInfo& vsShaderInfo = vs->GetShaderInfo();
	const std::vector<DeclaredRegister>& vsDeclaredRegs = vsShaderInfo.declaredRegisters;
	const unsigned numVsDeclRegs = vsDeclaredRegs.size();
	const ShaderInfo& psShaderInfo = ps->GetShaderInfo();
	const std::vector<DeclaredRegister>& psDeclaredRegs = psShaderInfo.declaredRegisters;
	const unsigned numPsDeclRegs = psDeclaredRegs.size();
	for (unsigned x = 0; x < numPsDeclRegs; ++x)
	{
		const DeclaredRegister& psReg = psDeclaredRegs[x];
		if (psReg.registerType != D3DSPR_INPUT) // Interpolator register
			continue;

		// Default to mapping across all registers
		psInputRegistersUnion.ps_3_0_registers.inputs[psReg.registerIndex] = x;

		for (unsigned y = 0; y < numVsDeclRegs; ++y)
		{
			const DeclaredRegister& vsReg = vsDeclaredRegs[y];
			if (!vsReg.isOutputRegister)
				continue;
			if (vsReg.usageType == psReg.usageType && vsReg.usageIndex == psReg.usageIndex)
			{
				psInputRegistersUnion.ps_3_0_registers.inputs[psReg.registerIndex] = vsReg.registerIndex;
				break;
			}
		}
	}
}
