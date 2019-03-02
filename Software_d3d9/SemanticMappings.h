#pragma once

#include "GlobalToggles.h"
#include "IDirect3DDevice9Hook.h"

// Mapping from vertex declarations and vertex streams to vertex shader inputs (or from vertex streams to pixel shader inputs for POSITIONT vert decls):
struct DeclarationSemanticMapping
{
	DeclarationSemanticMapping()
	{
		ClearSemanticMapping();
	}

	inline void ClearSemanticMapping()
	{
		for (unsigned x = 0; x <= MAXD3DDECLUSAGE; ++x)
			for (unsigned y = 0; y <= MAXD3DDECLUSAGEINDEX; ++y)
				vals[x][y] = NULL;
	}

	// Computes a mapping between vertex streams and vertex shader inputs
	void ComputeMappingVS(const IDirect3DVertexDeclaration9Hook* const vertexDecl, const IDirect3DVertexShader9Hook* const vertexShader);

	// Computes a mapping between vertex shader outputs and the postransformed vertex declaration
	void ComputeMappingVStoPS(const IDirect3DVertexDeclaration9Hook* const vertexDecl, const IDirect3DVertexShader9Hook* const vertexShader);

	// Computes a mapping between vertex streams and pixel shader inputs (for POSITIONT decls)
	void ComputeMappingPS(const IDirect3DVertexDeclaration9Hook* const vertexDecl, const IDirect3DPixelShader9Hook* const pixelShader);

	// Returns the address of the register within the input vertex stream given the usage and index in the map
	inline const D3DXVECTOR4* const GetAddress(const D3DDECLUSAGE usage, const unsigned index, const D3DXVECTOR4* const baseAddr) const
	{
		const DebuggableD3DVERTEXELEMENT9* const offsetElement = vals[usage][index];
		return baseAddr + offsetElement->Offset;
	}

	const DebuggableD3DVERTEXELEMENT9* vals[MAXD3DDECLUSAGE + 1][MAXD3DDECLUSAGEINDEX + 1];
};

// Mapping from vertex shader outputs to pixel shader inputs:
struct VStoPSMapping
{
	VStoPSMapping()
	{
		ClearSemanticMapping();
	}

	inline void ClearSemanticMapping()
	{
		memset(this, 0, sizeof(*this) );
	}

	void ComputeMappingVSToPS(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps);

	// VS1/VS2 to PS1:
	// Directly go across -> 2colors + 6texcoords
	void ComputeMappingVS2ToPS1(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps);

	// VS1/VS2 to PS2:
	// Directly go across -> 2colors + 8texcoords
	void ComputeMappingVS2ToPS2(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps);

	// VS1/VS2 to PS3:
	// Use semantics to map from 2colors + 8texcoords to 10 PS input registers
	void ComputeMappingVS2ToPS3(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps);

	// VS3 to PS2:
	// Map from 12 VS output registers to 2colors + 6texcoords (anything that isn't a color goes into texcoords)
	void ComputeMappingVS3ToPS1(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps);

	// VS3 to PS2:
	// Use semantics to map from 12 VS output registers to 2colors + 8texcoords (anything that isn't a color goes into texcoords)
	void ComputeMappingVS3ToPS2(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps);

	// VS3 to PS3:
	// Map across non-rasterizer VS outputs (no POSITIONT, no FOG, no PSIZE, etc.)
	void ComputeMappingVS3ToPS3(const IDirect3DVertexShader9Hook* const vs, const IDirect3DPixelShader9Hook* const ps);

	union _psInputRegistersUnion
	{
		struct _ps_2_0_registers
		{
			unsigned colors[D3DMCS_COLOR2]; // 2 colors
			unsigned texCoords[D3DDP_MAXTEXCOORD]; // 8 texcoords
		} ps_2_0_registers;

		struct _ps_3_0_registers
		{
			unsigned inputs[10]; // 10 pixel shader inputs
		} ps_3_0_registers;
	} psInputRegistersUnion;
};
