/*
* Copyright (c) 2008 - 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "log.h"
#include <openmax/il/common/omxilcallbacknotificationif.h>
#include <openmax/il/common/omxilutil.h>
#include "c3gpdemuxer.h"
#include "omxil3gpdemuxerpanic.h"
#include "comxil3gpdemuxertimeinputport.h"
#include "comxil3gpdemuxeraudiooutputport.h"
#include "comxil3gpdemuxervideooutputport.h"

C3GPDemuxer::CPort* C3GPDemuxer::CPort::NewL(TInt aBufferCount)
	{
	CPort* self = new (ELeave) CPort();
	CleanupStack::PushL(self);
	self->ConstructL(aBufferCount);
	CleanupStack::Pop(self);
	return self;
	}

C3GPDemuxer::CPort::CPort() :
	iEOS(EFalse)
	{
	}

void C3GPDemuxer::CPort::ConstructL(TInt aBufferCount)
	{
	iBuffers.ReserveL(aBufferCount);
	}

C3GPDemuxer::CPort::~CPort()
	{
	iBuffers.Reset();
	}

C3GPDemuxer* C3GPDemuxer::NewL(MOmxILCallbackNotificationIf& aCallbacks)
	{
	C3GPDemuxer* self = new (ELeave) C3GPDemuxer(aCallbacks);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void C3GPDemuxer::ConstructL()
	{
	i3GPParser = C3GPParse::NewL();
	}

C3GPDemuxer::C3GPDemuxer(MOmxILCallbackNotificationIf& aCallbacks) :
	CActive(EPriorityStandard),
	iCallbacks(aCallbacks),
	iVideoType(E3GPNoVideo),
	iAudioType(E3GPNoAudio),
	iFirstVideoFrame(ETrue),
	iFirstAudioFrame(ETrue),
	iAsyncBuf(0, 0)
	{
	iOmxAudioFormat.iCoding = OMX_AUDIO_CodingMax;
	iOmxVideoFormat.iCoding = OMX_VIDEO_CodingMax;

	for (TInt port = 0; port < COmxIL3GPDemuxer::EPortIndexMax; ++port)
		{
		iPort[port] = NULL;
		}

	CActiveScheduler::Add(this);
	}

C3GPDemuxer::~C3GPDemuxer()
	{
	Cancel();

	DeleteBufferQueue();

	if (i3GPParser)
		{
		HandleIfError(i3GPParser->Complete());
		delete i3GPParser;
		}
	}

OMX_ERRORTYPE C3GPDemuxer::AcquireResources(const TDesC& aFilename)
	{
	DEBUG_PRINTF(_L8("C3GPDemuxer::AcquireResources"));

	TInt error = i3GPParser->Open(aFilename);
	if (error == KErrNone)
		{
		iParserOpened = ETrue;

		if (iStartTimePosition != 0)
			{
			SetPosition(iStartTimePosition, iStartKeyFrame);
			iSeekPosition = iStartTimePosition;
			iStartTimePosition = 0;
			}

		TRAP(error, CreateBufferQueueL());
		}

	return SymbianOSErrorToOmx(error);
	}

void C3GPDemuxer::ReleaseResources()
	{
	DeleteBufferQueue();

	if (i3GPParser)
		{
		HandleIfError(i3GPParser->Complete());
		iParserOpened = EFalse;
		}
	}

void C3GPDemuxer::Start()
	{
	if (iPaused && iState != EStateWaitingToStart)
		{
		if (iState != EStateFillingBuffer)
			{
			Cancel();
			
			// Self complete so that we start to process any buffer received while
			// we were in paused state
			CompleteSelf();
			}
		}
	else
		{
		Cancel();
		iState = EStateWaitingForBuffer;
		// Self complete so that we start to process any buffer received while
		// we were in idle state
		CompleteSelf();
		}

	iPaused = EFalse;
	}

OMX_ERRORTYPE C3GPDemuxer::Stop()
	{
	Cancel();

	if (iState == EStateFillingBuffer)
		{
	 	i3GPParser->CancelReadFrame();
		}

	iState = EStateWaitingToStart;
	StartWaitingForBuffer();

	iAudioHeadersSent = EFalse;
	iFirstAudioFrame = ETrue;
	iVideoHeadersSent = EFalse;
	iFirstVideoFrame = ETrue;
	iPaused = EFalse;

	TInt error = SetPosition(0, EFalse);

	return SymbianOSErrorToOmx(error);
	}

void C3GPDemuxer::Pause()
	{
	iPaused = ETrue;
	}

TBool C3GPDemuxer::Invalid() const
	{
	return iInvalid;
	}

void C3GPDemuxer::ProcessThisBufferL(OMX_BUFFERHEADERTYPE* aBufferHeader,
		TUint32 aPortIndex)
	{
	__ASSERT_DEBUG(iBufferQueueCreated, Panic(EProcessThisBufferLNoBufferQueue));

	aBufferHeader->nFilledLen = 0;
	aBufferHeader->nOffset = 0;
	aBufferHeader->nTimeStamp = 0;
	aBufferHeader->nFlags = 0;

	if (iBufferQueueCreated)
		{
		TBufferMessage buffer;
		buffer.iBufferHeader = aBufferHeader;
		buffer.iPortIndex = aPortIndex;
		User::LeaveIfError(iBufferQueue.Send(buffer));
		}
	else
		{
		User::Leave(KErrNotReady);
		}
	}

void C3GPDemuxer::FlushBuffers(TUint32 aPortIndex)
	{
	__ASSERT_DEBUG(aPortIndex == OMX_ALL || aPortIndex < COmxIL3GPDemuxer::EPortIndexMax, User::Invariant());

	if (aPortIndex == OMX_ALL || aPortIndex < COmxIL3GPDemuxer::EPortIndexMax)
		{
		Cancel();
		ReceiveQueuedBuffers();

		if (aPortIndex == OMX_ALL || iCurrentPort == aPortIndex)
			{
			if (iState == EStateFillingBuffer)
				{
				// We are about to flush a buffer that is being filled.
			 	// Cancel the read operation.
			 	i3GPParser->CancelReadFrame();
				}

		 	iState = EStateWaitingForBuffer;
			}

		if (aPortIndex == OMX_ALL)
			{
			for (TInt portIndex = 0; portIndex < COmxIL3GPDemuxer::EPortIndexMax; ++portIndex)
				{
				DoFlushBuffers(portIndex);
				}
			}
		else
			{
			DoFlushBuffers(aPortIndex);
			}

		// Unless we are waiting for a read to complete, we need to self
		// complete just in case the ReceiveQueuedBuffers() call above
		// received new buffers
		if (iState != EStateFillingBuffer)
			{
			CompleteSelf();
			}
		}
	}

void C3GPDemuxer::DoFlushBuffers(TUint32 aPortIndex)
	{
	if (iPort[aPortIndex])
		{
		OMX_DIRTYPE direction = OMX_DirOutput;
		if (aPortIndex == COmxIL3GPDemuxer::EPortIndexTimeInput)
			{
			direction = OMX_DirInput;
			}

		RQueuedBuffers& buffers = iPort[aPortIndex]->iBuffers;
		while (buffers.Count() > 0)
			{
			iCallbacks.BufferDoneNotification(buffers[0], aPortIndex, direction);
			buffers.Remove(0);
			}
		}
	}

TBool C3GPDemuxer::RemoveBuffer(OMX_BUFFERHEADERTYPE* aBufferHeader,
                                OMX_DIRTYPE aDirection)
	{
	TInt port = 0;
	if (aDirection == OMX_DirOutput)
		{
		port = aBufferHeader->nOutputPortIndex;
		}
	else if (aDirection == OMX_DirInput)
		{
		port = aBufferHeader->nInputPortIndex;
		}
	else
		{
		Panic(ERemoveBufferInvalidDirection);
		}

	__ASSERT_DEBUG(port >= 0 && port < COmxIL3GPDemuxer::EPortIndexMax, Panic(ERemoveBufferInvalidPort));

	TBool found = EFalse;

	if (port >= 0 && port < COmxIL3GPDemuxer::EPortIndexMax)
		{
		Cancel();
		ReceiveQueuedBuffers();

		if (iPort[port])
			{
			RQueuedBuffers& buffers = iPort[port]->iBuffers;
			for (TInt buf = 0; buf < buffers.Count(); ++buf)
				{
				if (buffers[buf] == aBufferHeader)
					{
					if (iCurrentPort == port && buf == 0)
						{				
						if (iState == EStateFillingBuffer)
							{
							// We are about to remove a buffer that is being filled.
			 				// Cancel the read operation.
			 				i3GPParser->CancelReadFrame();
							}
		 				iState = EStateWaitingForBuffer;
						}

					buffers[buf]->nFilledLen = 0;
					buffers.Remove(buf);
					found = ETrue;
					break;
					}
				}
			}

		// Unless we are waiting for a read to complete, we need to self
		// complete just in case the ReceiveQueuedBuffers() call above
		// received new buffers
		if (iState != EStateFillingBuffer)
			{
			CompleteSelf();
			}
		}

	return found;
	}

TBool C3GPDemuxer::GetVideoFormat(TSize& aFrameSize, TVideoFormat& aFormat) const
	{
	if (!iVideoPropertiesRead)
		{
		return EFalse;
		}
	aFrameSize.iWidth = iVideoWidth;
	aFrameSize.iHeight = iVideoHeight;
	aFormat = iOmxVideoFormat;
	return ETrue;
	}

TBool C3GPDemuxer::GetAudioFormat(TAudioFormat& aFormat) const
	{
	if (!iAudioPropertiesRead)
		{
		return EFalse;
		}
	aFormat = iOmxAudioFormat;
	return ETrue;
	}

OMX_ERRORTYPE C3GPDemuxer::GetVideoTimestamp(OMX_TICKS& aOmxticks)
	{
	TInt err = KErrNone;

	TUint timestampInMilliSec(0);

	if (iParserOpened)
		{
		// return last requested seek time
		timestampInMilliSec = iSeekPosition;
		}
	else
		{
		timestampInMilliSec = iStartTimePosition;
		}

	if (err == KErrNone)
		{
		aOmxticks = timestampInMilliSec * 1000;
		}

	return SymbianOSErrorToOmx(err);
	}

OMX_ERRORTYPE C3GPDemuxer::Seek(const OMX_TICKS& aOmxticks, OMX_TIME_SEEKMODETYPE aSeekModeType)
    {
	TInt err = KErrNone;

	//Set the firstFrame flags to true
	iFirstVideoFrame = ETrue;
	iFirstAudioFrame = ETrue;

	TUint timeInMilliSec = aOmxticks / 1000;
	TBool keyFrame(aSeekModeType == OMX_TIME_SeekModeFast);

	if (iParserOpened)
		{
		err = SetPosition(timeInMilliSec, keyFrame);
		if (err != KErrNone)
			{
			iSeekPosition = timeInMilliSec;
			}
		}
	else
		{
		iStartTimePosition = timeInMilliSec;
		iStartKeyFrame = keyFrame;
		}

	return SymbianOSErrorToOmx(err);
	}

OMX_ERRORTYPE C3GPDemuxer::DetectStreams()
	{
	iAudioPropertiesRead = EFalse;

	TUint s_audioLength;
	TInt s_audioFramesPerSample;
	TUint s_audioAvgBitRate;
	TUint s_audioTimeScale;
	TInt error = i3GPParser->GetAudioProperties(iAudioType, s_audioLength,
			s_audioFramesPerSample, s_audioAvgBitRate, s_audioTimeScale);

	if (error != KErrNotSupported)
		{
		if (error)
			{
			return SymbianOSErrorToOmx(error);
			}
		else
			{
			iOmxAudioFormat.iFramesPerSample = s_audioFramesPerSample;
			iOmxAudioFormat.iSampleRate = s_audioTimeScale;
			iOmxAudioFormat.iAverageBitrate = s_audioAvgBitRate;
			switch (iAudioType)
				{
				case E3GPQcelp13K:
					{
					iOmxAudioFormat.iCoding = OMX_AUDIO_CodingQCELP13;
					}
					break;
				case E3GPMpeg4Audio:
					{
					iOmxAudioFormat.iCoding = OMX_AUDIO_CodingAAC;
					}
					break;
				case E3GPAmrNB:
				case E3GPAmrWB:
					{
					iOmxAudioFormat.iCoding = OMX_AUDIO_CodingAMR;
					}
					break;
				default:
					{
					iOmxAudioFormat.iCoding = OMX_AUDIO_CodingMax;
					}
				}

			iAudioPropertiesRead = ETrue;
			}

		}
	iVideoPropertiesRead = EFalse;

	TSize vidSize(iVideoWidth, iVideoHeight);
	TUint s_vidLength = 0;
	TReal s_vidFrameRate = 0.0;
	TUint s_vidAvgBitRate = 0;
	TSize s_vidSize;
	TUint s_vidTimeScale = 0;

	error = i3GPParser->GetVideoProperties(iVideoType, s_vidLength,
			s_vidFrameRate, s_vidAvgBitRate, s_vidSize, s_vidTimeScale);

	if (error != KErrNotSupported)
		{
		if (error)
			{
			return SymbianOSErrorToOmx(error);
			}
		else
			{
			iVideoWidth = s_vidSize.iWidth;
			iVideoHeight = s_vidSize.iHeight;

			// translate library video type to OpenMAX IL coding and profile type
			switch (iVideoType)
				{
				case E3GPMpeg4Video:
					iOmxVideoFormat.iCoding = OMX_VIDEO_CodingMPEG4;
					break;
				case E3GPH263Profile0:
					iOmxVideoFormat.iCoding = OMX_VIDEO_CodingH263;
					iOmxVideoFormat.iProfile.h263
							= OMX_VIDEO_H263ProfileBaseline;
					break;
				case E3GPH263Profile3:
					iOmxVideoFormat.iCoding = OMX_VIDEO_CodingH263;
					iOmxVideoFormat.iProfile.h263 = OMX_VIDEO_H263ProfileISWV2;
					break;
				case E3GPAvcProfileBaseline:
					iOmxVideoFormat.iCoding = OMX_VIDEO_CodingAVC;
					iOmxVideoFormat.iProfile.avc = OMX_VIDEO_AVCProfileBaseline;
					break;
				default:
					// do not return an error here, the error is signalled after the transition to Executing
					iOmxVideoFormat.iCoding = OMX_VIDEO_CodingMax;
					break;
				}
			}
			iVideoPropertiesRead = ETrue;
		}

	return OMX_ErrorNone;
	}


OMX_ERRORTYPE C3GPDemuxer::GetMetadataL(OMX_CONFIG_METADATAITEMTYPE* aMetadata)
	{
	// Metadata key size must be at least 4
	if (aMetadata->nKeySizeUsed < 4)
		{
		return OMX_ErrorBadParameter;
		}

	T3GPUdtaLocation udtaLocation;
	TUint32 udtaAtomType = Pack32(aMetadata->nKey);
	TUint32 buffersize = aMetadata->nValueMaxSize;

	RBuf8 buffer;
	CleanupClosePushL(buffer);
	User::LeaveIfError(buffer.Create(buffersize));

	TUint atomIndex = 0;
	if (aMetadata->nMetadataItemIndex != OMX_ALL)
		{
		atomIndex = aMetadata->nMetadataItemIndex;
		}

	TInt error = KErrNone;

	switch (aMetadata->eScopeMode)
		{
		case OMX_MetadataScopeAllLevels:
			{
			TUint subatomCount = 0;
			udtaLocation = E3GPUdtaMoov;

			error = i3GPParser->GetUserDataAtom(udtaAtomType, udtaLocation,	buffer, subatomCount);

			if (error == KErrNone)
				{
				if (atomIndex == 0)
					{
					break;
					}
				else if (atomIndex <= subatomCount)
					{
					error = i3GPParser->GetUserDataAtom(udtaAtomType, udtaLocation, buffer, atomIndex);
					break;
					}
				else
					{
					atomIndex -= (subatomCount + 1);
					}
				}
			else if (error != KErrNotFound)
				{
				break;
				}
			
			subatomCount = 0;
			udtaLocation = E3GPUdtaVideoTrak;
			error = i3GPParser->GetUserDataAtom(udtaAtomType, udtaLocation,
					buffer, subatomCount);
			if (error == KErrNone)
				{
				if (atomIndex == 0)
					{
					break;
					}
				else if (atomIndex <= subatomCount)
					{
					error = i3GPParser->GetUserDataAtom(udtaAtomType, udtaLocation, buffer, atomIndex);
					break;
					}
				else
					{
					atomIndex -= (subatomCount + 1);
					}
				}
			else if (error != KErrNotFound)
				{
				break;
				}
			
			udtaLocation = E3GPUdtaAudioTrak;
			error = i3GPParser->GetUserDataAtom(udtaAtomType, udtaLocation,
					buffer, atomIndex);
			}
			break;
		case OMX_MetadataScopeTopLevel:
			{
			udtaLocation = E3GPUdtaMoov;
			error = i3GPParser->GetUserDataAtom(udtaAtomType, udtaLocation,
					buffer, atomIndex);
			}
			break;
		case OMX_MetadataScopePortLevel:
			{
			if (aMetadata->nScopeSpecifier
					== COmxIL3GPDemuxer::EPortIndexVideoOutput)
				{
				udtaLocation = E3GPUdtaVideoTrak;
				error = i3GPParser->GetUserDataAtom(udtaAtomType, udtaLocation,
						buffer, atomIndex);
				}
			else if (aMetadata->nScopeSpecifier == COmxIL3GPDemuxer::EPortIndexAudioOutput)
				{
				udtaLocation = E3GPUdtaAudioTrak;
				error = i3GPParser->GetUserDataAtom(udtaAtomType, udtaLocation, buffer, atomIndex);
				}
			else
				{
				error = KErrArgument;
				}
			}
			break;
		default:
			{
			error = KErrArgument;
			}
		}

	if (error == KErrNone)
		{
		// strip size and atom type from the buffer
		Mem::Copy(aMetadata->nValue, (buffer.Ptr() + 8), (buffer.Size() - 8));
		aMetadata->nValueSizeUsed = (buffer.Size() - 8);
		}

	CleanupStack::PopAndDestroy();//buffer
	return SymbianOSErrorToOmx(error);
	}

void C3GPDemuxer::RunL()
	{
	iWaitingOnBufferQueue = EFalse;
	ReceiveQueuedBuffers();

	if (iPaused || iInvalid || iState == EStateWaitingToStart)
		{
		StartWaitingForBuffer();
		return;
		}

	if (iState == EStateWaitingToSubmit)
		{
		SubmitBuffer();
		}

	if (!ProcessBuffers())
		{
		iState = EStateWaitingForBuffer;
		StartWaitingForBuffer();
		}
	}

void C3GPDemuxer::DoCancel()
	{
	if (iWaitingOnBufferQueue)
		{
		iWaitingOnBufferQueue = EFalse;
		iBufferQueue.CancelDataAvailable();
		}
	}

TBool C3GPDemuxer::ProcessBuffers()
	{
	TUint32 startPort = iCurrentPort;

	do
		{
		if (iPort[iCurrentPort]->iBuffers.Count() > 0 && !iPort[iCurrentPort]->iEOS)
			{
			DoProcessBuffer();
			return ETrue;
			}
		}
	while (NextPort() != startPort);

	return EFalse;
	}

void C3GPDemuxer::DoProcessBuffer()
	{
	iState = EStateFillingBuffer;

	iCurrentBuffer = iPort[iCurrentPort]->iBuffers[0];
	iPort[iCurrentPort]->iBuffers.Remove(0);

	switch(iCurrentPort)
		{
		case COmxIL3GPDemuxer::EPortIndexTimeInput:
			{
			ProcessTimeBuffer();
			break;
			}
		case COmxIL3GPDemuxer::EPortIndexAudioOutput:
			{
			FillAudioBuffer();
			break;
			}
		case COmxIL3GPDemuxer::EPortIndexVideoOutput:
			{
			FillVideoBuffer();
			break;
			}
		}				
	}

void C3GPDemuxer::ProcessTimeBuffer()
	{
	// TODO
	User::Invariant();
	}

void C3GPDemuxer::FillAudioBuffer()
	{
	if (!iAudioHeadersSent)
		{
		TPtr8 audioBuffer(iCurrentBuffer->pBuffer + iCurrentBuffer->nOffset + iCurrentBuffer->nFilledLen,
				0,
				static_cast<TInt>(iCurrentBuffer->nAllocLen - iCurrentBuffer->nOffset - iCurrentBuffer->nFilledLen));
		TInt error = i3GPParser->GetAudioDecoderSpecificInfo(audioBuffer);

		if (error == KErrNone)
			{
			iCurrentBuffer->nFilledLen = audioBuffer.Length();
			iCurrentBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
			iAudioHeadersSent = ETrue;
			SubmitBuffer();
			CompleteSelf();
			}
		else
			{
			HandleIfError(error);
			}
		return;
		}

	iAsyncBuf.Set(iCurrentBuffer->pBuffer + iCurrentBuffer->nOffset + iCurrentBuffer->nFilledLen
	              ,0
	              ,static_cast<TInt>(iCurrentBuffer->nAllocLen - iCurrentBuffer->nOffset - iCurrentBuffer->nFilledLen));
	i3GPParser->ReadAudioFrames(*this, iAsyncBuf);
	}

void C3GPDemuxer::FillVideoBuffer()
	{
	if (!iVideoHeadersSent)
		{
		iCurrentBuffer->nOffset = 0;

		TPtr8 videoBuffer(iCurrentBuffer->pBuffer + iCurrentBuffer->nOffset + iCurrentBuffer->nFilledLen,
				0,
				static_cast<TInt>(iCurrentBuffer->nAllocLen - iCurrentBuffer->nOffset - iCurrentBuffer->nFilledLen));
		TInt error = i3GPParser->GetVideoDecoderSpecificInfo(videoBuffer);

		if (error == KErrNone)
			{
			iCurrentBuffer->nFilledLen = videoBuffer.Length();
			iCurrentBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
			iVideoHeadersSent = ETrue;
			SubmitBuffer();
			CompleteSelf();
			}
		else
			{
			HandleIfError(error);
			}
		return;
		}

	iAsyncBuf.Set(&iCurrentBuffer->pBuffer[0]
	              ,static_cast<TInt>((iCurrentBuffer->nFilledLen + iCurrentBuffer->nOffset))
	              ,static_cast<TInt>(iCurrentBuffer->nAllocLen));
	i3GPParser->ReadVideoFrame(*this, iAsyncBuf);
	}

void C3GPDemuxer::AudioFramesAvailable(TInt aError, TUint /*aReturnedFrames*/,
		TUint aTimeStampInMs, TUint /*aTimeStampInTimescale*/)
	{
	HandleIfError(aError);

	if (!iInvalid)
		{
		// Check if this is the last frame. It is the last frame if
		// GetAudioFramesSize returns KErrNotFound
		TUint frameSize = 0;
		TInt nextFrameError = i3GPParser->GetAudioFramesSize(frameSize);

		FileReadComplete(nextFrameError, aTimeStampInMs, iFirstAudioFrame, EFalse);
		}
	}

void C3GPDemuxer::VideoFrameAvailable(TInt aError, TBool aKeyFrame, TUint
aTimeStampInMs, TUint /*aTimeStampInTimescale*/)
	{
	HandleIfError(aError);

	if (!iInvalid)
		{
		// Check if this is the last frame. It is the last frame if
		// GetVideoFramesSize returns KErrNotFound
		TUint frameSize = 0;
		TInt nextFrameError = i3GPParser->GetVideoFrameSize(frameSize);

		FileReadComplete(nextFrameError, aTimeStampInMs, iFirstVideoFrame, aKeyFrame);
		}
	}

void C3GPDemuxer::FileReadComplete(TInt aNextFrameError, TUint32 aTimeStamp, TBool& aFirstFrame, TBool aKeyFrame)
	{
	// aNextFrameError is the error code returned when checking the size of the
	// next audio or video frame. If the error code is KErrNotFound, this
	// shows that no more frames exist in that stream so the end of stream flag
	// should be set when sending out the buffer.
	if (aNextFrameError == KErrNotFound)
		{
		iPort[iCurrentPort]->iEOS = ETrue;
		}
	else
		{
		HandleIfError(aNextFrameError);
		}

	if (!iInvalid)
		{
		iCurrentBuffer->nFilledLen += iAsyncBuf.Length();

		// Set presentation time 	
		iCurrentBuffer->nTimeStamp = aTimeStamp * 1000;

		if (aFirstFrame)
			{
			iCurrentBuffer->nFlags |= OMX_BUFFERFLAG_STARTTIME;
			aFirstFrame = EFalse;
			}
		else
			{
			iCurrentBuffer->nFlags &= ~OMX_BUFFERFLAG_STARTTIME;
			}

		if (aKeyFrame)
			{
			iCurrentBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
			}
		else
			{
			iCurrentBuffer->nFlags &= ~OMX_BUFFERFLAG_SYNCFRAME;			
			}

		// Demuxer just puts one whole frame into a buffer so we
		// can set the end of frame flag
		iCurrentBuffer->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

		SubmitBuffer();
		CompleteSelf();
		}
	}

void C3GPDemuxer::SubmitBuffer()
	{
	if (!iPaused)
		{
		if (iPort[iCurrentPort]->iEOS)
			{
			iCurrentBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
			iCallbacks.EventNotification(OMX_EventBufferFlag, iCurrentPort,
					iCurrentBuffer->nFlags, NULL);
			}
		#if 0
		else if (iCurrentBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME)
			{
			iCallbacks.EventNotification(OMX_EventBufferFlag, iCurrentPort,
				iCurrentBuffer->nFlags, NULL);
			}
		#endif

		iCallbacks.BufferDoneNotification(iCurrentBuffer, iCurrentPort,
				OMX_DirOutput);
	
		iState = EStateWaitingForBuffer;
		NextPort();	
		}
	else
		{
		iState = EStateWaitingToSubmit;
		}
	}

void C3GPDemuxer::CreateBufferQueueL()
	{
	DEBUG_PRINTF(_L8("C3GPDemuxer::CreateBufferQueueL++"));

	iPort[COmxIL3GPDemuxer::EPortIndexTimeInput] = CPort::NewL(KMaxTimeBuffers);
	iPort[COmxIL3GPDemuxer::EPortIndexAudioOutput] = CPort::NewL(KMaxAudioBuffers);
	iPort[COmxIL3GPDemuxer::EPortIndexVideoOutput] = CPort::NewL(KMaxVideoBuffers);

	User::LeaveIfError(iBufferQueue.CreateLocal(KMaxTimeBuffers + KMaxAudioBuffers + KMaxVideoBuffers));
	iBufferQueueCreated = ETrue;
	iState = EStateWaitingToStart;
	StartWaitingForBuffer();

	DEBUG_PRINTF(_L8("C3GPDemuxer::CreateBufferQueueL--"));
	}

void C3GPDemuxer::DeleteBufferQueue()
	{
	if (iBufferQueueCreated)
		{
		Cancel();
		iBufferQueueCreated = EFalse;
		iBufferQueue.Close();
		}

	for (TInt port = 0; port < COmxIL3GPDemuxer::EPortIndexMax; ++port)
		{
		delete iPort[port];
		iPort[port] = NULL;
		}
	}

void C3GPDemuxer::HandleIfError(TInt aError)
	{
	OMX_ERRORTYPE omxError = SymbianOSErrorToOmx(aError);

	if (omxError != OMX_ErrorNone)
		{
		iInvalid = ETrue;
		iCallbacks.ErrorEventNotification(omxError);
		}
	}

// Helper function to convert Symbian OS standard error code to c style error code 
OMX_ERRORTYPE C3GPDemuxer::SymbianOSErrorToOmx(TInt aError) const
	{
	OMX_ERRORTYPE error = OMX_ErrorUndefined;

	switch (aError)
		{
		case (KErrNone):
			{
			error = OMX_ErrorNone;
			}
			break;
		case (KErrNoMemory):
			{
			error = OMX_ErrorInsufficientResources;
			}
			break;
		case (KErrOverflow):
			{
			error = OMX_ErrorOverflow;
			}
			break;
		case (KErrAccessDenied):
			{
			error = OMX_ErrorContentPipeOpenFailed;
			}
			break;
		case (KErrCorrupt):
			{
			error = OMX_ErrorStreamCorrupt;
			}
			break;
		case (KErrArgument):
			{
			error = OMX_ErrorUnsupportedSetting;
			}
			break;
		default:
			{
			error = OMX_ErrorUndefined;
			}
		}
	return error;
	}

void C3GPDemuxer::ReceiveQueuedBuffers()
	{
	if (iBufferQueueCreated)
		{
		TBufferMessage message;
		while(iBufferQueue.Receive(message) != KErrUnderflow)
			{
			// Port buffers are pre-reserved so append should always work
			if (iPort[message.iPortIndex]->iBuffers.Append(message.iBufferHeader) != KErrNone)
				{
				Panic(EReceiveQueuedBuffersAppendFailed);
				}
			}
		}
	}

void C3GPDemuxer::StartWaitingForBuffer()
	{
	if (iBufferQueueCreated)
		{
		iWaitingOnBufferQueue = ETrue;
		iBufferQueue.NotifyDataAvailable(iStatus);
		SetActive();
		}
	}

void C3GPDemuxer::CompleteSelf()
	{
	iStatus = KRequestPending;
	SetActive();
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrNone);
	}

TInt C3GPDemuxer::NextPort()
	{
	++iCurrentPort;
	iCurrentPort %= COmxIL3GPDemuxer::EPortIndexMax;
	return iCurrentPort;
	}

/** Packs four bytes into a 32bit number */
TUint32 C3GPDemuxer::Pack32(const TUint8* aPtr)
	{
	TUint32 x = *aPtr++ << 24;
	x |= *aPtr++ << 16;
	x |= *aPtr++ << 8;
	x |= *aPtr++;
	return x;
	}

TInt C3GPDemuxer::SetPosition(TUint aTimePosition, TBool aKeyFrame)
	{
	TUint audioPosition = 0;
	TUint videoPosition = 0;

	TInt err = i3GPParser->Seek(aTimePosition, aKeyFrame, audioPosition, videoPosition);
	if(err)
	    {
	    return err;
	    }
	
	// clear EOS state so buffer handling resumes if previously hit EOS
	// TODO is this thread safe?
	iPort[0]->iEOS = EFalse;
	iPort[1]->iEOS = EFalse;
	
	return KErrNone;
	}
