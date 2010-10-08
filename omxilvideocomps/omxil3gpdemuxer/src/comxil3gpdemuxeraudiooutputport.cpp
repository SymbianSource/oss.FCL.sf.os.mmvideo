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

#include "log.h"
#include "comxil3gpdemuxeraudiooutputport.h"
#include "comxil3gpdemuxerprocessingfunction.h"
#include "taudioformat.h"

// TODO: Possibly add other mime types, for now only AAC handled
_LIT8(KMimeTypeAudioAac, "audio/aac");
_LIT(K3GPDemuxerAudioPortPanic, "COmxIL3GPDemuxerVideoOutputPort");


COmxIL3GPDemuxerAudioOutputPort* COmxIL3GPDemuxerAudioOutputPort::NewL(const TOmxILCommonPortData& aCommonPortData,
                                                                       COmxIL3GPDemuxerProcessingFunction& aProcessingFunction)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerAudioOutputPort::NewL"));
	// TODO this array must be left empty, to be removed from the audio port constructor

	COmxIL3GPDemuxerAudioOutputPort* self = new(ELeave) COmxIL3GPDemuxerAudioOutputPort(aProcessingFunction);
	CleanupStack::PushL(self);
	self->ConstructL(aCommonPortData);
	CleanupStack::Pop(self);
	return self;
	}

void COmxIL3GPDemuxerAudioOutputPort::ConstructL(const TOmxILCommonPortData& aCommonPortData)
	{
    RArray<OMX_AUDIO_CODINGTYPE> supportedCodings;
    CleanupClosePushL(supportedCodings);
    supportedCodings.AppendL(OMX_AUDIO_CodingAAC);
    COmxILAudioPort::ConstructL(aCommonPortData, supportedCodings);
    CleanupStack::PopAndDestroy(&supportedCodings);
    
    OMX_PARAM_PORTDEFINITIONTYPE& paramPortDefinition=GetParamPortDefinition();
	// We have to finish with iParamPortDefinition
    paramPortDefinition.eDomain = OMX_PortDomainAudio;
    paramPortDefinition.format.audio.pNativeRender = 0;

	// TODO: Possible add in the future other mime types that can be handled by
	// this audio port... for now only AAC Check
	// COmxILAudioPort::iParamAudioPortFormat.nIndex and use
	// COmxILAudioPort::iSupportedAudioFormats[iParamAudioPortFormat.nIndex]

	iMimeTypeBuf.CreateL(KMimeTypeAudioAac(), KMimeTypeAudioAac().Length() + 1);

	TUint8* pTUint = const_cast<TUint8*>(iMimeTypeBuf.PtrZ());
	paramPortDefinition.format.audio.cMIMEType = reinterpret_cast<OMX_STRING>(pTUint);

	paramPortDefinition.format.audio.bFlagErrorConcealment = OMX_FALSE;
	paramPortDefinition.format.audio.eEncoding = OMX_AUDIO_CodingAAC;
	}

COmxIL3GPDemuxerAudioOutputPort::COmxIL3GPDemuxerAudioOutputPort(COmxIL3GPDemuxerProcessingFunction& aProcessingFunction) 
:	iProcessingFunction(&aProcessingFunction)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerAudioOutputPort::COmxIL3GPDemuxerAudioOutputPort"));
	}

COmxIL3GPDemuxerAudioOutputPort::~COmxIL3GPDemuxerAudioOutputPort()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerAudioOutputPort::~COmxIL3GPDemuxerAudioOutputPort"));
	iMimeTypeBuf.Close();
	}

OMX_ERRORTYPE COmxIL3GPDemuxerAudioOutputPort::GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerAudioOutputPort::GetLocalOmxParamIndexes"));
	OMX_ERRORTYPE omxRetValue = COmxILAudioPort::GetLocalOmxParamIndexes(aIndexArray);		
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

OMX_ERRORTYPE COmxIL3GPDemuxerAudioOutputPort::GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILAudioPort::GetLocalOmxConfigIndexes(aIndexArray);
	}

OMX_ERRORTYPE COmxIL3GPDemuxerAudioOutputPort::GetParameter(OMX_INDEXTYPE aParamIndex,
                                                            TAny* apComponentParameterStructure) const
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerAudioOutputPort::GetParameter"));
	switch(aParamIndex)
		{		
		case OMX_IndexParamNumAvailableStreams:
			{
			OMX_PARAM_U32TYPE* u32Type = reinterpret_cast<OMX_PARAM_U32TYPE*>(apComponentParameterStructure);
			u32Type->nU32 = iProcessingFunction->NumAvailableStreams(COmxIL3GPDemuxer::EPortIndexAudioOutput);
			return OMX_ErrorNone;
			}

		case OMX_IndexParamActiveStream:
			{
			OMX_PARAM_U32TYPE* u32Type = reinterpret_cast<OMX_PARAM_U32TYPE*>(apComponentParameterStructure);
			u32Type->nU32 = iProcessingFunction->ActiveStream(COmxIL3GPDemuxer::EPortIndexAudioOutput);
			return OMX_ErrorNone;
			}

		default:
			{
			return COmxILAudioPort::GetParameter(aParamIndex, apComponentParameterStructure);
			}
		}
	}

OMX_ERRORTYPE COmxIL3GPDemuxerAudioOutputPort::SetParameter(OMX_INDEXTYPE aParamIndex,
                                                            const TAny* apComponentParameterStructure,
                                                            TBool& aUpdateProcessingFunction)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerAudioOutputPort::SetParameter"));
	switch(aParamIndex)
		{		
		case OMX_IndexParamActiveStream:
			{
			const OMX_PARAM_U32TYPE* u32Type = reinterpret_cast<const OMX_PARAM_U32TYPE*>(apComponentParameterStructure);
			return iProcessingFunction->SetActiveStream(COmxIL3GPDemuxer::EPortIndexAudioOutput, u32Type->nU32);
			}

		default:
			{
			return COmxILAudioPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);
			}
		}
	}

OMX_ERRORTYPE COmxIL3GPDemuxerAudioOutputPort::SetFormatInPortDefinition(
	const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
	TBool& /*aUpdateProcessingFunction*/)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerAudioOutputPort::SetFormatInPortDefinition"));

	if (aPortDefinition.nBufferCountActual > KMaxAudioBuffers)
		{
		return OMX_ErrorBadParameter;
		}

	GetParamPortDefinition().format.audio = aPortDefinition.format.audio;
	
	return OMX_ErrorNone;
	}

TBool COmxIL3GPDemuxerAudioOutputPort::IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& /*aPortDefinition*/) const
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerAudioOutputPort::IsTunnelledPortCompatible"));

	// This function only gets called on input ports, so panic if this is ever called
	User::Panic(K3GPDemuxerAudioPortPanic, KErrGeneral);
	return EFalse; // to keep compiler happy
	}

void COmxIL3GPDemuxerAudioOutputPort::FormatDetected(const TAudioFormat& aFormat)
	{
	GetParamPortDefinition().format.audio.eEncoding = aFormat.iCoding;
	}

OMX_AUDIO_CODINGTYPE COmxIL3GPDemuxerAudioOutputPort::AudioFormat()
	{
	return GetParamPortDefinition().format.audio.eEncoding;
	}

/** Returns the number of buffers configured on this port. */
TInt COmxIL3GPDemuxerAudioOutputPort::BufferCount() const
	{
	return GetParamPortDefinition().nBufferCountActual;
	}
