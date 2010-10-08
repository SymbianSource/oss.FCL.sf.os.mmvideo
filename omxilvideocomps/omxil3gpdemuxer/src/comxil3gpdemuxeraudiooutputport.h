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

#ifndef COMXIL3GPDEMUXERAUDIOOUTPUTPORT_H
#define COMXIL3GPDEMUXERAUDIOOUTPUTPORT_H

#include <openmax/il/common/omxilaudioport.h>

class COmxIL3GPDemuxerProcessingFunction;
class TAudioFormat;

const TInt KMaxAudioBuffers = 10;

NONSHARABLE_CLASS(COmxIL3GPDemuxerAudioOutputPort) : public COmxILAudioPort
	{
public:
	static COmxIL3GPDemuxerAudioOutputPort* NewL(const TOmxILCommonPortData& aCommonPortData,
	                                             COmxIL3GPDemuxerProcessingFunction& aProcessingFunction);
	~COmxIL3GPDemuxerAudioOutputPort();

	void FormatDetected(const TAudioFormat& aFormat);
	OMX_AUDIO_CODINGTYPE AudioFormat();
	TInt BufferCount() const;

	// from COmxILAudioPort
	OMX_ERRORTYPE GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex,
	                           TAny* apComponentParameterStructure) const;
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex,
	                           const TAny* apComponentParameterStructure,
	                           TBool& aUpdateProcessingFunction);

protected:
	// from COmxILAudioPort
	OMX_ERRORTYPE SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
	                               TBool& aUpdateProcessingFunction);
	TBool IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition) const;

private:
	COmxIL3GPDemuxerAudioOutputPort(COmxIL3GPDemuxerProcessingFunction& aProcessingFunction);

	void ConstructL(const TOmxILCommonPortData& aCommonPortData);

private:
	RBuf8 iMimeTypeBuf;
	COmxIL3GPDemuxerProcessingFunction* iProcessingFunction;	// not owned
	};

#endif //COMXIL3GPDEMUXERAUDIOOUTPUTPORT_H
