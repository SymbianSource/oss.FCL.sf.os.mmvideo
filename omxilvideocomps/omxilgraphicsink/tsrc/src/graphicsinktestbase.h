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

#ifndef GRAPHICSINKTESTBASE_H
#define GRAPHICSINKTESTBASE_H

#include <openmax/il/extensions/omxilsymbianvideographicsinkextensions.h>
#include <graphics/surfaceconfiguration.h>
#include <graphics/surfaceupdateclient.h>
#include <openmax/il/common/omxilspecversion.h>

#include "omxilgraphicsinktestbase.h"

const TInt KTSU_OMX_GS_PAUSE = 2000000;// 2 second timer for test shutdown

class CGraphicsSinkTestBase : public COmxGsTestBase
	{
public:
	CGraphicsSinkTestBase();
	~CGraphicsSinkTestBase();

public:
	void CloseTestStep();
	// Functions to act on callbackhandler functions
	void DoFillBufferDone(OMX_HANDLETYPE aComponent,
			  	OMX_BUFFERHEADERTYPE* aBufferHeader);

	void DoEmptyBufferDone(OMX_HANDLETYPE aComponent,
			   	OMX_BUFFERHEADERTYPE* aBufferHeader);

	void DoEventHandler(OMX_HANDLETYPE aComponent,OMX_EVENTTYPE aEvent,
				TUint aData1,TUint aData2,TAny* aExtra);
	// End of callbackhandler functions

	void InitiateNextStateTransition(OMX_HANDLETYPE aComponent, TUint aData1, 
				TUint aData2, TAny* aExtra);

	void UpdateSettingChanged(OMX_HANDLETYPE aComponent, TUint aData1, 
				TUint aData2, TAny* aExtra);

	
protected:
	
	void ErrorEventTask_001(
			OMX_ERRORTYPE aOmxError);

	TVerdict DoGSCompAllocTestL();

	void LoadedStateTask_002();
	void IdleStateTask_002();

	void LoadedStateTask_003();
	void IdleStateTask_003();

	void LoadedStateTask_004();
	void IdleStateTask_004();
	void ExecutingStateTask_004();
	
	void LoadedStateTask_005();
	void IdleStateTask_005();
	void ExecutingStateTask_005();
	
	void LoadedStateTask_006();
	void IdleStateTask_006();

	void LoadedStateTask_007();
	void IdleStateTask_007();
	void PauseStateTask_007();
	void ExecutingStateTask_007();

	void StartStateTransitionTask();
	void WaitForResourcesTransitionTask();
	void LoadedStateTransitionTask();
	void IdleStateTransitionTask();
	void ExecutingStateTransitionTask();
	
	void StartBufferDoneTask();
	void LoadedStateBufferTask();
	void IdleStateBufferTask();
	void ExecutingStateBufferTask();
	
	void DoROmxGsTestSetup();
	void AllocateCCameraBuf();
	void DeleteCCameraBuf();
	void AllocateBufferTask();
	void AllocateBufferTaskStress();
	void UseBufferTask();
	void FreeBufferTask(RPointerArray<OMX_BUFFERHEADERTYPE>* aBufferHeaders,
			OMX_U32 aPortIndex,TBool aSendCommand = ETrue);
	void FreeBufferTaskAlt(RPointerArray<OMX_BUFFERHEADERTYPE> aBufferHeaders,
			OMX_U32 aPortIndex,TBool aSendCommand = ETrue);
	void EmptyThisBufferTask();
	void FillCCamBuffer(
				const RChunk& aCamBuf, 
				OMX_U32 aFrameWidth, 
				OMX_U32 aFrameHeight, 
				TInt aBytePerPixel, 
				TInt aNumOfActualBuffer);
	void CreateOmxParamPortDefinitionType(OMX_PARAM_PORTDEFINITIONTYPE* aOmxParamPortType);
	void CreateOmxVideoParamPortFormatType(OMX_VIDEO_PARAM_PORTFORMATTYPE* aOmxVideoParamPortType);
	
	TInt PostKickOffTestL(TInt aTimerId);
	// General functions used to test the outcome of Get and Set tests.
	void CompareVCTParam(OMX_VIDEO_CODINGTYPE aInputParamType, OMX_VIDEO_CODINGTYPE aOutputParamType, OMX_INDEXTYPE aIndexType);
	void CompareU32Param(OMX_U32 aInputParamType, OMX_U32 aOutputParamType);
	void CompareCFTParam(OMX_COLOR_FORMATTYPE aInputParamType, OMX_COLOR_FORMATTYPE aOutputParamType, OMX_INDEXTYPE aIndexType);
	void CompareBoolParam(OMX_BOOL aInputParamType, OMX_BOOL aOutputParamType, OMX_INDEXTYPE aIndexType);

	void WaitForEvent(OMX_EVENTTYPE aEvent);

protected:
	//COmxILMMBuffer* iCamBuf;
	RChunk iChunk; // to replace with COmxILMMBuffer
	COmxILMMBuffer* iCamOutputBuffer;
	
	OMX_BUFFERHEADERTYPE* iInputBufferHeader;
	OMX_BUFFERHEADERTYPE* iOutputBufferHeader;
	RPointerArray<OMX_BUFFERHEADERTYPE> iInputBufferHeaders;
	RPointerArray<OMX_BUFFERHEADERTYPE> iOutputBufferHeaders;
	
	OMX_PARAM_PORTDEFINITIONTYPE iOmxParamPortInput;
	
	OMX_STATETYPE iOmxStateType;
	OMX_ERRORTYPE iOmxErrorType;

	TUint iPreviousState;
	TInt iDoEmptyBufferDoneCount;
	TInt iDoEmptyBufferDoneLimit;
	TInt iColSwitch;
	TInt iExecuteToIdleCount;
	TInt iIdleToLoadedCount;
	TInt iPauseStateCount;
	TInt iIdleToExecuteCount;
	TBool iExecutingToIdle;

    TSurfaceConfiguration* iSurfaceConfig;
    
    TInt iTestIteration;
    volatile TBool iTestTimedOut;
    volatile TBool iWaitForResources;
	TInt iInputBufferHeadersCount;
	
	OMX_EVENTTYPE iEventToWaitFor;
	
	TInt iTestCase;
	TInt iTestStep;

	TBool iIgnoreNextBufferDone;
    };

#endif //GRAPHICSINKTESTBASE_H
