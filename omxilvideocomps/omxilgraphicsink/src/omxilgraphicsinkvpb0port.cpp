/*
* Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

/**
 @file
 @internalComponent
*/

#include <e32std.h>
#include "omxilgraphicsinkvpb0port.h"
#include <openmax/il/khronos/v1_x/OMX_Component.h>
#include <openmax/il/shai/OMX_Symbian_ExtensionNames.h>
#include <w32std.h>
#include "log.h"

#include "omxilgraphicsinkextensionsindexes.h"

// Port definition mime type. Mime type is not relevant for uncompressed video frames
_LIT8(KMimeType, "");

// Constant numbers
static const TUint32 KRefGfxFramerate = 0;   // mandatory
static const TUint32 KRefGfxMinBufferSize = 1024;

/**
Create a new VPB0 port.

@param 	aCommonPortData The common information of the new VPB0 port.
		aSupportedVideoFormats The list of the supported video formats.
		aSupportedColorFormats The list of the supported color formats.
		aGraphicSinkPF The processing function for the VPB0 port.
		
@return A pointer of the VPB0Port to be created.
*/
COmxILGraphicSinkVPB0Port* COmxILGraphicSinkVPB0Port::NewL(
		const TOmxILCommonPortData& aCommonPortData,
		const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedVideoFormats,
		const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColorFormats,
		COmxILGraphicSinkProcessingFunction& aGraphicSinkPF)
	{	
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::NewL +"));
	COmxILGraphicSinkVPB0Port* self = new (ELeave)COmxILGraphicSinkVPB0Port(
										aGraphicSinkPF);
	CleanupStack::PushL(self);
	self->ConstructL(aCommonPortData, aSupportedVideoFormats, aSupportedColorFormats);
	CleanupStack::Pop(self);
	return self;
	}

/**
Second phase construction for the class COmxILGraphicSinkVPB0Port. Initializes the param structures and config structures for GraphicSink port. 
Also appends the supported color and video formats to the respective arrays.
*/
void COmxILGraphicSinkVPB0Port::ConstructL(const TOmxILCommonPortData& aCommonPortData,
                                const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedVideoFormats,
                                const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColorFormats)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::ConstructL +"));
	COmxILVideoPort::ConstructL(aCommonPortData, aSupportedVideoFormats, aSupportedColorFormats);
 	GetParamPortDefinition().eDomain = OMX_PortDomainVideo;
 	
 	iMimeTypeBuf.CreateL(KMimeType(), KMimeType().Length() + 1);
 	TUint8* pTUint = const_cast<TUint8*>(iMimeTypeBuf.PtrZ());
 	GetParamPortDefinition().format.video.cMIMEType = reinterpret_cast<OMX_STRING>(pTUint);
 	GetParamPortDefinition().format.video.pNativeRender = NULL;
 	GetParamPortDefinition().format.video.pNativeWindow = NULL;

	GetParamPortDefinition().nBufferSize = KRefGfxMinBufferSize;

 	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingUnused);
	GetSupportedColorFormats().AppendL(OMX_COLOR_FormatCbYCrY);
 	GetSupportedColorFormats().AppendL(OMX_COLOR_FormatYCrYCb); 	
 	GetSupportedColorFormats().AppendL(OMX_COLOR_Format16bitRGB565);
	GetSupportedColorFormats().AppendL(OMX_COLOR_Format32bitARGB8888);
	
 	InitParamsAndConfigs();

 	iSharedChunkMetadataExtensionIndex = OMX_IndexComponentStartUnused;
	}

/**
Constructor of the class.

@param 	aCommonPortData The common information of the new VPB0 port.
		aSupportedVideoFormats The list of the supported video formats.
		aSupportedColorFormats The list of the supported color formats.
		aGraphicSinkPF The processing function for the VPB0 port.	
*/
COmxILGraphicSinkVPB0Port::COmxILGraphicSinkVPB0Port(
		COmxILGraphicSinkProcessingFunction& aGraphicSinkPF)
	: iGraphicSinkPF(aGraphicSinkPF)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::COmxILGraphicSinkVPB0Port +"));
	}

/**
Destructor of the class.
*/
COmxILGraphicSinkVPB0Port::~COmxILGraphicSinkVPB0Port()
	{
    DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::~COmxILGraphicSinkVPB0Port +"));

    CleanUpPort();
	iMimeTypeBuf.Close();
	}

/**
Sets the format field available as part of OMX_PARAM_PORTDEFINITIONTYPE.

@param 	aPortDefinition structure containing the format field to be updated.
		aUpdateProcessingFunction indicates whether or not processing function needs to be updated.
		
@return OMX_ErrorNone if successful;
		OMX_ErrorBadParameter if both compression format and color format are unused at the same time, or nStride is invalid;
		OMX_ErrorUnsupportedSetting if unsupported compression format and color format, or bitrate is non-zero;
*/	
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,TBool& aUpdateProcessingFunction)
	{
    DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::SetFormatInPortDefinition +"));

    // Check settings in input OMX_PARAM_PORTDEFINITIONTYPE.
	if (!UpdateColorFormat(GetParamPortDefinition().format.video.eColorFormat, aPortDefinition.format.video.eColorFormat, aUpdateProcessingFunction))
		{
		return OMX_ErrorUnsupportedSetting;
		}

	if (!UpdateCodingType(GetParamPortDefinition().format.video.eCompressionFormat, aPortDefinition.format.video.eCompressionFormat, aUpdateProcessingFunction))
		{
		return OMX_ErrorUnsupportedSetting;
		}

	if(GetParamPortDefinition().format.video.nStride != aPortDefinition.format.video.nStride)
    	{
 		// nStride shall not be 0x0
 		if(0x0 == aPortDefinition.format.video.nStride)
	 		{
	 		return OMX_ErrorBadParameter;
	 		}

		GetParamPortDefinition().format.video.nStride = aPortDefinition.format.video.nStride;
		aUpdateProcessingFunction = ETrue;
    	}

	if (GetParamPortDefinition().format.video.nFrameWidth != aPortDefinition.format.video.nFrameWidth ||
	    GetParamPortDefinition().format.video.nFrameHeight != aPortDefinition.format.video.nFrameHeight ||
		GetParamPortDefinition().format.video.nBitrate != aPortDefinition.format.video.nBitrate)
	    {
        GetParamPortDefinition().format.video.nFrameWidth = aPortDefinition.format.video.nFrameWidth;
        GetParamPortDefinition().format.video.nFrameHeight = aPortDefinition.format.video.nFrameHeight;
        // nSliceHeight is a RO attribute. Policy is to set it to the frame height
        GetParamPortDefinition().format.video.nSliceHeight = aPortDefinition.format.video.nFrameHeight;
        GetParamPortDefinition().format.video.nBitrate = aPortDefinition.format.video.nBitrate;    
        aUpdateProcessingFunction = ETrue;
	    }
    return OMX_ErrorNone;
	}

/**
Insert new indices belonging to the static parameter category.

@param 	aIndexArray The array of indices for insert.

@return OMX_ErrorNone if successful;
		OMX_ErrorInsufficientResources if fail to insert indexes;
*/
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const
	{
    DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::GetLocalOmxParamIndexes +"));

	// Always collect param indexes from parents
	OMX_ERRORTYPE omxRetValue = COmxILVideoPort::GetLocalOmxParamIndexes(aIndexArray);
	
	if(OMX_ErrorNone != omxRetValue)
		{
		return omxRetValue; 
		}
	
	TInt err = aIndexArray.InsertInOrder(OMX_NokiaIndexParamGraphicSurfaceConfig);
	
	// Note that index duplication is OK.
	if (KErrNone != err && KErrAlreadyExists != err)
		{
		return OMX_ErrorInsufficientResources;
		}

	return OMX_ErrorNone;
	}

/**
Insert new indices belonging to the dynamic configuration category.

@param 	aIndexArray The array of indices for insert.

@return OMX_ErrorNone if successful;
		OMX_ErrorInsufficientResources if fail to insert indexes;
*/
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const
	{
    DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::GetLocalOmxConfigIndexes +"));

	// Always collect local indexes from parent
	OMX_ERRORTYPE omxRetValue = COmxILVideoPort::GetLocalOmxConfigIndexes(aIndexArray);
	if (omxRetValue != OMX_ErrorNone)
		{
		return omxRetValue;
		}
		
	TInt err = aIndexArray.InsertInOrder(OMX_IndexConfigCommonScale);
	// Note that index duplication is OK.
	if (err == KErrNone || err == KErrAlreadyExists)
		{
		err = aIndexArray.InsertInOrder(OMX_IndexConfigCommonOutputSize);
		}

	if (err == KErrNone || err == KErrAlreadyExists)
		{
		err = aIndexArray.InsertInOrder(OMX_IndexConfigCommonInputCrop);
		}

	if (err == KErrNone || err == KErrAlreadyExists)
		{
		err = aIndexArray.InsertInOrder(OMX_IndexConfigCommonOutputCrop);
		}

	if (err == KErrNone || err == KErrAlreadyExists)
		{
		err = aIndexArray.InsertInOrder(OMX_IndexConfigCommonExclusionRect);
		}
	
    if (err == KErrNone || err == KErrAlreadyExists)
        {
        err = aIndexArray.InsertInOrder(OMX_SymbianIndexConfigSharedChunkMetadata);
        }

	if (err != KErrNone && err != KErrAlreadyExists)
		{
		return OMX_ErrorInsufficientResources;
		}

	return OMX_ErrorNone;
	}

/**
This method provides the current values for the parameters present in the structure represented by the given index.

@param 	aParamIndex The specific param index for which the current parameter values are required.
		apComponentParameterStructure The pointer to the structure which will be updated to provide the current parameter values.
		
@return OMX_ErrorNone if successful;
		OMX_ErrorNoMore if no more formats;
		OMX_ErrorUnsupportedSetting if unsupported setting is passed;
*/
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::GetParameter(OMX_INDEXTYPE aParamIndex,TAny* apComponentParameterStructure) const
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::GetParameter +"));
	switch(aParamIndex)
		{
		case OMX_NokiaIndexParamGraphicSurfaceConfig:
			{
			OMX_SYMBIAN_VIDEO_PARAM_SURFACECONFIGURATION* videoSurface = static_cast<OMX_SYMBIAN_VIDEO_PARAM_SURFACECONFIGURATION*>(apComponentParameterStructure);
			*videoSurface = iParamVideoSurfaceConfiguration;
			break;
			}
		case OMX_IndexParamVideoPortFormat:
			{
			OMX_VIDEO_PARAM_PORTFORMATTYPE* compvideoPortFormat = static_cast<OMX_VIDEO_PARAM_PORTFORMATTYPE*>(apComponentParameterStructure);
			
			// framerate should be always zero for GFX otherwise unsupported value
			if(KRefGfxFramerate != compvideoPortFormat->xFramerate)
				{
				return OMX_ErrorUnsupportedSetting;
				}
			
			return COmxILVideoPort::GetParameter(aParamIndex,apComponentParameterStructure);
			}
		default:
			{
			// Try the parent's indexes
			return COmxILVideoPort::GetParameter(aParamIndex,apComponentParameterStructure);
			}
		};
		
	return OMX_ErrorNone;
	}

/**
This method sets the values of the parameters present in the structure represented by the given index.

@param 	aParamIndex The specific param index for which the parameter values have to be set.
		apComponentParameterStructure The pointer to the structure which will provide the desired parameter values to be set.
		aUpdateProcessingFunction informs whether the processing fucntion needs to be updated.
		
@return OMX_ErrorNone if successful;
		OMX_ErrorUnsupportedSetting if non-zero framerate value;
		OMX_ErrorUnsupportedIndex if request OMX_NokiaIndexParamGraphicSurfaceConfig index;
*/
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::SetParameter(OMX_INDEXTYPE aParamIndex,const TAny* apComponentParameterStructure, TBool& aUpdateProcessingFunction)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::SetParameter +"));
	
	OMX_ERRORTYPE error = OMX_ErrorNone; 
	switch(aParamIndex)
		{
		case OMX_IndexParamVideoPortFormat:
			{
			const OMX_VIDEO_PARAM_PORTFORMATTYPE *componentParameterStructure = static_cast<const OMX_VIDEO_PARAM_PORTFORMATTYPE*>(apComponentParameterStructure);
			
			if (GetParamPortDefinition().format.video.xFramerate != componentParameterStructure->xFramerate)
				{
				if (0 != componentParameterStructure->xFramerate)
					{
					// Known Frame rate is not supported. framerate should be always zero
					return OMX_ErrorUnsupportedSetting;
					}
				}

			error = COmxILVideoPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);
			if(error != OMX_ErrorNone)
				{
				return error;
				}

			if(aUpdateProcessingFunction)
				{
				// eColorFormat and/or eCompressionFormat and/or xFramerate is/are changed by OMX_IndexParamVideoPortFormat
				// Hence change same varibles associated to index OMX_IndexParamPortDefinition.
				// Update their value in OMX_PARAM_PORTDEFINITIONTYPE.
				UpdateParamInPortDefinitionStruct();
				return OMX_ErrorNone;
				}

			break;
			}
		case OMX_NokiaIndexParamGraphicSurfaceConfig:
			{
			// IL Client can only query OMX_GetParameter with this index but can not set any value on TSurfaceConfiguration
			// GFX simply returns OMX_ErrorUnsupportedIndex for the attempt on OMX_GetParameter with this index.
			return OMX_ErrorUnsupportedIndex;
			}
	    default:
			{
			// Try the parent's indexes
			return COmxILVideoPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);			
			}
		};	
	return error;
	}

/**
This method provides the current values for the configurations present in the structure represented by the given index.

@param 	aConfigIndex The specific configuration index for which the current configuration values are required.
		apComponentConfigStructure The pointer to the structure which will be updated to provide the current configuration values.
		
@return OMX_ErrorNone if successful;
		OMX_ErrorUnsupportedIndex if request non-inserted config index;
*/	
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::GetConfig(OMX_INDEXTYPE aConfigIndex,TAny* apComponentParameterStructure) const
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::GetConfig +"));
	switch(aConfigIndex)
		{
        case OMX_SymbianIndexConfigSharedChunkMetadata:
            {
            OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE*
                pSharedChunkMetadata
                = static_cast<
                        OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE*>(apComponentParameterStructure);
        
            iGraphicSinkPF.GetSharedChunkMetadata(
                    pSharedChunkMetadata->nHandleId, 
                    pSharedChunkMetadata->nOwnerThreadId);
            }
            break;
		case OMX_IndexConfigCommonScale:
			{
			OMX_CONFIG_SCALEFACTORTYPE* scaleFactor = static_cast<OMX_CONFIG_SCALEFACTORTYPE*>(apComponentParameterStructure);
			*scaleFactor = iConfigScaleFactor;
			break;
			}
		case OMX_IndexConfigCommonOutputSize:
			{
			OMX_FRAMESIZETYPE* frameSize = static_cast<OMX_FRAMESIZETYPE*>(apComponentParameterStructure);
			*frameSize = iConfigFrameSize;
			break;
			}
		case OMX_IndexConfigCommonInputCrop:
		case OMX_IndexConfigCommonOutputCrop:
		case OMX_IndexConfigCommonExclusionRect:
			{
			OMX_CONFIG_RECTTYPE* rec = static_cast<OMX_CONFIG_RECTTYPE*>(apComponentParameterStructure);
			*rec = iConfigRec;
			break;
			}
		default:
			{
			// Try the parent's indexes
			return COmxILVideoPort::GetConfig(aConfigIndex,apComponentParameterStructure);
			}
		};
		
	return OMX_ErrorNone;
	}

/**
This method sets the values of the configurations present in the structure represented by the given index.

@param 	aConfigIndex The specific configuration index for which the configuration values have to be set.
		apComponentConfigStructure The pointer to the structure which will be provide the desired configuration values to be set.
		aUpdateProcessingFunction informs whether the processing fucntion needs to be updated.
		
@return OMX_ErrorNone if successful;
		OMX_ErrorUnsupportedIndex if request OMX_NokiaIndexParamGraphicSurfaceConfig index;
*/	
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::SetConfig(OMX_INDEXTYPE aConfigIndex,const TAny* apComponentParameterStructure, TBool& aUpdateProcessingFunction)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::SetConfig +"));
	switch(aConfigIndex)
		{
	    case OMX_SymbianIndexConfigSharedChunkMetadata:
	        {
	        aUpdateProcessingFunction = ETrue;
	        break;
	        }
		case OMX_IndexConfigCommonScale:
			{
			const OMX_CONFIG_SCALEFACTORTYPE* scaleFactor = static_cast<const OMX_CONFIG_SCALEFACTORTYPE*>(apComponentParameterStructure);
			
			if(iConfigScaleFactor.xWidth != scaleFactor->xWidth)
				{
				iConfigScaleFactor.xWidth = scaleFactor->xWidth;
				aUpdateProcessingFunction = ETrue;
				}
			if(iConfigScaleFactor.xHeight != scaleFactor->xHeight)
				{	
				iConfigScaleFactor.xHeight = scaleFactor->xHeight;
				aUpdateProcessingFunction = ETrue;
				}
			break;
			}
		case OMX_IndexConfigCommonOutputSize:
			{
			const OMX_FRAMESIZETYPE* frameSize = static_cast<const OMX_FRAMESIZETYPE*>(apComponentParameterStructure);
			
			if(iConfigFrameSize.nWidth != frameSize->nWidth)
				{
				iConfigFrameSize.nWidth = frameSize->nWidth;
				aUpdateProcessingFunction = ETrue;
				}
			if(iConfigFrameSize.nHeight != frameSize->nHeight)
				{
				iConfigFrameSize.nHeight = frameSize->nHeight;
				aUpdateProcessingFunction = ETrue;
				}
			break;
			}
		case OMX_IndexConfigCommonInputCrop:
		case OMX_IndexConfigCommonOutputCrop:
		case OMX_IndexConfigCommonExclusionRect:
			{
			const OMX_CONFIG_RECTTYPE* rec = static_cast<const OMX_CONFIG_RECTTYPE*>(apComponentParameterStructure);
			
			if(iConfigRec.nTop != rec->nTop)
				{
				iConfigRec.nTop = rec->nTop;
				aUpdateProcessingFunction = ETrue;
				}
			if(iConfigRec.nLeft != rec->nLeft)
				{
				iConfigRec.nLeft = rec->nLeft;
				aUpdateProcessingFunction = ETrue;
				}
			if(iConfigRec.nWidth != rec->nWidth)
				{
				iConfigRec.nWidth = rec->nWidth;
				aUpdateProcessingFunction = ETrue;
				}
			if(iConfigRec.nHeight != rec->nHeight)
				{
				iConfigRec.nHeight = rec->nHeight;
				aUpdateProcessingFunction = ETrue;
				}
			break;
			}
	    default:
			{
			// Try the parent's indexes
			return COmxILVideoPort::SetConfig(aConfigIndex, apComponentParameterStructure, aUpdateProcessingFunction);
			}
		};

	return OMX_ErrorNone;
	}

/**
This method provides the index type represented by the given parameter name.

@param 	aParameterName The name of extention parameter to be retrieved.
		apIndexType The pointer which will retrieve the required index.
		
@return OMX_ErrorNone if successful;
		OMX_ErrorUnsupportedIndex if unsupported parameter name is passed;
*/	
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::GetExtensionIndex(OMX_STRING aParameterName,	OMX_INDEXTYPE* apIndexType) const
	{
    DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::GetExtensionIndex"));

	TPtrC8 requestedParameterNamePtr(const_cast<const TUint8*>(reinterpret_cast<TUint8*>(aParameterName)));
	
	// OMX_NokiaIndexParamGraphicSurfaceConfig
	TPtrC8 parameterNamePtr(reinterpret_cast<const TUint8*>(sOmxSymbianGfxSurfaceConfig));
	if (requestedParameterNamePtr == parameterNamePtr)
		{
		*apIndexType = static_cast<OMX_INDEXTYPE>(OMX_NokiaIndexParamGraphicSurfaceConfig);
		return OMX_ErrorNone;   
		}
	   
    // OMX_SymbianIndexConfigSharedChunkMetadata
	TPtrC8 parameterNamePtr2(reinterpret_cast<const TUint8*>(OMX_SYMBIAN_INDEX_CONFIG_SHAREDCHUNKMETADATA_NAME));
	if(requestedParameterNamePtr == parameterNamePtr2)
        {
        *apIndexType = static_cast<OMX_INDEXTYPE>(OMX_SymbianIndexConfigSharedChunkMetadata);
        return OMX_ErrorNone;
        }

    *apIndexType = OMX_IndexMax;

	return OMX_ErrorUnsupportedIndex;
	}

/**
Check whether or not the tunnelled ports are compatible.

@param 	aPortDefinition The port definition parameters to be checked for compatibility.

@return EFalse if tunnelled ports' parameters are incompatible;
		ETrue otherwise;
*/
TBool COmxILGraphicSinkVPB0Port::IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition) const
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::IsTunnelledPortCompatible +"));

	if(aPortDefinition.eDomain != GetParamPortDefinition().eDomain)
		{
		return EFalse;
		}

	if(OMX_VIDEO_CodingUnused != aPortDefinition.format.video.eCompressionFormat)
		{
		return EFalse;
		}
	
	if (GetParamPortDefinition().format.video.eColorFormat != aPortDefinition.format.video.eColorFormat)
		{
		return EFalse;
		}
	
	if (GetParamPortDefinition().format.video.nFrameWidth != aPortDefinition.format.video.nFrameWidth)
        {
        return EFalse;
        }
	
    if (GetParamPortDefinition().format.video.nFrameHeight != aPortDefinition.format.video.nFrameHeight)
        {
        return EFalse;
        }
	
    if (GetParamPortDefinition().format.video.nStride != aPortDefinition.format.video.nStride)
        {
        return EFalse;
        }

	return ETrue;
	}

/**
Allocates buffer, creates surface and updates the surface configuration.

@param 	aSizeBytes The size of buffer.
		apPortSpecificBuffer The address of buffer.
		apPortPrivate The pointer of COmxILMMBuffer.
		apAppPrivate Not used (default = 0).
		
@return OMX_ErrorNone if successful;
		OMX_ErrorInsufficientResources if failed to allocate buffer or update the surface configuration;
		OMX_ErrorBadParameter if illegal parameters are passed;
		
@see COmxILGraphicSinkProcessingFunction.
*/
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::DoBufferAllocation(OMX_U32 /*aSizeBytes*/, OMX_U8*& apPortSpecificBuffer, OMX_PTR& apPortPrivate, OMX_PTR& /*apPlatformPrivate*/, OMX_PTR /* apAppPrivate */)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::DoBufferAllocation +"));
	
	// Create buffers allocated to be used for other components. 
	TRAPD(error, iGraphicSinkPF.CreateBufferL(apPortSpecificBuffer, apPortPrivate, GetParamPortDefinition().nBufferCountActual));
	switch(error)
		{
		case KErrNone:
			break;
		case KErrNoMemory:
			return OMX_ErrorInsufficientResources;
		default:
			return OMX_ErrorBadParameter;
		};
	
	return OMX_ErrorNone;
	}

/**
Utilizes buffer given from allocator component to create surface, and updates the surface configuration.

@param 	aSizeBytes The size of buffer.
		apPortSpecificBuffer The address of buffer.
		apPortPrivate The pointer of COmxILMMBuffer.
		apAppPrivate Not used (default = 0).
		
@return OMX_ErrorNone if successful;
		OMX_ErrorInsufficientResources if failed to utilize buffer or update the surface configuration;
		OMX_ErrorBadParameter if illegal parameters are passed;
		
@see COmxILGraphicSinkProcessingFunction.
*/
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::DoBufferWrapping(OMX_U32 aSizeBytes, OMX_U8* apBuffer, OMX_PTR& /*apPortPrivate*/, OMX_PTR& /*apPlatformPrivate*/, OMX_PTR /*apAppPrivate*/)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::DoBufferWrapping +"));

	// Init buffers given from allocator component.
	TRAPD(error, iGraphicSinkPF.InitBufferL(aSizeBytes, apBuffer, GetParamPortDefinition().nBufferCountActual));
	switch(error)
		{
		case KErrNone:
			break;
		case KErrNoMemory:
			return OMX_ErrorInsufficientResources;
		default:
			return OMX_ErrorBadParameter;
		};

	return OMX_ErrorNone;
	}

/**
Deallocate buffer and deallocates local resources in the processing function.

@param 	apPortSpecificBuffer Not used.
		apPortPrivate Not used.
		apAppPrivate Not used (default = 0).
*/
void COmxILGraphicSinkVPB0Port::DoBufferDeallocation(OMX_PTR /*apPortSpecificBuffer*/, OMX_PTR apPortPrivate, OMX_PTR /*apPlatformPrivate*/, OMX_PTR /* apAppPrivate */)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::DoBufferDeallocation"));
	iGraphicSinkPF.DestroyBuffer(apPortPrivate);
	}

/**
Deinitialise buffer and deallocates local resources in the processing function.

@param 	apPortSpecificBuffer Not used.
		apPortPrivate Not used.
		apAppPrivate Not used (default = 0).
*/
void COmxILGraphicSinkVPB0Port::DoBufferUnwrapping(OMX_PTR /* apPortSpecificBuffer */, OMX_PTR /* apPortPrivate */, OMX_PTR /*apPlatformPrivate*/, OMX_PTR /* apAppPrivate */)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkVPB0Port::DoBufferUnwrapping"));
	iGraphicSinkPF.DeInitBuffer();
	}


/**
Update ParamInPortDefinition struct.
*/
void COmxILGraphicSinkVPB0Port::UpdateParamInPortDefinitionStruct()
	{
	// OMX_PARAM_PORTDEFINITION structure needs to be updated with respect to the changes from other OMX_PARAM structures.
	GetParamPortDefinition().format.video.eColorFormat = GetParamVideoPortFormat().eColorFormat;
	GetParamPortDefinition().format.video.xFramerate = GetParamVideoPortFormat().xFramerate;
	GetParamPortDefinition().format.video.eCompressionFormat = GetParamVideoPortFormat().eCompressionFormat;
	}

/**
Initialise all param and config structures for GraphicSink port.	
*/
void COmxILGraphicSinkVPB0Port::InitParamsAndConfigs()
	{
 	// init OMX_SYMBIAN_VIDEO_CONFIG_SURFACECONFIGRATION here
 	iParamVideoSurfaceConfiguration.nSize = sizeof(OMX_SYMBIAN_VIDEO_PARAM_SURFACECONFIGURATION);
 	iParamVideoSurfaceConfiguration.nVersion = TOmxILSpecVersion();
 	iParamVideoSurfaceConfiguration.nPortIndex = GetParamPortDefinition().nPortIndex;
 	iParamVideoSurfaceConfiguration.pSurfaceConfig = &iGraphicSinkPF.GetSurfaceConfiguration(); 
 	
 	// init OMX_CONFIG_SCALEFACTORTYPE
 	iConfigScaleFactor.nSize = sizeof(OMX_CONFIG_SCALEFACTORTYPE);
 	iConfigScaleFactor.nVersion = TOmxILSpecVersion();
 	iConfigScaleFactor.nPortIndex = GetParamPortDefinition().nPortIndex;
 	
 	// init OMX_FRAMESIZETYPE
 	iConfigFrameSize.nSize = sizeof(OMX_FRAMESIZETYPE);
 	iConfigFrameSize.nVersion = TOmxILSpecVersion();
 	iConfigFrameSize.nPortIndex = GetParamPortDefinition().nPortIndex;
 	
 	// init OMX_CONFIG_RECTTYPE
 	iConfigRec.nSize = sizeof(OMX_CONFIG_RECTTYPE);
 	iConfigRec.nVersion = TOmxILSpecVersion();
 	iConfigRec.nPortIndex = GetParamPortDefinition().nPortIndex;
 	iConfigRec.nTop = 0;
 	iConfigRec.nLeft = 0;

    iSharedChunkBufConfig.iNumBuffers = GetParamPortDefinition().nBufferCountActual;
    iSharedChunkBufConfig.iBufferSizeInBytes = GetParamPortDefinition().nBufferSize; 
    iGraphicSinkPF.SetSharedChunkBufConfig(iSharedChunkBufConfig);
	}

TInt BytesPerPixel(OMX_COLOR_FORMATTYPE aPixelFormat)
    {
    switch (aPixelFormat)
        {
        //fall through
        
        //case EUidPixelFormatRGB_565:
        case OMX_COLOR_Format16bitRGB565:
        //case EUidPixelFormatBGR_565:
        case OMX_COLOR_Format16bitBGR565:       
        //case EUidPixelFormatARGB_1555:
        case OMX_COLOR_Format16bitARGB1555:
        //case EUidPixelFormatXRGB_1555:
        //case EUidPixelFormatARGB_4444:
        case OMX_COLOR_Format16bitARGB4444:
        //case EUidPixelFormatARGB_8332:
        case OMX_COLOR_Format8bitRGB332:
        //case EUidPixelFormatBGRX_5551:
        //case EUidPixelFormatBGRA_5551:
        //case EUidPixelFormatBGRA_4444:
        //case EUidPixelFormatBGRX_4444:
        //case EUidPixelFormatAP_88:
        //case EUidPixelFormatXRGB_4444:
        //case EUidPixelFormatXBGR_4444:
        //case EUidPixelFormatYUV_422Interleaved:
        //case EUidPixelFormatYUV_422Planar:
        case OMX_COLOR_FormatYUV422Planar:
        //case EUidPixelFormatYUV_422Reversed:
        case OMX_COLOR_FormatYCrYCb:
        //case EUidPixelFormatYUV_422SemiPlanar:
        case OMX_COLOR_FormatYUV422SemiPlanar:
        //case EUidPixelFormatYUV_422InterleavedReversed:
        //case EUidPixelFormatYYUV_422Interleaved:
        case OMX_COLOR_FormatCbYCrY:
        case OMX_COLOR_FormatYCbYCr:
        case OMX_COLOR_FormatRawBayer10bit:
            {
            return 2;
            }
            
        //fall through
        //case EUidPixelFormatXRGB_8888:
        //case EUidPixelFormatBGRX_8888:
        //case EUidPixelFormatXBGR_8888:
        //case EUidPixelFormatBGRA_8888:
        case OMX_COLOR_Format32bitBGRA8888:
        //case EUidPixelFormatARGB_8888:
        case OMX_COLOR_Format32bitARGB8888:
        //case EUidPixelFormatABGR_8888:
        //case EUidPixelFormatARGB_8888_PRE:
        //case EUidPixelFormatABGR_8888_PRE:
        //case EUidPixelFormatBGRA_8888_PRE:
        //case EUidPixelFormatARGB_2101010:
        //case EUidPixelFormatABGR_2101010:
            {
            return 4;
            }
            
        //fall through  
        //case EUidPixelFormatBGR_888:
        case OMX_COLOR_Format24bitBGR888:
        //case EUidPixelFormatRGB_888:
        case OMX_COLOR_Format24bitRGB888:
            {
            return 3;   
            }
            
        default:
            return 0;
        }
    }

/**
Check the nStride value is valid.	
*/
OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::ValidateStride()
	{
	if (GetParamPortDefinition().format.video.nStride < GetParamPortDefinition().format.video.nFrameWidth * BytesPerPixel(GetParamPortDefinition().format.video.eColorFormat))
	    {
		return OMX_ErrorBadParameter;
		}
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxILGraphicSinkVPB0Port::DoOmxUseBuffer(
    OMX_HANDLETYPE aTunnelledComponent,
    OMX_BUFFERHEADERTYPE** appBufferHdr,
    OMX_U32 aTunnelledPortIndex,
    OMX_PTR apPortPrivate,
    OMX_PTR /* apPlatformPrivate */,
    OMX_U32 aSizeBytes,
    OMX_U8* apBuffer)
    {

    // Find out if the tunnelled port has support for the
    // OMX.SYMBIAN.INDEX.CONFIG.SHAREDCHUNKMETADATA extension
    if (OMX_IndexComponentStartUnused == iSharedChunkMetadataExtensionIndex)
        {
        if (OMX_ErrorNone == OMX_GetExtensionIndex(
                aTunnelledComponent,
                const_cast<char*>(OMX_SYMBIAN_INDEX_CONFIG_SHAREDCHUNKMETADATA_NAME),
                &iSharedChunkMetadataExtensionIndex))
            {
            // Communicate the shared chunk metadata to the tunnelled
            // component
            OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE configSharedChunkMetadata;
            configSharedChunkMetadata.nSize = sizeof(OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE);
            configSharedChunkMetadata.nVersion = TOmxILSpecVersion();
            configSharedChunkMetadata.nPortIndex = GetParamPortDefinition().nPortIndex;
            iGraphicSinkPF.GetSharedChunkMetadata(
                    configSharedChunkMetadata.nHandleId,
                    configSharedChunkMetadata.nOwnerThreadId);

            // Ignore any error returned by the tunnelled component
            (void) OMX_SetConfig(aTunnelledComponent,
                                 iSharedChunkMetadataExtensionIndex,
                                 &configSharedChunkMetadata);
            }
        else
            {
            // No support in the tunnelled component.
            iSharedChunkMetadataExtensionIndex = OMX_IndexMax;
            }
        }
 
    return OMX_UseBuffer(
        aTunnelledComponent,
        appBufferHdr,
        aTunnelledPortIndex,
        apPortPrivate,
        aSizeBytes,
        apBuffer);
    }
