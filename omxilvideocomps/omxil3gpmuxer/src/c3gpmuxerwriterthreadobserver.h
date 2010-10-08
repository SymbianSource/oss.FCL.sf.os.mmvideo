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
#ifndef C3GPMUXERWRITERTHREADOBSERVER_H_
#define C3GPMUXERWRITERTHREADOBSERVER_H_

#include "c3gpmuxer.h"
#include <openmax/il/common/omxilprocessingfunction.h>

//3GP Muxer writer thread observer class

class C3GPMuxerWriterThreadObserver : public CActive
{
public:

    static C3GPMuxerWriterThreadObserver* NewL(C3GPMuxer* aC3GPMuxer);
    ~C3GPMuxerWriterThreadObserver();
    void ConstructL();
    void IssueRequest();

protected:
    C3GPMuxerWriterThreadObserver(C3GPMuxer* aC3GPMuxer);
    
// From CActive class
    void RunL();
    void DoCancel();
    TInt RunError(TInt aError);
//data members
private:
    C3GPMuxer* iWriterThread;
};

#endif /*C3GPMUXERWRITERTHREADOBSERVER_H_*/
