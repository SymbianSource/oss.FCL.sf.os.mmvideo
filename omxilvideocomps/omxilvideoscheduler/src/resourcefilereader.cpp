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

#include <barsread.h>
#include <videoscheduler.rsg>
#include "resourcefilereader.h"


CResourceFileReader* CResourceFileReader::NewL(const TDesC& aResourceFile)
	{
	CResourceFileReader* self = CResourceFileReader::NewLC(aResourceFile);
	CleanupStack::Pop(self);
	return self;
	}

CResourceFileReader* CResourceFileReader::NewLC(const TDesC& aResourceFile)
	{
	CResourceFileReader* self = new (ELeave) CResourceFileReader();
	CleanupStack::PushL(self);
	self->ConstructL(aResourceFile);
	return self;
	}

CResourceFileReader::CResourceFileReader()
	{
	}

void CResourceFileReader::ConstructL(const TDesC& aResourceFile)
	{
	User::LeaveIfError(iFs.Connect());
    iResourceFile.OpenL(iFs, aResourceFile);
	}

CResourceFileReader::~CResourceFileReader()
	{
	iResourceFile.Close();
	iFs.Close();
	}

void CResourceFileReader::ReadTimerInfoL(TInt64& aInitialRenderTime, TInt64& aMaxLateness)
	{
	HBufC8* res = iResourceFile.AllocReadLC(TIMER);
	TResourceReader reader;
	reader.SetBuffer(res);
	
	aInitialRenderTime = static_cast<TInt64>(reader.ReadInt32());
	aMaxLateness = static_cast<TInt64>(reader.ReadInt32());
	
	CleanupStack::PopAndDestroy(res);
	}
