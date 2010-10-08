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


// NOTE: Assumes OMX_SKIP64BIT is not set when building OMX CORE

#include <hal.h>
#include <openmax/il/khronos/v1_x/OMX_Other.h>	// OMX port
#include <openmax/il/common/omxilspecversion.h>	// OMX version number
#include "clocksupervisor.h"
#include "comxilclockprocessingfunction.h"
#include "clockpanics.h"
#include "omxilclock.hrh"

#include "log.h"

//////////////
// File scoped constants
//

/** The maximum number of simultaneous outstanding requests (across all ports) */
static const TUint KMaxRequests = 20;
static const TUint KThreadStackSize = 1024;

/**
 * Assumption that timer will always take at least this long to complete.
 * How soon an outstanding delay must be before we complete immediately instead of setting a timer.
 */
static const TInt KMinTimerOverhead = 1000;	// 1000usecs = 1ms

// assumption that timers go off at some time rounded to this many microseconds
static const TInt KTimerQuantization = 
#ifdef __WINSCW__
	15000;
#else
	1;
#endif

// aim to select a counter accurate to ~1ms
#ifdef __WINSCW__
#define USE_FASTCOUNTER // typically faster than 1ms on emulator, 1ms on hardware
#else
#define USE_NTICKCOUNT	// typically 5ms on emulator, 1ms on hardware
#endif

#if !defined(USE_FASTCOUNTER) && !defined(USE_NTICKCOUNT)
#error Either USE_FASTCOUNTER or USE_NTICKCOUNT must be defined
#endif

/**
 * Structure to map OMX_INDEXTYPE to behaviour required by the Clock component.
 */

// Indexes for time configs start after OMX_IndexTimeStartUnused = 0x09000000
// Simply deduct this to index this table

static const OMX_INDEXTYPE KMinJumpTableIndex = OMX_IndexConfigTimeScale;
static const OMX_INDEXTYPE KMaxJumpTableIndex = OMX_IndexConfigTimeClientStartTime;

const CClockSupervisor::FunctionPtr CClockSupervisor::iJumpTable[] = 
	{
	/*OMX_IndexConfigTimeScale*/					&CClockSupervisor::HandleGetSetTimeScale	     ,// requires buffers
	/*OMX_IndexConfigTimeClockState*/				&CClockSupervisor::HandleGetSetClockState	     ,// requires buffers
	/*OMX_IndexConfigTimeActiveRefClock*/			&CClockSupervisor::HandleGetSetActiveRefClock  ,
	/*OMX_IndexConfigTimeCurrentMediaTime*/			&CClockSupervisor::HandleQueryCurrentMediaTime ,
	/*OMX_IndexConfigTimeCurrentWallTime*/			&CClockSupervisor::HandleQueryCurrentWallTime	 ,
	/*OMX_IndexConfigTimeCurrentAudioReference*/	&CClockSupervisor::HandleUpdateAudioReference	 ,
	/*OMX_IndexConfigTimeCurrentVideoReference*/	&CClockSupervisor::HandleUpdateVideoReference	 ,
	/*OMX_IndexConfigTimeMediaTimeRequest*/			&CClockSupervisor::HandleSubmitMediaTimeRequest,// requires buffers
	/*OMX_IndexConfigTimeClientStartTime*/			&CClockSupervisor::HandleSetPortClientStartTime,
	};

#ifdef _DEBUG
#define CHECK_DEBUG()	DbgCheck()
#else
#define CHECK_DEBUG()
#endif


//////////////


/**
 * Euclid's algorithm.
 * Returns the largest common factor of aX and aY.
 */
static TInt LargestCommonFactor(TInt aX, TInt aY)
	{
	// based on knowledge that lcf(x,0)=x, lcf(x,y)=lcf(y,x) and lcf(x,y)=lcf(y,x%y)
	while(aX != 0)
		{
		aY %= aX;
		if(aY == 0)
			{
			return aX;
			}
		aX %= aY;
		}
	return aY;
	}


/**
 *
 *
 */
CClockSupervisor* CClockSupervisor::NewL(COmxILClockProcessingFunction& aProcessingFunction)
	{
	CClockSupervisor* self = new (ELeave) CClockSupervisor(aProcessingFunction);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


/**
 *
 *
 */
void CClockSupervisor::ConstructL()
	{
	// calculate tick frequency
	//
#ifdef USE_FASTCOUNTER
	TInt frequency;	// tick frequency in Hz
	User::LeaveIfError(HAL::Get(HAL::EFastCounterFrequency, frequency));
	// conversion factor from ticks into microseconds
	// using a fraction in integer arithmetic
	iMicroConvNum = 1000000;
	iMicroConvDen = frequency;
	TInt countsUp;
	User::LeaveIfError(HAL::Get(HAL::EFastCounterCountsUp, countsUp));
	iSystemClockReversed = !countsUp;
#elif defined(USE_NTICKCOUNT)
	User::LeaveIfError(HAL::Get(HAL::ENanoTickPeriod, iMicroConvNum)); // tick period in microseconds
	iMicroConvDen = 1;
#else	
#error
#endif
	
	// divide out any common factor to reduce chance of overflow
	TInt factor = LargestCommonFactor(iMicroConvNum, iMicroConvDen);
	iMicroConvNum /= factor;
	iMicroConvDen /= factor;

	// wraparound time in microseconds is 2^32 * iMicroConv
	// take the heartbeat interval as half of this (i.e shift left by 31 places) to ensure we wake up often enough to implement the carry properly
	TUint64 interval = (static_cast<TUint64>(iMicroConvNum) << 31) / iMicroConvDen;
	if (interval > KMaxTInt32)
		{
		iHeartbeatTimerInterval = KMaxTInt32;
		}
	else
		{
		iHeartbeatTimerInterval = interval;
		}

	// if denominator is a power of 2, use shift instead
	// (shifting is faster than division)
	if(iMicroConvDen & (iMicroConvDen - 1) == 0)
		{
		// PRECONDITION:  iMicroConvDen = 2^n, iMicroConvShift = 0
		// POSTCONDITION: iMicroConvDen = 1,   iMicroConvShift = n
		while(iMicroConvDen >= 2)
			{
			iMicroConvDen >>= 1;
			iMicroConvShift++;
			}
		}

	// Create the locks
	User::LeaveIfError(iQueMutex.CreateLocal());
	
	// Create the memory block of empty Time requests that make up the free list.
	iRequestBlock = new(ELeave) TMediaRequest [iMaxRequests];
	
	// zero the fields for peace of mind
	Mem::FillZ(iRequestBlock, (sizeof(TMediaRequest)*iMaxRequests));
	
   	// Initialise the free list
   	for(TInt requestIndex = 0; requestIndex < iMaxRequests; requestIndex++)
   		{
   		// Add all free items with delta 0
   		iFreeRequestQue.Add(iRequestBlock+requestIndex, static_cast<TInt64>(0));
   		}
   	
   	iStartTimes.ReserveL(KNumPorts);
   	for(TInt portIndex = 0; portIndex < KNumPorts; portIndex++)
   		{
   		iStartTimes.Append(0);
   		}
   	
	__ASSERT_DEBUG(iMaxRequests == iFreeRequestQue.Count(), Panic(ERequestQueueCorrupt));

	// Create the timer consumer thread
	TBuf<19> threadName;
	threadName.Format(_L("OmxILClock@%08X"), this);
	
	// Timer created on creation of the thread as it is thread relative
	User::LeaveIfError(iThread.Create(threadName, ThreadEntryPoint, KThreadStackSize, NULL, this));

	// High priority thread
	iThread.SetPriority(EPriorityRealTime);
	// start the thread and wait for it to create the timer
	TRequestStatus status;
	iThread.Rendezvous(status);
	iThread.Resume();
	iThreadStarted = ETrue;
	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());
	}


/**
 *
 *
 */
CClockSupervisor::CClockSupervisor(COmxILClockProcessingFunction& aProcessingFunction) 
:	iActiveRefClock(OMX_TIME_RefClockAudio),
	iMaxRequests(KMaxRequests),
	iProcessingFunction(aProcessingFunction)
	{
	// compile time assertion that fields requiring atomic access are 
	// 4-byte aligned
	__ASSERT_COMPILE((_FOFF(CClockSupervisor, iWallTicks) & 3) == 0);

	// run time assertion that class itself is allocated on a 4-byte boundary
	__ASSERT_ALWAYS((reinterpret_cast<TInt>(this) & 3) == 0, Panic(EBadAlignment));

	iMediaClockState.nSize = sizeof(iMediaClockState);
	iMediaClockState.nVersion = TOmxILSpecVersion();
	iMediaClockState.eState = OMX_TIME_ClockStateStopped;
	
	iCancelStatus = KRequestPending;
	}

/**
 *
 *
 */
CClockSupervisor::~CClockSupervisor()
	{
	// signal the timing thread and wait for it to terminate
	if(iThreadStarted)
		{
		iThreadRunning = EFalse;
		TRequestStatus logonStatus;
		iThread.Logon(logonStatus);
		// Want the thread to terminate ASAP instead of waiting for a media
		// time request completion or a wall time heartbeat. Can't cancel the
		// timer without duplicating the handle, and that gives
		// KErrPermissionDenied. So we use a second TRequestStatus to wake the
		// timing thread before the timer completes.
		TRequestStatus* cancelStatus = &iCancelStatus;
		iThread.RequestComplete(cancelStatus, KErrCancel);
		User::WaitForRequest(logonStatus);
		}
	
	if (iRequestBlock)
		{
		delete[] iRequestBlock;
		iRequestBlock = NULL;
		}
	
	iStartTimes.Close();
	iThread.Close();
	iQueMutex.Close();
	}


/**
 * Timing thread entry point.
 */
TInt CClockSupervisor::ThreadEntryPoint(TAny* aPtr)
	{
	CClockSupervisor& supervisor = *static_cast<CClockSupervisor*>(aPtr);
	supervisor.iThreadRunning = ETrue;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if(cleanup == NULL)
		{
		return KErrNoMemory;
		}

	// Delegate to object's clock timing routine
	TRAPD(err, supervisor.RunTimingThreadL());
	delete cleanup;
	return err;
	}


/**
 *
 *
 */
void CClockSupervisor::RunTimingThreadL()
	{
	// Create the timer (thread relative)
	User::LeaveIfError(iTimer.CreateLocal());
	// rendezvous with the creating thread as it must be blocked until the timer is created
	iThread.Rendezvous(KErrNone);
	
	// Start the timer loop to enable us to wake up on heart beat or requests
	TimerLoop();
	
	iTimer.Close();
	}

/**
 * Services media time requests.
 */
void CClockSupervisor::TimerLoop()
	{
	// Here we are only interested in setting the timer for a request timeout
	// or the heart beat. States are taken care of in ConsumeRequests().
	// On shutdown the flag iThreadRunning is set to EFalse and the timer
	// completed with KErrCancel or something other than KRequestPending
	// Therefore we no longer try to get another request.
	
	TInt measuredTimerOverhead = KTimerQuantization;
	TRequestStatus timerStatus;
	
	while(iThreadRunning)
		{
		// Call our consumer function
		TInt timeout = ConsumeRequests();
		
		// round down sleep based on overhead and quantization of timers
		timeout -= timeout % KTimerQuantization;
		timeout -= measuredTimerOverhead;
		timeout -= KMinTimerOverhead;
		
		// limit sleep by the heartbeat interval
		if(timeout > iHeartbeatTimerInterval)
			{
			timeout = iHeartbeatTimerInterval;
			}
		
		// Perhaps timeout not positive from ConsumeRequests(), or due to
		// rounding down it is no longer positive. In this case we may spin, so
		// be careful when setting KTimerQuantization, KMinTimerOverhead and
		// KRequestDeltaLimit
		if(timeout <= 0)
			{
			continue;
			}

		iQueMutex.Wait();
		TInt64 actualSleepTime = WallTime();
		iQueMutex.Signal();
		iTimer.HighRes(timerStatus, timeout);			
		// can't cancel iTimer from another thread, instead we use a second
		// TRequestStatus to wake up early, then we can cancel the timer in
		// this thread.
		User::WaitForRequest(timerStatus, iCancelStatus);
		
		if(iCancelStatus.Int() != KRequestPending)
			{
			iTimer.Cancel();
			iCancelStatus = KRequestPending;
			}
		else
			{
			// update measuredTimerOverhead
			iQueMutex.Wait();
			actualSleepTime = WallTime() - actualSleepTime;
			iQueMutex.Signal();
			TInt sleepOverhead = (TInt) (actualSleepTime - timeout);
			
			/* Dampen adjustments to measuredTimerOverhead
			 * 
			 *   measuredTimerOverhead = max(sleepOverhead,
			 *                           avg(measuredTimerOverhead, sleepOverhead))
			 * 
			 * i.e. immediate increase, gradual decrease
			 */
			measuredTimerOverhead = (measuredTimerOverhead + sleepOverhead) >> 1;
			if(measuredTimerOverhead < sleepOverhead)
				{
				measuredTimerOverhead = sleepOverhead;
				}
			}
		}
	DEBUG_PRINTF(_L("Clock thread shutting down..."));
	}


/**
 * Update the wall clock with the system ticks
 *
 */
void CClockSupervisor::UpdateWallTicks()
	{
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	
	TUint32 oldLowPart(I64LOW(iWallTicks));		// save lower 32-bits
#ifdef USE_FASTCOUNTER
	// Gets the fast iCounter.
	// This is the current value of the machine's high resolution timer. 
	// If a high resolution timer is not available, it uses the millisecond timer instead.
	// The freqency of this iCounter can be determined by reading the HAL attribute EFastCounterFrequency.
	TUint32 newLowPart(User::FastCounter());	// set lower 32-bits
	if(iSystemClockReversed)
		{
		newLowPart = -newLowPart;
		}
#elif defined(USE_NTICKCOUNT)
	TUint32 newLowPart(User::NTickCount());		// set lower 32-bits
#else
#error
#endif
	TUint32 newHighPart(I64HIGH(iWallTicks));	// save upper 32-bits
	
	// note: did not use LockedInc() here because:
	// We need a critical section to capture the system time and update the wall clock
	// at the same time.
	// The wall clock is unsigned.
	// LockedInc doesn't stop a preemption directly after the test, only during the inc	
	if (newLowPart < oldLowPart) 
		{
		newHighPart++;
		}
	
	iWallTicks = MAKE_TUINT64(newHighPart,newLowPart);
	}


/**
 * Returns the current wall time in microseconds.
 */
TInt64 CClockSupervisor::WallTime()
	 {
	 __ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	 
	 UpdateWallTicks();
	 
	 // if power of two use shift (faster than division)
	 if (iMicroConvDen == 1)
		 {
		 return iWallTicks * iMicroConvNum >> iMicroConvShift;
		 }
	 return iWallTicks * iMicroConvNum / iMicroConvDen;
	 }

/**
 *
 *
 */
TBool CClockSupervisor::AllStartTimesReported ()
	{
	return !(static_cast<TBool>(iMediaClockState.nWaitMask));
	}

/**
 * Called when timer expires or is cancelled.
 *
 */
TInt CClockSupervisor::ConsumeRequests()
	{
	// Events are consumed here only in the Running state.
	// Waiting events, State change events, etc. are handled elsewhere.
	// These are called broadcast events and have a higher priority 
	// than the request events requested by the clients.
	// This function is called by the TimerLoop only.
	
	// IMPORTANT: every code path through here must result in a call to UpdateWallTicks to ensure
	// that 64-bit time is updated correctly (heartbeat interval makes sure ConsumeRequests is called
	// often enough).
	
	// Our event loop - return if stopped but not for pause (keep heart beat going)
	if(OMX_TIME_ClockStateStopped != iMediaClockState.eState)
		{
		// Acquire the mutex
		iQueMutex.Wait();

		// this can be entered from the timer on heartbeat or on a call to a state change. 
		// Don't want this to happen on a state change!
		if (OMX_TIME_ClockStateWaitingForStartTime == iMediaClockState.eState)
			{
			// We need to check if all clients have reported their start times
			if (!AllStartTimesReported())
				{
				// Not all reported as yet...
				UpdateWallTicks();
				
				// Release the mutex
				iQueMutex.Signal();
				
				// wait until they have! keep heat beat going in case it takes looooong!
				return iHeartbeatTimerInterval;
				}
			else
				{
				// depending on scale decide which start time to use
				CalculateStartTime();

				// no need to check error, we are in the waiting state
				DoTransitionToRunningState();
				
				// Now just go and send the iNext request!
				}
			}
		
		///////////////////////////////
		// There is a request pending !!!
		// 
				
		// check the timeout period against the wall clock to determine if this is a heart beat
		
		if(iMtc.iScaleQ16 == 0)
			{
			// do not pop front of the queue because we are effectively paused
			UpdateWallTicks();
			
			// Release the mutex
			iQueMutex.Signal();
			
			return iHeartbeatTimerInterval;
			}
		
		// try to pop the next pending request
		TInt64 delta;
		TBool present = iPendingRequestQue.FirstDelta(delta);
		
		// Is there nothing there?
		if (!present)
			{
			// Nothing on the queue
			// Here because of heart beat
			UpdateWallTicks();
			
			// Release the mutex
			iQueMutex.Signal();
			
			return iHeartbeatTimerInterval;
			}
		
		// update the delta against clock 
		delta -= WallTime();
		
		// Some time to go before head of queue should be serviced?
		if (delta > KMinTimerOverhead)
			{
			// Release the mutex
			iQueMutex.Signal();
			
			return delta;
			}

		// Request timed out, delta expired!
		// Fulfill the request
		TMediaRequest* request = iPendingRequestQue.RemoveFirst();
		
		// Acquire fulfillment buffer for a specific port
		OMX_BUFFERHEADERTYPE* buffer = iProcessingFunction.AcquireBuffer(request->iPortIndex);
		if(buffer == NULL)
			{
			// starved of buffers!
			DEBUG_PRINTF(_L("ConsumeRequests starved of buffers, transitioning to OMX_StateInvalid"));
			iThreadRunning = EFalse;
			iQueMutex.Signal();
			iProcessingFunction.InvalidateComponent();
			return 0;
			}
		OMX_TIME_MEDIATIMETYPE* mT = reinterpret_cast<OMX_TIME_MEDIATIMETYPE*>(buffer->pBuffer);
		buffer->nOffset = 0;
		buffer->nFilledLen = sizeof(OMX_TIME_MEDIATIMETYPE);
		
		mT->nSize = sizeof(OMX_TIME_MEDIATIMETYPE);
		mT->nVersion = TOmxILSpecVersion();
		mT->eUpdateType = OMX_TIME_UpdateRequestFulfillment;
		mT->nClientPrivate = reinterpret_cast<OMX_U32>(request->iClientPrivate);
		mT->xScale = iMtc.iScaleQ16;
		mT->eState = OMX_TIME_ClockStateRunning;
		mT->nMediaTimestamp = request->iMediaTime;
		mT->nWallTimeAtMediaTime = request->iTriggerWallTime + request->iOffset;
		mT->nOffset = mT->nWallTimeAtMediaTime - WallTime(); // could be waiting on buffer b4 this!

#ifdef _OMXIL_COMMON_DEBUG_TRACING_ON		
		DEBUG_PRINTF(_L8("CLOCK::ConsumeRequest*******************************OMX_TIME_UpdateRequestFulfillment***VS2"));
		TTime t;
		t.HomeTime();
		DEBUG_PRINTF2(_L8("CLOCK::ConsumeRequest : t.HomeTime() = %ld"), t.Int64());
		DEBUG_PRINTF2(_L8("CLOCK::ConsumeRequest : Buffer = 0x%X"), mT->nClientPrivate);
		DEBUG_PRINTF2(_L8("CLOCK::ConsumeRequest : mT->nMediaTimestamp = %ld"), mT->nMediaTimestamp);
#endif 
		
		iProcessingFunction.SendBuffer(buffer);
		
		// clear the delta on this now free request
		request->iTriggerWallTime = 0;
		
		// Add element back to the free pool
		iFreeRequestQue.Add(request, 0);

		// Release the mutex
		iQueMutex.Signal();

		// Update delta for next request.
		// NOTE: we do not know if scale change or not so when we re-enter here
		// we should recalculate the delta above and perform the appropriate action.
		return 0;
		}
	else
		{
		// clock is stopped - sleep for the heartbeat interval
		return iHeartbeatTimerInterval;
		}
	}

/**
 *
 *
 */
OMX_ERRORTYPE CClockSupervisor::ProduceRequest(OMX_INDEXTYPE aIndex, TEntryPoint aEntryPoint, TAny* aPassedStructPtr)
	{
	// Range checking on parameter/config index
	if(aIndex < KMinJumpTableIndex || aIndex > KMaxJumpTableIndex)
		{
		return OMX_ErrorUnsupportedIndex;
		}
	// Note also that certain combinations on Get/Set within the supported range are unsupported.
	// This is left to the function in the jump table.
	
	// Acquire the mutex
	iQueMutex.Wait();
	
	// Index the routing table for the correct handler
	FunctionPtr jumpTableFptr = iJumpTable[aIndex-KMinJumpTableIndex];
	OMX_ERRORTYPE ret = (this->*jumpTableFptr)(aEntryPoint, aPassedStructPtr);
	
	// Release the mutex
	iQueMutex.Signal();
	
	return ret;
	}


/**
 * According to scale, choose the earliest start time among the set of client
 * start times. For forward play this is the minimum, for reverse play this is
 * the maximum.
 */
void CClockSupervisor::CalculateStartTime()
	{
	// The nWaitMask field of the Media clock state is a bit mask specifying the client 
	// components that the clock component will wait on in the 
	// OMX_TIME_ClockStateWaitingForStartTime state. Bit masks are defined 
	// as OMX_CLOCKPORT0 through OMX_CLOCKPORT7.
		
	// Based on scale locate the minimum or maximum start times of all clients
	TInt64 startTime;
	
	if (iMtc.iScaleQ16 >= 0)
		{
		startTime = KMaxTInt64;
		
		// choose minimum
		for (TInt portIndex = 0; portIndex < iStartTimes.Count(); portIndex++)
			{
			if(iStartTimesSet & (1 << portIndex))
				{
				startTime = Min(startTime, iStartTimes[portIndex]);
				}
			}
		}
	else
		{
		startTime = KMinTInt64;
		
		// choose maximum
		for (TInt portIndex = 0; portIndex < iStartTimes.Count(); portIndex++)
			{
			if(iStartTimesSet & (1 << portIndex))
				{
				startTime = Max(startTime, iStartTimes[portIndex]);
				}
			}
		}
	
	// adjust the media time to the new start time
	UpdateMediaTime(startTime);
	}


/**
 * Perform actions related to placing the clock into the Stopped state
 */
void CClockSupervisor::DoTransitionToStoppedState()
	{
	// OMX_TIME_ClockStateStopped: Immediately stop the media clock, clear all
	// pending media time requests, clear all client start times, and transition to the
	// stopped state. This transition is valid from all other states.

	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
		
	// clear any current start time
	iMediaClockState.nStartTime = 0;
	iMediaClockState.nWaitMask = 0;
	
	// clear client start times
	for(TInt index = 0, count = iStartTimes.Count(); index < count; index++)
		{
		iStartTimes[index] = 0;
		}
	iStartTimesSet = 0;
	
	// clear all pending requests, placing them back on the free queue
	while (!iPendingRequestQue.IsEmpty())
		{
		TMediaRequest* request = iPendingRequestQue.RemoveFirst();

		// clear the delta on this now free request
		request->iTriggerWallTime = 0;

		iFreeRequestQue.Add(request,0);
		}

	TInt64 wallTimeNow = WallTime();
	
	// if clock was previously running, stop the media time at the present value
	if(iMediaClockState.eState == OMX_TIME_ClockStateRunning)
		{
		TInt64 mediaTimeNow = ((wallTimeNow - iMtc.iWallTimeBase) * iMtc.iScaleQ16 >> 16) + iMtc.iMediaTimeBase;
		iMtc.iWallTimeBase = wallTimeNow;
		iMtc.iMediaTimeBase = mediaTimeNow;
		}
	
	// Indicate stopped state
	iMediaClockState.eState = OMX_TIME_ClockStateStopped;

	// Indicate to clients a state change
	OMX_TIME_MEDIATIMETYPE update;
	update.nSize = sizeof(OMX_TIME_MEDIATIMETYPE);
	update.nVersion = TOmxILSpecVersion();
	update.nClientPrivate = NULL;
	update.eUpdateType = OMX_TIME_UpdateClockStateChanged;
	update.xScale = iMtc.iScaleQ16;
	update.eState = OMX_TIME_ClockStateStopped;
	update.nMediaTimestamp = iMtc.iMediaTimeBase;
	update.nWallTimeAtMediaTime = wallTimeNow;
	update.nOffset = 0;

	BroadcastUpdate(update);
	}


/**
 * Perform actions related to placing the clock into the Running state
 */
void CClockSupervisor::DoTransitionToRunningState()
	{
	// OMX_TIME_ClockStateRunning: Immediately start the media clock using the given
	// start time and offset, and transition to the running state. This transition is valid from
	// all other states.
	
	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	
	// if we transistioned from stopped to running then start time will be cleared.
	// we only set it on transition from waiting to running
	// this is enforced not by a check but by logic flow!
	// If this is not the case then start time should have been cleared and we can start now!

	// Indicate running state
	iMediaClockState.eState = OMX_TIME_ClockStateRunning;
	
	// Indicate to clients a state change
	OMX_TIME_MEDIATIMETYPE update;
	update.nSize = sizeof(OMX_TIME_MEDIATIMETYPE);
	update.nVersion = TOmxILSpecVersion();
	update.nClientPrivate = NULL;
	update.eUpdateType = OMX_TIME_UpdateClockStateChanged;
	update.xScale = iMtc.iScaleQ16;
	update.eState = iMediaClockState.eState;
	update.nMediaTimestamp = iMtc.iMediaTimeBase;
	update.nWallTimeAtMediaTime = iMtc.iWallTimeBase;
	update.nOffset = 0;
	
	BroadcastUpdate(update);
	}

/**
 * Perform actions related to placing the clock into the Waiting state
 */
void CClockSupervisor::DoTransitionToWaitingState(OMX_U32 nWaitMask)
	{
	// OMX_TIME_WaitingForStartTime: Transition immediately to the waiting state, wait
	// for all clients specified in nWaitMask to report their start time, start the media clock
	// using the minimum of all client start times and transition to
	// OMX_TIME_ClockStateRunning. This transition is only valid from the
	// OMX_TIME_ClockStateStopped state.
	// (*Added*) If in the backwards direction start the media clock using the maximum
	// of all client start times.
	
	// validity of state transition has been checked in HandleGetSetClockState()
	
	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	
	iMediaClockState.eState = OMX_TIME_ClockStateWaitingForStartTime;
	
	// Start times set in calling function HandleClientStartTime()
	
	// Remember all clients that need to report their start times
	iMediaClockState.nWaitMask = nWaitMask;

	// Indicate to clients a state change
	OMX_TIME_MEDIATIMETYPE update;
	update.nSize = sizeof(OMX_TIME_MEDIATIMETYPE);
	update.nVersion = TOmxILSpecVersion();
	update.nClientPrivate = NULL;
	update.eUpdateType = OMX_TIME_UpdateClockStateChanged;
	update.xScale = iMtc.iScaleQ16;
	update.eState = OMX_TIME_ClockStateWaitingForStartTime;
	update.nMediaTimestamp = iMtc.iMediaTimeBase;
	update.nWallTimeAtMediaTime = WallTime();
	update.nOffset = 0;
	
	BroadcastUpdate(update);
	}


 /**
  * Update the wall clock with the system ticks
  *
  */
 OMX_ERRORTYPE CClockSupervisor::HandleSubmitMediaTimeRequest(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
 	{
 	// We should already have the mutex!
 	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
 	
	// A client requests the transmission of a particular timestamp via OMX_SetConfig on its
 	// clock port using the OMX_IndexConfigTimeMediaTimeRequest configuration
 	// and structure OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE

 	// OMX_GetConfig doesn't make any sense for MediaTimeRequest
 	if(aEntry == EGetConfig)
 		{
 		return OMX_ErrorUnsupportedIndex;
 		}
 	
 	TMediaRequest *request = iFreeRequestQue.RemoveFirst();
	if(request == NULL)
		{
		// too many pending requests!
		return OMX_ErrorInsufficientResources;
		}

	OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE* rT = static_cast<OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE*>(aPassedStructPtr);
	
	request->iMediaTime = rT->nMediaTimestamp;
	request->iOffset = rT->nOffset;
	request->iTriggerWallTime = 
		((rT->nMediaTimestamp - iMtc.iMediaTimeBase) * iMtc.iInverseScaleQ16 >> 16) 
			+ iMtc.iWallTimeBase - rT->nOffset;
	request->iPortIndex = rT->nPortIndex;
	request->iClientPrivate = rT->pClientPrivate;
	
	TInt64 prevHeadTime(0);
	TBool nonEmpty = iPendingRequestQue.FirstDelta(prevHeadTime);
	iPendingRequestQue.Add(request, request->iTriggerWallTime);
	// Wake the thread immediately if head of queue now will complete sooner
	// than it would have previously, or if the queue was empty previously.
	// This causes the timing thread to recalculate the sleep time.
	if(!nonEmpty || prevHeadTime > request->iTriggerWallTime)
		{
		TRequestStatus* status = &iCancelStatus;
		iThread.RequestComplete(status, KErrCancel);
		}
	return OMX_ErrorNone;
 	}
 	

 /**
  *
  *
  */
 OMX_ERRORTYPE CClockSupervisor::HandleQueryCurrentWallTime(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
	 {
	 // An IL client may query the current wall time via OMX_GetConfig on OMX_IndexConfigTimeCurrentWallTime.	 
	 // A client may obtain the current wall time, which is obtained via OMX_GetConfig on
	 // OMX_IndexConfigTimeCurrentWallTime.
	 
	 // OMX_SetConfig doesn't make sense for wall time
	 if(aEntry == ESetConfig)
		 {
		 return OMX_ErrorUnsupportedIndex;
		 }
	 
	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	
	OMX_TIME_CONFIG_TIMESTAMPTYPE& wallTs = *static_cast<OMX_TIME_CONFIG_TIMESTAMPTYPE*>(aPassedStructPtr);
	wallTs.nTimestamp = WallTime();
	return OMX_ErrorNone;
	}


/**
 * Calculates and returns the current media time.
 */
OMX_ERRORTYPE CClockSupervisor::HandleQueryCurrentMediaTime(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
	{
	// The clock component can be queried for the current media clock time using
	// OMX_GetConfig with the read-only index OMX_IndexConfigTimeCurrentMediaTime and structure
	// OMX_TIME_CONFIG_TIMESTAMPTYPE.	

	// OMX_SetConfig cannot be used for Media Time (audio or video reference updates are used instead)
	if(aEntry == ESetConfig)
		{
		return OMX_ErrorUnsupportedIndex;
		}
	
	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	
	OMX_TIME_CONFIG_TIMESTAMPTYPE* ts = static_cast<OMX_TIME_CONFIG_TIMESTAMPTYPE*>(aPassedStructPtr);
	if(iMediaClockState.eState == OMX_TIME_ClockStateRunning)
		{
		ts->nTimestamp = ((WallTime() - iMtc.iWallTimeBase) * iMtc.iScaleQ16 >> 16) + iMtc.iMediaTimeBase;
		}
	else
		{
		ts->nTimestamp = iMtc.iMediaTimeBase;
		}
	return OMX_ErrorNone;
	}

/**
 * An external component is providing an Audio reference time update.
 */
OMX_ERRORTYPE CClockSupervisor::HandleUpdateAudioReference(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
	{
	// The clock component can accept an audio reference clock. 
	// The reference clock tracks the media time at its associated component 
	// (i.e., the timestamp of the data currently being processed at that component) 
	// and provides periodic references to the clock component via OMX_SetConfig 
	// using OMX_IndexConfigTimeCurrentAudioReference, and structure OMX_TIME_CONFIG_TIMESTAMPTYPE
	//
	// When the clock component receives a reference, it updates its internally maintained
	// media time with the reference. This action synchronizes the clock component with the
	// component that is providing the reference clock.
	
	// OMX_GetConfig not supported on reference time as it will generally be
	// some arbitary time in the past
	if(aEntry == EGetConfig)
		{
		return OMX_ErrorUnsupportedIndex;
		}
	
	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));

	if(iActiveRefClock == OMX_TIME_RefClockAudio)
		{
		OMX_TIME_CONFIG_TIMESTAMPTYPE* ts = static_cast<OMX_TIME_CONFIG_TIMESTAMPTYPE*>(aPassedStructPtr);
		UpdateMediaTime(ts->nTimestamp);
		}
	
	return OMX_ErrorNone;
	}

/**
 * An external component is providing a Video reference time update.
 */
OMX_ERRORTYPE CClockSupervisor::HandleUpdateVideoReference(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
	{
	// The clock component can accept a video reference clock. 
	// The reference clock tracks the media time at its associated component 
	// (i.e., the timestamp of the data currently being processed at that component) 
	// and provides periodic references to the clock component via OMX_SetConfig 
	// using OMX_IndexConfigTimeCurrentVideoReference, and structure OMX_TIME_CONFIG_TIMESTAMPTYPE
	//
	// When the clock component receives a reference, it updates its internally maintained
	// media time with the reference. This action synchronizes the clock component with the
	// component that is providing the reference clock.

	// OMX_GetConfig not supported on reference time as it will generally be
	// some arbitary time in the past
	if(aEntry == EGetConfig)
		{
		return OMX_ErrorUnsupportedIndex;
		}
	
	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	
	if(iActiveRefClock == OMX_TIME_RefClockVideo)
		{
		OMX_TIME_CONFIG_TIMESTAMPTYPE* ts = static_cast<OMX_TIME_CONFIG_TIMESTAMPTYPE*>(aPassedStructPtr);
		UpdateMediaTime(ts->nTimestamp);
		}
	
	return OMX_ErrorNone;
	}

 /**
  *
  *
  */
 OMX_ERRORTYPE CClockSupervisor::HandleSetPortClientStartTime(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
	{
	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	
	// When a clock component client receives a buffer with flag OMX_BUFFERFLAG_STARTTIME set, 
	// it performs an OMX_SetConfig call with OMX_IndexConfigTimeClientStartTime 
	// on the clock component that is sending the buffer’s timestamp. 
	// The transmission of the start time informs the clock component that the client’s stream 
	// is ready for presentation and the timestamp of the first data to be presented.
	// 
	// If the IL client requests a transition to OMX_TIME_ClockStateWaitingForStartTime, it
	// designates which clock component clients to wait for. The clock component then waits
	// for these clients to send their start times via the
	// OMX_IndexConfigTimeClientStartTime configuration. Once all required
	// clients have responded, the clock component starts the media clock using the earliest
	// client start time.
	// 
	// When a client is sent a start time (i.e., the timestamp of a buffer marked with the
	// OMX_BUFFERFLAG_STARTTIME flag ), it sends the start time to the clock component
	// via OMX_SetConfig on OMX_IndexConfigTimeClientStartTime. This
	// action communicates to the clock component the following information about the client’s
	// data stream:
	// - The stream is ready.
	// - The starting timestamp of the stream
	
	// TODO Perhaps OMX_GetConfig can be done for client start time, after all
	// the start times on each port have been stored. But for now this is not
	// supported.
	if(aEntry == EGetConfig)
		{
		return OMX_ErrorUnsupportedIndex;
		}
	
	if(iMediaClockState.eState != OMX_TIME_ClockStateWaitingForStartTime)
		{
		return OMX_ErrorIncorrectStateOperation;
		}
	
	OMX_TIME_CONFIG_TIMESTAMPTYPE* state = static_cast<OMX_TIME_CONFIG_TIMESTAMPTYPE*>(aPassedStructPtr);
	iMediaClockState.nWaitMask &= ~(1 << state->nPortIndex); // switch off bit
	iStartTimesSet |= 1 << state->nPortIndex;	// switch on bit
	iStartTimes[state->nPortIndex] = state->nTimestamp;
	
	if(iMediaClockState.nWaitMask == 0)
		{
		// cancel timer so transition occurs immediately instead of waiting for a heartbeat
		TRequestStatus* cancelStatus = &iCancelStatus;
		iThread.RequestComplete(cancelStatus, KErrCancel);
		}
	
	return OMX_ErrorNone;
	}

/**
 * Sets the current media time to the specified value.
 */
void CClockSupervisor::UpdateMediaTime(TInt64 aMediaTime)
	{
#ifdef _OMXIL_COMMON_DEBUG_TRACING_ON
	OMX_TIME_CONFIG_TIMESTAMPTYPE ts;
	HandleQueryCurrentMediaTime(EGetConfig, &ts);
	DEBUG_PRINTF4(_L8("Clock::UpdateMediaTime=[%ld]currentmediaTime=<%d> MediaTime <%ld>"), ts.nTimestamp-aMediaTime,ts.nTimestamp, aMediaTime);
#endif // _OMXIL_COMMON_DEBUG_TRACING_ON
	iMtc.iWallTimeBase = WallTime();
	iMtc.iMediaTimeBase = aMediaTime;
	iPendingRequestQue.RecalculateAndReorder(iMtc);
	
	TRequestStatus* status = &iCancelStatus;
    iThread.RequestComplete(status, KErrCancel);
	}

 /**
  *
  *
  */
 OMX_ERRORTYPE CClockSupervisor::HandleGetSetTimeScale(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
	{
	// The IL client queries and sets the media clock’s scale via the
	// OMX_IndexConfigTimeScale configuration, passing  structure
	// OMX_TIME_CONFIG_SCALETYPE 

	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));
	
	OMX_TIME_CONFIG_SCALETYPE* sT = static_cast<OMX_TIME_CONFIG_SCALETYPE*>(aPassedStructPtr);

	if (EGetConfig == aEntry)
		{
		sT->xScale = iMtc.iScaleQ16;
		}
	else 
		{ // ESetConfig
		
		if(sT->xScale == iMtc.iScaleQ16)
			{
			// do not broadcast update if the scale changed to the same value
			// IL client causes the scale change but only IL components receive the notification
			return OMX_ErrorNone;
			}
		iMtc.SetScaleQ16(sT->xScale, WallTime());
		
		// Reorder before sending notifications
		iPendingRequestQue.RecalculateAndReorder(iMtc);
		
		// Indicate to clients a scale change
		// The buffer payload is written here then copied to all clients' buffers
		// It should be noted that as this is happening across all ports, time can pass
		// making nMediaTimestamp and nWallTimeAtMediaTime inaccurate. Since at present we do
		// not recover buffer exhaustion scenarios, it is assumed that the messaging time is short
		// thus we do not recalculate the time.
		
		OMX_TIME_MEDIATIMETYPE update;
		update.nSize = sizeof(OMX_TIME_MEDIATIMETYPE);
		update.nVersion = TOmxILSpecVersion();
		update.nClientPrivate = NULL;
		update.eUpdateType = OMX_TIME_UpdateScaleChanged;
		update.xScale = iMtc.iScaleQ16;
		update.eState = iMediaClockState.eState;
		update.nMediaTimestamp = iMtc.iMediaTimeBase;
		update.nWallTimeAtMediaTime = iMtc.iWallTimeBase;
		update.nOffset = 0;
		
		BroadcastUpdate(update);
		
		// Signal the Timer thread as it may need to fulfill all requests on a direction change,
		// of fulfill some requests as we have moved forward in time
		TRequestStatus* cancelStatus = &iCancelStatus;
		iThread.RequestComplete(cancelStatus, KErrCancel);
		//******************************************
		}
	return OMX_ErrorNone;
	}
 
 
 /**
  *
  *
  */
 OMX_ERRORTYPE CClockSupervisor::HandleGetSetClockState(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
	{
	// An OMX_GetConfig execution using index OMX_IndexConfigTimeClockState
	// and structure OMX_TIME_CONFIG_CLOCKSTATETYPE queries the current clock state.
	// An OMX_SetConfig execution using index OMX_IndexConfigTimeClockState
	// and structure OMX_TIME_CONFIG_CLOCKSTATETYPE commands the clock
	// component to transition to the given state, effectively providing the IL client a
	// mechanism for starting and stopping the media clock.
	// 
	// Upon receiving OMX_SetConfig from the IL client that requests a transition to the
	// given state, the clock component will do the following:
	//
	// - OMX_TIME_ClockStateStopped: Immediately stop the media clock, clear all
	// pending media time requests, clear and all client start times, and transition to the
	// stopped state. This transition is valid from all other states.
	//
	// - OMX_TIME_ClockStateRunning: Immediately start the media clock using the given
	// start time and offset, and transition to the running state. This transition is valid from
	// all other states.
	//
	// - OMX_TIME_WaitingForStartTime: Transition immediately to the waiting state, wait
	// for all clients specified in nWaitMask to report their start time, start the media clock
	// using the minimum of all client start times and transition to
	// OMX_TIME_ClockStateRunning. This transition is only valid from the
	// OMX_TIME_ClockStateStopped state.

	// We should already have the mutex!
	__ASSERT_DEBUG(iQueMutex.IsHeld(), Panic(EMutexUnheld));

	OMX_ERRORTYPE ret = OMX_ErrorNone;
	
	OMX_TIME_CONFIG_CLOCKSTATETYPE* const &clkState 
		= static_cast<OMX_TIME_CONFIG_CLOCKSTATETYPE*>(aPassedStructPtr);
	
	if (ESetConfig == aEntry)
		{		
		if (iMediaClockState.eState == clkState->eState)
			{
			// Already in this state!
			return OMX_ErrorSameState;
			}
		if (clkState->eState != OMX_TIME_ClockStateStopped &&
			clkState->eState != OMX_TIME_ClockStateWaitingForStartTime &&
			clkState->eState != OMX_TIME_ClockStateRunning)
			{
			return OMX_ErrorUnsupportedSetting;
			}
		
		// need buffers to notify clock state changes, so require to be in Executing
		if(!iProcessingFunction.IsExecuting())
			{
			return OMX_ErrorIncorrectStateOperation;
			}
		
		switch (clkState->eState)
			{
			case OMX_TIME_ClockStateStopped:
				{
				DoTransitionToStoppedState();
				break;
				}
			case OMX_TIME_ClockStateWaitingForStartTime:
				{
				// Can't go into this state from Running state
				if (OMX_TIME_ClockStateRunning == iMediaClockState.eState)
					{
					ret = OMX_ErrorIncorrectStateTransition;
					break;
					}
				// Waiting for no ports makes no sense. Also don't allow wait on ports we don't have.
				if (clkState->nWaitMask == 0 || clkState->nWaitMask >= 1 << KNumPorts)
					{
					ret = OMX_ErrorUnsupportedSetting;
					break;
					}
				DoTransitionToWaitingState(clkState->nWaitMask);
				break;
				}			
			case OMX_TIME_ClockStateRunning:
				{
				// set media time to that passed by the IL client
				iMtc.iWallTimeBase = WallTime();
				iMtc.iMediaTimeBase = clkState->nStartTime;
				// changed time base so pending trigger wall times may be different
				iPendingRequestQue.RecalculateAndReorder(iMtc);
				DoTransitionToRunningState();
 				// wake the timer thread to reset timer or service pending updates
				TRequestStatus* statusPtr = &iCancelStatus;
				iThread.RequestComplete(statusPtr, KErrCancel);

				break;
				}
			// default condition already checked before Executing test
			}
		}
	else
		{
		clkState->eState = iMediaClockState.eState;
		}
	
	return ret;
	}
 
 /**
  *
  *
  */
 OMX_ERRORTYPE CClockSupervisor::HandleGetSetActiveRefClock(TEntryPoint aEntry, OMX_PTR aPassedStructPtr)
	{
	// The IL client controls which reference clock the clock component uses (if any) via the
	// OMX_IndexConfigTimeActiveRefClock configuration and structure OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE
	
	// don't need the mutex here as just getting/setting one machine word
	
	OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE& ref = *static_cast<OMX_TIME_CONFIG_ACTIVEREFCLOCKTYPE*>(aPassedStructPtr);

	if (ESetConfig == aEntry)
		{
		if (ref.eClock != OMX_TIME_RefClockAudio &&
			ref.eClock != OMX_TIME_RefClockVideo &&
			ref.eClock != OMX_TIME_RefClockNone)
			{
			return OMX_ErrorUnsupportedSetting;
			}
		iActiveRefClock = ref.eClock;
		}
	else // EGetConfig
		{
		ref.eClock = iActiveRefClock;
		}
	return OMX_ErrorNone;
	}

void CClockSupervisor::BroadcastUpdate(const OMX_TIME_MEDIATIMETYPE& aUpdate)
	{
	// notify state change on all enabled ports
 	for(TInt portIndex = 0; portIndex < KNumPorts; portIndex++)
 		{
 		if(iProcessingFunction.PortEnabled(portIndex))
 			{
 			OMX_BUFFERHEADERTYPE* buffer = iProcessingFunction.AcquireBuffer(portIndex);
 			if(buffer == NULL)
 				{
 				// starved of buffers!
 				iThreadRunning = EFalse;
 				iProcessingFunction.InvalidateComponent();
 				return;
 				}
 			OMX_TIME_MEDIATIMETYPE& mT = *reinterpret_cast<OMX_TIME_MEDIATIMETYPE*>(buffer->pBuffer);
 			mT = aUpdate;
 			buffer->nOffset = 0;
 			buffer->nFilledLen = sizeof(OMX_TIME_MEDIATIMETYPE);
 			iProcessingFunction.SendBuffer(buffer);
 			}
 		}
	}

void CClockSupervisor::ReportClockThreadPanic()
    {
        iThreadRunning = EFalse;
        iProcessingFunction.InvalidateComponent();
    }
/**
 * Adjusts the media time scale.
 * The wall/media time bases are updated so there is no instantaneous change to media time.
 * iInverseScaleQ16 is also recalculated.
 */
void TMediaTimeContext::SetScaleQ16(TInt32 aScaleQ16, TInt64 aWallTimeNow)
	{
	TInt64 mediaTimeNow = ((aWallTimeNow - iWallTimeBase) * iScaleQ16 >> 16) + iMediaTimeBase;
	iWallTimeBase = aWallTimeNow;
	iMediaTimeBase = mediaTimeNow;
	iScaleQ16 = aScaleQ16;
	
	// calculate inverse scale
	// 1.0/scale in Q16 format becomes 2^32/scaleQ16
	// values of -1 and +1 will cause overflow and are clipped
	// division by zero also yields KMaxTInt (2^31 - 1)
	if(iScaleQ16 == 0 || iScaleQ16 == 1)
		{
		iInverseScaleQ16 = 0x7FFFFFFF;
		}
	else if(iScaleQ16 == -1)
		{
		iInverseScaleQ16 = 0x80000000;
		}
	else
		{
		iInverseScaleQ16 = static_cast<TInt32>(0x100000000LL / iScaleQ16);
		}
	}


TRequestDeltaQue::TRequestDeltaQue()
 :	iHead(0),
 	iCount(0)
	 {
	 // do nothing
	 }

 void TRequestDeltaQue::Add(TMediaRequest* aElement, TInt64 aDelta)
	{
	CHECK_DEBUG();
	
	__ASSERT_DEBUG(((iHead == NULL && iCount == 0) || (iHead != NULL && iCount > 0)), 
					Panic(ERequestQueueCorrupt));
	
	if (!iHead)
		{
		iHead = aElement;
		aElement->iPrev = aElement;
		aElement->iNext = aElement;
		}
	else // set the new element links
		{
		TBool front(EFalse);
		TMediaRequest* item(NULL);
		front = InsertBeforeFoundPosition (aDelta, item);
		
		if (front)
			{
			// insert infront BEFORE as we have the lesser delta
			// set up the element
			aElement->iPrev = item->iPrev;
			aElement->iNext = item;
			
			// set up the item before in the list
			item->iPrev->iNext = aElement;
			
			// set up the item after in the list
			item->iPrev = aElement;
			
			// setup the new head
			iHead = aElement;
			}
		else
			{
			// insert this element AFTER the item in the list
			// set up the element
			aElement->iPrev = item;
			aElement->iNext = item->iNext;
			
			// set up the item before in the list
			item->iNext = aElement;
			
			// set up the item after in the list
			aElement->iNext->iPrev = aElement;
			}
		}
	iCount++;
	
#ifdef _OMXIL_COMMON_DEBUG_TRACING_ON
	// print the trigger times in debug mode
	DbgPrint();
#endif
	
	CHECK_DEBUG();
	}

 TBool TRequestDeltaQue::InsertBeforeFoundPosition(TInt64 aDelta, TMediaRequest*& aItem) const
	{
	CHECK_DEBUG();
	
	__ASSERT_DEBUG(((iHead == NULL && iCount == 0) || (iHead != NULL && iCount > 0)), 
						Panic(ERequestQueueCorrupt));
	
	// search for the position where to insert
	// and insert after this
	
	aItem = iHead->iPrev; // tail
	
	// start from the end and linearly work backwards
	while (aItem->iTriggerWallTime > aDelta && aItem != iHead)
		{
		aItem = aItem->iPrev;
		}
	
	// indicates that we insert before the item and not after it
	if (aItem == iHead && aItem->iTriggerWallTime > aDelta)
		{
		return ETrue;
		}

	// no CHECK_DEBUG required as this method is const
	
	return EFalse;
	}
 
/**
 * If scale changes, the iTriggerWallTimes must be recalculated.
 *
 * In addition, because offset is not affected by scale, it is possible for
 * the order of the elements to change. This event is assumed to be rare, and
 * when it does occur we expect the list to remain 'roughly sorted' requiring
 * few exchanges.
 * 
 * So we choose ** bubble sort **.
 *
 * Time recalculation is merged with the first pass of bubble sort. Times are
 * recalculated in the first iteration only. Swaps can occur as necessary in
 * all iterations. We expect that in most cases there will only be one pass
 * with no swaps.
 *
 * Note that bubble sort would be worst-case complexity if reversing the list
 * due to a time direction change. In such cases future requests complete
 * immediately (because they are now in the past). So we do not sort at at all
 * in this case.
 */
 void TRequestDeltaQue::RecalculateAndReorder(TMediaTimeContext& aMtc)
	{
	CHECK_DEBUG();
	
	__ASSERT_DEBUG(((iHead == NULL && iCount == 0) || (iHead != NULL && iCount > 0)), 
						Panic(ERequestQueueCorrupt));
	
	if(iCount == 0)
		{
		// nothing to do
		return;
		}
	// note if there is 1 item there is no reorder but we do need to recalculate
	
	TBool swapped(EFalse);
	TBool deltaCalculated(EFalse);

	do
		{
		// start from end of queue
		swapped = EFalse;
		TMediaRequest* item(iHead->iPrev); // tail
		
		if (!deltaCalculated)
			{
			// calculate the tails new delta
			item->iTriggerWallTime = 
				((item->iMediaTime - aMtc.iMediaTimeBase) * aMtc.iInverseScaleQ16 >> 16) 
					+ aMtc.iWallTimeBase - item->iOffset;
			}

		while (item != iHead)
			{
			TMediaRequest* swap = item->iPrev;
			if (!deltaCalculated)
				{
				// recalculate the Prev item delta
				swap->iTriggerWallTime = 
					((swap->iMediaTime - aMtc.iMediaTimeBase) * aMtc.iInverseScaleQ16 >> 16) 
						+ aMtc.iWallTimeBase - swap->iOffset;
				}

			if (swap->iTriggerWallTime > item->iTriggerWallTime)
				{
				// switch (swap, item) for (item, swap)
				item->Deque();
				item->AddBefore(swap);
				if(swap == iHead)
					{
					iHead = item;
					}

				swapped = ETrue;
				}
			else
				{
				// move along list
				item = item->iPrev;
				}
			
			} // while

		// after the first pass of the queue, all deltas calculated
		deltaCalculated = ETrue;
		
		} while (swapped);
	
#ifdef _OMXIL_COMMON_DEBUG_TRACING_ON
	DbgPrint();
#endif

	CHECK_DEBUG();
	}
 
 
 TMediaRequest* TRequestDeltaQue::RemoveFirst()
	{
	CHECK_DEBUG();
	
	__ASSERT_DEBUG(((iHead == NULL && iCount == 0) || (iHead != NULL && iCount > 0)), 
						Panic(ERequestQueueCorrupt));
	
	TMediaRequest* item = NULL;
	
	// empty?
	if (!iHead)
		{
		return NULL;
		}
	else
	// only one element?
	if (1 == iCount)
		{
		item = iHead;
		iHead = NULL;
		}
	else
		{
		item = iHead;
		item->iPrev->iNext = item->iNext;
		item->iNext->iPrev = item->iPrev;
		
		// setup the new head
		iHead = item->iNext;
		}
	
	iCount--;
	
	CHECK_DEBUG();
	
	return item;
	}

 TBool TRequestDeltaQue::FirstDelta(TInt64& aDelta) const
	{
	CHECK_DEBUG();
	
	__ASSERT_DEBUG(((iHead == NULL && iCount == 0) || (iHead != NULL && iCount > 0)), 
						Panic(ERequestQueueCorrupt));
		
	if (!iHead)
		{
		return EFalse;
		}
	
	aDelta = iHead->iTriggerWallTime;
	return ETrue;
	}

 TBool TRequestDeltaQue::IsEmpty() const
	{
	__ASSERT_DEBUG(((iHead == NULL && iCount == 0) || (iHead != NULL && iCount > 0)), 
						Panic(ERequestQueueCorrupt));
		
	return (!iHead);
	}

 TUint TRequestDeltaQue::Count() const
	{
	return iCount;
	}

void TMediaRequest::Deque()
	{
	iPrev->iNext = iNext;
	iNext->iPrev = iPrev;
	}

void TMediaRequest::AddBefore(TMediaRequest* x)
	{
	x->iPrev->iNext = this;
	iPrev = x->iPrev;
	iNext = x;
	x->iPrev = this;
	}

#ifdef _DEBUG
 
 /**
  * Checks the linked list for consistency.
  */
 void TRequestDeltaQue::DbgCheck() const
	 {
	 if(iCount == 0)
		 {
		 __ASSERT_DEBUG(iHead == NULL, Panic(ERequestQueueCorrupt));
		 }
	 else
		 {
		 TMediaRequest* last = iHead;
		 TInt index = iCount - 1;
		 while(index-- > 0)
			 {
			 TMediaRequest* current = last->iNext;
			 __ASSERT_DEBUG(current->iPrev == last, Panic(ERequestQueueCorrupt));
			 __ASSERT_DEBUG(current->iTriggerWallTime >= last->iTriggerWallTime, Panic(ERequestQueueUnordered));
			 last = current;
			 }
		 __ASSERT_DEBUG(last->iNext == iHead, Panic(ERequestQueueCorrupt));
		 __ASSERT_DEBUG(iHead->iPrev == last, Panic(ERequestQueueCorrupt));
		 }
	 }
 
#endif
 
#ifdef _OMXIL_COMMON_DEBUG_TRACING_ON

void TRequestDeltaQue::DbgPrint() const
 	{
	TMediaRequest* x = iHead;
	TBuf8<256> msg;
 	msg.Append(_L8("pending times: "));
 	for(TInt index = 0; index < iCount; index++)
 		{
 		if(index > 0)
 			{
 			msg.Append(_L8(", "));
 			}
 		msg.AppendFormat(_L8("%ld"), x->iTriggerWallTime);
 		x = x->iNext;
 		}
 	DEBUG_PRINTF(msg);
 	}
 
#endif
