#pragma once

// This is copied from d3d9types.h
enum DEBUGGABLE_D3DSHADER_INSTRUCTION_OPCODE_TYPE : unsigned short
{
    _D3DSIO_NOP          = 0,
    _D3DSIO_MOV          ,
    _D3DSIO_ADD          ,
    _D3DSIO_SUB          ,
    _D3DSIO_MAD          ,
    _D3DSIO_MUL          ,
    _D3DSIO_RCP          ,
    _D3DSIO_RSQ          ,
    _D3DSIO_DP3          ,
    _D3DSIO_DP4          ,
    _D3DSIO_MIN          ,
    _D3DSIO_MAX          ,
    _D3DSIO_SLT          ,
    _D3DSIO_SGE          ,
    _D3DSIO_EXP          ,
    _D3DSIO_LOG          ,
    _D3DSIO_LIT          ,
    _D3DSIO_DST          ,
    _D3DSIO_LRP          ,
    _D3DSIO_FRC          ,
    _D3DSIO_M4x4         ,
    _D3DSIO_M4x3         ,
    _D3DSIO_M3x4         ,
    _D3DSIO_M3x3         ,
    _D3DSIO_M3x2         ,
    _D3DSIO_CALL         ,
    _D3DSIO_CALLNZ       ,
    _D3DSIO_LOOP         ,
    _D3DSIO_RET          ,
    _D3DSIO_ENDLOOP      ,
    _D3DSIO_LABEL        ,
    _D3DSIO_DCL          ,
    _D3DSIO_POW          ,
    _D3DSIO_CRS          ,
    _D3DSIO_SGN          ,
    _D3DSIO_ABS          ,
    _D3DSIO_NRM          ,
    _D3DSIO_SINCOS       ,
    _D3DSIO_REP          ,
    _D3DSIO_ENDREP       ,
    _D3DSIO_IF           ,
    _D3DSIO_IFC          ,
    _D3DSIO_ELSE         ,
    _D3DSIO_ENDIF        ,
    _D3DSIO_BREAK        ,
    _D3DSIO_BREAKC       ,
    _D3DSIO_MOVA         ,
    _D3DSIO_DEFB         ,
    _D3DSIO_DEFI         ,

    _D3DSIO_TEXCOORD     = 64,
    _D3DSIO_TEXKILL      ,
    _D3DSIO_TEX          ,
    _D3DSIO_TEXBEM       ,
    _D3DSIO_TEXBEML      ,
    _D3DSIO_TEXREG2AR    ,
    _D3DSIO_TEXREG2GB    ,
    _D3DSIO_TEXM3x2PAD   ,
    _D3DSIO_TEXM3x2TEX   ,
    _D3DSIO_TEXM3x3PAD   ,
    _D3DSIO_TEXM3x3TEX   ,
    _D3DSIO_RESERVED0    ,
    _D3DSIO_TEXM3x3SPEC  ,
    _D3DSIO_TEXM3x3VSPEC ,
    _D3DSIO_EXPP         ,
    _D3DSIO_LOGP         ,
    _D3DSIO_CND          ,
    _D3DSIO_DEF          ,
    _D3DSIO_TEXREG2RGB   ,
    _D3DSIO_TEXDP3TEX    ,
    _D3DSIO_TEXM3x2DEPTH ,
    _D3DSIO_TEXDP3       ,
    _D3DSIO_TEXM3x3      ,
    _D3DSIO_TEXDEPTH     ,
    _D3DSIO_CMP          ,
    _D3DSIO_BEM          ,
    _D3DSIO_DP2ADD       ,
    _D3DSIO_DSX          ,
    _D3DSIO_DSY          ,
    _D3DSIO_TEXLDD       ,
    _D3DSIO_SETP         ,
    _D3DSIO_TEXLDL       ,
    _D3DSIO_BREAKP       ,

    _D3DSIO_PHASE        = 0xFFFD,
    _D3DSIO_COMMENT      = 0xFFFE,
    _D3DSIO_END          = 0xFFFF
};

enum debuggableSwizzleChannel : unsigned char
{
	sourceX = 0x0,
	sourceY = 0x1,
	sourceZ = 0x2,
	sourceW = 0x3
};

enum debuggableFullSwizzle : unsigned char
{
	_xxxx = 0x00, _rrrr = 0x00,
	_yxxx = 0x01, _grrr = 0x01,
	_zxxx = 0x02, _brrr = 0x02,
	_wxxx = 0x03, _arrr = 0x03,
	_xyxx = 0x04, _rgrr = 0x04,
	_yyxx = 0x05, _ggrr = 0x05,
	_zyxx = 0x06, _bgrr = 0x06,
	_wyxx = 0x07, _agrr = 0x07,
	_xzxx = 0x08, _rbrr = 0x08,
	_yzxx = 0x09, _gbrr = 0x09,
	_zzxx = 0x0A, _bbrr = 0x0A,
	_wzxx = 0x0B, _abrr = 0x0B,
	_xwxx = 0x0C, _rarr = 0x0C,
	_ywxx = 0x0D, _garr = 0x0D,
	_zwxx = 0x0E, _barr = 0x0E,
	_wwxx = 0x0F, _aarr = 0x0F,
	_xxyx = 0x10, _rrgr = 0x10,
	_yxyx = 0x11, _grgr = 0x11,
	_zxyx = 0x12, _brgr = 0x12,
	_wxyx = 0x13, _argr = 0x13,
	_xyyx = 0x14, _rggr = 0x14,
	_yyyx = 0x15, _gggr = 0x15,
	_zyyx = 0x16, _bggr = 0x16,
	_wyyx = 0x17, _aggr = 0x17,
	_xzyx = 0x18, _rbgr = 0x18,
	_yzyx = 0x19, _gbgr = 0x19,
	_zzyx = 0x1A, _bbgr = 0x1A,
	_wzyx = 0x1B, _abgr = 0x1B,
	_xwyx = 0x1C, _ragr = 0x1C,
	_ywyx = 0x1D, _gagr = 0x1D,
	_zwyx = 0x1E, _bagr = 0x1E,
	_wwyx = 0x1F, _aagr = 0x1F,
	_xxzx = 0x20, _rrbr = 0x20,
	_yxzx = 0x21, _grbr = 0x21,
	_zxzx = 0x22, _brbr = 0x22,
	_wxzx = 0x23, _arbr = 0x23,
	_xyzx = 0x24, _rgbr = 0x24,
	_yyzx = 0x25, _ggbr = 0x25,
	_zyzx = 0x26, _bgbr = 0x26,
	_wyzx = 0x27, _agbr = 0x27,
	_xzzx = 0x28, _rbbr = 0x28,
	_yzzx = 0x29, _gbbr = 0x29,
	_zzzx = 0x2A, _bbbr = 0x2A,
	_wzzx = 0x2B, _abbr = 0x2B,
	_xwzx = 0x2C, _rabr = 0x2C,
	_ywzx = 0x2D, _gabr = 0x2D,
	_zwzx = 0x2E, _babr = 0x2E,
	_wwzx = 0x2F, _aabr = 0x2F,
	_xxwx = 0x30, _rrar = 0x30,
	_yxwx = 0x31, _grar = 0x31,
	_zxwx = 0x32, _brar = 0x32,
	_wxwx = 0x33, _arar = 0x33,
	_xywx = 0x34, _rgar = 0x34,
	_yywx = 0x35, _ggar = 0x35,
	_zywx = 0x36, _bgar = 0x36,
	_wywx = 0x37, _agar = 0x37,
	_xzwx = 0x38, _rbar = 0x38,
	_yzwx = 0x39, _gbar = 0x39,
	_zzwx = 0x3A, _bbar = 0x3A,
	_wzwx = 0x3B, _abar = 0x3B,
	_xwwx = 0x3C, _raar = 0x3C,
	_ywwx = 0x3D, _gaar = 0x3D,
	_zwwx = 0x3E, _baar = 0x3E,
	_wwwx = 0x3F, _aaar = 0x3F,
	_xxxy = 0x40, _rrrg = 0x40,
	_yxxy = 0x41, _grrg = 0x41,
	_zxxy = 0x42, _brrg = 0x42,
	_wxxy = 0x43, _arrg = 0x43,
	_xyxy = 0x44, _rgrg = 0x44,
	_yyxy = 0x45, _ggrg = 0x45,
	_zyxy = 0x46, _bgrg = 0x46,
	_wyxy = 0x47, _agrg = 0x47,
	_xzxy = 0x48, _rbrg = 0x48,
	_yzxy = 0x49, _gbrg = 0x49,
	_zzxy = 0x4A, _bbrg = 0x4A,
	_wzxy = 0x4B, _abrg = 0x4B,
	_xwxy = 0x4C, _rarg = 0x4C,
	_ywxy = 0x4D, _garg = 0x4D,
	_zwxy = 0x4E, _barg = 0x4E,
	_wwxy = 0x4F, _aarg = 0x4F,
	_xxyy = 0x50, _rrgg = 0x50,
	_yxyy = 0x51, _grgg = 0x51,
	_zxyy = 0x52, _brgg = 0x52,
	_wxyy = 0x53, _argg = 0x53,
	_xyyy = 0x54, _rggg = 0x54,
	_yyyy = 0x55, _gggg = 0x55,
	_zyyy = 0x56, _bggg = 0x56,
	_wyyy = 0x57, _aggg = 0x57,
	_xzyy = 0x58, _rbgg = 0x58,
	_yzyy = 0x59, _gbgg = 0x59,
	_zzyy = 0x5A, _bbgg = 0x5A,
	_wzyy = 0x5B, _abgg = 0x5B,
	_xwyy = 0x5C, _ragg = 0x5C,
	_ywyy = 0x5D, _gagg = 0x5D,
	_zwyy = 0x5E, _bagg = 0x5E,
	_wwyy = 0x5F, _aagg = 0x5F,
	_xxzy = 0x60, _rrbg = 0x60,
	_yxzy = 0x61, _grbg = 0x61,
	_zxzy = 0x62, _brbg = 0x62,
	_wxzy = 0x63, _arbg = 0x63,
	_xyzy = 0x64, _rgbg = 0x64,
	_yyzy = 0x65, _ggbg = 0x65,
	_zyzy = 0x66, _bgbg = 0x66,
	_wyzy = 0x67, _agbg = 0x67,
	_xzzy = 0x68, _rbbg = 0x68,
	_yzzy = 0x69, _gbbg = 0x69,
	_zzzy = 0x6A, _bbbg = 0x6A,
	_wzzy = 0x6B, _abbg = 0x6B,
	_xwzy = 0x6C, _rabg = 0x6C,
	_ywzy = 0x6D, _gabg = 0x6D,
	_zwzy = 0x6E, _babg = 0x6E,
	_wwzy = 0x6F, _aabg = 0x6F,
	_xxwy = 0x70, _rrag = 0x70,
	_yxwy = 0x71, _grag = 0x71,
	_zxwy = 0x72, _brag = 0x72,
	_wxwy = 0x73, _arag = 0x73,
	_xywy = 0x74, _rgag = 0x74,
	_yywy = 0x75, _ggag = 0x75,
	_zywy = 0x76, _bgag = 0x76,
	_wywy = 0x77, _agag = 0x77,
	_xzwy = 0x78, _rbag = 0x78,
	_yzwy = 0x79, _gbag = 0x79,
	_zzwy = 0x7A, _bbag = 0x7A,
	_wzwy = 0x7B, _abag = 0x7B,
	_xwwy = 0x7C, _raag = 0x7C,
	_ywwy = 0x7D, _gaag = 0x7D,
	_zwwy = 0x7E, _baag = 0x7E,
	_wwwy = 0x7F, _aaag = 0x7F,
	_xxxz = 0x80, _rrrb = 0x80,
	_yxxz = 0x81, _grrb = 0x81,
	_zxxz = 0x82, _brrb = 0x82,
	_wxxz = 0x83, _arrb = 0x83,
	_xyxz = 0x84, _rgrb = 0x84,
	_yyxz = 0x85, _ggrb = 0x85,
	_zyxz = 0x86, _bgrb = 0x86,
	_wyxz = 0x87, _agrb = 0x87,
	_xzxz = 0x88, _rbrb = 0x88,
	_yzxz = 0x89, _gbrb = 0x89,
	_zzxz = 0x8A, _bbrb = 0x8A,
	_wzxz = 0x8B, _abrb = 0x8B,
	_xwxz = 0x8C, _rarb = 0x8C,
	_ywxz = 0x8D, _garb = 0x8D,
	_zwxz = 0x8E, _barb = 0x8E,
	_wwxz = 0x8F, _aarb = 0x8F,
	_xxyz = 0x90, _rrgb = 0x90,
	_yxyz = 0x91, _grgb = 0x91,
	_zxyz = 0x92, _brgb = 0x92,
	_wxyz = 0x93, _argb = 0x93,
	_xyyz = 0x94, _rggb = 0x94,
	_yyyz = 0x95, _gggb = 0x95,
	_zyyz = 0x96, _bggb = 0x96,
	_wyyz = 0x97, _aggb = 0x97,
	_xzyz = 0x98, _rbgb = 0x98,
	_yzyz = 0x99, _gbgb = 0x99,
	_zzyz = 0x9A, _bbgb = 0x9A,
	_wzyz = 0x9B, _abgb = 0x9B,
	_xwyz = 0x9C, _ragb = 0x9C,
	_ywyz = 0x9D, _gagb = 0x9D,
	_zwyz = 0x9E, _bagb = 0x9E,
	_wwyz = 0x9F, _aagb = 0x9F,
	_xxzz = 0xA0, _rrbb = 0xA0,
	_yxzz = 0xA1, _grbb = 0xA1,
	_zxzz = 0xA2, _brbb = 0xA2,
	_wxzz = 0xA3, _arbb = 0xA3,
	_xyzz = 0xA4, _rgbb = 0xA4,
	_yyzz = 0xA5, _ggbb = 0xA5,
	_zyzz = 0xA6, _bgbb = 0xA6,
	_wyzz = 0xA7, _agbb = 0xA7,
	_xzzz = 0xA8, _rbbb = 0xA8,
	_yzzz = 0xA9, _gbbb = 0xA9,
	_zzzz = 0xAA, _bbbb = 0xAA,
	_wzzz = 0xAB, _abbb = 0xAB,
	_xwzz = 0xAC, _rabb = 0xAC,
	_ywzz = 0xAD, _gabb = 0xAD,
	_zwzz = 0xAE, _babb = 0xAE,
	_wwzz = 0xAF, _aabb = 0xAF,
	_xxwz = 0xB0, _rrab = 0xB0,
	_yxwz = 0xB1, _grab = 0xB1,
	_zxwz = 0xB2, _brab = 0xB2,
	_wxwz = 0xB3, _arab = 0xB3,
	_xywz = 0xB4, _rgab = 0xB4,
	_yywz = 0xB5, _ggab = 0xB5,
	_zywz = 0xB6, _bgab = 0xB6,
	_wywz = 0xB7, _agab = 0xB7,
	_xzwz = 0xB8, _rbab = 0xB8,
	_yzwz = 0xB9, _gbab = 0xB9,
	_zzwz = 0xBA, _bbab = 0xBA,
	_wzwz = 0xBB, _abab = 0xBB,
	_xwwz = 0xBC, _raab = 0xBC,
	_ywwz = 0xBD, _gaab = 0xBD,
	_zwwz = 0xBE, _baab = 0xBE,
	_wwwz = 0xBF, _aaab = 0xBF,
	_xxxw = 0xC0, _rrra = 0xC0,
	_yxxw = 0xC1, _grra = 0xC1,
	_zxxw = 0xC2, _brra = 0xC2,
	_wxxw = 0xC3, _arra = 0xC3,
	_xyxw = 0xC4, _rgra = 0xC4,
	_yyxw = 0xC5, _ggra = 0xC5,
	_zyxw = 0xC6, _bgra = 0xC6,
	_wyxw = 0xC7, _agra = 0xC7,
	_xzxw = 0xC8, _rbra = 0xC8,
	_yzxw = 0xC9, _gbra = 0xC9,
	_zzxw = 0xCA, _bbra = 0xCA,
	_wzxw = 0xCB, _abra = 0xCB,
	_xwxw = 0xCC, _rara = 0xCC,
	_ywxw = 0xCD, _gara = 0xCD,
	_zwxw = 0xCE, _bara = 0xCE,
	_wwxw = 0xCF, _aara = 0xCF,
	_xxyw = 0xD0, _rrga = 0xD0,
	_yxyw = 0xD1, _grga = 0xD1,
	_zxyw = 0xD2, _brga = 0xD2,
	_wxyw = 0xD3, _arga = 0xD3,
	_xyyw = 0xD4, _rgga = 0xD4,
	_yyyw = 0xD5, _ggga = 0xD5,
	_zyyw = 0xD6, _bgga = 0xD6,
	_wyyw = 0xD7, _agga = 0xD7,
	_xzyw = 0xD8, _rbga = 0xD8,
	_yzyw = 0xD9, _gbga = 0xD9,
	_zzyw = 0xDA, _bbga = 0xDA,
	_wzyw = 0xDB, _abga = 0xDB,
	_xwyw = 0xDC, _raga = 0xDC,
	_ywyw = 0xDD, _gaga = 0xDD,
	_zwyw = 0xDE, _baga = 0xDE,
	_wwyw = 0xDF, _aaga = 0xDF,
	_xxzw = 0xE0, _rrba = 0xE0,
	_yxzw = 0xE1, _grba = 0xE1,
	_zxzw = 0xE2, _brba = 0xE2,
	_wxzw = 0xE3, _arba = 0xE3,
	_xyzw = 0xE4, _rgba = 0xE4,
	_yyzw = 0xE5, _ggba = 0xE5,
	_zyzw = 0xE6, _bgba = 0xE6,
	_wyzw = 0xE7, _agba = 0xE7,
	_xzzw = 0xE8, _rbba = 0xE8,
	_yzzw = 0xE9, _gbba = 0xE9,
	_zzzw = 0xEA, _bbba = 0xEA,
	_wzzw = 0xEB, _abba = 0xEB,
	_xwzw = 0xEC, _raba = 0xEC,
	_ywzw = 0xED, _gaba = 0xED,
	_zwzw = 0xEE, _baba = 0xEE,
	_wwzw = 0xEF, _aaba = 0xEF,
	_xxww = 0xF0, _rraa = 0xF0,
	_yxww = 0xF1, _graa = 0xF1,
	_zxww = 0xF2, _braa = 0xF2,
	_wxww = 0xF3, _araa = 0xF3,
	_xyww = 0xF4, _rgaa = 0xF4,
	_yyww = 0xF5, _ggaa = 0xF5,
	_zyww = 0xF6, _bgaa = 0xF6,
	_wyww = 0xF7, _agaa = 0xF7,
	_xzww = 0xF8, _rbaa = 0xF8,
	_yzww = 0xF9, _gbaa = 0xF9,
	_zzww = 0xFA, _bbaa = 0xFA,
	_wzww = 0xFB, _abaa = 0xFB,
	_xwww = 0xFC, _raaa = 0xFC,
	_ywww = 0xFD, _gaaa = 0xFD,
	_zwww = 0xFE, _baaa = 0xFE,
	_wwww = 0xFF, _aaaa = 0xFF,

	// Special swizzles:
	_NoSwizzleXYZW = _xyzw, _NoSwizzleRGBA = _rgba,
	_ReplicateX = _xxxx, _ReplicateRed = _rrrr,
	_ReplicateY = _yyyy, _ReplicateGreen = _gggg,
	_ReplicateZ = _zzzz, _ReplicateBlue = _bbbb,
	_ReplicateW = _wwww, _ReplicateAlpha = _aaaa
};
static_assert(_NoSwizzleXYZW == _NoSwizzleRGBA, "Error: Color and vector channels mismatch!");
static_assert(_ReplicateX == _ReplicateRed, "Error: Color and vector channels mismatch!");
static_assert(_ReplicateY == _ReplicateGreen, "Error: Color and vector channels mismatch!");
static_assert(_ReplicateZ == _ReplicateBlue, "Error: Color and vector channels mismatch!");
static_assert(_ReplicateW == _ReplicateAlpha, "Error: Color and vector channels mismatch!");
static_assert(_NoSwizzleXYZW == (D3DSP_NOSWIZZLE >> D3DSP_SWIZZLE_SHIFT), "Error: Enum mismatches header definition!");
static_assert(_ReplicateRed == (D3DSP_REPLICATERED >> D3DSP_SWIZZLE_SHIFT), "Error: Enum mismatches header definition!");
static_assert(_ReplicateGreen == (D3DSP_REPLICATEGREEN >> D3DSP_SWIZZLE_SHIFT), "Error: Enum mismatches header definition!");
static_assert(_ReplicateBlue == (D3DSP_REPLICATEBLUE >> D3DSP_SWIZZLE_SHIFT), "Error: Enum mismatches header definition!");
static_assert(_ReplicateAlpha == (D3DSP_REPLICATEALPHA >> D3DSP_SWIZZLE_SHIFT), "Error: Enum mismatches header definition!");

enum writeMaskType : unsigned char
{
	WM_None = 0x0,
	WM_X = 0x1,
	WM_Y = 0x2,
	WM_XY = 0x3,
	WM_Z = 0x4,
	WM_XZ = 0x5,
	WM_YZ = 0x6,
	WM_XYZ = 0x7,
	WM_W = 0x8,
	WM_XW = 0x9,
	WM_YW = 0xA,
	WM_XYW = 0xB,
	WM_ZW = 0xC,
	WM_XZW = 0xD,
	WM_YZW = 0xE,
	WM_XYZW = 0xF,

	WM_ALL = WM_XYZW
};

enum resultModifierType : unsigned char
{
	RM_None = (D3DSPDM_NONE >> D3DSP_DSTMOD_SHIFT), // nop
	RM_Saturate = (D3DSPDM_SATURATE >> D3DSP_DSTMOD_SHIFT), // clamp to 0. to 1. range
	RM_PartialPrecision = (D3DSPDM_PARTIALPRECISION >> D3DSP_DSTMOD_SHIFT), // Partial precision hint
	RM_CentroidSample = (D3DSPDM_MSAMPCENTROID >> D3DSP_DSTMOD_SHIFT) /* Relevant to multisampling only:
																	  When the pixel center is not covered, sample
																	  attribute or compute gradients/LOD
																	  using multisample "centroid" location.
																	  "Centroid" is some location within the covered
																	  region of the pixel. */
};

enum debuggableSourceModifierType : unsigned char
{
	SM_None = (D3DSPSM_NONE >> D3DSP_SRCMOD_SHIFT), // nop
	SM_Negate = (D3DSPSM_NEG >> D3DSP_SRCMOD_SHIFT), // negate
    SM_Bias = (D3DSPSM_BIAS >> D3DSP_SRCMOD_SHIFT), // bias
    SM_BiasNegate = (D3DSPSM_BIASNEG >> D3DSP_SRCMOD_SHIFT), // bias and negate
    SM_Sign = (D3DSPSM_SIGN >> D3DSP_SRCMOD_SHIFT), // sign
    SM_SignNegate = (D3DSPSM_SIGNNEG >> D3DSP_SRCMOD_SHIFT), // sign and negate
    SM_Complement = (D3DSPSM_COMP >> D3DSP_SRCMOD_SHIFT), // complement
    SM_x2 = (D3DSPSM_X2 >> D3DSP_SRCMOD_SHIFT), // *2
    SM_x2Negate = (D3DSPSM_X2NEG >> D3DSP_SRCMOD_SHIFT), // *2 and negate
    SM_DivZ = (D3DSPSM_DZ >> D3DSP_SRCMOD_SHIFT), // divide through by z component
    SM_DivW = (D3DSPSM_DW >> D3DSP_SRCMOD_SHIFT), // divide through by w component
    SM_Abs = (D3DSPSM_ABS >> D3DSP_SRCMOD_SHIFT), // abs()
    SM_AbsNegate = (D3DSPSM_ABSNEG >> D3DSP_SRCMOD_SHIFT), // -abs()
    SM_Not = (D3DSPSM_NOT >> D3DSP_SRCMOD_SHIFT), // for predicate register: "!p0"
};

#define D3DFVF_TEXTUREFORMAT2U 0u         // Two floating point values
#define D3DFVF_TEXTUREFORMAT1U 3u         // One floating point value
#define D3DFVF_TEXTUREFORMAT3U 1u         // Three floating point values
#define D3DFVF_TEXTUREFORMAT4U 2u         // Four floating point values

// Using U for Unsigned here, otherwise the compiler chokes on D3DFVF_TEXCOORDSIZE4(7) and D3DFVF_TEXCOORDSIZE1(7) as the sign bit gets accessed and then the number can't be converted to a DWORD properly
#define D3DFVF_TEXCOORDSIZE3U(CoordIndex) (D3DFVF_TEXTUREFORMAT3U << (CoordIndex * 2u + 16u))
#define D3DFVF_TEXCOORDSIZE4U(CoordIndex) (D3DFVF_TEXTUREFORMAT4U << (CoordIndex * 2u + 16u))
#define D3DFVF_TEXCOORDSIZE1U(CoordIndex) (D3DFVF_TEXTUREFORMAT1U << (CoordIndex * 2u + 16u))

enum debuggableFVF_textureFormat : unsigned char
{
	textureFormatFloat2 = D3DFVF_TEXTUREFORMAT2U, // float2 (default)
	textureFormatFloat1 = D3DFVF_TEXTUREFORMAT1U, // float
	textureFormatFloat3 = D3DFVF_TEXTUREFORMAT3U, // float3
	textureFormatFloat4 = D3DFVF_TEXTUREFORMAT4U // float4
};

enum debuggableFVF_positionTypeLow : unsigned char
{
	dbgD3DFVF_NoPosition = 0x000, // This is most commonly used when this FVF is all 0x00000000, like when initializing a non-FVF vertex buffer intended for use with a vertex declaration instead
	dbgD3DFVF_RESERVED0 = D3DFVF_RESERVED0, // 0x001 // The RESERVED0 bit seems to be completely ignored by the FVF to Decl converter
	dbgD3DFVF_XYZ = D3DFVF_XYZ, // 0x002
	dbgD3DFVF_XYZRHW = D3DFVF_XYZRHW, // 0x004
	dbgD3DFVF_XYZB1 = D3DFVF_XYZB1, // 0x006
	dbgD3DFVF_XYZB2 = D3DFVF_XYZB2, // 0x008
	dbgD3DFVF_XYZB3 = D3DFVF_XYZB3, // 0x00a
	dbgD3DFVF_XYZB4 = D3DFVF_XYZB4, // 0x00c
	dbgD3DFVF_XYZB5 = D3DFVF_XYZB5 // 0x00e
};

// Some of these entries are commented-out. This makes me sad, but Visual Studio's debug visualizer will only properly display bitfield enums down if all of the enum's entries are power-of-2.
// Fortunately the majority of common FVF codes are comprised only out of power-of-2 FVF bits, but some rarer ones will not be displayed properly in the debugger...
enum debuggableFVF_enum : DWORD
{
	dD3DFVF_RESERVED0 = D3DFVF_RESERVED0, // 0x001
	dD3DFVF_XYZ = D3DFVF_XYZ, // 0x002
	dD3DFVF_XYZRHW = D3DFVF_XYZRHW, // 0x004
	//dD3DFVF_XYZB1 = D3DFVF_XYZB1, // 0x006
	dD3DFVF_XYZB2 = D3DFVF_XYZB2, // 0x008
	/*dD3DFVF_XYZB3 = D3DFVF_XYZB3, // 0x00a
	dD3DFVF_XYZB4 = D3DFVF_XYZB4, // 0x00c
	dD3DFVF_XYZB5 = D3DFVF_XYZB5, // 0x00e
	dD3DFVF_XYZW = D3DFVF_XYZW, // 0x4002*/
	
	dD3DFVF_NORMAL = D3DFVF_NORMAL, // 0x010
	dD3DFVF_PSIZE = D3DFVF_PSIZE, // 0x020
	dD3DFVF_DIFFUSE = D3DFVF_DIFFUSE, // 0x040
	dD3DFVF_SPECULAR = D3DFVF_SPECULAR, // 0x080
	
	dD3DFVF_TEX1 = D3DFVF_TEX1, // 0x100
	dD3DFVF_TEX2 = D3DFVF_TEX2, // 0x200
	//dD3DFVF_TEX3 = D3DFVF_TEX3, // 0x300
	dD3DFVF_TEX4 = D3DFVF_TEX4, // 0x400
	/*dD3DFVF_TEX5 = D3DFVF_TEX5, // 0x500
	dD3DFVF_TEX6 = D3DFVF_TEX6, // 0x600
	dD3DFVF_TEX7 = D3DFVF_TEX7, // 0x700*/
	dD3DFVF_TEX8 = D3DFVF_TEX8, // 0x800
	
	dD3DFVF_LASTBETA_UBYTE4 = D3DFVF_LASTBETA_UBYTE4, // 0x1000
	dD3DFVF_LASTBETA_D3DCOLOR = D3DFVF_LASTBETA_D3DCOLOR, // 0x8000
	
	//dD3DFVF_RESERVED2 = D3DFVF_RESERVED2, // 0x6000

	dTEXCOORDSIZE3_TEX1 = D3DFVF_TEXCOORDSIZE3U(0),
	dTEXCOORDSIZE4_TEX1 = D3DFVF_TEXCOORDSIZE4U(0),
	//dTEXCOORDSIZE1_TEX1 = D3DFVF_TEXCOORDSIZE1U(0),

	dTEXCOORDSIZE3_TEX2 = D3DFVF_TEXCOORDSIZE3U(1),
	dTEXCOORDSIZE4_TEX2 = D3DFVF_TEXCOORDSIZE4U(1),
	//dTEXCOORDSIZE1_TEX2 = D3DFVF_TEXCOORDSIZE1U(1),

	dTEXCOORDSIZE3_TEX3 = D3DFVF_TEXCOORDSIZE3U(2),
	dTEXCOORDSIZE4_TEX3 = D3DFVF_TEXCOORDSIZE4U(2),
	//dTEXCOORDSIZE1_TEX3 = D3DFVF_TEXCOORDSIZE1U(2),

	dTEXCOORDSIZE3_TEX4 = D3DFVF_TEXCOORDSIZE3U(3),
	dTEXCOORDSIZE4_TEX4 = D3DFVF_TEXCOORDSIZE4U(3),
	//dTEXCOORDSIZE1_TEX4 = D3DFVF_TEXCOORDSIZE1U(3),

	dTEXCOORDSIZE3_TEX5 = D3DFVF_TEXCOORDSIZE3U(4),
	dTEXCOORDSIZE4_TEX5 = D3DFVF_TEXCOORDSIZE4U(4),
	//dTEXCOORDSIZE1_TEX5 = D3DFVF_TEXCOORDSIZE1U(4),

	dTEXCOORDSIZE3_TEX6 = D3DFVF_TEXCOORDSIZE3U(5),
	dTEXCOORDSIZE4_TEX6 = D3DFVF_TEXCOORDSIZE4U(5),
	//dTEXCOORDSIZE1_TEX6 = D3DFVF_TEXCOORDSIZE1U(5),

	dTEXCOORDSIZE3_TEX7 = D3DFVF_TEXCOORDSIZE3U(6),
	dTEXCOORDSIZE4_TEX7 = D3DFVF_TEXCOORDSIZE4U(6),
	//dTEXCOORDSIZE1_TEX7 = D3DFVF_TEXCOORDSIZE1U(6),

	dTEXCOORDSIZE3_TEX8 = D3DFVF_TEXCOORDSIZE3U(7),
	dTEXCOORDSIZE4_TEX8 = D3DFVF_TEXCOORDSIZE4U(7),
	//dTEXCOORDSIZE1_TEX8 = D3DFVF_TEXCOORDSIZE1U(7)
};

union debuggableFVF
{
	struct _namedFVF
	{
		debuggableFVF_positionTypeLow positionTypeLow : 4; // This is the XYZ + matrix palette blending
		bool hasNormal : 1; // D3DFVF_NORMAL // This is the NORMAL0 semantic (float3) // 0x010
		bool hasPSize : 1; // D3DFVF_PSIZE // This is the PSIZE0 semantic (float) // 0x020
		bool hasDiffuse : 1; // D3DFVF_DIFFUSE // This is the COLOR0 semantic (D3DCOLOR) // 0x040
		bool hasSpecular : 1; // D3DFVF_SPECULAR // This is the COLOR1 semantic (D3DCOLOR) // 0x080
		unsigned char numTexcoordElementsPresent : 4; // Valid values for this are from 0 (meaning no texcoords present) thru 8 (meaning 8 sets of texcoords present)
		bool lastBeta_UBYTE4 : 1; // D3DFVF_LASTBETA_UBYTE4 // 0x1000
		bool hasFog : 1; // This is the underdocumented 0x2000 flag "D3DFVF_FOG". It is a float1 channel on stream 0 with usage 0 that implies D3DDECLUSAGE_FOG. It is placed between the lighting attributes and the texcoords in the vertex struct. // RESERVED2 is 0x6000, which is split between "has fog" and D3DFVF_XYZW // 0x2000
		bool positionTypeContainsUntransformedW : 1; // D3DFVF_XYZW // 0x4002
		bool lastBeta_D3DCOLOR : 1; // D3DFVF_LASTBETA_D3DCOLOR // 0x8000
		debuggableFVF_textureFormat texCoord0type : 2; // TEX1 type
		debuggableFVF_textureFormat texCoord1type : 2; // TEX2 type
		debuggableFVF_textureFormat texCoord2type : 2; // TEX3 type
		debuggableFVF_textureFormat texCoord3type : 2; // TEX4 type
		debuggableFVF_textureFormat texCoord4type : 2; // TEX5 type
		debuggableFVF_textureFormat texCoord5type : 2; // TEX6 type
		debuggableFVF_textureFormat texCoord6type : 2; // TEX7 type
		debuggableFVF_textureFormat texCoord7type : 2; // TEX8 type
	} namedFVF;

	debuggableFVF_enum typedFVF;

	DWORD rawFVF_DWORD;
};
static_assert(sizeof(debuggableFVF) == sizeof(DWORD), "Error! Unexpected struct size!");
