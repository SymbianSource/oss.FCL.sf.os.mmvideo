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

#include "buffercopier.h"

_LIT(KBufferCopierPanic, "CBufferCopier");

CBufferCopier* CBufferCopier::NewL(MBufferCopierIf& aCallback, TInt aMaxBuffers)
	{
	CBufferCopier* self = new(ELeave) CBufferCopier(aCallback);
	CleanupStack::PushL(self);
	self->ConstructL(aMaxBuffers);
	CleanupStack::Pop(self);
	return self;
	}

CBufferCopier::CBufferCopier(MBufferCopierIf& aCallback):
CActive(EPriorityNormal),
iCallback(aCallback)
	{
	CActiveScheduler::Add(this);
	}

void CBufferCopier::ConstructL(TInt aMaxBuffers)
	{
	User::LeaveIfError(iInQueue.CreateLocal(aMaxBuffers));
	User::LeaveIfError(iOutQueue.CreateLocal(aMaxBuffers));
	User::LeaveIfError(iRemoveQueue.CreateLocal(aMaxBuffers));
	SetActive();
	iInQueue.NotifyDataAvailable(iStatus);
	}

CBufferCopier::~CBufferCopier()
	{
	Cancel();
	iInQueue.Close();
	iOutQueue.Close();
	iRemoveQueue.Close();
	}
	
void CBufferCopier::DeliverBuffer(OMX_BUFFERHEADERTYPE* aBuffer, OMX_DIRTYPE aDirection)
	{
	RMsgQueue<OMX_BUFFERHEADERTYPE*>& queue = aDirection == OMX_DirInput ? iInQueue : iOutQueue;
	TInt err = queue.Send(aBuffer);
	// do not expect error as maximum buffer count already allocated
	if(err != KErrNone)
		{
		User::Panic(KBufferCopierPanic, err);
		}
	}
	
void CBufferCopier::RunL()
	{
	if(iInBuffer == NULL)
		{
		// ignore error, if KErrUnderflow then we go back to waiting on iInQueue
		iInQueue.Receive(iInBuffer);
		}
	else
		{
		OMX_BUFFERHEADERTYPE* outBuffer = NULL;
		TInt err = iOutQueue.Receive(outBuffer);
		// if KErrUnderflow then we go back to waiting on iOutQueue
		if(err == KErrNone)
			{
			CopyBuffer(iInBuffer, outBuffer);
			iInBuffer = NULL;
			}
		}
	
	SetActive();
	if(iInBuffer == NULL)
		{
		iInQueue.NotifyDataAvailable(iStatus);
		}
	else
		{
		iOutQueue.NotifyDataAvailable(iStatus);
		}
	}

void CBufferCopier::DoCancel()
	{
	iInQueue.CancelDataAvailable();
	iOutQueue.CancelDataAvailable();
	}

void CBufferCopier::CopyBuffer(OMX_BUFFERHEADERTYPE* aInBuffer, OMX_BUFFERHEADERTYPE* aOutBuffer)
	{
	aOutBuffer->nOffset = 0;
	TPtr8 desPtr(aOutBuffer->pBuffer, aOutBuffer->nAllocLen);
 	desPtr.Copy(aInBuffer->pBuffer + aInBuffer->nOffset, aInBuffer->nFilledLen);
		
	aOutBuffer->nFilledLen			 = aInBuffer->nFilledLen;
	aOutBuffer->hMarkTargetComponent = aInBuffer->hMarkTargetComponent;
	aOutBuffer->pMarkData			 = aInBuffer->pMarkData;
	aOutBuffer->nTickCount			 = aInBuffer->nTickCount;
	aOutBuffer->nTimeStamp			 = aInBuffer->nTimeStamp;
	aOutBuffer->nFlags				 = aInBuffer->nFlags;			

	iCallback.MbcBufferCopied(aInBuffer, aOutBuffer);
	}

TBool CBufferCopier::RemoveBuffer(OMX_BUFFERHEADERTYPE* aBuffer, OMX_DIRTYPE aDirection)
	{
	switch(aDirection)
		{
	case OMX_DirInput:
		{
		if(aBuffer == iInBuffer)
			{
			// also check the slot used for storing the input buffer when waiting for an output buffer
			iInBuffer = NULL;
			return ETrue;
			}
		else
			{
			return RemoveFromQueue(iInQueue, aBuffer);
			}
		}
	case OMX_DirOutput:
		return RemoveFromQueue(iOutQueue, aBuffer);
	default:
		User::Panic(KBufferCopierPanic, KErrArgument);
		return EFalse;	// prevent compiler warning
		}
	}

void CBufferCopier::FlushBuffers(OMX_DIRTYPE aDirection)
	{
	OMX_BUFFERHEADERTYPE* buffer;
	if(aDirection == OMX_DirInput)
		{
        if(iInBuffer)
            {
            iCallback.MbcBufferFlushed(iInBuffer, OMX_DirInput);
            iInBuffer = NULL;
            }
		
		while(iInQueue.Receive(buffer) != KErrUnderflow)
			{
			iCallback.MbcBufferFlushed(buffer, OMX_DirInput);
			}
		}
	else if(aDirection == OMX_DirOutput)
		{
		while(iOutQueue.Receive(buffer) != KErrUnderflow)
			{
			iCallback.MbcBufferFlushed(buffer, OMX_DirOutput);
			}
		}
	else
		{
		User::Panic(KBufferCopierPanic, KErrArgument);
		}
	}

TBool CBufferCopier::RemoveFromQueue(RMsgQueue<OMX_BUFFERHEADERTYPE*>& aQueue, OMX_BUFFERHEADERTYPE* aBufferHeader)
	{
	TBool removed = EFalse;
	OMX_BUFFERHEADERTYPE* bufferHeader = NULL;
	while(aQueue.Receive(bufferHeader) != KErrUnderflow)
		{
		if(bufferHeader != aBufferHeader)
			{
			TInt err = iRemoveQueue.Send(bufferHeader);
			__ASSERT_DEBUG(err == KErrNone, User::Panic(KBufferCopierPanic, err));
			}
		else
			{
			removed = ETrue;
			}
		}
	while(iRemoveQueue.Receive(bufferHeader) != KErrUnderflow)
		{
		TInt err = aQueue.Send(bufferHeader);
		__ASSERT_DEBUG(err == KErrNone, User::Panic(KBufferCopierPanic, err));
		}
	return removed;
	}
