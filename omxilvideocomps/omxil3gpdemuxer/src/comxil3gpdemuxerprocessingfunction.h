/*
* Copyright (c) 2008 - 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COMXIL3GPDEMUXERPROCESSINGFUNCTION_H
#define COMXIL3GPDEMUXERPROCESSINGFUNCTION_H

#include <openmax/il/common/omxilprocessingfunction.h>
#include "comxil3gpdemuxer.h"
#include "comxil3gpdemuxerrequesthelper.h"

class COmxIL3GPDemuxerConfigManager;
class COmxIL3GPDemuxerVideoOutputPort;
class COmxIL3GPDemuxerAudioOutputPort;
class C3GPDemuxer;

NONSHARABLE_CLASS(COmxIL3GPDemuxerProcessingFunction) : public COmxILProcessingFunction
	{
public:
	static COmxIL3GPDemuxerProcessingFunction* NewL(MOmxILCallbackNotificationIf& aCallbacks);
	~COmxIL3GPDemuxerProcessingFunction();

	void SetConfigManager(COmxIL3GPDemuxerConfigManager& aConfigManager);
	void SetVideoPort(COmxIL3GPDemuxerVideoOutputPort& aVideoPort);
	void SetAudioPort(COmxIL3GPDemuxerAudioOutputPort& aAudioPort);
	OMX_ERRORTYPE StateTransitionIndication(TStateIndex aNewState);
	OMX_ERRORTYPE BufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection);
	OMX_ERRORTYPE ParamIndication(OMX_INDEXTYPE aParamIndex, const TAny* apComponentParameterStructure);
	OMX_ERRORTYPE ConfigIndication(OMX_INDEXTYPE aConfigIndex, const TAny* apComponentConfigStructure);
	OMX_ERRORTYPE BufferIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);
	OMX_BOOL BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);

	OMX_U32 NumAvailableStreams(COmxIL3GPDemuxer::TPortIndex aPortType);
	OMX_U32 ActiveStream(COmxIL3GPDemuxer::TPortIndex aPortType);
	OMX_ERRORTYPE SetActiveStream(COmxIL3GPDemuxer::TPortIndex aPortType, OMX_U32 aActiveStream);

private:
	COmxIL3GPDemuxerProcessingFunction(MOmxILCallbackNotificationIf& aCallbacks);
	void ConstructL();
	
    OMX_ERRORTYPE DoStateTransitionIndication(TStateIndex aNewState);
    OMX_ERRORTYPE DoBufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection);
    OMX_BOOL DoBufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);
    
	static TInt StreamDetectCallBack(TAny* aPtr);
	void DoStreamDetect();
	
private:
	C3GPDemuxer* iDemuxer;
	COmxIL3GPDemuxerConfigManager* iConfigManager;	// not owned
	COmxIL3GPDemuxerVideoOutputPort* iVideoPort;	// not owned
	COmxIL3GPDemuxerAudioOutputPort* iAudioPort; 	// not owned
	CAsyncCallBack* iStreamDetectCallback;

	TBool iStreamsDetected;
	TBool iExecuting;
	
	COmxIL3GPDemuxerRequestHelper* iRequestHelper;
	friend class COmxIL3GPDemuxerRequestHelper;
	};

#endif //COMXIL3GPDEMUXERPROCESSINGFUNCTION_H
