/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "omxilgraphicsinktestbase.h"
#include <mmf/server/mmfbuffer.h>
#include <mmf/server/mmfdatabuffer.h>

CCallbackHandler*
CCallbackHandler::NewL(COmxGsTestBase& aCameraSourceTest)
	{
	CCallbackHandler* self = new (ELeave) CCallbackHandler(aCameraSourceTest);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}


void
CCallbackHandler::ConstructL()
	{
	OMX_CALLBACKTYPE h =
			{
			CCallbackHandler::EventHandler,
			CCallbackHandler::EmptyBufferDone,
			CCallbackHandler::FillBufferDone
			};

	iHandle = h;
	CActiveScheduler::Add(this);

	User::LeaveIfError(iMsgQueue.CreateLocal(KMaxMsgQueueEntries));
	iMsgQueue.NotifyDataAvailable(iStatus);
	SetActive();
	}

CCallbackHandler::CCallbackHandler(COmxGsTestBase& aCameraSourceTest)
	: CActive(EPriorityStandard),
	  iCameraSourceTest(aCameraSourceTest)
	{
	}


CCallbackHandler::operator OMX_CALLBACKTYPE*()
	{
	return &iHandle;
	}


void
CCallbackHandler::RunL()
	{
	TOmxMessage msg;
	while (iMsgQueue.Receive(msg)==KErrNone)
		{
		switch (msg.iType)
			{
			case EEmptyBufferCallback:
				{
//				MOmxInputPortCallbacks* callback = msg.iBuffer->InputPortCallbacks();
//				const CMMFBuffer* buffer = msg.iBuffer->MmfBuffer();
//				callback->EmptyBufferDone(msg.iComponent, buffer);
				iCameraSourceTest.DoEmptyBufferDone(msg.iComponent, static_cast<OMX_BUFFERHEADERTYPE*>(msg.iBuffer));
				break;
				}

			case EFillBufferCallback:
				{
				//iCameraSourceTest.DoFillBufferDone(msg.iComponent,static_cast<CCameraBuffer*>(msg.iBuffer));
				iCameraSourceTest.DoFillBufferDone(msg.iComponent, static_cast<OMX_BUFFERHEADERTYPE*>(msg.iBuffer));
				break;
				}
			case EEventCallback:
				{
				iCameraSourceTest.DoEventHandler(msg.iComponent,
											msg.iEventParams.iEvent,
											msg.iEventParams.iData1,
											msg.iEventParams.iData2,
											msg.iEventParams.iExtra);
				break;
				}
			default:
				{
				// This is an invalid state
				ASSERT(EFalse);
				}
			}
		}

	// setup for next callbacks
	iStatus = KRequestPending;
	iMsgQueue.NotifyDataAvailable(iStatus);
	SetActive();

	}

CCallbackHandler::~CCallbackHandler()
	{
	Cancel();
	iMsgQueue.Close();
	}


void
CCallbackHandler::DoCancel()
	{
	if (iMsgQueue.Handle())
		{
		iMsgQueue.CancelDataAvailable();
		}
	}

OMX_ERRORTYPE
CCallbackHandler::FillBufferDone(OMX_HANDLETYPE aComponent,
									TAny* aAppData,
									OMX_BUFFERHEADERTYPE* aBuffer)
	{
	ASSERT(aAppData);
	//CCameraBuffer* pBuffer = static_cast<CCameraBuffer*>(aBuffer->pAppPrivate);
	return static_cast<CCallbackHandler*>(aAppData)->DoFillBufferDone(aComponent, aBuffer);
	}

OMX_ERRORTYPE
CCallbackHandler::EmptyBufferDone(OMX_HANDLETYPE aComponent,
									 TAny* aAppData,
									 OMX_BUFFERHEADERTYPE* aBuffer)
	{
	ASSERT(aAppData);
	//CCameraBuffer* pBuffer = static_cast<CCameraBuffer*>(aBuffer->pAppPrivate);
	return static_cast<CCallbackHandler*>(aAppData)->DoEmptyBufferDone(aComponent, aBuffer);

	}

OMX_ERRORTYPE
CCallbackHandler::EventHandler(OMX_HANDLETYPE aComponent,
								  TAny* aAppData,
								  OMX_EVENTTYPE aEvent,
								  TUint32 aData1,
								  TUint32 aData2,
								  TAny* aExtra)
	{
	ASSERT(aAppData);
	CCallbackHandler::TEventParams eventParams;
	eventParams.iEvent = aEvent;
	eventParams.iData1 = aData1;
	eventParams.iData2 = aData2;
	eventParams.iExtra = aExtra;	
	return static_cast<CCallbackHandler*>(aAppData)->DoEventHandler(aComponent, eventParams);
	}

OMX_ERRORTYPE CCallbackHandler::DoFillBufferDone(OMX_HANDLETYPE aComponent,
								  OMX_BUFFERHEADERTYPE* aBufferHeader)
	{
	TOmxMessage message;
	message.iType = EFillBufferCallback;
	message.iComponent = aComponent;
	message.iBuffer = static_cast <OMX_BUFFERHEADERTYPE*>(aBufferHeader);
	TInt error = iMsgQueue.Send(message);
	//RDebug::Printf("CCallbackHandler:: Error: %d",error);

	return (error == KErrNone ? OMX_ErrorNone : OMX_ErrorUndefined);
	}

OMX_ERRORTYPE CCallbackHandler::DoEmptyBufferDone(OMX_HANDLETYPE aComponent,
								   OMX_BUFFERHEADERTYPE* aBufferHeader)
	{
	TOmxMessage message;
	message.iType = EEmptyBufferCallback;
	message.iComponent = aComponent;
	message.iBuffer = static_cast <OMX_BUFFERHEADERTYPE*> (aBufferHeader);
	TInt error = iMsgQueue.Send(message);
	//RDebug::Printf("CCallbackHandler:: Error: %d",error);

	return (error == KErrNone ? OMX_ErrorNone : OMX_ErrorUndefined);
	}


OMX_ERRORTYPE CCallbackHandler::DoEventHandler(OMX_HANDLETYPE aComponent, TEventParams aEventParams)
	{
	TOmxMessage message;
	message.iType = EEventCallback;
	message.iComponent = aComponent;
	message.iEventParams = aEventParams;

	/*
	OMX_EventCmdComplete,
    OMX_EventError,
    OMX_EventMark,
    OMX_EventPortSettingsChanged,
    OMX_EventBufferFlag,
    OMX_EventResourcesAcquired,
    OMX_EventComponentResumed,
    OMX_EventDynamicResourcesAvailable,
    OMX_EventPortFormatDetected,
    OMX_EventMax = 0x7FFFFFFF
	*/

	// TO DO - Put in switch on OMX_EVENTTYPE iEvent and print out the event type and relevant error code in test logs
	TInt error = iMsgQueue.Send(message);
	//RDebug::Printf("CCallbackHandler:: Error: %d",error);

	return (error == KErrNone ? OMX_ErrorNone : OMX_ErrorUndefined);
	}

//
// COmxGsTestBase
//

// Device driver constants

TVerdict COmxGsTestBase::doTestStepPreambleL()
/**
 * @return - TVerdict
 * Implementation of CTestStep base class virtual
 * It is used for doing all initialisation common to derived classes in here.
 * Make it being able to leave if there are any errors here as there's no point in
 * trying to run a test step if anything fails.
 * The leave will be picked up by the framework.
 */
	{
	// Here we check to see if there is already an active scheduler
	// installed, as this function will receive multiple calls during
	// Alloc testing of CS and GS components
	__UHEAP_MARK;
	if (!CActiveScheduler::Current())
		{
		iScheduler = new (ELeave) CActiveScheduler;
		CActiveScheduler::Install(iScheduler);
		}

	// Make sure we are not running the Alloc tests, if we are
	// then we will create these elsewhere.
	if (TestStepName() != (_L("MMVIDEO-OMX-CS-004-HP")) &&
		TestStepName() != (_L("MMVIDEO-OMX-GS-001-00-HP")) &&
		TestStepName() != (_L("MMVIDEO-OMX-JP-001-00-HP")) &&
		TestStepName() != (_L("MMVIDEO-OMX-FS-001-00-HP")) &&
		TestStepName() != (_L("MMVIDEO-OMX-IW-001-00-HP")))
		{
		InitialiseOmxComponents();
		if (TestStepResult() != EPass)
			{
			INFO_PRINTF1(_L("*** InitialiseOmxComponents() failed. No point running test case. ***"));

			// If the preamble leaves then the postamble isn't called by TEF.
			// We have to do it ourselves to ensure stuff is cleaned up.
			TRAP_IGNORE(doTestStepPostambleL());
			User::Leave(KErrGeneral);
			}
		}

	return TestStepResult();
	}

TVerdict COmxGsTestBase::doTestStepPostambleL()
/**
 * @return - TVerdict
 * Implementation of CTestStep base class virtual
 * It is used for doing all after test treatment common to derived classes in here.
 * Make it being able to leave
 * The leave will be picked up by the framework.
 */
	{
	CloseTestStep();

	delete iScheduler;
	__UHEAP_MARKEND;
	return TestStepResult();
	}

COmxGsTestBase::~COmxGsTestBase()
	{
	}




COmxGsTestBase::COmxGsTestBase() : iCamPrevState(OMX_StateInvalid),
										iGphxPrevState(OMX_StateInvalid)
	{
	}


void
COmxGsTestBase::PrintOmxState(OMX_STATETYPE aOmxState)
	{
	switch(aOmxState)
		{
	    case OMX_StateInvalid:
			{
			INFO_PRINTF1(_L("OMX STATE : OMX_StateInvalid"));
			}
			break;
	    case OMX_StateLoaded:
			{
			INFO_PRINTF1(_L("OMX STATE : OMX_StateLoaded"));
			}
			break;
	    case OMX_StateIdle:
			{
			INFO_PRINTF1(_L("OMX STATE : OMX_StateIdle"));
			}
			break;
	    case OMX_StateExecuting:
			{
			INFO_PRINTF1(_L("OMX STATE : OMX_StateExecuting"));
			}
			break;
	    case OMX_StatePause:
			{
			INFO_PRINTF1(_L("OMX STATE : OMX_StatePause"));
			}
			break;
	    case OMX_StateWaitForResources:
			{
			//INFO_PRINTF1(_L("OMX STATE : OMX_StateWaitForResources"));
			}
			break;
		default:
			{
			//INFO_PRINTF1(_L("OMX STATE : Wrong state found"));
			}
		}
	}

TInt COmxGsTestBase::PrintOmxError(OMX_ERRORTYPE aOmxError)
	{
    switch(aOmxError)
		{
		case OMX_ErrorNone:
			{
			INFO_PRINTF1(_L("OMX ERROR : OMX_ErrorNone"));
			return KErrNone;
			}

		case OMX_ErrorInsufficientResources:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorInsufficientResources"));
			return KErrNoMemory; //KErrNotReady;
			}

		case OMX_ErrorUndefined:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorUndefined"));
			return KErrGeneral;
			}

		case OMX_ErrorInvalidComponentName:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorInvalidComponentName"));
			return KErrBadName;
			}

		case OMX_ErrorComponentNotFound:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorComponentNotFound"));
			return KErrNotFound;
			}

		case OMX_ErrorInvalidComponent:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorInvalidComponent"));
			return KErrBadHandle;
			}

		case OMX_ErrorBadParameter:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorBadParameter"));
			return KErrArgument;
			}

		case OMX_ErrorNotImplemented:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorNotImplemented"));
			return KErrNotSupported;
			}

		case OMX_ErrorUnderflow:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorUnderflow"));
			return KErrUnderflow;
			}

		case OMX_ErrorOverflow:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorOverflow"));
			return KErrOverflow;
			}

		case OMX_ErrorHardware:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorHardware"));
			return KErrUnknown;
			}

		case OMX_ErrorInvalidState:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorInvalidState"));
			return KErrGeneral;
			}

		case OMX_ErrorStreamCorrupt:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorStreamCorrupt"));
			return KErrCorrupt;
			}

		case OMX_ErrorPortsNotCompatible:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorPortsNotCompatible"));
			//ERR_PRINTF1(_L("Ports being connected are not compatible"));
			return KErrUnknown;
			}

		case OMX_ErrorResourcesLost:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorResourcesLost"));
			//ERR_PRINTF1(_L("Resources allocated to an idle component have been lost resulting in the component returning to the loaded state"));
			return KErrNotReady;
			}

		case OMX_ErrorNoMore:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorNoMore"));
			//ERR_PRINTF1(_L("No more indicies can be enumerated"));
			return KErrGeneral;
			}

		case OMX_ErrorVersionMismatch:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorVersionMismatch"));
			//ERR_PRINTF1(_L("The component detected a version mismatch"));
			return KErrArgument;
			}

		case OMX_ErrorNotReady:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorNotReady"));
			//ERR_PRINTF1(_L("The component is not ready to return data at this time"));
			return KErrNotReady;
			}

		case OMX_ErrorTimeout:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorTimeout"));
			//ERR_PRINTF1(_L("There was a timeout that occurred"));
			return KErrTimedOut;
			}

		case OMX_ErrorSameState:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorSameState"));
			//ERR_PRINTF1(_L("This error occurs when trying to transition into the state you are already in"));
			return KErrGeneral;
			}

		case OMX_ErrorResourcesPreempted:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorResourcesPreempted"));
			//ERR_PRINTF1(_L("Resources allocated to an executing or paused component have been preempted, causing the component to return to the idle state"));
			return KErrGeneral;
			}

		case OMX_ErrorPortUnresponsiveDuringAllocation:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorPortUnresponsiveDuringAllocation"));
			//ERR_PRINTF1(_L("A non-supplier port sends this error to the IL client (via the EventHandler callback) during the allocation of buffers (on a transition from the LOADED to the IDLE state or on a port restart) when it deems that it has waited an unusually long time for the supplier to send it an allocated buffer via a UseBuffer call."));
			return KErrGeneral; //KErrTimedOut
			}

		case OMX_ErrorPortUnresponsiveDuringDeallocation:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorPortUnresponsiveDuringDeallocation"));
			//ERR_PRINTF1(_L("A non-supplier port sends this error to the IL client (via the EventHandler callback) during the deallocation of buffers (on a transition from the IDLE to LOADED state or on a port stop) when it deems that it has waited an unsually long time for the supplier to request the deallocation of a buffer header via a FreeBuffer call."));
			return KErrGeneral; //KErrTimedOut
			}

		case OMX_ErrorPortUnresponsiveDuringStop:
			{
		ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorPortUnresponsiveDuringStop"));
			//ERR_PRINTF1(_L("A supplier port sends this error to the IL client (via the EventHandler callback) during the stopping of a port (either on a transition from the IDLE to LOADED state or a port stop) when it deems that it has waited an unusually long time for the non-supplier to return a buffer via an EmptyThisBuffer or FillThisBuffer call."));
			return KErrGeneral; //KErrTimedOut
			}

		case OMX_ErrorIncorrectStateTransition:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorIncorrectStateTransition"));
			//ERR_PRINTF1(_L("Attempting a state transtion that is not allowed"));
			return KErrGeneral;
			}

		case OMX_ErrorIncorrectStateOperation:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorIncorrectStateOperation"));
			//ERR_PRINTF1(_L("Attempting a command that is not allowed during the present state."));
			return KErrGeneral; //KErrNotSupported
			}

		case OMX_ErrorUnsupportedSetting:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorUnsupportedSetting"));
			//ERR_PRINTF1(_L("The values encapsulated in the parameter or config structure are not supported."));
			return KErrGeneral; //KErrNotSupported
			}

		case OMX_ErrorUnsupportedIndex:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorUnsupportedIndex"));
			//ERR_PRINTF1(_L("The parameter or config indicated by the given index is not supported."));
			return KErrNotSupported; //KErrTooBig or KErrGeneral
			}

		case OMX_ErrorBadPortIndex:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorBadPortIndex"));
			//ERR_PRINTF1(_L("The port index supplied is incorrect."));
			return KErrArgument; //KErrTooBig
			}

		case OMX_ErrorPortUnpopulated:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorPortUnpopulated"));
			//ERR_PRINTF1(_L("The port has lost one or more of its buffers and it thus unpopulated."));
			return KErrGeneral;
			}

		case OMX_ErrorComponentSuspended:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorComponentSuspended"));
			//ERR_PRINTF1(_L("Component suspended due to temporary loss of resources"));
			return KErrAbort;
			}

		case OMX_ErrorDynamicResourcesUnavailable:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorDynamicResourcesUnavailable"));
			//ERR_PRINTF1(_L("Component suspended due to an inability to acquire dynamic resources"));
			return KErrAbort; //KErrGeneral
			}

		case OMX_ErrorMbErrorsInFrame:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorMbErrorsInFrame"));
			//ERR_PRINTF1(_L("When the macroblock error reporting is enabled the component returns new error for every frame that has errors"));
			return KErrGeneral;
			}

		case OMX_ErrorFormatNotDetected:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorFormatNotDetected"));
			//ERR_PRINTF1(_L("A component reports this error when it cannot parse or determine the format of an input stream."));
			return KErrCorrupt; //KErrGeneral
			}

		case OMX_ErrorContentPipeOpenFailed:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorContentPipeOpenFailed"));
			//R//ERR_PRINTF1(_L("The content open operation failed."));
			return KErrGeneral;
			//break;
			}

		case OMX_ErrorContentPipeCreationFailed:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorContentPipeCreationFailed"));
			//ERR_PRINTF1(_L("The content creation operation failed."));
			return KErrGeneral;
			}

		case OMX_ErrorSeperateTablesUsed:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorSeperateTablesUsed"));
			//ERR_PRINTF1(_L("Separate table information is being used"));
			return KErrGeneral;
			}

		case OMX_ErrorTunnelingUnsupported:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorTunnelingUnsupported"));
			//ERR_PRINTF1(_L("Tunneling is unsupported by the component"));
			return KErrNotSupported;
			}

		case OMX_ErrorMax:
			{
			ERR_PRINTF1(_L("OMX ERROR : OMX_ErrorMax"));
			//ERR_PRINTF1(_L("OMX_ErrorMax"));
			return KErrTooBig; // KErrGeneral
			}

		default:
			{
			ERR_PRINTF1(_L("OMX ERROR : Wrong error code found"));
			return KErrGeneral;
			}
		}
	}

void COmxGsTestBase::InfoPrint1(const TDesC& aPrint)
	{
	INFO_PRINTF1(aPrint);
	}

OMX_ERRORTYPE COmxGsTestBase::ConvertSymbianErrorType(TInt aError)
	{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	switch (aError)
		{
	case KErrNone:
		err = OMX_ErrorNone;
		break;
	case KErrNoMemory:
		err = OMX_ErrorInsufficientResources;
		break;
	case KErrGeneral:
		break;
	default:
		err = OMX_ErrorUndefined;
		}
	return err;
	}


void COmxGsTestBase::InitialiseOmxComponents()
	{
	iError = OMX_Init();

	if (OMX_ErrorNone != iError)
		{
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}

	ASSERT(!iCallbackHandler);
	TRAPD(error, iCallbackHandler = CCallbackHandler::NewL(*this));
	OMX_CALLBACKTYPE* omxCallbacks = *iCallbackHandler;

	if (error)
		{
		SetTestStepError(error);
		return SetTestStepResult(EFail);
		}

	OMX_PTR appData = iCallbackHandler;

	OMX_HANDLETYPE graphicsSinkHandle = NULL;
	OMX_STRING graphicsSinkComponentName = "OMX.SYMBIAN.VIDEO.GRAPHICSINK";

	iError = OMX_GetHandle(
	    &graphicsSinkHandle,
	    graphicsSinkComponentName,
	    appData,
	    omxCallbacks);

	if (OMX_ErrorNone != iError)
		{
		INFO_PRINTF2(_L("Error %08X loading graphics sink"), iError);
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}

	iGraphicSinkCompHandle = (OMX_COMPONENTTYPE*) graphicsSinkHandle;
	INFO_PRINTF2(_L("OMX.SYMBIAN.VIDEO.GRAPHICSINK: Handle: %X"), iGraphicSinkCompHandle);
/*
	OMX_HANDLETYPE cameraSourceHandle = NULL;
	OMX_STRING cameraSourceComponentName = "OMX.SYMBIAN.VIDEO.CAMERASOURCE";

	iError = OMX_GetHandle(
	    &cameraSourceHandle,
	    cameraSourceComponentName,
	    appData,
	    omxCallbacks);

	if (OMX_ErrorNone != iError)
		{
		INFO_PRINTF2(_L("Error %08X loading camera source"), iError);
		OMX_FreeHandle(iGraphicSinkCompHandle);
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}
	iCameraSourceCompHandle = (OMX_COMPONENTTYPE*)cameraSourceHandle;
	INFO_PRINTF2(_L("OMX.SYMBIAN.VIDEO.CAMERASOURCE: Handle: %X"), iCameraSourceCompHandle);
	
 	OMX_HANDLETYPE fileSinkHandle = NULL;
	OMX_STRING fileSinkComponentName = "OMX.SYMBIAN.OTHER.FILESINK";

	iError = OMX_GetHandle(
	    &fileSinkHandle,
	    fileSinkComponentName,
	    appData,
	    omxCallbacks);

	if (OMX_ErrorNone != iError)
		{
		INFO_PRINTF2(_L("Error %08X loading file sink"), iError);
		OMX_FreeHandle(iGraphicSinkCompHandle);
		OMX_FreeHandle(iCameraSourceCompHandle);
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}

	iFileSinkCompHandle = (OMX_COMPONENTTYPE*)fileSinkHandle;
	INFO_PRINTF2(_L("OMX.SYMBIAN.OTHER.FILESINK: Handle: %X"), iFileSinkCompHandle);


	OMX_HANDLETYPE jpegEncoderHandle = NULL;
	OMX_STRING jpegEncoderComponentName = "OMX.SYMBIAN.IMAGE.ENCODER.JPEG";

	iError = OMX_GetHandle(&jpegEncoderHandle,
							jpegEncoderComponentName,
							appData,
							omxCallbacks);

	if (OMX_ErrorNone != iError)
		{
		INFO_PRINTF2(_L("Error %08X loading jpeg encoder"), iError);
		OMX_FreeHandle(iGraphicSinkCompHandle);
		OMX_FreeHandle(iCameraSourceCompHandle);
		OMX_FreeHandle(iFileSinkCompHandle);
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}

	iJpegEncoderCompHandle = (OMX_COMPONENTTYPE*) jpegEncoderHandle;
	INFO_PRINTF2(_L("OMX.SYMBIAN.IMAGE.ENCODER.JPEG: Handle: %X"), iJpegEncoderCompHandle);
	
	
    OMX_HANDLETYPE imageWriterHandle = NULL;
    OMX_STRING imageWriterComponentName = "OMX.SYMBIAN.IMAGE.IMAGEWRITER";

    iError = OMX_GetHandle(&imageWriterHandle,
                            imageWriterComponentName,
                            appData,
                            omxCallbacks);

    if (OMX_ErrorNone != iError)
        {
        INFO_PRINTF2(_L("Error %08X loading image writer"), iError);
        OMX_FreeHandle(iGraphicSinkCompHandle);
        OMX_FreeHandle(iCameraSourceCompHandle);
        OMX_FreeHandle(iFileSinkCompHandle);
        OMX_FreeHandle(iJpegEncoderCompHandle);
        SetTestStepError(PrintOmxError(iError));
        return SetTestStepResult(EFail);
        }

    iImageWriterCompHandle = (OMX_COMPONENTTYPE*) imageWriterHandle;
    INFO_PRINTF2(_L("OMX.SYMBIAN.IMAGE.IMAGEWRITER: Handle: %X"), iImageWriterCompHandle);
*/
	InitialiseTestSpecificOmxComponents();
	
	SetTestStepResult(EPass);
	}

void COmxGsTestBase::InitialiseTestSpecificOmxComponents()
    {
    // Disable clock port in Camera Source
//    INFO_PRINTF1(_L("send port 3 to disable"));
//    SendCommand(iCameraSourceCompHandle, OMX_CommandPortDisable, KCameraClockPortIndex, 0);
    
    // Disable port VC in Camera Source
//    INFO_PRINTF1(_L("send port 1 to disable"));
//    SendCommand(iCameraSourceCompHandle, OMX_CommandPortDisable, KCameraVCPortIndex, 0);
    }

void COmxGsTestBase::CreateWindowL()
	{
	iSurfaceUpdateSession.Close();
		iSurfaceManager.Close();iWindowHandle = 0;
	// Connect to a WS Session.
    User::LeaveIfError(iWsSession.Connect());

    // Create a Window Group.
    iWindowGroup = RWindowGroup(iWsSession);
	User::LeaveIfError(iWindowGroup.Construct(iWindowHandle++));

    // Create the Screen device for the WServ session.
    iWsSd = new (ELeave) CWsScreenDevice(iWsSession);
    User::LeaveIfError(iWsSd->Construct());

    //Done in different class
    iWindow = new (ELeave) RWindow(iWsSession);
    User::LeaveIfError(iWindow->Construct(iWindowGroup, iWindowHandle++));

    iWindow2 = new (ELeave) RWindow(iWsSession);
    User::LeaveIfError(iWindow2->Construct(iWindowGroup, iWindowHandle));

    //done in different class
	iGc = new (ELeave) CWindowGc(iWsSd);
    User::LeaveIfError(iGc->Construct());

	iGc2 = new (ELeave) CWindowGc(iWsSd);
    User::LeaveIfError(iGc2->Construct());

    // Reset the screen mode.
    if(iWsSd->CurrentScreenMode() != 0)
	    {
	    iWsSd->SetAppScreenMode(0);
	    iWsSd->SetScreenMode(0);
	    }

	iWindow->Activate();
	iGc->Activate(*iWindow);
	iGc->Clear();
	iWindow->SetVisible(ETrue);
	iWsSession.Flush();
	}

void COmxGsTestBase::CloseTest()
	{
	delete iCallbackHandler;
	iCallbackHandler = NULL;
	if(iTestChunk.Handle())
		{
		iTestChunk.Close();
		}

	if (iGc)
	    {
	    delete iGc;
	    iGc = NULL;
	    }
	if (iGc2)
	    {
	    delete iGc2;
	    iGc2 = NULL;
	    }
	
	if (iWsSd)
	    {
	    delete iWsSd;
	    iWsSd = NULL;
	    }
	
	iWsSession.Close();
	
	if (iWindow)
	    {
	    delete iWindow;
	    iWindow = NULL;
	    }
	
	if (iWindow2)
	    {
	    delete iWindow2;
	    iWindow2 = NULL;
	    }

	if (iTestShutdown)
	    {
	    delete iTestShutdown;
	    iTestShutdown = NULL;
	    }
	
	if (iTestPause)
	    {
	    delete iTestPause;
	    iTestPause = NULL;
	    }
	
	if (iTestStateTransition)
	    {
	    delete iTestStateTransition;
	    iTestStateTransition = NULL;	    
	    }

	iSurfaceUpdateSession.Close();
	iSurfaceManager.Close();
	// Make sure we are not running the Alloc tests, if we are
	// then we have already cleaned up the OMX components and core.
	if(TestStepName() != (_L("MMVIDEO-OMX-CS-004-HP")) && TestStepName() != (_L("MMVIDEO-OMX-GS-001-00-HP"))
			&& TestStepName() != (_L("MMVIDEO-OMX-JP-001-00-HP")) && TestStepName() != (_L("MMVIDEO-OMX-FS-001-00-HP"))
			&& TestStepName() != (_L("MMVIDEO-OMX-IW-001-00-HP")))
		{
		FreeHandles();
		iError = OMX_Deinit();
		}

	REComSession::FinalClose();
	}

TInt COmxGsTestBase::InitialiseSurfaceManager()
	{
	TInt err = iSurfaceManager.Open();
	if (err == KErrNone)
		{
		err = iSurfaceUpdateSession.Connect(3);
		}

	if (err == KErrNotFound)
		{
		INFO_PRINTF1(_L("Failed to find surfacemanger or surfaceupdatesession"));
		INFO_PRINTF1(_L("Make sure 'SYMBIAN_GRAPHICS_USE_GCE ON' is specified in epoc.ini"));
		}

	return err;
	}

TInt COmxGsTestBase::CreateAndMapSurface(const RSurfaceManager::TSurfaceCreationAttributesBuf& aReqs, TSurfaceId& aSurfaceId)
	{
	TInt err = iSurfaceManager.CreateSurface(aReqs, aSurfaceId);
	if (err)
		{
		ERR_PRINTF2(_L("RSurfaceManager::CreateSurface() failed with %d"), err);
		return err;
		}

	err = iSurfaceManager.MapSurface(aSurfaceId, iTestChunk);
	if (err)
		{
		ERR_PRINTF2(_L("RSurfaceManager::MapSurface() failed with %d"), err);
		return err;
		}

	iSurfaceId = aSurfaceId;
	return KErrNone;
	}

void COmxGsTestBase::SendCommand(
			            OMX_HANDLETYPE aComponent,
			            OMX_COMMANDTYPE aCmd,
			            OMX_U32 aParam1,
			            OMX_PTR aCmdData,
			            OMX_ERRORTYPE aExpError)
	{
	iError = OMX_SendCommand(aComponent,aCmd,aParam1,aCmdData);

	if (aExpError != iError)
		{
		ERR_PRINTF1(_L("SendCommandErr"));
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}
	}

void COmxGsTestBase::GetState(OMX_HANDLETYPE aComponent,OMX_STATETYPE* aState,OMX_STATETYPE aExpectedState)
	{
	iError = OMX_GetState(aComponent,aState);
	if (OMX_ErrorNone != iError)
		{
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}

	// PrintOmxState(*aState);
	if (aExpectedState != OMX_StateMax)
		{
		if (aExpectedState != *aState)
			{
			ERR_PRINTF1(_L("Did not return expected state"));
			PrintOmxState(*aState);
			SetTestStepError(KErrGeneral);
			return SetTestStepResult(EFail);
			}
		}

	}

void COmxGsTestBase::SetParameter(
						OMX_HANDLETYPE aComponent,
            			OMX_INDEXTYPE aIndex,
            			OMX_PTR aComponentParameterStructure,
            			OMX_ERRORTYPE aExpError)
	{
	iError = OMX_SetParameter(aComponent,
										aIndex,
										aComponentParameterStructure);
	if (aExpError != iError)
		{
		INFO_PRINTF3(_L("SetParameter() returned %08X instead of %08X"), iError, aExpError);
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}
	}

void COmxGsTestBase::GetParameter(
					OMX_HANDLETYPE aComponent,
            		OMX_INDEXTYPE aIndex,
            		OMX_PTR aComponentParameterStructure,
            		OMX_ERRORTYPE aExpError)
	{
	iError = OMX_GetParameter(aComponent,
										aIndex,
										aComponentParameterStructure);
	if (aExpError != iError)
		{
		ERR_PRINTF3(_L("COmxGsTestBase::GetParameter - expected: %08X got: %08X"), aExpError, iError);
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}
	}

void COmxGsTestBase::SetConfig(
						OMX_HANDLETYPE aComponent,
            			OMX_INDEXTYPE aIndex,
            			OMX_PTR aComponentParameterStructure,
            			OMX_ERRORTYPE aExpError)
	{
	iError = OMX_SetConfig(aComponent,
										aIndex,
										aComponentParameterStructure);
	if (aExpError != iError)
		{
		INFO_PRINTF1(_L("COmxGsTestBase::SetConfig fail"));
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}
	}

void COmxGsTestBase::GetConfig(
					OMX_HANDLETYPE aComponent,
            		OMX_INDEXTYPE aIndex,
            		OMX_PTR aComponentParameterStructure,
            		OMX_ERRORTYPE aExpError)
	{
	iError = OMX_GetConfig(aComponent,
										aIndex,
										aComponentParameterStructure);
	if (aExpError != iError)
		{
		INFO_PRINTF1(_L("COmxGsTestBase::GetConfig fail"));
		SetTestStepError(PrintOmxError(iError));
		return SetTestStepResult(EFail);
		}
	}

void COmxGsTestBase::FreeBuffer(
		OMX_HANDLETYPE aComponent,
        OMX_U32 aPortIndex,
        RPointerArray<OMX_BUFFERHEADERTYPE> aArrayBufferHeaderType,
        OMX_ERRORTYPE aExpError)
	{
	TInt bufferCount = aArrayBufferHeaderType.Count();

	for (TInt i =0; i < bufferCount ; ++i)
		{
		iError = OMX_FreeBuffer(aComponent,aPortIndex,aArrayBufferHeaderType[i]);
		if (aExpError != iError)
			{
			SetTestStepError(PrintOmxError(iError));
			return SetTestStepResult(EFail);
			}
		}
	}

void COmxGsTestBase::AllocateBuffer(
		OMX_HANDLETYPE aComponent,
		OMX_BUFFERHEADERTYPE** aBufferHeaderType,
        OMX_U32 aPortIndex,
        OMX_PTR aAppPrivate,
        OMX_U32 aSizeBytes,
        RPointerArray<OMX_BUFFERHEADERTYPE>* aArrayBufferHeaderType,
        OMX_U32 aCount,
        OMX_ERRORTYPE aExpError)
	{
	for(TInt i = 0 ; i < aCount; i++ )
		{
		iError = OMX_AllocateBuffer(aComponent,*&aBufferHeaderType,aPortIndex,aAppPrivate,aSizeBytes);
		if (aExpError != iError)
			{
			SetTestStepError(PrintOmxError(iError));
			return SetTestStepResult(EFail);
			}

		// Used for tracking
		TInt err = aArrayBufferHeaderType->Append(*aBufferHeaderType);
		if (err != KErrNone)
			{
			INFO_PRINTF2(_L("Append Buffer Failed %d"), err);
			SetTestStepError(err);
			return SetTestStepResult(EFail);
			}
		}
	}

void COmxGsTestBase::FreeHandles()
	{
	// Don't call this function after OMX_DeInit call
	if (iGraphicSinkCompHandle)
		{
		INFO_PRINTF1(_L("FreeHandle for GFX ++"));
		OMX_FreeHandle(iGraphicSinkCompHandle);
		iGraphicSinkCompHandle = NULL;
		}
/*
	if(iCameraSourceCompHandle)
		{
		INFO_PRINTF1(_L("FreeHandle for CAM ++"));
		OMX_FreeHandle(iCameraSourceCompHandle);
		iCameraSourceCompHandle = NULL;
		}

	if (iFileSinkCompHandle)
		{
		INFO_PRINTF1(_L("FreeHandle for FSK ++"));
		OMX_FreeHandle(iFileSinkCompHandle);
		iFileSinkCompHandle = NULL;
		}

	if(iJpegEncoderCompHandle)
		{
		INFO_PRINTF1(_L("FreeHandle for JPEG Encoder ++"));
		OMX_FreeHandle(iJpegEncoderCompHandle);
		iJpegEncoderCompHandle = NULL;
		}
	
    if(iImageWriterCompHandle)
        {
        INFO_PRINTF1(_L("FreeHandle for Image Writer ++"));
        OMX_FreeHandle(iImageWriterCompHandle);
        iImageWriterCompHandle = NULL;
        }
	
	if(iXvidEncoderCompHandle)
        {
        INFO_PRINTF1(_L("FreeHandle for Xvid Encoder ++"));
        OMX_FreeHandle(iXvidEncoderCompHandle);
        iXvidEncoderCompHandle = NULL;
        }
    
    if(i3gpMuxerCompHandle)
        {
        INFO_PRINTF1(_L("FreeHandle for 3gp Muxer ++"));
        OMX_FreeHandle(i3gpMuxerCompHandle);
        i3gpMuxerCompHandle = NULL;
        }
    
    if(iClockCompHandle)
        {
        INFO_PRINTF1(_L("FreeHandle for Clock ++"));
        OMX_FreeHandle(iClockCompHandle);
        iClockCompHandle = NULL;
        }
*/
	}



TInt COmxGsTestBase::StateTransition(const TDesC& /*aStateTransitionName*/)
	{
	return KErrNone;
	}

void COmxGsTestBase::StartTimer()
	{
	TRAPD(err, iTestShutdown = COmxGsTestShutdown::NewL(this,1));
	if(err)
		{
		ERR_PRINTF1(_L("OOM ERROR"));
		return SetTestStepError(err);
		}
	iInterval = KTSU_OMX_GS_Interval;
	iTestShutdown->Start(iInterval,KErrNone, EPass);
	CActiveScheduler::Start();
	}

void COmxGsTestBase::WaitForCallbacks()
	{
	TRAPD(error, iTestShutdown = COmxGsTestShutdown::NewL(this,2));
	if(error)
		{
		ERR_PRINTF1(_L("OOM ERROR"));
		return SetTestStepError(error);
		}

	iInterval = KTSU_OMX_GS_CALLBACK;
	iTestShutdown->Start(iInterval,KErrGeneral, EPass);
	CActiveScheduler::Start();
	}

void COmxGsTestBase::PauseTimer()
	{
	TRAPD(error, iTestPause = COmxGsTestShutdown::NewL(this,3));
	if(error)
		{
		ERR_PRINTF1(_L("OOM ERROR"));
		return SetTestStepError(error);
		}

	iInterval = KTSU_OMX_GS_Pause_Interval/2;
	iTestPause->Start(iInterval,KErrGeneral, EPass);
	}

// ShutDown Class
COmxGsTestShutdown* COmxGsTestShutdown::NewL(COmxGsTestBase* aOmxGsTest,TInt aTimerId)
	{
	COmxGsTestShutdown* self = new(ELeave) COmxGsTestShutdown(aOmxGsTest,aTimerId);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}

COmxGsTestShutdown::COmxGsTestShutdown(COmxGsTestBase* aOmxGsTest,TInt aTimerId)
: CTimer(EPriorityUserInput), iOmxGsTest(aOmxGsTest), iTimerId(aTimerId)
	{
	CActiveScheduler::Add(this);
	}

COmxGsTestShutdown::~COmxGsTestShutdown()
	{
	Cancel();
	}

void COmxGsTestShutdown::ConstructL()
	{
	CTimer::ConstructL();
	}

void COmxGsTestShutdown::Start(TTimeIntervalMicroSeconds32 aInterval, TInt aReason, TVerdict aResult)
	{
	iReason = aReason;
	iResult = aResult;
	After(aInterval);
	}

void COmxGsTestShutdown::RunL()
	{
	iOmxGsTest->PostKickOffTestL(iTimerId);
	}

COmxGsTestStateTransition* COmxGsTestStateTransition::NewL(COmxGsTestBase* aOmxGsTest,TInt aPriority)
	{
	COmxGsTestStateTransition* self = new(ELeave) COmxGsTestStateTransition(aOmxGsTest,aPriority);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;

	}

COmxGsTestStateTransition::COmxGsTestStateTransition(COmxGsTestBase* aOmxGsTest,TInt aPriority)
: CTimer(aPriority), iOmxGsTest(aOmxGsTest)
	{
	CActiveScheduler::Add(this);
	}

COmxGsTestStateTransition::~COmxGsTestStateTransition()
	{
	Cancel();
	}

void COmxGsTestStateTransition::ConstructL()
	{
	CTimer::ConstructL();
	}

void COmxGsTestStateTransition::Start(TTimeIntervalMicroSeconds32 aInterval,const TDesC& aStateTransitionName)
	{
	iStateTransitionName = aStateTransitionName;
	After(aInterval);
	}

void COmxGsTestStateTransition::RunL()
	{
	iOmxGsTest->StateTransition(iStateTransitionName);
	}
