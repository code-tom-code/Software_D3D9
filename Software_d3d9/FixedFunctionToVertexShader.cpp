#include "FixedFunctionToShader.h"
#include "IDirect3DVertexShader9Hook.h"
#include "IDirect3DVertexDeclaration9Hook.h"
#include "resource.h"

#define M_PI       3.14159265358979323846f   // pi

static const unsigned char WVP_REGISTERS = 3u;
static const unsigned char TWEENFACTOR_REGISTER = 7u;
static const unsigned char POINTSCALEPARAMS_REGISTER = 8u;
static const unsigned char POINTSCALEDATA_REGISTER = 9u;
static const unsigned char FOGDATA_REGISTER = 14u;
static const unsigned char MATERIALDATA_REGISTERS = 15u;
static const unsigned char AMBIENT_REGISTER = 19u;
static const unsigned char TEXCOORD_TRANSFORM_REGISTERS = 20u;
static const unsigned char LIGHTDATA_REGISTERS = 52u;
static const unsigned char PROJMATRIX_REGISTERS = 108u;
static const unsigned char WORLDVIEW_TRANSFORM_REGISTERS = 112u;

#pragma pack(push)
#pragma pack(1)
struct wvShaderMatrix
{
	D3DXMATRIXA16 worldView; // Forwards worldview matrix (used for transforming vertex positions from modelspace to view-space)
	D3DXMATRIXA16 invWorldView; // Inverse of the worldView matrix (used for transforming vertex normals from modelspace to view-space)
};

struct shaderPackedLight
{
	D3DCOLORVALUE Diffuse;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Ambient;
	D3DXVECTOR4 Position_Range; // XYZ = Position, W = Range
	D3DXVECTOR4 Direction_Falloff; // XYZ = Direction (normalized), W = Falloff
	D3DXVECTOR4 Attenuation; // XYZ = Attenunation, W = 1.0f / (cos(theta / 2) - cos(phi / 2) )
	D3DXVECTOR4 SpotLightParams; // X = phi, Y = theta, Z = cos(theta / 2), W = cos(phi / 2)
};
#pragma pack(pop)
static_assert(sizeof(wvShaderMatrix) == 4 * 4 * 2 * sizeof(float), "Error - unexpected struct size for wvShaderMatrix!");
static_assert(sizeof(shaderPackedLight) == 4 * 7 * sizeof(float), "Error - unexpected struct size for shaderPackedLight!");

const FixedFunctionStateHash HashVertexState(const DeviceState& state)
{
	FixedFunctionStateHash retHash = 0;

	for (unsigned x = 0; x < 8; ++x)
	{
		const TextureStageState::_stageStateUnion::_namedStates& currentStageStates = state.currentStageStates[x].stageStateUnion.namedStates;
		HashContinue<unsigned char>(retHash, (unsigned char)(LOWORD(currentStageStates.texCoordIndex) ) );
		HashContinue<unsigned char>(retHash, (unsigned char)(HIWORD(currentStageStates.texCoordIndex) ) );
		if (state.currentStageStates[x].stageStateUnion.namedStates.textureTransformFlags & D3DTTFF_PROJECTED)
			HashContinue<unsigned char>(retHash, (unsigned char)(currentStageStates.textureTransformFlags) | 0x8);
		else
			HashContinue<unsigned char>(retHash, currentStageStates.textureTransformFlags);
	}
	const RenderStates::_renderStatesUnion::_namedStates& currentNamedStates = state.currentRenderStates.renderStatesUnion.namedStates;
	HashContinue(retHash, currentNamedStates.colorVertex);
	HashContinue<unsigned char>(retHash, currentNamedStates.diffuseMaterialSource);
	HashContinue<unsigned char>(retHash, currentNamedStates.specularMaterialSource);
	HashContinue<unsigned char>(retHash, currentNamedStates.ambientMaterialSource);
	HashContinue<unsigned char>(retHash, currentNamedStates.emissiveMaterialSource);
	if (currentNamedStates.vertexBlend == D3DVBF_0WEIGHTS)
		HashContinue<unsigned char>(retHash, (unsigned char)(currentNamedStates.vertexBlend) | 0x4);
	else
		HashContinue<unsigned char>(retHash, currentNamedStates.vertexBlend);
	HashContinue(retHash, currentNamedStates.indexedVertexBlendEnable);
	HashContinue(retHash, currentNamedStates.normalizeNormals);
	HashContinue(retHash, currentNamedStates.fogEnable);
	if (currentNamedStates.fogEnable)
	{
		HashContinue<unsigned char>(retHash, currentNamedStates.fogVertexMode);
		HashContinue<unsigned char>(retHash, currentNamedStates.fogTableMode);
		HashContinue<unsigned char>(retHash, currentNamedStates.rangeFogEnable);
	}
	HashContinue(retHash, currentNamedStates.lighting);
	HashContinue(retHash, currentNamedStates.specularEnable);
	HashContinue(retHash, currentNamedStates.localViewer);
	if (currentNamedStates.lighting)
	{
		for (unsigned x = 0; x < 8; ++x)
		{
			D3DLIGHTTYPE thisLightType = (const D3DLIGHTTYPE)0; // Note that 0 does not map to a valid enum value, so this is okay
			if (state.enabledLightIndices[x])
				thisLightType = state.enabledLightIndices[x]->light.Type;
			HashContinue<unsigned char>(retHash, thisLightType);
		}
	}

	if (state.currentVertexDecl)
	{
		unsigned inputVertexDeclBits = 0;
		const std::vector<DebuggableD3DVERTEXELEMENT9>& declElements = state.currentVertexDecl->GetElementsInternal();
		const unsigned numElements = declElements.size();
		for (unsigned x = 0; x < numElements; ++x)
		{
			const DebuggableD3DVERTEXELEMENT9& thisElement = declElements[x];
			
			// These are all of the input types that the fixed function vertex pipeline cares about that could change its shadercode:
			if (thisElement.Usage == D3DDECLUSAGE_POSITION && thisElement.UsageIndex == 0) // POSITION0
				inputVertexDeclBits |= (1 << 0);
			else if (thisElement.Usage == D3DDECLUSAGE_POSITION && thisElement.UsageIndex == 1) // POSITION1 (used only when vertex tweening)
				inputVertexDeclBits |= (1 << 1);
			else if (thisElement.Usage == D3DDECLUSAGE_BLENDWEIGHT && thisElement.UsageIndex == 0) // BLENDWEIGHT
				inputVertexDeclBits |= (1 << 2);
			else if (thisElement.Usage == D3DDECLUSAGE_BLENDINDICES && thisElement.UsageIndex == 0) // BLENDINDICES
				inputVertexDeclBits |= (1 << 3);
			else if (thisElement.Usage == D3DDECLUSAGE_NORMAL && thisElement.UsageIndex == 0) // NORMAL0
				inputVertexDeclBits |= (1 << 4);
			else if (thisElement.Usage == D3DDECLUSAGE_NORMAL && thisElement.UsageIndex == 1) // NORMAL1 (used only when vertex tweening)
				inputVertexDeclBits |= (1 << 5);
			else if (thisElement.Usage == D3DDECLUSAGE_PSIZE && thisElement.UsageIndex == 0) // PSIZE
				inputVertexDeclBits |= (1 << 6);
			else if (thisElement.Usage == D3DDECLUSAGE_COLOR && thisElement.UsageIndex == 0) // COLOR0 (diffuse color)
				inputVertexDeclBits |= (1 << 7);
			else if (thisElement.Usage == D3DDECLUSAGE_COLOR && thisElement.UsageIndex == 1) // COLOR1 (specular color)
				inputVertexDeclBits |= (1 << 8);
			else if (thisElement.Usage == D3DDECLUSAGE_TEXCOORD) // TEXCOORDN (texcoords)
				inputVertexDeclBits |= (1 << (9 + thisElement.UsageIndex) );
		}
		HashContinue(retHash, inputVertexDeclBits);
	}

	return retHash;
}

static const char* const STAGE_MATERIALSOURCE[3] =
{
	"D3DMCS_MATERIAL", // 0
	"D3DMCS_COLOR1", // 1
	"D3DMCS_COLOR2" // 2
};

static const char* const VERTEX_FOGMODE[4] =
{
	"D3DFOG_NONE", //                 = 0,
    "D3DFOG_EXP", //                  = 1,
    "D3DFOG_EXP2", //                 = 2,
    "D3DFOG_LINEAR", //               = 3,
};

static const char* const lightCountStr[9] = 
{
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8"
};

static const char* const lightTypeStr[] =
{
	"D3DLIGHT_POINT",//          = 1,
    "D3DLIGHT_SPOT",//           = 2,
    "D3DLIGHT_DIRECTIONAL",//    = 3,
};

static const char* const lightTypesNamesStr[8] =
{
	"LIGHTTYPE0",
	"LIGHTTYPE1",
	"LIGHTTYPE2",
	"LIGHTTYPE3",
	"LIGHTTYPE4",
	"LIGHTTYPE5",
	"LIGHTTYPE6",
	"LIGHTTYPE7"
};

static const char* const texRemapStrings[8] = 
{
	"TEX0REMAP",
	"TEX1REMAP",
	"TEX2REMAP",
	"TEX3REMAP",
	"TEX4REMAP",
	"TEX5REMAP",
	"TEX6REMAP",
	"TEX7REMAP"
};

static const char* const texRemapDefStrings[8] = 
{
	"tex0",
	"tex1",
	"tex2",
	"tex3",
	"tex4",
	"tex5",
	"tex6",
	"tex7"
};

static const char* const texCoordGenerationTypeName[8] =
{
	"D3DTSS_TEXCOORDINDEX0TYPE",
	"D3DTSS_TEXCOORDINDEX1TYPE",
	"D3DTSS_TEXCOORDINDEX2TYPE",
	"D3DTSS_TEXCOORDINDEX3TYPE",
	"D3DTSS_TEXCOORDINDEX4TYPE",
	"D3DTSS_TEXCOORDINDEX5TYPE",
	"D3DTSS_TEXCOORDINDEX6TYPE",
	"D3DTSS_TEXCOORDINDEX7TYPE"
};

static const char* const texCoordGenerationTypeString[5] = 
{
	"D3DTSS_TCI_PASSTHRU",//                            0x0000
	"D3DTSS_TCI_CAMERASPACENORMAL",//                   0x0001
	"D3DTSS_TCI_CAMERASPACEPOSITION",//                 0x0002
	"D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR",//         0x0003
	"D3DTSS_TCI_SPHEREMAP"//                            0x0004
};

static const char* const texTransformStrings[8] =
{
	"USE_TEXTRANSFORM0",
	"USE_TEXTRANSFORM1",
	"USE_TEXTRANSFORM2",
	"USE_TEXTRANSFORM3",
	"USE_TEXTRANSFORM4",
	"USE_TEXTRANSFORM5",
	"USE_TEXTRANSFORM6",
	"USE_TEXTRANSFORM7"
};

struct elementMatchDefine
{
	const D3DDECLUSAGE usage;
	const DWORD usageIndex;
	const char* const defineString;
};

static const elementMatchDefine matchDefines[] =
{
	{ D3DDECLUSAGE_POSITION, 0, "DECL_HAS_POSITION0" },
	{ D3DDECLUSAGE_POSITION, 1, "DECL_HAS_POSITION1" },
	{ D3DDECLUSAGE_BLENDWEIGHT, 0, "INPUT_HAS_BLENDWEIGHTS" },
	{ D3DDECLUSAGE_BLENDINDICES, 0, "INPUT_HAS_LASTBETA" },
	{ D3DDECLUSAGE_NORMAL, 0, "DECL_HAS_NORMAL0" },
	{ D3DDECLUSAGE_PSIZE, 0, "PER_VERTEX_POINTSIZE" },
	{ D3DDECLUSAGE_COLOR, 0, "INPUT_HAS_DIFFUSE" },
	{ D3DDECLUSAGE_COLOR, 1, "INPUT_HAS_SPECULAR" },
	{ D3DDECLUSAGE_TEXCOORD, 0, "INPUT_HAS_TEXCOORD0" },
	{ D3DDECLUSAGE_TEXCOORD, 1, "INPUT_HAS_TEXCOORD1" },
	{ D3DDECLUSAGE_TEXCOORD, 2, "INPUT_HAS_TEXCOORD2" },
	{ D3DDECLUSAGE_TEXCOORD, 3, "INPUT_HAS_TEXCOORD3" },
	{ D3DDECLUSAGE_TEXCOORD, 4, "INPUT_HAS_TEXCOORD4" },
	{ D3DDECLUSAGE_TEXCOORD, 5, "INPUT_HAS_TEXCOORD5" },
	{ D3DDECLUSAGE_TEXCOORD, 6, "INPUT_HAS_TEXCOORD6" },
	{ D3DDECLUSAGE_TEXCOORD, 7, "INPUT_HAS_TEXCOORD7" }
};

static inline void BuildVertexDeclStateDefines(const IDirect3DVertexDeclaration9Hook* const decl, std::vector<D3DXMACRO>& defines)
{
	if (!decl)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		return;
	}

	const std::vector<DebuggableD3DVERTEXELEMENT9>& elements = decl->GetElementsInternal();
	const unsigned numElements = elements.size();
	for (unsigned x = 0; x < numElements; ++x)
	{
		const DebuggableD3DVERTEXELEMENT9& thisElement = elements[x];
		for (unsigned y = 0; y < ARRAYSIZE(matchDefines); ++y)
		{
			const elementMatchDefine& thisMatchDefine = matchDefines[y];
			if (thisMatchDefine.usage == thisElement.Usage && thisMatchDefine.usageIndex == thisElement.UsageIndex)
			{
				D3DXMACRO newDefine = {0};
				newDefine.Name = thisMatchDefine.defineString;
				newDefine.Definition = "1";
				defines.push_back(newDefine);
				break;
			}
		}
	}
}

static inline void BuildOutputStateDefines(const DeviceState& state, std::vector<D3DXMACRO>& defines)
{
	// TODO: VS Output defines
	/*VS OUTPUT (is this right???)
	OUTPUT_POINTSIZE // Only if point scale or point sprites are enabled?
	OUTPUT_FOG // Only if fog is enabled?
	OUTPUT_TEX0 thru OUTPUT_TEX7 // Check for consumption from pixelshader or FF pipeline*/
	{
		D3DXMACRO tex0_mcs = {0};
		tex0_mcs.Name = "OUTPUT_TEX0";
		tex0_mcs.Definition = "1";
		defines.push_back(tex0_mcs);
	}
}

static inline void BuildVertexStateDefines(const DeviceState& state, std::vector<D3DXMACRO>& defines)
{
	{
		D3DXMACRO diffuse_mcs = {0};
		diffuse_mcs.Name = "DIFFUSEMATERIALSOURCE";
		diffuse_mcs.Definition = STAGE_MATERIALSOURCE[state.currentRenderStates.renderStatesUnion.namedStates.colorVertex ? state.currentRenderStates.renderStatesUnion.namedStates.diffuseMaterialSource : D3DMCS_MATERIAL];
		defines.push_back(diffuse_mcs);
	}
	{
		D3DXMACRO specular_mcs = {0};
		specular_mcs.Name = "SPECULARMATERIALSOURCE";
		specular_mcs.Definition = STAGE_MATERIALSOURCE[state.currentRenderStates.renderStatesUnion.namedStates.colorVertex ? state.currentRenderStates.renderStatesUnion.namedStates.specularMaterialSource : D3DMCS_MATERIAL];
		defines.push_back(specular_mcs);
	}
	{
		D3DXMACRO ambient_mcs = {0};
		ambient_mcs.Name = "AMBIENTMATERIALSOURCE";
		ambient_mcs.Definition = STAGE_MATERIALSOURCE[state.currentRenderStates.renderStatesUnion.namedStates.ambientMaterialSource]; // D3DRS_AMBIENTMATERIALSOURCE does not appear to be affected by D3DRS_COLORVERTEX
		defines.push_back(ambient_mcs);
	}
	{
		D3DXMACRO emissive_mcs = {0};
		emissive_mcs.Name = "EMISSIVEMATERIALSOURCE";
		emissive_mcs.Definition = STAGE_MATERIALSOURCE[state.currentRenderStates.renderStatesUnion.namedStates.emissiveMaterialSource]; // D3DRS_EMISSIVEMATERIALSOURCE does not appear to be affected by D3DRS_COLORVERTEX
		defines.push_back(emissive_mcs);
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.vertexBlend == D3DVBF_TWEENING)
	{
		D3DXMACRO tweening = {0};
		tweening.Name = "VERTEX_TWEENING";
		tweening.Definition = "1";
		defines.push_back(tweening);
	}

	{
		D3DXMACRO vertexBlend = {0};
		state.currentRenderStates.renderStatesUnion.namedStates.vertexBlend;
		vertexBlend.Name = "VERTEXBLEND";
		switch (state.currentRenderStates.renderStatesUnion.namedStates.vertexBlend)
		{
		default:
		case D3DVBF_DISABLE:
			vertexBlend.Definition = "D3DVBF_DISABLE";
			break;
		case D3DVBF_1WEIGHTS:
			vertexBlend.Definition = "D3DVBF_1WEIGHTS";
			break;
		case D3DVBF_2WEIGHTS:
			vertexBlend.Definition = "D3DVBF_2WEIGHTS";
			break;
		case D3DVBF_3WEIGHTS:
			vertexBlend.Definition = "D3DVBF_3WEIGHTS";
			break;
		case D3DVBF_TWEENING:
			vertexBlend.Definition = "D3DVBF_TWEENING";
			break;
		case D3DVBF_0WEIGHTS:
			vertexBlend.Definition = "D3DVBF_0WEIGHTS";
			break;
		}
		defines.push_back(vertexBlend);
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.indexedVertexBlendEnable)
	{
		D3DXMACRO indexedVertexBlendEnable = {0};
		indexedVertexBlendEnable.Name = "INDEXEDVERTEXBLENDENABLE";
		indexedVertexBlendEnable.Definition = "1";
		defines.push_back(indexedVertexBlendEnable);
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.normalizeNormals)
	{
		D3DXMACRO normalizeNormalsEnable = {0};
		normalizeNormalsEnable.Name = "D3DRS_NORMALIZENORMALS";
		normalizeNormalsEnable.Definition = "1";
		defines.push_back(normalizeNormalsEnable);
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.fogEnable)
	{
		D3DXMACRO fogEnable = {0};
		fogEnable.Name = "OUTPUT_FOG";
		fogEnable.Definition = "1";
		defines.push_back(fogEnable);

		D3DFOGMODE fogMode = D3DFOG_NONE;

		// TODO: Confirm that table-fog overrides vertex-fog and not the other way around:
		if (state.currentRenderStates.renderStatesUnion.namedStates.fogVertexMode > D3DFOG_NONE)
			fogMode = state.currentRenderStates.renderStatesUnion.namedStates.fogVertexMode;
		if (state.currentRenderStates.renderStatesUnion.namedStates.fogTableMode > D3DFOG_NONE)
			fogMode = state.currentRenderStates.renderStatesUnion.namedStates.fogTableMode;

		D3DXMACRO vertexFogmode = {0};
		vertexFogmode.Name = "D3DRS_FOGVERTEXMODE";
		vertexFogmode.Definition = VERTEX_FOGMODE[fogMode];
		defines.push_back(vertexFogmode);

		if (state.currentRenderStates.renderStatesUnion.namedStates.rangeFogEnable)
		{
			D3DXMACRO rangeFogEnable = {0};
			rangeFogEnable.Name = "D3DRS_RANGEFOGENABLE";
			rangeFogEnable.Definition = "1";
			defines.push_back(rangeFogEnable);
		}
	}

	if (state.currentRenderStates.renderStatesUnion.namedStates.lighting)
	{
		unsigned numEnabledLights = 0;
		for (unsigned x = 0; x < 8; ++x)
		{
			if (state.enabledLightIndices[x] != NULL)
			{
				++numEnabledLights;
			}
		}

		// Note that it *is* possible to have a situation where NUM_ENABLED_LIGHTS is 0 and LIGHTING is 1
		D3DXMACRO lightCount = {0};
		lightCount.Name = "NUM_ENABLED_LIGHTS";
		lightCount.Definition = lightCountStr[numEnabledLights];
		defines.push_back(lightCount);

		D3DXMACRO lightingMacro = {0};
		lightingMacro.Name = "LIGHTING";
		lightingMacro.Definition = "1";
		defines.push_back(lightingMacro);

		if (state.currentRenderStates.renderStatesUnion.namedStates.specularEnable)
		{
			D3DXMACRO specularEnableMacro = {0};
			specularEnableMacro.Name = "D3DRS_SPECULARENABLE";
			specularEnableMacro.Definition = "1";
			defines.push_back(specularEnableMacro);
		}

		if (state.currentRenderStates.renderStatesUnion.namedStates.localViewer)
		{
			D3DXMACRO specularEnableMacro = {0};
			specularEnableMacro.Name = "D3DRS_LOCALVIEWER";
			specularEnableMacro.Definition = "1";
			defines.push_back(specularEnableMacro);
		}

		// LIGHTTYPE0 thru LIGHTTYPE7 (defined as strings D3DLIGHT_POINT, D3DLIGHT_SPOT, or D3DLIGHT_DIRECTIONAL)
		unsigned foundLightTypes = 0;
		for (unsigned y = 0; y < 8; ++y)
		{
			if (foundLightTypes < numEnabledLights)
			{
				const LightInfo* const thisLightInfo = state.enabledLightIndices[y];
				if (thisLightInfo)
				{
					D3DXMACRO lightNType = {0};
					lightNType.Name = lightTypesNamesStr[foundLightTypes++];
					lightNType.Definition = lightTypeStr[thisLightInfo->light.Type - 1]; // Minus one here because light types are 1-based instead of 0-based
					defines.push_back(lightNType);
				}
			}
		}
	}
	else
	{
		D3DXMACRO numEnabledLights = {0};
		numEnabledLights.Name = "NUM_ENABLED_LIGHTS";
		numEnabledLights.Definition = "0";
		defines.push_back(numEnabledLights);
	}

	// TEX0REMAP thru TEX7REMAP (defined as strings "tex0" thru "tex7" which remap indices for texcoord lookups)
	for (unsigned x = 0; x < 8; ++x)
	{
		const TextureStageState& thisStageState = state.currentStageStates[x];
		const UINT texCoordIndex = thisStageState.stageStateUnion.namedStates.texCoordIndex;
		const UINT targetIndex = LOWORD(texCoordIndex);
		const UINT texCoordGeneration = HIWORD(texCoordIndex);
		const D3DTEXTURETRANSFORMFLAGS transformFlags = thisStageState.stageStateUnion.namedStates.textureTransformFlags;

#ifdef _DEBUG
		if (targetIndex >= 8)
		{
			// Invalid texcoord remapping for the fixed-function vertex pipeline
			__debugbreak();
		}

		if (texCoordGeneration > D3DTSS_TCI_SPHEREMAP)
		{
			__debugbreak(); // Invalid texcoord generation method
		}
#endif

		D3DXMACRO texCoordGenerationType = {0};
		texCoordGenerationType.Name = texCoordGenerationTypeName[x];
		texCoordGenerationType.Definition = texCoordGenerationTypeString[texCoordGeneration];
		defines.push_back(texCoordGenerationType);

		if (texCoordGeneration == D3DTSS_TCI_PASSTHRU)
		{
			D3DXMACRO texCoordRemap = {0};
			texCoordRemap.Name = texRemapStrings[x];
			texCoordRemap.Definition = texRemapDefStrings[targetIndex % 8];
			defines.push_back(texCoordRemap);
		}

		// Texture matrix transforms take place after texcoord generation and texcoord remapping
		if (LOBYTE(transformFlags) != D3DTTFF_DISABLE)
		{
			D3DXMACRO texTransformEnable = {0};
			texTransformEnable.Name = texTransformStrings[x];
			texTransformEnable.Definition = "1";
			defines.push_back(texTransformEnable);
		}
	}

	BuildVertexDeclStateDefines(state.currentVertexDecl, defines);

	BuildOutputStateDefines(state, defines);

	if (!defines.empty() )
	{
		D3DXMACRO emptyLastMacro = {0};
		defines.push_back(emptyLastMacro);
	}
}

void BuildVertexShader(const DeviceState& state, IDirect3DDevice9Hook* const dev, IDirect3DVertexShader9Hook** const outNewShader)
{
#ifdef _DEBUG
	if (!outNewShader)
	{
		__debugbreak();
	}
#endif

	std::vector<D3DXMACRO> defines;
	BuildVertexStateDefines(state, defines);

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
	const char* const resourceBytes = (const char* const)GetShaderResourceFile(MAKEINTRESOURCEA(IDR_HLSL_FFVS_SRC), resourceSize);
	if (resourceBytes != NULL)
	{
		hr = D3DXCompileShader( (const char* const)resourceBytes, resourceSize, defines.empty() ? NULL : &defines.front(), D3DXIncludeHandler::GetGlobalIncludeHandlerSingleton(), 
			"main", "vs_3_0", flags, &outBytecode, &errorMessages, NULL);
	}
	if (FAILED(hr) || !outBytecode)
	{
		const char* const errorMessage = errorMessages ? ( (const char* const)errorMessages->GetBufferPointer() ) : NULL;

		// Should never happen for fixed-function shaders!
		MessageBoxA(NULL, errorMessage, "Fixed Function VS Compile Failure!", MB_OK);
		__debugbreak();

		printf("%s", errorMessage); // Don't optimize this away

		return;
	}

#ifdef _DEBUG
	const char* const debugErrorMessage = errorMessages ? ( (const char* const)errorMessages->GetBufferPointer() ) : NULL;
	UNREFERENCED_PARAMETER(debugErrorMessage);
#endif

	IDirect3DVertexShader9* newVertexShader = NULL;
	if (FAILED(dev->CreateVertexShader( (const DWORD* const)outBytecode->GetBufferPointer(), &newVertexShader) ) || !newVertexShader)
	{
		// Should never happen for fixed-function shaders!
		__debugbreak();
	}

	IDirect3DVertexShader9Hook* newVertexShaderHook = dynamic_cast<IDirect3DVertexShader9Hook*>(newVertexShader);
	if (!newVertexShaderHook)
	{
		DbgBreakPrint("Error: CreateVertexShader returned a non-hooked pointer!");
	}

	newVertexShaderHook->GetModifyShaderInfo().fixedFunctionMacroDefines.swap(defines);

	*outNewShader = newVertexShaderHook;

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

static void SetFixedFunctionVertexShaderState_IndexedVertexBlendWorldViews(const Transforms& transforms, IDirect3DDevice9Hook* const dev)
{
	// This takes up quite a lot of stack-space, which is why it's broken out into its own function
	wvShaderMatrix indexedWorldViewBlendingMatrices[MAX_WORLD_TRANSFORMS];
	for (unsigned char x = 0; x < 4; ++x)
	{
		wvShaderMatrix& thisShaderWVMatrices = indexedWorldViewBlendingMatrices[x];
		thisShaderWVMatrices.worldView = transforms.GetWVTransformFromCache(x);
		thisShaderWVMatrices.invWorldView = transforms.GetInvWVTransformFromCache(x);
	}
	for (unsigned short x = 4; x < MAX_WORLD_TRANSFORMS; ++x)
	{
		wvShaderMatrix& thisShaderWVMatrices = indexedWorldViewBlendingMatrices[x];
		thisShaderWVMatrices.worldView = transforms.WorldTransforms[x] * transforms.ViewTransform;
		D3DXMatrixInverse(&thisShaderWVMatrices.invWorldView, NULL, &(thisShaderWVMatrices.worldView) );
	}
	for (unsigned short x = 0; x < MAX_WORLD_TRANSFORMS; ++x)
	{
		dev->SetVertexShaderConstantF(WORLDVIEW_TRANSFORM_REGISTERS + 7 * x, (const float* const)&(indexedWorldViewBlendingMatrices[x]), 7);
	}
}

void SetFixedFunctionVertexShaderState(const DeviceState& state, IDirect3DDevice9Hook* const dev)
{
	const Transforms& transforms = state.currentTransforms;

	// WVP transform:
	const D3DXMATRIXA16& wvpTransform = transforms.GetWVPTransform();
	dev->SetVertexShaderConstantF(WVP_REGISTERS, (const float* const)&(wvpTransform), 4);

	const RenderStates::_renderStatesUnion::_namedStates& namedRenderStates = state.currentRenderStates.renderStatesUnion.namedStates;

	// Vertex tweening factor
	const D3DXVECTOR4 tweenFactor(namedRenderStates.tweenFactor, 0.0f, 0.0f, 0.0f);
	dev->SetVertexShaderConstantF(TWEENFACTOR_REGISTER, &tweenFactor.x, 1);

	// Point Scale
	const D3DXVECTOR4 pointScaleParams(namedRenderStates.pointScale_A, namedRenderStates.pointScale_B, namedRenderStates.pointScale_C, 0.0f);
	dev->SetVertexShaderConstantF(POINTSCALEPARAMS_REGISTER, &pointScaleParams.x, 1);
	const D3DXVECTOR4 pointScaleData(namedRenderStates.pointSize_Max, namedRenderStates.pointSize_Min, namedRenderStates.pointSize, state.cachedViewport.fHeight);
	dev->SetVertexShaderConstantF(POINTSCALEDATA_REGISTER, &pointScaleData.x, 1);

	// Fog
	const D3DXVECTOR4 fogData(namedRenderStates.fogStart, namedRenderStates.fogEnd, namedRenderStates.fogDensity, 0.0f);
	dev->SetVertexShaderConstantF(FOGDATA_REGISTER, &fogData.x, 1);

	// Material data (with specular power packed into specular.a)
	const D3DXVECTOR4 packedMaterialData[4] =
	{
		*(const D3DXVECTOR4* const)&(state.currentMaterial.Diffuse),
		D3DXVECTOR4(state.currentMaterial.Specular.r, state.currentMaterial.Specular.g, state.currentMaterial.Specular.b, state.currentMaterial.Power),
		*(const D3DXVECTOR4* const)&(state.currentMaterial.Ambient),
		*(const D3DXVECTOR4* const)&(state.currentMaterial.Emissive)
	};
	dev->SetVertexShaderConstantF(MATERIALDATA_REGISTERS, (const float* const)&packedMaterialData, 4);

	// Ambient color
	D3DXVECTOR4 ambient;
	ColorDWORDToFloat4<0xF>(namedRenderStates.ambient, ambient);
	dev->SetVertexShaderConstantF(AMBIENT_REGISTER, &ambient.x, 1);

	// Texture transform matrices (8 4x4 matrices):
	dev->SetVertexShaderConstantF(TEXCOORD_TRANSFORM_REGISTERS, (const float* const)&(transforms.TextureTransforms), 4 * D3DDP_MAXTEXCOORD);

	// Light data:
	if (namedRenderStates.lighting && state.enabledLightIndices[0] != NULL)
	{
		D3DXMATRIXA16 inverseViewMat, invTransposeViewMat;
		D3DXMatrixInverse(&inverseViewMat, NULL, &transforms.ViewTransform);
		D3DXMatrixTranspose(&invTransposeViewMat, &inverseViewMat);
		shaderPackedLight enabledLights[8];
		for (unsigned char enabledLightIndex = 0; enabledLightIndex < 8; ++enabledLightIndex)
		{
			const LightInfo* const enabledLight = state.enabledLightIndices[enabledLightIndex];
			if (!enabledLight)
				break;

			const D3DLIGHT9& lightInfo = enabledLight->light;

			shaderPackedLight& thisPackedLight = enabledLights[enabledLightIndex];
			thisPackedLight.Diffuse = lightInfo.Diffuse;
			thisPackedLight.Specular = lightInfo.Specular;
			thisPackedLight.Ambient = lightInfo.Ambient;
			float range = lightInfo.Range;
			if (range < 0.0f)
				range = 0.0f;
			else if (range > sqrtf(FLT_MAX) )
				range = sqrtf(FLT_MAX);
			D3DXVECTOR3 transformedPosition;
			D3DXVec3TransformCoord(&transformedPosition, (const D3DXVECTOR3* const)&lightInfo.Position, &transforms.ViewTransform);
			thisPackedLight.Position_Range = D3DXVECTOR4(transformedPosition.x, transformedPosition.y, transformedPosition.z, range);
			D3DXVECTOR3 normalizedDirection, viewspaceNormalizedDirection;
			D3DXVec3Normalize(&normalizedDirection, (const D3DXVECTOR3* const)&lightInfo.Direction);
			D3DXVec3TransformNormal(&viewspaceNormalizedDirection, &normalizedDirection, &invTransposeViewMat);
			thisPackedLight.Direction_Falloff = D3DXVECTOR4(viewspaceNormalizedDirection.x, viewspaceNormalizedDirection.y, viewspaceNormalizedDirection.z, lightInfo.Falloff);
			
			float phi = lightInfo.Phi;
			if (phi < 0.0f)
				phi = 0.0f;
			else if (phi > M_PI)
				phi = M_PI;

			float theta = lightInfo.Theta;
			if (theta < 0.0f)
				theta = 0.0f;
			else if (theta > phi)
				theta = phi;

			const float halfThetaStrength = cosf(theta * 0.5f);
			const float halfPhiStrength = cosf(phi * 0.5f);

			thisPackedLight.Attenuation = D3DXVECTOR4(lightInfo.Attenuation0, lightInfo.Attenuation1, lightInfo.Attenuation2, 1.0f / (halfThetaStrength - halfPhiStrength) );

			thisPackedLight.SpotLightParams = D3DXVECTOR4(phi, theta, halfThetaStrength, halfPhiStrength);
		}
		dev->SetVertexShaderConstantF(LIGHTDATA_REGISTERS, (const float* const)&enabledLights, 7 * 8);
	}

	// Forwards projection matrix ("p matrix")
	dev->SetVertexShaderConstantF(PROJMATRIX_REGISTERS, (const float* const)&(transforms.ProjectionTransform), 4);

	// World-view matrices:
	if (namedRenderStates.indexedVertexBlendEnable)
	{
		SetFixedFunctionVertexShaderState_IndexedVertexBlendWorldViews(transforms, dev);
	}
	else
	{
		for (unsigned char x = 0; x < 4; ++x)
		{
			wvShaderMatrix thisShaderWVMatrices;
			thisShaderWVMatrices.worldView = transforms.GetWVTransformFromCache(x);
			thisShaderWVMatrices.invWorldView = transforms.GetInvWVTransformFromCache(x);
			dev->SetVertexShaderConstantF(WORLDVIEW_TRANSFORM_REGISTERS + 7 * x, (const float* const)&thisShaderWVMatrices, 7);
		}
	}
}
