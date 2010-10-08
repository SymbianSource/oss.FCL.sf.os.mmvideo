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
#include "c3gpmuxerwriterthreadobserver.h"

_LIT(KMuxerWriterThreadPanic,"MUXER-THREAD-PANIC");

C3GPMuxerWriterThreadObserver* C3GPMuxerWriterThreadObserver::NewL(C3GPMuxer* aC3GPMuxer)
    {
    C3GPMuxerWriterThreadObserver* self = new(ELeave)C3GPMuxerWriterThreadObserver(aC3GPMuxer);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

C3GPMuxerWriterThreadObserver::C3GPMuxerWriterThreadObserver(C3GPMuxer* aC3GPMuxer):
CActive(EPriorityStandard)
    {
        iWriterThread = aC3GPMuxer;
    }

void C3GPMuxerWriterThreadObserver::ConstructL()
    {
    CActiveScheduler::Add(this);
    }

void C3GPMuxerWriterThreadObserver::IssueRequest()
    {
    __ASSERT_ALWAYS(!IsActive(), User::Panic(KMuxerWriterThreadPanic, 0));
    iWriterThread->iThread.Logon(iStatus);
    SetActive();
    }

void C3GPMuxerWriterThreadObserver::RunL()
    {
    if(KErrDied==iStatus.Int())
        {  
            iWriterThread->HandleError(KErrDied);
        }
    }
void C3GPMuxerWriterThreadObserver::DoCancel()
    {
    //Do Nothing
    }
C3GPMuxerWriterThreadObserver::~C3GPMuxerWriterThreadObserver()
    {
    Cancel();
    }
TInt C3GPMuxerWriterThreadObserver::RunError(TInt)
    {
       return KErrNone;
    }
