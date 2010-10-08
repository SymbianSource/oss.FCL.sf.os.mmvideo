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

#ifndef COMXILCLOCKPROCESSINGFUNCTION_H
#define COMXILCLOCKPROCESSINGFUNCTION_H

#include <openmax/il/common/omxilprocessingfunction.h>
#include "clocksupervisor.h"
#include "clockthreadnotifier.h"

class COmxILClockComponent;

class COmxILClockProcessingFunction : public COmxILProcessingFunction
	{
public:
	static COmxILClockProcessingFunction* NewL(MOmxILCallbackNotificationIf& aCallbacks, COmxILClockComponent& aComponent);
	~COmxILClockProcessingFunction();

	OMX_ERRORTYPE ProduceRequest(OMX_INDEXTYPE aIdx, CClockSupervisor::TEntryPoint aEntryPoint, TAny* pComponentConfigStructure);

	// from COmxILProcessingFunction
	OMX_ERRORTYPE StateTransitionIndication(TStateIndex aNewState);
	OMX_ERRORTYPE BufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection);
	OMX_ERRORTYPE ParamIndication(OMX_INDEXTYPE aParamIndex, const TAny* apComponentParameterStructure);
	OMX_ERRORTYPE ConfigIndication(OMX_INDEXTYPE aConfigIndex, const TAny* apComponentConfigStructure);
	OMX_ERRORTYPE BufferIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);
	OMX_BOOL BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);

	// used by CClockSupervisor
	OMX_BUFFERHEADERTYPE* AcquireBuffer(TInt aPortIndex);
	void SendBuffer(OMX_BUFFERHEADERTYPE* aBuffer);
	TBool PortEnabled(TInt aPortIndex) const;
	TBool IsExecuting() const;
	void InvalidateComponent();
	
	
private:
	COmxILClockProcessingFunction(MOmxILCallbackNotificationIf& aCallbacks, COmxILClockComponent& aComponent);
	void ConstructL();
	void DoBufferFlushingIndication(TUint32 aPortIndex);

private:
	CClockSupervisor* iClock;
	CClockThreadNotifier* iThreadNotifier;
	COmxILClockComponent& iComponent;
	RPointerArray<CCirBuf<OMX_BUFFERHEADERTYPE*> > iBufferQueues;
	RMutex iBufferMutex;
	TBool iExecuting;
  	};

#endif //COMXILCLOCKPROCESSINGFUNCTION_H
