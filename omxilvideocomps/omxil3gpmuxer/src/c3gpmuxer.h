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

#ifndef C3GPMUXER_H
#define C3GPMUXER_H

#include <e32base.h>
#include <e32std.h>
#include <f32file.h>
#include <e32msgqueue.h>
#include <openmax/il/khronos/v1_x/OMX_Core.h>
#include <c3gplibrary.h>

#include "comxil3gpmuxer.h"
#include "endwaitao.h"



class MOmxILCallbackNotificationIf;

NONSHARABLE_CLASS(C3GPMuxer) : public CActive
	{
	
	friend class C3GPMuxerWriterThreadObserver;
public:	
	static C3GPMuxer* NewL(MOmxILCallbackNotificationIf& aCallbacks);
	~C3GPMuxer();
	
	void ProcessThisBufferL(OMX_BUFFERHEADERTYPE* aBufferHeader, TUint32 aPortIndex);
	void FlushBuffers(TUint32 aPortIndex);
	TBool RemoveBuffer(OMX_BUFFERHEADERTYPE* aBufferHeader);
	void SetFilename(const HBufC* aFilename);
	void SetAudioVideoProperties(OMX_U32& aFrameWidth, OMX_U32& aFrameHeight, 
									OMX_U32& aFramerate, OMX_U32& aBitRate, OMX_U32& aAudioFramerate);
	void StartL(TBool aAudioPortEnabled, TBool aVideoPortEnabled);
	void Pause();
	TBool IsMuxerInvalid() const;
	void HandleError(TInt aError);
	OMX_ERRORTYPE SymbianErrorToOmx(TInt aError);
protected:
	void RunL();	
	void RunAudioVideoL();
	void RunAudioL();
	void RunVideoL();
	void DoCancel();

private:
	C3GPMuxer(MOmxILCallbackNotificationIf& aCallbacks);
	void ConstructL();

	void OpenComposerL();
	void WriteVideoBufferL();
	void WriteAudioBuffer();
	void DoFlushBuffers(TUint32 aPortIndex);
	void ReturnBuffer(OMX_BUFFERHEADERTYPE* aBuffer, TUint32 aPortIndex);
	static TInt ThreadEntryPoint(TAny* aPtr);
	void RunThreadL();
	TBool RemoveFromQueue(RMsgQueue<OMX_BUFFERHEADERTYPE*>& aQueue, OMX_BUFFERHEADERTYPE* aBufferHeader);
	void CalculateVideoTimeScale(OMX_U32& aFramerate);
	TInt CalculateNextFrameDuration();
	TInt CalculateNextFrameDurationPartial(TBool aFrameWritten);

private:
	MOmxILCallbackNotificationIf& iCallbacks;
	C3GPCompose* iComposer;
	const HBufC* iFilename;  // Not owned by this class
	TSize iVideoSize;
	TInt iVideoTimeScale;
	TInt iDefaultVideoFrameDuration;
	TUint iBitRate;
	TBool iAudioPortEnabled;
	TBool iVideoPortEnabled;
	RMsgQueue<OMX_BUFFERHEADERTYPE*> iAudioQueue;
	RMsgQueue<OMX_BUFFERHEADERTYPE*> iVideoQueue;
	OMX_BUFFERHEADERTYPE* iAudioBuffer;	 
	OMX_BUFFERHEADERTYPE* iCurrentVideoBuffer;	 
	TBool iComposerOpened;
	TUint iAudioFrameDuration;
	RThread iThread;
	RMutex iQueMutex;	
	TBool iThreadRunning;
	CEndWaitAO* iEndWaitAO;
	RMsgQueue<OMX_BUFFERHEADERTYPE*> iRemoveQueue;
	TBool iPaused;
	TBool iMuxerInvalid;
	RBuf8 iPartialFrame;
	OMX_BUFFERHEADERTYPE* iNextVideoBuffer;	 
	TInt64 iVideoDuration;
	TUint iRemoveQueueLength;
	TBool iRemoveQueueNeedsReallocation;
	TBool iRequestOutstanding;
	};

#endif //C3GPMUXER_H
