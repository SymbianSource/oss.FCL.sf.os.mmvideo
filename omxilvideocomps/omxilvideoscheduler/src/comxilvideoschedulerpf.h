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
#ifndef COMXILVIDEOSCHEDULERPF_H_
#define COMXILVIDEOSCHEDULERPF_H_

#include <e32std.h>
#include <openmax/il/common/omxilprocessingfunction.h>
#include <openmax/il/khronos/v1_x/OMX_Component.h>
#include "buffercopier.h"


// forward class declarations
class COmxILVideoScheduler;
class CBufferCopierStateMonitor;

static const TInt KRenderTimeListLength = 5;

/**
 * These panics can only be raised in debug builds, and indicate an assertion failure due to programmer error.
 */
enum TVideoSchedulerPanic
	{
	EPanicMutexUnheld,                  // the mutex was unheld where it was expected to be held
	EPanicTimestampEmissionUnordered,   // buffer emission was triggered for a timestamp less than the previous emission's timestamp
	EPanicBadOutputRegulation,          // more output buffers were sent than intended or iSinkPendingBuffer at inappropriate time
	EPanicBadAssociation                // inconsistency between media time info and corresponding buffer header
	};

NONSHARABLE_CLASS(COmxILVideoSchedulerPF) : public COmxILProcessingFunction, public MBufferCopierIf
	{
public:
	static COmxILVideoSchedulerPF* NewL(MOmxILCallbackNotificationIf& aCallbacks, COmxILVideoScheduler& aComponent, OMX_COMPONENTTYPE* aHandle);
	~COmxILVideoSchedulerPF();

	// from COmxILProcessingFunction
	OMX_ERRORTYPE StateTransitionIndication(TStateIndex aNewState);
	OMX_ERRORTYPE BufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection);
	OMX_ERRORTYPE ParamIndication(OMX_INDEXTYPE aParamIndex, const TAny* apComponentParameterStructure);
	OMX_ERRORTYPE ConfigIndication(OMX_INDEXTYPE aConfigIndex, const TAny* apComponentConfigStructure);
	OMX_ERRORTYPE BufferIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);
	OMX_BOOL BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);
	OMX_ERRORTYPE MediaTimeIndication(const OMX_TIME_MEDIATIMETYPE& aTimeInfo);

	// from MBufferCopierIf
	void MbcBufferCopied(OMX_BUFFERHEADERTYPE* aInBuffer, OMX_BUFFERHEADERTYPE* aOutBuffer);
	void MbcBufferFlushed(OMX_BUFFERHEADERTYPE* aBuffer, OMX_DIRTYPE aDirection);
	
	MOmxILCallbackNotificationIf& GetCallbacks();

private:
	class TBufferMessage
		{
	public:
		OMX_BUFFERHEADERTYPE* iBufferHeader;
		OMX_TIME_MEDIATIMETYPE iMediaTimeInfo;
		};
	
private:
	COmxILVideoSchedulerPF(MOmxILCallbackNotificationIf& aCallbacks, COmxILVideoScheduler& aComponent, OMX_COMPONENTTYPE* aHandle);
	void ConstructL();

	TBool FindWaitingBuffer(const OMX_BUFFERHEADERTYPE* aBuffer, const OMX_TICKS& aMediaTime, TInt& aIndex) const;
	void SubmitBufferHeldByPause();
	TBool SendTimedOutputBuffer(OMX_BUFFERHEADERTYPE* aBuffer, const OMX_TIME_MEDIATIMETYPE& aMediaTimeInfo, TInt aIndex);
	void SendOutputBuffer(OMX_BUFFERHEADERTYPE* aBuffer);
	void DoSendOutputBuffer(OMX_BUFFERHEADERTYPE* aBuffer);
	void HandleIfError(OMX_ERRORTYPE aOmxError);
	OMX_ERRORTYPE SymbianErrorToOmx(TInt aError);
	
	void Panic(TVideoSchedulerPanic aPanicCode) const;
	
private:
	COmxILVideoScheduler& iComponent;
	OMX_TICKS iRenderTime;   // time it takes for Graphic Sink to render a frame
	TBool iPausedState;
	CBufferCopierStateMonitor* iBufferCopierStateMonitor;
	RPointerArray<OMX_BUFFERHEADERTYPE> iWaitingBuffers; // all waiting buffers, including those that received time updates
	TUint32 iOutputBufferSentCount;	// Only allowed to send 2 buffers at a time
	RArray<TBufferMessage> iCompletedBuffersHeldByPause;	// buffers that receive time indications while component is paused
	TInt64 iMaxLateness;	// how late a buffer is allowed to be before it is dropped

	// to keep track of ClockState
	OMX_TIME_CONFIG_CLOCKSTATETYPE iClockState;

	// hold on to start time if received before clock enters WaitingForStartTime state
	TInt64 iStartTime;
	TBool iStartTimePending;
	
	// any buffer that is ready to be displayed but sink is not ready to receive
	OMX_BUFFERHEADERTYPE* iSinkPendingBuffer;
	
	TBool iIsClockStopped;
	TBool iInvalid;
    TUint32 iFrameDroppedCount;  // shouldn't drop more than 2 frames at a time when decoder is slow
    OMX_TICKS iTimeStamp;
    RMutex iMutex;
    OMX_BOOL iEnableDropFrameEvent;  //enable the extension to notify error when drop frame happen

	TUint32 iRenderTimeList[KRenderTimeListLength];
	TInt iRenderTimeListPos;
	TUint32 iRenderTimeSum;    // the sum of the values in iRenderTimeList

	OMX_COMPONENTTYPE* iHandle;
	};

#endif /*CCOMXILVIDEOSCHEDULERPF_H_*/
