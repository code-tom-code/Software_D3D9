#define MAKE_COLOR_STAGE_FUNCNAME(x) ApplyColorStage##x
#define MAKE_ALPHA_STAGE_FUNCNAME(x) ApplyAlphaStage##x

// Temp. defines for testing!
/*#define COLOROP D3DTOP_SELECTARG1
#define STAGE_NUM 0*/

inline float3 MAKE_COLOR_STAGE_FUNCNAME(STAGE_NUM)(in const float4 ARG1, 
	in const float4 ARG2,
	in const float4 ARG0,
	in const float VertexAlpha,
	in const float CurrentTexStageTextureAlpha,
	in const float4 CURRENT,
	inout float4 NextStageTexcoordsForBump,
	inout float NextStageLuma)
{
	// Reset the luma for the next stage:
#if COLOROP != D3DTOP_BUMPENVMAPLUMINANCE
	NextStageLuma = 1.0f;
#endif // #if COLOROP != D3DTOP_BUMPENVMAPLUMINANCE

#if COLOROP == D3DTOP_DISABLE
		return CURRENT.rgb;
#elif COLOROP == D3DTOP_SELECTARG1
		return ARG1.rgb;
#elif COLOROP == D3DTOP_SELECTARG2
		return ARG2.rgb;
#elif COLOROP == D3DTOP_MODULATE
		return ARG1.rgb * ARG2.rgb;
#elif COLOROP == D3DTOP_MODULATE2X
		return ARG1.rgb * ARG2.rgb * 2.0f;
#elif COLOROP == D3DTOP_MODULATE4X
		return ARG1.rgb * ARG2.rgb * 4.0f;
#elif COLOROP == D3DTOP_ADD
		return ARG1.rgb + ARG2.rgb;
#elif COLOROP == D3DTOP_ADDSIGNED
		return ARG1.rgb + ARG2.rgb - 0.5f;
#elif COLOROP == D3DTOP_ADDSIGNED2X
		return (ARG1.rgb + ARG2.rgb - 0.5f) * 2.0f;
#elif COLOROP == D3DTOP_SUBTRACT
		return ARG1.rgb - ARG2.rgb;
#elif COLOROP == D3DTOP_ADDSMOOTH
		return ARG1.rgb + ARG2.rgb - ARG1.rgb * ARG2.rgb;
#elif COLOROP == D3DTOP_BLENDDIFFUSEALPHA
		return ARG1.rgb * VertexAlpha + ARG2.rgb * (1.0f - VertexAlpha);
#elif COLOROP == D3DTOP_BLENDTEXTUREALPHA
		return ARG1.rgb * CurrentTexStageTextureAlpha + ARG2.rgb * (1.0f - CurrentTexStageTextureAlpha);
#elif COLOROP == D3DTOP_BLENDFACTORALPHA
		return ARG1.rgb * TFACTOR.a + ARG2.rgb * (1.0f - TFACTOR.a);
#elif COLOROP == D3DTOP_BLENDTEXTUREALPHAPM
		return ARG1.rgb + ARG2.rgb * (1.0f - CurrentTexStageTextureAlpha);
#elif COLOROP == D3DTOP_BLENDCURRENTALPHA
		return ARG1.rgb * CURRENT.a + ARG2.rgb * (1.0f - CURRENT.a);
#elif COLOROP == D3DTOP_PREMODULATE /* The output of stage n is arg1. Additionally, if there is a texture in stage n + 1, any D3DTA_CURRENT in stage n + 1 is premultiplied by texture in stage n + 1. */
		return ARG1.rgb;
#elif COLOROP == D3DTOP_MODULATEALPHA_ADDCOLOR
		return ARG1.rgb + ARG1.aaa * ARG2.rgb;
#elif COLOROP == D3DTOP_MODULATECOLOR_ADDALPHA
		return ARG1.rgb * ARG2.rgb + ARG1.aaa;
#elif COLOROP == D3DTOP_MODULATEINVALPHA_ADDCOLOR
		return (1.0f - ARG1.a) * ARG2.rgb + ARG1.rgb;
#elif COLOROP == D3DTOP_MODULATEINVCOLOR_ADDALPHA
		return (1.0f - ARG1.rgb) * ARG2.rgb + ARG1.aaa;
#elif COLOROP == D3DTOP_BUMPENVMAP
	#if STAGE_NUM < 7
		const float xComp = dot(ARG1.rgb, BUMPENVMAT[STAGE_NUM]._m00_m01_m02);
		const float yComp = dot(ARG1.rgb, BUMPENVMAT[STAGE_NUM]._m10_m11_m12);
		NextStageTexcoordsForBump.xy += float2(xComp, yComp);
	#endif // #if STAGE_NUM < 7
		return CURRENT.rgb;
#elif COLOROP == D3DTOP_BUMPENVMAPLUMINANCE
	#if STAGE_NUM < 7
		const float xComp = dot(ARG1.rgb, BUMPENVMAT[STAGE_NUM]._m00_m01_m02);
		const float yComp = dot(ARG1.rgb, BUMPENVMAT[STAGE_NUM]._m10_m11_m12);
		const float lumaScale = BUMPENVMAT[STAGE_NUM]._m03;
		const float lumaBias = BUMPENVMAT[STAGE_NUM]._m13;
		const float lumaValue = saturate(mad(ARG1.b, lumaScale, lumaBias) );
		NextStageTexcoordsForBump.xy += float2(xComp, yComp);
		NextStageLuma = lumaValue;
	#endif // #if STAGE_NUM < 7
		return CURRENT.rgb;
#elif COLOROP == D3DTOP_DOTPRODUCT3
		const float3 signedARG1 = (ARG1.rgb - 0.5f) * 4.0f;
		const float3 signedARG2 = (ARG2.rgb - 0.5f);
		const float dotResult = saturate(dot(signedARG1, signedARG2) );
		return float3(dotResult, dotResult, dotResult);
#elif COLOROP == D3DTOP_MULTIPLYADD
		return mad(ARG2.rgb, ARG1.rgb, ARG0.rgb);
#elif COLOROP == D3DTOP_LERP
		return lerp(ARG2.rgb, ARG1.rgb, ARG0.rgb);
#else
	#error ERROR: Unknown D3DTOP value passed to COLOROP used!
	//return float3(0.0f, 0.0f, 0.0f);
#endif
}

inline float MAKE_ALPHA_STAGE_FUNCNAME(STAGE_NUM)(in const float4 ARG1, 
	in const float4 ARG2,
	in const float4 ARG0,
	in const float VertexAlpha,
	in const float CurrentTexStageTextureAlpha,
	in const float4 CURRENT)
{

// This is a little bit strange, but in the case of DOT3 color-ops, they override whatever the alpha op is and broadcast
// the dot3 result into the alpha channel instead:
#if COLOROP == D3DTOP_DOTPRODUCT3
	{
		const float3 signedARG1 = (ARG1.rgb - 0.5f) * 4.0f;
		const float3 signedARG2 = (ARG2.rgb - 0.5f);
		const float dotResult = saturate(dot(signedARG1, signedARG2) );
		return dotResult;
	}
#endif

// It looks like D3DTOP_DISABLE passed to ALPHAOP means D3DTOP_SELECTARG1 and ARG1 = D3DTA_TFACTOR
#if ALPHAOP == D3DTOP_DISABLE
	return CURRENT.a;
	//return ARG1.a;
#elif ALPHAOP == D3DTOP_SELECTARG1
	return ARG1.a;
#elif ALPHAOP == D3DTOP_SELECTARG2
	return ARG2.a;
#elif ALPHAOP == D3DTOP_MODULATE
	return ARG1.a * ARG2.a;
#elif ALPHAOP == D3DTOP_MODULATE2X
	return ARG1.a * ARG2.a * 2.0f;
#elif ALPHAOP == D3DTOP_MODULATE4X
	return ARG1.a * ARG2.a * 4.0f;
#elif ALPHAOP == D3DTOP_ADD
	return ARG1.a + ARG2.a;
#elif ALPHAOP == D3DTOP_ADDSIGNED
	return ARG1.a + ARG2.a - 0.5f;
#elif ALPHAOP == D3DTOP_ADDSIGNED2X
	return (ARG1.a + ARG2.a - 0.5f) * 2.0f;
#elif ALPHAOP == D3DTOP_SUBTRACT
	return ARG1.a - ARG2.a;
#elif ALPHAOP == D3DTOP_ADDSMOOTH
	return ARG1.a + ARG2.a - ARG1.a * ARG2.a;
#elif ALPHAOP == D3DTOP_BLENDDIFFUSEALPHA
	return ARG1.a * VertexAlpha + ARG2.a * (1.0f - VertexAlpha);
#elif ALPHAOP == D3DTOP_BLENDTEXTUREALPHA
	return ARG1.a * CurrentTexStageTextureAlpha + ARG2.a * (1.0f - CurrentTexStageTextureAlpha);
#elif ALPHAOP == D3DTOP_BLENDFACTORALPHA
	return ARG1.a * TFACTOR.a + ARG2.a * (1.0f - TFACTOR.a);
#elif ALPHAOP == D3DTOP_BLENDTEXTUREALPHAPM
	return ARG1.a + ARG2.a * (1.0f - CurrentTexStageTextureAlpha);
#elif ALPHAOP == D3DTOP_BLENDCURRENTALPHA
	return ARG1.a * CURRENT.a + ARG2.a * (1.0f - CURRENT.a);
#elif ALPHAOP == D3DTOP_PREMODULATE /* The output of stage n is arg1. Additionally, if there is a texture in stage n + 1, any D3DTA_CURRENT in stage n + 1 is premultiplied by texture in stage n + 1. */
	return ARG1.a;
#elif ALPHAOP == D3DTOP_MODULATEALPHA_ADDCOLOR // Note: This is technically illegal to use as an D3DTSS_ALPHAOP
	return 1.0f;
#elif ALPHAOP == D3DTOP_MODULATECOLOR_ADDALPHA // Note: This is technically illegal to use as an D3DTSS_ALPHAOP
	return 1.0f;
#elif ALPHAOP == D3DTOP_MODULATEINVALPHA_ADDCOLOR // Note: This is technically illegal to use as an D3DTSS_ALPHAOP
	return 1.0f;
#elif ALPHAOP == D3DTOP_MODULATEINVCOLOR_ADDALPHA // Note: This is technically illegal to use as an D3DTSS_ALPHAOP
	return 1.0f;
#elif ALPHAOP == D3DTOP_BUMPENVMAP // Note: This is technically illegal to use as an D3DTSS_ALPHAOP
	return 1.0f;
#elif ALPHAOP == D3DTOP_BUMPENVMAPLUMINANCE // Note: This is technically illegal to use as an D3DTSS_ALPHAOP
	return 1.0f;
#elif ALPHAOP == D3DTOP_DOTPRODUCT3
	const float3 signedARG1 = (ARG1.rgb - 0.5f) * 4.0f;
	const float3 signedARG2 = (ARG2.rgb - 0.5f);
	const float dotResult = saturate(dot(signedARG1, signedARG2) );
	return dotResult;
#elif ALPHAOP == D3DTOP_MULTIPLYADD
	return mad(ARG2.a, ARG1.a, ARG0.a);
#elif ALPHAOP == D3DTOP_LERP
	return lerp(ARG2.a, ARG1.a, ARG0.a);
#else
	#error Unknown D3DTSS_ALPHAOP value used!
	//return float3(0.0f, 0.0f, 0.0f);
#endif
}

#undef MAKE_ALPHA_STAGE_FUNCNAME
#undef MAKE_COLOR_STAGE_FUNCNAME
