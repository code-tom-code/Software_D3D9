#define D3DTSS_TCI_PASSTHRU 0
#define D3DTSS_TCI_CAMERASPACENORMAL 1
#define D3DTSS_TCI_CAMERASPACEPOSITION 2
#define D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR 3
#define D3DTSS_TCI_SPHEREMAP 4

#define D3DMCS_MATERIAL 0
#define D3DMCS_COLOR1 1
#define D3DMCS_COLOR2 2

#define D3DLIGHT_POINT 1
#define D3DLIGHT_SPOT 2
#define D3DLIGHT_DIRECTIONAL 3

#define D3DFOG_NONE		0
#define D3DFOG_EXP		1
#define D3DFOG_EXP2		2
#define D3DFOG_LINEAR	3

#define D3DVBF_DISABLE 0 // Disable vertex blending
#define D3DVBF_0WEIGHTS 256 // one matrix is used with weight 1.0
#define D3DVBF_1WEIGHTS 1 // 2 matrix blending
#define D3DVBF_2WEIGHTS 2 // 3 matrix blending
#define D3DVBF_3WEIGHTS 3 // 4 matrix blending
#define D3DVBF_TWEENING 255 // blending using D3DRS_TWEENFACTOR

#define MAX_WORLD_TRANSFORMS 256

#ifndef TEX0TYPE
	#define TEX0TYPE float4
#endif // #ifndef TEX0TYPE

#ifndef TEX1TYPE
	#define TEX1TYPE float4
#endif // #ifndef TEX1TYPE

#ifndef TEX2TYPE
	#define TEX2TYPE float4
#endif // #ifndef TEX2TYPE

#ifndef TEX3TYPE
	#define TEX3TYPE float4
#endif // #ifndef TEX3TYPE

#ifndef TEX4TYPE
	#define TEX4TYPE float4
#endif // #ifndef TEX4TYPE

#ifndef TEX5TYPE
	#define TEX5TYPE float4
#endif // #ifndef TEX5TYPE

#ifndef TEX6TYPE
	#define TEX6TYPE float4
#endif // #ifndef TEX6TYPE

#ifndef TEX7TYPE
	#define TEX7TYPE float4
#endif // #ifndef TEX7TYPE

// These TEXNREMAP defines correspond to the D3DTSS_TEXCOORDINDEX setting for which texcoord channel to
// source from (when D3DTSS_TCI_PASSTHRU is used anyway, since auto-generated texcoords ignore input texcoord channels)
#ifndef TEX0REMAP
	#define TEX0REMAP tex0
#endif // #ifndef TEX0REMAP

#ifndef TEX1REMAP
	#define TEX1REMAP tex1
#endif // #ifndef TEX1REMAP

#ifndef TEX2REMAP
	#define TEX2REMAP tex2
#endif // #ifndef TEX2REMAP

#ifndef TEX3REMAP
	#define TEX3REMAP tex3
#endif // #ifndef TEX3REMAP

#ifndef TEX4REMAP
	#define TEX4REMAP tex4
#endif // #ifndef TEX4REMAP

#ifndef TEX5REMAP
	#define TEX5REMAP tex5
#endif // #ifndef TEX5REMAP

#ifndef TEX6REMAP
	#define TEX6REMAP tex6
#endif // #ifndef TEX6REMAP

#ifndef TEX7REMAP
	#define TEX7REMAP tex7
#endif // #ifndef TEX7REMAP

#ifndef D3DTSS_TEXCOORDINDEX0TYPE
	#define D3DTSS_TEXCOORDINDEX0TYPE D3DTSS_TCI_PASSTHRU
#endif // #ifndef D3DTSS_TEXCOORDINDEX0TYPE

#ifndef D3DTSS_TEXCOORDINDEX1TYPE
	#define D3DTSS_TEXCOORDINDEX1TYPE D3DTSS_TCI_PASSTHRU
#endif // #ifndef D3DTSS_TEXCOORDINDEX1TYPE

#ifndef D3DTSS_TEXCOORDINDEX2TYPE
	#define D3DTSS_TEXCOORDINDEX2TYPE D3DTSS_TCI_PASSTHRU
#endif // #ifndef D3DTSS_TEXCOORDINDEX2TYPE

#ifndef D3DTSS_TEXCOORDINDEX3TYPE
	#define D3DTSS_TEXCOORDINDEX3TYPE D3DTSS_TCI_PASSTHRU
#endif // #ifndef D3DTSS_TEXCOORDINDEX3TYPE

#ifndef D3DTSS_TEXCOORDINDEX4TYPE
	#define D3DTSS_TEXCOORDINDEX4TYPE D3DTSS_TCI_PASSTHRU
#endif // #ifndef D3DTSS_TEXCOORDINDEX4TYPE

#ifndef D3DTSS_TEXCOORDINDEX5TYPE
	#define D3DTSS_TEXCOORDINDEX5TYPE D3DTSS_TCI_PASSTHRU
#endif // #ifndef D3DTSS_TEXCOORDINDEX5TYPE

#ifndef D3DTSS_TEXCOORDINDEX6TYPE
	#define D3DTSS_TEXCOORDINDEX6TYPE D3DTSS_TCI_PASSTHRU
#endif // #ifndef D3DTSS_TEXCOORDINDEX6TYPE

#ifndef D3DTSS_TEXCOORDINDEX7TYPE
	#define D3DTSS_TEXCOORDINDEX7TYPE D3DTSS_TCI_PASSTHRU
#endif // #ifndef D3DTSS_TEXCOORDINDEX7TYPE

struct D3DMATERIAL9
{
	float4 Diffuse;
	float4 Specular; // This contains Specular as RGB and Specular Power as A
	float4 Ambient;
	float3 Emissive; // Emissive.a is not used
};

struct LightData
{
	float4 Diffuse;
	float4 Specular;
	float4 Ambient;
	float4 Position; // XYZ = Position (in view-space), W = Range
	float4 Direction; // XYZ = Direction (normalized and in view-space), W = Falloff
	float4 Attenuation; // XYZ = attenuation, W = 1.0f / (cos(theta / 2) - cos(phi / 2) )
	float4 SpotlightParams; // X = phi, Y = theta, Z = cos(theta / 2), W = cos(phi / 2)
};

#ifndef DIFFUSEMATERIALSOURCE
	#define DIFFUSEMATERIALSOURCE D3DMCS_COLOR1
#endif

#ifndef SPECULARMATERIALSOURCE
	#define SPECULARMATERIALSOURCE D3DMCS_COLOR2
#endif

#ifndef AMBIENTMATERIALSOURCE
	#define AMBIENTMATERIALSOURCE D3DMCS_MATERIAL
#endif

#ifndef EMISSIVEMATERIALSOURCE
	#define EMISSIVEMATERIALSOURCE D3DMCS_MATERIAL
#endif

// Testing!
/*#define OUTPUT_FOG 1
#define D3DFOG_LINEAR 3
#define D3DRS_FOGVERTEXMODE D3DFOG_LINEAR
#define OUTPUT_POINTSIZE 1
#define DECL_HAS_TEX0 1
#define DECL_HAS_NORMAL0 1
#define VERTEXBLEND D3DVBF_3WEIGHTS
#define INPUT_HAS_BLENDWEIGHTS 1
#define INPUT_HAS_LASTBETA 1
#define INDEXEDVERTEXBLENDENABLE 1
#define INPUT_HAS_TEXCOORD0 1
#define OUTPUT_TEX0 1
#define USE_TEXTRANSFORM0 1*/

struct wvMatrix
{
	row_major float4x4 wv;
	row_major float3x3 invTransposeWV;
};

const row_major float4x4 wvp : register(c3); // row_major seems to be more efficient than column_major from the vertex shader PoV
#ifdef VERTEX_TWEENING
	const float TWEENFACTOR : register(c7);
#endif // #ifdef VERTEX_TWEENING
#ifdef OUTPUT_POINTSIZE
	const float3 pointScaleParams : register(c8); // float3(D3DRS_POINTSCALE_A, D3DRS_POINTSCALE_B, D3DRS_POINTSCALE_C). Example (using default D3D9 values): float3(1.0f, 0.0f, 0.0f)
	const float4 pointScaleData : register(c9); // float4(D3DRS_POINTSIZE_MAX, D3DRS_POINTSIZE_MIN, D3DRS_POINTSIZE, ViewportHeight (in whole-number pixels). Example: float4(8192.0f, 0.0f, 64.0f, 480.0f) for a 640x480 viewport
#endif // #ifdef OUTPUT_POINTSIZE
#ifdef OUTPUT_FOG
	const float3 fogData : register(c14); // float3(D3DRS_FOGSTART, D3DRS_FOGEND, D3DRS_FOGDENSITY) // TODO: Check that this is not reversed start/end or scale/offset format
#endif // #ifdef OUTPUT_FOG
const D3DMATERIAL9 materialData : register(c15); // c15, c16, c17, c18
const float4 AMBIENT : register(c19); // Global ambient color, set by SetRenderState(D3DRS_AMBIENT, ...)
const row_major float4x4 texMatrices[8] : register(c20); // It looks like texcoord matrix transforms get applied *after* autogeneration of texcoords (i.e. D3DTSS_TCI_CAMERA_*). c20 thru c51
const LightData enabledLightsData[8] : register(c52); // c52 thru c107
const row_major float4x4 proj : register(c108); // c108 thru c111
const wvMatrix wvBlendingMatrices[MAX_WORLD_TRANSFORMS] : register(c112); // This should always be last

#define D3DRS_POINTSIZE_MAX pointScaleData.x
#define D3DRS_POINTSIZE_MIN pointScaleData.y
#define D3DRS_POINTSIZE pointScaleData.z
#define ViewportHeight pointScaleData.w
#define D3DRS_POINTSCALE_A pointScaleParams.x
#define D3DRS_POINTSCALE_B pointScaleParams.y
#define D3DRS_POINTSCALE_C pointScaleParams.z
#define D3DRS_FOGSTART fogData.x
#define D3DRS_FOGEND fogData.y
#define D3DRS_FOGDENSITY fogData.z

struct inVert
{
	float3 POS0 : POSITION0;
#ifdef DECL_HAS_POSITION1
	float3 POS1 : POSITION1;
#endif // #ifdef DECL_HAS_POSITION1

#ifdef INPUT_HAS_BLENDWEIGHTS
	float4 blendweights : BLENDWEIGHT0;
#ifdef INPUT_HAS_LASTBETA
	int4 blendindices : BLENDINDICES0;
#endif // #ifdef INPUT_HAS_LASTBETA
#endif // #ifdef INPUT_HAS_BLENDWEIGHTS
#ifdef DECL_HAS_NORMAL0
	float3 NORM0 : NORMAL0;
#endif // #ifdef DECL_HAS_NORMAL0
#ifdef DECL_HAS_NORMAL1
	float3 NORM1 : NORMAL1;
#endif // #ifdef DECL_HAS_NORMAL1
#ifdef PER_VERTEX_POINTSIZE
	float PSIZ0 : PSIZE0;
#endif // #ifdef PER_VERTEX_POINTSIZE
#ifdef INPUT_HAS_DIFFUSE
	float4 diffuse : COLOR0;
#endif // #ifdef INPUT_HAS_DIFFUSE
#ifdef INPUT_HAS_SPECULAR
	float4 specular : COLOR1;
#endif // #ifdef INPUT_HAS_SPECULAR

#ifdef INPUT_HAS_TEXCOORD0
	TEX0TYPE TEX0 : TEXCOORD0;
#endif // #ifdef INPUT_HAS_TEX0
#ifdef INPUT_HAS_TEXCOORD1
	TEX1TYPE TEX1 : TEXCOORD1;
#endif // #ifdef INPUT_HAS_TEX1
#ifdef INPUT_HAS_TEXCOORD2
	TEX2TYPE TEX2 : TEXCOORD2;
#endif // #ifdef INPUT_HAS_TEX2
#ifdef INPUT_HAS_TEXCOORD3
	TEX3TYPE TEX3 : TEXCOORD3;
#endif // #ifdef INPUT_HAS_TEX3
#ifdef INPUT_HAS_TEXCOORD4
	TEX4TYPE TEX4 : TEXCOORD4;
#endif // #ifdef INPUT_HAS_TEX4
#ifdef INPUT_HAS_TEXCOORD5
	TEX5TYPE TEX5 : TEXCOORD5;
#endif // #ifdef INPUT_HAS_TEX5
#ifdef INPUT_HAS_TEXCOORD6
	TEX6TYPE TEX6 : TEXCOORD6;
#endif // #ifdef INPUT_HAS_TEX6
#ifdef INPUT_HAS_TEXCOORD7
	TEX7TYPE TEX7 : TEXCOORD7;
#endif // #ifdef INPUT_HAS_TEX7

};

struct outVert
{
	float4 OPOS : POSITION0;
#ifdef OUTPUT_POINTSIZE
	float oPts : PSIZE;
#endif // #ifdef OUTPUT_POINTSIZE
	float4 oDiffuse : COLOR0;
#if defined(D3DRS_SPECULARENABLE) && defined(LIGHTING)
	float4 oSpecular : COLOR1;
#endif // #if defined(D3DRS_SPECULARENABLE) && defined(LIGHTING)
#ifdef OUTPUT_FOG
	float oFog : FOG;
#endif // #ifdef OUTPUT_FOG
#ifdef OUTPUT_TEX0
	TEX0TYPE TEX0 : TEXCOORD0;
#endif // #ifdef OUTPUT_TEX0
#ifdef OUTPUT_TEX1
	TEX0TYPE TEX1 : TEXCOORD1;
#endif // #ifdef OUTPUT_TEX1
#ifdef OUTPUT_TEX2
	TEX0TYPE TEX2 : TEXCOORD2;
#endif // #ifdef OUTPUT_TEX2
#ifdef OUTPUT_TEX3
	TEX0TYPE TEX3 : TEXCOORD3;
#endif // #ifdef OUTPUT_TEX3
#ifdef OUTPUT_TEX4
	TEX0TYPE TEX4 : TEXCOORD4;
#endif // #ifdef OUTPUT_TEX4
#ifdef OUTPUT_TEX5
	TEX0TYPE TEX5 : TEXCOORD5;
#endif // #ifdef OUTPUT_TEX5
#ifdef OUTPUT_TEX6
	TEX0TYPE TEX6 : TEXCOORD6;
#endif // #ifdef OUTPUT_TEX6
#ifdef OUTPUT_TEX7
	TEX0TYPE TEX7 : TEXCOORD7;
#endif // #ifdef OUTPUT_TEX7
};

struct posNormal
{
	float4 OPOS;
	float3 worldViewPos;
	float3 oNormal;
};

const float4 GetVertexBlendWeights(in const inVert input)
{
#ifdef INPUT_HAS_BLENDWEIGHTS
	return input.blendweights;
#else // #ifdef INPUT_HAS_BLENDWEIGHTS
	return float4(1.0f, 0.0f, 0.0f, 0.0f);
#endif // #ifdef INPUT_HAS_BLENDWEIGHTS
}

const int4 GetVertexBlendIndices(in const inVert input)
{
#if defined(INPUT_HAS_BLENDWEIGHTS) && defined(INPUT_HAS_LASTBETA)
	return input.blendindices;
#else // #if defined(INPUT_HAS_BLENDWEIGHTS) && defined(INPUT_HAS_LASTBETA)
	return int4(0, 1, 2, 3);
#endif // #if defined(INPUT_HAS_BLENDWEIGHTS) && defined(INPUT_HAS_LASTBETA)
}

const float4 GetVertexDiffuse(in const inVert input)
{
#ifdef INPUT_HAS_DIFFUSE
	return input.diffuse;
#else // #ifdef INPUT_HAS_DIFFUSE
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif // #ifdef INPUT_HAS_DIFFUSE
}

const float4 GetVertexSpecular(in const inVert input)
{
#ifdef INPUT_HAS_SPECULAR
	return input.specular;
#else // #ifdef INPUT_HAS_SPECULAR
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif // #ifdef INPUT_HAS_SPECULAR
}

const float4 GetMaterialDiffuse(in const inVert input)
{
#if (DIFFUSEMATERIALSOURCE < D3DMCS_MATERIAL) || (DIFFUSEMATERIALSOURCE > D3DMCS_COLOR2)
	#error Error: Invalid DIFFUSEMATERIALSOURCE enum specified!
#endif

#if DIFFUSEMATERIALSOURCE == D3DMCS_COLOR1 && defined(INPUT_HAS_DIFFUSE)
	return input.diffuse;
#elif DIFFUSEMATERIALSOURCE == D3DMCS_COLOR2 && defined(INPUT_HAS_SPECULAR)
	return input.specular;
#else
	return materialData.Diffuse;
#endif
}

const float4 GetMaterialSpecular(in const inVert input)
{
#if (SPECULARMATERIALSOURCE < D3DMCS_MATERIAL) || (SPECULARMATERIALSOURCE > D3DMCS_COLOR2)
	#error Error: Invalid SPECULARMATERIALSOURCE enum specified!
#endif

#if SPECULARMATERIALSOURCE == D3DMCS_COLOR1 && defined(INPUT_HAS_DIFFUSE)
	return input.diffuse;
#elif SPECULARMATERIALSOURCE == D3DMCS_COLOR2 && defined(INPUT_HAS_SPECULAR)
	return input.specular;
#else
	return materialData.Specular;
#endif
}

const float4 GetMaterialAmbient(in const inVert input)
{
#if (AMBIENTMATERIALSOURCE < D3DMCS_MATERIAL) || (AMBIENTMATERIALSOURCE > D3DMCS_COLOR2)
	#error Error: Invalid AMBIENTMATERIALSOURCE enum specified!
#endif

#if AMBIENTMATERIALSOURCE == D3DMCS_COLOR1 && defined(INPUT_HAS_DIFFUSE)
	return input.diffuse;
#elif AMBIENTMATERIALSOURCE == D3DMCS_COLOR2 && defined(INPUT_HAS_SPECULAR)
	return input.specular;
#else
	return materialData.Ambient;
#endif
}

const float3 GetMaterialEmissive(in const inVert input)
{
#if (EMISSIVEMATERIALSOURCE < D3DMCS_MATERIAL) || (EMISSIVEMATERIALSOURCE > D3DMCS_COLOR2)
	#error Error: Invalid EMISSIVEMATERIALSOURCE enum specified!
#endif

#if EMISSIVEMATERIALSOURCE == D3DMCS_COLOR1 && defined(INPUT_HAS_DIFFUSE)
	return input.diffuse.rgb;
#elif EMISSIVEMATERIALSOURCE == D3DMCS_COLOR2 && defined(INPUT_HAS_SPECULAR)
	return input.specular.rgb;
#else
	return materialData.Emissive.rgb;
#endif
}

const float3 GetInputPosition0(in const inVert input)
{
#ifdef DECL_HAS_POSITION0
	return input.POS0;
#else // #ifdef DECL_HAS_POSITION0
	return float3(0.0f, 0.0f, 0.0f);
#endif // #ifdef DECL_HAS_POSITION0
}

const float3 GetInputNormal0(in const inVert input)
{
#ifdef DECL_HAS_NORMAL0
	return input.NORM0;
#else // #ifdef DECL_HAS_NORMAL0
	return float3(0.0f, 0.0f, 0.0f);
#endif // #ifdef DECL_HAS_NORMAL0
}

#ifdef VERTEX_TWEENING
const float3 GetInputPosition1(in const inVert input)
{
#ifdef DECL_HAS_POSITION1
	return input.POS1;
#else // #ifdef DECL_HAS_POSITION1
	reutrn input.POS0; // If a second vertex position is not available, don't do any tweening
#endif // #ifdef DECL_HAS_POSITION1
}

const float3 GetInputNormal1(in const inVert input)
{
#ifdef DECL_HAS_NORMAL1
	return input.NORM1;
#else // #ifdef DECL_HAS_NORMAL1
	return input.NORM0; // If a second vertex normal is not available, don't do any tweening
#endif // #ifdef DECL_HAS_NORMAL1
}
#endif // #ifdef VERTEX_TWEENING

const posNormal ComputePosNormal(in const inVert input)
{
	posNormal ret;

	float3 inPos = GetInputPosition0(input);
	float3 inNormal = GetInputNormal0(input);

#if VERTEXBLEND == D3DVBF_TWEENING
	const float3 inPos2 = GetInputPosition1(input);
	const float3 inNorm2 = GetInputNormal1(input);
	inPos = lerp(inPos, inPos2, TWEENFACTOR); // https://docs.microsoft.com/en-us/windows/win32/direct3d9/vertex-tweening
	inNormal = lerp(inNormal, inNorm2, TWEENFACTOR); // https://docs.microsoft.com/en-us/windows/win32/direct3d9/vertex-tweening
#endif // #if VERTEXBLEND == D3DVBF_TWEENING

#ifdef INDEXEDVERTEXBLENDENABLE
	const int4 indices = GetVertexBlendIndices(input);
#else
	const int4 indices = int4(0, 1, 2, 3);
#endif

	const float4 blendweights = GetVertexBlendWeights(input);

#if VERTEXBLEND == D3DVBF_3WEIGHTS // 4 matrix blending
	const float3 blend0 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.x].wv).xyz;
	const float3 blend1 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.y].wv).xyz;
	const float3 blend2 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.z].wv).xyz;
	const float3 blend3 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.w].wv).xyz;
	const float3 blendNormal0 = mul(wvBlendingMatrices[indices.x].invTransposeWV, inNormal);
	const float3 blendNormal1 = mul(wvBlendingMatrices[indices.y].invTransposeWV, inNormal);
	const float3 blendNormal2 = mul(wvBlendingMatrices[indices.z].invTransposeWV, inNormal);
	const float3 blendNormal3 = mul(wvBlendingMatrices[indices.w].invTransposeWV, inNormal);
	const float residual = (1.0f - (blendweights.x + blendweights.y + blendweights.z) );
	ret.worldViewPos = blend0 * blendweights.x + blend1 * blendweights.y + blend2 * blendweights.z + blend3 * residual;
	ret.OPOS = mul(float4(ret.worldViewPos, 1.0f), proj);
	ret.oNormal = blendNormal0 * blendweights.x + blendNormal1 * blendweights.y + blendNormal2 * blendweights.z + blendNormal3 * residual;
#elif VERTEXBLEND == D3DVBF_2WEIGHTS // 3 matrix blending
	const float3 blend0 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.x].wv).xyz;
	const float3 blend1 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.y].wv).xyz;
	const float3 blend2 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.z].wv).xyz;
	const float3 blendNormal0 = mul(wvBlendingMatrices[indices.x].invTransposeWV, inNormal);
	const float3 blendNormal1 = mul(wvBlendingMatrices[indices.y].invTransposeWV, inNormal);
	const float3 blendNormal2 = mul(wvBlendingMatrices[indices.z].invTransposeWV, inNormal);
	const float residual = (1.0f - (blendweights.x + blendweights.y) );
	ret.worldViewPos = blend0 * blendweights.x + blend1 * blendweights.y + blend2 * residual;
	ret.OPOS = mul(float4(ret.worldViewPos, 1.0f), proj);
	ret.oNormal = blendNormal0 * blendweights.x + blendNormal1 * blendweights.y + blendNormal2 * residual;
#elif VERTEXBLEND == D3DVBF_1WEIGHTS // 2 matrix blending
	const float3 blend0 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.x].wv).xyz;
	const float3 blend1 = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.y].wv).xyz;
	const float3 blendNormal0 = mul(wvBlendingMatrices[indices.x].invTransposeWV, inNormal);
	const float3 blendNormal1 = mul(wvBlendingMatrices[indices.y].invTransposeWV, inNormal);
	ret.worldViewPos = lerp(blend0, blend1, blendweights.x);
	ret.OPOS = mul(float4(ret.worldViewPos, 1.0f), proj);
	ret.oNormal = lerp(blendNormal0, blendNormal1, blendweights.x);
#elif VERTEXBLEND == D3DVBF_0WEIGHTS // 1 matrix blending
	ret.worldViewPos = mul(float4(inPos, 1.0f), wvBlendingMatrices[indices.x].wv).xyz;
	ret.OPOS = mul(float4(ret.worldViewPos, 1.0f), proj);
	ret.oNormal = mul(wvBlendingMatrices[indices.x].invTransposeWV, inNormal);
#else
	ret.worldViewPos = mul(float4(inPos, 1.0f), wvBlendingMatrices[0].wv).xyz;
	ret.OPOS = mul(float4(inPos, 1.0f), wvp);
	ret.oNormal = mul(wvBlendingMatrices[0].invTransposeWV, inNormal);
#endif

#ifdef D3DRS_NORMALIZENORMALS // This corresponds to the D3DRS_NORMALIZENORMALS render state
	ret.oNormal = (length(ret.oNormal) == 0.0f ? float3(0.0f, 0.0f, 0.0f) : normalize(ret.oNormal) );
#endif // #ifdef NORMALIZE_NORMALS

	return ret;
}

const float3 AutoGenTexcoords_CAMERASPACENORMAL(in const posNormal viewspacePN)
{
	return viewspacePN.oNormal;
}

const float3 AutoGenTexcoords_CAMERASPACEPOSITION(in const posNormal viewspacePN)
{
	return viewspacePN.worldViewPos;
}

const float3 AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(in const posNormal viewspacePN)
{
	const float3 normalizedViewPos = normalize(viewspacePN.worldViewPos);
	const float scaleFactor = dot(normalizedViewPos, viewspacePN.oNormal) * 2.0f;
	return mad(-scaleFactor, viewspacePN.oNormal, normalizedViewPos);
}

const float2 AutoGenTexcoords_SPHEREMAP(in const posNormal viewspacePN)
{
	const float3 normalizedViewPos = normalize(viewspacePN.worldViewPos);
	const float rawSphereDirection = dot(viewspacePN.oNormal, normalizedViewPos) * 2.0f;
	float3 sphereReflectionVector = mad(-rawSphereDirection, viewspacePN.oNormal, normalizedViewPos);
	sphereReflectionVector.z -= 1.0f;
	sphereReflectionVector.z = length(sphereReflectionVector) * 0.5f;
	const float2 retVec = mad(sphereReflectionVector.xy, sphereReflectionVector.z, 0.5f);
	return retVec;
}

const TEX0TYPE GetTex0(in const inVert input)
{
#ifdef INPUT_HAS_TEXCOORD0
	return input.TEX0;
#else // #ifdef INPUT_HAS_TEXCOORD0
	return 0.0f;
#endif // #ifdef INPUT_HAS_TEXCOORD0
}

const TEX1TYPE GetTex1(in const inVert input)
{
#ifdef INPUT_HAS_TEXCOORD1
	return input.TEX1;
#else // #ifdef INPUT_HAS_TEXCOORD1
	return 0.0f;
#endif // #ifdef INPUT_HAS_TEXCOORD1
}

const TEX2TYPE GetTex2(in const inVert input)
{
#ifdef INPUT_HAS_TEXCOORD2
	return input.TEX2;
#else // #ifdef INPUT_HAS_TEXCOORD2
	return 0.0f;
#endif // #ifdef INPUT_HAS_TEXCOORD2
}

const TEX3TYPE GetTex3(in const inVert input)
{
#ifdef INPUT_HAS_TEXCOORD3
	return input.TEX3;
#else // #ifdef INPUT_HAS_TEXCOORD3
	return 0.0f;
#endif // #ifdef INPUT_HAS_TEXCOORD3
}

const TEX4TYPE GetTex4(in const inVert input)
{
#ifdef INPUT_HAS_TEXCOORD4
	return input.TEX4;
#else // #ifdef INPUT_HAS_TEXCOORD4
	return 0.0f;
#endif // #ifdef INPUT_HAS_TEXCOORD4
}

const TEX5TYPE GetTex5(in const inVert input)
{
#ifdef INPUT_HAS_TEXCOORD5
	return input.TEX5;
#else // #ifdef INPUT_HAS_TEXCOORD5
	return 0.0f;
#endif // #ifdef INPUT_HAS_TEXCOORD5
}

const TEX6TYPE GetTex6(in const inVert input)
{
#ifdef INPUT_HAS_TEXCOORD6
	return input.TEX6;
#else // #ifdef INPUT_HAS_TEXCOORD6
	return 0.0f;
#endif // #ifdef INPUT_HAS_TEXCOORD6
}

const TEX7TYPE GetTex7(in const inVert input)
{
#ifdef INPUT_HAS_TEXCOORD7
	return input.TEX7;
#else // #ifdef INPUT_HAS_TEXCOORD7
	return 0.0f;
#endif // #ifdef INPUT_HAS_TEXCOORD7
}

const float4 TransformOutTex0(in const float4 inTex)
{
#ifdef USE_TEXTRANSFORM0 // If enabled, texcoord transforms take place last (after D3DTSS_TEXCOORDINDEX selection, and after possible auto-generation of texcoords)
	return mul(inTex, texMatrices[0]);
#else // #ifdef USE_TEXTRANSFORM0
	return inTex;
#endif // #ifdef USE_TEXTRANSFORM0
}

const float4 TransformOutTex1(in const float4 inTex)
{
#ifdef USE_TEXTRANSFORM1 // If enabled, texcoord transforms take place last (after D3DTSS_TEXCOORDINDEX selection, and after possible auto-generation of texcoords)
	return mul(inTex, texMatrices[1]);
#else // #ifdef USE_TEXTRANSFORM1
	return inTex;
#endif // #ifdef USE_TEXTRANSFORM1
}

const float4 TransformOutTex2(in const float4 inTex)
{
#ifdef USE_TEXTRANSFORM2 // If enabled, texcoord transforms take place last (after D3DTSS_TEXCOORDINDEX selection, and after possible auto-generation of texcoords)
	return mul(inTex, texMatrices[2]);
#else // #ifdef USE_TEXTRANSFORM2
	return inTex;
#endif // #ifdef USE_TEXTRANSFORM2
}

const float4 TransformOutTex3(in const float4 inTex)
{
#ifdef USE_TEXTRANSFORM3 // If enabled, texcoord transforms take place last (after D3DTSS_TEXCOORDINDEX selection, and after possible auto-generation of texcoords)
	return mul(inTex, texMatrices[3]);
#else // #ifdef USE_TEXTRANSFORM3
	return inTex;
#endif // #ifdef USE_TEXTRANSFORM3
}

const float4 TransformOutTex4(in const float4 inTex)
{
#ifdef USE_TEXTRANSFORM4 // If enabled, texcoord transforms take place last (after D3DTSS_TEXCOORDINDEX selection, and after possible auto-generation of texcoords)
	return mul(inTex, texMatrices[4]);
#else // #ifdef USE_TEXTRANSFORM4
	return inTex;
#endif // #ifdef USE_TEXTRANSFORM4
}

const float4 TransformOutTex5(in const float4 inTex)
{
#ifdef USE_TEXTRANSFORM5 // If enabled, texcoord transforms take place last (after D3DTSS_TEXCOORDINDEX selection, and after possible auto-generation of texcoords)
	return mul(inTex, texMatrices[5]);
#else // #ifdef USE_TEXTRANSFORM5
	return inTex;
#endif // #ifdef USE_TEXTRANSFORM5
}

const float4 TransformOutTex6(in const float4 inTex)
{
#ifdef USE_TEXTRANSFORM6 // If enabled, texcoord transforms take place last (after D3DTSS_TEXCOORDINDEX selection, and after possible auto-generation of texcoords)
	return mul(inTex, texMatrices[6]);
#else // #ifdef USE_TEXTRANSFORM6
	return inTex;
#endif // #ifdef USE_TEXTRANSFORM6
}

const float4 TransformOutTex7(in const float4 inTex)
{
#ifdef USE_TEXTRANSFORM7 // If enabled, texcoord transforms take place last (after D3DTSS_TEXCOORDINDEX selection, and after possible auto-generation of texcoords)
	return mul(inTex, texMatrices[7]);
#else // #ifdef USE_TEXTRANSFORM7
	return inTex;
#endif // #ifdef USE_TEXTRANSFORM7
}

#ifdef OUTPUT_POINTSIZE
const float CalculatePointSizeOut(const float pointSize, const float3 viewspaceVertexPos)
{
	const float distEye = length(viewspaceVertexPos); // This is named "De" in the official D3D9 docs
	const float innerTerm = D3DRS_POINTSCALE_A + D3DRS_POINTSCALE_B * distEye + D3DRS_POINTSCALE_C * (distEye * distEye);
	[branch] if (innerTerm < 0.0f) // Branching here because the real FF VS branches on this too
	{
		return D3DRS_POINTSIZE_MAX;
	}
	else
	{
		// So this is strange, but the D3D9 documentation states that you need to clamp to D3DRS_POINTSIZE_MIN as well as D3DRS_POINTSIZE_MAX, however the FF VS only
		// seems to ever clamp to D3DRS_POINTSIZE_MAX. Maybe there's something weird going on that makes the min never needed?
		const float normalizedTerm = rsqrt(innerTerm);
		return normalizedTerm * pointSize * ViewportHeight;
	}
}
#endif // #ifdef OUTPUT_POINTSIZE

const float CalculateFogOut(const float3 viewspaceVertexPos)
{
#if D3DRS_FOGVERTEXMODE != D3DFOG_NONE
#ifdef D3DRS_RANGEFOGENABLE
	const float fogDistance = length(viewspaceVertexPos);
#else
	const float fogDistance = abs(viewspaceVertexPos.z);
#endif
#endif // #if D3DRS_FOGVERTEXMODE != D3DFOG_NONE

#if D3DRS_FOGVERTEXMODE == D3DFOG_NONE
	return 1.0f;
#elif D3DRS_FOGVERTEXMODE == D3DFOG_EXP
	const float dTimesDensity = fogDistance * D3DRS_FOGDENSITY;
	return exp(-dTimesDensity);
#elif D3DRS_FOGVERTEXMODE == D3DFOG_EXP2
	const float dTimesDensity = fogDistance * D3DRS_FOGDENSITY;
	const float dTimesDensitySquared = dTimesDensity * dTimesDensity;
	return exp(-dTimesDensitySquared);
#elif D3DRS_FOGVERTEXMODE == D3DFOG_LINEAR
	// These are the two constants used by the real FFVS. It precomputes these constants and stores them in c9.xy, then uses them to compute a mad() into oFog
	const float fogRange = D3DRS_FOGEND - D3DRS_FOGSTART;
	const float FFVSx = -1.0f / fogRange;
	const float FFVSy = D3DRS_FOGEND / fogRange;
	return mad(fogDistance, FFVSx, FFVSy);
#else
	#error Unknown D3DRS_FOGVERTEXMODE specified!
	return 1.0f;
#endif
}

struct PerLightResults
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
};

#if NUM_ENABLED_LIGHTS >= 1
#define LIGHTTYPE LIGHTTYPE0
#define LIGHTINDEX 0
#include "FFVS_CalculateSingleLight.fxh"
#if NUM_ENABLED_LIGHTS >= 2
#define LIGHTTYPE LIGHTTYPE1
#define LIGHTINDEX 1
#include "FFVS_CalculateSingleLight.fxh"
#if NUM_ENABLED_LIGHTS >= 3
#define LIGHTTYPE LIGHTTYPE2
#define LIGHTINDEX 2
#include "FFVS_CalculateSingleLight.fxh"
#if NUM_ENABLED_LIGHTS >= 4
#define LIGHTTYPE LIGHTTYPE3
#define LIGHTINDEX 3
#include "FFVS_CalculateSingleLight.fxh"
#if NUM_ENABLED_LIGHTS >= 5
#define LIGHTTYPE LIGHTTYPE4
#define LIGHTINDEX 4
#include "FFVS_CalculateSingleLight.fxh"
#if NUM_ENABLED_LIGHTS >= 6
#define LIGHTTYPE LIGHTTYPE5
#define LIGHTINDEX 5
#include "FFVS_CalculateSingleLight.fxh"
#if NUM_ENABLED_LIGHTS >= 7
#define LIGHTTYPE LIGHTTYPE6
#define LIGHTINDEX 6
#include "FFVS_CalculateSingleLight.fxh"
#if NUM_ENABLED_LIGHTS == 8
#define LIGHTTYPE LIGHTTYPE7
#define LIGHTINDEX 7
#include "FFVS_CalculateSingleLight.fxh"
#endif // #if NUM_ENABLED_LIGHTS == 8
#endif // #if NUM_ENABLED_LIGHTS >= 7
#endif // #if NUM_ENABLED_LIGHTS >= 6
#endif // #if NUM_ENABLED_LIGHTS >= 5
#endif // #if NUM_ENABLED_LIGHTS >= 4
#endif // #if NUM_ENABLED_LIGHTS >= 3
#endif // #if NUM_ENABLED_LIGHTS >= 2
#endif // #if NUM_ENABLED_LIGHTS >= 1

const PerLightResults CalculateFixedFunctionLightingForAllLights(in const float3 viewspaceVertexPos, in const float3 viewspaceVertexNormal)
{
	PerLightResults allLightsResults;

#if NUM_ENABLED_LIGHTS < 1 || NUM_ENABLED_LIGHTS > 8
	allLightsResults.Ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	allLightsResults.Diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	allLightsResults.Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
	return allLightsResults;
#endif

#if NUM_ENABLED_LIGHTS >= 1
	{
	const PerLightResults light0result = CalculateFixedFunctionLightingForSingleLight0(enabledLightsData[0], viewspaceVertexPos, viewspaceVertexNormal);
	allLightsResults.Ambient = light0result.Ambient;
	allLightsResults.Diffuse = light0result.Diffuse;
	allLightsResults.Specular = light0result.Specular;
	}

#if NUM_ENABLED_LIGHTS >= 2
	{
	const PerLightResults light1result = CalculateFixedFunctionLightingForSingleLight1(enabledLightsData[1], viewspaceVertexPos, viewspaceVertexNormal);
	allLightsResults.Ambient += light1result.Ambient;
	allLightsResults.Diffuse += light1result.Diffuse;
	allLightsResults.Specular += light1result.Specular;
	}

#if NUM_ENABLED_LIGHTS >= 3
	{
	const PerLightResults light2result = CalculateFixedFunctionLightingForSingleLight2(enabledLightsData[2], viewspaceVertexPos, viewspaceVertexNormal);
	allLightsResults.Ambient += light2result.Ambient;
	allLightsResults.Diffuse += light2result.Diffuse;
	allLightsResults.Specular += light2result.Specular;
	}

#if NUM_ENABLED_LIGHTS >= 4
	{
	const PerLightResults light3result = CalculateFixedFunctionLightingForSingleLight3(enabledLightsData[3], viewspaceVertexPos, viewspaceVertexNormal);
	allLightsResults.Ambient += light3result.Ambient;
	allLightsResults.Diffuse += light3result.Diffuse;
	allLightsResults.Specular += light3result.Specular;
	}

#if NUM_ENABLED_LIGHTS >= 5
	{
	const PerLightResults light4result = CalculateFixedFunctionLightingForSingleLight4(enabledLightsData[4], viewspaceVertexPos, viewspaceVertexNormal);
	allLightsResults.Ambient += light4result.Ambient;
	allLightsResults.Diffuse += light4result.Diffuse;
	allLightsResults.Specular += light4result.Specular;
	}

#if NUM_ENABLED_LIGHTS >= 6
	{
	const PerLightResults light5result = CalculateFixedFunctionLightingForSingleLight5(enabledLightsData[5], viewspaceVertexPos, viewspaceVertexNormal);
	allLightsResults.Ambient += light5result.Ambient;
	allLightsResults.Diffuse += light5result.Diffuse;
	allLightsResults.Specular += light5result.Specular;
	}

#if NUM_ENABLED_LIGHTS >= 7
	{
	const PerLightResults light6result = CalculateFixedFunctionLightingForSingleLight6(enabledLightsData[6], viewspaceVertexPos, viewspaceVertexNormal);
	allLightsResults.Ambient += light6result.Ambient;
	allLightsResults.Diffuse += light6result.Diffuse;
	allLightsResults.Specular += light6result.Specular;
	}

#if NUM_ENABLED_LIGHTS == 8
	{
	const PerLightResults light7result = CalculateFixedFunctionLightingForSingleLight7(enabledLightsData[7], viewspaceVertexPos, viewspaceVertexNormal);
	allLightsResults.Ambient += light7result.Ambient;
	allLightsResults.Diffuse += light7result.Diffuse;
	allLightsResults.Specular += light7result.Specular;
	}

#endif // #if NUM_ENABLED_LIGHTS == 8
#endif // #if NUM_ENABLED_LIGHTS >= 7
#endif // #if NUM_ENABLED_LIGHTS >= 6
#endif // #if NUM_ENABLED_LIGHTS >= 5
#endif // #if NUM_ENABLED_LIGHTS >= 4
#endif // #if NUM_ENABLED_LIGHTS >= 3
#endif // #if NUM_ENABLED_LIGHTS >= 2
#endif // #if NUM_ENABLED_LIGHTS >= 1

	return allLightsResults;
}

const float4 CalculateFixedFunctionLighting(in const inVert input, const posNormal viewspacePN, out float4 outSpecular)
{
	const float4 materialAmbient = GetMaterialAmbient(input);
	const float3 emissive = GetMaterialEmissive(input);
#if NUM_ENABLED_LIGHTS >= 1
	const PerLightResults allLightsCombined = CalculateFixedFunctionLightingForAllLights(viewspacePN.worldViewPos, viewspacePN.oNormal);
	const float4 materialDiffuse = GetMaterialDiffuse(input);
	const float4 materialSpecular = GetMaterialSpecular(input);

	// These equations come from this page that describes how the D3D9 fixed-function pipeline handles lighting and materials: https://docs.microsoft.com/en-us/windows/win32/direct3d9/mathematics-of-lighting
	const float4 ambient = materialAmbient * (AMBIENT + allLightsCombined.Ambient);
	const float4 diffuse = materialDiffuse * allLightsCombined.Diffuse;
	const float4 specular = materialSpecular * allLightsCombined.Specular;

	outSpecular = specular;

	return ambient + diffuse + float4(emissive, 0.0f);
#else
	// With no lights enabled, the Diffuse and Specular terms completely fall away, and we're left with just Ambient and Emissive terms
	outSpecular = float4(0.0f, 0.0f, 0.0f, 1.0f);
	const float4 ambient = materialAmbient * AMBIENT;
	return ambient + float4(emissive, 0.0f);
#endif
}

const float4 ResolveTex0(const float4 tex0source, const posNormal viewspacePN)
{
#if D3DTSS_TEXCOORDINDEX0TYPE == D3DTSS_TCI_PASSTHRU
	return tex0source;
#elif D3DTSS_TEXCOORDINDEX0TYPE == D3DTSS_TCI_CAMERASPACENORMAL
	return float4(AutoGenTexcoords_CAMERASPACENORMAL(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX0TYPE == D3DTSS_TCI_CAMERASPACEPOSITION
	return float4(AutoGenTexcoords_CAMERASPACEPOSITION(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX0TYPE == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
	return float4(AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX0TYPE == D3DTSS_TCI_SPHEREMAP
	return float4(AutoGenTexcoords_SPHEREMAP(viewspacePN), 0.0f, 0.0f);
#else
	#error Error: Unknown D3DTSS_TEXCOORDINDEX0TYPE
#endif
}

const float4 ResolveTex1(const float4 tex1source, const posNormal viewspacePN)
{
#if D3DTSS_TEXCOORDINDEX1TYPE == D3DTSS_TCI_PASSTHRU
	return tex1source;
#elif D3DTSS_TEXCOORDINDEX1TYPE == D3DTSS_TCI_CAMERASPACENORMAL
	return float4(AutoGenTexcoords_CAMERASPACENORMAL(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX1TYPE == D3DTSS_TCI_CAMERASPACEPOSITION
	return float4(AutoGenTexcoords_CAMERASPACEPOSITION(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX1TYPE == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
	return float4(AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX1TYPE == D3DTSS_TCI_SPHEREMAP
	return float4(AutoGenTexcoords_SPHEREMAP(viewspacePN), 0.0f, 0.0f);
#else
	#error Error: Unknown D3DTSS_TEXCOORDINDEX1TYPE
#endif
}

const float4 ResolveTex2(const float4 tex2source, const posNormal viewspacePN)
{
#if D3DTSS_TEXCOORDINDEX2TYPE == D3DTSS_TCI_PASSTHRU
	return tex2source;
#elif D3DTSS_TEXCOORDINDEX2TYPE == D3DTSS_TCI_CAMERASPACENORMAL
	return float4(AutoGenTexcoords_CAMERASPACENORMAL(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX2TYPE == D3DTSS_TCI_CAMERASPACEPOSITION
	return float4(AutoGenTexcoords_CAMERASPACEPOSITION(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX2TYPE == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
	return float4(AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX2TYPE == D3DTSS_TCI_SPHEREMAP
	return float4(AutoGenTexcoords_SPHEREMAP(viewspacePN), 0.0f, 0.0f);
#else
	#error Error: Unknown D3DTSS_TEXCOORDINDEX2TYPE
#endif
}

const float4 ResolveTex3(const float4 tex3source, const posNormal viewspacePN)
{
#if D3DTSS_TEXCOORDINDEX3TYPE == D3DTSS_TCI_PASSTHRU
	return tex3source;
#elif D3DTSS_TEXCOORDINDEX3TYPE == D3DTSS_TCI_CAMERASPACENORMAL
	return float4(AutoGenTexcoords_CAMERASPACENORMAL(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX3TYPE == D3DTSS_TCI_CAMERASPACEPOSITION
	return float4(AutoGenTexcoords_CAMERASPACEPOSITION(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX3TYPE == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
	return float4(AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX3TYPE == D3DTSS_TCI_SPHEREMAP
	return float4(AutoGenTexcoords_SPHEREMAP(viewspacePN), 0.0f, 0.0f);
#else
	#error Error: Unknown D3DTSS_TEXCOORDINDEX3TYPE
#endif
}

const float4 ResolveTex4(const float4 tex4source, const posNormal viewspacePN)
{
#if D3DTSS_TEXCOORDINDEX4TYPE == D3DTSS_TCI_PASSTHRU
	return tex4source;
#elif D3DTSS_TEXCOORDINDEX4TYPE == D3DTSS_TCI_CAMERASPACENORMAL
	return float4(AutoGenTexcoords_CAMERASPACENORMAL(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX4TYPE == D3DTSS_TCI_CAMERASPACEPOSITION
	return float4(AutoGenTexcoords_CAMERASPACEPOSITION(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX4TYPE == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
	return float4(AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX4TYPE == D3DTSS_TCI_SPHEREMAP
	return float4(AutoGenTexcoords_SPHEREMAP(viewspacePN), 0.0f, 0.0f);
#else
	#error Error: Unknown D3DTSS_TEXCOORDINDEX4TYPE
#endif
}

const float4 ResolveTex5(const float4 tex5source, const posNormal viewspacePN)
{
#if D3DTSS_TEXCOORDINDEX5TYPE == D3DTSS_TCI_PASSTHRU
	return tex5source;
#elif D3DTSS_TEXCOORDINDEX5TYPE == D3DTSS_TCI_CAMERASPACENORMAL
	return float4(AutoGenTexcoords_CAMERASPACENORMAL(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX5TYPE == D3DTSS_TCI_CAMERASPACEPOSITION
	return float4(AutoGenTexcoords_CAMERASPACEPOSITION(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX5TYPE == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
	return float4(AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX5TYPE == D3DTSS_TCI_SPHEREMAP
	return float4(AutoGenTexcoords_SPHEREMAP(viewspacePN), 0.0f, 0.0f);
#else
	#error Error: Unknown D3DTSS_TEXCOORDINDEX5TYPE
#endif
}

const float4 ResolveTex6(const float4 tex6source, const posNormal viewspacePN)
{
#if D3DTSS_TEXCOORDINDEX6TYPE == D3DTSS_TCI_PASSTHRU
	return tex6source;
#elif D3DTSS_TEXCOORDINDEX6TYPE == D3DTSS_TCI_CAMERASPACENORMAL
	return float4(AutoGenTexcoords_CAMERASPACENORMAL(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX6TYPE == D3DTSS_TCI_CAMERASPACEPOSITION
	return float4(AutoGenTexcoords_CAMERASPACEPOSITION(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX6TYPE == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
	return float4(AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX6TYPE == D3DTSS_TCI_SPHEREMAP
	return float4(AutoGenTexcoords_SPHEREMAP(viewspacePN), 0.0f, 0.0f);
#else
	#error Error: Unknown D3DTSS_TEXCOORDINDEX6TYPE
#endif
}

const float4 ResolveTex7(const float4 tex7source, const posNormal viewspacePN)
{
#if D3DTSS_TEXCOORDINDEX7TYPE == D3DTSS_TCI_PASSTHRU
	return tex7source;
#elif D3DTSS_TEXCOORDINDEX7TYPE == D3DTSS_TCI_CAMERASPACENORMAL
	return float4(AutoGenTexcoords_CAMERASPACENORMAL(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX7TYPE == D3DTSS_TCI_CAMERASPACEPOSITION
	return float4(AutoGenTexcoords_CAMERASPACEPOSITION(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX7TYPE == D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR
	return float4(AutoGenTexcoords_CAMERASPACEREFLECTIONVECTOR(viewspacePN), 0.0f);
#elif D3DTSS_TEXCOORDINDEX7TYPE == D3DTSS_TCI_SPHEREMAP
	return float4(AutoGenTexcoords_SPHEREMAP(viewspacePN), 0.0f, 0.0f);
#else
	#error Error: Unknown D3DTSS_TEXCOORDINDEX7TYPE
#endif
}

outVert main(in const inVert input)
{
	outVert ret;

	const posNormal pn = ComputePosNormal(input);
	ret.OPOS = pn.OPOS;

	float4 specularColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

	ret.oDiffuse =
#ifdef LIGHTING
		CalculateFixedFunctionLighting(input, pn, specularColor);
#else // #ifdef LIGHTING
		GetVertexDiffuse(input);
#endif // #ifdef LIGHTING

#if defined(D3DRS_SPECULARENABLE) && defined(LIGHTING)
	ret.oSpecular = specularColor;
#endif // #if defined(D3DRS_SPECULARENABLE) && defined(LIGHTING)

#ifdef OUTPUT_POINTSIZE
	const float pointSize =
#ifdef PER_VERTEX_POINTSIZE
		input.PSIZ0;
#else // #ifdef PER_VERTEX_POINTSIZE
		D3DRS_POINTSIZE;
#endif // #ifdef PER_VERTEX_POINTSIZE
	ret.oPts = CalculatePointSizeOut(pointSize, pn.worldViewPos);
#endif // #ifdef OUTPUT_POINTSIZE

#ifdef OUTPUT_FOG
	ret.oFog = saturate(CalculateFogOut(pn.worldViewPos) );
#endif // #ifdef OUTPUT_FOG

	// For texture coordinates, the order in which they pass through the fixed-function pipeline when auto-generated texture coordinates are enabled for a texcoord channel is:
	// 1) Automatic texcoord generation (based on the camera data)
	// 2) Texture Transform matrix (optional)
	// 3) Output to the correct texcoord output register
	// For texture coordinates, the order in which they pass through the fixed-function pipeline when auto-generated texture coordinates are disabled for a texcoord channel is:
	// 1) Input and possible remapping (remapping is optional based on D3DTCI_ settings set via SetTextureStageState() calls)
	// 2) Texture Transform matrix (optional)
	// 3) Output to the correct texcoord output register
	const TEX0TYPE tex0 = GetTex0(input);
	const TEX1TYPE tex1 = GetTex1(input);
	const TEX2TYPE tex2 = GetTex2(input);
	const TEX3TYPE tex3 = GetTex3(input);
	const TEX4TYPE tex4 = GetTex4(input);
	const TEX5TYPE tex5 = GetTex5(input);
	const TEX6TYPE tex6 = GetTex6(input);
	const TEX7TYPE tex7 = GetTex7(input);

#ifdef OUTPUT_TEX0
	const float4 resolvedTex0 = ResolveTex0(TEX0REMAP, pn);
	ret.TEX0 = TransformOutTex0(resolvedTex0);
#endif // #ifdef OUTPUT_TEX0
#ifdef OUTPUT_TEX1
	const float4 resolvedTex1 = ResolveTex1(TEX1REMAP, pn);
	ret.TEX1 = TransformOutTex1(resolvedTex1);
#endif // #ifdef OUTPUT_TEX1
#ifdef OUTPUT_TEX2
	const float4 resolvedTex2 = ResolveTex2(TEX2REMAP, pn);
	ret.TEX2 = TransformOutTex2(resolvedTex2);
#endif // #ifdef OUTPUT_TEX2
#ifdef OUTPUT_TEX3
	const float4 resolvedTex3 = ResolveTex3(TEX3REMAP, pn);
	ret.TEX3 = TransformOutTex3(resolvedTex3);
#endif // #ifdef OUTPUT_TEX3
#ifdef OUTPUT_TEX4
	const float4 resolvedTex4 = ResolveTex4(TEX4REMAP, pn);
	ret.TEX4 = TransformOutTex4(resolvedTex4);
#endif // #ifdef OUTPUT_TEX4
#ifdef OUTPUT_TEX5
	const float4 resolvedTex5 = ResolveTex5(TEX5REMAP, pn);
	ret.TEX5 = TransformOutTex5(resolvedTex5);
#endif // #ifdef OUTPUT_TEX5
#ifdef OUTPUT_TEX6
	const float4 resolvedTex6 = ResolveTex6(TEX6REMAP, pn);
	ret.TEX6 = TransformOutTex6(resolvedTex6);
#endif // #ifdef OUTPUT_TEX6
#ifdef OUTPUT_TEX7
	const float4 resolvedTex7 = ResolveTex7(TEX7REMAP, pn);
	ret.TEX7 = TransformOutTex7(resolvedTex7);
#endif // #ifdef OUTPUT_TEX7

	return ret;
}
