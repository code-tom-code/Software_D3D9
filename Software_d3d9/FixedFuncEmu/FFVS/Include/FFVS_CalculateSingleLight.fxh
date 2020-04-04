#ifndef LIGHTTYPE
	#error Error: Do not include this file without defining LIGHTTYPE first to one of: D3DLIGHT_POINT, D3DLIGHT_SPOT, or D3DLIGHT_DIRECTIONAL
#endif
#ifndef LIGHTINDEX
	#error Error: Do not include this file without defining LIGHTINDEX first!
#endif

#define FuncName(x) CalculateFixedFunctionLightingForSingleLight##x

const PerLightResults FuncName(LIGHTINDEX)(in const LightData lightdata, in const float3 viewspaceVertexPos, in const float3 viewspaceVertexNormal)
{
	PerLightResults ret;

	// Calculating Ldir (the light direction relative to the current vertex) is different for directional vs. point/spot lights: https://docs.microsoft.com/en-us/windows/win32/direct3d9/camera-space-transformations
#if LIGHTTYPE == D3DLIGHT_DIRECTIONAL
	const float3 vertexToLightDirNormalized = -lightdata.Direction.xyz;
#else
	const float3 vertexToLightDirRaw = lightdata.Position.xyz - viewspaceVertexPos;
	const float3 vertexToLightDirNormalized = normalize(vertexToLightDirRaw);
#endif

	// Attenuation is calculated based on the equations on this page: https://docs.microsoft.com/en-us/windows/win32/direct3d9/attenuation-and-spotlight-factor#attenuation
#if LIGHTTYPE == D3DLIGHT_POINT || LIGHTTYPE == D3DLIGHT_SPOT
	const float distToLight = length(vertexToLightDirRaw);
	float attenuation;
	[flatten] if (distToLight > lightdata.Position.w)
		attenuation = 0.0f;
	else
		attenuation = 1.0f / (lightdata.Attenuation.x + lightdata.Attenuation.y * distToLight + lightdata.Attenuation.z * (distToLight * distToLight) );
#endif // #if LIGHTTYPE == D3DLIGHT_POINT || LIGHTTYPE == D3DLIGHT_SPOT

	// Spotlight Factor is calculated based on the equations on this page: https://docs.microsoft.com/en-us/windows/win32/direct3d9/attenuation-and-spotlight-factor#spotlight-factor
#if LIGHTTYPE == D3DLIGHT_SPOT
	const float rho = -lightdata.Direction.xyz * vertexToLightDirNormalized;
	float spotlight;
	[flatten] if (rho > lightdata.SpotlightParams.z)
		spotlight = 1.0f;
	else if (rho <= lightdata.SpotlightParams.w)
		spotlight = 0.0f;
	else
		spotlight = pow( (rho - lightdata.SpotlightParams.w) * lightdata.Attenuation.w, lightdata.Direction.w);
#endif // #if LIGHTTYPE == D3DLIGHT_SPOT

#if (LIGHTTYPE < D3DLIGHT_POINT) || (LIGHTTYPE > D3DLIGHT_DIRECTIONAL)
	#error Error: Invalid light type for LIGHTTYPE
#endif

#if LIGHTTYPE == D3DLIGHT_POINT
	ret.Ambient = attenuation * lightdata.Ambient;
#elif LIGHTTYPE == D3DLIGHT_SPOT
	ret.Ambient = attenuation * spotlight * lightdata.Ambient;
#elif LIGHTTYPE == D3DLIGHT_DIRECTIONAL
	ret.Ambient = lightdata.Ambient;
#endif

#if LIGHTTYPE == D3DLIGHT_POINT
	const float NdotL = dot(viewspaceVertexNormal, vertexToLightDirNormalized);
	ret.Diffuse = (NdotL > 0.0f) ? (attenuation * NdotL * lightdata.Diffuse) : float4(0.0f, 0.0f, 0.0f, 0.0f);
#elif LIGHTTYPE == D3DLIGHT_SPOT
	const float NdotL = dot(viewspaceVertexNormal, vertexToLightDirNormalized);
	ret.Diffuse = (NdotL > 0.0f) ? (attenuation * spotlight * NdotL * lightdata.Diffuse) : float4(0.0f, 0.0f, 0.0f, 0.0f);
#elif LIGHTTYPE == D3DLIGHT_DIRECTIONAL
	const float NdotL = dot(viewspaceVertexNormal, lightdata.Direction.xyz);
	ret.Diffuse = (NdotL > 0.0f) ? (NdotL * lightdata.Diffuse) : float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif

	// D3DRS_LOCALVIEWER changes the computation of the halfway vector, so account for that here
#ifdef D3DRS_LOCALVIEWER
	#if LIGHTTYPE == D3DLIGHT_POINT
	const float3 halfVector = normalize(normalize(-viewspaceVertexPos) + vertexToLightDirNormalized);
	#elif LIGHTTYPE == D3DLIGHT_SPOT
	const float3 halfVector = normalize(normalize(-viewspaceVertexPos) + lightdata.Direction.xyz);
	#elif LIGHTTYPE == D3DLIGHT_DIRECTIONAL
	const float3 halfVector = normalize(normalize(-viewspaceVertexPos) + lightdata.Direction.xyz);
	#endif
#else // #ifdef D3DRS_LOCALVIEWER
	#if LIGHTTYPE == D3DLIGHT_POINT
	const float3 halfVector = normalize(float3(0.0f, 0.0f, 1.0f) + vertexToLightDirNormalized);
	#elif LIGHTTYPE == D3DLIGHT_SPOT
	const float3 halfVector = normalize(float3(0.0f, 0.0f, 1.0f) + lightdata.Direction.xyz);
	#elif LIGHTTYPE == D3DLIGHT_DIRECTIONAL
	const float3 halfVector = normalize(float3(0.0f, 0.0f, 1.0f) + lightdata.Direction.xyz);
	#endif
#endif // #ifdef D3DRS_LOCALVIEWER

#if LIGHTTYPE == D3DLIGHT_POINT
	const float specComponent = pow(dot(viewspaceVertexNormal, halfVector), materialData.Specular.w);
	ret.Specular = attenuation * specComponent * lightdata.Specular;
#elif LIGHTTYPE == D3DLIGHT_SPOT
	const float specComponent = pow(dot(viewspaceVertexNormal, halfVector), materialData.Specular.w);
	ret.Specular = attenuation * spotlight * specComponent * lightdata.Specular;
#elif LIGHTTYPE == D3DLIGHT_DIRECTIONAL
	const float specComponent = pow(dot(viewspaceVertexNormal, halfVector), materialData.Specular.w);
	ret.Specular = specComponent * lightdata.Specular;
#endif

	return ret;
}

#undef LIGHTINDEX
#undef LIGHTTYPE
