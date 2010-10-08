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

#include "comxilvideoschedulerpf.h"
#include "comxilvideoscheduler.h"
#include "resourcefilereader.h"
#include "buffercopierstatemonitor.h"
#include "omxilvideoschedulerextensionsindexes.h"
#include <openmax/il/extensions/omxildroppedframeeventextension.h>
#include <openmax/il/common/omxilcallbacknotificationif.h>
#include "log.h"

_LIT(KVideoSchedulerPanicCategory, "omxilvscheduler"); //should restrict to 16 characters as it is used in User::Panic
_LIT(KResourceFileName, "Z:\\resource\\videoscheduler\\videoscheduler.rsc");
const TInt KMaxRenderTime = 1000000;
const TInt KMaxGraphicSinkBufferCount(2); //This is set as the maximum number of buffers that can be sent to the graphic sink without the risk of overloading it.



COmxILVideoSchedulerPF* COmxILVideoSchedulerPF::NewL(MOmxILCallbackNotificationIf& aCallbacks, COmxILVideoScheduler& aComponent, OMX_COMPONENTTYPE* aHandle)
	{
	COmxILVideoSchedulerPF* self = new (ELeave) COmxILVideoSchedulerPF(aCallbacks, aComponent, aHandle);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

COmxILVideoSchedulerPF::COmxILVideoSchedulerPF(MOmxILCallbackNotificationIf& aCallbacks, COmxILVideoScheduler& aComponent, OMX_COMPONENTTYPE* aHandle)
: COmxILProcessingFunction(aCallbacks),
  iComponent(aComponent),
  iIsClockStopped(ETrue),
  iInvalid(EFalse),
  iTimeStamp(KMinTInt64),
  iHandle(aHandle)
 	{
	}

void COmxILVideoSchedulerPF::ConstructL()
	{
	User::LeaveIfError(iMutex.CreateLocal());
	iBufferCopierStateMonitor = CBufferCopierStateMonitor::NewL(*this, iComponent);
	// get timer info
	CResourceFileReader* reader = CResourceFileReader::NewLC(KResourceFileName);
	reader->ReadTimerInfoL(iRenderTime, iMaxLateness);
	CleanupStack::PopAndDestroy(reader);

	// Prefill the render time array with the default render time read from resource file
	for (TInt count = 0; count < KRenderTimeListLength; ++count)
		{
		iRenderTimeList[count] = iRenderTime;
		}
	iRenderTimeSum = iRenderTime * KRenderTimeListLength;
	}

COmxILVideoSchedulerPF::~COmxILVideoSchedulerPF()
	{
	delete iBufferCopierStateMonitor;
    iMutex.Wait();
	iWaitingBuffers.Reset();
	iCompletedBuffersHeldByPause.Reset();
    iMutex.Signal();	
	iMutex.Close();
	}

OMX_ERRORTYPE COmxILVideoSchedulerPF::StateTransitionIndication(TStateIndex aNewState)
	{
	switch(aNewState)
		{
		case EStateExecuting:
			{
			if (iPausedState)
				{
			    iMutex.Wait();
				iPausedState = EFalse;
				
				// send any buffers that received time updates during paused state
				if (iOutputBufferSentCount < KMaxGraphicSinkBufferCount)   // only allowed to send 2 buffers at a time
					{
					SubmitBufferHeldByPause();
					}
                iMutex.Signal();
				}
			break;
			}
		case EStatePause:
			{
			iPausedState = ETrue;
			break;
			}
		case ESubStateLoadedToIdle:
			{
			TUint32 bufferCount = iComponent.BufferCount();
			
			TInt error = iBufferCopierStateMonitor->SetState(CBufferCopierStateMonitor::ESubLoadedToIdle);
			
			if (error != KErrNone)
			    {
			    return SymbianErrorToOmx(error);                    
			    }
			
			error = iWaitingBuffers.Reserve(bufferCount);
			if (error != KErrNone)
			    {
			    return SymbianErrorToOmx(error);                    
			    }

			error = iCompletedBuffersHeldByPause.Reserve(bufferCount);
			if (error != KErrNone)
			    {
			    return SymbianErrorToOmx(error);                    
			    }
			
			break;
			}
		case EStateIdle:
			{
			iOutputBufferSentCount = iComponent.BufferCount();
			break;
			}
		case ESubStateIdleToLoaded:
			{
			TInt error = iBufferCopierStateMonitor->SetState(CBufferCopierStateMonitor::ESubIdleToLoaded);
			if (error != KErrNone)
			    {
			    return SymbianErrorToOmx(error);                    
			    }
			break;
			}
		default:
			{
			break;
			}	
		}
    
	return OMX_ErrorNone;	
	}

OMX_ERRORTYPE COmxILVideoSchedulerPF::BufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection)
	{
	// called from command thread
	
    iMutex.Wait();
	if (iBufferCopierStateMonitor->BufferCopier())
		{
		if (aPortIndex == OMX_ALL)
			{
			iBufferCopierStateMonitor->BufferCopier()->FlushBuffers(OMX_DirInput);
			iBufferCopierStateMonitor->BufferCopier()->FlushBuffers(OMX_DirOutput);
			}
		else
			{
			iBufferCopierStateMonitor->BufferCopier()->FlushBuffers(aDirection);
			}
		}

	if (aDirection == OMX_DirOutput || aPortIndex == OMX_ALL)
		{
		while (iWaitingBuffers.Count() > 0)
			{
			iWaitingBuffers[0]->nFilledLen = 0;
			iWaitingBuffers[0]->nOffset = 0;
			iWaitingBuffers[0]->nTimeStamp = 0;
			iCallbacks.BufferDoneNotification(iWaitingBuffers[0], 1, OMX_DirOutput);
			iWaitingBuffers.Remove(0);
			iOutputBufferSentCount++;
			}
		if(iSinkPendingBuffer)
			{
			iSinkPendingBuffer->nFilledLen = 0;
			iSinkPendingBuffer->nOffset = 0;
			iSinkPendingBuffer->nTimeStamp = 0;
			iCallbacks.BufferDoneNotification(iSinkPendingBuffer, 1, OMX_DirOutput);
			iSinkPendingBuffer = NULL;
			iOutputBufferSentCount++;
			}
		}
    iMutex.Signal();        

	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxILVideoSchedulerPF::ParamIndication(OMX_INDEXTYPE aParamIndex,
													const TAny* apComponentParameterStructure)
	{
	DEBUG_PRINTF(_L8("COmxILVideoSchedulerProcessingFunction::ParamIndication"));

	if(aParamIndex == OMX_NokiaIndexParamDroppedFrameEvent)
		{
		const OMX_NOKIA_PARAM_DROPPEDFRAMEEVENT* dropFrame = static_cast<const OMX_NOKIA_PARAM_DROPPEDFRAMEEVENT*>(apComponentParameterStructure);
		iEnableDropFrameEvent = dropFrame->bEnabled;
		}

	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxILVideoSchedulerPF::ConfigIndication(OMX_INDEXTYPE /*aConfigIndex*/, const TAny* /*apComponentConfigStructure*/)
	{
	return OMX_ErrorNone;	
	}

OMX_ERRORTYPE COmxILVideoSchedulerPF::BufferIndication(OMX_BUFFERHEADERTYPE* apBufferHeader,
	  												   OMX_DIRTYPE aDirection)			
	{
	if (iInvalid)
	    {
	    return OMX_ErrorInvalidState;
	    }

	// called from decoder data thread or sink data thread
	iMutex.Wait();
	if(aDirection == OMX_DirOutput)
		{
		apBufferHeader->nFlags = 0;
		iOutputBufferSentCount--;
		ASSERT(iOutputBufferSentCount <= iComponent.BufferCount());
		
		DEBUG_PRINTF2(_L8("VS2::BufferIndication : apBufferHeader->nTickCount = %d"), apBufferHeader->nTickCount);

		// update the render time if it is set
		if (apBufferHeader->nTickCount > 0 && apBufferHeader->nTickCount <= KMaxRenderTime)
			{
			// Add new render time to render time list, and recalculate average
			iRenderTimeSum -= iRenderTimeList[iRenderTimeListPos];
			iRenderTimeSum += apBufferHeader->nTickCount;
			iRenderTimeList[iRenderTimeListPos] = apBufferHeader->nTickCount;
			++iRenderTimeListPos;
			iRenderTimeListPos %= KRenderTimeListLength;

			iRenderTime = iRenderTimeSum / KRenderTimeListLength;

			DEBUG_PRINTF2(_L8("VS2::BufferIndication : New iRenderTime = %ld"), iRenderTime);
			}

		// previously sent buffer has come back
		// send any buffers that received time updates
		// at startup, iOutputBufferSentCount may be >2 if output port is non-supplier
		if (!iPausedState && iOutputBufferSentCount < KMaxGraphicSinkBufferCount)
			{
			SubmitBufferHeldByPause();
			}
		}
	else if(apBufferHeader->nFlags & OMX_BUFFERFLAG_DECODEONLY)
		{
		// this frame is not to be rendered (probably as part of an accurate seek)
		// drop the data and send it back to the decoder
		apBufferHeader->nFilledLen = 0;
		apBufferHeader->nFlags = 0;
		apBufferHeader->nOffset = 0;
		iCallbacks.BufferDoneNotification(apBufferHeader, 0, OMX_DirInput);
		iMutex.Signal();
		return OMX_ErrorNone;
		}
	if (iBufferCopierStateMonitor->BufferCopier())
	    {
	    iBufferCopierStateMonitor->BufferCopier()->DeliverBuffer(apBufferHeader, aDirection);
	    }
    iMutex.Signal();
    
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxILVideoSchedulerPF::MediaTimeIndication(const OMX_TIME_MEDIATIMETYPE& aTimeInfo)
	{
	// called from clock thread
	
	switch(aTimeInfo.eUpdateType)
		{
	case OMX_TIME_UpdateRequestFulfillment:
		{
		
		iMutex.Wait();

		TInt index = -1;
		OMX_BUFFERHEADERTYPE* buffer = reinterpret_cast<OMX_BUFFERHEADERTYPE*>(aTimeInfo.nClientPrivate);
		__ASSERT_DEBUG(buffer->nTimeStamp == aTimeInfo.nMediaTimestamp, Panic(EPanicBadAssociation));
		if(FindWaitingBuffer(buffer, aTimeInfo.nMediaTimestamp, index))
			{
			if (iPausedState || iCompletedBuffersHeldByPause.Count() > 0)
				{
				TBufferMessage bufferMessage;
				bufferMessage.iBufferHeader = buffer;
				bufferMessage.iMediaTimeInfo = aTimeInfo;
				
				OMX_ERRORTYPE error = SymbianErrorToOmx(iCompletedBuffersHeldByPause.Append(bufferMessage)); // note append cannot fail, allocated enough slots
				iMutex.Signal();
				return error;
				}
			else 
				{
				SendTimedOutputBuffer(buffer, aTimeInfo, index);
				}
			}
		else
			{
			// TODO [SL] now what?
			User::Invariant();
			}

		iMutex.Signal();
		return OMX_ErrorNone;
		}

	case OMX_TIME_UpdateScaleChanged:
		if(aTimeInfo.xScale >= 0)
			{
			// the clock takes care completing requests at the correct media time
			return OMX_ErrorNone;
			}
		else
			{
			// TODO think harder about implications of negative scale
			// certainly the iTimeStamp checking must be reversed
			ASSERT(0);
			return OMX_ErrorNotImplemented;
			}

	case OMX_TIME_UpdateClockStateChanged:
		iClockState.eState = aTimeInfo.eState;
		switch(aTimeInfo.eState)
			{
		case OMX_TIME_ClockStateStopped:
		    {
		    // clock stopped so remove any pending buffers from the list as time requests
		    // will be resent when the clock is running again
		    
	        iIsClockStopped	= ETrue;	 

	        iMutex.Wait();
			while (iCompletedBuffersHeldByPause.Count() > 0)
                { 
				iCompletedBuffersHeldByPause.Remove(0);
                }           	         
	        
			if(iSinkPendingBuffer && iBufferCopierStateMonitor)
				{
				// if sink pending buffer exist (as sink is bottleneck) then drop the frame
				iBufferCopierStateMonitor->BufferCopier()->DeliverBuffer(iSinkPendingBuffer, OMX_DirOutput);

				// dropped a frame, so send an event if the dropped frame extension is enabled
				if (iEnableDropFrameEvent)
					{
					//TODO DL: iCallbacks.EventNotification(OMX_EventNokiaDroppedFrame, 1, 0, NULL);
					}

				iSinkPendingBuffer = NULL;
				}
            iMutex.Signal();            
	        }
			break;
			
		case OMX_TIME_ClockStateWaitingForStartTime:
		    {
		    iIsClockStopped = EFalse;
		    // if now in WaitingForStartTime state and start time already received, send it now
		    if (iStartTimePending)
		        {
		        OMX_ERRORTYPE error = iComponent.SetVideoStartTime(iStartTime);
		        if (error != OMX_ErrorNone)
		            {
		            // iStartTimePending = EFalse; // FIXME - Is this required?
		            return error;
		            }
		        }
		    }
			break;
			
		case OMX_TIME_ClockStateRunning:
			{
			iTimeStamp = KMinTInt64;
			if(iIsClockStopped)
				{
				// the clock is running after being stopped previously
				// resend time requests for waiting buffers
				iIsClockStopped = EFalse;
				
				for (TInt i = 0; i < iWaitingBuffers.Count(); ++i)
					{
					iComponent.MediaTimeRequest(iWaitingBuffers[i], iWaitingBuffers[i]->nTimeStamp, iRenderTime);
					}
				}
			}
			break;
			}
		
		iStartTimePending = EFalse;
		DEBUG_PRINTF2(_L8("VS2::MediaTimeIndication : ClockStateChanged = %d"), aTimeInfo.eState);
		
		return OMX_ErrorNone;
		
	default:
		return OMX_ErrorBadParameter;
		}
	
	}

/* Check if aBuffer still exist in the waiting queue */ 
TBool COmxILVideoSchedulerPF::FindWaitingBuffer(const OMX_BUFFERHEADERTYPE* aBuffer, const OMX_TICKS& aMediaTime, TInt& aIndex) const
	{
	__ASSERT_DEBUG(const_cast<RMutex&>(iMutex).IsHeld(), Panic(EPanicMutexUnheld));
	
	TBool found = EFalse;
		
	for (TInt i=0; i<iWaitingBuffers.Count(); ++i)
		{
		if ((iWaitingBuffers[i] == aBuffer) && (iWaitingBuffers[i]->nTimeStamp == aMediaTime))
			{
			found = ETrue;
			aIndex = i;
			break;
			}
		}

	return found;
	}

/**
Check if a specified buffer is currently held by the processing function,
and remove it if found.

@param apBufferHeader Buffer to remove
@param aDirection Port direction
@return Flag to indicate if buffer was removed.
*/
OMX_BOOL COmxILVideoSchedulerPF::BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection)
	{
    iMutex.Wait();	
	if(iBufferCopierStateMonitor->BufferCopier() && iBufferCopierStateMonitor->BufferCopier()->RemoveBuffer(apBufferHeader, aDirection))
		{
		if(aDirection == OMX_DirOutput)
			{
			iOutputBufferSentCount++;
			}
		iMutex.Signal();
		return OMX_TRUE;
		}
	else if(aDirection == OMX_DirOutput)
		{
		for (TInt i = 0; i < iWaitingBuffers.Count(); ++i)
			{
			if (iWaitingBuffers[i] == apBufferHeader)
				{
				iWaitingBuffers[i]->nFilledLen = 0;
				iWaitingBuffers.Remove(i);
				iOutputBufferSentCount++;
			    iMutex.Signal();
				return OMX_TRUE;
				}
			}
		if(apBufferHeader == iSinkPendingBuffer)
			{
			iSinkPendingBuffer = NULL;
			iOutputBufferSentCount++;
			iMutex.Signal();
			return OMX_TRUE;
			}
		}
 
	iMutex.Signal();    
	return OMX_FALSE;
	}

/* Submit the first time update buffer that still exists in the waiting queue. */
void COmxILVideoSchedulerPF::SubmitBufferHeldByPause()
	{
	__ASSERT_DEBUG(iMutex.IsHeld(), Panic(EPanicMutexUnheld));
	__ASSERT_DEBUG(iOutputBufferSentCount < KMaxGraphicSinkBufferCount, Panic(EPanicBadOutputRegulation));

	if(iSinkPendingBuffer)
		{
		DEBUG_PRINTF(_L8("VS2::SubmitBufferHeldByPause ***************************SEND SINK PENDING BUFFER"));
		OMX_BUFFERHEADERTYPE* buffer = iSinkPendingBuffer;
		iSinkPendingBuffer = NULL;
		SendOutputBuffer(buffer);
		return;
		}

	TInt index = -1;
	TBool bufferSent = EFalse;
	while (iCompletedBuffersHeldByPause.Count() > 0 && !bufferSent)
		{
		TBufferMessage& msg = iCompletedBuffersHeldByPause[0];
		if (FindWaitingBuffer(msg.iBufferHeader, 
					msg.iMediaTimeInfo.nMediaTimestamp, index))
			{
			DEBUG_PRINTF(_L8("VS2::SubmitBufferHeldByPause ***************************SEND HELD BUFFER"));
			bufferSent = SendTimedOutputBuffer(msg.iBufferHeader, msg.iMediaTimeInfo, index);
			}
		iCompletedBuffersHeldByPause.Remove(0);
		}
	}

/** Returns ETrue if aBuffer was sent, EFalse otherwise */
TBool COmxILVideoSchedulerPF::SendTimedOutputBuffer(OMX_BUFFERHEADERTYPE* aBuffer, const OMX_TIME_MEDIATIMETYPE& aMediaTimeInfo, TInt aIndex)
	{
	__ASSERT_DEBUG(iMutex.IsHeld(), Panic(EPanicMutexUnheld));
	__ASSERT_DEBUG(aBuffer->nTimeStamp == aMediaTimeInfo.nMediaTimestamp, Panic(EPanicBadAssociation));
	__ASSERT_DEBUG(aBuffer == reinterpret_cast<OMX_BUFFERHEADERTYPE*>(aMediaTimeInfo.nClientPrivate), Panic(EPanicBadAssociation));
	
#ifdef _OMXIL_COMMON_DEBUG_TRACING_ON	
	DEBUG_PRINTF(_L8("VS2::SendTimedOutputBuffer **********************************"));
	TTime t;
	t.HomeTime();
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : t.HomeTime() = %ld"), t.Int64());
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : aMediaTimeInfo.nClientPrivate = 0x%X"), aMediaTimeInfo.nClientPrivate);
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : aMediaTimeInfo.nMediaTimestamp = %ld"), aMediaTimeInfo.nMediaTimestamp);
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : aMediaTimeInfo.nOffset = %ld"), aMediaTimeInfo.nOffset);
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : aMediaTimeInfo.nWallTimeAtMediaTime = %ld"), aMediaTimeInfo.nWallTimeAtMediaTime);
#endif
	
	TBool bufferSent = EFalse;

	OMX_U32 flags = aBuffer->nFlags;

	// Work out how long it is from now until the frame will be rendered.
	// This will be the time it takes the sink to render, minus the offset
	// value from the clock completion (i.e how far before the requested
	// time that the clock has completed us). A lateness of 0 means we are at
	// the correct time to send the buffer, a positive lateness means we are
	// late sending the buffer, and a lateness waitTime means we are early.
	// For the first frame we were not able to request an early completion to
	// offset the render time, so assume that the render time is 0.
	OMX_TICKS lateness = 0 - aMediaTimeInfo.nOffset;
	if (!(flags & OMX_BUFFERFLAG_STARTTIME))
		{
		lateness += iRenderTime;
		}

	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : iRenderTime = %ld"), iRenderTime);
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : lateness = %ld"), lateness);
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : iMaxLateness = %ld"), iMaxLateness);
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : iTimeStamp = %ld"), iTimeStamp);
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : iFrameDroppedCount = %d"), iFrameDroppedCount);
	DEBUG_PRINTF2(_L8("VS2::SendTimedOutputBuffer : flags = %d"), flags);

	iWaitingBuffers.Remove(aIndex);

	// Send the buffer if the wait time is within the maximum allowed delay and timestamp is later than the previous timestamp, otherwise skip the buffer	
	if ((lateness <= iMaxLateness || iFrameDroppedCount >= KMaxGraphicSinkBufferCount) && aMediaTimeInfo.nMediaTimestamp > iTimeStamp)   // shouldn't drop more than 2 frames at a time when decoder is slow
		{
		DEBUG_PRINTF(_L8("VS2::SendTimedOutputBuffer ***************************SHOW"));
		
		bufferSent = ETrue;
		iFrameDroppedCount = 0;

		SendOutputBuffer(aBuffer);
		}
	else
		{
		DEBUG_PRINTF(_L8("VS2::SendTimedOutputBuffer ***************************DROP"));
		
		iFrameDroppedCount++;

		// dropped a frame, so send an event if the dropped frame extension is enabled
		if(iEnableDropFrameEvent)
			{
            //TODO DL: iCallbacks.EventNotification(OMX_EventNokiaDroppedFrame, 1, 0, NULL);
			}

		// if EOS was on the buffer, send an empty buffer with EOS and send the EOS event
		// if not, discard the buffer contents and post the buffer for another copy
		if(flags & OMX_BUFFERFLAG_EOS)
			{
			DEBUG_PRINTF(_L8("VS2::SendTimedOutputBuffer ***************************SEND EMPTY EOS BUFFER"));
			aBuffer->nFilledLen = 0;
			aBuffer->nOffset = 0;
			aBuffer->nTimeStamp = 0;
			SendOutputBuffer(aBuffer);
			}
		else
			{
			TOmxILUtil::ClearBufferContents(aBuffer);
			aBuffer->nOffset = 0;
			if (iBufferCopierStateMonitor->BufferCopier())
			    {
			    iBufferCopierStateMonitor->BufferCopier()->DeliverBuffer(aBuffer, OMX_DirOutput);
			    }
			}
		}
	
	return bufferSent;	
	}

void COmxILVideoSchedulerPF::SendOutputBuffer(OMX_BUFFERHEADERTYPE* aBuffer)
	{
	__ASSERT_DEBUG(iMutex.IsHeld(), Panic(EPanicMutexUnheld));
	__ASSERT_DEBUG(iTimeStamp < aBuffer->nTimeStamp || aBuffer->nFlags & OMX_BUFFERFLAG_EOS, Panic(EPanicTimestampEmissionUnordered));

	if(iOutputBufferSentCount >= KMaxGraphicSinkBufferCount)
		{
		DEBUG_PRINTF(_L8("VS2::SendOutputBuffer : *****************STORING SINK PENDING BUFFER"));

		// sink is bottleneck, keep the most recent pending frame but return the rest so decoder keeps running
		// when sink returns a buffer send the most recent frame
		if(iSinkPendingBuffer && iBufferCopierStateMonitor->BufferCopier())
			{
			if (iSinkPendingBuffer->nFlags & OMX_BUFFERFLAG_EOS)
			    {
			    //if (bizarrely) pending buffer has EOS flag and another buffer replaces it.
			    DoSendOutputBuffer(iSinkPendingBuffer);
			    }
			else
			    {
			    iBufferCopierStateMonitor->BufferCopier()->DeliverBuffer(iSinkPendingBuffer, OMX_DirOutput);
			    }

			DEBUG_PRINTF(_L8("VS2::SendOutputBuffer : *****************DROPPED EXISTING SINK PENDING BUFFER"));

			// dropped a frame, so send an event if the dropped frame extension is enabled
			if(iEnableDropFrameEvent)
				{
                //TODO DL: iCallbacks.EventNotification(OMX_EventNokiaDroppedFrame, 1, 0, NULL);
				}
			}

		iSinkPendingBuffer = aBuffer;
		}
	else
		{
		DoSendOutputBuffer(aBuffer);
		}
	}

/** Called when the buffer copier has transferred the data from an input buffer to an output buffer. */
void COmxILVideoSchedulerPF::MbcBufferCopied(OMX_BUFFERHEADERTYPE* aInBuffer, OMX_BUFFERHEADERTYPE* aOutBuffer)
	{
	iMutex.Wait();
	
	// send input buffer back
	aInBuffer->nFilledLen = 0;
	aInBuffer->nOffset = 0;
	aInBuffer->nFlags = 0;
	aInBuffer->nTimeStamp = 0;

	// Deal with any buffer marks. Currently the component framework makes an attempt to deal with
	// them, but it cannot associate the input buffer mark with the corresponding output buffer so
	// we may need to do some tweaking here.
	if (aInBuffer->hMarkTargetComponent)
		{
		if (aInBuffer->hMarkTargetComponent == iHandle)
			{
			// There was a buffer mark on the input buffer intended for us. That means there is no
			// need to send it out on the output buffer. Also, it is OK to let the component framework
			// deal with it in this situation.
			aOutBuffer->hMarkTargetComponent = NULL;
			aOutBuffer->pMarkData = NULL;
			}
		else
			{
			// There was a buffer mark on the input buffer but it is not intended for us. If
			// we let the component framework deal with it then we will get multiple marks sent
			// out because we have copied it to the output buffer, and the framework will also
			// store it to send out later. Clear it here so the framework does not see it.
			aInBuffer->hMarkTargetComponent = NULL;
			aInBuffer->pMarkData = NULL;
			}
		}

	OMX_ERRORTYPE error;

	iCallbacks.BufferDoneNotification(aInBuffer, 0, OMX_DirInput);
	
	if(aOutBuffer->nFilledLen > 0 || (aOutBuffer->nFlags & OMX_BUFFERFLAG_EOS))
		{
		iWaitingBuffers.Append(aOutBuffer);  // note append cannot fail, allocated enough slots
		}
	
	if(aOutBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME)
		{
		if(OMX_TIME_ClockStateWaitingForStartTime == iClockState.eState)
			{
			error = iComponent.SetVideoStartTime(aOutBuffer->nTimeStamp);
			if (error != OMX_ErrorNone)
			    {
			    HandleIfError(error);
			    }
			iStartTimePending = EFalse;
			}
		else
			{
			// delay sending until clock transitions to WaitingForStartTime
			iStartTime = aOutBuffer->nTimeStamp;
			iStartTimePending = ETrue;
			}
		}
		
#ifdef _OMXIL_COMMON_DEBUG_TRACING_ON	
	DEBUG_PRINTF(_L8("VS2::MbcBufferCopied **********************************"));
	TTime t;
	t.HomeTime();
	DEBUG_PRINTF2(_L8("VS2::MbcBufferCopied : t.HomeTime() = %ld"), t.Int64());
	DEBUG_PRINTF2(_L8("VS2::MbcBufferCopied : aOutBuffer = 0x%X"), aOutBuffer);
	DEBUG_PRINTF2(_L8("VS2::MbcBufferCopied : aOutBuffer->nTimeStamp = %ld"), aOutBuffer->nTimeStamp);
#endif	
	iMutex.Signal();
	
	if (aOutBuffer->nFilledLen == 0 && !(aOutBuffer->nFlags & OMX_BUFFERFLAG_EOS))
		{
		// A likely cause of receiving an empty buffer is if the decoder implements a flush as
		// returning buffers to supplier or always sending buffers to peer, rather than emptied
		// and queued on the output port. In this case we return the buffer immediately without
		// making a media time request. Probably the timestamp is invalid or the clock is not in
		// the running state, in either case we could deadlock by queueing the empty buffers after
		// a flush and preventing new data from being delivered. However these buffers were not
		// returned in BufferIndication() in case there were any flags that need processing.
		iBufferCopierStateMonitor->BufferCopier()->DeliverBuffer(aOutBuffer, OMX_DirOutput);
		}
	else
		{
		if (!iIsClockStopped)
		    {
		    error = iComponent.MediaTimeRequest(aOutBuffer, aOutBuffer->nTimeStamp, iRenderTime);
		    if (error != OMX_ErrorNone)
		        {
		        HandleIfError(error);
		        }
		    }
		}
	}

/** Called when a buffer is flushed from the buffer copier. */
void COmxILVideoSchedulerPF::MbcBufferFlushed(OMX_BUFFERHEADERTYPE* aBuffer, OMX_DIRTYPE aDirection)
	{
	TInt portIndex = 0;
	aBuffer->nFilledLen = 0;
	aBuffer->nOffset = 0;
	aBuffer->nFlags = 0;
	aBuffer->nTimeStamp = 0;

	if (aDirection == OMX_DirOutput)
		{
		++iOutputBufferSentCount;
		portIndex = 1;
		}

	iCallbacks.BufferDoneNotification(aBuffer, portIndex, aDirection);
	}

void COmxILVideoSchedulerPF::DoSendOutputBuffer(OMX_BUFFERHEADERTYPE* aBuffer)
    {
    OMX_ERRORTYPE error;
    // A zero length buffer means this buffer is just being sent because it
    // has the EOS flag.
    if (aBuffer->nFilledLen > 0)
        {
        aBuffer->nTickCount = 0xC0C0C0C0;
        iTimeStamp = aBuffer->nTimeStamp;
        }
    
    error = iCallbacks.BufferDoneNotification(aBuffer, 1, OMX_DirOutput);
    if (error != OMX_ErrorNone)
        {
        HandleIfError(error);
        }

    iOutputBufferSentCount++;

    OMX_U32 flags = aBuffer->nFlags;
    if(flags & OMX_BUFFERFLAG_EOS)
        {
        error = iCallbacks.EventNotification(OMX_EventBufferFlag, 1, flags, NULL);
        if (error != OMX_ErrorNone)
            {
            HandleIfError(error);
            }
        }
    }

void COmxILVideoSchedulerPF::HandleIfError(OMX_ERRORTYPE aOmxError)
    {
    if (aOmxError != OMX_ErrorNone)
        {
        iInvalid = ETrue;
        iCallbacks.ErrorEventNotification(aOmxError);
        }
    }

OMX_ERRORTYPE COmxILVideoSchedulerPF::SymbianErrorToOmx(TInt aError)
	{
	switch(aError)
		{
	case KErrNone:
		return OMX_ErrorNone;
	case KErrNoMemory:
		return OMX_ErrorInsufficientResources;
	default:
		return OMX_ErrorUndefined;
		}
	}



void COmxILVideoSchedulerPF::Panic(TVideoSchedulerPanic aPanicCode) const
	{
	// const allows const methods to panic using this method
	// however we wish to release the mutex to avoid blocking other threads
	RMutex& mutex = const_cast<RMutex&>(iMutex);
	if(mutex.IsHeld())
		{
		mutex.Signal();
		}
	User::Panic(KVideoSchedulerPanicCategory, aPanicCode);
	}
