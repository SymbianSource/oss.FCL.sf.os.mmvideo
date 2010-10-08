/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "comxil3gpdemuxerrequesthelper.h"
#include "comxil3gpdemuxerprocessingfunction.h"
#include "comxil3gpdemuxer.h"

COmxIL3GPDemuxerRequestHelper* COmxIL3GPDemuxerRequestHelper::NewL(COmxILProcessingFunction* aProcessingFunction)
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::NewL"));

    COmxIL3GPDemuxerRequestHelper* self = new (ELeave) COmxIL3GPDemuxerRequestHelper(aProcessingFunction);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

COmxIL3GPDemuxerRequestHelper::COmxIL3GPDemuxerRequestHelper(COmxILProcessingFunction* aProcessingFunction):
    CActive(EPriorityStandard),
    iProcessingFunction(aProcessingFunction),
    iFunctionCode(EC3GPDemuxerFunctionCodeNone)
    {
    CActiveScheduler::Add(this);
    }

void COmxIL3GPDemuxerRequestHelper::ConstructL()
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::ConstructL"));

    RThread me;
    iConstructionThreadId = me.Id();
    
    iStatus = KRequestPending;
    SetActive();
    }

COmxIL3GPDemuxerRequestHelper::~COmxIL3GPDemuxerRequestHelper()
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::~COmxIL3GPDemuxerRequestHelper"));
    
    Cancel(); // safe as takes place in core thread which has Active Scheduler running. 
    }

void COmxIL3GPDemuxerRequestHelper::RunL()
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::RunL"));

    __ASSERT_DEBUG(iStatus == KErrNone, User::Panic(_L("Demuxer COmxIL3GPDemuxerRequestHelper::RunL"), KErrUnknown));

    iStatus = KRequestPending;
    SetActive();
    
    // list of requests that need to be synchronised with AS
    switch(iFunctionCode)
        {
        case EC3GPDemuxerFunctionCodeStateTransitionIndication:
            DoStateTransitionIndication();
            break;
            
        case EC3GPDemuxerFunctionCodeBufferFlushingIndication:
            DoBufferFlushingIndication();
            break;
                    
        case EC3GPDemuxerFunctionCodeBufferRemovalIndication:
            DoBufferRemovalIndication();
            break;
                    
        default:
            // should never reach here
            User::Panic(_L("COmxIL3GPDemuxerRequestHelper"), KErrArgument);
        }
  
     TRequestStatus *status = iCallingStatus;
     RThread callingThread;
     callingThread.Open(iCallingThreadId);
     callingThread.RequestComplete(status, KErrNone);
     callingThread.Close();
    }

void COmxIL3GPDemuxerRequestHelper::DoCancel()
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::DoCancel"));
    
    TRequestStatus* status = &iStatus;
    User::RequestComplete(status, KErrCancel);
    }

OMX_ERRORTYPE COmxIL3GPDemuxerRequestHelper::StateTransitionIndication(TStateIndex aNewState)
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::StateTransitionIndication"));

    iNewState = aNewState;
    
    RThread me;
    if(me.Id() != iConstructionThreadId)
        {
        // in different thread to that which 'this' was constructed
        iFunctionCode = EC3GPDemuxerFunctionCodeStateTransitionIndication;
        DoRequest();
        }
    else
        {
        // in same thread to that which 'this' was constructed therefore
        // active scheduler must exist and following call is safe
        DoStateTransitionIndication();
        }
    
    return iOMXError;
    }

OMX_ERRORTYPE COmxIL3GPDemuxerRequestHelper::BufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection)
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::BufferFlushingIndication"));
    
    iPortIndex = aPortIndex;
    iDirection = aDirection;
    
    RThread me;
    if(me.Id() != iConstructionThreadId)
        {
        // in different thread to that which 'this' was constructed
        iFunctionCode = EC3GPDemuxerFunctionCodeBufferFlushingIndication;
        DoRequest();
        }
    else
        {
        // in same thread to that which 'this' was constructed therefore
        // active scheduler must exist and following call is safe
        DoBufferFlushingIndication();
        }
    
    return iOMXError;
    }

OMX_BOOL COmxIL3GPDemuxerRequestHelper::BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection)
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::BufferRemovalIndication"));
    
    ipBufferHeader = apBufferHeader;
    iDirection = aDirection;
    
    RThread me;
    if(me.Id() != iConstructionThreadId)
        {
        // in different thread to that which 'this' was constructed
        iFunctionCode = EC3GPDemuxerFunctionCodeBufferRemovalIndication;
        DoRequest();
        }
    else
        {
        // in same thread to that which 'this' was constructed therefore
        // active scheduler must exist and following call is safe
        DoBufferRemovalIndication();
        }
    
    return iOMXBool;
    }

void COmxIL3GPDemuxerRequestHelper::DoRequest()
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::DoRequest"));
    
    RThread me;
    iCallingThreadId = me.Id();
    TRequestStatus requestStatus = KRequestPending;
    iCallingStatus = &requestStatus;
    
    // send request to active scheduler thread
    RThread schedulerThread;
    schedulerThread.Open(iConstructionThreadId);
    
    TRequestStatus* schedulerRequest = &iStatus;
    schedulerThread.RequestComplete(schedulerRequest, KErrNone);
    schedulerThread.Close();
    
    // block until request completes
    User::WaitForRequest(requestStatus);
    
    iFunctionCode = EC3GPDemuxerFunctionCodeNone;
    }

void COmxIL3GPDemuxerRequestHelper::DoStateTransitionIndication()
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::DoStateTransitionIndication"));

    COmxIL3GPDemuxerProcessingFunction* processingFunction = 
             dynamic_cast<COmxIL3GPDemuxerProcessingFunction*>(iProcessingFunction);
    
    iOMXError = processingFunction->DoStateTransitionIndication(iNewState);
    }

void COmxIL3GPDemuxerRequestHelper::DoBufferFlushingIndication()
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::DoBufferFlushingIndication"));
    
    COmxIL3GPDemuxerProcessingFunction* processingFunction = 
             dynamic_cast<COmxIL3GPDemuxerProcessingFunction*>(iProcessingFunction);
    
    iOMXError = processingFunction->DoBufferFlushingIndication(iPortIndex, iDirection);
    }

void COmxIL3GPDemuxerRequestHelper::DoBufferRemovalIndication()
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerRequestHelper::DoBufferRemovalIndication"));
    
    COmxIL3GPDemuxerProcessingFunction* processingFunction = 
             dynamic_cast<COmxIL3GPDemuxerProcessingFunction*>(iProcessingFunction);
    
    iOMXBool = processingFunction->DoBufferRemovalIndication(ipBufferHeader, iDirection);
    }

