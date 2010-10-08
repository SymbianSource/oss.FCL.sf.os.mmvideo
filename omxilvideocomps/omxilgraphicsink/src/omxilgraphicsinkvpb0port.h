/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef OMXILGRAPHICSINKVPB0PORT_H
#define OMXILGRAPHICSINKVPB0PORT_H

#include <openmax/il/common/omxilvideoport.h>
#include "omxilgraphicsinkprocessingfunction.h"
#include <openmax/il/extensions/omxilsymbianvideographicsinkextensions.h>
#include <openmax/il/shai/OMX_Symbian_ComponentExt.h>
#include "mmfbuffershared.h"

/**
Class COmxILGraphicSinkVPB0Port represents the input port 0 for the OpenMAX IL based graphics sink component.
*/
class COmxILGraphicSinkVPB0Port : public COmxILVideoPort
	{	
public:

	static COmxILGraphicSinkVPB0Port* NewL(
		const TOmxILCommonPortData& aCommonPortData,
		const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedVideoFormats,
		const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColorFormats,
		COmxILGraphicSinkProcessingFunction& aGraphicSinkPF);
	
	~COmxILGraphicSinkVPB0Port();

	OMX_ERRORTYPE GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const;

	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex, TAny* apComponentParameterStructure) const;
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex, const TAny* apComponentParameterStructure, TBool& aUpdateProcessingFunction);

	OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE aConfigIndex, TAny* apComponentConfigStructure) const;
	OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE aConfigIndex, const TAny* apComponentConfigStructure, TBool& aUpdateProcessingFunction);
	
	OMX_ERRORTYPE GetExtensionIndex(OMX_STRING aParameterName, OMX_INDEXTYPE* apIndexType) const;
	OMX_ERRORTYPE ValidateStride();

private:

	COmxILGraphicSinkVPB0Port(
		COmxILGraphicSinkProcessingFunction& aGraphicSinkPF);
	
	void ConstructL(const TOmxILCommonPortData& aCommonPortData,
	        const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedVideoFormats,
	        const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColorFormats);
	
	OMX_ERRORTYPE SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition, TBool& aUpdateProcessingFunction);

	TBool IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition) const;

	OMX_ERRORTYPE DoBufferAllocation(OMX_U32 aSizeBytes, OMX_U8*& apPortSpecificBuffer, OMX_PTR& apPortPrivate, OMX_PTR& apPlatformPrivate, OMX_PTR apAppPrivate = 0);
	void DoBufferDeallocation(OMX_PTR apPortSpecificBuffer, OMX_PTR apPortPrivate, OMX_PTR apPlatformPrivate, OMX_PTR apAppPrivate = 0);
	OMX_ERRORTYPE DoBufferWrapping(OMX_U32 aSizeBytes, OMX_U8* apBuffer, OMX_PTR& apPortPrivate, OMX_PTR& apPlatformPrivate, OMX_PTR apAppPrivate = 0);
	void DoBufferUnwrapping(OMX_PTR apPortSpecificBuffer, OMX_PTR apPortPrivate, OMX_PTR apPlatformPrivate, OMX_PTR apAppPrivate = 0);
	OMX_ERRORTYPE DoOmxUseBuffer(OMX_HANDLETYPE aTunnelledComponent, OMX_BUFFERHEADERTYPE** appBufferHdr, OMX_U32 aTunnelledPortIndex, OMX_PTR apPortPrivate, OMX_PTR apPlatformPrivate, OMX_U32 aSizeBytes, OMX_U8* apBuffer);

	void InitParamsAndConfigs();
	void UpdateParamInPortDefinitionStruct();
	
private:
	/** The reference of GraphicSink ProcessingFunction. */
	COmxILGraphicSinkProcessingFunction& iGraphicSinkPF;
	/** The type of Mime. */
	RBuf8 iMimeTypeBuf;
	
	// The scaling size of video or image data.
	OMX_CONFIG_SCALEFACTORTYPE iConfigScaleFactor;
	// The size of frame.
	OMX_FRAMESIZETYPE iConfigFrameSize;
	// The size of rectangle.
	OMX_CONFIG_RECTTYPE iConfigRec;

	// Extension to provide a structure for Surface Configuration.
	OMX_SYMBIAN_VIDEO_PARAM_SURFACECONFIGURATION iParamVideoSurfaceConfiguration;

    TMMSharedChunkBufConfig iSharedChunkBufConfig;
    OMX_INDEXTYPE iSharedChunkMetadataExtensionIndex;
	};

#endif // OMXILGRAPHICSINKVPB0PORT_H
