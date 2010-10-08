/*
* Copyright (c) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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
 * @file 
 * @internalTechnology
 */

#ifndef OMXILGRAPHICSINKTESTBASE_H
#define OMXILGRAPHICSINKTESTBASE_H

#include <test/testexecutestepbase.h>
#include <e32msgqueue.h>
#include <openmax/il/khronos/v1_x/OMX_Core.h>
#include <graphics/surfacemanager.h>
#include <graphics/surfaceupdateclient.h>
#include <w32std.h>

#include "omxilmmbuffer.h"

const TInt KTSU_OMX_GS_Interval = 0x0500000;
const TInt KTSU_OMX_GS_Pause_Interval = 0x0100000;
const TInt KTSU_OMX_GS_Pause_Wait = 2000000;
const TInt KTSU_OMX_GS_Pause_Video = 0x0050000;
const TInt KTSU_OMX_GS_State_Transition_Interval = 2000000; // 2 Seconds
const TInt KTSU_OMX_GS_CALLBACK = 1000000 ;// 2 second timer for test shutdown

const TInt KMaxLenStateTransitionName = 126;

const TInt KCameraVFPortIndex = 0;
const TInt KCameraVCPortIndex = 1;
const TInt KCameraICPortIndex = 2;
const TInt KCameraClockPortIndex = 3;

_LIT(KMMTestCase, "RTestCase");
_LIT(KMMTestStep, "RTestStep");

// Forward declarations
struct OMX_COMPONENTTYPE;
class COmxGsTestStateTransition;
class COmxGsTestBase;
class COmxGsTestShutdown;


/**
 * OpenMAX call back handler used in the test code.
 */
class CCallbackHandler : public CActive
	{
public:

	enum TMessageType
		{
		EFillBufferCallback,
		EEmptyBufferCallback,
		EEventCallback
		};

	class TEventParams
		{
	public:
		OMX_EVENTTYPE iEvent;
		TUint iData1;
		TUint iData2;
		TAny* iExtra;
		};

	class TOmxMessage
		{
	public:
		TMessageType iType;   
		OMX_HANDLETYPE iComponent;
		union
			{
			TAny* iBuffer;
			TEventParams iEventParams;
			};
		};


	static const TInt KMaxMsgQueueEntries = 10;

public:

	static CCallbackHandler* NewL(COmxGsTestBase& aCameraSourceTest);
	virtual ~CCallbackHandler();

	operator OMX_CALLBACKTYPE*();

	void RunL();
	void DoCancel();

	static OMX_ERRORTYPE FillBufferDone(OMX_HANDLETYPE aComponent,
										TAny* aAppData,
										OMX_BUFFERHEADERTYPE* aBuffer);

 	static OMX_ERRORTYPE EmptyBufferDone(OMX_HANDLETYPE aComponent,
										 TAny* aAppData,
										 OMX_BUFFERHEADERTYPE* aBuffer);

	static OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE aComponent,
									  TAny* aAppData,
									  OMX_EVENTTYPE aEvent,
									  TUint32 aData1,
									  TUint32 aData2,
									  TAny* aExtra);


private:

	OMX_ERRORTYPE DoEventHandler(OMX_HANDLETYPE aComponent,
								 TEventParams aParams);

	OMX_ERRORTYPE DoFillBufferDone(OMX_HANDLETYPE aComponent,
								  OMX_BUFFERHEADERTYPE* aBufferHeader);
								  
	OMX_ERRORTYPE DoEmptyBufferDone(OMX_HANDLETYPE aComponent,
								   OMX_BUFFERHEADERTYPE* aBufferHeader);								 


	void ConstructL();
	CCallbackHandler(COmxGsTestBase& aCameraSourceTest);

private:

	COmxGsTestBase& iCameraSourceTest;
	RMsgQueue<TOmxMessage> iMsgQueue;
	OMX_CALLBACKTYPE iHandle;

	};



class COmxGsTestBase : public CTestStep
	{
	friend class COmxGsTestShutdown;
	friend class COmxGsTestStateTransition;
public:

	COmxGsTestBase();

	~COmxGsTestBase();
	TVerdict doTestStepPreambleL(); 
	TVerdict doTestStepPostambleL();
	virtual TVerdict doTestStepL() = 0;
	virtual void CloseTestStep() = 0;
	void CloseTest();
	
	void InfoPrint1(const TDesC& aPrint);

	virtual void DoFillBufferDone(OMX_HANDLETYPE aComponent,
								  OMX_BUFFERHEADERTYPE* aBufferHeader) = 0;

	virtual void DoEmptyBufferDone(OMX_HANDLETYPE aComponent,
								   OMX_BUFFERHEADERTYPE* aBufferHeader) = 0;
   
	virtual void DoEventHandler(OMX_HANDLETYPE aComponent,
								OMX_EVENTTYPE aEvent,
								TUint aData1,	
								TUint aData2,
								TAny* aExtra) = 0;


	static OMX_ERRORTYPE ConvertSymbianErrorType(TInt aError);

protected:
	void PrintOmxState(OMX_STATETYPE aOmxState);
	TInt PrintOmxError(OMX_ERRORTYPE aOmxError);

	virtual void InitialiseOmxComponents();
	virtual void InitialiseTestSpecificOmxComponents();
	void CreateWindowL();
	void SendCommand(
					OMX_HANDLETYPE aComponent,
					OMX_COMMANDTYPE aCmd,
					OMX_U32 aParam1,
					OMX_PTR aCmdData,
					OMX_ERRORTYPE aExpError = OMX_ErrorNone);
					
	void GetState(OMX_HANDLETYPE aComponent,
				  OMX_STATETYPE* aState,
				  OMX_STATETYPE aExpectedState = OMX_StateMax);
	
	void SetParameter(
					OMX_HANDLETYPE aComponent, 
            		OMX_INDEXTYPE aIndex,
            		OMX_PTR aComponentParameterStructure,
            		OMX_ERRORTYPE aExpError = OMX_ErrorNone);

	void GetParameter(
					OMX_HANDLETYPE aComponent, 
            		OMX_INDEXTYPE aIndex,
            		OMX_PTR aComponentParameterStructure,
            		OMX_ERRORTYPE aExpError = OMX_ErrorNone);
            		
    void SetConfig(
					OMX_HANDLETYPE aComponent, 
            		OMX_INDEXTYPE aIndex,
            		OMX_PTR aComponentParameterStructure,
            		OMX_ERRORTYPE aExpError = OMX_ErrorNone);

	void GetConfig(
					OMX_HANDLETYPE aComponent, 
            		OMX_INDEXTYPE aIndex,
            		OMX_PTR aComponentParameterStructure,
            		OMX_ERRORTYPE aExpError = OMX_ErrorNone);        		
   
    void FreeBuffer(
    		OMX_HANDLETYPE aComponent,
            OMX_U32 aPortIndex,
            RPointerArray<OMX_BUFFERHEADERTYPE> aArrayBufferHeaderType,
            OMX_ERRORTYPE aExpError = OMX_ErrorNone);
	
    void AllocateBuffer(
    		OMX_HANDLETYPE aComponent,
    		OMX_BUFFERHEADERTYPE** aBufferHeaderType,
            OMX_U32 aPortIndex,
            OMX_PTR aAppPrivate,
            OMX_U32 aSizeBytes,
            RPointerArray<OMX_BUFFERHEADERTYPE>* aArrayBufferHeaderType,
            OMX_U32 aCount,
            OMX_ERRORTYPE aExpError = OMX_ErrorNone);
    
    void FreeHandles();
    virtual TInt PostKickOffTestL(TInt aTimerId) = 0;
	virtual TInt StateTransition(const TDesC& aStateTransitionName);
	TInt InitialiseSurfaceManager();
	TInt CreateAndMapSurface(const RSurfaceManager::TSurfaceCreationAttributesBuf& aReqs, TSurfaceId& aSurfaceId);
	void StartTimer();
	void WaitForCallbacks();
	void PauseTimer();
	
protected:

	CCallbackHandler* iCallbackHandler;
//	OMX_COMPONENTTYPE* iCameraSourceCompHandle;
//	OMX_COMPONENTTYPE* iCameraSourceCompHandlePort1;
	OMX_COMPONENTTYPE* iGraphicSinkCompHandle;
	OMX_COMPONENTTYPE* iGraphicSinkCompHandlePort1;
	OMX_COMPONENTTYPE* iBufferSupplierComponent;
	OMX_COMPONENTTYPE* iNonBufferSupplierComponent;
//	OMX_COMPONENTTYPE* iFileSinkCompHandle;
//	OMX_COMPONENTTYPE* iJpegEncoderCompHandle;
//	OMX_COMPONENTTYPE* iXvidEncoderCompHandle;
//	OMX_COMPONENTTYPE* i3gpMuxerCompHandle;
//	OMX_COMPONENTTYPE* iClockCompHandle;
//	OMX_COMPONENTTYPE* iImageWriterCompHandle;
	OMX_ERRORTYPE iError;
	OMX_STATETYPE iState;
	OMX_INDEXTYPE iSurfaceConfigExt;
	
	COmxGsTestShutdown* iTestShutdown;
	COmxGsTestShutdown* iTestPause;
	COmxGsTestStateTransition* iTestStateTransition;
	TTimeIntervalMicroSeconds32 iInterval;	
	
	volatile OMX_STATETYPE iCamPrevState;
	volatile OMX_STATETYPE iGphxPrevState;	
	volatile OMX_STATETYPE iBufferSupplierPrevState;
	volatile OMX_STATETYPE iNonBufferSupplierPrevState;	
	volatile OMX_STATETYPE iGphxPort1PrevState;
	volatile OMX_STATETYPE iFilePrevState;
	volatile OMX_STATETYPE iJpegEncoderPrevState;
	volatile OMX_STATETYPE iXvidEncoderPrevState;
	volatile OMX_STATETYPE i3gpMuxerPrevState;
	volatile OMX_STATETYPE iClockPrevState;
	volatile OMX_STATETYPE iImageWriterPrevState;
	
	RWsSession              iWsSession;
	RWindowGroup            iWindowGroup;  // Window group of the AO windows. 
	TInt                    iWindowHandle; // Window handle(s) for the AO windows. This handle
	    	                                      //  is incremental and reused by various WServ artifacts. 
	CWsScreenDevice         *iWsSd;        // Screen Device for this WServ session. 
	CWindowGc               *iGc;     // Graphics Context associated with the window. 
	RWindow					*iWindow;
	CWindowGc               *iGc2;
	RWindow					*iWindow2;
	
    RSurfaceManager         iSurfaceManager;
    RSurfaceUpdateSession   iSurfaceUpdateSession;
	RChunk iTestChunk;
	TSurfaceId iSurfaceId;
	CActiveScheduler* iScheduler;
	};


/**
 * Shutdown timer for tests.
 */
class COmxGsTestShutdown : public CTimer
	{
public:
	
	static COmxGsTestShutdown* NewL(COmxGsTestBase* aOmxGsTest,TInt aTimerId = 1);
	
	COmxGsTestShutdown(COmxGsTestBase* aOmxGsTest,TInt aTimerId = 1);
	~COmxGsTestShutdown();
	void ConstructL();
	void Start(TTimeIntervalMicroSeconds32 aInterval, TInt aReason, TVerdict aResult);

private:

	void RunL();
	
private:

	COmxGsTestBase* iOmxGsTest;
	TTimeIntervalMicroSeconds32 iInterval;
	TInt iReason;
	TVerdict iResult;
	TInt iTimerId;
	
	};


class COmxGsTestStateTransition : public CTimer
	{
	
public:
	
	static COmxGsTestStateTransition* NewL(COmxGsTestBase* aOmxGsTest,TInt aPriority);
	
	COmxGsTestStateTransition(COmxGsTestBase* aOmxGsTest,TInt aPriority);
	~COmxGsTestStateTransition();
	void ConstructL();
	void Start(TTimeIntervalMicroSeconds32 aInterval,const TDesC& aStateTransitionName);

protected:	
	TBuf<KMaxLenStateTransitionName> iStateTransitionName;

private:

	void RunL();
	
private:

	COmxGsTestBase* iOmxGsTest;
	TTimeIntervalMicroSeconds32 iInterval;
	};



#endif // OMXILGRAPHICSINKTESTBASE_H
