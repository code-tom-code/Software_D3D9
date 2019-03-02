// Note: Allocate fixed-function registers backwards from 255 to avoid stomping the more commonly-used lower register indices (this may
// have issues with Unity games that perform automatic D3D9/D3D10 shadercode fixups, or with games that use many registers)

// This comes from SetTransform()
const row_major float4x4 worldViewProj : register(c252);

// This comes from the D3DMATERIALCOLORSOURCE enum in d3d9types.h
#define D3DMCS_MATERIAL 0
#define D3DMCS_COLOR1 1
#define D3DMCS_COLOR2 2

// This is a straight copy-paste of the D3DMATERIAL9 struct from d3d9types.h
struct D3DMATERIAL9
{
	float4 Diffuse;
	float4 Ambient;
	float4 Specular;
	float4 Emissive;
	float SpecularPower;
};

// These material parameters come from SetMaterial()
const D3DMATERIAL9 materialParameters : register(c247);

// This color comes from SetRenderState(D3DRS_AMBIENT)
#ifdef WITH_LIGHTING
	const float4 globalAmbientColor : register(c246);
#endif

struct inVert
{
	float3 inPos : POSITION;
	float4 diffuse : COLOR0;
	float4 specular : COLOR1;
#ifdef WITH_LIGHTING
	float3 normal : NORMAL0;
#endif
	float2 texCoord0 : TEXCOORD0;
};

struct outVert
{
	float4 outPos : POSITION;

	// The diffuse (COLOR0) element returned from a fixed function vertex shader should be a combination (addition) of the ambient, diffuse and emissive light values
	float4 ambientDiffuseEmissive : COLOR0;
#ifdef WITH_LIGHTING
	float4 specular : COLOR1;
#endif
	float2 texCoord0 : TEXCOORD0;
};

// https://docs.microsoft.com/en-us/windows/desktop/direct3d9/diffuse-lighting
const float4 GetDiffuseColor(in const inVert inputVert)
{
	float4 diffuseColor = materialParameters.Diffuse;
#ifdef WITH_COLORVERTEX
	#if defined(MATERIAL_DIFFUSE_SOURCE) && MATERIAL_DIFFUSE_SOURCE == D3DMCS_COLOR1
		diffuseColor = inputVert.diffuse;
	#elif defined(MATERIAL_DIFFUSE_SOURCE) && MATERIAL_DIFFUSE_SOURCE == D3DMCS_COLOR2
		diffuseColor = inputVert.specular;
	#endif
#else // #ifdef COLOR_VERTEX
	diffuseColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif // #ifdef COLOR_VERTEX
	return diffuseColor;
}

// https://docs.microsoft.com/en-us/windows/desktop/direct3d9/ambient-lighting
const float4 GetAmbientColor(in const inVert inputVert)
{
	float4 ambientColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
#ifdef WITH_LIGHTING
	#if defined(MATERIAL_AMBIENT_SOURCE) && MATERIAL_AMBIENT_SOURCE == D3DMCS_COLOR1
		ambientColor = inputVert.diffuse;
	#elif defined(MATERIAL_AMBIENT_SOURCE) && MATERIAL_AMBIENT_SOURCE == D3DMCS_COLOR2
		ambientColor = inputVert.specular;
	#else
		ambientColor = materialParameters.Ambient;
	#endif

	const float4 sumOfLightAmbients = float4(0.0f, 0.0f, 0.0f, 0.0f); // TODO: Calculate per-light ambient
	return ambientColor * (globalAmbientColor + sumOfLightAmbients); 
#endif // #ifdef WITH_LIGHTING
	return ambientColor;
}

// https://docs.microsoft.com/en-us/windows/desktop/direct3d9/specular-lighting
const float4 GetSpecularColor(in const inVert inputVert)
{
	float4 specularColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
#ifdef WITH_LIGHTING
	#if defined(MATERIAL_SPECULAR_SOURCE) && MATERIAL_SPECULAR_SOURCE == D3DMCS_COLOR1
		specularColor = inputVert.diffuse;
	#elif defined(MATERIAL_SPECULAR_SOURCE) && MATERIAL_SPECULAR_SOURCE == D3DMCS_COLOR2
		specularColor = inputVert.specular;
	#else
		specularColor = materialParameters.Specular;
	#endif
	const float4 sumOfLightSpeculars = float4(0.0f, 0.0f, 0.0f, 0.0f); // TODO: Calculate per-light specular
	return specularColor * sumOfLightSpeculars;
#endif // #ifdef WITH_LIGHTING
	return specularColor;
}

// https://docs.microsoft.com/en-us/windows/desktop/direct3d9/emissive-lighting
const float4 GetEmissiveColor(in const inVert inputVert)
{
	float4 emissiveColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
#ifdef WITH_LIGHTING
	#if defined(MATERIAL_EMISSIVE_SOURCE) && MATERIAL_EMISSIVE_SOURCE == D3DMCS_COLOR1
		emissiveColor = inputVert.diffuse;
	#elif defined(MATERIAL_EMISSIVE_SOURCE) && MATERIAL_EMISSIVE_SOURCE == D3DMCS_COLOR2
		emissiveColor = inputVert.specular;
	#else
		emissiveColor = materialParameters.Emissive;
	#endif
#endif // #ifdef WITH_LIGHTING
	return emissiveColor;
}

outVert main( in const inVert input)
{
	outVert ret;

	ret.outPos = mul(float4(input.inPos, 1.0f), worldViewProj);
	const float4 diffuse = GetDiffuseColor(input);
	float3 ambient = GetAmbientColor(input).rgb;
	float3 emissive = GetEmissiveColor(input).rgb;
#ifdef WITH_LIGHTING
	float3 vertNormal = input.normal;
	#ifdef NORMALIZE_NORMALS
		vertNormal = normalize(vertNormal);
	#endif // NORMALIZE_NORMALS
#endif // WITH_LIGHTING

#ifdef WITH_LIGHTING
	float4 specular = GetSpecularColor(input);

	// Specular components are clamped to be from 0 to 255, after all lights are processed and interpolated separately.
	ret.specular = saturate(specular);

	// Diffuse components are clamped to be from 0 to 255, after all lights are processed and interpolated separately.
	ret.ambientDiffuseEmissive.rgb = saturate(saturate(ambient) + saturate(diffuse.rgb) + saturate(emissive) );
#else
	ret.ambientDiffuseEmissive.rgb = saturate(diffuse.rgb);
#endif

	ret.ambientDiffuseEmissive.a = saturate(diffuse.a);

	ret.texCoord0 = input.texCoord0;

	return ret;
}
