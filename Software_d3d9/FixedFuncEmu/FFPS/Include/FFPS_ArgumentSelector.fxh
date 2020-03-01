#define MAKE_ARGSELECTOR_NAME(x) ArgumentSelect##x

inline const float4 MAKE_ARGSELECTOR_NAME(ARGNAME)(in const float4 diffuse, in const float4 specular, 
	in const float4 CURRENT, in const float4 currentStageTexture, in const float4 currentStateConstant, in const float4 TEMP, in const float TextureLumaScale)
{
#if ( (ARG) & (D3DTA_ALPHAREPLICATE | D3DTA_COMPLEMENT) ) == 0
	#if (ARG) == D3DTA_DIFFUSE
		return diffuse;
	#elif (ARG) == D3DTA_CURRENT
		#if STAGE_NUM == 0
			return diffuse;
		#else
			return CURRENT;
		#endif
	#elif (ARG) == D3DTA_TEXTURE
		#if STAGE_NUM == 0
			return currentStageTexture;
		#else
			return currentStageTexture * TextureLumaScale;
		#endif
	#elif (ARG) == D3DTA_TFACTOR
		return TFACTOR;
	#elif (ARG) == D3DTA_SPECULAR
		return specular;
	#elif (ARG) == D3DTA_TEMP
		return TEMP;
	#elif (ARG) == D3DTA_CONSTANT
		return currentStateConstant;
	#else
		#error ERROR: Invalid D3DTA type passed to ArgumentSelect!
	#endif
#else // #if (ARG & (D3DTA_ALPHAREPLICATE | D3DTA_COMPLEMENT) ) == 0
	float4 arg;
	#if ( (ARG) & D3DTA_SELECTMASK) == D3DTA_DIFFUSE
		arg = diffuse;
	#elif ( (ARG) & D3DTA_SELECTMASK) == D3DTA_CURRENT
		#if STAGE_NUM == 0
			arg = diffuse;
		#else
			arg = CURRENT;
		#endif
	#elif ( (ARG) & D3DTA_SELECTMASK) == D3DTA_TEXTURE
		arg = currentStageTexture;
	#elif ( (ARG) & D3DTA_SELECTMASK) == D3DTA_TFACTOR
		arg = TFACTOR;
	#elif ( (ARG) & D3DTA_SELECTMASK) == D3DTA_SPECULAR
		arg = specular;
	#elif ( (ARG) & D3DTA_SELECTMASK) == D3DTA_TEMP
		arg = TEMP;
		#if ARG == (D3DTA_ALPHAREPLICATE | D3DTA_TEMP) // This is some crazy strange undocumented behavior.
		arg.a = 1.0f;
		#endif // #if ARG == (D3DTA_ALPHAREPLICATE | TEMP)
	#elif ( (ARG) & D3DTA_SELECTMASK) == D3DTA_CONSTANT
		arg = currentStateConstant;
	#else
		#error ERROR: Invalid D3DTA type passed to ArgumentSelect!
	#endif

	// Note that the order of D3DTA_ALPHAREPLICATE and D3DTA_COMPLEMENT doesn't matter because these two modifiers act commutatively with one another:
	// args = 1.0 - args.aaaa;
	// is the same as:
	// args = args.a; args = 1.0f - args;
	#if ( ( (ARG) & (D3DTA_ALPHAREPLICATE | D3DTA_COMPLEMENT) ) == (D3DTA_ALPHAREPLICATE | D3DTA_COMPLEMENT) )
		float alphaArg = arg.a;
		alphaArg = 1.0f - alphaArg;
		arg.rgba = float4(alphaArg, alphaArg, alphaArg, alphaArg);
	#elif ( ( (ARG) & D3DTA_ALPHAREPLICATE) != 0)
		arg = arg.aaaa;
	#elif ( ( (ARG) & D3DTA_COMPLEMENT) != 0)
		arg = 1.0f - arg;
	#endif
	return arg;
#endif // #if (ARG & (D3DTA_ALPHAREPLICATE | D3DTA_COMPLEMENT) ) == 0
}

#undef MAKE_ARGSELECTOR_NAME
#undef ARG
#undef ARGNAME
