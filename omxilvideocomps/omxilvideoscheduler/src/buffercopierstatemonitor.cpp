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
#include "buffercopier.h"
#include "comxilvideoscheduler.h"
#include "buffercopierstatemonitor.h"
#include "log.h"

CBufferCopierStateMonitor* CBufferCopierStateMonitor::NewL(MBufferCopierIf& aCallback, COmxILVideoScheduler& aComponent)
    {
    CBufferCopierStateMonitor* self = new (ELeave) CBufferCopierStateMonitor(aCallback, aComponent);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CBufferCopierStateMonitor::CBufferCopierStateMonitor(MBufferCopierIf& aCallback, COmxILVideoScheduler& aComponent) :
CActive(EPriorityNormal),
iCallback(aCallback),
iComponent(aComponent),
iState(ENull)
    {
    CActiveScheduler::Add(this);
    }

CBufferCopierStateMonitor::~CBufferCopierStateMonitor()
    {
    Cancel();
    delete iBufferCopier;
    }

void CBufferCopierStateMonitor::ConstructL()
    {
    RThread me;
    iConstructionThreadId = me.Id();
    
    iStatus = KRequestPending;
    SetActive();
    }

void CBufferCopierStateMonitor::RunL()
    {
    iStatus = KRequestPending;
    SetActive();

    TInt err = DoSetState();

    TRequestStatus *status = iCallingStatus;
    RThread callingThread;
    callingThread.Open(iCallingThreadId);
    callingThread.RequestComplete(status, err);
    callingThread.Close();
    }

void CBufferCopierStateMonitor::DoCancel()
    {
    TRequestStatus* pStat = &iStatus;
    User::RequestComplete(pStat, KErrCancel);
    }

TInt CBufferCopierStateMonitor::RunError(TInt aError)
    {
    DEBUG_PRINTF2(_L8("CBufferCopierStateMonitor::RunError %d"), aError);
    aError=KErrNone; //Do not want to panic the server!
    return aError; 
    }

TInt CBufferCopierStateMonitor::SetState(TPFState aState)
    {
    DEBUG_PRINTF(_L8("CBufferCopierStateMonitor::SetState"));

    TInt err = KErrNone;
    iState = aState;
    
    RThread me;
    if(me.Id() != iConstructionThreadId)
        {
        // in different thread to that which 'this' was constructed
        err = DoRequest();
        }
    else
        {
        // in same thread to that which 'this' was constructed therefore
        // active scheduler must exist and following call is safe
        err = DoSetState();
        }

    return err;
    }

TInt CBufferCopierStateMonitor::DoSetState()
    {
    DEBUG_PRINTF(_L8("CBufferCopierStateMonitor::DoSetState"));

    TInt err = KErrNone;

    switch(iState)
        {
        case ESubLoadedToIdle:
            iMaxBuffers = iComponent.BufferCount();
            delete iBufferCopier;
            iBufferCopier = NULL;
            TRAP(err, iBufferCopier = CBufferCopier::NewL(iCallback, iMaxBuffers));
            break;
        case ESubIdleToLoaded:
            delete iBufferCopier;
            iBufferCopier = NULL;
            break;
        default:
            break;
        };

    return err;
    }

TInt CBufferCopierStateMonitor::DoRequest()
    {
    DEBUG_PRINTF(_L8("CBufferCopierStateMonitor::DoRequest"));

    RThread me;
    iCallingThreadId = me.Id();
    TRequestStatus requestStatus = KRequestPending;
    iCallingStatus = &requestStatus;

    // send request to active scheduler thread
    RThread schedulerThread;
    TInt error = schedulerThread.Open(iConstructionThreadId);
    if(error != KErrNone)
        {
        return error;
        }

    TRequestStatus* schedulerRequest = &iStatus;
    schedulerThread.RequestComplete(schedulerRequest, KErrNone);
    schedulerThread.Close();

    // block until request completes
    User::WaitForRequest(requestStatus);

    return requestStatus.Int();
    }
