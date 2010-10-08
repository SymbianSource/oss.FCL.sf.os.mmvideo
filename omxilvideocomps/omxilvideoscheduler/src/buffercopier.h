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

#ifndef BUFFERCOPIER_H_
#define BUFFERCOPIER_H_

#include <e32base.h>
#include <e32msgqueue.h>
#include <openmax/il/khronos/v1_x/OMX_Core.h>

class MBufferCopierIf
	{
public:
	/** Called when the buffer copier has transferred the data from an input buffer to an output buffer. */
	virtual void MbcBufferCopied(OMX_BUFFERHEADERTYPE* aInBuffer, OMX_BUFFERHEADERTYPE* aOutBuffer) = 0;
	/** Called when a buffer is flushed from the buffer copier. */
	virtual void MbcBufferFlushed(OMX_BUFFERHEADERTYPE* aBuffer, OMX_DIRTYPE aDirection) = 0;
	};

class CBufferCopier : public CActive
	{
public:
	static CBufferCopier* NewL(MBufferCopierIf& aCallback, TInt aMaxBuffers);
	~CBufferCopier();
	
	void DeliverBuffer(OMX_BUFFERHEADERTYPE* aBuffer, OMX_DIRTYPE aDirection);
	TBool RemoveBuffer(OMX_BUFFERHEADERTYPE* aBuffer, OMX_DIRTYPE aDirection);
	void FlushBuffers(OMX_DIRTYPE aDirection);
	
protected:
	void RunL();
	void DoCancel();
	
private:
	CBufferCopier(MBufferCopierIf& aCallback);
	void ConstructL(TInt aMaxBuffers);
	
	void CopyBuffer(OMX_BUFFERHEADERTYPE* aInBuffer, OMX_BUFFERHEADERTYPE* aOutBuffer);
	TBool RemoveFromQueue(RMsgQueue<OMX_BUFFERHEADERTYPE*>& aQueue, OMX_BUFFERHEADERTYPE* aBufferHeader);

	MBufferCopierIf& iCallback;
	RMsgQueue<OMX_BUFFERHEADERTYPE*> iInQueue;
	RMsgQueue<OMX_BUFFERHEADERTYPE*> iOutQueue;
	RMsgQueue<OMX_BUFFERHEADERTYPE*> iRemoveQueue;
	OMX_BUFFERHEADERTYPE* iInBuffer;
	};

#endif /*BUFFERCOPIER_H_*/
