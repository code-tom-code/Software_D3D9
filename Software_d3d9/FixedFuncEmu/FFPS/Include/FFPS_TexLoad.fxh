#define MAKE_TEXLOAD_NAME(x) TexLoad##x
#define TEXTURENAME(x) texture##x

inline const float4 MAKE_TEXLOAD_NAME(STAGE_NUM)(in const float4 texcoords)
{
#ifndef HAS_TEX_BOUND
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
#else // #ifndef HAS_TEX_BOUND
	#if (TEXTYPE & TEXTYPEPROJ) != 0
		#if ( (TEXTYPE & 0x3) == TEXTYPE1D)
			float a;
			return tex1Dproj(TEXTURENAME(STAGE_NUM), texcoords);
		#elif ( (TEXTYPE & 0x3) == TEXTYPE2D)
			float b;
			return tex2Dproj(TEXTURENAME(STAGE_NUM), texcoords);
		#elif ( (TEXTYPE & 0x3) == TEXTYPE3D)
			float c;
			return tex3Dproj(TEXTURENAME(STAGE_NUM), texcoords);
		#elif ( (TEXTYPE & 0x3) == TEXTYPECUBE)
			float d;
			return texCUBEproj(TEXTURENAME(STAGE_NUM), texcoords);
		#else // // #if ( (TEXTYPE & 0x3) == TEXTYPE1D)
			#error INVALID TEXTYPE FOR PROJ TYPE!
		#endif // #if ( (TEXTYPE & 0x3) == TEXTYPE1D)
	#else // #if (TEXTYPE & TEXTYPEPROJ) != 0
		#if TEXTYPE == TEXTYPE1D
			float e;
			return tex1D(TEXTURENAME(STAGE_NUM), texcoords.x);
		#elif TEXTYPE == TEXTYPE2D
			float f;
			return tex2D(TEXTURENAME(STAGE_NUM), texcoords.xy);
		#elif TEXTYPE == TEXTYPE3D
			float g;
			return tex3D(TEXTURENAME(STAGE_NUM), texcoords.xyz);
		#elif TEXTYPE == TEXTYPECUBE
			float h;
			return texCUBE(TEXTURENAME(STAGE_NUM), texcoords.xyz);
		#else // #if TEXTYPE == TEXTYPE1D
			#error INVALID TEXTYPE FOR NON-PROJ TYPE!
		#endif // #if TEXTYPE == TEXTYPE1D
	#endif // #if (TEXTYPE & TEXTYPEPROJ) != 0
#endif // #ifndef HAS_TEX_BOUND
}

#undef TEXTURENAME
#undef MAKE_TEXLOAD_NAME
#undef HAS_TEX_BOUND
#undef TEXTYPE
