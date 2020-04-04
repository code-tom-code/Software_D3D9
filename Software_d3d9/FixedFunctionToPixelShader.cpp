#include "FixedFunctionToShader.h"
#include "IDirect3DPixelShader9Hook.h"
#include "IDirect3DTexture9Hook.h"
#include "resource.h"

static const unsigned char TFACTOR_REGISTER = 0u; // c0
static const unsigned char FOGCOLOR_REGISTER = 1u; // c1
static const unsigned char TEXTURESTAGE_CONSTANTS_REGISTERS = 2u; // c2 thru c9
static const unsigned char STAGE_BUMPENVMAT_REGISTERS = 10u; // c10 thru c27

const FixedFunctionStateHash HashPixelState(const DeviceState& state)
{
	FixedFunctionStateHash retHash = 0;

	// Fixed function pixel pipeline cares about:
	// - Texture state stages
	// - Texture types for each stage (1D, 2D, 3D, Cubemap textures)

	for (unsigned x = 0; x < MAX_NUM_TEXTURE_STAGE_STATES; ++x)
	{
		const TextureStageState& thisTSS = state.currentStageStates[x];
		HashStruct(retHash, thisTSS);
	}

	for (unsigned x = 0; x < MAX_NUM_TEXTURE_STAGE_STATES; ++x)
	{
		if (state.currentTextures[x] != NULL) // tex2D or tex1D bound to this stage
		{
		}
		else if (state.currentCubeTextures[x] != NULL) // texCube bound to this stage
		{
		}
		else if (state.currentVolumeTextures[x] != NULL) // tex3D bound to this stage
		{
		}
		else // NULL texture bound to this stage
		{
		}
	}

	// Render states:
	// - Fog
	// - Texture factor (shared by all texture stages)
	// - Shading mode (gourad vs. flat shading)

	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.lighting);
	if (state.currentRenderStates.renderStatesUnion.namedStates.lighting)
	{
		HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.specularEnable);
	}

	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogEnable);
	if (state.currentRenderStates.renderStatesUnion.namedStates.fogEnable)
	{
		HashContinue<unsigned char>(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogVertexMode);
		HashContinue<unsigned char>(retHash, state.currentRenderStates.renderStatesUnion.namedStates.fogTableMode);
		HashContinue<unsigned char>(retHash, state.currentRenderStates.renderStatesUnion.namedStates.rangeFogEnable);
	}
	HashContinue(retHash, state.currentRenderStates.renderStatesUnion.namedStates.textureFactor);
	HashContinue<unsigned char>(retHash, state.currentRenderStates.renderStatesUnion.namedStates.shadeMode);

	return retHash;
}

static const char* const coordIndices[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7"
};

static const char* const D3DTSS_TEXCOORDINDEXNStrings[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"D3DTSS_TEXCOORDINDEX0",
	"D3DTSS_TEXCOORDINDEX1",
	"D3DTSS_TEXCOORDINDEX2",
	"D3DTSS_TEXCOORDINDEX3",
	"D3DTSS_TEXCOORDINDEX4",
	"D3DTSS_TEXCOORDINDEX5",
	"D3DTSS_TEXCOORDINDEX6",
	"D3DTSS_TEXCOORDINDEX7"
};

static const char* const HAS_TEXN_BOUNDStrings[MAX_NUM_TEXTURE_STAGE_STATES] = 
{
	"HAS_TEX0_BOUND",
	"HAS_TEX1_BOUND",
	"HAS_TEX2_BOUND",
	"HAS_TEX3_BOUND",
	"HAS_TEX4_BOUND",
	"HAS_TEX5_BOUND",
	"HAS_TEX6_BOUND",
	"HAS_TEX7_BOUND"
};

static const char* const HAS_TEXCOORDNStrings[MAX_NUM_TEXTURE_STAGE_STATES] = 
{
	"HAS_TEXCOORD0",
	"HAS_TEXCOORD1",
	"HAS_TEXCOORD2",
	"HAS_TEXCOORD3",
	"HAS_TEXCOORD4",
	"HAS_TEXCOORD5",
	"HAS_TEXCOORD6",
	"HAS_TEXCOORD7"
};

static const char* const SamplerTypeNStrings[MAX_NUM_TEXTURE_STAGE_STATES] = 
{
	"SAMPLERTYPE0",
	"SAMPLERTYPE1",
	"SAMPLERTYPE2",
	"SAMPLERTYPE3",
	"SAMPLERTYPE4",
	"SAMPLERTYPE5",
	"SAMPLERTYPE6",
	"SAMPLERTYPE7"
};

static const char* const TexTypeNStrings[MAX_NUM_TEXTURE_STAGE_STATES] = 
{
	"TEXTYPE0",
	"TEXTYPE1",
	"TEXTYPE2",
	"TEXTYPE3",
	"TEXTYPE4",
	"TEXTYPE5",
	"TEXTYPE6",
	"TEXTYPE7"
};

static const char* const TexTypeEnumStrings[4 << 1] = 
{
	"TEXTYPE1D",
	"TEXTYPE2D",
	"TEXTYPE3D",
	"TEXTYPECUBE",
	"TEXTYPE1D | TEXTYPEPROJ",
	"TEXTYPE2D | TEXTYPEPROJ",
	"TEXTYPE3D | TEXTYPEPROJ",
	"TEXTYPECUBE | TEXTYPEPROJ"
};

static const char* const BoundTextureTypeStrings[4] =
{
	"sampler1D",
	"sampler2D",
	"sampler3D",
	"samplerCUBE"
};

enum boundTextureType
{
	TEXTYPE1D = 0x0,
	TEXTYPE2D = 0x1,
	TEXTYPE3D = 0x2,
	TEXTYPECUBE = 0x3,
	TEXTYPEPROJ = 0x4, // This is not a texture type per-se, but a flag on top of the texture type
};

static const char* const TEXCOORDNTYPEStrings[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"TEXCOORD0TYPE",
	"TEXCOORD1TYPE",
	"TEXCOORD2TYPE",
	"TEXCOORD3TYPE",
	"TEXCOORD4TYPE",
	"TEXCOORD5TYPE",
	"TEXCOORD6TYPE",
	"TEXCOORD7TYPE"
};

static const char* const D3DTTFF_STRINGS[4] =
{
	"D3DTTFF_COUNT1TYPE",
	"D3DTTFF_COUNT2TYPE",
	"D3DTTFF_COUNT3TYPE",
	"D3DTTFF_COUNT4TYPE"
};

static const char* const d3dtopStrings[D3DTOP_LERP + 1] =
{
	"D3DTOP_UNKNOWN (0)",
	"D3DTOP_DISABLE",                   
	"D3DTOP_SELECTARG1",                
	"D3DTOP_SELECTARG2",                
	"D3DTOP_MODULATE",                  
	"D3DTOP_MODULATE2X",                
	"D3DTOP_MODULATE4X",                
	"D3DTOP_ADD",                       
	"D3DTOP_ADDSIGNED",                 
	"D3DTOP_ADDSIGNED2X",               
	"D3DTOP_SUBTRACT",                  
	"D3DTOP_ADDSMOOTH",                 
	"D3DTOP_BLENDDIFFUSEALPHA",         
	"D3DTOP_BLENDTEXTUREALPHA",         
	"D3DTOP_BLENDFACTORALPHA",          
	"D3DTOP_BLENDTEXTUREALPHAPM",       
	"D3DTOP_BLENDCURRENTALPHA",         
	"D3DTOP_PREMODULATE",               
	"D3DTOP_MODULATEALPHA_ADDCOLOR",    
	"D3DTOP_MODULATECOLOR_ADDALPHA",    
	"D3DTOP_MODULATEINVALPHA_ADDCOLOR", 
	"D3DTOP_MODULATEINVCOLOR_ADDALPHA", 
	"D3DTOP_BUMPENVMAP",                
	"D3DTOP_BUMPENVMAPLUMINANCE",       
	"D3DTOP_DOTPRODUCT3",               
	"D3DTOP_MULTIPLYADD",               
	"D3DTOP_LERP"
};

static const char* const d3dtaStrings[64] =
{
	"D3DTA_DIFFUSE", 
	"D3DTA_CURRENT",  
	"D3DTA_TEXTURE",  
	"D3DTA_TFACTOR",  
	"D3DTA_SPECULAR", 
	"D3DTA_TEMP",     
	"D3DTA_CONSTANT",
	"UNKNOWN(7)",
	"UNKNOWN(8)",
	"UNKNOWN(9)",
	"UNKNOWN(10)",
	"UNKNOWN(11)",
	"UNKNOWN(12)",
	"UNKNOWN(13)",
	"UNKNOWN(14)",
	"UNKNOWN(15)",

	"D3DTA_DIFFUSE | D3DTA_COMPLEMENT", 
	"D3DTA_CURRENT | D3DTA_COMPLEMENT",  
	"D3DTA_TEXTURE | D3DTA_COMPLEMENT",  
	"D3DTA_TFACTOR | D3DTA_COMPLEMENT",  
	"D3DTA_SPECULAR | D3DTA_COMPLEMENT", 
	"D3DTA_TEMP | D3DTA_COMPLEMENT",     
	"D3DTA_CONSTANT | D3DTA_COMPLEMENT",
	"UNKNOWN | D3DTA_COMPLEMENT(23)",
	"UNKNOWN | D3DTA_COMPLEMENT(24)",
	"UNKNOWN | D3DTA_COMPLEMENT(25)",
	"UNKNOWN | D3DTA_COMPLEMENT(26)",
	"UNKNOWN | D3DTA_COMPLEMENT(27)",
	"UNKNOWN | D3DTA_COMPLEMENT(28)",
	"UNKNOWN | D3DTA_COMPLEMENT(29)",
	"UNKNOWN | D3DTA_COMPLEMENT(30)",
	"UNKNOWN | D3DTA_COMPLEMENT(31)",

	"D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE", 
	"D3DTA_CURRENT | D3DTA_ALPHAREPLICATE",  
	"D3DTA_TEXTURE | D3DTA_ALPHAREPLICATE",  
	"D3DTA_TFACTOR | D3DTA_ALPHAREPLICATE",  
	"D3DTA_SPECULAR | D3DTA_ALPHAREPLICATE", 
	"D3DTA_TEMP | D3DTA_ALPHAREPLICATE",     
	"D3DTA_CONSTANT | D3DTA_ALPHAREPLICATE",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(39)",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(40)",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(41)",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(42)",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(43)",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(44)",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(45)",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(46)",
	"UNKNOWN | D3DTA_ALPHAREPLICATE(47)",

	"D3DTA_DIFFUSE | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE", 
	"D3DTA_CURRENT | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE",  
	"D3DTA_TEXTURE | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE",  
	"D3DTA_TFACTOR | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE",  
	"D3DTA_SPECULAR | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE", 
	"D3DTA_TEMP | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE",     
	"D3DTA_CONSTANT | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE"
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(55)",
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(56)",
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(57)",
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(58)",
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(59)",
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(60)",
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(61)",
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(62)",
	"UNKNOWN | D3DTA_COMPLEMENT | D3DTA_ALPHAREPLICATE(63)"
};

static const char* const STAGEN_COLOROPSTRINGS[MAX_NUM_TEXTURE_STAGE_STATES] = 
{
	"D3DTSS_STAGE0_COLOROP",
	"D3DTSS_STAGE1_COLOROP",
	"D3DTSS_STAGE2_COLOROP",
	"D3DTSS_STAGE3_COLOROP",
	"D3DTSS_STAGE4_COLOROP",
	"D3DTSS_STAGE5_COLOROP",
	"D3DTSS_STAGE6_COLOROP",
	"D3DTSS_STAGE7_COLOROP"
};

static const char* const STAGEN_ALPHAOPSTRINGS[MAX_NUM_TEXTURE_STAGE_STATES] = 
{
	"D3DTSS_STAGE0_ALPHAOP",
	"D3DTSS_STAGE1_ALPHAOP",
	"D3DTSS_STAGE2_ALPHAOP",
	"D3DTSS_STAGE3_ALPHAOP",
	"D3DTSS_STAGE4_ALPHAOP",
	"D3DTSS_STAGE5_ALPHAOP",
	"D3DTSS_STAGE6_ALPHAOP",
	"D3DTSS_STAGE7_ALPHAOP"
};

static const char* const STAGEN_COLORARG1STRINGS[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"D3DTSS_STAGE0_COLORARG1",
	"D3DTSS_STAGE1_COLORARG1",
	"D3DTSS_STAGE2_COLORARG1",
	"D3DTSS_STAGE3_COLORARG1",
	"D3DTSS_STAGE4_COLORARG1",
	"D3DTSS_STAGE5_COLORARG1",
	"D3DTSS_STAGE6_COLORARG1",
	"D3DTSS_STAGE7_COLORARG1"
};

static const char* const STAGEN_COLORARG2STRINGS[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"D3DTSS_STAGE0_COLORARG2",
	"D3DTSS_STAGE1_COLORARG2",
	"D3DTSS_STAGE2_COLORARG2",
	"D3DTSS_STAGE3_COLORARG2",
	"D3DTSS_STAGE4_COLORARG2",
	"D3DTSS_STAGE5_COLORARG2",
	"D3DTSS_STAGE6_COLORARG2",
	"D3DTSS_STAGE7_COLORARG2"
};

static const char* const STAGEN_COLORARG0STRINGS[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"D3DTSS_STAGE0_COLORARG0",
	"D3DTSS_STAGE1_COLORARG0",
	"D3DTSS_STAGE2_COLORARG0",
	"D3DTSS_STAGE3_COLORARG0",
	"D3DTSS_STAGE4_COLORARG0",
	"D3DTSS_STAGE5_COLORARG0",
	"D3DTSS_STAGE6_COLORARG0",
	"D3DTSS_STAGE7_COLORARG0"
};

static const char* const STAGEN_ALPHAARG1STRINGS[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"D3DTSS_STAGE0_ALPHAARG1",
	"D3DTSS_STAGE1_ALPHAARG1",
	"D3DTSS_STAGE2_ALPHAARG1",
	"D3DTSS_STAGE3_ALPHAARG1",
	"D3DTSS_STAGE4_ALPHAARG1",
	"D3DTSS_STAGE5_ALPHAARG1",
	"D3DTSS_STAGE6_ALPHAARG1",
	"D3DTSS_STAGE7_ALPHAARG1"
};

static const char* const STAGEN_ALPHAARG2STRINGS[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"D3DTSS_STAGE0_ALPHAARG2",
	"D3DTSS_STAGE1_ALPHAARG2",
	"D3DTSS_STAGE2_ALPHAARG2",
	"D3DTSS_STAGE3_ALPHAARG2",
	"D3DTSS_STAGE4_ALPHAARG2",
	"D3DTSS_STAGE5_ALPHAARG2",
	"D3DTSS_STAGE6_ALPHAARG2",
	"D3DTSS_STAGE7_ALPHAARG2"
};

static const char* const STAGEN_ALPHAARG0STRINGS[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"D3DTSS_STAGE0_ALPHAARG0",
	"D3DTSS_STAGE1_ALPHAARG0",
	"D3DTSS_STAGE2_ALPHAARG0",
	"D3DTSS_STAGE3_ALPHAARG0",
	"D3DTSS_STAGE4_ALPHAARG0",
	"D3DTSS_STAGE5_ALPHAARG0",
	"D3DTSS_STAGE6_ALPHAARG0",
	"D3DTSS_STAGE7_ALPHAARG0"
};

static const char* const STAGEN_RESULTARGSTRINGS[MAX_NUM_TEXTURE_STAGE_STATES] =
{
	"D3DTSS_STAGE0_RESULTARG",
	"D3DTSS_STAGE1_RESULTARG",
	"D3DTSS_STAGE2_RESULTARG",
	"D3DTSS_STAGE3_RESULTARG",
	"D3DTSS_STAGE4_RESULTARG",
	"D3DTSS_STAGE5_RESULTARG",
	"D3DTSS_STAGE6_RESULTARG",
	"D3DTSS_STAGE7_RESULTARG"
};

static inline const bool GetTextureTypeBoundForStage(const DeviceState& state, const unsigned stageNum, boundTextureType& outBoundTextureType)
{
	if (const IDirect3DTexture9Hook* const texturePtr = state.currentTextures[stageNum])
	{
		if (texturePtr->GetInternalHeight() > 1)
			outBoundTextureType = TEXTYPE2D;
		else
			outBoundTextureType = TEXTYPE1D;
		return true;
	}
	else if (state.currentCubeTextures[stageNum])
	{
		outBoundTextureType = TEXTYPECUBE;
		return true;
	}
	else if (state.currentVolumeTextures[stageNum])
	{
		outBoundTextureType = TEXTYPE3D;
		return true;
	}
	else
	{
		return false;
	}
}

static inline void BuildPixelStateDefines(const DeviceState& state, std::vector<D3DXMACRO>& defines)
{
	if (state.currentRenderStates.renderStatesUnion.namedStates.lighting)
	{
		D3DXMACRO lightingMacro = {0};
		lightingMacro.Name = "WITH_LIGHTING";
		lightingMacro.Definition = "1";
		defines.push_back(lightingMacro);

		if (state.currentRenderStates.renderStatesUnion.namedStates.specularEnable)
		{
			D3DXMACRO specularEnableMacro = {0};
			specularEnableMacro.Name = "SPECULAR_ENABLE";
			specularEnableMacro.Definition = "1";
			defines.push_back(specularEnableMacro);
		}
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.fogEnable)
	{
		D3DXMACRO fogEnableMacro = {0};
		fogEnableMacro.Name = "FOG_ENABLE";
		fogEnableMacro.Definition = "1";
		defines.push_back(fogEnableMacro);
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.colorVertex)
	{
		D3DXMACRO colorVertexMacro = {0};
		colorVertexMacro.Name = "WITH_COLORVERTEX";
		colorVertexMacro.Definition = "1";
		defines.push_back(colorVertexMacro);
	}

	if (true) // TODO: Make this work without vertex color too
	{
		D3DXMACRO vertexColor = {0};
		vertexColor.Name = "HAS_VERTEX_COLOR";
		vertexColor.Definition = "1";
		defines.push_back(vertexColor);
	}

	for (unsigned x = 0; x < MAX_NUM_TEXTURE_STAGE_STATES; ++x)
	{
		const TextureStageState& thisTextureStageState = state.currentStageStates[x];

		D3DXMACRO D3DTSS_TEXCOORDINDEXN = {0};
		D3DTSS_TEXCOORDINDEXN.Name = D3DTSS_TEXCOORDINDEXNStrings[x];
		D3DTSS_TEXCOORDINDEXN.Definition = coordIndices[x];
		defines.push_back(D3DTSS_TEXCOORDINDEXN);

		boundTextureType textureType;
		if (GetTextureTypeBoundForStage(state, x, textureType) )
		{
			D3DXMACRO HAS_TEXN_BOUND = {0};
			HAS_TEXN_BOUND.Name = HAS_TEXN_BOUNDStrings[x];
			HAS_TEXN_BOUND.Definition = "1";
			defines.push_back(HAS_TEXN_BOUND);

			// TODO: Change this to check in with the currently-set FVF or vertex declaration:
			D3DXMACRO HAS_TEXCOORDN = {0};
			HAS_TEXCOORDN.Name = HAS_TEXCOORDNStrings[x];
			HAS_TEXCOORDN.Definition = "1";
			defines.push_back(HAS_TEXCOORDN);

			D3DXMACRO SamplerTypeN = {0};
			SamplerTypeN.Name = SamplerTypeNStrings[x];
			SamplerTypeN.Definition = BoundTextureTypeStrings[textureType];
			defines.push_back(SamplerTypeN);

			D3DXMACRO TexTypeN = {0};
			TexTypeN.Name = TexTypeNStrings[x];
			if (thisTextureStageState.stageStateUnion.namedStates.textureTransformFlags & D3DTTFF_PROJECTED)
				TexTypeN.Definition = TexTypeEnumStrings[textureType | TEXTYPEPROJ];
			else
				TexTypeN.Definition = TexTypeEnumStrings[textureType];
			defines.push_back(TexTypeN);
		}

		{
			D3DXMACRO TEXCOORDNTYPE = {0};
			TEXCOORDNTYPE.Name = TEXCOORDNTYPEStrings[x];
			unsigned stringTableIndex = thisTextureStageState.stageStateUnion.namedStates.textureTransformFlags & 0xF;
			if (stringTableIndex > D3DTTFF_COUNT4)
				stringTableIndex = D3DTTFF_COUNT4;
			if (stringTableIndex == D3DTTFF_DISABLE)
				stringTableIndex = D3DTTFF_COUNT2;
			TEXCOORDNTYPE.Definition = D3DTTFF_STRINGS[stringTableIndex - 1]; // Minus one to convert from a 1-based index to a 0-based index
			defines.push_back(TEXCOORDNTYPE);
		}

		{
			D3DXMACRO STAGEN_COLOROP = {0};
			STAGEN_COLOROP.Name = STAGEN_COLOROPSTRINGS[x];
			STAGEN_COLOROP.Definition = d3dtopStrings[thisTextureStageState.stageStateUnion.namedStates.colorOp];
			defines.push_back(STAGEN_COLOROP);
		}

		{
			D3DXMACRO STAGEN_ALPHAOP = {0};
			STAGEN_ALPHAOP.Name = STAGEN_ALPHAOPSTRINGS[x];
			STAGEN_ALPHAOP.Definition = d3dtopStrings[thisTextureStageState.stageStateUnion.namedStates.alphaOp];
			defines.push_back(STAGEN_ALPHAOP);
		}

		{
			D3DXMACRO STAGEN_COLORARG1 = {0};
			STAGEN_COLORARG1.Name = STAGEN_COLORARG1STRINGS[x];
			STAGEN_COLORARG1.Definition = d3dtaStrings[thisTextureStageState.stageStateUnion.namedStates.colorArg1];
			defines.push_back(STAGEN_COLORARG1);
		}

		{
			D3DXMACRO STAGEN_COLORARG2 = {0};
			STAGEN_COLORARG2.Name = STAGEN_COLORARG2STRINGS[x];
			STAGEN_COLORARG2.Definition = d3dtaStrings[thisTextureStageState.stageStateUnion.namedStates.colorArg2];
			defines.push_back(STAGEN_COLORARG2);
		}

		{
			D3DXMACRO STAGEN_COLORARG0 = {0};
			STAGEN_COLORARG0.Name = STAGEN_COLORARG0STRINGS[x];
			STAGEN_COLORARG0.Definition = d3dtaStrings[thisTextureStageState.stageStateUnion.namedStates.colorArg0];
			defines.push_back(STAGEN_COLORARG0);
		}

		{
			D3DXMACRO STAGEN_ALPHAARG1 = {0};
			STAGEN_ALPHAARG1.Name = STAGEN_ALPHAARG1STRINGS[x];
			STAGEN_ALPHAARG1.Definition = d3dtaStrings[thisTextureStageState.stageStateUnion.namedStates.alphaArg1];
			defines.push_back(STAGEN_ALPHAARG1);
		}

		{
			D3DXMACRO STAGEN_ALPHAARG2 = {0};
			STAGEN_ALPHAARG2.Name = STAGEN_ALPHAARG2STRINGS[x];
			STAGEN_ALPHAARG2.Definition = d3dtaStrings[thisTextureStageState.stageStateUnion.namedStates.alphaArg2];
			defines.push_back(STAGEN_ALPHAARG2);
		}

		{
			D3DXMACRO STAGEN_ALPHAARG0 = {0};
			STAGEN_ALPHAARG0.Name = STAGEN_ALPHAARG0STRINGS[x];
			STAGEN_ALPHAARG0.Definition = d3dtaStrings[thisTextureStageState.stageStateUnion.namedStates.alphaArg0];
			defines.push_back(STAGEN_ALPHAARG0);
		}

		{
			D3DXMACRO STAGEN_RESULTARG = {0};
			STAGEN_RESULTARG.Name = STAGEN_RESULTARGSTRINGS[x];
			STAGEN_RESULTARG.Definition = d3dtaStrings[thisTextureStageState.stageStateUnion.namedStates.resultArg];
			defines.push_back(STAGEN_RESULTARG);
		}

		if (thisTextureStageState.stageStateUnion.namedStates.colorOp == D3DTOP_DISABLE)
		{
			// We can safely stop after we reach the first texture stage that is set to disabled
			break;
		}
	}

	if (!defines.empty() )
	{
		D3DXMACRO emptyLastMacro = {0};
		defines.push_back(emptyLastMacro);
	}
}

void BuildPixelShader(const DeviceState& state, IDirect3DDevice9Hook* const dev, IDirect3DPixelShader9Hook** const outNewShader)
{
#ifdef _DEBUG
	if (!outNewShader)
	{
		__debugbreak();
	}
#endif

	std::vector<D3DXMACRO> defines;
	BuildPixelStateDefines(state, defines);

	LPD3DXBUFFER outBytecode = NULL;
	LPD3DXBUFFER errorMessages = NULL;
	DWORD flags = D3DXSHADER_AVOID_FLOW_CONTROL | D3DXSHADER_PARTIALPRECISION; // Branching isn't currently well-supported in the software shader system
#ifdef _DEBUG
	flags |= D3DXSHADER_DEBUG;
#else
	flags |= D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif

	HRESULT hr = E_FAIL;

	unsigned resourceSize = 0;
	const char* const resourceBytes = (const char* const)GetShaderResourceFile(MAKEINTRESOURCEA(IDR_HLSL_FFPS_SRC), resourceSize);
	if (resourceBytes != NULL)
	{
		hr = D3DXCompileShader( (const char* const)resourceBytes, resourceSize, defines.empty() ? NULL : &defines.front(), D3DXIncludeHandler::GetGlobalIncludeHandlerSingleton(), 
			"main", "ps_3_0", flags, &outBytecode, &errorMessages, NULL);
	}
	if (FAILED(hr) || !outBytecode)
	{
		const char* const errorMessage = errorMessages ? ( (const char* const)errorMessages->GetBufferPointer() ) : NULL;
		printf("%s", errorMessage); // Don't optimize this away

		// Should never happen for fixed-function shaders!
		MessageBoxA(NULL, errorMessage, "Fixed Function PS Compile Failure!", MB_OK);
		__debugbreak();

		return;
	}

#ifdef _DEBUG
	const char* const debugErrorMessage = errorMessages ? ( (const char* const)errorMessages->GetBufferPointer() ) : NULL;
	UNREFERENCED_PARAMETER(debugErrorMessage);
#endif

	IDirect3DPixelShader9* newPixelShader = NULL;
	if (FAILED(dev->CreatePixelShader( (const DWORD* const)outBytecode->GetBufferPointer(), &newPixelShader) ) || !newPixelShader)
	{
		// Should never happen for fixed-function shaders!
		__debugbreak();
	}

	IDirect3DPixelShader9Hook* newPixelShaderHook = dynamic_cast<IDirect3DPixelShader9Hook*>(newPixelShader);
	if (!newPixelShaderHook)
	{
		DbgBreakPrint("Error: CreatePixelShader returned a non-hooked pointer!");
	}

	newPixelShaderHook->GetModifyShaderInfo().fixedFunctionMacroDefines.swap(defines);

	*outNewShader = newPixelShaderHook;

	if (outBytecode)
	{
		outBytecode->Release();
		outBytecode = NULL;
	}
	if (errorMessages)
	{
		errorMessages->Release();
		errorMessages = NULL;
	}
}

void SetFixedFunctionPixelShaderState(const DeviceState& state, IDirect3DDevice9Hook* const dev)
{
	// TFACTOR (c0)
	const D3DCOLOR tfactorColor = state.currentRenderStates.renderStatesUnion.namedStates.textureFactor;
	const D3DXVECTOR4 tfactor( ( (tfactorColor >> 16) & 0xFF) / 255.0f,  // R
		( (tfactorColor >> 8) & 0xFF) / 255.0f,  // G
		(tfactorColor & 0xFF) / 255.0f, 		 // B
		( (tfactorColor >> 24) & 0xFF) / 255.0f); // A
	dev->SetPixelShaderConstantF(TFACTOR_REGISTER, &tfactor.x, 1);

	// FOGCOLOR (c1)
	const D3DCOLOR fogColor = state.currentRenderStates.renderStatesUnion.namedStates.fogColor;
	const D3DXVECTOR4 fogColorVec( ( (fogColor >> 16) & 0xFF) / 255.0f,  // R
		( (fogColor >> 8) & 0xFF) / 255.0f,  // G
		(fogColor & 0xFF) / 255.0f, 		 // B
		( (fogColor >> 24) & 0xFF) / 255.0f); // A
	dev->SetPixelShaderConstantF(FOGCOLOR_REGISTER, &fogColorVec.x, 1);

	// D3DTSS_CONSTANT (c2 thru c10)
	for (unsigned x = 0; x < MAX_NUM_TEXTURE_STAGE_STATES; ++x)
	{
		const D3DCOLOR stageConstantColor = state.currentStageStates[x].stageStateUnion.namedStates.constant;
		const D3DXVECTOR4 stageConstant( ( (stageConstantColor >> 16) & 0xFF) / 255.0f,  // R
			( (stageConstantColor >> 8) & 0xFF) / 255.0f,  // G
			(stageConstantColor & 0xFF) / 255.0f, 		 // B
			( (stageConstantColor >> 24) & 0xFF) / 255.0f); // A
		dev->SetPixelShaderConstantF(TEXTURESTAGE_CONSTANTS_REGISTERS + x, &stageConstant.x, 1);
	}

	// BUMPENVMAT matrices (c10 thru c27)
	for (unsigned x = 0; x < MAX_NUM_TEXTURE_STAGE_STATES; ++x)
	{
		D3DXVECTOR4 stageBumpEnvMat[2];
		stageBumpEnvMat[0] = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
		stageBumpEnvMat[1] = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);

		const TextureStageState& currentStageState = state.currentStageStates[x];

		// Transpose the matrix on the CPU before sending to the GPU for more efficient matrix multiplication:
		// [0, 1]		  [0, 2]
		// [2, 3] becomes [1, 3]
		stageBumpEnvMat[0].x = currentStageState.stageStateUnion.namedStates.bumpEnvMat00;
		stageBumpEnvMat[0].y = currentStageState.stageStateUnion.namedStates.bumpEnvMat10;
		stageBumpEnvMat[1].x = currentStageState.stageStateUnion.namedStates.bumpEnvMat01;
		stageBumpEnvMat[1].y = currentStageState.stageStateUnion.namedStates.bumpEnvMat11;
		// Append the scale and offset to the last column of the matrices (this should be completely untouched by the "m3x2" instruction)
		stageBumpEnvMat[0].w = currentStageState.stageStateUnion.namedStates.bumpEnvLScale;
		stageBumpEnvMat[1].w = currentStageState.stageStateUnion.namedStates.bumpEnvLOffset;
		dev->SetPixelShaderConstantF(STAGE_BUMPENVMAT_REGISTERS + x * 2, &(stageBumpEnvMat[0].x), 2);
	}
}
