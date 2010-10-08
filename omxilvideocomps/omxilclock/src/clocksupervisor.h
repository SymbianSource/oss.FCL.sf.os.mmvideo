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


#ifndef CLOCKSUPERVISOR_H_
#define CLOCKSUPERVISOR_H_

#include <e32base.h>
#include <openmax/il/khronos/v1_x/OMX_Other.h>

/**
 @file
 @internalComponent
 */

// forward declaration as supervisor calls back to processing function
class COmxILClockProcessingFunction;

///////////////////////////////////////////////////////////////////////////////
// List of structures declared in this file:
// 
// TMediaRequest
// TEntryPoint
// TIndexToFunctionMapping
// CClockSupervisor
// 
///////////////////////////////////////////////////////////////////////////////
// Storage of Clock requests.
//
// The Media Update storage holds a number of request updates to send to 
// clients.
// The updates are entered in a queue based on the delta of the 
// 'expected MediaTime minus Offset' giving an absolute time (the delta). 
// This difference is then calculated by the queue implementation on insertion
// of the element.
//
// When the clock changes state it notifies the clients by sending them a state 
// change notification broadcast.
// The important thing to note here is that there is no seperate queue for 
// outgoing notifications.
// As soon as a request timer expires that request is forwarded. 
// Broadcast requests are raised and placed first on the queue so are sent 
// as and when they occur.
//
// When the scale is a change of direction (i.e. playing in the backwards 
// direction), the queue is cleared and the delta's absolute value is taken.
// 
// The delta is a TInt which matches the Ticks datatype returned by the system.
// 
// We do not expect the phone to be in operation beyond the maximum duration 
// held by the 64-bit Wall clock. Behaviour beyond this is undefined.
//
// The reason for choosing a list over an array is mainly access efficiency.
//
// 1) The insertions must be in order so as to be pulled off quickly.
//    Updates are pulled off from the front of the queue, so you would have 
//    to adjust the whole array each time.
// 2) Updates arrive at different times and can expect to be fulfilled prior 
//    to earlier updates, again the list would need to be adjusted after the 
//    point of insertion.
// 
// Clients are attached via ports and the notifications are sent to them via 
// these ports
//
///////////////////////////////////////////////////////////////////////////////
// Clock functionality is invoked both externally & internally.
//
// Based on the OMX IL 1.1.1 standard, we route these incoming external 
// requests to their relevant behaviour.
//
// This Clock component is intended to be created per use case.
// That is for every use case that requires synchronisation.
// This could result in several clocks being created, each with an audio and/or
// video as an input, and each with a high-priority clock thread.
// At most it is considered that there should only be a few of these components
// in existance at any one time, as mobile devices should not really require it.
// However, saying this, it should be factored into the design. 
// At this stage it is envisioned that any clock component should reduce its 
// timer events to once at just after the 32-bit wraparound when its 
// application is not in focus. 
// This is due to the time being reported to us in a 32-bit value, whilst our 
// Wall clock is 64-bits. 
// To avoid any wraparound and losing time by not regulary updating the upper 
// word of our wall clock, we need to somehow check the system clock at this 
// inteval. This is not a problem during normal operation but can be when the 
// application is Paused or goes out of focus.
//
// NOTE: Assumes OMX_SKIP64BIT is not set when building OMX CORE
///////////////////////////////////////////////////////////////////////////////


/**
 * 
 *
 */
class TMediaRequest
	{
public:
	TInt64 iMediaTime;
	TInt64 iOffset;
	TInt64 iTriggerWallTime;		// == iWallTimeAtMediaTime - iOffset
	TInt iPortIndex;
	TAny* iClientPrivate;
	
	inline void Deque();
	inline void AddBefore(TMediaRequest*);
	
public:
	TMediaRequest *iPrev;
	TMediaRequest *iNext;
	};

/**
 * 
 *
 */
class TMediaTimeContext
	{
public:
	TMediaTimeContext(): iScaleQ16 (1 << 16), iInverseScaleQ16 (1 << 16)
		{/*do nothing*/};

	void SetScaleQ16(TInt32 aScaleQ16, TInt64 aWallTimeNow);
public:
	TInt64 iWallTimeBase;
	TInt64 iMediaTimeBase;
	
	/**
	 * Scale ranges map to modes of playback.
	 * A Q16 value relative to a 1X forward advancement of the media clock.
	 */
	TInt32 iScaleQ16;
	
	/**
	 * The reciprocal of iScaleQ16 (i.e. 1 / iScaleQ16), but adjusted for the
	 * Q16 format.
	 * 
	 * It therefore has the value 2^32 / iScaleQ16.
	 * 
	 * If iScaleQ16 == 0, this field takes the value KMaxTInt (2^31 - 1)
	 * 
	 * If magnitude of iInverseScaleQ16 would be too large for a signed 32-bit
	 * value, the value is clipped to KMaxTInt or KMinTInt (-2^31 or
	 * (2^31 - 1)). This can only happen if iScaleQ16 == +/- 1.
	 */
	TInt32 iInverseScaleQ16;
	};

/**
 * 
 *
 */
class TRequestDeltaQue
	{
public:
	TRequestDeltaQue();
	
	void Add(TMediaRequest* aElement, TInt64 aDelta);
	TMediaRequest* RemoveFirst();
	TBool FirstDelta(TInt64& aDelta) const;
	void RecalculateAndReorder(TMediaTimeContext& aMTC);
	TBool IsEmpty() const;
	TUint Count() const;
	
private:
	TBool InsertBeforeFoundPosition(TInt64 aDelta, TMediaRequest*& aItem) const;

#ifdef _DEBUG
	void DbgCheck() const;
#endif
#ifdef _OMXIL_COMMON_DEBUG_TRACING_ON
	void DbgPrint() const;
#endif

private:
	TMediaRequest* iHead;
	TInt iCount;
	};


/**
 * 
 *
 */
class CClockSupervisor : public CBase
	{
public:
#ifdef STABILITY_TEST_WRAPPER
	friend class CStabilityTestWrapper;
	friend class CStabilityTestNegativeWrapper;
#endif
friend class CClockThreadNotifier;	
public:
	static CClockSupervisor* NewL(COmxILClockProcessingFunction& aCallbacks);
	~CClockSupervisor();

	/**
	 * Defines how a request arrived at the clock component
	 */
	enum TEntryPoint
		{
		EGetConfig,
		ESetConfig
		};
	
	// Producer function
	// Invoked via clients of this component, i.e. GetConfig() and SetConfig()
	OMX_ERRORTYPE ProduceRequest(OMX_INDEXTYPE aIndex, TEntryPoint aEntryPoint,
								 TAny* aPassedStructPtr);
								 
	const OMX_TIME_CONFIG_CLOCKSTATETYPE GetMediaClockState() const
		{
		return iMediaClockState;
		}

private:
	CClockSupervisor(COmxILClockProcessingFunction& aProcessingFunction);
	void ConstructL();

private:
	//////////////
	// This section describes the Clock thread related functions
	//////////////
	
	// The timing thread's entry point
	static TInt ThreadEntryPoint(TAny* aPtr);
	void RunTimingThreadL();
	
private:
	//////////////
	// This section describes the request handler functions
	//////////////

	// From the Audio/Video feeds
	OMX_ERRORTYPE HandleUpdateAudioReference(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);
	OMX_ERRORTYPE HandleUpdateVideoReference(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);
	
	// From the OMX component
	OMX_ERRORTYPE HandleSetPortClientStartTime(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);
	OMX_ERRORTYPE HandleSubmitMediaTimeRequest(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);

	// From the IL client
	OMX_ERRORTYPE HandleGetSetTimeScale(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);
	OMX_ERRORTYPE HandleGetSetSeekMode(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);
	OMX_ERRORTYPE HandleGetSetClockState(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);
	OMX_ERRORTYPE HandleGetSetActiveRefClock(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);

	// Called from either IL client or IL components
	OMX_ERRORTYPE HandleQueryCurrentWallTime(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);
	OMX_ERRORTYPE HandleQueryCurrentMediaTime(TEntryPoint aEntry, OMX_PTR aPassedStructPtr);
	
private:
	//////////////
	// This section describes the Clock components internal house-keeping 
	// routines
	//////////////

	// Perform actions related to placing the clock into the Stopped state
	void DoTransitionToStoppedState();
	
	// Perform actions related to placing the clock into the Running state
	void DoTransitionToRunningState();
	
	// Perform actions related to placing the clock into the Waiting state
	void DoTransitionToWaitingState(OMX_U32 nWaitMask);
	
	// Update the tick counter, generating higher 32 bits
	void UpdateWallTicks();
	
	// Consumer function
	// Invoked at initialisation and runs in event loop.
	TInt ConsumeRequests();
	
	// The timer loop to enable us to wake up on heart beat or requests
	void TimerLoop();

	// obtains the most appropriate start time based on scale (direction)
	void CalculateStartTime();
	
	// reports whether all clients have reported their start times
	TBool AllStartTimesReported();
	
	TInt64 WallTime();
	
	void UpdateMediaTime(TInt64 aMediaTime);
	void BroadcastUpdate(const OMX_TIME_MEDIATIMETYPE& aUpdate);
	
	// reports error when clock thread panics
	
	void ReportClockThreadPanic();
	
private:
	//////////////
	// This section describes the clock's miscellaneous structures
	//////////////

	/**
	 * State of the clock component's media clock:
	 */
	OMX_TIME_CONFIG_CLOCKSTATETYPE iMediaClockState;

	/**
	 * Choice of the clock component's reference clock:
	 */
	OMX_TIME_REFCLOCKTYPE iActiveRefClock;
	
	/**
	 * Array of clients' start times:
	 */
	RArray<TInt64> iStartTimes;
	TUint iStartTimesSet;			/* bit mask representing which elements of
	                                   the start time array are valid */

private:
	//////////////
	// This section describes the Clock component's routing structures
	//////////////
	
	/*
	 * typedef for function to handle incoming requests
	 */
	typedef OMX_ERRORTYPE (CClockSupervisor::*FunctionPtr)(TEntryPoint, OMX_PTR);

	/*
	 * Function jump table. Note that this is declared in class scope so that the table
	 * definition has access to private methods.
	 */
	static const FunctionPtr iJumpTable[];
	
private:
	//////////////
	// This section describes the Clock component's Timer Control
	//////////////

	/**
	 * Holds the current Wall Clock. The duration of a 'tick' is platform specific.
	 * This needs to be 4-byte aligned to allow for atomic access.
	 */
	OMX_TICKS iWallTicks;
	
	/**
	 * Encapsulates the relationship between wall time and media time.
	 */
	TMediaTimeContext iMtc;
	
	// conversion factors from wall 'ticks' to microseconds
	TInt iMicroConvNum;
	TInt iMicroConvDen;
	TInt iMicroConvShift;
	
	// maximum time between calls to WallTime() - avoids 32-bit counter overflow
	TInt iHeartbeatTimerInterval;
	
	// on some platforms User::FastCounter() counts backwards
	TBool iSystemClockReversed;
	
private:
	//////////////
	// This section describes the Clock component's thread and other resources
	//////////////

	/**
	 * This Clock component object needs to run in a high priority thread as it 
	 * represents the OMX component timings
	 */
	RThread iThread;
	
	/**
	 * Thread access control Mutex, 
	 * used to control updating of the components requests by mutiple threads
	 */
	RMutex iQueMutex;

	/**
	 * Event timer to wake up this object to complete fulfillment of a request
	 * This handle belongs to the High-Pri thread
	 */
	RTimer iTimer;
	
	/**
	 * Flags to control timing thread exit/destroy.
	 */
	TBool iThreadStarted;
	TBool iThreadRunning;
	
	/**
	 * Complete this status on the timing thread to interrupt the timer sleep.
	 * Timer itself can't be cancelled since timer handle is local to timing
	 * thread.
	 */
	TRequestStatus iCancelStatus;

private:
	//////////////
	// This section describes the Clock component's request management 
	// infrastructure
	//////////////

	/**
	 * Pending Media Time Requests
	 */
	TRequestDeltaQue iPendingRequestQue;

	/**
	 * The free queue header.
	 * Items ared remove from here and placed on the pending queue.
	 */
	TRequestDeltaQue iFreeRequestQue;

	/**
	 * memory block where request list nodes are allocated
	 */
	TMediaRequest* iRequestBlock;

	/**
	 * Max number of request list nodes allocated
	 */
	const TUint iMaxRequests;
	
	COmxILClockProcessingFunction& iProcessingFunction;
	}; // class CClockSupervisor


#endif // CLOCKSUPERVISOR_H_
