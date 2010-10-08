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


#include "endwaitao.h"
#include <e32debug.h>
#include "log.h"

CEndWaitAO* CEndWaitAO::NewL()
	{
	DEBUG_PRINTF(_L8("CEndWaitAO::NewL"));
	CEndWaitAO* self = new(ELeave) CEndWaitAO();
	return self;
	}

CEndWaitAO::CEndWaitAO():
CAsyncOneShot(EPriorityHigh)
	{
	}

CEndWaitAO::~CEndWaitAO()
	{
	DEBUG_PRINTF(_L8("CEndWaitAO::~CEndWaitAO"));
	Cancel();
	}

void CEndWaitAO::RunL()
	{
	DEBUG_PRINTF(_L8("CEndWaitAO::RunL"));
	CActiveScheduler::Stop();
	}

void CEndWaitAO::DoCancel()
	{
	DEBUG_PRINTF(_L8("CEndWaitAO::DoCancel"));
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrCancel);
	}
