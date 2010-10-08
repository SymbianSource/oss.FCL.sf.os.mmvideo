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

#ifndef CLOCKPANICS_H_
#define CLOCKPANICS_H_

enum TClockPanicCode
	{
	EBufferUnderflow = 0,				// ran out of buffers with which to issue notifications
	/*ERequestOverflow = 1,				// NO LONGER USED. previously, too many media time requests to add to pending queue*/
	EBadAlignment = 2,					// unexpected pointer alignment
	EMutexUnheld = 3,					// the mutex was expected to be held but it was not
	ERequestQueueCorrupt = 4,			// request queue structure in an inconsistent state
	ERequestQueueUnordered = 5,			// request queue trigger times not in ascending order
	EBufferQueueOverflow = 6,			// error adding buffer to queue. this shouldn't happen
										// as queue is preallocated with total buffer count
	ECompatibilityCheckOnOutput = 7,	// IsTunnelledPortCompatible() was called on output port which shouldn't happen
	EBufferFlushingInvalidPort = 8,		// Invalid port index passed to BufferFlushingIndication() routine
	ECallbackManagerBufferError = 9,	// The callback manager returned an error during BufferDoneNotification,
	                                	// this is not expected since the callee component should handle an error internally
	ECallbackManagerEventError = 10,	// The callback manager returned an error during EventNotification,
	                                	// this is not expected since the client should handle an error internally
	
	EUndefinedErrorCode = 1000			// received an error code that isn't translated (debug builds only)
	};

void Panic(TClockPanicCode aCode);

#endif /*CLOCKPANICS_H_*/
