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

#ifndef COMXIL3GPMUXERPROCESSINGFUNCTION_H
#define COMXIL3GPMUXERPROCESSINGFUNCTION_H

#include <openmax/il/common/omxilprocessingfunction.h>

// forward class declarations
class COmxIL3GPMuxerConfigManager;
class COmxIL3GPMuxerVideoInputPort;
class COmxIL3GPMuxerAudioInputPort;
class C3GPMuxer;
class C3GPMuxerWriterThreadObserver;

NONSHARABLE_CLASS(COmxIL3GPMuxerProcessingFunction) : public COmxILProcessingFunction
	{
public:
	static COmxIL3GPMuxerProcessingFunction* NewL(MOmxILCallbackNotificationIf& aCallbacks);
	~COmxIL3GPMuxerProcessingFunction();
	
	void SetConfigManager(COmxIL3GPMuxerConfigManager& aConfigManager);
	void SetVideoPort(COmxIL3GPMuxerVideoInputPort& aVideoPort);
	void SetAudioPort(COmxIL3GPMuxerAudioInputPort& aAudioPort);
	
	// from COmxILProcessingFunction
	OMX_ERRORTYPE StateTransitionIndication(TStateIndex aNewState);
	OMX_ERRORTYPE BufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection);
	OMX_ERRORTYPE ParamIndication(OMX_INDEXTYPE aParamIndex, const TAny* apComponentParameterStructure);
	OMX_ERRORTYPE ConfigIndication(OMX_INDEXTYPE aConfigIndex, const TAny* apComponentConfigStructure);
	OMX_ERRORTYPE BufferIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);
	OMX_BOOL BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);

private:
	COmxIL3GPMuxerProcessingFunction(MOmxILCallbackNotificationIf& aCallbacks);
	void ConstructL();

	OMX_ERRORTYPE SymbianErrorToOmx(TInt aError);
	
private:
	C3GPMuxer* iMuxer;
	COmxIL3GPMuxerConfigManager* iConfigManager;	// not owned
	COmxIL3GPMuxerVideoInputPort* iVideoPort;	// not owned
	COmxIL3GPMuxerAudioInputPort* iAudioPort;	// not owned
	C3GPMuxerWriterThreadObserver* iWriterThreadObserver;
	};

#endif //COMXIL3GPMUXERPROCESSINGFUNCTION_H
