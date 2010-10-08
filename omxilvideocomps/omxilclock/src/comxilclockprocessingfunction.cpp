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

#include <openmax/il/common/omxilutil.h>
#include "comxilclockprocessingfunction.h"
#include "comxilclockcomponent.h"
#include "clocksupervisor.h"
#include <openmax/il/common/omxilcallbacknotificationif.h>
#include "clockpanics.h"
#include "omxilclock.hrh"

#include <openmax/il/common/omxilspecversion.h>


OMX_ERRORTYPE SymbianErrorToOmx(TInt aError);

COmxILClockProcessingFunction* COmxILClockProcessingFunction::NewL(MOmxILCallbackNotificationIf& aCallbacks,
		                                                           COmxILClockComponent& aComponent)
	{
	COmxILClockProcessingFunction* self = new (ELeave) COmxILClockProcessingFunction(aCallbacks, aComponent);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

COmxILClockProcessingFunction::COmxILClockProcessingFunction(MOmxILCallbackNotificationIf& aCallbacks,
		                                                     COmxILClockComponent& aComponent) :
 COmxILProcessingFunction(aCallbacks), iComponent(aComponent)
 	{
	}

void COmxILClockProcessingFunction::ConstructL()
	{
	iClock = CClockSupervisor::NewL(*this);
	iThreadNotifier = CClockThreadNotifier::NewL(iClock);
	iThreadNotifier->IssueRequest();
	User::LeaveIfError(iBufferMutex.CreateLocal());
	// create a buffer queue for each port and add it to the iBufferQueues array
	for(TInt portIndex = 0; portIndex < KNumPorts; portIndex++)
		{
		CCirBuf<OMX_BUFFERHEADERTYPE*>* queue = new(ELeave) CCirBuf<OMX_BUFFERHEADERTYPE*>();
		CleanupStack::PushL(queue);
		iBufferQueues.AppendL(queue);
		CleanupStack::Pop(queue);
		}
	}

COmxILClockProcessingFunction::~COmxILClockProcessingFunction()
	{
	delete iClock;
	delete iThreadNotifier;
	iBufferMutex.Close();
	// empty iBufferQueues and delete any CCirBuf objects
	// the CCirBuf objects don't own any buffers they contain, so we don't delete those here
	iBufferQueues.ResetAndDestroy();
	}

OMX_ERRORTYPE COmxILClockProcessingFunction::StateTransitionIndication(TStateIndex aNewState)
	{	
	OMX_ERRORTYPE omxError = OMX_ErrorNone;

	switch(aNewState)
		{
		case ESubStateLoadedToIdle:
			{
			// allocate space for the buffer queues now the buffer count on each port is known
			for(TInt portIndex = 0; portIndex < KNumPorts; portIndex++)
				{
				TInt length = iComponent.PortBufferCount(portIndex);
				TRAPD(error, iBufferQueues[portIndex]->SetLengthL(length));
				// error not actually a problem if queue was originally longer than we need
				if(error != KErrNone && iBufferQueues[portIndex]->Length() < length)
					{
					omxError = OMX_ErrorInsufficientResources;
					break;
					}
				}
			break;
			}
			
		case EStateExecuting:
			{
			iExecuting = ETrue;
			break;
			}
			
		case EStateIdle:
			{
			iExecuting = EFalse;
			break;
			}
			
		case EStatePause:
			{
			// NOTE we do not change iExecuting
			// The value of iExecuting maintains the direction from which
			// Paused was entered (i.e. from Idle or Executing).
			// This is because we wan't to know whether buffers are available,
			// regardless of whether we are paused or not.
			// TODO TBD is there an implicit stopping of the clock (e.g. set scale to 0)?
			break;
			}
		default:
			{
			break;
			}
		}
	
	return omxError;		
	}

OMX_ERRORTYPE COmxILClockProcessingFunction::BufferFlushingIndication(TUint32 aPortIndex,
                                                                      OMX_DIRTYPE /*aDirection*/)
	{
	__ASSERT_DEBUG(aPortIndex == OMX_ALL || aPortIndex < KNumPorts, Panic(EBufferFlushingInvalidPort));

	if (aPortIndex == OMX_ALL || aPortIndex < KNumPorts)
		{
		iBufferMutex.Wait();

		if (aPortIndex == OMX_ALL)
			{
			for (TInt portIndex = 0; portIndex < KNumPorts; portIndex++)
				{
				DoBufferFlushingIndication(portIndex);
				}
			}
		else
			{
			DoBufferFlushingIndication(aPortIndex);
			}

		iBufferMutex.Signal();
		}

	return OMX_ErrorNone;
	}

void COmxILClockProcessingFunction::DoBufferFlushingIndication(TUint32 aPortIndex)
	{
	CCirBuf<OMX_BUFFERHEADERTYPE*>& queue = *iBufferQueues[aPortIndex];

	while(queue.Count() > 0)
		{
		OMX_BUFFERHEADERTYPE* buffer;
		// ignore error, we just checked Count() > 0 and we have the mutex, so a buffer
		// should definitely be there
		queue.Remove(&buffer);
		OMX_ERRORTYPE error = iCallbacks.BufferDoneNotification(buffer, aPortIndex, OMX_DirOutput);
		// the callback manager should return OMX_ErrorNone
		// the callback manager ignores any error code from the callee component since it is the component's responsibility
		// to respond to errors generated internally
		__ASSERT_DEBUG(error == OMX_ErrorNone, Panic(ECallbackManagerBufferError));
		}
	}

OMX_ERRORTYPE COmxILClockProcessingFunction::ParamIndication(OMX_INDEXTYPE /*aParamIndex*/,
                                                                      const TAny* /*apComponentParameterStructure*/)
	{
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxILClockProcessingFunction::ConfigIndication(OMX_INDEXTYPE /*aConfigIndex*/,
                                                                       const TAny* /*apComponentConfigStructure*/)
	{
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxILClockProcessingFunction::BufferIndication(OMX_BUFFERHEADERTYPE* apBufferHeader,
                                                              OMX_DIRTYPE /*aDirection*/)
	{
	// add buffer to the appropriate port queue
	
	CCirBuf<OMX_BUFFERHEADERTYPE*>& queue = *iBufferQueues[apBufferHeader->nOutputPortIndex];
	TOmxILUtil::ClearBufferContents(apBufferHeader);
	apBufferHeader->nOffset = 0;
	iBufferMutex.Wait();
	TInt added = queue.Add(&apBufferHeader);
	// don't expect 0 (not added) as total number of buffers is known
	__ASSERT_DEBUG(added, Panic(EBufferQueueOverflow));

	iBufferMutex.Signal();
	return OMX_ErrorNone;
	}

/**
 * Finds and removes an item from a CCirBuf.
 * NOTE items are NOT guaranteed to be in their original position!
 * The queue must be protected from concurrent access when calling this
 * function.
 * 
 * @param aQueue queue to modify
 * @param aRemoveItem item to remove
 * @return ETrue if item was found and removed from the queue, EFalse otherwise.
 */
template<class T>
static TBool RemoveFromPool(CCirBuf<T>&aQueue, T aRemoveItem)
	{
	TInt numItems = aQueue.Count();
	for(TInt index = 0; index < numItems; index++)
		{
		T item;
		TInt removed = aQueue.Remove(&item);	// ignore error since queue cannot be empty (numItems > 0)
		__ASSERT_DEBUG(removed == 1, Panic(EBufferUnderflow));
		if(item == aRemoveItem)
			{
			return ETrue;
			}
		else
			{
			TInt added = aQueue.Add(&item);	// ignore error since always space for 1 item as one has just been removed
			__ASSERT_DEBUG(added == 1, Panic(EBufferQueueOverflow));
			}
		}
	return EFalse;
	}

OMX_BOOL COmxILClockProcessingFunction::BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader,
		                                                        OMX_DIRTYPE /*aDirection*/)
	{
	CCirBuf<OMX_BUFFERHEADERTYPE*>& queue = *iBufferQueues[apBufferHeader->nOutputPortIndex];
	iBufferMutex.Wait();
	// note queue may be reordered, but that's OK since these are empty buffers
	TBool removed = RemoveFromPool(queue, apBufferHeader);
	iBufferMutex.Signal();

	return removed ? OMX_TRUE : OMX_FALSE;
	}

/**
 * Returns a free buffer from the queue for the specified port.
 * If no buffer is ready, this method returns NULL
 * This method does not block for a buffer to become available.
 */
OMX_BUFFERHEADERTYPE* COmxILClockProcessingFunction::AcquireBuffer(TInt aPortIndex)
	{
	CCirBuf<OMX_BUFFERHEADERTYPE*>& queue = *iBufferQueues[aPortIndex];
	iBufferMutex.Wait();
	OMX_BUFFERHEADERTYPE* buffer;
	TInt count = queue.Remove(&buffer);
	iBufferMutex.Signal();
	if(count > 0)
		{
		return buffer;
		}
	else
		{
		return NULL;
		}
	}

/**
 * Sends a buffer out on a port.
 */
void COmxILClockProcessingFunction::SendBuffer(OMX_BUFFERHEADERTYPE* aBuffer)
	{
	OMX_ERRORTYPE error = iCallbacks.BufferDoneNotification(aBuffer, aBuffer->nOutputPortIndex, OMX_DirOutput);
	// the callback manager should return OMX_ErrorNone
	// the callback manager ignores any error code from the callee component since it is the component's responsibility
	// to respond to errors generated internally
	__ASSERT_DEBUG(error == OMX_ErrorNone, Panic(ECallbackManagerBufferError));
	}

OMX_ERRORTYPE COmxILClockProcessingFunction::ProduceRequest(OMX_INDEXTYPE aIdx, CClockSupervisor::TEntryPoint aEntryPoint, TAny* pComponentConfigStructure)
	{
	return iClock->ProduceRequest(aIdx, aEntryPoint, pComponentConfigStructure);
	}

/**
 * Returns true iff the specified port is currently enabled.
 */
TBool COmxILClockProcessingFunction::PortEnabled(TInt aPortIndex) const
	{
	return iComponent.PortEnabled(aPortIndex);
	}

TBool COmxILClockProcessingFunction::IsExecuting() const
	{
	return iExecuting;
	}

/**
 * Called when CClockSupervisor encounters a fatal error and needs to transition the
 * component to OMX_StateInvalid.
 */
void COmxILClockProcessingFunction::InvalidateComponent()
	{
	OMX_ERRORTYPE error = iCallbacks.ErrorEventNotification(OMX_ErrorInvalidState);
	// the callback manager should return OMX_ErrorNone
	// the callback manager ignores any error code from the client since it is the client's responsibility to respond
	// to errors generated internally
	__ASSERT_DEBUG(error == OMX_ErrorNone, Panic(ECallbackManagerEventError));
	// TODO this sends the invalidated event up to the IL client, but does not alter
	// the internal state of the component to reflect being invalid. It seems you need
	// access to private, non-exported FsmTransition to do that?
	}

