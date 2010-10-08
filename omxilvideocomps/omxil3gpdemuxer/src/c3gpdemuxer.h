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

#ifndef C3GPDEMUXER_H
#define C3GPDEMUXER_H

#include <f32file.h>
#include <e32msgqueue.h>
#include <c3gplibrary.h>

#include "comxil3gpdemuxer.h"
#include "tvideoformat.h"
#include "taudioformat.h"

class MOmxILCallbackNotificationIf;

NONSHARABLE_CLASS(C3GPDemuxer) : public CActive
								,public M3GPParseCallback

	{
public:	
	static C3GPDemuxer* NewL(MOmxILCallbackNotificationIf& aCallbacks);
	~C3GPDemuxer();

	OMX_ERRORTYPE AcquireResources(const TDesC& aFilename);
	void ReleaseResources();
	void Start();
	OMX_ERRORTYPE Stop();
	void Pause();
	TBool Invalid() const;

	void ProcessThisBufferL(OMX_BUFFERHEADERTYPE* aBufferHeader, TUint32 aPortIndex);
	void FlushBuffers(TUint32 aPortIndex);
	TBool RemoveBuffer(OMX_BUFFERHEADERTYPE* aBufferHeader, OMX_DIRTYPE aDirection);

	TBool GetVideoFormat(TSize& aFrameSize, TVideoFormat& aFormat) const;
	TBool GetAudioFormat(TAudioFormat& aFormat) const;
	OMX_ERRORTYPE GetVideoTimestamp(OMX_TICKS& aOmxTicks);
	OMX_ERRORTYPE Seek(const OMX_TICKS& aOmxTicks, OMX_TIME_SEEKMODETYPE aSeekModeType);
	OMX_ERRORTYPE DetectStreams();
	OMX_ERRORTYPE GetMetadataL(OMX_CONFIG_METADATAITEMTYPE* aMetadata);

protected:
	void RunL();
	void DoCancel();

private:
	typedef RPointerArray<OMX_BUFFERHEADERTYPE> RQueuedBuffers;

	class CPort : public CBase
		{
	public:
		static CPort* NewL(TInt aBufferCount);
		~CPort();
	
	private:	
		CPort();	
		void ConstructL(TInt aBufferCount);

	public:
		// Queue of buffers waiting to be processed. Buffers are not owned.
		RQueuedBuffers iBuffers;
		TBool iEOS;
		};
		
	class TBufferMessage
		{
	public:
		OMX_BUFFERHEADERTYPE* iBufferHeader;
		TUint32 iPortIndex;
		};

	enum TState
		{
		EStateWaitingToStart,
		EStateWaitingForBuffer,
		EStateFillingBuffer,
		EStateWaitingToSubmit
		};

private:
	C3GPDemuxer(MOmxILCallbackNotificationIf& aCallbacks);
	void ConstructL();
	void DoFlushBuffers(TUint32 aPortIndex);
	TBool ProcessBuffers();
	void DoProcessBuffer();
	void ProcessTimeBuffer();
	void FillAudioBuffer();
	void FillVideoBuffer();
	void FileReadComplete(TInt aNextFrameError, TUint32 aTimeStamp, TBool& aFirstFrame, TBool aKeyFrame);
	void SubmitBuffer();
	void CreateBufferQueueL();
	void DeleteBufferQueue();
	void HandleIfError(TInt aError);
	OMX_ERRORTYPE SymbianOSErrorToOmx(TInt aError) const;
	void ReceiveQueuedBuffers();
	void StartWaitingForBuffer();
	void CompleteSelf();
	TInt NextPort();
	TUint32 Pack32(const TUint8* aPtr);
	TInt SetPosition(TUint aTimePosition, TBool aKeyFrame);

	//M3GPParseCallback functions
	void AudioFramesAvailable(TInt aError, TUint aReturnedFrames, TUint aTimeStampInMs, TUint aTimeStampInTimescale);
	void VideoFrameAvailable(TInt aError, TBool aKeyFrame, TUint aTimeStampInMs, TUint aTimeStampInTimescale);
	
private:
	MOmxILCallbackNotificationIf& iCallbacks;
	TUint32 iCurrentPort;
	OMX_BUFFERHEADERTYPE* iCurrentBuffer;  // Not owned
	CPort* iPort[COmxIL3GPDemuxer::EPortIndexMax];
	C3GPParse* i3GPParser;

	TBool iVideoPropertiesRead;
	T3GPVideoType iVideoType;
	TVideoFormat iOmxVideoFormat;
	TUint32 iVideoWidth;
	TUint32 iVideoHeight;
	
	TBool iAudioPropertiesRead;
	T3GPAudioType iAudioType;
	TAudioFormat iOmxAudioFormat;

	TBool iInvalid;
	TBool iPaused;
	TState iState;

	TBool iVideoHeadersSent;
	TBool iAudioHeadersSent;
	TBool iFirstVideoFrame;
	TBool iFirstAudioFrame;

	RMsgQueue<TBufferMessage> iBufferQueue;
	TBool iBufferQueueCreated;
	TBool iWaitingOnBufferQueue;

	TPtr8 iAsyncBuf;

	TBool iParserOpened;
	TUint iStartTimePosition;
	TBool iStartKeyFrame;
	TUint iSeekPosition; // The requested seek time in ms.
	};

#endif //C3GPDEMUXER_H
