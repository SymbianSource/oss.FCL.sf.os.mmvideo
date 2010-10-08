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


#ifndef ENDWAITAO_H_
#define ENDWAITAO_H_

#include <e32base.h>

class CEndWaitAO : public CAsyncOneShot
	{
public:
	static CEndWaitAO* NewL();
	~CEndWaitAO();
	
protected:
	void RunL();
	void DoCancel();
	
private:
	CEndWaitAO();
	
	RThread iSchedulerThread;
	};

#endif /*ENDWAITAO_H_*/
