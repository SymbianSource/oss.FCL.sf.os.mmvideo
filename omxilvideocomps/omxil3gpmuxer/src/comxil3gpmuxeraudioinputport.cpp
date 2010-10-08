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

#include "comxil3gpmuxeraudioinputport.h"
#include "log.h"

// TODO: Possibly add other mime types, for now only AAC handled
_LIT8(KMimeTypeAudioAac, "audio/aac");

COmxIL3GPMuxerAudioInputPort* COmxIL3GPMuxerAudioInputPort::NewL(const TOmxILCommonPortData& aCommonPortData)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerAudioInputPort::NewL"));

	COmxIL3GPMuxerAudioInputPort* self = new(ELeave) COmxIL3GPMuxerAudioInputPort();
	CleanupStack::PushL(self);
	self->ConstructL(aCommonPortData);
	CleanupStack::Pop(self);
	return self;
	}

void COmxIL3GPMuxerAudioInputPort::ConstructL(const TOmxILCommonPortData& aCommonPortData)
	{
	RArray<OMX_AUDIO_CODINGTYPE> supportedCodings;
	CleanupClosePushL(supportedCodings);
	supportedCodings.AppendL(OMX_AUDIO_CodingAAC);
	COmxILAudioPort::ConstructL(aCommonPortData, supportedCodings);
	CleanupStack::PopAndDestroy(&supportedCodings);
	
	// We have to finish with iParamPortDefinition
	GetParamPortDefinition().eDomain = OMX_PortDomainAudio;
	GetParamPortDefinition().format.audio.pNativeRender = 0;

	// TODO: Possible add in the future other mime types that can be handled by
	// this audio port... for now only AAC Check
	// COmxILAudioPort::iParamAudioPortFormat.nIndex and use
	// COmxILAudioPort::iSupportedAudioFormats[iParamAudioPortFormat.nIndex]

	iMimeTypeBuf.CreateL(KMimeTypeAudioAac(), KMimeTypeAudioAac().Length() + 1);

	TUint8* pTUint = const_cast<TUint8*>(iMimeTypeBuf.PtrZ());
	GetParamPortDefinition().format.audio.cMIMEType = reinterpret_cast<OMX_STRING>(pTUint);

	GetParamPortDefinition().format.audio.bFlagErrorConcealment = OMX_FALSE;
	GetParamPortDefinition().format.audio.eEncoding = OMX_AUDIO_CodingAAC;	
	}

COmxIL3GPMuxerAudioInputPort::COmxIL3GPMuxerAudioInputPort()
	{
	}
	
COmxIL3GPMuxerAudioInputPort::~COmxIL3GPMuxerAudioInputPort()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerAudioInputPort::~COmxIL3GPMuxerAudioInputPort"));
	iMimeTypeBuf.Close();
	}

OMX_U32 COmxIL3GPMuxerAudioInputPort::GetAudioFrameRate() const
	{
	// TODO return nSamplingRate
	return 0;
	}

OMX_ERRORTYPE COmxIL3GPMuxerAudioInputPort::GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILAudioPort::GetLocalOmxParamIndexes(aIndexArray);
	}

OMX_ERRORTYPE COmxIL3GPMuxerAudioInputPort::GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILAudioPort::GetLocalOmxConfigIndexes(aIndexArray);
	}

OMX_ERRORTYPE COmxIL3GPMuxerAudioInputPort::GetParameter(OMX_INDEXTYPE aParamIndex, TAny* apComponentParameterStructure) const
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerAudioInputPort::GetParameter : aParamIndex[%u]"), aParamIndex);
	return COmxILAudioPort::GetParameter(aParamIndex, apComponentParameterStructure);
	}

OMX_ERRORTYPE COmxIL3GPMuxerAudioInputPort::SetParameter(OMX_INDEXTYPE aParamIndex,
                           const TAny* apComponentParameterStructure,
                           TBool& aUpdateProcessingFunction)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerAudioInputPort::SetParameter : aParamIndex[%u]"), aParamIndex);
	return COmxILAudioPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);
	}

OMX_ERRORTYPE COmxIL3GPMuxerAudioInputPort::SetFormatInPortDefinition(
	const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
	TBool& /*aUpdateProcessingFunction*/)
	{
	if (aPortDefinition.nBufferCountActual > KMaxAudioBuffers)
		{
		return OMX_ErrorBadParameter;
		}

	GetParamPortDefinition().format.audio = aPortDefinition.format.audio;
	
	return OMX_ErrorNone;
	}

TBool COmxIL3GPMuxerAudioInputPort::IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& /*aPortDefinition*/) const
	{
	// TODO (The base class should do this checking) 
	return ETrue;
	}
