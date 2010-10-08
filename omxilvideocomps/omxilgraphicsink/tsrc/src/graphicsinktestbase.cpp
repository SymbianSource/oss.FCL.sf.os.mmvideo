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


#include "graphicsinktestbase.h"
#include <e32math.h>
#include <openmax/il/shai/OMX_Symbian_ExtensionNames.h>
#include <openmax/il/shai/OMX_Symbian_ComponentExt.h>

const TInt MAXNUMOFBUFFERS = 30; // Due to the size of buffers we limit the max number of buffers

/*
 * There are known issues due to limitations in the OMX core with tests that repeatedly load
 * and unload buffers. If left to run until the timer runs out they inevitably end up failing
 * and causing problems for subsequent test steps due to iffy clean-up. This constant is therefore
 * used to limit the amount of test iterations to a "safe" number.
 */
const TInt KMaxTestIterations = 25;

CGraphicsSinkTestBase::CGraphicsSinkTestBase()
	{
	iTestIteration = 0;
	iTestTimedOut = EFalse;
	iWaitForResources = EFalse;
	iExecuteToIdleCount = 0;
	iDoEmptyBufferDoneLimit = 20;
	iExecutingToIdle = ETrue;
	iDoEmptyBufferDoneCount = 0;
	iIdleToExecuteCount = 0;
	iPauseStateCount = 0;
	}

CGraphicsSinkTestBase::~CGraphicsSinkTestBase()
	{
	
	}

void CGraphicsSinkTestBase::WaitForEvent(OMX_EVENTTYPE aEvent)
	{
	INFO_PRINTF2(_L("Wait for event %d cmd"),aEvent);
	iEventToWaitFor = aEvent;
	CActiveScheduler::Start();
	}

void CGraphicsSinkTestBase::DoFillBufferDone(OMX_HANDLETYPE /*aComponent*/,OMX_BUFFERHEADERTYPE* /*aBufferHeader*/)
	{
	// Should never be called as graphic sink does not support this
    INFO_PRINTF1(_L("CGraphicsSinkTestBase::DoFillBufferDone"));
	}

void CGraphicsSinkTestBase::DoEmptyBufferDone(OMX_HANDLETYPE aComponent,OMX_BUFFERHEADERTYPE* aBufferHeader)
	{
    // INFO_PRINTF3(_L("CGraphicsSinkTestBase::DoEmptyBufferDone: Count %d Limit %d"), iDoEmptyBufferDoneCount,iDoEmptyBufferDoneLimit);
	
    //INFO_PRINTF2(_L("CGraphicsSinkTestBase::DoEmptyBufferDone: Received pBuffer 0x%08x"), aBufferHeader->pBuffer );
    
	TInt error = iInputBufferHeaders.Append(aBufferHeader);
    if (error != KErrNone)
    	{
		ERR_PRINTF1(_L("OOM ERROR"));
		CActiveScheduler::Stop();
		return SetTestStepError(error);
    	}

    if (iIgnoreNextBufferDone)
        {
        iIgnoreNextBufferDone = EFalse;
        return;
        }

	if (iInputBufferHeaders.Count() == 0)
		{
		ERR_PRINTF1(_L("iInputBufferHeaders count has dropped to 0"));
		CActiveScheduler::Stop();
		return SetTestStepResult(EFail);	
		}

	iDoEmptyBufferDoneCount++;

    OMX_COMPONENTTYPE* comp = static_cast<OMX_COMPONENTTYPE*>(aComponent);
	
    if (iTestCase == 10)
    	{
    	if ((iDoEmptyBufferDoneCount < iDoEmptyBufferDoneLimit || iDoEmptyBufferDoneLimit == 0) && !iTestTimedOut)
    		{
			iInputBufferHeader = iInputBufferHeaders[0];
			iInputBufferHeaders.Remove(0);
			
			iInputBufferHeader->nFilledLen = iOmxParamPortInput.format.video.nFrameWidth * 
				COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat) *
				iOmxParamPortInput.format.video.nFrameHeight;
			
			iInputBufferHeader->nFlags = OMX_BUFFERFLAG_SYNCFRAME;
			
			iOmxErrorType = comp->EmptyThisBuffer(comp,iInputBufferHeader);
			if (OMX_ErrorNone != iOmxErrorType)
				{
				ERR_PRINTF1(_L("EmptyThisBuffer returned an error"));
				PrintOmxError(iOmxErrorType);
				return SetTestStepError(iOmxErrorType);
				}
			
    		if (iTestStep == 6)
    			{
    			SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
    			}
			}
    	else
    		{
    		iDoEmptyBufferDoneCount = 0;
            iIgnoreNextBufferDone = ETrue;
    		if (iTestStep != 5 && iTestStep != 6)
    			{
    			SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
    			}
    		}
    	}
    else
    	{
		if (TestStepName() == (_L("MMVIDEO-OMX-GS-005-05-HP")))
			{
			iDoEmptyBufferDoneCount = 0;
			}
		
		if (iDoEmptyBufferDoneCount < iDoEmptyBufferDoneLimit && !iTestTimedOut)
			{
		    //INFO_PRINTF2(_L("CGraphicsSinkTestBase::DoEmptyBufferDone: Sending pBuffer 0x%08x"), iInputBufferHeaders[0]->pBuffer );
		    
			iInputBufferHeader = iInputBufferHeaders[0];
			iInputBufferHeaders.Remove(0);
			
			TInt bytesperpixel = COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat);
			iInputBufferHeader->nFilledLen = iOmxParamPortInput.format.video.nFrameWidth * bytesperpixel *
				iOmxParamPortInput.format.video.nFrameHeight;
			iInputBufferHeader->nOffset = aBufferHeader->nOffset;
			iInputBufferHeader->nFlags = OMX_BUFFERFLAG_SYNCFRAME;
						
			iOmxErrorType = comp->EmptyThisBuffer(comp,iInputBufferHeader);
			if (OMX_ErrorNone != iOmxErrorType)
				{
				ERR_PRINTF1(_L("EmptyThisBuffer returned an error"));
				PrintOmxError(iOmxErrorType);
				return SetTestStepError(iOmxErrorType);
				}
			}
		else
			{
			iDoEmptyBufferDoneCount = 0;
            iIgnoreNextBufferDone = ETrue;
			if (TestStepName() != (_L("MMVIDEO-OMX-GS-004-06-HP")))
				{
				if (iExecutingToIdle)
					{
					SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
					}
				else
					{
					SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StatePause,0);
					}
				}
			}
    	}
	}
	

TVerdict CGraphicsSinkTestBase::DoGSCompAllocTestL()
	{
	TVerdict result = EPass;
	
	// Create a callback handler, graphic sink handle and destroy it
	iError = OMX_Init();
	if(iError != OMX_ErrorNone)
		{
		iError = OMX_Deinit();
		User::Leave(KErrNoMemory);
		}
	
	delete iCallbackHandler;
	iCallbackHandler = NULL;
	TRAPD(error, iCallbackHandler = CCallbackHandler::NewL(*this));
		
	if (!error)
		{
		OMX_CALLBACKTYPE* omxCallbacks = *iCallbackHandler;
		OMX_PTR appData = iCallbackHandler;		
		OMX_HANDLETYPE graphicsSinkHandle = NULL;
		OMX_STRING graphicsSinkComponentName = "OMX.SYMBIAN.VIDEO.GRAPHICSINK";	

		iError = OMX_GetHandle(
		    &graphicsSinkHandle, 
		    graphicsSinkComponentName,
		    appData,
		    omxCallbacks);

		if (iError == OMX_ErrorNone)
			{		
			iGraphicSinkCompHandle = (OMX_COMPONENTTYPE*)graphicsSinkHandle;
			OMX_FreeHandle(iGraphicSinkCompHandle);
			delete iCallbackHandler;
			iCallbackHandler = NULL;
			iError = OMX_Deinit();
			if(iError != OMX_ErrorNone)
				{
				User::Leave(KErrGeneral);
				}
			}
		else 
			{
			result = EFail;
			PrintOmxError(iError);
			ERR_PRINTF2(_L("Unexpected %d return OMX_GetHandle"), iError);
			delete iCallbackHandler;
			iCallbackHandler = NULL;
			iError = OMX_Deinit();
			User::Leave(KErrNoMemory);				
			}
		}
	else
		{
		ERR_PRINTF2(_L("Unexpected %d return CCallbackHandler::NewL()"), iError);
		result = EFail;
		iError = OMX_Deinit();
		User::Leave(KErrNoMemory);				
		}	

	return result;
	}


void CGraphicsSinkTestBase::DoEventHandler(OMX_HANDLETYPE aComponent, OMX_EVENTTYPE aEvent,
			TUint aData1, TUint aData2, TAny* aExtra)
	{
/*	OMX_COMPONENTTYPE* comp = static_cast<OMX_COMPONENTTYPE*>(aComponent);
	if (comp == iCameraSourceCompHandle &&
	        aEvent == OMX_EventCmdComplete &&
	        aData1 == OMX_CommandPortDisable &&
	        (aData2 == KCameraVCPortIndex || aData2 == KCameraClockPortIndex))
	    {
	    return;
	    }
*/	
	OMX_ERRORTYPE errorEvent = static_cast<OMX_ERRORTYPE>( aData1 );
	
	switch (aEvent)
		{
		case OMX_EventError:
			{
			if(TestStepName() == (_L("MMVIDEO-OMX-GS-001-01-HP")))
				{
				ErrorEventTask_001(errorEvent);
				}
			else
				{
				switch (errorEvent)
					{
					case OMX_ErrorFormatNotDetected:
						{
						// INFO_PRINTF1(_L("DoEventHandler: OMX_EventError [OMX_ErrorFormatNotDetected]"));
						PrintOmxError(OMX_ErrorFormatNotDetected);
						CActiveScheduler::Stop();
						return SetTestStepError(KErrGeneral);
						}
					case OMX_ErrorIncorrectStateOperation:
						{
						// INFO_PRINTF1(_L("DoEventHandler: OMX_EventError [OMX_ErrorIncorrectStateOperation]"));
						if(TestStepName() == (_L("MMVIDEO-OMX-GS-002-04-HP")))
							{
							CActiveScheduler::Stop();
							return SetTestStepResult(EPass);
							}
						CActiveScheduler::Stop();
						return SetTestStepError(PrintOmxError(OMX_ErrorIncorrectStateOperation));
						}
					case OMX_ErrorInvalidState:
						{
						// INFO_PRINTF1(_L("DoEventHandler: OMX_EventError [OMX_ErrorInvalidState]"));
						CActiveScheduler::Stop();
						PrintOmxError(OMX_ErrorInvalidState);
						return SetTestStepError(KErrGeneral);
						}
					case OMX_ErrorPortUnpopulated:
						{
						// INFO_PRINTF1(_L("DoEventHandler: OMX_EventError [OMX_ErrorPortUnpopulated]"));
						if (TestStepName() == (_L("MMVIDEO-OMX-GS-006-02-HP")))
							{
							if (--iInputBufferHeadersCount == 0)
							CActiveScheduler::Stop();
							return SetTestStepResult(EPass);
							}
						}
					default:
						{
						INFO_PRINTF2(_L("DoEventHandler: OMX_EventError [%d]"), aData1);
						CActiveScheduler::Stop();
						return SetTestStepError(KErrGeneral);
						}
					}
				}
			}
		case OMX_EventBufferFlag:
			{
			break;
			}
		case OMX_EventCmdComplete:
			{
			InitiateNextStateTransition(aComponent, aData1, aData2, aExtra);
			break;
			}
		case OMX_EventPortSettingsChanged:
			{
			if(iEventToWaitFor == aEvent)
				{
				iEventToWaitFor = OMX_EventMax;
				CActiveScheduler::Stop();
				return SetTestStepResult(EPass);
				}
			else
				{
				UpdateSettingChanged(aComponent, aData1, aData2, aExtra);
				}
			break;
			}

	/*	case OMX_EventNokiaFirstFrameDisplayed:
			{			
			break;
			}*/

		default:
			{
			INFO_PRINTF2(_L("DoEventHandler: OMX Event [%d]"), aEvent);
			CActiveScheduler::Stop();
			return SetTestStepError(KErrGeneral);
			}
		}
	}
	
void CGraphicsSinkTestBase::InitiateNextStateTransition(OMX_HANDLETYPE /*aComponent*/, TUint /*aData1*/,
			TUint aData2, TAny* /*aExtra*/)
	{
	switch(aData2)
		{
		case OMX_StateLoaded:
			{
			if(TestStepName() == (_L("MMVIDEO-OMX-GS-002-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-002-02-HP"))
					 || TestStepName() == (_L("MMVIDEO-OMX-GS-002-03-HP")))
				{
				LoadedStateTask_002();
				}
			else
				{
				if(TestStepName() == (_L("MMVIDEO-OMX-GS-003-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-003-02-HP"))
						 || TestStepName() == (_L("MMVIDEO-OMX-GS-003-03-HP")))
					{
					LoadedStateTask_003();
					}
				else
					{
					if(TestStepName() == (_L("MMVIDEO-OMX-GS-004-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-02-HP"))
								|| TestStepName() == (_L("MMVIDEO-OMX-GS-004-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-04-HP"))
								|| TestStepName() == (_L("MMVIDEO-OMX-GS-004-05-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-06-HP")))
						{
						LoadedStateTask_004();
						}
					else
						{
						if(TestStepName() == (_L("MMVIDEO-OMX-GS-005-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-005-02-HP"))
									|| TestStepName() == (_L("MMVIDEO-OMX-GS-005-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-005-04-HP"))
									|| TestStepName() == (_L("MMVIDEO-OMX-GS-005-05-HP")))
							{
							LoadedStateTask_005();
							}
						else
							{
							if (TestStepName() == (_L("MMVIDEO-OMX-GS-006-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-006-02-HP")))
								{
								LoadedStateTask_006();
								}
							else
								{
								if(TestStepName() == (_L("MMVIDEO-OMX-GS-007-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-007-02-HP")) ||
										TestStepName() == (_L("MMVIDEO-OMX-GS-007-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-007-04-HP")))
									{
									LoadedStateTask_007();
									}
								else
									{
									if (iTestCase == 9)
										{
										LoadedStateTransitionTask();
										}
									else
										{
										if (iTestCase == 10)
											{
											LoadedStateBufferTask();
											}
										else
											{
											INFO_PRINTF1(_L("CGraphicsSinkTestBase::DoEventHandler State set to  OMX_StateLoaded"));
											}
										}
									}
								}
							}
						}
					}
				}
			break;		
			}
		case OMX_StateWaitForResources:
			{
			if (iTestCase == 9)
				{
				WaitForResourcesTransitionTask();
				}
			else
				{
				INFO_PRINTF1(_L("CGraphicsSinkTestBase::DoEventHandler State set to  OMX_StateWaitForResources"));
				}
			break;
			}
		case OMX_StateIdle:
			{
			if(TestStepName() == (_L("MMVIDEO-OMX-GS-002-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-002-02-HP"))
					 || TestStepName() == (_L("MMVIDEO-OMX-GS-002-03-HP")))
				{
				IdleStateTask_002();
				}
			else
				{
				if(TestStepName() == (_L("MMVIDEO-OMX-GS-003-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-003-02-HP"))
						 || TestStepName() == (_L("MMVIDEO-OMX-GS-003-03-HP")))
					{
					IdleStateTask_003();
					}
				else
					{
					if(TestStepName() == (_L("MMVIDEO-OMX-GS-004-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-02-HP"))
								|| TestStepName() == (_L("MMVIDEO-OMX-GS-004-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-04-HP"))
								|| TestStepName() == (_L("MMVIDEO-OMX-GS-004-05-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-06-HP")))
						{
						IdleStateTask_004();
						}
					else
						{
						if(TestStepName() == (_L("MMVIDEO-OMX-GS-005-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-005-02-HP"))
									|| TestStepName() == (_L("MMVIDEO-OMX-GS-005-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-005-04-HP"))
									|| TestStepName() == (_L("MMVIDEO-OMX-GS-005-05-HP")))
							{
							IdleStateTask_005();
							}
						else
							{
							if (TestStepName() == (_L("MMVIDEO-OMX-GS-006-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-006-02-HP")))
								{
								IdleStateTask_006();
								}
							else
								{
								if(TestStepName() == (_L("MMVIDEO-OMX-GS-007-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-007-02-HP")) ||
										TestStepName() == (_L("MMVIDEO-OMX-GS-007-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-007-04-HP")))
									{
									IdleStateTask_007();
									}
								else
									{
									if (iTestCase == 9)
										{
										IdleStateTransitionTask();
										}
									else
										{
										if (iTestCase == 10)
											{
											IdleStateBufferTask();
											}
										else
											{
											INFO_PRINTF1(_L("CGraphicsSinkTestBase::DoEventHandler State set to OMX_StateIdle"));
											}
										}
									}
								}
							}
						}
					}
				}
			break;
			}
		case OMX_StateExecuting:
			{
			if(TestStepName() == (_L("MMVIDEO-OMX-GS-004-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-02-HP"))
										|| TestStepName() == (_L("MMVIDEO-OMX-GS-004-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-04-HP"))
										|| TestStepName() == (_L("MMVIDEO-OMX-GS-004-05-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-06-HP")))
				{
				ExecutingStateTask_004();
				}
			else
				{
				if(TestStepName() == (_L("MMVIDEO-OMX-GS-005-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-005-02-HP"))
							|| TestStepName() == (_L("MMVIDEO-OMX-GS-005-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-005-04-HP"))
							|| TestStepName() == (_L("MMVIDEO-OMX-GS-005-05-HP")))
					{
					ExecutingStateTask_005();
					}
				else
					{
					if (TestStepName() == (_L("MMVIDEO-OMX-GS-007-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-007-02-HP"))
							|| TestStepName() == (_L("MMVIDEO-OMX-GS-007-03-HP")))
						{
						ExecutingStateTask_007();
						}
					else
						{
						if (iTestCase == 9)
							{
							ExecutingStateTransitionTask();
							}
						else
							{
							if (iTestCase == 10)
								{
								ExecutingStateBufferTask();
								}
							else
								{
								switch(iPreviousState)
									{
									case OMX_StateIdle:
										{
										iPreviousState = OMX_StateExecuting;
										EmptyThisBufferTask();
										break;
										}
									case OMX_StatePause:
										{
										iPreviousState = OMX_StateExecuting;
										EmptyThisBufferTask();
										break;
										}
									}
								}
							}
						}
					}
				}
			break;
			}
		case OMX_StatePause:
			{
			if(TestStepName() == (_L("MMVIDEO-OMX-GS-007-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-007-02-HP")) ||
					TestStepName() == (_L("MMVIDEO-OMX-GS-007-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-007-04-HP")))
				{
				PauseStateTask_007();
				}
			else
				{
				INFO_PRINTF1(_L("CGraphicsSinkTestBase::DoEventHandler State set to  OMX_StatePause"));
				}
			break;
			}
		case OMX_StateInvalid:
			{
			if(TestStepName() == (_L("MMVIDEO-OMX-GS-001-01-HP")))
				{
				INFO_PRINTF1(_L("CGraphicsSinkTestBase::GSTest001: Loaded to Invalid"));
				// Not really intended for loaded to invalid test but booleon needed
				iExecutingToIdle = EFalse;
				}
			else
				{
				INFO_PRINTF1(_L("CGraphicsSinkTestBase::DoEventHandler State set to  OMX_StateInvalid"));
				}
			break;
			}
		}
	}

void CGraphicsSinkTestBase::UpdateSettingChanged(OMX_HANDLETYPE /*aComponent*/,TUint aData1,TUint /*aData2*/,TAny* /*aExtra*/)
	{
	TInt err = KErrGeneral;
	
	if(aData1 == iSurfaceConfigExt ||
	   aData1 == OMX_IndexConfigCommonScale ||
	   aData1 == OMX_IndexConfigCommonOutputSize ||
	   aData1 == OMX_IndexConfigCommonInputCrop ||
	   aData1 == OMX_IndexConfigCommonOutputCrop ||
	   aData1 == OMX_IndexConfigCommonExclusionRect)
		{
		// port setting has been changed with Configs
		TSurfaceId surfaceId;
		(*iSurfaceConfig).GetSurfaceId(surfaceId);
		// INFO_PRINTF2(_L("Surface Id:%d"),surfaceId);
		err = iWindow->SetBackgroundSurface(*iSurfaceConfig, ETrue);
		if(KErrNone != err)
			{
			ERR_PRINTF2(_L("SetSurfaceId fails with error: %d"),err);
			SetTestStepError(KErrGeneral);
			return SetTestStepResult(EFail);
			}
		// SetBackgroundSurface OK
		}
	}
	

void CGraphicsSinkTestBase::CloseTestStep()
	{
	delete iCamOutputBuffer;
	
	FreeBuffer( iGraphicSinkCompHandle, 0, iInputBufferHeaders, OMX_ErrorNone );
	iInputBufferHeaders.Reset();
	
	COmxGsTestBase::CloseTest();
	}

void CGraphicsSinkTestBase::EmptyThisBufferTask()
	{
	// INFO_PRINTF2(_L("CGraphicsSinkTestBase::EmptyThisBufferTask: Empty this buffer task Count %d"),iDoEmptyBufferDoneCount);

    TInt bytesperpixel = COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat);
    // CGraphicsSinkTestBase::EmptyThisBufferTask: Fill COmxILMMBuffer* with colour
    FillCCamBuffer(iChunk, iOmxParamPortInput.format.video.nFrameWidth,iOmxParamPortInput.format.video.nFrameHeight,bytesperpixel,iOmxParamPortInput.nBufferCountActual);
    
    //INFO_PRINTF2(_L("CGraphicsSinkTestBase::EmptyThisBufferTask: Sending pBuffer 0x%08x"), iInputBufferHeaders[0]->pBuffer  );
    
    iInputBufferHeader = iInputBufferHeaders[0];
    iInputBufferHeader->nFilledLen = iOmxParamPortInput.format.video.nFrameWidth * 
        COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat) *
        iOmxParamPortInput.format.video.nFrameHeight;
    iInputBufferHeaders.Remove(0);
    iOmxErrorType = iGraphicSinkCompHandle->EmptyThisBuffer(iGraphicSinkCompHandle,iInputBufferHeader);
        
	
	if (OMX_ErrorNone != iOmxErrorType)
		{
		ERR_PRINTF1(_L("EmptyThisBuffer returned an error"));
		return SetTestStepError(KErrGeneral);
		}
	
	if (TestStepName() == (_L("MMVIDEO-OMX-GS-004-06-HP")))
		{
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle, 0);
		}
	else
		{
	    if( iIgnoreNextBufferDone ){
	        iIgnoreNextBufferDone = EFalse;
	        return;
	    }
	    // else...
	    
	    //INFO_PRINTF2(_L("CGraphicsSinkTestBase::EmptyThisBufferTask: Sending pBuffer 0x%08x"), iInputBufferHeaders[0]->pBuffer  );
	    
		iInputBufferHeader = iInputBufferHeaders[0];
		iInputBufferHeaders.Remove(0);
		iInputBufferHeader->nFilledLen = iOmxParamPortInput.format.video.nFrameWidth * 
			COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat) *
			iOmxParamPortInput.format.video.nFrameHeight;
		iInputBufferHeader->nFlags = OMX_BUFFERFLAG_SYNCFRAME;
		iOmxErrorType = iGraphicSinkCompHandle->EmptyThisBuffer(iGraphicSinkCompHandle,iInputBufferHeader);
		if (OMX_ErrorNone != iOmxErrorType)
			{
			ERR_PRINTF1(_L("EmptyThisBuffer returned an error"));
			return SetTestStepError(KErrGeneral);
			}
		}
	}

void CGraphicsSinkTestBase::DoROmxGsTestSetup()
	{
	TRAPD(err,CreateWindowL());
	if(err != KErrNone)
		{
		ERR_PRINTF2(_L("CreateWindow Failed %d"),err);
		return SetTestStepError(err);
		}

	OMX_INDEXTYPE videoSurfaceConfigIndex = OMX_IndexMax;
	
	iError = iGraphicSinkCompHandle->GetExtensionIndex(iGraphicSinkCompHandle, const_cast<char*>(sOmxSymbianGfxSurfaceConfig), &videoSurfaceConfigIndex);
	if(OMX_ErrorNone != iError)
		{
		ERR_PRINTF1(_L("OMX_GetExtensionIndex Failed"));
		PrintOmxError(iError);
		return SetTestStepError(KErrGeneral);
		}
	
	OMX_SYMBIAN_VIDEO_PARAM_SURFACECONFIGURATION surfaceConfig;
	surfaceConfig.nSize = sizeof(OMX_SYMBIAN_VIDEO_PARAM_SURFACECONFIGURATION);
	surfaceConfig.nVersion = TOmxILSpecVersion();
	surfaceConfig.nPortIndex = 0;
	
	GetParameter(iGraphicSinkCompHandle, videoSurfaceConfigIndex, &surfaceConfig);
	if(NULL == surfaceConfig.pSurfaceConfig)
		{
		ERR_PRINTF1(_L("GetParameter Failed 01"));
		return SetTestStepError(KErrGeneral);
		}
		
	iSurfaceConfig = reinterpret_cast<TSurfaceConfiguration*>(surfaceConfig.pSurfaceConfig);
	// INFO_PRINTF2(_L("SurfaceConfig : %x"), iSurfaceConfig);
	TSurfaceId surfaceId;
	(*iSurfaceConfig).GetSurfaceId(surfaceId);
	// INFO_PRINTF2(_L("Surface Id:%d: "),surfaceId);
	
	// Get default OMX_PARAM_PORTDEFINITIONTYPE paramaters
	iOmxParamPortInput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	iOmxParamPortInput.nVersion = TOmxILSpecVersion();
	iOmxParamPortInput.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
	
    // Set the color format to the supported type OMX_COLOR_FormatCbYCrY
	// initial settings
    iOmxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
    iOmxParamPortInput.format.video.nFrameWidth = 320;
    iOmxParamPortInput.format.video.nFrameHeight = 240;
    iOmxParamPortInput.format.video.nStride = 320*2;
    iOmxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
	}

void CGraphicsSinkTestBase::DeleteCCameraBuf()
	{
	// Clean up from AllocateCCameraBuf
	if (iCamOutputBuffer)
		{
		delete iCamOutputBuffer;
		iCamOutputBuffer = NULL;
	
		iTestChunk.Close();
		iSurfaceUpdateSession.CancelAllUpdateNotifications();
		iSurfaceUpdateSession.Close();
		iSurfaceManager.CloseSurface(iSurfaceId);
		iSurfaceId = TSurfaceId::CreateNullId();
		iSurfaceManager.Close();
		}
	}

void CGraphicsSinkTestBase::AllocateCCameraBuf()
	{
	// Setting paramters for surface manager
    iOmxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
    iOmxParamPortInput.format.video.xFramerate = 0;
    iOmxParamPortInput.format.video.nFrameHeight = 320;
    iOmxParamPortInput.format.video.nFrameWidth = 240;
    iOmxParamPortInput.format.video.nStride = iOmxParamPortInput.format.video.nFrameWidth * COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat);
    SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
    
	// Create surface manager
	RSurfaceManager::TSurfaceCreationAttributesBuf bf;
    RSurfaceManager::TSurfaceCreationAttributes& b = bf();
    
    b.iSize.iWidth = iOmxParamPortInput.format.video.nFrameWidth;
    b.iSize.iHeight = iOmxParamPortInput.format.video.nFrameHeight;
    b.iBuffers = iOmxParamPortInput.nBufferCountActual;
    b.iPixelFormat = EUidPixelFormatRGB_565; // Look for conversion method
    b.iStride = iOmxParamPortInput.format.video.nStride;
    b.iOffsetToFirstBuffer = 0;
    b.iOffsetBetweenBuffers = 0; // let surfacemanager choose.
#ifndef __WINSCW__
    b.iAlignment = RSurfaceManager::EPageAligned;
#else
    b.iAlignment = 2;  //TBC  //working algn = 2z
#endif
    b.iContiguous = ETrue;
    b.iMappable = ETrue;
    
	TInt grapherr = InitialiseSurfaceManager();
	if(grapherr != KErrNone && grapherr != KErrAlreadyExists)
		{
		ERR_PRINTF1(_L("CGraphicsSurfaceSink::NewL FAILED"));
		return SetTestStepError(grapherr);
		}

	TRAPD(error, iCamOutputBuffer = COmxILMMBuffer::NewL(iTestChunk));
	if(error != KErrNone)
		{
		ERR_PRINTF1(_L("COmxILMMBuffer::NewL FAILED"));
		return SetTestStepError(error);
		}

	TInt ret = CreateAndMapSurface(bf, iCamOutputBuffer->SurfaceId());
	if(ret != KErrNone)
		{
		ERR_PRINTF2(_L("CreateAndMapSurfaceL FAILED: %d"), ret);
		return SetTestStepError(ret);
		}
	//INFO_PRINTF2(_L("CGraphicsSinkTestBase::UseBufferTask: Chunk size: %d"),iCamOutputBuffer->Chunk().Size());	
	
	RSurfaceManager::TInfoBuf surfacebuf;
	ret = iSurfaceManager.SurfaceInfo(iCamOutputBuffer->SurfaceId(), surfacebuf);
	if(ret != KErrNone)
		{
		ERR_PRINTF1(_L("RSurfaceManager::SurfaceInfo FAILED"));
		return SetTestStepError(ret);
		}
	
	// INFO_PRINTF2(_L("Surface Id:%d "),iCamOutputBuffer->SurfaceId());
	iCamOutputBuffer->SurfaceInfoV01() = surfacebuf();
	// INFO_PRINTF2(_L("Handle of RChunk graph: %d"),iCamOutputBuffer->Chunk().Handle());

	TInt numberofbuffer = b.iBuffers;
	for(TInt i = 0 ; i < numberofbuffer; i++ )
		{
		TInt offset;
		iSurfaceManager.GetBufferOffset(iCamOutputBuffer->SurfaceId(),i,offset);
		
		//INFO_PRINTF3(_L("CGraphicsSinkTestBase::AllocateCCameraBuf() offset no %d = %d"), i , offset);
		
		iCamOutputBuffer->OffsetInfoArray().Append(offset);
		}

	}

void CGraphicsSinkTestBase::AllocateBufferTask()
	{
	TInt bytesperpixel = COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat);
	TInt size = bytesperpixel * iOmxParamPortInput.format.video.nFrameWidth * iOmxParamPortInput.format.video.nFrameHeight;
	
	// Assume that allocate buffer is only called in state loaded
	iPreviousState = OMX_StateLoaded;
	SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
	AllocateBuffer(iGraphicSinkCompHandle,&iInputBufferHeader,0,NULL,size,&iInputBufferHeaders,iOmxParamPortInput.nBufferCountActual);
	// This also confirms that the buffer count is correct, as is shouldnt be zero
	if (iInputBufferHeaders.Count() == 0)
		{
		ERR_PRINTF2(_L("CGraphicsSinkTestBase::AllocateBufferTask: iInputBufferHeaders Count %d"),iInputBufferHeaders.Count());
		CActiveScheduler::Stop();
		return SetTestStepError(KErrGeneral);
		}
	iInputBufferHeadersCount = iInputBufferHeaders.Count();
	// Create COmxILMMBuffer*
	// INFO_PRINTF2(_L("CGraphicsSinkTestBase::AllocateBufferTask: reinterpret_cast COmxILMMBuffer* %d"),iInputBufferHeaders[0]->pInputPortPrivate);
	
	
	// duplicate chunk given from component by getconfig
	OMX_INDEXTYPE sharedChunkMetadataExtensionIndex;
    // due to chunk extension support in gfx
    if (OMX_ErrorNone == OMX_GetExtensionIndex(
            iGraphicSinkCompHandle,
         OMX_SYMBIAN_INDEX_CONFIG_SHAREDCHUNKMETADATA_NAME,
         &sharedChunkMetadataExtensionIndex))
        {
        // Communicate the shared chunk metadata to the tunnelled
        // component
        OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE configSharedChunkMetadata;
        configSharedChunkMetadata.nSize = sizeof(OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE);
        configSharedChunkMetadata.nVersion = TOmxILSpecVersion();            
        configSharedChunkMetadata.nPortIndex = 0;
        
        (void) OMX_GetConfig(iGraphicSinkCompHandle,
                          sharedChunkMetadataExtensionIndex,
                          &configSharedChunkMetadata);
        

        //map the chunk into this process
        RThread chunkOwnerThread;
        User::LeaveIfError(chunkOwnerThread.Open(TThreadId(configSharedChunkMetadata.nOwnerThreadId)));
        CleanupClosePushL(chunkOwnerThread);
                
        iChunk.SetHandle(configSharedChunkMetadata.nHandleId);
        User::LeaveIfError(iChunk.Duplicate(chunkOwnerThread));
        CleanupStack::PopAndDestroy(&chunkOwnerThread);
      
        }
    else
        {
        ERR_PRINTF1(_L("Failed to fetch shared chunk metadata from gfx sink."));
        CActiveScheduler::Stop();
        return SetTestStepError( KErrGeneral );
        }
	
	}

void CGraphicsSinkTestBase::UseBufferTask()
	{
	TInt size = 0;
	if( iCamOutputBuffer->OffsetInfoArray().Count() < 2 ){
	    iSurfaceManager.GetBufferOffset( iCamOutputBuffer->SurfaceId(), 0, size );
	}
	else{
	    size = iCamOutputBuffer->OffsetInfoArray()[1] - iCamOutputBuffer->OffsetInfoArray()[0];
	}
    
    // Assume that use buffer is only called in state loaded
    iPreviousState = OMX_StateLoaded;
    SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
    
    // due to chunk extension support in gfx
    OMX_INDEXTYPE sharedChunkMetadataExtensionIndex;
    if (OMX_ErrorNone == OMX_GetExtensionIndex(
            iGraphicSinkCompHandle,
         OMX_SYMBIAN_INDEX_CONFIG_SHAREDCHUNKMETADATA_NAME,
         &sharedChunkMetadataExtensionIndex))
        {
        // Communicate the shared chunk metadata to the tunnelled
        // component
        OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE configSharedChunkMetadata;
        configSharedChunkMetadata.nSize = sizeof(OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE);
        configSharedChunkMetadata.nVersion = TOmxILSpecVersion();            
        configSharedChunkMetadata.nPortIndex = 0;
        configSharedChunkMetadata.nHandleId = iCamOutputBuffer->Chunk().Handle();
        configSharedChunkMetadata.nOwnerThreadId = RThread().Id().Id();            
        
        iError = OMX_SetConfig(iGraphicSinkCompHandle,
                          sharedChunkMetadataExtensionIndex,
                          &configSharedChunkMetadata);
        
        if (OMX_ErrorNone != iError)
            {
            ERR_PRINTF1(_L("CGraphicsSinkTestBase::UseBufferTask: Set shared chunk config error!"));
            return SetTestStepError(PrintOmxError(iError));
            } 
        }
       
    
	for (TInt i = 0 ; i < iOmxParamPortInput.nBufferCountActual; i++)
		{
	    //INFO_PRINTF4(_L("CGraphicsSinkTestBase::UseBufferTask: size = %d, chunk base = 0x%08x, offset = %d.") , size, iCamOutputBuffer->Chunk().Base(), iCamOutputBuffer->OffsetInfoArray()[i] );
		iError = iGraphicSinkCompHandle->UseBuffer(iGraphicSinkCompHandle,
												&iOutputBufferHeader,
												0,					// input port
												NULL, /*iCamOutputBuffer,*/		// pAppPrivate
												size,		// update
												iCamOutputBuffer->Chunk().Base() + iCamOutputBuffer->OffsetInfoArray()[i] /* // change made due to OMXILBufferClass */);		//
		if (OMX_ErrorNone != iError)
			{
			ERR_PRINTF1(_L("CGraphicsSinkTestBase::UseBufferTask: UseBuffer Error"));
			return SetTestStepError(PrintOmxError(iError));
			}
		// INFO_PRINTF2(_L("surface ID y : %d"), iCamOutputBuffer->SurfaceId());	
		// INFO_PRINTF2(_L("Buffer offset : %d"), iCamOutputBuffer->iBufferOffset);	
		if (iOutputBufferHeaders.Append(iOutputBufferHeader) != KErrNone)
			{
			ERR_PRINTF1(_L("OOM ERROR"));
			return SetTestStepError(KErrGeneral);
			}
		}
	}

void CGraphicsSinkTestBase::AllocateBufferTaskStress()
	{
	// Color format should be default value: OMX_COLOR_Format16bitRGB565
	TInt bytesperpixel = COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat);
	TInt size = bytesperpixel * iOmxParamPortInput.format.video.nFrameWidth * iOmxParamPortInput.format.video.nFrameHeight;
	
	// Assume that allocate buffer is only called in state loaded
	iPreviousState = OMX_StateLoaded;
	SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
	
	for(TInt i = 0 ; i < iOmxParamPortInput.nBufferCountActual; i++ )
		{
		iOmxErrorType = iGraphicSinkCompHandle->AllocateBuffer(iGraphicSinkCompHandle,&iInputBufferHeader,0,NULL,size);
		if (iOmxErrorType != OMX_ErrorNone)
			{
			if (iOmxErrorType == OMX_ErrorInsufficientResources)
				{
				INFO_PRINTF2(_L("CGraphicsSinkTestBase::AllocateBufferTaskStress: AllocateBuffer returned InsufficientResources & nBufferCountActual was set to %d"),iOmxParamPortInput.nBufferCountActual);
				CActiveScheduler::Stop();
				return SetTestStepResult(EPass);
				}
			else
				{
				CActiveScheduler::Stop();
				return SetTestStepError(PrintOmxError(iOmxErrorType));
				}
			}
		
		// Used for tracking
		TRAPD(err,iInputBufferHeaders.AppendL(iInputBufferHeader));
		if(err != KErrNone)
			{
			ERR_PRINTF2(_L("AppendL Buffer Failed %d"),err);
			CActiveScheduler::Stop();
			return SetTestStepError(err);
			}
		}
	
	// This also confirms that the buffer count is correct, as is shouldnt be zero
	if (iInputBufferHeaders.Count() == 0)
		{
		ERR_PRINTF2(_L("CGraphicsSinkTestBase::AllocateBufferTaskStress: iInputBufferHeaders Count %d"),iInputBufferHeaders.Count());
		CActiveScheduler::Stop();
		return SetTestStepError(KErrGeneral);
		}
	
	// Create COmxILMMBuffer*
	// INFO_PRINTF2(_L("CGraphicsSinkTestBase::AllocateBufferTaskStress: reinterpret_cast COmxILMMBuffer* %d"),iInputBufferHeaders[0]->pInputPortPrivate);
	//iCamBuf = static_cast<COmxILMMBuffer*>(iInputBufferHeaders[0]->pInputPortPrivate);
	// duplicate chunk given from component by getconfig
    OMX_INDEXTYPE sharedChunkMetadataExtensionIndex;
    // due to chunk extension support in gfx
    if (OMX_ErrorNone == OMX_GetExtensionIndex(
            iGraphicSinkCompHandle,
         OMX_SYMBIAN_INDEX_CONFIG_SHAREDCHUNKMETADATA_NAME,
         &sharedChunkMetadataExtensionIndex))
        {
        // Communicate the shared chunk metadata to the tunnelled
        // component
        OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE configSharedChunkMetadata;
        configSharedChunkMetadata.nSize = sizeof(OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE);
        configSharedChunkMetadata.nVersion = TOmxILSpecVersion();            
        configSharedChunkMetadata.nPortIndex = 0;
        
        (void) OMX_GetConfig(iGraphicSinkCompHandle,
                          sharedChunkMetadataExtensionIndex,
                          &configSharedChunkMetadata);
        

        //map the chunk into this process
        RThread chunkOwnerThread;
        User::LeaveIfError(chunkOwnerThread.Open(TThreadId(configSharedChunkMetadata.nOwnerThreadId)));
        CleanupClosePushL(chunkOwnerThread);
                
        iChunk.SetHandle(configSharedChunkMetadata.nHandleId);
        User::LeaveIfError(iChunk.Duplicate(chunkOwnerThread));
        CleanupStack::PopAndDestroy(&chunkOwnerThread);
      
        }
    else
        {
        ERR_PRINTF1(_L("Failed to fetch shared chunk metadata from gfx sink."));
        CActiveScheduler::Stop();
        return SetTestStepError( KErrGeneral );
        }
	}

void CGraphicsSinkTestBase::FreeBufferTask(RPointerArray<OMX_BUFFERHEADERTYPE>* aBufferHeaders,
		OMX_U32 aPortIndex,TBool aSendCommand)
	{
	if (aSendCommand)
		{
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateLoaded,0);
		}
	
	FreeBuffer(iGraphicSinkCompHandle,aPortIndex,*aBufferHeaders);
	aBufferHeaders->Reset();
	}

void CGraphicsSinkTestBase::FreeBufferTaskAlt(RPointerArray<OMX_BUFFERHEADERTYPE> aBufferHeaders,
		OMX_U32 aPortIndex,TBool aSendCommand)
	{
	if (aSendCommand)
		{
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateLoaded,0);
		}
	
	FreeBuffer(iGraphicSinkCompHandle,aPortIndex,aBufferHeaders);
	}

void CGraphicsSinkTestBase::FillCCamBuffer(const RChunk& aCamBuf, OMX_U32 aFrameWidth,
		OMX_U32 aFrameHeight, TInt aBytePerPixel, TInt aNumOfActualBuffer)
	{	
	// FillThisBuffer
	TRgb red_col(255,0,0);
	TRgb blue_col(0,255,0);
	TRgb green_col(0,0,255);
	TRgb black_col(0,0,0);
	TUint32 red_color = red_col.Color64K();
	TUint32 blue_color = blue_col.Color64K();
	TUint32 green_color = green_col.Color64K();
	TUint32 black_color = black_col.Color64K();
	TUint32 col[] = {red_color, blue_color, green_color, black_color};
	
	TInt stride = aFrameWidth * aBytePerPixel;
	TInt colIndex = 0;
	for(TInt index=0; index<aNumOfActualBuffer; ++index)
		{
		//TODO: TUint8* surfacePtr = aCamBuf->Chunk().Base() + aCamBuf->BufferOffset();
		TUint8* surfacePtr = aCamBuf.Base() + (iOmxParamPortInput.nBufferSize * index);
		TUint8* linePtr = surfacePtr;
		
		TUint16* ptr = reinterpret_cast<TUint16*>(surfacePtr);
		
		// Fill first line
		for (TInt xx = 0; xx < aFrameWidth; xx++)
			{
			colIndex = Math::Random() % 4;
			ptr[xx] = (TUint16)col[colIndex];
			//ptr[xx] = (TUint16)col[colIndex%4];
			colIndex++;
			}
			
		// Now copy that to the other lines
		for (TInt yy = 1; yy < aFrameHeight; yy++)
			{
			linePtr += stride;
			Mem::Move(linePtr, surfacePtr, stride);
			}	
		}
	}

void CGraphicsSinkTestBase::CreateOmxParamPortDefinitionType(OMX_PARAM_PORTDEFINITIONTYPE* aOmxParamPortType)
	{
	aOmxParamPortType->nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	aOmxParamPortType->nVersion = TOmxILSpecVersion();
	aOmxParamPortType->nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,aOmxParamPortType,OMX_ErrorNone);
    // initial settings
	aOmxParamPortType->format.video.nFrameWidth = 320;
	aOmxParamPortType->format.video.nFrameHeight = 240;
	aOmxParamPortType->format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
	aOmxParamPortType->format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	aOmxParamPortType->format.video.xFramerate = 0;
	aOmxParamPortType->format.video.nStride = 320*2;
	aOmxParamPortType->format.video.nSliceHeight = 10;
	aOmxParamPortType->format.video.nBitrate = 96000;
	}

void CGraphicsSinkTestBase::CreateOmxVideoParamPortFormatType(OMX_VIDEO_PARAM_PORTFORMATTYPE* aOmxVideoParamPortType)
	{
	aOmxVideoParamPortType->nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	aOmxVideoParamPortType->nVersion = TOmxILSpecVersion();
	aOmxVideoParamPortType->nPortIndex = 0;
	aOmxVideoParamPortType->nIndex = 0;
	// The following 2 values should be over written by the correct ones for this index
	aOmxVideoParamPortType->eCompressionFormat = OMX_VIDEO_CodingUnused;
	aOmxVideoParamPortType->eColorFormat = OMX_COLOR_FormatCrYCbY;
	aOmxVideoParamPortType->xFramerate = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,aOmxVideoParamPortType,OMX_ErrorNone);
	}

void CGraphicsSinkTestBase::CompareU32Param(OMX_U32 aSetParamType, OMX_U32 aGetParamType)
	{
	if (aSetParamType != aGetParamType)
		{
		ERR_PRINTF3(_L("Comparison failed:  Set value: %d Get value: %d"),aSetParamType,aGetParamType);
		return SetTestStepError(KErrGeneral);
		}
	}

void CGraphicsSinkTestBase::CompareCFTParam(OMX_COLOR_FORMATTYPE aInputParamType, OMX_COLOR_FORMATTYPE aOutputParamType, OMX_INDEXTYPE aIndexType)
	{
	if (aInputParamType != aOutputParamType)
		{
		ERR_PRINTF3(_L("Comparison failed:  set value: %d Index : %X"),aInputParamType,aIndexType);
		ERR_PRINTF3(_L("Comparison failed:  get value: %d Index : %X"),aOutputParamType,aIndexType);
		return SetTestStepError(KErrGeneral);
		}
	}

void CGraphicsSinkTestBase::CompareVCTParam(OMX_VIDEO_CODINGTYPE aInputParamType, OMX_VIDEO_CODINGTYPE aOutputParamType, OMX_INDEXTYPE aIndexType)
	{
	if (aInputParamType != aOutputParamType)
		{
		ERR_PRINTF3(_L("Comparison failed:  set value: %d Index : %X"),aInputParamType,aIndexType);
		ERR_PRINTF3(_L("Comparison failed:  get value: %d Index : %X"),aOutputParamType,aIndexType);
		return SetTestStepError(KErrGeneral);
		}
	}

void CGraphicsSinkTestBase::CompareBoolParam(OMX_BOOL aInputParamType, OMX_BOOL aOutputParamType, OMX_INDEXTYPE aIndexType)
	{
	if (aInputParamType != aOutputParamType)
		{
		ERR_PRINTF3(_L("Comparison failed:  set value: %d Index : %X"),aInputParamType,aIndexType);
		ERR_PRINTF3(_L("Comparison failed:  get value: %d Index : %X"),aOutputParamType,aIndexType);
		return SetTestStepError(KErrGeneral);
		}
	}

TInt CGraphicsSinkTestBase::PostKickOffTestL(TInt aTimerId)
	{
	if (aTimerId == 1)
		{
		iTestTimedOut = ETrue;
		return KErrNone;
		}
	else
		{
		if (iPreviousState == OMX_StatePause && (TestStepName() == (_L("MMVIDEO-OMX-GS-007-01-HP")) ||
				TestStepName() == (_L("MMVIDEO-OMX-GS-007-03-HP"))))
			{
			SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
			return KErrNone;
			}
		else
			{
			if (iPreviousState == OMX_StatePause && (TestStepName() == (_L("MMVIDEO-OMX-GS-007-02-HP")) ||
					TestStepName() == (_L("MMVIDEO-OMX-GS-007-04-HP"))))
				{
				SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
				return KErrNone;
				}
			}
		}
	return KErrNone;
	}

void CGraphicsSinkTestBase::ErrorEventTask_001(OMX_ERRORTYPE aOmxError)
	{
	// INFO_PRINTF2(_L("GSTest001: EventError iteration: %d"), iTestIteration);
	if (iTestIteration == 0)
		{
		if (aOmxError == OMX_ErrorIncorrectStateTransition)
			{
			GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
			SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StatePause,0,OMX_ErrorNone);
			}
		else
			{
			ERR_PRINTF1(_L("CGraphicsSinkTestBase::ErrorEventTask_001: Invoked incorrect error event"));
			CActiveScheduler::Stop();
			return SetTestStepError(KErrGeneral);
			}
		}
	if (iTestIteration == 1)
		{
		if (aOmxError == OMX_ErrorIncorrectStateTransition)
			{
			GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
			SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateLoaded,0,OMX_ErrorNone);
			}
		else
			{
			ERR_PRINTF1(_L("CGraphicsSinkTestBase::ErrorEventTask_001: Invoked incorrect error event"));
			CActiveScheduler::Stop();
			return SetTestStepError(KErrGeneral);
			}
		}
	if (iTestIteration == 2)
		{
		if (aOmxError == OMX_ErrorSameState)
			{
			GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
			SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateInvalid,0,OMX_ErrorNone);
			}
		else
			{
			ERR_PRINTF1(_L("CGraphicsSinkTestBase::ErrorEventTask_001: Invoked incorrect error event"));
			CActiveScheduler::Stop();
			return SetTestStepError(KErrGeneral);
			}
		}
	if (iTestIteration == 3)
		{
		if (iExecutingToIdle == EFalse && aOmxError == OMX_ErrorInvalidState)
			{
			// INFO_PRINTF1(_L("CGraphicsSinkTestBase::ErrorEventTask_001: Invoked correct error event"));
			CActiveScheduler::Stop();
			return SetTestStepResult(EPass);
			}
		else
			{
			ERR_PRINTF1(_L("CGraphicsSinkTestBase::ErrorEventTask_001: Invoked incorrect error event"));
			CActiveScheduler::Stop();
			return SetTestStepResult(EPass);
			}
		}
	iTestIteration++;
	}


void CGraphicsSinkTestBase::LoadedStateTask_002()
	{
	iPreviousState = OMX_StateLoaded;
	// INFO_PRINTF2(_L("GSTest002: Test complete iteration: %d"), iTestIteration);
	iTestIteration++;

	if(TestStepName() == (_L("MMVIDEO-OMX-GS-002-01-HP")))
		{
		if (iTestIteration == 1)
			{
			INFO_PRINTF1(_L("CGraphicsSinkTestBase::LoadedStateTask_002: __MM_HEAP_MARK"));
			__UHEAP_MARK;
			}
		else
			{
			INFO_PRINTF1(_L("CGraphicsSinkTestBase::LoadedStateTask_002: __MM_HEAP_MARKEND"));
			__UHEAP_MARKEND;
			iTestTimedOut = ETrue;
			}
		}
	
	if(TestStepName() == (_L("MMVIDEO-OMX-GS-002-03-HP")))
		{
		// Only increase the buffer count every 5 times
		iOmxParamPortInput.nBufferCountActual = iOmxParamPortInput.nBufferCountMin + iTestIteration/5;
		// INFO_PRINTF2(_L("CGraphicsSinkTestBase::LoadedStateTask_002: Setting nBufferCountActual: %d"), iOmxParamPortInput.nBufferCountActual);
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
		}
	
	if(iTestIteration < KMaxTestIterations && !iTestTimedOut)
		{
		AllocateBufferTask();
		}
	else
		{
		CActiveScheduler::Stop();
		return SetTestStepResult(EPass);
		}
	}
	
void CGraphicsSinkTestBase::IdleStateTask_002()
	{
	iPreviousState = OMX_StateIdle;
	FreeBufferTask(&iInputBufferHeaders,0);
	}

void CGraphicsSinkTestBase::LoadedStateTask_003()
	{
	iPreviousState = OMX_StateLoaded;
	// INFO_PRINTF2(_L("GSTest003: Test complete iteration: %d"), iTestIteration);
	iTestIteration++;

	if(TestStepName() == (_L("MMVIDEO-OMX-GS-003-01-HP")))
		{
		if (iTestIteration == 1)
			{
			INFO_PRINTF1(_L("CGraphicsSinkTestBase::LoadedStateTask_003: __MM_HEAP_MARK"));
			__UHEAP_MARK;
			}
		else
			{
			INFO_PRINTF1(_L("CGraphicsSinkTestBase::LoadedStateTask_003: __MM_HEAP_MARKEND"));
			DeleteCCameraBuf();
			__UHEAP_MARKEND;
			iTestTimedOut = ETrue;
			}
		}

	if(TestStepName() == (_L("MMVIDEO-OMX-GS-003-03-HP")))
		{
		// Only increase the buffer count every 5 times
		iOmxParamPortInput.nBufferCountActual = iOmxParamPortInput.nBufferCountMin + iTestIteration/5;
		// INFO_PRINTF2(_L("CGraphicsSinkTestBase::LoadedStateTask_003: Setting nBufferCountActual: %d"), iOmxParamPortOutput.nBufferCountActual);
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
		}
	
	if(!iTestTimedOut)
		{
		DeleteCCameraBuf();
		AllocateCCameraBuf();
		UseBufferTask();
		}
	else
		{
		CActiveScheduler::Stop();
		return SetTestStepResult(EPass);
		}
	}
	
void CGraphicsSinkTestBase::IdleStateTask_003()
	{
	iPreviousState = OMX_StateIdle;
	FreeBufferTask(&iOutputBufferHeaders,0);
	}

void CGraphicsSinkTestBase::LoadedStateTask_004()
	{
	// INFO_PRINTF2(_L("GSTest004: Test complete iteration: %d"), iTestIteration);
	iTestIteration++;
	
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	iPreviousState = OMX_StateLoaded;
	if(TestStepName() == (_L("MMVIDEO-OMX-GS-004-01-HP")))
		{
		if (iTestIteration == 1)
			{
			INFO_PRINTF1(_L("CGraphicsSinkTestBase::LoadedStateTask_004: __MM_HEAP_MARK"));
			__UHEAP_MARK;
			}
		else
			{
			INFO_PRINTF1(_L("CGraphicsSinkTestBase::LoadedStateTask_004: __MM_HEAP_MARKEND"));
			__UHEAP_MARKEND;
			iTestTimedOut = ETrue;
			}
		}
	
	if(TestStepName() == (_L("MMVIDEO-OMX-GS-004-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-05-HP")))
		{
		iOmxParamPortInput.nBufferCountActual = 5 + iTestIteration/5;
		INFO_PRINTF2(_L("CGraphicsSinkTestBase::LoadedStateTask_004: Setting nBufferCountActual: %d"), iOmxParamPortInput.nBufferCountActual);
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
		}
	
	if(iTestIteration < KMaxTestIterations && !iTestTimedOut)
		{
		AllocateBufferTask();
		}
	else 
		{
		CActiveScheduler::Stop();
		return SetTestStepResult(EPass);
		}
	}

void CGraphicsSinkTestBase::IdleStateTask_004()
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateIdle);
	if(iPreviousState == OMX_StateLoaded)
		{
		iPreviousState = OMX_StateIdle;
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
		}
	else if(iPreviousState == OMX_StateExecuting)
		{
		// INFO_PRINTF2(_L("CGraphicsSinkTestBase::IdleStateTask_004: Executing to Idle task Count %d"),iExecuteToIdleCount);
		iExecuteToIdleCount++;
		iPreviousState = OMX_StateIdle;
		
		if (TestStepName() == (_L("MMVIDEO-OMX-GS-004-01-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-02-HP"))
				|| TestStepName() == (_L("MMVIDEO-OMX-GS-004-03-HP")))
			{
			FreeBufferTask(&iInputBufferHeaders,0);
			}
		
		if (TestStepName() == (_L("MMVIDEO-OMX-GS-004-04-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-004-05-HP"))
				 || TestStepName() == (_L("MMVIDEO-OMX-GS-004-06-HP")))
			{
			if (iExecuteToIdleCount < 20)
				{
				SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
				}
			else
				{
				FreeBufferTask(&iInputBufferHeaders,0);
				}
			}
		}
	}
	
void CGraphicsSinkTestBase::ExecutingStateTask_004()
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateExecuting);
	iPreviousState = OMX_StateExecuting;
	
	if (TestStepName() == (_L("MMVIDEO-OMX-GS-004-06-HP")))
		{
		iExecuteToIdleCount = 20;
		iTestTimedOut = ETrue;
		EmptyThisBufferTask();
		}
	else
		{
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle, 0);
		}
	}

void CGraphicsSinkTestBase::LoadedStateTask_005()
	{
	// INFO_PRINTF2(_L("GSTest005: Test complete iteration: %d"), iTestIteration);
	iTestIteration++;
	
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	iPreviousState = OMX_StateLoaded;
	if(TestStepName() == (_L("MMVIDEO-OMX-GS-005-01-HP")))
		{
		if (iTestIteration == 1)
			{
			INFO_PRINTF1(_L("CGraphicsSinkTestBase::LoadedStateTask_005: __MM_HEAP_MARK"));
			__UHEAP_MARK;
			}
		else
			{
			INFO_PRINTF1(_L("CGraphicsSinkTestBase::LoadedStateTask_005: __MM_HEAP_MARKEND"));
			__UHEAP_MARKEND;
			iTestTimedOut = ETrue;
			}
		}
	
	if(TestStepName() == (_L("MMVIDEO-OMX-GS-005-03-HP")))
		{
		iOmxParamPortInput.nBufferCountActual = Min(5 + iTestIteration, 100); // let's cap the amount of buffers :-)
		// INFO_PRINTF2(_L("CGraphicsSinkTestBase::LoadedStateTask_005: Setting nBufferCountActual: %d"), iOmxParamPortInput.nBufferCountActual);
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
		}
	
	if(iTestIteration < KMaxTestIterations && !iTestTimedOut)
		{
		AllocateBufferTask();
		}
	else
		{
		
		CActiveScheduler::Stop();
		return SetTestStepResult(EPass);
		}
	}

void CGraphicsSinkTestBase::IdleStateTask_005()
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateIdle);
	if(iPreviousState == OMX_StateLoaded)
		{
		iPreviousState = OMX_StateIdle;
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
		}
	if(iPreviousState == OMX_StateExecuting)
		{
		iPreviousState = OMX_StateIdle;
		if (TestStepName() == (_L("MMVIDEO-OMX-GS-005-04-HP")) && !iTestTimedOut)
			{
			// INFO_PRINTF2(_L("CGraphicsSinkTestBase::IdleStateTask_005: Test complete iteration: %d"), iTestIteration);
			iTestIteration++;
			SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
			}
		else
			{
			FreeBufferTask(&iInputBufferHeaders,0);
			}
		}
	}

void CGraphicsSinkTestBase::ExecutingStateTask_005()
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateExecuting);
	iPreviousState = OMX_StateExecuting;
	EmptyThisBufferTask();
	}

void CGraphicsSinkTestBase::LoadedStateTask_006()
	{
	// INFO_PRINTF2(_L("GSTest006: Test complete iteration: %d"), iTestIteration);
	iTestIteration++;

	iPreviousState = OMX_StateLoaded;
	if(TestStepName() == (_L("MMVIDEO-OMX-GS-006-01-HP")) && !iTestTimedOut)
		{
		iOmxParamPortInput.nBufferCountActual = iOmxParamPortInput.nBufferCountActual + 1;
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
		AllocateBufferTaskStress();
		}
	else
		{
		CActiveScheduler::Stop();
		return SetTestStepResult(EPass);
		}
	}
		
void CGraphicsSinkTestBase::IdleStateTask_006()
	{
	iPreviousState = OMX_StateIdle;
	if(TestStepName() == (_L("MMVIDEO-OMX-GS-006-01-HP")))
		{
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateLoaded,0);

		FreeBuffer(iGraphicSinkCompHandle,0,iInputBufferHeaders);
		iInputBufferHeaders.Reset();
		}
	if(TestStepName() == (_L("MMVIDEO-OMX-GS-006-02-HP")))
		{
		// Allocate to many buffers
		TInt bytesperpixel = COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat);
		TInt size = bytesperpixel * iOmxParamPortInput.format.video.nFrameWidth * iOmxParamPortInput.format.video.nFrameHeight;
		
		iOmxErrorType = iGraphicSinkCompHandle->AllocateBuffer(iGraphicSinkCompHandle,&iInputBufferHeader,0,NULL,size);
		if (OMX_ErrorIncorrectStateOperation != iOmxErrorType)
			{
			ERR_PRINTF1(_L("AllocateBuffer failed to return error"));
			CActiveScheduler::Stop();
			return SetTestStepError(PrintOmxError(iOmxErrorType));
			}

		// Confirm state remains OMX_StateIdle
		GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateIdle);
		
		// FreeBuffer without calling SendCommand to OMX_StateLoaded
		FreeBuffer(iGraphicSinkCompHandle, 0, iInputBufferHeaders);
		iInputBufferHeaders.Reset();
		}
	}

void CGraphicsSinkTestBase::LoadedStateTask_007()
	{
	switch (iPreviousState)
		{
		case OMX_StateIdle:
			{
			iPreviousState = OMX_StateLoaded;
			CActiveScheduler::Stop();
			return SetTestStepResult(EPass);
			}
		case OMX_StateWaitForResources:
			{
			// State is OMX_StateIdle from OMX_StateWaitForResources
			break;
			}
		}
	}

void CGraphicsSinkTestBase::IdleStateTask_007()
	{
	switch (iPreviousState)
		{
		case OMX_StateLoaded:
			{
			iPreviousState = OMX_StateIdle;
			if (TestStepName() == (_L("MMVIDEO-OMX-GS-007-03-HP")) || TestStepName() == (_L("MMVIDEO-OMX-GS-007-04-HP")))
				{
				SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StatePause,0);
				}
			else
				{
				SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
				}
			break;
			}
		case OMX_StateExecuting:
			{
			iPreviousState = OMX_StateIdle;
			if (TestStepName() == (_L("MMVIDEO-OMX-GS-007-03-HP")) && !iTestTimedOut)
				{
				SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StatePause,0);
				}
			else
				{
				// INFO_PRINTF2(_L("CGraphicsSinkTestBase::IdleStateTask_007:: Executing to Idle task Count %d"),iExecuteToIdleCount);
				iExecuteToIdleCount++;
				SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateLoaded,0);
				
				FreeBuffer(iGraphicSinkCompHandle,0,iInputBufferHeaders);
				iInputBufferHeaders.Reset();
				}
			break;
			}
		case OMX_StatePause:
			{
			iPreviousState = OMX_StateIdle;
			if (TestStepName() == (_L("MMVIDEO-OMX-GS-007-04-HP")) && !iTestTimedOut)
				{
				SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StatePause,0);
				}
			else
				{
				if (iTestTimedOut)
					{
					// INFO_PRINTF2(_L("CGraphicsSinkTestBase::IdleStateTask_007:: Executing to Idle task Count %d"),iExecuteToIdleCount);
					iExecuteToIdleCount++;
					SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateLoaded,0);
					
					FreeBuffer(iGraphicSinkCompHandle,0,iInputBufferHeaders);
					iInputBufferHeaders.Reset();
					}
				else
					{
					SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
					}
				}
			}
		}
	}

void CGraphicsSinkTestBase::PauseStateTask_007()
	{
	iPreviousState = OMX_StatePause;
	// INFO_PRINTF2(_L("CGraphicsSinkTestBase::PauseStateTask_007:: Pause state task count %d"),iPauseStateCount);
	if(++iPauseStateCount == 5)
		{
		iTestTimedOut = ETrue;
		}
	
	if(iTestShutdown)
		{
		delete iTestShutdown;
		}
	
	TRAPD(err, iTestShutdown = COmxGsTestShutdown::NewL(this,2));
	if(err)
		{
		ERR_PRINTF1(_L("OOM ERROR"));
		CActiveScheduler::Stop();
		return SetTestStepError(err);
		}
	iInterval = KTSU_OMX_GS_PAUSE;
	iTestShutdown->Start(iInterval,KErrGeneral, EPass);
	}

void CGraphicsSinkTestBase::ExecutingStateTask_007()
	{
	iPreviousState = OMX_StateExecuting;
	// INFO_PRINTF2(_L("CGraphicsSinkTestBase::ExecutingStateTask_007 Idle to Executing task Count %d"),iIdleToExecuteCount);
    iIdleToExecuteCount++;
    
    if (TestStepName() == (_L("MMVIDEO-OMX-GS-007-03-HP")) || iTestTimedOut)
    	{
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
    	}
    else
    	{
    	EmptyThisBufferTask();
    	}
	}

void CGraphicsSinkTestBase::StartStateTransitionTask()
	{
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateLoaded);
	iGphxPrevState = iState;
	
	if (iTestStep == 4)
		{
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateWaitForResources,0);
		}
	else
		{
		AllocateBufferTask();
		}
	}

void CGraphicsSinkTestBase::WaitForResourcesTransitionTask()
	{
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateWaitForResources);
	iGphxPrevState = iState;
	
	if (iTestStep == 4)
		{
		iWaitForResources = ETrue;
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateLoaded,0);
		}
	}

void CGraphicsSinkTestBase::LoadedStateTransitionTask()
	{
	//INFO_PRINTF3(_L("GSTest00%d: Test complete iteration: %d"), iTestCase, iTestIteration);
	iTestIteration++;
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateLoaded);
	
	iGphxPrevState = iState;
	if(iTestStep == 1)
		{
		if (iTestIteration == 1)
			{
			INFO_PRINTF3(_L("LoadedStateTransitionTask_00%d_0%d: __MM_HEAP_MARK"), iTestCase,iTestStep);
			__UHEAP_MARK;
			}
		else
			{
			INFO_PRINTF3(_L("LoadedStateTransitionTask_00%d_0%d: __MM_HEAP_MARKEND"), iTestCase,iTestStep);
			__UHEAP_MARKEND;
			iTestTimedOut = ETrue;
			}
		}
	
	if(iTestStep == 3)
		{
		if ((iOmxParamPortInput.nBufferCountMin + iTestIteration/10) <= MAXNUMOFBUFFERS)
			{
			iOmxParamPortInput.nBufferCountActual = iOmxParamPortInput.nBufferCountMin + iTestIteration/10;
			//INFO_PRINTF3(_L("GSTest00%d: Setting INPUT nBufferCountActual: %d"),iTestCase,iOmxParamPortInput.nBufferCountActual);
			SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
			}
		}
	
	if(iTestIteration < KMaxTestIterations && !iTestTimedOut)
		{
		if (iTestStep == 4 && iWaitForResources)
			{
			SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateWaitForResources,0);
			}
		else
			{
			iWaitForResources = EFalse;
			AllocateBufferTask();
			}
		}
	else
		{
		CActiveScheduler::Stop();
		}
	}

void CGraphicsSinkTestBase::IdleStateTransitionTask()
	{
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateIdle);
	if(iGphxPrevState == OMX_StateLoaded)
		{
		iGphxPrevState = iState;
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
		}
	else if(iGphxPrevState == OMX_StateExecuting)
		{
		iGphxPrevState = iState;
		FreeBufferTaskAlt(iInputBufferHeaders,0);
		iInputBufferHeaders.Reset();
		}
	}

void CGraphicsSinkTestBase::ExecutingStateTransitionTask()
	{
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateExecuting);
	iGphxPrevState = iState;
	
	SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
	}

void CGraphicsSinkTestBase::StartBufferDoneTask()
	{
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateLoaded);
	iGphxPrevState = iState;
	
	AllocateBufferTask();
	}

void CGraphicsSinkTestBase::LoadedStateBufferTask()
	{
	// INFO_PRINTF3(_L("GSTest00%d: Test complete iteration: %d"), iTestCase, iTestIteration);
	iTestIteration++;
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateLoaded);
	iGphxPrevState = iState;
	if(iTestStep == 1 || iTestStep == 5|| iTestStep == 6)
		{
		if (iTestIteration == 1)
			{
			INFO_PRINTF3(_L("LoadedStateTransitionTask_00%d_0%d: __MM_HEAP_MARK"), iTestCase,iTestStep);
			__UHEAP_MARK;
			}
		else
			{
			INFO_PRINTF3(_L("LoadedStateTransitionTask_00%d_0%d: __MM_HEAP_MARKEND"), iTestCase,iTestStep);
			__UHEAP_MARKEND;
			iTestTimedOut = ETrue;
			}
		}
	
	if(iTestStep == 3)
		{
		OMX_U32 setbuffercount = iOmxParamPortInput.nBufferCountMin + iTestIteration/10;
		if (setbuffercount <= MAXNUMOFBUFFERS && setbuffercount != iOmxParamPortInput.nBufferCountActual)
			{
			iOmxParamPortInput.nBufferCountActual = iOmxParamPortInput.nBufferCountMin + iTestIteration/10;
			iDoEmptyBufferDoneLimit = iOmxParamPortInput.nBufferCountActual;
			INFO_PRINTF4(_L("GSTest00%d: Setting INPUT nBufferCountActual: %d DoEmptyBufferDoneLimit: %d")
					,iTestCase,iOmxParamPortInput.nBufferCountActual,iDoEmptyBufferDoneLimit);
			SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&iOmxParamPortInput);
			}
		}
	
	if(iTestIteration < KMaxTestIterations && !iTestTimedOut)
		{
		iWaitForResources = EFalse;
		AllocateBufferTask();
		}
	else
		{
		CActiveScheduler::Stop(); // End the test
		}
	}

void CGraphicsSinkTestBase::IdleStateBufferTask()
	{
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateIdle);
	if(iGphxPrevState == OMX_StateLoaded)
		{
		iGphxPrevState = iState;
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0);
		}
	else if(iGphxPrevState == OMX_StateExecuting)
		{
		iGphxPrevState = iState;
		FreeBufferTaskAlt(iInputBufferHeaders,0);
		iInputBufferHeaders.Reset();
		}
	}

void CGraphicsSinkTestBase::ExecutingStateBufferTask()
	{
	GetState(iGraphicSinkCompHandle,&iState,OMX_StateExecuting);
	iGphxPrevState = iState;
	
	EmptyThisBufferTask();
	if (iTestStep == 5)
		{
		SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
		}
	}
