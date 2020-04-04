#pragma once

#include "IDirect3DDevice9Hook.h"

enum StateBlockSetCallType : unsigned char
{
	SBT_SetFVF = 0,
	SBT_SetIndices,
	SBT_SetMaterial,
	SBT_SetNPatchMode,
	SBT_SetPixelShader,
	SBT_SetScissorRect,
	SBT_SetViewport,
	SBT_SetVertexDeclaration,
	SBT_SetVertexShader,
	SBT_SetCurrentTexturePalette,

	SBT_MAX
};

struct capturedStateBitmask
{
	capturedStateBitmask()
	{
		memset(this, 0, sizeof(*this) );
	}

	~capturedStateBitmask()
	{
		delete capturedLights;
		capturedLights = NULL;
	}

	// TODO: All of the other captureable device state (textures, vertex decls, VB's, IB's, texture stage states, etc.)
	struct _capturedRenderstates
	{
		unsigned captureRenderstate[0x100 / 32];
	} capturedRenderstates;

	union _capturedUserClipPlanes
	{
		DWORD userClipPlanesBitmask;

		struct
		{
			BOOL userClipPlane0 : 1; BOOL userClipPlane1 : 1; BOOL userClipPlane2 : 1; BOOL userClipPlane3 : 1;
			BOOL userClipPlane4 : 1; BOOL userClipPlane5 : 1; BOOL userClipPlane6 : 1; BOOL userClipPlane7 : 1;
			BOOL userClipPlane8 : 1; BOOL userClipPlane9 : 1; BOOL userClipPlane10 : 1; BOOL userClipPlane11 : 1;
			BOOL userClipPlane12 : 1; BOOL userClipPlane13 : 1; BOOL userClipPlane14 : 1; BOOL userClipPlane15 : 1;
			BOOL userClipPlane16 : 1; BOOL userClipPlane17 : 1; BOOL userClipPlane18 : 1; BOOL userClipPlane19 : 1;
			BOOL userClipPlane20 : 1; BOOL userClipPlane21 : 1; BOOL userClipPlane22 : 1; BOOL userClipPlane23 : 1;
			BOOL userClipPlane24 : 1; BOOL userClipPlane25 : 1; BOOL userClipPlane26 : 1; BOOL userClipPlane27 : 1;
			BOOL userClipPlane28 : 1; BOOL userClipPlane29 : 1; BOOL userClipPlane30 : 1; BOOL userClipPlane31 : 1;
		} userClipPlanesNamed;
	} capturedUserClipPlanes;
	static_assert(sizeof(capturedUserClipPlanes) == sizeof(DWORD), "Error: Unexpected union size!");

	union _capturedStreamSources
	{
		unsigned short streamSourcesBitmask;
		struct
		{
			unsigned short streamSource0 : 1; unsigned short streamSource1 : 1; unsigned short streamSource2 : 1; unsigned short streamSource3 : 1;
			unsigned short streamSource4 : 1; unsigned short streamSource5 : 1; unsigned short streamSource6 : 1; unsigned short streamSource7 : 1;
			unsigned short streamSource8 : 1; unsigned short streamSource9 : 1; unsigned short streamSource10 : 1; unsigned short streamSource11 : 1;
			unsigned short streamSource12 : 1; unsigned short streamSource13 : 1; unsigned short streamSource14 : 1; unsigned short streamSource15 : 1;
		} streamSourcesNamed;
	} capturedStreamSources;
	static_assert(sizeof(capturedStreamSources) == sizeof(unsigned short), "Error: Unexpected union size!");

	union _capturedStreamSourceFreq
	{
		unsigned short streamSourceFreqBitmask;
		struct
		{
			unsigned short streamSourceFreq0 : 1; unsigned short streamSourceFreq1 : 1; unsigned short streamSourceFreq2 : 1; unsigned short streamSourceFreq3 : 1;
			unsigned short streamSourceFreq4 : 1; unsigned short streamSourceFreq5 : 1; unsigned short streamSourceFreq6 : 1; unsigned short streamSourceFreq7 : 1;
			unsigned short streamSourceFreq8 : 1; unsigned short streamSourceFreq9 : 1; unsigned short streamSourceFreq10 : 1; unsigned short streamSourceFreq11 : 1;
			unsigned short streamSourceFreq12 : 1; unsigned short streamSourceFreq13 : 1; unsigned short streamSourceFreq14 : 1; unsigned short streamSourceFreq15 : 1;
		} streamSourceFreqsNamed;
	} capturedStreamSourceFreq;
	static_assert(sizeof(capturedStreamSourceFreq) == sizeof(unsigned short), "Error: Unexpected union size!");

	struct _capturedTransforms
	{
		bool viewCaptured;
		bool projectionCaptured;

		union 
		{
			unsigned char textureTransformsCapturedBitmask;

			struct
			{
				unsigned char textureTransform0 : 1; unsigned char textureTransform1 : 1; unsigned char textureTransform2 : 1; unsigned char textureTransform3 : 1;
				unsigned char textureTransform4 : 1; unsigned char textureTransform5 : 1; unsigned char textureTransform6 : 1; unsigned char textureTransform7 : 1;
			} textureTransformsCapturedNamed;
		} textureTransformsCaptured;
		static_assert(sizeof(textureTransformsCaptured) == sizeof(unsigned char), "Error: Unexpected union size!");

		unsigned captureWorldTransforms[MAX_WORLD_TRANSFORMS / 32];

	} capturedTransforms;

	struct capturedShaderConstants
	{
		unsigned long floatsCaptured[4096 / 32];

		union nonFloatCapturedBits
		{
			struct
			{
				unsigned short constant0 : 1; unsigned short constant1 : 1; unsigned short constant2 : 1; unsigned short constant3 : 1;
				unsigned short constant4 : 1; unsigned short constant5 : 1; unsigned short constant6 : 1; unsigned short constant7 : 1;
				unsigned short constant8 : 1; unsigned short constant9 : 1; unsigned short constant10 : 1; unsigned short constant11 : 1;
				unsigned short constant12 : 1; unsigned short constant13 : 1; unsigned short constant14 : 1; unsigned short constant15 : 1;
			} capturedConstantsNamed;

			unsigned short capturedConstantsBitmask;
		};
		static_assert(sizeof(nonFloatCapturedBits) == sizeof(unsigned short), "Error: Unexpected union size!");

		nonFloatCapturedBits intCaptured;
		nonFloatCapturedBits boolCaptured;

		inline void MarkSetShaderConstantF(const UINT constantIndex)
		{
			if (constantIndex >= 4096)
				return;

			const unsigned constantSetIndex = constantIndex / 32;
			const unsigned constantSetBitmask = (1 << (constantIndex % 32) );
			floatsCaptured[constantSetIndex] |= constantSetBitmask;
		}

		inline void MarkSetShaderConstantNonF(const UINT constantIndex, nonFloatCapturedBits& nonFCapture)
		{
			if (constantIndex >= 16)
				return;

			const unsigned short constantSetBitmask = (1 << constantIndex);
			nonFCapture.capturedConstantsBitmask |= constantSetBitmask;
		}

		inline void MarkSetShaderConstantI(const UINT constantIndex)
		{
			MarkSetShaderConstantNonF(constantIndex, intCaptured);
		}

		inline void MarkSetShaderConstantB(const UINT constantIndex)
		{
			MarkSetShaderConstantNonF(constantIndex, boolCaptured);
		}

		inline void MarkSetAllShaderConstantsCaptured()
		{
			for (unsigned x = 0; x < ARRAYSIZE(floatsCaptured); ++x)
				floatsCaptured[x] = 0xFFFFFFFF;

			intCaptured.capturedConstantsBitmask = 0xFFFF;
			boolCaptured.capturedConstantsBitmask = 0xFFFF;
		}
	};
	capturedShaderConstants capturedPixelShaderConstants;
	capturedShaderConstants capturedVertexShaderConstants;

	struct _capturedTextures
	{
		unsigned captureTextures[(MAX_NUM_SAMPLERS + 31) / 32];
	} capturedTextures;

	struct _capturedTextureStageStates
	{
		union _stageStateBits
		{
			struct
			{
				unsigned colorOp : 1;				// D3DTSS_COLOROP                = bit 0
				unsigned colorArg1 : 1;				// D3DTSS_COLORARG1              = bit 1
				unsigned colorArg2 : 1;				// D3DTSS_COLORARG2              = bit 2
				unsigned alphaOp : 1;				// D3DTSS_ALPHAOP                = bit 3
				unsigned alphaArg1 : 1;				// D3DTSS_ALPHAARG1              = bit 4
				unsigned alphaArg2 : 1;				// D3DTSS_ALPHAARG2              = bit 5
				unsigned bumpEnvMat00 : 1;			// D3DTSS_BUMPENVMAT00           = bit 6
				unsigned bumpEnvMat01 : 1;			// D3DTSS_BUMPENVMAT01           = bit 7
				unsigned bumpEnvMat10 : 1;			// D3DTSS_BUMPENVMAT10           = bit 8
				unsigned bumpEnvMat11 : 1;			// D3DTSS_BUMPENVMAT11           = bit 9
				unsigned texCoordIndex : 1;			// D3DTSS_TEXCOORDINDEX          = bit 10
				unsigned unused0 : 10;
				unsigned bumpEnvLScale : 1;			// D3DTSS_BUMPENVLSCALE          = bit 21
				unsigned bumpEnvLOffset : 1;		// D3DTSS_BUMPENVLOFFSET         = bit 22
				unsigned textureTransformFlags : 1;	// D3DTSS_TEXTURETRANSFORMFLAGS  = bit 23
				unsigned unused1 : 1;
				unsigned colorArg0 : 1;				// D3DTSS_COLORARG0              = bit 25
				unsigned alphaArg0 : 1;				// D3DTSS_ALPHAARG0              = bit 26
				unsigned resultArg : 1;				// D3DTSS_RESULTARG              = bit 27
				unsigned unused2 : 3;
				unsigned constant : 1;				// D3DTSS_CONSTANT               = bit 31
			} stageStateNamed;

			unsigned stageStateBitfields;
		};
		static_assert(sizeof(_stageStateBits) == sizeof(unsigned), "Error: Unexpected union size!");

		_stageStateBits capturedTextureStages[MAX_NUM_TEXTURE_STAGE_STATES];
	} capturedTextureStageStates;

	struct _capturedSamplerStates
	{
		union _samplerStateUnion
		{
			struct
			{
				unsigned short unused0		  : 1;//= 0
				unsigned short ADDRESSU       : 1;//= 1   D3DTEXTUREADDRESS for U coordinate
				unsigned short ADDRESSV       : 1;//= 2   D3DTEXTUREADDRESS for V coordinate
				unsigned short ADDRESSW       : 1;//= 3   D3DTEXTUREADDRESS for W coordinate
				unsigned short BORDERCOLOR    : 1;//= 4   D3DCOLOR
				unsigned short MAGFILTER      : 1;//= 5   D3DTEXTUREFILTER filter to use for magnification
				unsigned short MINFILTER      : 1;//= 6   D3DTEXTUREFILTER filter to use for minification
				unsigned short MIPFILTER      : 1;//= 7   D3DTEXTUREFILTER filter to use between mipmaps during minification
				unsigned short MIPMAPLODBIAS  : 1;//= 8   float Mipmap LOD bias
				unsigned short MAXMIPLEVEL    : 1;//= 9   DWORD 0..(n-1) LOD index of largest map to use (0 == largest)
				unsigned short MAXANISOTROPY  : 1;//= 10  DWORD maximum anisotropy
				unsigned short SRGBTEXTURE    : 1;//= 11  Default = 0 (which means Gamma 1.0, no correction required.) else correct for Gamma = 2.2
				unsigned short ELEMENTINDEX   : 1;//= 12  When multi-element texture is assigned to sampler, this indicates which element index to use.  Default = 0.
				unsigned short DMAPOFFSET     : 1;//= 13  Offset in vertices in the pre-sampled displacement map. Only valid for D3DDMAPSAMPLER sampler
			} samplerStateNamed;

			unsigned short samplerStateBitmask;
		};

		_samplerStateUnion samplerStates[MAX_NUM_SAMPLERS];
	} capturedSamplerStates;

	struct lightCaptureStruct
	{
		lightCaptureStruct() : captureLightEnable(false), lightEnable(false), captureSetLight(false)
		{
		}

		bool captureLightEnable;
		bool lightEnable;
		bool captureSetLight;
	};
	std::map<UINT, lightCaptureStruct>* capturedLights;

	bool singleCallStatesCaptured[SBT_MAX];
};

class IDirect3DStateBlock9Hook : public IDirect3DStateBlock9
{
public:
	IDirect3DStateBlock9Hook(LPDIRECT3DSTATEBLOCK9 _realObject, IDirect3DDevice9Hook* _parentDevice, const bool _isCompleteStateBlock) : realObject(_realObject), parentDevice(_parentDevice), refCount(1), internalStateBlockType( (const D3DSTATEBLOCKTYPE)0), isCompleteStateBlock(_isCompleteStateBlock)
	{
#ifdef _DEBUG
		if (realObject)
			CreationCallStack = realObject->CreationCallStack;
#endif
	}

	virtual ~IDirect3DStateBlock9Hook()
	{
#ifdef WIPE_ON_DESTRUCT_D3DHOOKOBJECT
		memset(this, 0x00000000, sizeof(*this) );
#endif
	}

	/*** IUnknown methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE QueryInterface(THIS_ REFIID riid, void** ppvObj) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE AddRef(THIS) override;
    virtual COM_DECLSPEC_NOTHROW ULONG STDMETHODCALLTYPE Release(THIS) override;

    /*** IDirect3DStateBlock9 methods ***/
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE GetDevice(THIS_ IDirect3DDevice9** ppDevice) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Capture(THIS) override;
    virtual COM_DECLSPEC_NOTHROW HRESULT STDMETHODCALLTYPE Apply(THIS) override;

	/*** IDirect3DStateBlock9Hook methods ***/
	inline void SetRealObject(LPDIRECT3DSTATEBLOCK9 _realObject)
	{
#ifdef _DEBUG
		if (_realObject == NULL)
		{
			// SetRealObject() should never be used to NULL out the underlying state block object
			__debugbreak();
		}
		if (realObject != NULL)
		{
			// SetRealObject() should never be used to change an existing state block's underlying real state block
			__debugbreak();
		}
#endif
		realObject = _realObject;
#ifdef _DEBUG
		if (realObject)
			CreationCallStack = realObject->CreationCallStack;
#endif
	}

	inline DeviceState* const GetDeviceStateForWrite()
	{
		return &stateBlockState;
	}

	inline void MarkRenderStateAsCaptured(const D3DRENDERSTATETYPE renderState)
	{
#ifdef _DEBUG
		if (renderState > 255)
		{
			__debugbreak(); // Should never get here
		}
#endif
		const unsigned dwordIndex = renderState / 32;
		const unsigned bitMask = 1 << (renderState % 32);
		capturedStates.capturedRenderstates.captureRenderstate[dwordIndex] |= bitMask;
	}

	template <const StateBlockSetCallType callType>
	inline void MarkSetCallAsCaptured()
	{
		capturedStates.singleCallStatesCaptured[callType] = true;
	}

	inline void MarkSetClipPlaneCaptured(const DWORD clipPlaneIndex)
	{
		if (clipPlaneIndex < D3DMAXUSERCLIPPLANES)
			capturedStates.capturedUserClipPlanes.userClipPlanesBitmask |= (1 << clipPlaneIndex);
	}

	inline void MarkSetStreamSourceCaptured(const UINT streamNumber)
	{
		if (streamNumber < MAX_D3D9_STREAMS)
			capturedStates.capturedStreamSources.streamSourcesBitmask |= (1 << streamNumber);
	}

	inline void MarkSetStreamSourceFreqCaptured(const UINT streamNumber)
	{
		if (streamNumber < MAX_D3D9_STREAMS)
			capturedStates.capturedStreamSourceFreq.streamSourceFreqBitmask |= (1 << streamNumber);
	}

	inline void MarkSetTransformCaptured(const D3DTRANSFORMSTATETYPE Transform)
	{
		if (Transform < D3DTS_WORLD)
		{
			switch (Transform)
			{
			case D3DTS_VIEW:
				capturedStates.capturedTransforms.viewCaptured = true;
				break;
			case D3DTS_PROJECTION:
				capturedStates.capturedTransforms.projectionCaptured = true;
				break;
			case D3DTS_TEXTURE0:
			case D3DTS_TEXTURE1:
			case D3DTS_TEXTURE2:
			case D3DTS_TEXTURE3:
			case D3DTS_TEXTURE4:
			case D3DTS_TEXTURE5:
			case D3DTS_TEXTURE6:
			case D3DTS_TEXTURE7:
			{
				const unsigned char setBitMask = (1 << (Transform - D3DTS_TEXTURE0) );
				capturedStates.capturedTransforms.textureTransformsCaptured.textureTransformsCapturedBitmask |= setBitMask;
			}
				break;
			default:
				// Should never be here, but do nothing in this case
				break;
			}
		}
		else if (Transform < D3DTS_WORLDMATRIX(MAX_WORLD_TRANSFORMS) ) // World transforms
		{
			const unsigned worldTransformIndex = Transform - D3DTS_WORLD;
			const unsigned dwordIndex = worldTransformIndex / 32;
			const unsigned bitMask = 1 << (worldTransformIndex % 32);
			capturedStates.capturedTransforms.captureWorldTransforms[dwordIndex] |= bitMask;
		}
		else
		{
			// Should never be here, but do nothing in this case
#ifdef _DEBUG
			__debugbreak();
#endif
		}
	}

	inline void MarkSetPixelShaderConstantF(const UINT constantIndex)
	{
		capturedStates.capturedPixelShaderConstants.MarkSetShaderConstantF(constantIndex);
	}

	inline void MarkSetPixelShaderConstantI(const UINT constantIndex)
	{
		capturedStates.capturedPixelShaderConstants.MarkSetShaderConstantI(constantIndex);
	}

	inline void MarkSetPixelShaderConstantB(const UINT constantIndex)
	{
		capturedStates.capturedPixelShaderConstants.MarkSetShaderConstantB(constantIndex);
	}

	inline void MarkSetVertexShaderConstantF(const UINT constantIndex)
	{
		capturedStates.capturedVertexShaderConstants.MarkSetShaderConstantF(constantIndex);
	}

	inline void MarkSetVertexShaderConstantI(const UINT constantIndex)
	{
		capturedStates.capturedVertexShaderConstants.MarkSetShaderConstantI(constantIndex);
	}

	inline void MarkSetVertexShaderConstantB(const UINT constantIndex)
	{
		capturedStates.capturedVertexShaderConstants.MarkSetShaderConstantB(constantIndex);
	}

	inline void MarkSetTextureCaptured(const UINT textureIndex)
	{
#ifdef _DEBUG
		if (textureIndex > MAX_NUM_SAMPLERS)
		{
			__debugbreak(); // Should never get here
		}
#endif
		const unsigned dwordIndex = textureIndex / 32;
		const unsigned bitMask = 1 << (textureIndex % 32);
		capturedStates.capturedTextures.captureTextures[dwordIndex] |= bitMask;
	}

	inline void MarkSetTextureStageStateCaptured(const UINT textureStageNum, const D3DTEXTURESTAGESTATETYPE stateType)
	{
		if (textureStageNum >= D3DDP_MAXTEXCOORD)
		{
#ifdef _DEBUG
			__debugbreak(); // Should never be here
#endif
			return;
		}

		if (stateType < D3DTSS_COLOROP || stateType > D3DTSS_CONSTANT)
		{
#ifdef _DEBUG
			__debugbreak(); // Should never be here
#endif
			return;
		}

		const unsigned stateIndex = stateType - 1;
		const unsigned stateBitmask = 1 << stateIndex;
		capturedStates.capturedTextureStageStates.capturedTextureStages[textureStageNum].stageStateBitfields |= stateBitmask;
	}

	inline void MarkSetSamplerStateCaptured(const UINT SamplerNum, const D3DSAMPLERSTATETYPE type)
	{
		if (SamplerNum >= MAX_NUM_SAMPLERS)
		{
#ifdef _DEBUG
			__debugbreak(); // Should never be here
#endif
			return;
		}

		if (type < D3DSAMP_ADDRESSU || type > D3DSAMP_DMAPOFFSET)
		{
#ifdef _DEBUG
			__debugbreak(); // Should never be here
#endif
			return;
		}

		const unsigned short samplerTypeBitmask = 1 << type;
		capturedStates.capturedSamplerStates.samplerStates[SamplerNum].samplerStateBitmask |= samplerTypeBitmask;
	}

	inline void MarkLightEnableCaptured(const UINT lightNum, const bool doEnable)
	{
		if (capturedStates.capturedLights == NULL)
			capturedStates.capturedLights = new std::map<UINT, capturedStateBitmask::lightCaptureStruct>;

		std::map<UINT, capturedStateBitmask::lightCaptureStruct>::iterator findExistingLightIt = capturedStates.capturedLights->find(lightNum);
		if (findExistingLightIt == capturedStates.capturedLights->end() )
		{
			capturedStateBitmask::lightCaptureStruct newLightCapture;
			newLightCapture.captureLightEnable = true;
			newLightCapture.lightEnable = doEnable;

			capturedStates.capturedLights->insert(std::make_pair(lightNum, newLightCapture) );
		}
		else
		{
			capturedStateBitmask::lightCaptureStruct& foundLightCapture = findExistingLightIt->second;
			foundLightCapture.captureLightEnable = true;
			foundLightCapture.lightEnable = doEnable;
		}
	}

	inline void MarkSetLightCaptured(const UINT lightNum)
	{
		if (capturedStates.capturedLights == NULL)
			capturedStates.capturedLights = new std::map<UINT, capturedStateBitmask::lightCaptureStruct>;

		std::map<UINT, capturedStateBitmask::lightCaptureStruct>::iterator findExistingLightIt = capturedStates.capturedLights->find(lightNum);
		if (findExistingLightIt == capturedStates.capturedLights->end() )
		{
			capturedStateBitmask::lightCaptureStruct newLightCapture;
			newLightCapture.captureSetLight = true;

			capturedStates.capturedLights->insert(std::make_pair(lightNum, newLightCapture) );
		}
		else
		{
			capturedStateBitmask::lightCaptureStruct& foundLightCapture = findExistingLightIt->second;
			foundLightCapture.captureSetLight = true;
		}
	}

	// This is intended to be called from IDirect3DDevice9Hook::CreateStateBlock
	void InitializeListAndCapture(const D3DSTATEBLOCKTYPE type);

protected:
	LPDIRECT3DSTATEBLOCK9 realObject;
	IDirect3DDevice9Hook* parentDevice;
	unsigned __int64 refCount;

	D3DSTATEBLOCKTYPE internalStateBlockType;
	bool isCompleteStateBlock; // true if this state block was created from IDirect3DDevice9::CreateStateBlock(), or false if this state block was created from IDirect3DDevice9::BeginStateBlock() + IDirect3DDevice9::EndStateBlock()
	__declspec(align(16) ) DeviceState stateBlockState;
	__declspec(align(16) ) capturedStateBitmask capturedStates;
};
