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

#ifndef COMXIL3GPMUXERAUDIOINPUTPORT_H
#define COMXIL3GPMUXERAUDIOINPUTPORT_H

#include <openmax/il/common/omxilaudioport.h>

class COmxIL3GPMuxerProcessingFunction;

const TInt KMaxAudioBuffers = 20;

NONSHARABLE_CLASS(COmxIL3GPMuxerAudioInputPort) : public COmxILAudioPort
	{
public:
	static COmxIL3GPMuxerAudioInputPort* NewL(const TOmxILCommonPortData& aCommonPortData);
	
	~COmxIL3GPMuxerAudioInputPort();
	
	OMX_U32 GetAudioFrameRate() const;

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
	COmxIL3GPMuxerAudioInputPort();

	void ConstructL(const TOmxILCommonPortData& aCommonPortData);

private:
	RBuf8 iMimeTypeBuf;
	};

#endif /*COMXIL3GPMUXERAUDIOINPUTPORT_H*/
