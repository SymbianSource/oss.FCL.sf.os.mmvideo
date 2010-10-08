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


#include "clockthreadnotifier.h"

_LIT(KClockThreadPanicMsg, "CLOCK-THREAD-NOTIFIER");

CClockThreadNotifier* CClockThreadNotifier::NewL(CClockSupervisor* aClockSupervisor)
    {
    CClockThreadNotifier* self = new(ELeave)CClockThreadNotifier(aClockSupervisor);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CClockThreadNotifier::CClockThreadNotifier(CClockSupervisor* aClockSupervisor):CActive(CActive::EPriorityStandard)
    {
        iClockSupervisor = aClockSupervisor;
    }
void CClockThreadNotifier::ConstructL()
    {
        CActiveScheduler::Add(this);
    }
void CClockThreadNotifier::RunL()
    {
 
      iClockSupervisor->ReportClockThreadPanic();
 
    }
void CClockThreadNotifier::DoCancel()
    {
     // Do Nothing
    }
CClockThreadNotifier::~CClockThreadNotifier()
    {
        Cancel();
    }
TInt CClockThreadNotifier::RunError(TInt)
    {
       return KErrNone;
    }
void CClockThreadNotifier::IssueRequest()
    {
    __ASSERT_ALWAYS(!IsActive(), User::Panic(KClockThreadPanicMsg, 0));
    iClockSupervisor->iThread.Logon(iStatus);
    SetActive();
    }
