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

#include "comxil3gpdemuxervideooutputport.h"
#include "comxil3gpdemuxerprocessingfunction.h"
#include "tvideoformat.h"


_LIT(K3GPDemuxerVideoPortPanic, "COmxIL3GPDemuxerVideoOutputPort");

COmxIL3GPDemuxerVideoOutputPort* COmxIL3GPDemuxerVideoOutputPort::NewL(const TOmxILCommonPortData& aCommonPortData, COmxIL3GPDemuxerProcessingFunction& aProcessingFunction)

	{
	// TODO these arrays must left empty, to be removed from the video port constructor
	RArray<OMX_VIDEO_CODINGTYPE> supportedCodings;
	RArray<OMX_COLOR_FORMATTYPE> supportedColorFormats;
	CleanupClosePushL(supportedCodings);
	CleanupClosePushL(supportedColorFormats);
	
	COmxIL3GPDemuxerVideoOutputPort* self = new(ELeave) COmxIL3GPDemuxerVideoOutputPort(aProcessingFunction);
	CleanupStack::PushL(self);
	self->ConstructL(aCommonPortData, supportedCodings, supportedColorFormats);
	CleanupStack::Pop(self);
	
	CleanupStack::PopAndDestroy(2, &supportedCodings);
	
	return self;
	}

COmxIL3GPDemuxerVideoOutputPort::COmxIL3GPDemuxerVideoOutputPort(COmxIL3GPDemuxerProcessingFunction& aProcessingFunction) :
 iProcessingFunction(&aProcessingFunction)
	{
	}

void COmxIL3GPDemuxerVideoOutputPort::ConstructL(const TOmxILCommonPortData& aCommonPortData,
                                                const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedCodings,
                                                const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColorFormats)
	{
    COmxILVideoPort::ConstructL(aCommonPortData, aSupportedCodings, aSupportedColorFormats);
    GetParamPortDefinition().format.video.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
	// as there are only four items, do not require a sort order
	// if this list gets larger consider using binary search
    GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingAutoDetect);
	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingAVC);
	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingH263);
	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingMPEG4);

	GetSupportedColorFormats().AppendL(OMX_COLOR_FormatUnused);			
	}

COmxIL3GPDemuxerVideoOutputPort::~COmxIL3GPDemuxerVideoOutputPort()
	{
	}
	
OMX_ERRORTYPE COmxIL3GPDemuxerVideoOutputPort::GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const
	{
	OMX_ERRORTYPE omxRetValue = COmxILVideoPort::GetLocalOmxParamIndexes(aIndexArray);
	if (omxRetValue != OMX_ErrorNone)
		{
		return omxRetValue;
		}

	TInt err = aIndexArray.InsertInOrder(OMX_IndexParamNumAvailableStreams);
	// Note that index duplication is OK.
	if (err == KErrNone || err == KErrAlreadyExists)
		{
		err = aIndexArray.InsertInOrder(OMX_IndexParamActiveStream);
		}
				
	if (err != KErrNone && err != KErrAlreadyExists)
		{
		return OMX_ErrorInsufficientResources;
		}

	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxIL3GPDemuxerVideoOutputPort::GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILVideoPort::GetLocalOmxConfigIndexes(aIndexArray);
	}

OMX_ERRORTYPE COmxIL3GPDemuxerVideoOutputPort::GetParameter(OMX_INDEXTYPE aParamIndex,
                                                            TAny* apComponentParameterStructure) const
	{
	switch(aParamIndex)
		{		
		case OMX_IndexParamNumAvailableStreams:
			{
			OMX_PARAM_U32TYPE* u32Type = reinterpret_cast<OMX_PARAM_U32TYPE*>(apComponentParameterStructure);
			u32Type->nU32 = iProcessingFunction->NumAvailableStreams(COmxIL3GPDemuxer::EPortIndexVideoOutput);
			return OMX_ErrorNone;
			}

		case OMX_IndexParamActiveStream:
			{
			OMX_PARAM_U32TYPE* u32Type = reinterpret_cast<OMX_PARAM_U32TYPE*>(apComponentParameterStructure);
			u32Type->nU32 = iProcessingFunction->ActiveStream(COmxIL3GPDemuxer::EPortIndexVideoOutput);
			return OMX_ErrorNone;
			}
	
		default:
			{
			return COmxILVideoPort::GetParameter(aParamIndex, apComponentParameterStructure);
			}
		}
	}

OMX_ERRORTYPE COmxIL3GPDemuxerVideoOutputPort::SetParameter(OMX_INDEXTYPE aParamIndex,
                                                            const TAny* apComponentParameterStructure,
                                                            TBool& aUpdateProcessingFunction)
	{
	switch(aParamIndex)
		{		
		case OMX_IndexParamActiveStream:
			{
			const OMX_PARAM_U32TYPE* u32Type = reinterpret_cast<const OMX_PARAM_U32TYPE*>(apComponentParameterStructure);
			return iProcessingFunction->SetActiveStream(COmxIL3GPDemuxer::EPortIndexVideoOutput, u32Type->nU32);
			}

		default:
			{
			return COmxILVideoPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);
			}
		}
	}

OMX_ERRORTYPE COmxIL3GPDemuxerVideoOutputPort::SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
                                                                TBool& aUpdateProcessingFunction)
	{
	const OMX_VIDEO_PORTDEFINITIONTYPE& newVidDef = aPortDefinition.format.video;
	OMX_VIDEO_PORTDEFINITIONTYPE&  myVidDef = GetParamPortDefinition().format.video;

	if (aPortDefinition.nBufferCountActual > KMaxVideoBuffers)
		{
		return OMX_ErrorBadParameter;
		}

	if(newVidDef.eColorFormat != OMX_COLOR_FormatUnused)
		{
		return OMX_ErrorBadParameter;
		}

	// change to FindInUnsignedKeyOrder if many more formats are added
	// TODO this should allow OMX_VIDEO_CodingAutoDetect
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

TBool COmxIL3GPDemuxerVideoOutputPort::IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& /*aPortDefinition*/) const
	{
	// This function only gets called on input ports, so panic if this is ever called
	User::Panic(K3GPDemuxerVideoPortPanic, KErrGeneral);
	return EFalse; // to keep compiler happy
	}

void COmxIL3GPDemuxerVideoOutputPort::FormatDetected(const TSize& aFrameSize, const TVideoFormat& aFormat)
	{
	GetParamPortDefinition().format.video.nFrameWidth  = aFrameSize.iWidth;
	GetParamPortDefinition().format.video.nFrameHeight = aFrameSize.iHeight;
	GetParamPortDefinition().format.video.eCompressionFormat = aFormat.iCoding;
	// TODO deal with H263/AVC profile
	}

OMX_VIDEO_CODINGTYPE COmxIL3GPDemuxerVideoOutputPort::VideoFormat()
	{
	return GetParamPortDefinition().format.video.eCompressionFormat;
	}

/** Returns the number of buffers configured on this port. */
TInt COmxIL3GPDemuxerVideoOutputPort::BufferCount() const
	{
	return GetParamPortDefinition().nBufferCountActual;
	}
