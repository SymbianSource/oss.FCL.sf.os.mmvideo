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

#ifndef OMXIL3GPDEMUXERPANIC_H
#define OMXIL3GPDEMUXERPANIC_H

enum TPanicCode
	{
	EProcessThisBufferLNoBufferQueue = 0,
	ERemoveBufferInvalidDirection = 1,
	ERemoveBufferInvalidPort = 2,
	EReceiveQueuedBuffersAppendFailed = 3,
	ECreateBufferQueueLZeroBufferCount = 4,
	ECheckBufferQueueSizeLZeroBufferCount = 5
	};

void Panic(TPanicCode aCode);

#endif //OMXIL3GPDEMUXERPANIC_H
