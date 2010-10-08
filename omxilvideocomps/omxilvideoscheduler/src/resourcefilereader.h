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

#ifndef RESOURCEFILEREADER_H
#define RESOURCEFILEREADER_H

#include <e32base.h>
#include <barsc.h>




/** This class is responsible for reading the supplied resource file */
NONSHARABLE_CLASS(CResourceFileReader) : public CBase
	{
public:
	static CResourceFileReader* NewL(const TDesC& aResourceFile);
	static CResourceFileReader* NewLC(const TDesC& aResourceFile);
	~CResourceFileReader();
	
	void ReadTimerInfoL(TInt64& aInitialRenderTime, TInt64& aMaxLateness);

private:
	CResourceFileReader();
	void ConstructL(const TDesC& aResourceFile);

private:
	RFs iFs;
	RResourceFile iResourceFile;
	};

#endif // RESOURCEFILEREADER_H

