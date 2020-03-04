#define D3DTOP_DISABLE                   1
#define D3DTOP_SELECTARG1                2
#define D3DTOP_SELECTARG2                3
#define D3DTOP_MODULATE                  4
#define D3DTOP_MODULATE2X                5
#define D3DTOP_MODULATE4X                6
#define D3DTOP_ADD                       7
#define D3DTOP_ADDSIGNED                 8
#define D3DTOP_ADDSIGNED2X               9
#define D3DTOP_SUBTRACT                  10
#define D3DTOP_ADDSMOOTH                 11
#define D3DTOP_BLENDDIFFUSEALPHA         12
#define D3DTOP_BLENDTEXTUREALPHA         13
#define D3DTOP_BLENDFACTORALPHA          14
#define D3DTOP_BLENDTEXTUREALPHAPM       15
#define D3DTOP_BLENDCURRENTALPHA         16
#define D3DTOP_PREMODULATE               17
#define D3DTOP_MODULATEALPHA_ADDCOLOR    18
#define D3DTOP_MODULATECOLOR_ADDALPHA    19
#define D3DTOP_MODULATEINVALPHA_ADDCOLOR 20
#define D3DTOP_MODULATEINVCOLOR_ADDALPHA 21
#define D3DTOP_BUMPENVMAP                22
#define D3DTOP_BUMPENVMAPLUMINANCE       23
#define D3DTOP_DOTPRODUCT3               24
#define D3DTOP_MULTIPLYADD               25
#define D3DTOP_LERP                      26

#define D3DTA_SELECTMASK        0x0000000f  // mask for arg selector
#define D3DTA_DIFFUSE           0x00000000  // select diffuse color (read only)
#define D3DTA_CURRENT           0x00000001  // select stage destination register (read/write)
#define D3DTA_TEXTURE           0x00000002  // select texture color (read only)
#define D3DTA_TFACTOR           0x00000003  // select D3DRS_TEXTUREFACTOR (read only)
#define D3DTA_SPECULAR          0x00000004  // select specular color (read only)
#define D3DTA_TEMP              0x00000005  // select temporary register color (read/write)
#define D3DTA_CONSTANT          0x00000006  // select texture stage constant
#define D3DTA_COMPLEMENT        0x00000010  // take 1.0 - x (read modifier)
#define D3DTA_ALPHAREPLICATE    0x00000020  // replicate alpha to color components (read modifier)

#define TEXTYPE1D 0x0
#define TEXTYPE2D 0x1
#define TEXTYPE3D 0x2
#define TEXTYPECUBE 0x3
#define TEXTYPEPROJ 0x4

#ifndef SAMPLERTYPE0
	#define SAMPLERTYPE0 sampler2D
#endif // #ifndef SAMPLERTYPE0
#ifndef SAMPLERTYPE1
	#define SAMPLERTYPE1 sampler2D
#endif // #ifndef SAMPLERTYPE1
#ifndef SAMPLERTYPE2
	#define SAMPLERTYPE2 sampler2D
#endif // #ifndef SAMPLERTYPE2
#ifndef SAMPLERTYPE3
	#define SAMPLERTYPE3 sampler2D
#endif // #ifndef SAMPLERTYPE3
#ifndef SAMPLERTYPE4
	#define SAMPLERTYPE4 sampler2D
#endif // #ifndef SAMPLERTYPE4
#ifndef SAMPLERTYPE5
	#define SAMPLERTYPE5 sampler2D
#endif // #ifndef SAMPLERTYPE5
#ifndef SAMPLERTYPE6
	#define SAMPLERTYPE6 sampler2D
#endif // #ifndef SAMPLERTYPE6
#ifndef SAMPLERTYPE7
	#define SAMPLERTYPE7 sampler2D
#endif // #ifndef SAMPLERTYPE7

// Sample macro values follow for testing purposes (these should all be set programmatically!)
/*
#define D3DTSS_TEXCOORDINDEX0 0
#define D3DTSS_TEXCOORDINDEX1 1
#define D3DTSS_TEXCOORDINDEX2 2
#define D3DTSS_TEXCOORDINDEX3 3
#define D3DTSS_TEXCOORDINDEX4 4
#define D3DTSS_TEXCOORDINDEX5 5
#define D3DTSS_TEXCOORDINDEX6 6
#define D3DTSS_TEXCOORDINDEX7 7

#define D3DTSS_STAGE0_COLOROP D3DTOP_MODULATE
#define D3DTSS_STAGE0_ALPHAOP D3DTOP_SELECTARG1
#define D3DTSS_STAGE0_COLORARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE0_COLORARG2 D3DTA_CURRENT
#define D3DTSS_STAGE0_COLORARG0 D3DTA_CURRENT
#define D3DTSS_STAGE0_ALPHAARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE0_ALPHAARG2 D3DTA_CURRENT
#define D3DTSS_STAGE0_ALPHAARG0 D3DTA_CURRENT
#define D3DTSS_STAGE0_RESULTARG D3DTA_CURRENT

#define D3DTSS_STAGE1_COLOROP D3DTOP_MODULATE
#define D3DTSS_STAGE1_ALPHAOP D3DTOP_SELECTARG1
#define D3DTSS_STAGE1_COLORARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE1_COLORARG2 D3DTA_CURRENT
#define D3DTSS_STAGE1_COLORARG0 D3DTA_CURRENT
#define D3DTSS_STAGE1_ALPHAARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE1_ALPHAARG2 D3DTA_CURRENT
#define D3DTSS_STAGE1_ALPHAARG0 D3DTA_CURRENT
#define D3DTSS_STAGE1_RESULTARG D3DTA_CURRENT

#define D3DTSS_STAGE2_COLOROP D3DTOP_MODULATE
#define D3DTSS_STAGE2_ALPHAOP D3DTOP_SELECTARG1
#define D3DTSS_STAGE2_COLORARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE2_COLORARG2 D3DTA_CURRENT
#define D3DTSS_STAGE2_COLORARG0 D3DTA_CURRENT
#define D3DTSS_STAGE2_ALPHAARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE2_ALPHAARG2 D3DTA_CURRENT
#define D3DTSS_STAGE2_ALPHAARG0 D3DTA_CURRENT
#define D3DTSS_STAGE2_RESULTARG D3DTA_CURRENT

#define D3DTSS_STAGE3_COLOROP D3DTOP_MODULATE
#define D3DTSS_STAGE3_ALPHAOP D3DTOP_SELECTARG1
#define D3DTSS_STAGE3_COLORARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE3_COLORARG2 D3DTA_CURRENT
#define D3DTSS_STAGE3_COLORARG0 D3DTA_CURRENT
#define D3DTSS_STAGE3_ALPHAARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE3_ALPHAARG2 D3DTA_CURRENT
#define D3DTSS_STAGE3_ALPHAARG0 D3DTA_CURRENT
#define D3DTSS_STAGE3_RESULTARG D3DTA_CURRENT

#define D3DTSS_STAGE4_COLOROP D3DTOP_MODULATE
#define D3DTSS_STAGE4_ALPHAOP D3DTOP_SELECTARG1
#define D3DTSS_STAGE4_COLORARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE4_COLORARG2 D3DTA_CURRENT
#define D3DTSS_STAGE4_COLORARG0 D3DTA_CURRENT
#define D3DTSS_STAGE4_ALPHAARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE4_ALPHAARG2 D3DTA_CURRENT
#define D3DTSS_STAGE4_ALPHAARG0 D3DTA_CURRENT
#define D3DTSS_STAGE4_RESULTARG D3DTA_CURRENT

#define D3DTSS_STAGE5_COLOROP D3DTOP_MODULATE
#define D3DTSS_STAGE5_ALPHAOP D3DTOP_SELECTARG1
#define D3DTSS_STAGE5_COLORARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE5_COLORARG2 D3DTA_CURRENT
#define D3DTSS_STAGE5_COLORARG0 D3DTA_CURRENT
#define D3DTSS_STAGE5_ALPHAARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE5_ALPHAARG2 D3DTA_CURRENT
#define D3DTSS_STAGE5_ALPHAARG0 D3DTA_CURRENT
#define D3DTSS_STAGE5_RESULTARG D3DTA_CURRENT

#define D3DTSS_STAGE6_COLOROP D3DTOP_MODULATE
#define D3DTSS_STAGE6_ALPHAOP D3DTOP_SELECTARG1
#define D3DTSS_STAGE6_COLORARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE6_COLORARG2 D3DTA_CURRENT
#define D3DTSS_STAGE6_COLORARG0 D3DTA_CURRENT
#define D3DTSS_STAGE6_ALPHAARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE6_ALPHAARG2 D3DTA_CURRENT
#define D3DTSS_STAGE6_ALPHAARG0 D3DTA_CURRENT
#define D3DTSS_STAGE6_RESULTARG D3DTA_CURRENT

#define D3DTSS_STAGE7_COLOROP D3DTOP_MODULATE
#define D3DTSS_STAGE7_ALPHAOP D3DTOP_SELECTARG1
#define D3DTSS_STAGE7_COLORARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE7_COLORARG2 D3DTA_CURRENT
#define D3DTSS_STAGE7_COLORARG0 D3DTA_CURRENT
#define D3DTSS_STAGE7_ALPHAARG1 D3DTA_TEXTURE
#define D3DTSS_STAGE7_ALPHAARG2 D3DTA_CURRENT
#define D3DTSS_STAGE7_ALPHAARG0 D3DTA_CURRENT
#define D3DTSS_STAGE7_RESULTARG D3DTA_CURRENT

#define HAS_TEXCOORD0 1
#define HAS_TEXCOORD1 1
#define HAS_TEXCOORD2 1
#define HAS_TEXCOORD3 1
#define HAS_TEXCOORD4 1
#define HAS_TEXCOORD5 1
#define HAS_TEXCOORD6 1
#define HAS_TEXCOORD7 1
#define HAS_VERTEX_COLOR 1
#define HAS_SPECULAR 1
#define HAS_TEX0_BOUND 1
#define HAS_TEX1_BOUND 1
#define HAS_TEX2_BOUND 1
#define HAS_TEX3_BOUND 1
#define HAS_TEX4_BOUND 1
#define HAS_TEX5_BOUND 1
#define HAS_TEX6_BOUND 1
#define HAS_TEX7_BOUND 1
*/
// End sample macro values for testing

const float4 TFACTOR : register(c0); // This is the value set by SetRenderState(D3DTA_TFACTOR). It is shared across all texture stages.
const float4 FOGCOLOR : register(c1); // This is the value set by SetRenderState(D3DRS_FOGCOLOR). It is shared across all texture stages, and only applied if fog is enabled.
const float4 TEXTURESTAGE_CONSTANT[8] : register(c2); // This encompasses c2 thru c9. These are the values set by SetTextureStageState(x, D3DTSS_CONSTANT).
const row_major float2x4 BUMPENVMAT[8] : register(c10); // This encompasses c10 thru c27. These are the values set by SetTextureStageState(x, D3DTSS_BUMPENVMAT00 through D3DTSS_BUMPENVMAT11). The BUMPENVLUMASCALE and BUMPENVLUMAOFFSET values are stored in the .w components of each of the rows (scale in row 0's w, offset in row 1's w).
SAMPLERTYPE0 texture0 : register(s0);
SAMPLERTYPE1 texture1 : register(s1);
SAMPLERTYPE2 texture2 : register(s2);
SAMPLERTYPE3 texture3 : register(s3);
SAMPLERTYPE4 texture4 : register(s4);
SAMPLERTYPE5 texture5 : register(s5);
SAMPLERTYPE6 texture6 : register(s6);
SAMPLERTYPE7 texture7 : register(s7);

#ifdef FLAT_SHADING
	#define LIGHTING_INTERPOLATION nointerpolation
#else
	#define LIGHTING_INTERPOLATION linear
#endif

struct inProcessedVert
{
	LIGHTING_INTERPOLATION float4 diffuse : COLOR0;
	LIGHTING_INTERPOLATION float4 specular : COLOR1;

#ifdef FOG_ENABLE
	float fog : FOG;
#endif

#ifdef HAS_TEXCOORD0
	float4 texCoords0 : TEXCOORD0;
#endif // #ifdef HAS_TEXCOORD0
#ifdef HAS_TEXCOORD1
	float4 texCoords1 : TEXCOORD1;
#endif // #ifdef HAS_TEXCOORD1
#ifdef HAS_TEXCOORD2
	float4 texCoords2 : TEXCOORD2;
#endif // #ifdef HAS_TEXCOORD2
#ifdef HAS_TEXCOORD3
	float4 texCoords3 : TEXCOORD3;
#endif // #ifdef HAS_TEXCOORD3
#ifdef HAS_TEXCOORD4
	float4 texCoords4 : TEXCOORD4;
#endif // #ifdef HAS_TEXCOORD4
#ifdef HAS_TEXCOORD5
	float4 texCoords5 : TEXCOORD5;
#endif // #ifdef HAS_TEXCOORD5
#ifdef HAS_TEXCOORD6
	float4 texCoords6 : TEXCOORD6;
#endif // #ifdef HAS_TEXCOORD6
#ifdef HAS_TEXCOORD7
	float4 texCoords7 : TEXCOORD7;
#endif // #ifdef HAS_TEXCOORD7
};

#if D3DTSS_STAGE0_COLOROP != D3DTOP_DISABLE
	#define STAGE_NUM 0
	#ifdef HAS_TEX0_BOUND
		#define HAS_TEX_BOUND
		#define TEXTYPE TEXTYPE0
	#endif
	#include "FFPS_TexLoad.fxh"
	#define ARGNAME STAGE0_COLORARG1
	#define ARG D3DTSS_STAGE0_COLORARG1
	#include "FFPS_ArgumentSelector.fxh"
	#define ARGNAME STAGE0_COLORARG2
	#define ARG D3DTSS_STAGE0_COLORARG2
	#include "FFPS_ArgumentSelector.fxh"
	#define ARGNAME STAGE0_COLORARG0
	#define ARG D3DTSS_STAGE0_COLORARG0
	#include "FFPS_ArgumentSelector.fxh"
	#define ARGNAME STAGE0_ALPHAARG1
	#define ARG D3DTSS_STAGE0_ALPHAARG1
	#include "FFPS_ArgumentSelector.fxh"
	#define ARGNAME STAGE0_ALPHAARG2
	#define ARG D3DTSS_STAGE0_ALPHAARG2
	#include "FFPS_ArgumentSelector.fxh"
	#define ARGNAME STAGE0_ALPHAARG0
	#define ARG D3DTSS_STAGE0_ALPHAARG0
	#include "FFPS_ArgumentSelector.fxh"
	#define COLOROP D3DTSS_STAGE0_COLOROP
	#define ALPHAOP D3DTSS_STAGE0_ALPHAOP
	#include "FFPS_ApplyColorStage.fxh"
	#undef ALPHAOP
	#undef COLOROP
	#undef STAGE_NUM

	#if D3DTSS_STAGE1_COLOROP != D3DTOP_DISABLE
		#define STAGE_NUM 1
		#ifdef HAS_TEX1_BOUND
			#define HAS_TEX_BOUND
			#define TEXTYPE TEXTYPE1
		#endif
		#include "FFPS_TexLoad.fxh"
		#define ARGNAME STAGE1_COLORARG1
		#define ARG D3DTSS_STAGE1_COLORARG1
		#include "FFPS_ArgumentSelector.fxh"
		#define ARGNAME STAGE1_COLORARG2
		#define ARG D3DTSS_STAGE1_COLORARG2
		#include "FFPS_ArgumentSelector.fxh"
		#define ARGNAME STAGE1_COLORARG0
		#define ARG D3DTSS_STAGE1_COLORARG0
		#include "FFPS_ArgumentSelector.fxh"
		#define ARGNAME STAGE1_ALPHAARG1
		#define ARG D3DTSS_STAGE1_ALPHAARG1
		#include "FFPS_ArgumentSelector.fxh"
		#define ARGNAME STAGE1_ALPHAARG2
		#define ARG D3DTSS_STAGE1_ALPHAARG2
		#include "FFPS_ArgumentSelector.fxh"
		#define ARGNAME STAGE1_ALPHAARG0
		#define ARG D3DTSS_STAGE1_ALPHAARG0
		#include "FFPS_ArgumentSelector.fxh"
		#define COLOROP D3DTSS_STAGE1_COLOROP
		#define ALPHAOP D3DTSS_STAGE1_ALPHAOP
		#include "FFPS_ApplyColorStage.fxh"
		#undef ALPHAOP
		#undef COLOROP
		#undef STAGE_NUM
		#if D3DTSS_STAGE2_COLOROP != D3DTOP_DISABLE
			#define STAGE_NUM 2
			#ifdef HAS_TEX2_BOUND
				#define HAS_TEX_BOUND
				#define TEXTYPE TEXTYPE2
			#endif
			#include "FFPS_TexLoad.fxh"
			#define ARGNAME STAGE2_COLORARG1
			#define ARG D3DTSS_STAGE2_COLORARG1
			#include "FFPS_ArgumentSelector.fxh"
			#define ARGNAME STAGE2_COLORARG2
			#define ARG D3DTSS_STAGE2_COLORARG2
			#include "FFPS_ArgumentSelector.fxh"
			#define ARGNAME STAGE2_COLORARG0
			#define ARG D3DTSS_STAGE2_COLORARG0
			#include "FFPS_ArgumentSelector.fxh"
			#define ARGNAME STAGE2_ALPHAARG1
			#define ARG D3DTSS_STAGE2_ALPHAARG1
			#include "FFPS_ArgumentSelector.fxh"
			#define ARGNAME STAGE2_ALPHAARG2
			#define ARG D3DTSS_STAGE2_ALPHAARG2
			#include "FFPS_ArgumentSelector.fxh"
			#define ARGNAME STAGE2_ALPHAARG0
			#define ARG D3DTSS_STAGE2_ALPHAARG0
			#include "FFPS_ArgumentSelector.fxh"
			#define COLOROP D3DTSS_STAGE2_COLOROP
			#define ALPHAOP D3DTSS_STAGE2_ALPHAOP
			#include "FFPS_ApplyColorStage.fxh"
			#undef ALPHAOP
			#undef COLOROP
			#undef STAGE_NUM
			#if D3DTSS_STAGE3_COLOROP != D3DTOP_DISABLE
				#define STAGE_NUM 3
				#ifdef HAS_TEX3_BOUND
					#define HAS_TEX_BOUND
					#define TEXTYPE TEXTYPE3
				#endif
				#include "FFPS_TexLoad.fxh"
				#define ARGNAME STAGE3_COLORARG1
				#define ARG D3DTSS_STAGE3_COLORARG1
				#include "FFPS_ArgumentSelector.fxh"
				#define ARGNAME STAGE3_COLORARG2
				#define ARG D3DTSS_STAGE3_COLORARG2
				#include "FFPS_ArgumentSelector.fxh"
				#define ARGNAME STAGE3_COLORARG0
				#define ARG D3DTSS_STAGE3_COLORARG0
				#include "FFPS_ArgumentSelector.fxh"
				#define ARGNAME STAGE3_ALPHAARG1
				#define ARG D3DTSS_STAGE3_ALPHAARG1
				#include "FFPS_ArgumentSelector.fxh"
				#define ARGNAME STAGE3_ALPHAARG2
				#define ARG D3DTSS_STAGE3_ALPHAARG2
				#include "FFPS_ArgumentSelector.fxh"
				#define ARGNAME STAGE3_ALPHAARG0
				#define ARG D3DTSS_STAGE3_ALPHAARG0
				#include "FFPS_ArgumentSelector.fxh"
				#define COLOROP D3DTSS_STAGE3_COLOROP
				#define ALPHAOP D3DTSS_STAGE3_ALPHAOP
				#include "FFPS_ApplyColorStage.fxh"
				#undef ALPHAOP
				#undef COLOROP
				#undef STAGE_NUM
				#if D3DTSS_STAGE4_COLOROP != D3DTOP_DISABLE
					#define STAGE_NUM 4
					#ifdef HAS_TEX4_BOUND
						#define HAS_TEX_BOUND
						#define TEXTYPE TEXTYPE4
					#endif
					#include "FFPS_TexLoad.fxh"
					#define ARGNAME STAGE4_COLORARG1
					#define ARG D3DTSS_STAGE4_COLORARG1
					#include "FFPS_ArgumentSelector.fxh"
					#define ARGNAME STAGE4_COLORARG2
					#define ARG D3DTSS_STAGE4_COLORARG2
					#include "FFPS_ArgumentSelector.fxh"
					#define ARGNAME STAGE4_COLORARG0
					#define ARG D3DTSS_STAGE4_COLORARG0
					#include "FFPS_ArgumentSelector.fxh"
					#define ARGNAME STAGE4_ALPHAARG1
					#define ARG D3DTSS_STAGE4_ALPHAARG1
					#include "FFPS_ArgumentSelector.fxh"
					#define ARGNAME STAGE4_ALPHAARG2
					#define ARG D3DTSS_STAGE4_ALPHAARG2
					#include "FFPS_ArgumentSelector.fxh"
					#define ARGNAME STAGE4_ALPHAARG0
					#define ARG D3DTSS_STAGE4_ALPHAARG0
					#include "FFPS_ArgumentSelector.fxh"
					#define COLOROP D3DTSS_STAGE4_COLOROP
					#define ALPHAOP D3DTSS_STAGE4_ALPHAOP
					#include "FFPS_ApplyColorStage.fxh"
					#undef ALPHAOP
					#undef COLOROP
					#undef STAGE_NUM
					#if D3DTSS_STAGE5_COLOROP != D3DTOP_DISABLE
						#define STAGE_NUM 5
						#ifdef HAS_TEX5_BOUND
							#define HAS_TEX_BOUND
							#define TEXTYPE TEXTYPE5
						#endif
						#include "FFPS_TexLoad.fxh"
						#define ARGNAME STAGE5_COLORARG1
						#define ARG D3DTSS_STAGE5_COLORARG1
						#include "FFPS_ArgumentSelector.fxh"
						#define ARGNAME STAGE5_COLORARG2
						#define ARG D3DTSS_STAGE5_COLORARG2
						#include "FFPS_ArgumentSelector.fxh"
						#define ARGNAME STAGE5_COLORARG0
						#define ARG D3DTSS_STAGE5_COLORARG0
						#include "FFPS_ArgumentSelector.fxh"
						#define ARGNAME STAGE5_ALPHAARG1
						#define ARG D3DTSS_STAGE5_ALPHAARG1
						#include "FFPS_ArgumentSelector.fxh"
						#define ARGNAME STAGE5_ALPHAARG2
						#define ARG D3DTSS_STAGE5_ALPHAARG2
						#include "FFPS_ArgumentSelector.fxh"
						#define ARGNAME STAGE5_ALPHAARG0
						#define ARG D3DTSS_STAGE5_ALPHAARG0
						#include "FFPS_ArgumentSelector.fxh"
						#define COLOROP D3DTSS_STAGE5_COLOROP
						#define ALPHAOP D3DTSS_STAGE5_ALPHAOP
						#include "FFPS_ApplyColorStage.fxh"
						#undef ALPHAOP
						#undef COLOROP
						#undef STAGE_NUM
						#if D3DTSS_STAGE6_COLOROP != D3DTOP_DISABLE
							#define STAGE_NUM 6
							#ifdef HAS_TEX6_BOUND
								#define HAS_TEX_BOUND
								#define TEXTYPE TEXTYPE6
							#endif
							#include "FFPS_TexLoad.fxh"
							#define ARGNAME STAGE6_COLORARG1
							#define ARG D3DTSS_STAGE6_COLORARG1
							#include "FFPS_ArgumentSelector.fxh"
							#define ARGNAME STAGE6_COLORARG2
							#define ARG D3DTSS_STAGE6_COLORARG2
							#include "FFPS_ArgumentSelector.fxh"
							#define ARGNAME STAGE6_COLORARG0
							#define ARG D3DTSS_STAGE6_COLORARG0
							#include "FFPS_ArgumentSelector.fxh"
							#define ARGNAME STAGE6_ALPHAARG1
							#define ARG D3DTSS_STAGE6_ALPHAARG1
							#include "FFPS_ArgumentSelector.fxh"
							#define ARGNAME STAGE6_ALPHAARG2
							#define ARG D3DTSS_STAGE6_ALPHAARG2
							#include "FFPS_ArgumentSelector.fxh"
							#define ARGNAME STAGE6_ALPHAARG0
							#define ARG D3DTSS_STAGE6_ALPHAARG0
							#include "FFPS_ArgumentSelector.fxh"
							#define COLOROP D3DTSS_STAGE6_COLOROP
							#define ALPHAOP D3DTSS_STAGE6_ALPHAOP
							#include "FFPS_ApplyColorStage.fxh"
							#undef ALPHAOP
							#undef COLOROP
							#undef STAGE_NUM
							#if D3DTSS_STAGE7_COLOROP != D3DTOP_DISABLE
								#define STAGE_NUM 7
								#ifdef HAS_TEX7_BOUND
									#define HAS_TEX_BOUND
									#define TEXTYPE TEXTYPE7
								#endif
								#include "FFPS_TexLoad.fxh"
								#define ARGNAME STAGE7_COLORARG1
								#define ARG D3DTSS_STAGE7_COLORARG1
								#include "FFPS_ArgumentSelector.fxh"
								#define ARGNAME STAGE7_COLORARG2
								#define ARG D3DTSS_STAGE7_COLORARG2
								#include "FFPS_ArgumentSelector.fxh"
								#define ARGNAME STAGE7_COLORARG0
								#define ARG D3DTSS_STAGE7_COLORARG0
								#include "FFPS_ArgumentSelector.fxh"
								#define ARGNAME STAGE7_ALPHAARG1
								#define ARG D3DTSS_STAGE7_ALPHAARG1
								#include "FFPS_ArgumentSelector.fxh"
								#define ARGNAME STAGE7_ALPHAARG2
								#define ARG D3DTSS_STAGE7_ALPHAARG2
								#include "FFPS_ArgumentSelector.fxh"
								#define ARGNAME STAGE7_ALPHAARG0
								#define ARG D3DTSS_STAGE7_ALPHAARG0
								#include "FFPS_ArgumentSelector.fxh"
								#define COLOROP D3DTSS_STAGE7_COLOROP
								#define ALPHAOP D3DTSS_STAGE7_ALPHAOP
								#include "FFPS_ApplyColorStage.fxh"
								#undef ALPHAOP
								#undef COLOROP
								#undef STAGE_NUM
							#endif // #if D3DTSS_STAGE7_COLOROP != D3DTOP_DISABLE
						#endif // #if D3DTSS_STAGE6_COLOROP != D3DTOP_DISABLE
					#endif // #if D3DTSS_STAGE5_COLOROP != D3DTOP_DISABLE
				#endif // #if D3DTSS_STAGE4_COLOROP != D3DTOP_DISABLE
			#endif // #if D3DTSS_STAGE3_COLOROP != D3DTOP_DISABLE
		#endif // #if D3DTSS_STAGE2_COLOROP != D3DTOP_DISABLE
	#endif // #if D3DTSS_STAGE1_COLOROP != D3DTOP_DISABLE
#endif // #if D3DTSS_STAGE0_COLOROP != D3DTOP_DISABLE

float4 main(const in inProcessedVert input) : COLOR0
{
	float4 texCoords[8];
#ifdef HAS_TEXCOORD0
	texCoords[0] = input.texCoords0;
#else
	texCoords[0] = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif
#ifdef HAS_TEXCOORD1
	texCoords[1] = input.texCoords1;
#else
	texCoords[1] = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif
#ifdef HAS_TEXCOORD2
	texCoords[2] = input.texCoords2;
#else
	texCoords[2] = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif
#ifdef HAS_TEXCOORD3
	texCoords[3] = input.texCoords3;
#else
	texCoords[3] = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif
#ifdef HAS_TEXCOORD4
	texCoords[4] = input.texCoords4;
#else
	texCoords[4] = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif
#ifdef HAS_TEXCOORD5
	texCoords[5] = input.texCoords5;
#else
	texCoords[5] = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif
#ifdef HAS_TEXCOORD6
	texCoords[6] = input.texCoords6;
#else
	texCoords[6] = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif
#ifdef HAS_TEXCOORD7
	texCoords[7] = input.texCoords7;
#else
	texCoords[7] = float4(0.0f, 0.0f, 0.0f, 0.0f);
#endif

	float4 texRegisters[8];

	const float4 diffuseColor = input.diffuse;
	const float4 specularColor = input.specular;

	// Docs for D3DTA_TEMP specify that it should be initialized to (0, 0, 0, 0), but it seems like it might actually be initialized to (0,0,0,1) when combined with ALPHAREPLICATE?
	float4 tempRegister = float4(0.0f, 0.0f, 0.0f, 0.0f);

	float NextStageLumaMultiplier = 1.0f;

	float4 currentRegister = diffuseColor;

#if D3DTSS_STAGE0_COLOROP != D3DTOP_DISABLE
	texRegisters[0] = TexLoad0(texCoords[D3DTSS_TEXCOORDINDEX0]);
	const float4 stage0colorArg1 = ArgumentSelectSTAGE0_COLORARG1(diffuseColor, specularColor, currentRegister, texRegisters[0], TEXTURESTAGE_CONSTANT[0], tempRegister, 1.0f);
	const float4 stage0colorArg2 = ArgumentSelectSTAGE0_COLORARG2(diffuseColor, specularColor, currentRegister, texRegisters[0], TEXTURESTAGE_CONSTANT[0], tempRegister, 1.0f);
	const float4 stage0colorArg0 = ArgumentSelectSTAGE0_COLORARG0(diffuseColor, specularColor, currentRegister, texRegisters[0], TEXTURESTAGE_CONSTANT[0], tempRegister, 1.0f);
	const float4 stage0alphaArg1 = ArgumentSelectSTAGE0_ALPHAARG1(diffuseColor, specularColor, currentRegister, texRegisters[0], TEXTURESTAGE_CONSTANT[0], tempRegister, 1.0f);
	const float4 stage0alphaArg2 = ArgumentSelectSTAGE0_ALPHAARG2(diffuseColor, specularColor, currentRegister, texRegisters[0], TEXTURESTAGE_CONSTANT[0], tempRegister, 1.0f);
	const float4 stage0alphaArg0 = ArgumentSelectSTAGE0_ALPHAARG0(diffuseColor, specularColor, currentRegister, texRegisters[0], TEXTURESTAGE_CONSTANT[0], tempRegister, 1.0f);
	const float3 stage0resultColor = ApplyColorStage0(stage0colorArg1, stage0colorArg2, stage0colorArg0, diffuseColor.a, texRegisters[0].a, currentRegister, texCoords[D3DTSS_TEXCOORDINDEX1], NextStageLumaMultiplier);
	const float stage0resultAlpha = ApplyAlphaStage0(stage0alphaArg1, stage0alphaArg2, stage0alphaArg0, diffuseColor.a, texRegisters[0].a, currentRegister);
#if D3DTSS_STAGE0_RESULTARG == D3DTA_CURRENT
	currentRegister
#elif D3DTSS_STAGE0_RESULTARG == D3DTA_TEMP
	tempRegister
#else
	#error ERROR: Unknown RESULTARG register used in stage 0!
#endif
	 = float4(stage0resultColor, stage0resultAlpha);
	#if D3DTSS_STAGE1_COLOROP != D3DTOP_DISABLE
		texRegisters[1] = TexLoad1(texCoords[D3DTSS_TEXCOORDINDEX1]);
		const float4 stage1colorArg1 = ArgumentSelectSTAGE1_COLORARG1(diffuseColor, specularColor, currentRegister, texRegisters[1], TEXTURESTAGE_CONSTANT[1], tempRegister, NextStageLumaMultiplier);
		const float4 stage1colorArg2 = ArgumentSelectSTAGE1_COLORARG2(diffuseColor, specularColor, currentRegister, texRegisters[1], TEXTURESTAGE_CONSTANT[1], tempRegister, NextStageLumaMultiplier);
		const float4 stage1colorArg0 = ArgumentSelectSTAGE1_COLORARG0(diffuseColor, specularColor, currentRegister, texRegisters[1], TEXTURESTAGE_CONSTANT[1], tempRegister, NextStageLumaMultiplier);
		const float4 stage1alphaArg1 = ArgumentSelectSTAGE1_ALPHAARG1(diffuseColor, specularColor, currentRegister, texRegisters[1], TEXTURESTAGE_CONSTANT[1], tempRegister, NextStageLumaMultiplier);
		const float4 stage1alphaArg2 = ArgumentSelectSTAGE1_ALPHAARG2(diffuseColor, specularColor, currentRegister, texRegisters[1], TEXTURESTAGE_CONSTANT[1], tempRegister, NextStageLumaMultiplier);
		const float4 stage1alphaArg0 = ArgumentSelectSTAGE1_ALPHAARG0(diffuseColor, specularColor, currentRegister, texRegisters[1], TEXTURESTAGE_CONSTANT[1], tempRegister, NextStageLumaMultiplier);
		const float3 stage1resultColor = ApplyColorStage1(stage1colorArg1, stage1colorArg2, stage1colorArg0, diffuseColor.a, texRegisters[1].a, currentRegister, texCoords[D3DTSS_TEXCOORDINDEX2], NextStageLumaMultiplier);
		const float stage1resultAlpha = ApplyAlphaStage1(stage1alphaArg1, stage1alphaArg2, stage1alphaArg0, diffuseColor.a, texRegisters[1].a, currentRegister);
		#if D3DTSS_STAGE1_RESULTARG == D3DTA_CURRENT
		currentRegister
		#elif D3DTSS_STAGE1_RESULTARG == D3DTA_TEMP
		tempRegister
		#else
			#error ERROR: Unknown RESULTARG register used in stage 1!
		#endif
		 = float4(stage1resultColor, stage1resultAlpha);
		#if D3DTSS_STAGE2_COLOROP != D3DTOP_DISABLE
			texRegisters[2] = TexLoad2(texCoords[D3DTSS_TEXCOORDINDEX2]);
			const float4 stage2colorArg1 = ArgumentSelectSTAGE2_COLORARG1(diffuseColor, specularColor, currentRegister, texRegisters[2], TEXTURESTAGE_CONSTANT[2], tempRegister, NextStageLumaMultiplier);
			const float4 stage2colorArg2 = ArgumentSelectSTAGE2_COLORARG2(diffuseColor, specularColor, currentRegister, texRegisters[2], TEXTURESTAGE_CONSTANT[2], tempRegister, NextStageLumaMultiplier);
			const float4 stage2colorArg0 = ArgumentSelectSTAGE2_COLORARG0(diffuseColor, specularColor, currentRegister, texRegisters[2], TEXTURESTAGE_CONSTANT[2], tempRegister, NextStageLumaMultiplier);
			const float4 stage2alphaArg1 = ArgumentSelectSTAGE2_ALPHAARG1(diffuseColor, specularColor, currentRegister, texRegisters[2], TEXTURESTAGE_CONSTANT[2], tempRegister, NextStageLumaMultiplier);
			const float4 stage2alphaArg2 = ArgumentSelectSTAGE2_ALPHAARG2(diffuseColor, specularColor, currentRegister, texRegisters[2], TEXTURESTAGE_CONSTANT[2], tempRegister, NextStageLumaMultiplier);
			const float4 stage2alphaArg0 = ArgumentSelectSTAGE2_ALPHAARG0(diffuseColor, specularColor, currentRegister, texRegisters[2], TEXTURESTAGE_CONSTANT[2], tempRegister, NextStageLumaMultiplier);
			const float3 stage2resultColor = ApplyColorStage2(stage2colorArg1, stage2colorArg2, stage2colorArg0, diffuseColor.a, texRegisters[2].a, currentRegister, texCoords[D3DTSS_TEXCOORDINDEX3], NextStageLumaMultiplier);
			const float stage2resultAlpha = ApplyAlphaStage2(stage2alphaArg1, stage2alphaArg2, stage2alphaArg0, diffuseColor.a, texRegisters[2].a, currentRegister);
			#if D3DTSS_STAGE2_RESULTARG == D3DTA_CURRENT
			currentRegister
			#elif D3DTSS_STAGE2_RESULTARG == D3DTA_TEMP
			tempRegister
			#else
				#error ERROR: Unknown RESULTARG register used in stage 2!
			#endif
			 = float4(stage2resultColor, stage2resultAlpha);
			#if D3DTSS_STAGE3_COLOROP != D3DTOP_DISABLE
				texRegisters[3] = TexLoad3(texCoords[D3DTSS_TEXCOORDINDEX3]);
				const float4 stage3colorArg1 = ArgumentSelectSTAGE3_COLORARG1(diffuseColor, specularColor, currentRegister, texRegisters[3], TEXTURESTAGE_CONSTANT[3], tempRegister, NextStageLumaMultiplier);
				const float4 stage3colorArg2 = ArgumentSelectSTAGE3_COLORARG2(diffuseColor, specularColor, currentRegister, texRegisters[3], TEXTURESTAGE_CONSTANT[3], tempRegister, NextStageLumaMultiplier);
				const float4 stage3colorArg0 = ArgumentSelectSTAGE3_COLORARG0(diffuseColor, specularColor, currentRegister, texRegisters[3], TEXTURESTAGE_CONSTANT[3], tempRegister, NextStageLumaMultiplier);
				const float4 stage3alphaArg1 = ArgumentSelectSTAGE3_ALPHAARG1(diffuseColor, specularColor, currentRegister, texRegisters[3], TEXTURESTAGE_CONSTANT[3], tempRegister, NextStageLumaMultiplier);
				const float4 stage3alphaArg2 = ArgumentSelectSTAGE3_ALPHAARG2(diffuseColor, specularColor, currentRegister, texRegisters[3], TEXTURESTAGE_CONSTANT[3], tempRegister, NextStageLumaMultiplier);
				const float4 stage3alphaArg0 = ArgumentSelectSTAGE3_ALPHAARG0(diffuseColor, specularColor, currentRegister, texRegisters[3], TEXTURESTAGE_CONSTANT[3], tempRegister, NextStageLumaMultiplier);
				const float3 stage3resultColor = ApplyColorStage3(stage3colorArg1, stage3colorArg2, stage3colorArg0, diffuseColor.a, texRegisters[3].a, currentRegister, texCoords[D3DTSS_TEXCOORDINDEX4], NextStageLumaMultiplier);
				const float stage3resultAlpha = ApplyAlphaStage3(stage3alphaArg1, stage3alphaArg2, stage3alphaArg0, diffuseColor.a, texRegisters[3].a, currentRegister);
				#if D3DTSS_STAGE3_RESULTARG == D3DTA_CURRENT
				currentRegister
				#elif D3DTSS_STAGE3_RESULTARG == D3DTA_TEMP
				tempRegister
				#else
					#error ERROR: Unknown RESULTARG register used in stage 3!
				#endif
				 = float4(stage3resultColor, stage3resultAlpha);
				#if D3DTSS_STAGE4_COLOROP != D3DTOP_DISABLE
					texRegisters[4] = TexLoad4(texCoords[D3DTSS_TEXCOORDINDEX4]);
					const float4 stage4colorArg1 = ArgumentSelectSTAGE4_COLORARG1(diffuseColor, specularColor, currentRegister, texRegisters[4], TEXTURESTAGE_CONSTANT[4], tempRegister, NextStageLumaMultiplier);
					const float4 stage4colorArg2 = ArgumentSelectSTAGE4_COLORARG2(diffuseColor, specularColor, currentRegister, texRegisters[4], TEXTURESTAGE_CONSTANT[4], tempRegister, NextStageLumaMultiplier);
					const float4 stage4colorArg0 = ArgumentSelectSTAGE4_COLORARG0(diffuseColor, specularColor, currentRegister, texRegisters[4], TEXTURESTAGE_CONSTANT[4], tempRegister, NextStageLumaMultiplier);
					const float4 stage4alphaArg1 = ArgumentSelectSTAGE4_ALPHAARG1(diffuseColor, specularColor, currentRegister, texRegisters[4], TEXTURESTAGE_CONSTANT[4], tempRegister, NextStageLumaMultiplier);
					const float4 stage4alphaArg2 = ArgumentSelectSTAGE4_ALPHAARG2(diffuseColor, specularColor, currentRegister, texRegisters[4], TEXTURESTAGE_CONSTANT[4], tempRegister, NextStageLumaMultiplier);
					const float4 stage4alphaArg0 = ArgumentSelectSTAGE4_ALPHAARG0(diffuseColor, specularColor, currentRegister, texRegisters[4], TEXTURESTAGE_CONSTANT[4], tempRegister, NextStageLumaMultiplier);
					const float3 stage4resultColor = ApplyColorStage4(stage4colorArg1, stage4colorArg2, stage4colorArg0, diffuseColor.a, texRegisters[4].a, currentRegister, texCoords[D3DTSS_TEXCOORDINDEX5], NextStageLumaMultiplier);
					const float stage4resultAlpha = ApplyAlphaStage4(stage4alphaArg1, stage4alphaArg2, stage4alphaArg0, diffuseColor.a, texRegisters[4].a, currentRegister);
					#if D3DTSS_STAGE4_RESULTARG == D3DTA_CURRENT
					currentRegister
					#elif D3DTSS_STAGE4_RESULTARG == D3DTA_TEMP
					tempRegister
					#else
						#error ERROR: Unknown RESULTARG register used in stage 4!
					#endif
					 = float4(stage4resultColor, stage4resultAlpha);
					#if D3DTSS_STAGE5_COLOROP != D3DTOP_DISABLE
						texRegisters[5] = TexLoad5(texCoords[D3DTSS_TEXCOORDINDEX5]);
						const float4 stage5colorArg1 = ArgumentSelectSTAGE5_COLORARG1(diffuseColor, specularColor, currentRegister, texRegisters[5], TEXTURESTAGE_CONSTANT[5], tempRegister, NextStageLumaMultiplier);
						const float4 stage5colorArg2 = ArgumentSelectSTAGE5_COLORARG2(diffuseColor, specularColor, currentRegister, texRegisters[5], TEXTURESTAGE_CONSTANT[5], tempRegister, NextStageLumaMultiplier);
						const float4 stage5colorArg0 = ArgumentSelectSTAGE5_COLORARG0(diffuseColor, specularColor, currentRegister, texRegisters[5], TEXTURESTAGE_CONSTANT[5], tempRegister, NextStageLumaMultiplier);
						const float4 stage5alphaArg1 = ArgumentSelectSTAGE5_ALPHAARG1(diffuseColor, specularColor, currentRegister, texRegisters[5], TEXTURESTAGE_CONSTANT[5], tempRegister, NextStageLumaMultiplier);
						const float4 stage5alphaArg2 = ArgumentSelectSTAGE5_ALPHAARG2(diffuseColor, specularColor, currentRegister, texRegisters[5], TEXTURESTAGE_CONSTANT[5], tempRegister, NextStageLumaMultiplier);
						const float4 stage5alphaArg0 = ArgumentSelectSTAGE5_ALPHAARG0(diffuseColor, specularColor, currentRegister, texRegisters[5], TEXTURESTAGE_CONSTANT[5], tempRegister, NextStageLumaMultiplier);
						const float3 stage5resultColor = ApplyColorStage5(stage5colorArg1, stage5colorArg2, stage5colorArg0, diffuseColor.a, texRegisters[5].a, currentRegister, texCoords[D3DTSS_TEXCOORDINDEX6], NextStageLumaMultiplier);
						const float stage5resultAlpha = ApplyAlphaStage5(stage5alphaArg1, stage5alphaArg2, stage5alphaArg0, diffuseColor.a, texRegisters[5].a, currentRegister);
						#if D3DTSS_STAGE5_RESULTARG == D3DTA_CURRENT
						currentRegister
						#elif D3DTSS_STAGE5_RESULTARG == D3DTA_TEMP
						tempRegister
						#else
							#error ERROR: Unknown RESULTARG register used in stage 5!
						#endif
						 = float4(stage5resultColor, stage5resultAlpha);
						#if D3DTSS_STAGE6_COLOROP != D3DTOP_DISABLE
							texRegisters[6] = TexLoad6(texCoords[D3DTSS_TEXCOORDINDEX6]);
							const float4 stage6colorArg1 = ArgumentSelectSTAGE6_COLORARG1(diffuseColor, specularColor, currentRegister, texRegisters[6], TEXTURESTAGE_CONSTANT[6], tempRegister, NextStageLumaMultiplier);
							const float4 stage6colorArg2 = ArgumentSelectSTAGE6_COLORARG2(diffuseColor, specularColor, currentRegister, texRegisters[6], TEXTURESTAGE_CONSTANT[6], tempRegister, NextStageLumaMultiplier);
							const float4 stage6colorArg0 = ArgumentSelectSTAGE6_COLORARG0(diffuseColor, specularColor, currentRegister, texRegisters[6], TEXTURESTAGE_CONSTANT[6], tempRegister, NextStageLumaMultiplier);
							const float4 stage6alphaArg1 = ArgumentSelectSTAGE6_ALPHAARG1(diffuseColor, specularColor, currentRegister, texRegisters[6], TEXTURESTAGE_CONSTANT[6], tempRegister, NextStageLumaMultiplier);
							const float4 stage6alphaArg2 = ArgumentSelectSTAGE6_ALPHAARG2(diffuseColor, specularColor, currentRegister, texRegisters[6], TEXTURESTAGE_CONSTANT[6], tempRegister, NextStageLumaMultiplier);
							const float4 stage6alphaArg0 = ArgumentSelectSTAGE6_ALPHAARG0(diffuseColor, specularColor, currentRegister, texRegisters[6], TEXTURESTAGE_CONSTANT[6], tempRegister, NextStageLumaMultiplier);
							const float3 stage6resultColor = ApplyColorStage6(stage6colorArg1, stage6colorArg2, stage6colorArg0, diffuseColor.a, texRegisters[6].a, currentRegister, texCoords[D3DTSS_TEXCOORDINDEX7], NextStageLumaMultiplier);
							const float stage6resultAlpha = ApplyAlphaStage6(stage6alphaArg1, stage6alphaArg2, stage6alphaArg0, diffuseColor.a, texRegisters[6].a, currentRegister);
							#if D3DTSS_STAGE6_RESULTARG == D3DTA_CURRENT
							currentRegister
							#elif D3DTSS_STAGE6_RESULTARG == D3DTA_TEMP
							tempRegister
							#else
								#error ERROR: Unknown RESULTARG register used in stage 6!
							#endif
							 = float4(stage6resultColor, stage6resultAlpha);
							#if D3DTSS_STAGE7_COLOROP != D3DTOP_DISABLE
								texRegisters[7] = TexLoad7(texCoords[D3DTSS_TEXCOORDINDEX7]);
								const float4 stage7colorArg1 = ArgumentSelectSTAGE7_COLORARG1(diffuseColor, specularColor, currentRegister, texRegisters[7], TEXTURESTAGE_CONSTANT[7], tempRegister, NextStageLumaMultiplier);
								const float4 stage7colorArg2 = ArgumentSelectSTAGE7_COLORARG2(diffuseColor, specularColor, currentRegister, texRegisters[7], TEXTURESTAGE_CONSTANT[7], tempRegister, NextStageLumaMultiplier);
								const float4 stage7colorArg0 = ArgumentSelectSTAGE7_COLORARG0(diffuseColor, specularColor, currentRegister, texRegisters[7], TEXTURESTAGE_CONSTANT[7], tempRegister, NextStageLumaMultiplier);
								const float4 stage7alphaArg1 = ArgumentSelectSTAGE7_ALPHAARG1(diffuseColor, specularColor, currentRegister, texRegisters[7], TEXTURESTAGE_CONSTANT[7], tempRegister, NextStageLumaMultiplier);
								const float4 stage7alphaArg2 = ArgumentSelectSTAGE7_ALPHAARG2(diffuseColor, specularColor, currentRegister, texRegisters[7], TEXTURESTAGE_CONSTANT[7], tempRegister, NextStageLumaMultiplier);
								const float4 stage7alphaArg0 = ArgumentSelectSTAGE7_ALPHAARG0(diffuseColor, specularColor, currentRegister, texRegisters[7], TEXTURESTAGE_CONSTANT[7], tempRegister, NextStageLumaMultiplier);
								float4 dummyPostLastStageTexcoords = float4(0.0f, 0.0f, 0.0f, 0.0f);
								const float3 stage7resultColor = ApplyColorStage7(stage7colorArg1, stage7colorArg2, stage7colorArg0, diffuseColor.a, texRegisters[7].a, currentRegister, dummyPostLastStageTexcoords, NextStageLumaMultiplier);
								const float stage7resultAlpha = ApplyAlphaStage7(stage7alphaArg1, stage7alphaArg2, stage7alphaArg0, diffuseColor.a, texRegisters[7].a, currentRegister);
								#if D3DTSS_STAGE7_RESULTARG == D3DTA_CURRENT
								currentRegister
								#elif D3DTSS_STAGE7_RESULTARG == D3DTA_TEMP
								tempRegister
								#else
									#error ERROR: Unknown RESULTARG register used in stage 7!
								#endif
								 = float4(stage7resultColor, stage7resultAlpha);
							#endif // #if D3DTSS_STAGE7_COLOROP != D3DTOP_DISABLE
						#endif // #if D3DTSS_STAGE6_COLOROP != D3DTOP_DISABLE
					#endif // #if D3DTSS_STAGE5_COLOROP != D3DTOP_DISABLE
				#endif // #if D3DTSS_STAGE4_COLOROP != D3DTOP_DISABLE
			#endif // #if D3DTSS_STAGE3_COLOROP != D3DTOP_DISABLE
		#endif // #if D3DTSS_STAGE2_COLOROP != D3DTOP_DISABLE
	#endif // #if D3DTSS_STAGE1_COLOROP != D3DTOP_DISABLE
#endif // #if D3DTSS_STAGE0_COLOROP != D3DTOP_DISABLE

#ifdef HAS_SPECULAR
	currentRegister.rgb += input.specular.rgb; // Specular .a is not used (it actually may be the vertex shader's oFog scalar register, so the pixel shader should ignore it)
#endif // HAS_SPECULAR

#ifdef FOG_ENABLE
	currentRegister.rgb = lerp(FOGCOLOR.rgb, currentRegister.rgb, input.fog);
#endif

	return saturate(currentRegister);
}
