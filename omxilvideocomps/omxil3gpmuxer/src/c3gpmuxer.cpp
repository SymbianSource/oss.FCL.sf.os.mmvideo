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

#include "comxil3gpmuxeraudioinputport.h"
#include "comxil3gpmuxervideoinputport.h"
#include "c3gpmuxer.h"
#include "log.h"
#include <openmax/il/common/omxilcallbacknotificationif.h>

_LIT(K3GPMuxerPanic, "C3GPMuxer");
const TUint KAudioTimeScale = 44100;
static const TUint KThreadStackSize = 8192;	// 8KB
const TInt KMaxBufferSize = 62400 * 2;
const TInt KMicroSecondsPerSecond = 1000000;


C3GPMuxer* C3GPMuxer::NewL(MOmxILCallbackNotificationIf& aCallbacks)
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::NewL"));
	C3GPMuxer* self = new (ELeave) C3GPMuxer(aCallbacks);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

C3GPMuxer::C3GPMuxer(MOmxILCallbackNotificationIf& aCallbacks): 
 CActive(EPriorityStandard), 
 iCallbacks(aCallbacks)
	{
	}

void C3GPMuxer::ConstructL()
	{
	User::LeaveIfError(iVideoQueue.CreateLocal(KMaxVideoBuffers));	
	iPartialFrame.CreateL(KMaxBufferSize);	
	User::LeaveIfError(iAudioQueue.CreateLocal(KMaxAudioBuffers));
	User::LeaveIfError(iRemoveQueue.CreateLocal(Max(KMaxVideoBuffers, KMaxAudioBuffers)));
	User::LeaveIfError(iQueMutex.CreateLocal());

	iComposer = C3GPCompose::NewL();
	}

void C3GPMuxer::StartL(TBool aAudioPortEnabled, TBool aVideoPortEnabled)
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::StartL"));

	iPaused = EFalse;

	if (!iThreadRunning)
		{
		iAudioPortEnabled = aAudioPortEnabled;
		iVideoPortEnabled = aVideoPortEnabled;

		TBuf<40> threadName;
		threadName.Format(_L("C3GPMuxerThread@%08X"), this);
	
		User::LeaveIfError(iThread.Create(threadName, ThreadEntryPoint, KThreadStackSize, NULL, this));

		// start the thread and wait for it to start the active scheduler
		TRequestStatus status;
		iThread.Rendezvous(status);
		iThread.Resume();
		User::WaitForRequest(status);
		User::LeaveIfError(status.Int());
		}
	else
	    {
	    // self complete
        iQueMutex.Wait();
        if (!iRequestOutstanding)
            {
            TRequestStatus* statusPtr = &iStatus;
            iThread.RequestComplete(statusPtr, KErrNone);        
            }
        iQueMutex.Signal();
	    }
	}

TInt C3GPMuxer::ThreadEntryPoint(TAny* aPtr)
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(!cleanup)
		{
		return KErrNoMemory;
		}

	// Call this objects clock initialisation routine
	TRAPD(err, static_cast<C3GPMuxer*>(aPtr)->RunThreadL());
	delete cleanup;
	return err;
	}

void C3GPMuxer::RunThreadL()
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::RunThreadL"));
	CActiveScheduler* sched = new (ELeave) CActiveScheduler;
	CleanupStack::PushL(sched);
	
	CActiveScheduler::Install(sched);
	CActiveScheduler::Add(this);

	if (iAudioPortEnabled)
		{
		iAudioQueue.NotifyDataAvailable(iStatus);
		SetActive();												
        iRequestOutstanding = ETrue;
		}
	else if (iVideoPortEnabled)
		{
		iVideoQueue.NotifyDataAvailable(iStatus);
		SetActive();									
        iRequestOutstanding = ETrue;
		}

	iEndWaitAO = CEndWaitAO::NewL();
	
	iThreadRunning = ETrue;

	iThread.Rendezvous(KErrNone);

	CActiveScheduler::Start();

	HandleError(iComposer->Complete());
	iComposerOpened = EFalse;
	
	CleanupStack::PopAndDestroy(sched);
	}

C3GPMuxer::~C3GPMuxer()
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::~C3GPMuxer"));

	if(iThreadRunning)
		{		
		iThreadRunning = EFalse;
		TRequestStatus logonStatus;
		iThread.Logon(logonStatus);

		iEndWaitAO->Call();
		User::WaitForRequest(logonStatus);

		delete iEndWaitAO;
		iEndWaitAO = NULL;
		}

	iVideoQueue.Close();
	iPartialFrame.Close();
	iAudioQueue.Close();
	iRemoveQueue.Close();

	delete iComposer;

	iThread.Close();
	iQueMutex.Close();
	}

void C3GPMuxer::ProcessThisBufferL(OMX_BUFFERHEADERTYPE* aBufferHeader, TUint32 aPortIndex)
	{
	DEBUG_PRINTF3(_L8("C3GPMuxer::ProcessThisBufferL : aBufferHeader[%X]; aPortIndex[%u]"), aBufferHeader, aPortIndex);
	__ASSERT_ALWAYS(aBufferHeader->nOffset + aBufferHeader->nFilledLen <= aBufferHeader->nAllocLen, User::Invariant());
	
	if (aPortIndex == COmxIL3GPMuxer::EPortIndexAudioInput && iAudioPortEnabled)
		{
		User::LeaveIfError(iAudioQueue.Send(aBufferHeader));		
		}
	else if (aPortIndex == COmxIL3GPMuxer::EPortIndexVideoInput && iVideoPortEnabled)
		{
		User::LeaveIfError(iVideoQueue.Send(aBufferHeader));		
		}
	// FIXME buffer arrived on disabled port?? if we don't consider this an error why bother checking?
	}

void C3GPMuxer::FlushBuffers(TUint32 aPortIndex)
	{
	DEBUG_PRINTF2(_L8("C3GPMuxer::FlushBuffers : aPortIndex[%u]"), aPortIndex);

	__ASSERT_DEBUG(aPortIndex == OMX_ALL || aPortIndex < COmxIL3GPMuxer::EPortIndexMax, User::Invariant());

	if (aPortIndex == OMX_ALL || aPortIndex < COmxIL3GPMuxer::EPortIndexMax)
		{
		iQueMutex.Wait();

		if (aPortIndex == OMX_ALL)
			{
			for (TInt portIndex = 0; portIndex < COmxIL3GPMuxer::EPortIndexMax; ++portIndex)
				{
				DoFlushBuffers(portIndex);
				}
			}
		else
			{
			DoFlushBuffers(aPortIndex);
			}

		iQueMutex.Signal();
		}
	}

void C3GPMuxer::DoFlushBuffers(TUint32 aPortIndex)
	{
	DEBUG_PRINTF2(_L8("C3GPMuxer::DoFlushBuffers : aPortIndex[%u]"), aPortIndex);

	OMX_BUFFERHEADERTYPE* buffer = NULL;
	switch(aPortIndex)
		{
		case COmxIL3GPMuxer::EPortIndexAudioInput:
			{
			if (iAudioBuffer)
				{
				ReturnBuffer(iAudioBuffer, aPortIndex);
				}
				
			while(iAudioQueue.Receive(buffer) != KErrUnderflow)
				{
				ReturnBuffer(buffer, aPortIndex); 
				}
			break;
			}
		case COmxIL3GPMuxer::EPortIndexVideoInput:
			{
			if (iCurrentVideoBuffer)
				{
				ReturnBuffer(iCurrentVideoBuffer, aPortIndex);
				iCurrentVideoBuffer = NULL;
				}
			if (iNextVideoBuffer)
				{
				ReturnBuffer(iNextVideoBuffer, aPortIndex);
				iNextVideoBuffer = NULL;
				}
			while(iVideoQueue.Receive(buffer) != KErrUnderflow)
				{
				ReturnBuffer(buffer, aPortIndex); 
				}
			break;			
			}
		default:
			{
			User::Panic(K3GPMuxerPanic, KErrArgument);
			break;
			}
		}
	}

TBool C3GPMuxer::RemoveBuffer(OMX_BUFFERHEADERTYPE* aBufferHeader)
	{
	DEBUG_PRINTF2(_L8("C3GPMuxer::RemoveBuffer : aBufferHeader[%X]"), aBufferHeader);
	iQueMutex.Wait();	
	TBool found = EFalse;
	
	// check the current audio/video buffers first
	if(aBufferHeader == iAudioBuffer)
		{
		iAudioBuffer = NULL;
		found = ETrue;
		}

	if(!found && aBufferHeader == iCurrentVideoBuffer)
		{
		iCurrentVideoBuffer = NULL;
		found = ETrue;
		}
	if(!found && aBufferHeader == iNextVideoBuffer)
		{
		iNextVideoBuffer = NULL;
		found = ETrue;
		}

	if (!found)
		{
		found = RemoveFromQueue(iAudioQueue, aBufferHeader);
		}

	if (!found)
		{
		found = RemoveFromQueue(iVideoQueue, aBufferHeader);
		}

	iQueMutex.Signal();

	return found;
	}

void C3GPMuxer::SetFilename(const HBufC* aFilename)
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::SetFilename"));
 	iFilename = aFilename;
	}

void C3GPMuxer::SetAudioVideoProperties(OMX_U32& aFrameWidth, OMX_U32& aFrameHeight, 
									OMX_U32& aFramerate, OMX_U32& aBitRate, OMX_U32& /*aAudioFramerate*/)
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::SetAudioVideoProperties"));
	iVideoSize.iWidth = static_cast<TInt>(aFrameWidth);
	iVideoSize.iHeight = static_cast<TInt>(aFrameHeight);
	
	iBitRate = aBitRate;

	CalculateVideoTimeScale(aFramerate);
	}

/**
Calculate video timescale and frame duration.
The video timescale is the nearest whole number multiple of the frame rate.
Where the frame rate includes more than 2 decimal places, it is rounded down
to 2 decimal places. This means the calculation is not wholly accurate in those
circumstances, but hopefully it is close enough.
*/
void C3GPMuxer::CalculateVideoTimeScale(OMX_U32& aFramerate)
	{
	// Get rid of floating point and round to 2 decimal places
	TInt fps = ((static_cast<TReal>(aFramerate) / (1<<16)) * 100.0) + 0.5;

	iVideoTimeScale = 0;
	iDefaultVideoFrameDuration = 1;

	// Find nearest whole number multiple
	for (TInt i = 1; i <= 100; ++i)
		{
		if ((fps * i) % 100 == 0)
			{
			iVideoTimeScale = fps * i / 100;
			iDefaultVideoFrameDuration = i;
			break;
			}
		}
	}

void C3GPMuxer::RunL()
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::RunL"));
	iQueMutex.Wait();

	iRequestOutstanding = EFalse;	
	
	if (!iPaused)
	    {
	    if (iAudioPortEnabled && iVideoPortEnabled)
	        {
	        RunAudioVideoL();
	        }
	    else if (iAudioPortEnabled)
	        {
	        RunAudioL();
	        }
	    else if (iVideoPortEnabled)
	        {
	        RunVideoL();
	        }
	    }
	else
	    {
	    iStatus = KRequestPending;	    
	    }

    SetActive();
	iQueMutex.Signal();
	}

void C3GPMuxer::RunAudioVideoL()
	{
	// When opening the composer, both audio and video properties must be passed in
	if(!iAudioBuffer)
		{
		// ignore error, if KErrUnderflow then we go back to waiting on iAudioQueue
		iAudioQueue.Receive(iAudioBuffer);
		}
	else
		{
		TInt err = KErrNone;
		if (!iCurrentVideoBuffer)
			{
			err = iVideoQueue.Receive(iCurrentVideoBuffer);					

			// if KErrUnderflow then we go back to waiting on iVideoQueue
			if (err == KErrNone)
				{
				// both audio and video buffers has been received
				if (!iComposerOpened)
					{
					OpenComposerL();
					iComposerOpened = ETrue;

					iVideoDuration = (iCurrentVideoBuffer->nTimeStamp * iVideoTimeScale) / KMicroSecondsPerSecond;

					ReturnBuffer(iAudioBuffer, COmxIL3GPMuxer::EPortIndexAudioInput);
					iAudioBuffer = NULL;
					// [SL] FIXME may also need to return iVideoBuffer in nFilledLen = 0
					// (VideoSpecificInfo was entire buffer)
					}

				// We need the timestamp of the next buffer to work out the frame duration of the current video buffer
				// so wait to receive the next video buffer
				}
			}
		else
			{
			if(!iNextVideoBuffer)
				{
				err = iVideoQueue.Receive(iNextVideoBuffer);
				}

			// if KErrUnderflow then we go back to waiting on iVideoQueue
			if(err == KErrNone)
				{
				// next video buffer received
						
				// audio/video frames are added into the composer in the order of timestap
				if (iAudioBuffer->nTimeStamp < iCurrentVideoBuffer->nTimeStamp)
					{
					WriteAudioBuffer();
					iAudioBuffer = NULL;					
					}
				else
					{
					WriteVideoBufferL();
					iCurrentVideoBuffer = iNextVideoBuffer;
					iNextVideoBuffer = NULL;									
							
					if (iCurrentVideoBuffer->nFlags & OMX_BUFFERFLAG_EOS)
						{
						// this is the last video buffer
						WriteVideoBufferL();
						iCurrentVideoBuffer = NULL;
						}			
					}
				}			
			}
		}
			
	if(!iAudioBuffer)
		{
		iAudioQueue.NotifyDataAvailable(iStatus);
		}
	else if(!iNextVideoBuffer)
		{
		iVideoQueue.NotifyDataAvailable(iStatus);
		}
	else
		{
		// already have both buffers available
		TRequestStatus* statusPtr = &iStatus;
		User::RequestComplete(statusPtr, KErrNone);
		}

	iRequestOutstanding = ETrue;
	}

void C3GPMuxer::RunAudioL()
	{
	if(iAudioBuffer == NULL)
		{
		TInt err = iAudioQueue.Receive(iAudioBuffer);
		// KErrUnderflow if RemoveBuffer or FlushBuffers occurs between notify completion and RunL
		if(err != KErrNone && err != KErrUnderflow)
			{
			HandleError(err);
			return;
			}
		}

	if(iAudioBuffer != NULL)
		{
		if (!iComposerOpened)
			{
			OpenComposerL();
			iComposerOpened = ETrue;
			//return the buffer
			ReturnBuffer(iAudioBuffer, COmxIL3GPMuxer::EPortIndexAudioInput);
			}
		else
			{
			//write the non header audio buffer.
			WriteAudioBuffer();
			}
		iAudioBuffer = NULL;
		}
	
	iAudioQueue.NotifyDataAvailable(iStatus);
    iRequestOutstanding = ETrue;	
	}

void C3GPMuxer::RunVideoL()
	{
	if (!iCurrentVideoBuffer)
		{
		// FIXME KErrUnderflow if RemoveBuffer or FlushBuffers occurs between notify completion and RunL
		HandleError(iVideoQueue.Receive(iCurrentVideoBuffer));				

		if (!iComposerOpened)
			{
			OpenComposerL();
			iComposerOpened = ETrue;
			}

		iVideoDuration = (iCurrentVideoBuffer->nTimeStamp * iVideoTimeScale) / KMicroSecondsPerSecond;

		// We need the timestamp of the next buffer to work out the frame duration of the current buffer
		// so wait to receive the next buffer
		}
	else
		{
		// next buffer received
		// FIXME KErrUnderflow if RemoveBuffer or FlushBuffers occurs between notify completion and RunL
		HandleError(iVideoQueue.Receive(iNextVideoBuffer));

		WriteVideoBufferL();

		iCurrentVideoBuffer = iNextVideoBuffer;
		iNextVideoBuffer = NULL;
		
		if (iCurrentVideoBuffer->nFlags & OMX_BUFFERFLAG_EOS)
			{
			// this is the last buffer
			WriteVideoBufferL();
			iCurrentVideoBuffer = NULL;
			}
		}
		
	iVideoQueue.NotifyDataAvailable(iStatus);
	iRequestOutstanding = ETrue;	
	}

void C3GPMuxer::OpenComposerL()
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::OpenComposer"));
	T3GPAudioPropertiesBase* audioProperties = NULL;
	TPtr8 audioPtr(NULL,0);
	
	if (iAudioPortEnabled)
		{
 		// The first buffer is the decoder specific info.
 		audioPtr.Set(iAudioBuffer->pBuffer + iAudioBuffer->nOffset, 
 					iAudioBuffer->nFilledLen, 
 					iAudioBuffer->nAllocLen - iAudioBuffer->nOffset);

		}
	//The following obj need to be declared in the scope of this function and 
	//also after audioPtr is set.
	//Mpeg4Audio is hardcoded for now.
	T3GPAudioPropertiesMpeg4Audio audioPropertiesObj(KAudioTimeScale, audioPtr);
	
	// If audio port is disabled, audioProperties needs to be NULL
	if (iAudioPortEnabled)
		{
		audioProperties=&audioPropertiesObj;
		}
	
	T3GPVideoPropertiesBase* videoProperties = NULL;
	TPtr8 videoPtr(NULL,0);
	TBool foundVideoFrame = EFalse;
	if (iVideoPortEnabled)
		{
		// get decoder specific info length
		TInt offset;	
		const TUint8* buf = reinterpret_cast<const TUint8*>(iCurrentVideoBuffer->pBuffer + iCurrentVideoBuffer->nOffset);
		TUint32 stitch = 0xFFFFFFFF;
		for(offset = 0; offset < iCurrentVideoBuffer->nFilledLen; offset++)
			{
			stitch <<= 8;
			stitch |= buf[offset];
			if(stitch == 0x000001B6)	// start of a frame
				{
				foundVideoFrame = ETrue;
				break;
				}
			}		

		if (foundVideoFrame)
			{
			offset -= 3;
			videoPtr.Set(iCurrentVideoBuffer->pBuffer + iCurrentVideoBuffer->nOffset, 
			             offset, iCurrentVideoBuffer->nAllocLen - iCurrentVideoBuffer->nOffset);
			iCurrentVideoBuffer->nOffset += offset;
			iCurrentVideoBuffer->nFilledLen -= offset;
			}
		else
			{
			// We assume that the whole frame is the decoder specific info
			videoPtr.Set(iCurrentVideoBuffer->pBuffer + iCurrentVideoBuffer->nOffset,
			             iCurrentVideoBuffer->nFilledLen, iCurrentVideoBuffer->nAllocLen - iCurrentVideoBuffer->nOffset);

			// Whole buffer has been used. Set remaining length to zero so that when the
			// next buffer arrives we don't try to write out the decoder specific info
			// as a frame
			iCurrentVideoBuffer->nOffset += iCurrentVideoBuffer->nFilledLen;
			iCurrentVideoBuffer->nFilledLen = 0;
			}

		// FIXME unhandled allocation error - store this on the stack?
		videoProperties = new T3GPVideoPropertiesMpeg4Video(iVideoTimeScale, iVideoSize, iBitRate, iBitRate, videoPtr);
		}

	HandleError(iComposer->Open(E3GP3GP, videoProperties, audioProperties, *iFilename, E3GPLongClip));

	delete videoProperties;
	}

void C3GPMuxer::WriteVideoBufferL()
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::WriteVideoBuffer"));
	TUint8* currentBuffer = NULL;
	TInt currentBufferLength = 0;
	TInt currentBufferMaxLength = 0;
	TBool multipleFrameInBuffer = EFalse;
			
	if (iPartialFrame.Length() > 0)
		{
		// merge the partial frame from the previous buffer
				
		if (iPartialFrame.Length()+iCurrentVideoBuffer->nFilledLen > KMaxBufferSize)
			{
			iPartialFrame.ReAllocL(iPartialFrame.Length()+iCurrentVideoBuffer->nFilledLen);
			}
				
		iPartialFrame.Append(iCurrentVideoBuffer->pBuffer + iCurrentVideoBuffer->nOffset, iCurrentVideoBuffer->nFilledLen);			
				
		currentBuffer = const_cast<TUint8*>(iPartialFrame.Ptr());
		currentBufferLength = iPartialFrame.Length();
		currentBufferMaxLength = iPartialFrame.MaxLength();
		
		multipleFrameInBuffer = ETrue;		
		}
	else
		{
		currentBuffer = iCurrentVideoBuffer->pBuffer + iCurrentVideoBuffer->nOffset;
		currentBufferLength = iCurrentVideoBuffer->nFilledLen;
		currentBufferMaxLength = iCurrentVideoBuffer->nAllocLen - iCurrentVideoBuffer->nOffset;
		}

	// Write video frames from buffer.
	// Check for non-zero buffer length in case we have been sent an empty buffer
	if (currentBufferLength > 0)
		{
		if(!multipleFrameInBuffer && (iCurrentVideoBuffer->nFlags & OMX_BUFFERFLAG_SYNCFRAME))
			{
			// buffer contains one full I frame
			TInt nextFrameDuration = CalculateNextFrameDuration();
			TPtr8 videoPtr(currentBuffer, currentBufferLength, currentBufferMaxLength);									
			HandleError(iComposer->WriteVideoFrame(videoPtr, nextFrameDuration, ETrue));
			}
		else if (!multipleFrameInBuffer && (iCurrentVideoBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME))
			{
			// buffer contains one full frame
	
			// Evaluate whether it is an I frame 
			TInt offset = 0;	
			TUint32 stitch = 0xFFFFFFFF;
			TBool isIframe = EFalse;
					
			for(offset = 0; offset < currentBufferLength; offset++)
				{					
				stitch <<= 8;
				stitch |= currentBuffer[offset];
		
				if(stitch == 0x000001B6)		// start of a frame
					{
					TUint8 iframebits = currentBuffer[offset+1] >> 6;	// get the next 2 bits					
					if (iframebits == 0)
						{
						isIframe = ETrue;
						}
					else
						{
						isIframe = EFalse;							
						}
								
					break;		
					}
				}
		 
			TInt nextFrameDuration = CalculateNextFrameDuration();
			TPtr8 videoPtr(currentBuffer, currentBufferLength, currentBufferMaxLength);								
			HandleError(iComposer->WriteVideoFrame(videoPtr, nextFrameDuration, isIframe));
			}
		else
			{
			// buffer either contains multiple frames or part of a single frame
			// so need to parse the buffer to retrieve the frames
	
			TInt offset = 0;
			TUint32 stitch = 0xFFFFFFFF;
	
			TBool foundNextFrame = EFalse;
			TInt frameLength = 0;
			TInt frameOffset = 0;
			TBool isIframe = EFalse;
			TBool frameWritten = EFalse;
					
			// retieve and write video frames
			for(offset = 0; offset < currentBufferLength; offset++)
				{					
				stitch <<= 8;
				stitch |= currentBuffer[offset];
	
				frameLength++;	
		
				if(!foundNextFrame && stitch == 0x000001B6)
					{
					foundNextFrame = ETrue;
							
					// Evaluate whether it is an I frame 
					if (offset+1 >= currentBufferLength)
						{
						break; // reached the end of the buffer
						}
								
					TUint8 iframebits = currentBuffer[offset+1] >> 6;	// get the next 2 bits					
					if (iframebits == 0)
						{
						isIframe = ETrue;
						}
					else
						{
						isIframe = EFalse;							
						}	
					}
				else if (foundNextFrame && (stitch == 0x000001B0 || stitch == 0x00000120))
					{
					// start of next frame (which includes VOS and/or VOL header)
	
					// write the current frame
					TInt nextFrameDuration = CalculateNextFrameDurationPartial(frameWritten);
					TPtr8 videoPtr(currentBuffer + frameOffset, frameLength - 4, currentBufferMaxLength - frameOffset);										
					HandleError(iComposer->WriteVideoFrame(videoPtr, nextFrameDuration, isIframe));
					frameWritten = ETrue;

					frameOffset = offset - 3; // to include the VOS/VOL start code in the next frame
					frameLength = 4;

					foundNextFrame = EFalse;
					isIframe = EFalse;
					}
				else if (foundNextFrame && stitch == 0x000001B6)
					{
					// start of next frame
	
					// write the current frame
					TInt nextFrameDuration = CalculateNextFrameDurationPartial(frameWritten);
					TPtr8 videoPtr(currentBuffer + frameOffset,	frameLength - 4, currentBufferMaxLength - frameOffset);
					HandleError(iComposer->WriteVideoFrame(videoPtr, nextFrameDuration, isIframe));
					frameWritten = ETrue;
		
					frameOffset = offset - 3; // to include the VOP start code in the next frame		
					frameLength = 4;						
							
					foundNextFrame = ETrue;
							
					// Evaluate whether it is an I frame 	
					if (offset+1 >= currentBufferLength)
						{
						break; // reached the end of the buffer
						}
	
					TUint8 iframebits = currentBuffer[offset+1] >> 6;  // get the next 2 bits						
					if (iframebits == 0)
						{
						isIframe = ETrue;
						}
					else
						{
						isIframe = EFalse;							
						}	
					}
				}
					
			if (frameLength > 0)
				{
				if (iCurrentVideoBuffer->nFlags & OMX_BUFFERFLAG_EOS)
					{
					// this is the last buffer, so we can assume that the rest of the buffer contains one full frame
					TInt nextFrameDuration = CalculateNextFrameDurationPartial(frameWritten);
					TPtr8 videoPtr(currentBuffer + frameOffset,	frameLength, currentBufferMaxLength - frameOffset);
					HandleError(iComposer->WriteVideoFrame(videoPtr, nextFrameDuration, isIframe));
					frameWritten = ETrue;
					}
				else
					{
					// the buffer may contain part of a frame
					// save it for use later
					iPartialFrame.Copy(currentBuffer + frameOffset, frameLength);
					}
				}
			else
				{
				User::Panic(K3GPMuxerPanic, KErrGeneral);  // frameLength should never be 0
				}	
			}
		}

	ReturnBuffer(iCurrentVideoBuffer, COmxIL3GPMuxer::EPortIndexVideoInput);										
	}

TInt C3GPMuxer::CalculateNextFrameDuration()
	{
	TInt64 durationSinceStart = (iCurrentVideoBuffer->nTimeStamp * iVideoTimeScale) / KMicroSecondsPerSecond;
	TInt nextFrameDuration = durationSinceStart - iVideoDuration;
	if (nextFrameDuration < iDefaultVideoFrameDuration)
		{
		nextFrameDuration = iDefaultVideoFrameDuration;
		}

	iVideoDuration = durationSinceStart;

	return nextFrameDuration;
	}

TInt C3GPMuxer::CalculateNextFrameDurationPartial(TBool aFrameWritten)
	{
	if (!aFrameWritten)
		{
		return CalculateNextFrameDuration();
		}

	iVideoDuration += iDefaultVideoFrameDuration;
	
	return iDefaultVideoFrameDuration;
	}

void C3GPMuxer::WriteAudioBuffer()
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::WriteAudioBuffer"));
	
	if (iAudioBuffer->nFilledLen != 0)
		{
		TPtr8 audioPtr(iAudioBuffer->pBuffer + iAudioBuffer->nOffset, 
								iAudioBuffer->nFilledLen, iAudioBuffer->nAllocLen - iAudioBuffer->nOffset);


 		//Calculate the audio frame duration in timescale
		//nTimeStamp is in microseconds
		TUint durationInTimeSlice=KAudioTimeScale*iAudioBuffer->nTimeStamp/KMicroSecondsPerSecond;
 		TUint duration=durationInTimeSlice-iAudioFrameDuration;
 		iAudioFrameDuration=durationInTimeSlice;
 		HandleError(iComposer->WriteAudioFrames(audioPtr, duration));		


	
		}
	ReturnBuffer(iAudioBuffer, COmxIL3GPMuxer::EPortIndexAudioInput);
	}

void C3GPMuxer::ReturnBuffer(OMX_BUFFERHEADERTYPE* aBuffer, TUint32 aPortIndex)
	{
	DEBUG_PRINTF3(_L8("C3GPMuxer::ReturnBuffer : aBuffer[%X]; aPortIndex[%u]"), aBuffer, aPortIndex);
	OMX_U32 flags = aBuffer->nFlags;
	aBuffer->nFilledLen = 0;
	aBuffer->nFlags = 0;
	aBuffer->nOffset = 0;
	aBuffer->nTimeStamp = 0;
	iCallbacks.BufferDoneNotification(aBuffer, aPortIndex, OMX_DirInput);

	if(flags & OMX_BUFFERFLAG_EOS)
		{
		iCallbacks.EventNotification(OMX_EventBufferFlag, aPortIndex, flags, NULL);
		}
	}
	
void C3GPMuxer::DoCancel()
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::DoCancel"));
	if (iAudioPortEnabled)
		{
		iAudioQueue.CancelDataAvailable();
		}
	
	if (iVideoPortEnabled)
		{
		iVideoQueue.CancelDataAvailable();		
		}
	}

TBool C3GPMuxer::RemoveFromQueue(RMsgQueue<OMX_BUFFERHEADERTYPE*>& aQueue, OMX_BUFFERHEADERTYPE* aBufferHeader)
	{
	DEBUG_PRINTF3(_L8("C3GPMuxer::RemoveFromQueue : aQueue[%X]; aBufferHeader[%X]"), &aQueue, aBufferHeader);
	TBool removed = EFalse;
	OMX_BUFFERHEADERTYPE* bufferHeader = NULL;
	while(aQueue.Receive(bufferHeader) != KErrUnderflow)
		{
		if(bufferHeader != aBufferHeader)
			{
			TInt error = iRemoveQueue.Send(bufferHeader);
			__ASSERT_DEBUG(error == KErrNone, User::Panic(K3GPMuxerPanic, error));
			}
		else
			{
			removed = ETrue;
			}
		}
	while(iRemoveQueue.Receive(bufferHeader) != KErrUnderflow)
		{
		TInt error = aQueue.Send(bufferHeader);
		__ASSERT_DEBUG(error == KErrNone, User::Panic(K3GPMuxerPanic, error));
		}
	return removed;
	}

void C3GPMuxer::Pause()
	{
	DEBUG_PRINTF(_L8("C3GPMuxer::Pause"));
	iPaused = ETrue;
	}

void C3GPMuxer::HandleError(TInt aError)
	{
	DEBUG_PRINTF2(_L8("C3GPMuxer::HandleError : aError[%d]"), aError);
	OMX_ERRORTYPE omxError = SymbianErrorToOmx(aError);
	if (omxError != OMX_ErrorNone)
		{
		iMuxerInvalid = ETrue;
		iCallbacks.ErrorEventNotification(omxError);
		}
	}
	
TBool C3GPMuxer::IsMuxerInvalid() const
	{
	return iMuxerInvalid;				
	}	

OMX_ERRORTYPE C3GPMuxer::SymbianErrorToOmx(TInt aError)
	{
	switch(aError)
		{
	case KErrNone:
		return OMX_ErrorNone;
	case KErrNoMemory:
		return OMX_ErrorInsufficientResources;
	case KErrWrite:
		return OMX_ErrorContentPipeOpenFailed;
	default:
		return OMX_ErrorUndefined;
		}
	}
