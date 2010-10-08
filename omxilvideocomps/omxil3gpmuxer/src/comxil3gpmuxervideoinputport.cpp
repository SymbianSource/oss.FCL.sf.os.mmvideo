/*
* Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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

#include "comxil3gpmuxervideoinputport.h"
#include "comxil3gpmuxerprocessingfunction.h"
#include "log.h"

const TUint KDefaultBitRate = 5000;



COmxIL3GPMuxerVideoInputPort* COmxIL3GPMuxerVideoInputPort::NewL(const TOmxILCommonPortData& aCommonPortData)

	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerVideoInputPort::NewL"));
	// TODO these arrays must left empty, to be removed from the video port constructor
	RArray<OMX_VIDEO_CODINGTYPE> supportedCodings;
	RArray<OMX_COLOR_FORMATTYPE> supportedColorFormats;
	CleanupClosePushL(supportedCodings);
	CleanupClosePushL(supportedColorFormats);
	
	COmxIL3GPMuxerVideoInputPort* self = new(ELeave) COmxIL3GPMuxerVideoInputPort();
	CleanupStack::PushL(self);
	self->ConstructL(aCommonPortData, supportedCodings, supportedColorFormats);
	CleanupStack::Pop(self);
	
	CleanupStack::PopAndDestroy(2, &supportedCodings);
	
	return self;
	}

COmxIL3GPMuxerVideoInputPort::COmxIL3GPMuxerVideoInputPort()
	{
	GetParamPortDefinition().format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	}

void COmxIL3GPMuxerVideoInputPort::ConstructL(const TOmxILCommonPortData& aCommonPortData, 
                                const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedCodings,
                                const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColourFormats)
	{
	COmxILVideoPort::ConstructL(aCommonPortData, aSupportedCodings, aSupportedColourFormats);
	// as there are only four items, do not require a sort order
	// if this list gets larger consider using binary search
	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingUnused);
	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingAVC);
	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingH263);
	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingMPEG4);

	GetSupportedColorFormats().AppendL(OMX_COLOR_FormatUnused);
	
	GetParamPortDefinition().format.video.nBitrate = KDefaultBitRate;
	}

COmxIL3GPMuxerVideoInputPort::~COmxIL3GPMuxerVideoInputPort()
	{
	}

OMX_ERRORTYPE COmxIL3GPMuxerVideoInputPort::GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const
	{
	OMX_ERRORTYPE omxRetValue = COmxILVideoPort::GetLocalOmxParamIndexes(aIndexArray);
	if (omxRetValue != OMX_ErrorNone)
		{
		return omxRetValue;
		}
	
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxIL3GPMuxerVideoInputPort::GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILVideoPort::GetLocalOmxConfigIndexes(aIndexArray);
	}

OMX_ERRORTYPE COmxIL3GPMuxerVideoInputPort::GetParameter(OMX_INDEXTYPE aParamIndex,
                                                            TAny* apComponentParameterStructure) const
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerVideoInputPort::GetParameter : aParamIndex[%u]"), aParamIndex);
	return COmxILVideoPort::GetParameter(aParamIndex, apComponentParameterStructure);
	}

OMX_ERRORTYPE COmxIL3GPMuxerVideoInputPort::SetParameter(OMX_INDEXTYPE aParamIndex,
                                                            const TAny* apComponentParameterStructure,
                                                            TBool& aUpdateProcessingFunction)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerVideoInputPort::SetParameter : aParamIndex[%u]"), aParamIndex);
	return COmxILVideoPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);
	}

OMX_ERRORTYPE COmxIL3GPMuxerVideoInputPort::SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
                                                                TBool& aUpdateProcessingFunction)
	{
	if (aPortDefinition.nBufferCountActual > KMaxVideoBuffers)
		{
		return OMX_ErrorBadParameter;
		}

	const OMX_VIDEO_PORTDEFINITIONTYPE& newVidDef = aPortDefinition.format.video;
	OMX_VIDEO_PORTDEFINITIONTYPE&  myVidDef = GetParamPortDefinition().format.video;
	
	if(newVidDef.eColorFormat != OMX_COLOR_FormatUnused)
		{
		return OMX_ErrorBadParameter;
		}
	// change to FindInUnsignedKeyOrder if many more formats are added
	// FIXME this should allow OMX_VIDEO_CodingAutoDetect
	if(GetSupportedVideoFormats().Find(newVidDef.eCompressionFormat) == KErrNotFound)
		{
		return OMX_ErrorBadParameter;
 		}
	
	// copy the new port definition
	myVidDef = newVidDef;
	
	// ignore parameters which make no sense, either because the output is compressed or because
	// we are not a display component
	myVidDef.nSliceHeight = 0;
	myVidDef.nStride = 0;
	myVidDef.pNativeRender = NULL;
	myVidDef.pNativeWindow = NULL;
	
	// ignore the MIME type - the field eCompressionFormat will identify the stream type.
	// if we want to support GetParameter() for the MIME type, I think the component needs to
	// make a copy of the C string passed in and return a pointer to that. (cMIMEType is a char*).
	myVidDef.cMIMEType = NULL;
	
	aUpdateProcessingFunction = ETrue;

	return OMX_ErrorNone;
	}

TBool COmxIL3GPMuxerVideoInputPort::IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& /*aPortDefinition*/) const
	{
	// TODO (The base class should do this checking) 
	return ETrue;
	}

void COmxIL3GPMuxerVideoInputPort::GetVideoProperties(OMX_U32& aFrameWidth, OMX_U32& aFrameHeight, OMX_U32& aFramerate, OMX_U32& aBitRate)
	{
	aFrameWidth = GetParamPortDefinition().format.video.nFrameWidth;
	aFrameHeight = GetParamPortDefinition().format.video.nFrameHeight;
	aFramerate = GetParamPortDefinition().format.video.xFramerate;
	aBitRate = GetParamPortDefinition().format.video.nBitrate;
	}
