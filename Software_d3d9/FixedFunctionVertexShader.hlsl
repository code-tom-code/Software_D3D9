// Allocate fixed-function registers backwards from 255 to avoid stomping the more commonly-used lower register indices (this may
// have issues with Unity games that perform automatic D3D9/D3D10 shadercode fixups, or with games that use many registers)
const row_major float4x4 worldViewProj : register(c252);

struct D3DMATERIAL9
{
	float4 Diffuse;
	float4 Ambient;
	float4 Specular;
	float4 Emissive;
	float SpecularPower;
};

const D3DMATERIAL9 materialParameters : register(c247);

#ifdef WITH_GLOBAL_AMBIENT
	const float4 globalAmbientColor : register(c246);
#endif

struct inVert
{
	float3 inPos : POSITION;
#ifdef WITH_COLORVERTEX
	float4 diffuse : COLOR0;
#endif
#ifdef WITH_LIGHTING
	float4 specular : COLOR1;
	float3 normal : NORMAL0;
#endif
	float2 texCoord0 : TEXCOORD0;
};

struct outVert
{
	float4 outPos : POSITION;
	float4 diffuse : COLOR0;
#ifdef WITH_LIGHTING
	float4 specular : COLOR1;
#endif
	float2 texCoord0 : TEXCOORD0;
};

outVert main( in const inVert input)
{
	outVert ret;

	ret.outPos = mul(float4(input.inPos, 1.0f), worldViewProj);
#ifdef WITH_COLORVERTEX
	ret.diffuse = input.diffuse;
#endif

#ifdef WITH_LIGHTING
	float3 vertNormal = input.normal;
	#ifdef NORMALIZE_NORMALS
		vertNormal = normalize(vertNormal);
	#endif // NORMALIZE_NORMALS
#endif // WITH_LIGHTING

#ifdef MATERIAL_DIFFUSE_OVERRIDES_VERTEX_DIFFUSE
	ret.diffuse = materialParameters.Diffuse;
#elif WITH_COLORVERTEX
	ret.diffuse = input.diffuse;
#else
	ret.diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif

#ifdef WITH_LIGHTING
	ret.specular = input.specular;
#endif
	ret.texCoord0 = input.texCoord0;

	return ret;
}
