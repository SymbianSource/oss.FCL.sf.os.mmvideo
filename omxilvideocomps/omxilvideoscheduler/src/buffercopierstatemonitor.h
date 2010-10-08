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
#ifndef BUFFERCOPIERSTATEMONITOR_H_
#define BUFFERCOPIERSTATEMONITOR_H_

#include <e32base.h>
#include <e32std.h>

class MBufferCopierIf;
class CBufferCopier;
class COmxILVideoScheduler;


class CBufferCopierStateMonitor : public CActive
    {
public:
    enum TPFState
        {
        ENull =0,
        EExecuting,
        EPause,
        ESubLoadedToIdle,
        ESubIdleToLoaded
        };
public:
    static CBufferCopierStateMonitor* NewL(MBufferCopierIf& aCallback, COmxILVideoScheduler& aComponent);
    ~CBufferCopierStateMonitor();
    TInt SetState(TPFState aState);
    CBufferCopier* BufferCopier(){return iBufferCopier;}
    
protected:
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);
    
private:
    CBufferCopierStateMonitor(MBufferCopierIf& aCallback, COmxILVideoScheduler& aComponent);
    void ConstructL();
    TInt DoSetState();
    TInt DoRequest();
    
private:
    MBufferCopierIf& iCallback;
    COmxILVideoScheduler& iComponent;
    TInt iMaxBuffers;
    CBufferCopier* iBufferCopier;
    TPFState iState;
    //Members to make the component thread safe.
    TThreadId iConstructionThreadId;
    TRequestStatus* iCallingStatus; // not owned
    TThreadId iCallingThreadId;
    };


#endif /* BUFFERCOPIERSTATEMONITOR_H_ */
