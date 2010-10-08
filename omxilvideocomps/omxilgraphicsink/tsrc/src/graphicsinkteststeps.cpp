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

#include "graphicsinkteststeps.h"
#include <openmax/il/shai/OMX_Symbian_ExtensionNames.h>
#include <openmax/il/shai/OMX_Symbian_ComponentExt.h>

COmxGsTest0001Step01::~COmxGsTest0001Step01()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step01::COmxGsTest0001Step01()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step01);
	}

TVerdict COmxGsTest0001Step01::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	
	// Check GetComponentVersion
	const TInt maxComponentNameSize = 128;

	char componentNameArray[maxComponentNameSize];
	OMX_VERSIONTYPE componentVersion;
	OMX_VERSIONTYPE specVersion;
	OMX_UUIDTYPE componentUUID;
	
	// Obtain the component's version
	iOmxErrorType = iGraphicSinkCompHandle->GetComponentVersion(iGraphicSinkCompHandle,
			componentNameArray,&componentVersion,&specVersion,&componentUUID);
	if (iOmxErrorType != OMX_ErrorNone)
		{
		SetTestStepError(KErrGeneral);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	// Translate component name
	TBuf8<maxComponentNameSize> componentNameBuf8;
	componentNameBuf8 = const_cast<const TUint8*>(reinterpret_cast<TUint8*>(componentNameArray));
	TBuf<maxComponentNameSize> componentNameBuf16;
	// INFO_PRINTF2(_L("Component Name length: %d"), componentNameBuf8.Length());
	componentNameBuf16.Copy(componentNameBuf8);
	componentNameBuf16.PtrZ();
	const TBuf<maxComponentNameSize> componentNameConst = _L("OMX.SYMBIAN.VIDEO.GRAPHICSINK");
	
	INFO_PRINTF2(_L("Component Name: %S"), &componentNameBuf16);
	INFO_PRINTF2(_L("Component Version Major: %d"), componentVersion.s.nVersionMajor);
	INFO_PRINTF2(_L("Component Version Minor: %d"), componentVersion.s.nVersionMinor);
	INFO_PRINTF2(_L("Component Version Revision: %d"), componentVersion.s.nRevision);
	INFO_PRINTF2(_L("Component Version Step: %d"), componentVersion.s.nStep);
	INFO_PRINTF2(_L("OMX Version Major: %d"), specVersion.s.nVersionMajor);
	INFO_PRINTF2(_L("OMX Version Minor: %d"), specVersion.s.nVersionMinor);
	INFO_PRINTF2(_L("OMX Version Revision: %d"), specVersion.s.nRevision);
	INFO_PRINTF2(_L("OMX Version Step: %d"), specVersion.s.nStep);
	INFO_PRINTF2(_L("Component UUID: %X"), componentUUID);

	// Quick check to confirm component name
	if (componentNameConst != componentNameBuf16)
		{
		ERR_PRINTF1(_L("Incorrect component name retuned"));
		SetTestStepError(KErrGeneral);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// Simple GetParam test to display data change when using GetParam
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortDefType;
	// Confirm error when required paramaters not set
	omxParamPortDefType.nPortIndex = 5;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortDefType,OMX_ErrorBadPortIndex);

	// Set required paramaters and test GetParameter 
	omxParamPortDefType.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortDefType.nVersion = TOmxILSpecVersion();
	omxParamPortDefType.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortDefType);

	INFO_PRINTF2(_L("PORTDEFINITIONTYPE nBufferCountActual: %X"), omxParamPortDefType.nBufferCountActual);
	INFO_PRINTF2(_L("PORTDEFINITIONTYPE nBufferCountMin: %X"), omxParamPortDefType.nBufferCountMin);
	INFO_PRINTF2(_L("PORTDEFINITIONTYPE nBufferSize: %X"), omxParamPortDefType.nBufferSize);
	INFO_PRINTF2(_L("PORTDEFINITIONTYPE eCompressionFormat: %X"), omxParamPortDefType.format.video.eCompressionFormat);
	INFO_PRINTF2(_L("PORTDEFINITIONTYPE eColorFormat: %X"), omxParamPortDefType.format.video.eColorFormat);
	INFO_PRINTF2(_L("PORTDEFINITIONTYPE xFramerate: %X"), omxParamPortDefType.format.video.xFramerate);

	omxParamPortDefType.format.video.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
	omxParamPortDefType.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
	
	// Attempt to set bad paramaters
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortDefType, OMX_ErrorUnsupportedSetting);
	
	// Obtain the new port def params for OMX_VIDEO_PARAM_PORTFORMATTYPE
	OMX_VIDEO_PARAM_PORTFORMATTYPE omxVideoParamPortForType;
	
	omxVideoParamPortForType.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	omxVideoParamPortForType.nVersion = TOmxILSpecVersion();
	omxVideoParamPortForType.nPortIndex = 0;
	omxVideoParamPortForType.nIndex = 0;
	omxVideoParamPortForType.eCompressionFormat = OMX_VIDEO_CodingUnused;
	omxVideoParamPortForType.eColorFormat = OMX_COLOR_FormatCrYCbY;
	omxVideoParamPortForType.xFramerate = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamPortForType);

	INFO_PRINTF2(_L("PORTFORMATTYPE nIndex: %X"), omxVideoParamPortForType.nIndex);
	INFO_PRINTF2(_L("PORTFORMATTYPE nPortIndex: %X"), omxVideoParamPortForType.nPortIndex);
	INFO_PRINTF2(_L("PORTFORMATTYPE nSize: %X"), omxVideoParamPortForType.nSize);
	INFO_PRINTF2(_L("PORTFORMATTYPE nVersion: %X"), omxVideoParamPortForType.nVersion);
	INFO_PRINTF2(_L("PORTFORMATTYPE eCompressionFormat: %X"), omxVideoParamPortForType.eCompressionFormat);
	INFO_PRINTF2(_L("PORTFORMATTYPE eColorFormat: %X"), omxVideoParamPortForType.eColorFormat);
	INFO_PRINTF2(_L("PORTFORMATTYPE xFramerate: %X"), omxVideoParamPortForType.xFramerate);
	
	omxParamPortDefType.format.video.eCompressionFormat = OMX_VIDEO_CodingAutoDetect;
	omxParamPortDefType.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
	
	 // Attempt to set bad paramaters
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamPortForType,OMX_ErrorUnsupportedSetting);
	
	// Obtain the new port def params for OMX_VIDEO_PARAM_PORTFORMATTYPE
	OMX_VIDEO_PARAM_PORTFORMATTYPE omxVideoParamPortForType2;
	
	omxVideoParamPortForType2.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	omxVideoParamPortForType2.nVersion = TOmxILSpecVersion();
	omxVideoParamPortForType2.nPortIndex = 0;
	omxVideoParamPortForType2.nIndex = 1;
	omxVideoParamPortForType2.eCompressionFormat = OMX_VIDEO_CodingUnused;
	omxVideoParamPortForType2.eColorFormat = OMX_COLOR_FormatCrYCbY;
	omxVideoParamPortForType2.xFramerate = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamPortForType2);

	INFO_PRINTF2(_L("PORTFORMATTYPE nIndex: %X"), omxVideoParamPortForType2.nIndex);
	INFO_PRINTF2(_L("PORTFORMATTYPE nPortIndex: %X"), omxVideoParamPortForType2.nPortIndex);
	INFO_PRINTF2(_L("PORTFORMATTYPE nSize: %X"), omxVideoParamPortForType2.nSize);
	INFO_PRINTF2(_L("PORTFORMATTYPE nVersion: %X"), omxVideoParamPortForType2.nVersion);
	INFO_PRINTF2(_L("PORTFORMATTYPE eCompressionFormat: %X"), omxVideoParamPortForType2.eCompressionFormat);
	INFO_PRINTF2(_L("PORTFORMATTYPE eColorFormat: %X"), omxVideoParamPortForType2.eColorFormat);
	INFO_PRINTF2(_L("PORTFORMATTYPE xFramerate: %X"), omxVideoParamPortForType2.xFramerate);
	
	omxVideoParamPortForType2.eCompressionFormat = OMX_VIDEO_CodingUnused;
	omxVideoParamPortForType2.eColorFormat = OMX_COLOR_Format12bitRGB444;
	
	// Attempt to set bad paramaters
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamPortForType2,OMX_ErrorUnsupportedSetting);
	
	OMX_INDEXTYPE videosurfaceconfigindex = OMX_IndexMax;
	const char tempconfig [] = "OMX.BRIAN.INDEX.PARAM.VIDEO.GFX.SURFACECONFIG";
	iError = iGraphicSinkCompHandle->GetExtensionIndex(iGraphicSinkCompHandle, const_cast<char*>(tempconfig), &videosurfaceconfigindex);
	if(OMX_ErrorUnsupportedIndex != iError)
		{
		INFO_PRINTF1(_L("OMX_GetExtensionIndex Failed"));
		SetTestStepError(PrintOmxError(iError));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateExecuting,0,OMX_ErrorNone);
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0001Step00::~COmxGsTest0001Step00()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step00::COmxGsTest0001Step00()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step00);
	}

TVerdict COmxGsTest0001Step00::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	TInt err;
	TVerdict result = EFail;

	TRAP(err, result = DoGSCompAllocTestL());

	if ((err != KErrNoMemory ))
		{
		INFO_PRINTF1(_L("Alloc testing completed successfully"));
		result = EPass;
		}

	SetTestStepError(err);
	SetTestStepResult(result);
	return TestStepResult();
	}

COmxGsTest0001Step02::~COmxGsTest0001Step02()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step02::COmxGsTest0001Step02()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step02);
	}


TVerdict COmxGsTest0001Step02::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);

	// Set Framerate and confirm OMX error
	// Declare the structs for Parameter tests
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortInput;
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortOutput; 
	OMX_VIDEO_PARAM_PORTFORMATTYPE omxVideoParamInput;
	OMX_VIDEO_PARAM_PORTFORMATTYPE omxVideoParamOutput;
	// framerate is always 0 for gfx component.
	const OMX_U32 setParam = 0;
	
	/*
	* Set the Framerate with the OMX_PARAM_PORTDEFINITIONTYPE struct
	* Then compares it against other structs to ensure Framerate is updated
	*/
	omxParamPortInput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortInput.nVersion = TOmxILSpecVersion();
	omxParamPortInput.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);
	
	// initial settings
	omxParamPortInput.format.video.nFrameWidth = 320;
    omxParamPortInput.format.video.nFrameHeight = 240;
    omxParamPortInput.format.video.nStride = 320*2;
    omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
    omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	
    // for test
    omxParamPortInput.format.video.xFramerate = 35;
	
	// Test Framerate in the OMX_PARAM_PORTDEFINITIONTYPE struct
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput);

	omxParamPortOutput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortOutput.nVersion = TOmxILSpecVersion();
	omxParamPortOutput.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
	CompareU32Param(setParam, omxParamPortOutput.format.video.xFramerate);
	
	// Test Framerate in the OMX_VIDEO_PARAM_PORTFORMATTYPE struct
	omxVideoParamOutput.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	omxVideoParamOutput.nVersion = TOmxILSpecVersion();
	omxVideoParamOutput.nPortIndex = 0;
	omxVideoParamOutput.nIndex = 0;
	omxVideoParamOutput.eCompressionFormat = OMX_VIDEO_CodingUnused;
	omxVideoParamOutput.eColorFormat = OMX_COLOR_FormatCrYCbY;
	omxVideoParamOutput.xFramerate = 0;
	// by using getparameter with OMX_VIDEO_PARAM_PORTFORMATTYPE, test client specifies all fields including framerate.
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamOutput,OMX_ErrorNone);
	CompareU32Param(setParam, omxVideoParamOutput.xFramerate);
	omxVideoParamOutput.xFramerate = 45;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamOutput,OMX_ErrorUnsupportedSetting);
	omxVideoParamOutput.xFramerate = 0;

	/*
	* Set the Framerate with the OMX_VIDEO_PARAM_PORTFORMATTYPE struct
	* Then compares it against other structs to ensure Framerate is updated
	*/
	omxVideoParamInput.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	omxVideoParamInput.nVersion = TOmxILSpecVersion();
	omxVideoParamInput.nPortIndex = 0;
	omxVideoParamInput.nIndex = 0;
	omxVideoParamInput.eCompressionFormat = OMX_VIDEO_CodingUnused;
	omxVideoParamInput.eColorFormat = OMX_COLOR_FormatCrYCbY;
	omxVideoParamInput.xFramerate = 0;
	// by using getparameter with OMX_VIDEO_PARAM_PORTFORMATTYPE, test client specifies all fields including framerate.
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorNone);
	omxVideoParamInput.xFramerate = 45;
	// Test Framerate in the OMX_VIDEO_PARAM_PORTFORMATTYPE struct
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorUnsupportedSetting);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamOutput,OMX_ErrorNone);
	CompareU32Param(setParam, omxVideoParamOutput.xFramerate);
	
	// Test Framerate in the OMX_PARAM_PORTDEFINITIONTYPE struct
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
	CompareU32Param(setParam, omxParamPortOutput.format.video.xFramerate);
	
	// Negative SetParamater test with OMX_SymbianIndexParamVideoGFXSurfaceConfig
	iError = OMX_SetParameter(iGraphicSinkCompHandle, iSurfaceConfigExt,&omxVideoParamInput);
	// TBD is OMX_ErrorUnsupportedIndex really the correct error code?
	if (OMX_ErrorUnsupportedIndex != iError)
		{
		SetTestStepError(PrintOmxError(iError));
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

COmxGsTest0001Step03::~COmxGsTest0001Step03()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step03::COmxGsTest0001Step03()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step03);
	}

TVerdict COmxGsTest0001Step03::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
		
	// Set Framesize and confirm
	// Declare the structs for Parameter tests
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortInput;
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortOutput;
	OMX_U32 setParamHeight;
	OMX_U32 setParamWidth;

	for (TInt i = 1; i <= 50; i++)
		{
		/*
		* Set the Framsize with the OMX_PARAM_PORTDEFINITIONTYPE struct
		* Then compares it against other structs to ensure Framerate is updated
		*/
		omxParamPortInput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
		omxParamPortInput.nVersion = TOmxILSpecVersion();
		omxParamPortInput.nPortIndex = 0;
		GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);
		
		omxParamPortInput.format.video.nFrameHeight = 10 * i;
		omxParamPortInput.format.video.nFrameWidth = 10 * i;
		
		omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
		TInt bytesPerPixel = COmxILMMBuffer::BytesPerPixel(omxParamPortInput.format.video.eColorFormat);
		TInt stride = bytesPerPixel * omxParamPortInput.format.video.nFrameWidth;
		omxParamPortInput.format.video.nStride = stride;
		
		setParamHeight = omxParamPortInput.format.video.nFrameHeight;
		setParamWidth = omxParamPortInput.format.video.nFrameWidth;
		// Test Framsize in the OMX_PARAM_PORTDEFINITIONTYPE struct
		
		// INFO_PRINTF4(_L("i: %d, width: %d, height: %d"),i, omxParamPortInput.format.video.nFrameWidth, omxParamPortInput.format.video.nFrameHeight);	
		
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);

		omxParamPortOutput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
		omxParamPortOutput.nVersion = TOmxILSpecVersion();
		omxParamPortOutput.nPortIndex = 0;
		GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
		CompareU32Param(setParamHeight, omxParamPortInput.format.video.nFrameHeight);
		CompareU32Param(setParamWidth, omxParamPortInput.format.video.nFrameWidth);
		CompareU32Param(setParamHeight, omxParamPortOutput.format.video.nFrameHeight);
		CompareU32Param(setParamWidth, omxParamPortOutput.format.video.nFrameWidth);
		}
	
	SetTestStepResult(EPass);
	
	return TestStepResult();
	}

COmxGsTest0001Step04::~COmxGsTest0001Step04()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step04::COmxGsTest0001Step04()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step04);
	}

TVerdict COmxGsTest0001Step04::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);

	// Set FrameFormat and confirm
	// Declare the structs for Parameter tests
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortInput;
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortOutput; 
	OMX_VIDEO_PARAM_PORTFORMATTYPE omxVideoParamInput;
	OMX_VIDEO_PARAM_PORTFORMATTYPE omxVideoParamOutput;
	OMX_COLOR_FORMATTYPE setColorFormat;
	
	/*
	* Set the Frameformat with the OMX_PARAM_PORTDEFINITIONTYPE struct
	* Then compares it against other structs to ensure Framerate is updated
	*/
	omxParamPortInput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortInput.nVersion = TOmxILSpecVersion();
	omxParamPortInput.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);
	
	   
    // initial settings
    omxParamPortInput.format.video.nFrameWidth = 320;
    omxParamPortInput.format.video.nFrameHeight = 240;
    omxParamPortInput.format.video.nStride = 320*2;
    omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
    omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    
	// Store default value should be OMX_COLOR_FormatCbYCrY
	setColorFormat = omxParamPortInput.format.video.eColorFormat;
	// Test Framerate in the OMX_PARAM_PORTDEFINITIONTYPE struct
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);
	
	omxParamPortOutput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortOutput.nVersion = TOmxILSpecVersion();
	omxParamPortOutput.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
	CompareCFTParam(setColorFormat, omxParamPortInput.format.video.eColorFormat,OMX_IndexParamPortDefinition);
	CompareCFTParam(setColorFormat, omxParamPortOutput.format.video.eColorFormat,OMX_IndexParamPortDefinition);
	
	// Test Framerate in the OMX_VIDEO_PARAM_PORTFORMATTYPE struct
	omxVideoParamOutput.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	omxVideoParamOutput.nVersion = TOmxILSpecVersion();
	omxVideoParamOutput.nPortIndex = 0;
	omxVideoParamOutput.nIndex = 0;
	// The following 2 values should be over written by the correct ones for this index
	omxVideoParamOutput.eCompressionFormat = OMX_VIDEO_CodingUnused;
	omxVideoParamOutput.eColorFormat = OMX_COLOR_FormatCrYCbY;
	omxVideoParamOutput.xFramerate = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamOutput,OMX_ErrorNone);
	CompareCFTParam(setColorFormat, omxVideoParamOutput.eColorFormat,OMX_IndexParamVideoPortFormat);
	
    // Negative GetParameter test for OMX_VIDEO_PARAM_PORTFORMATTYPE
    omxVideoParamOutput.nIndex = 10;
    GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamOutput,OMX_ErrorNoMore);
    omxVideoParamOutput.nIndex = 0;
    
	// Because we only have one valid color format, set color format to unused before next test
	omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatUnused;
	
	//should expect OMX_ErrorUnsupportedSetting when to try changing unsupported pixelformats.
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorUnsupportedSetting);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
	CompareCFTParam(setColorFormat, omxParamPortOutput.format.video.eColorFormat,OMX_IndexParamPortDefinition);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamOutput,OMX_ErrorNone);
	CompareCFTParam(setColorFormat, omxVideoParamOutput.eColorFormat,OMX_IndexParamVideoPortFormat);

	/*
	* Set the Frameformat with the OMX_VIDEO_PARAM_PORTFORMATTYPE struct
	* Then compares it against other structs to ensure Framerate is updated
	*/
	// by using getparameter with OMX_VIDEO_PARAM_PORTFORMATTYPE, test client specifies all fields including framerate.
	omxVideoParamInput.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
	omxVideoParamInput.nVersion = TOmxILSpecVersion();
	omxVideoParamInput.nPortIndex = 0;
	omxVideoParamInput.nIndex = 0;
	// The following 2 values should be over written by the correct ones for this index
	omxVideoParamInput.eCompressionFormat = OMX_VIDEO_CodingUnused;
	omxVideoParamInput.eColorFormat = OMX_COLOR_FormatCrYCbY;
	omxVideoParamInput.xFramerate = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorNone);
	setColorFormat = omxVideoParamInput.eColorFormat;
	
	// Test Framerate in the OMX_VIDEO_PARAM_PORTFORMATTYPE struct
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorNone);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamOutput,OMX_ErrorNone);
	CompareCFTParam(setColorFormat, omxVideoParamInput.eColorFormat,OMX_IndexParamVideoPortFormat);
	CompareCFTParam(setColorFormat, omxVideoParamOutput.eColorFormat,OMX_IndexParamVideoPortFormat);
	
	// Test Framerate in the OMX_PARAM_PORTDEFINITIONTYPE struct
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
	CompareCFTParam(setColorFormat, omxParamPortOutput.format.video.eColorFormat,OMX_IndexParamPortDefinition);
	// Because we only have one valid color format, set color format to unused before next test
	omxVideoParamInput.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
	omxVideoParamInput.eColorFormat = OMX_COLOR_FormatUnused;
	// Should expect OMX_ErrorUnsupportedSetting when to try changing unsupported pixelformats.
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorUnsupportedSetting);
	// The failed set paramater should not change the colour value in the component
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamOutput,OMX_ErrorNone);
	CompareCFTParam(setColorFormat, omxVideoParamOutput.eColorFormat,OMX_IndexParamVideoPortFormat);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
	CompareCFTParam(setColorFormat, omxParamPortOutput.format.video.eColorFormat,OMX_IndexParamPortDefinition);

	SetTestStepResult(EPass);
	return TestStepResult();
	}

COmxGsTest0001Step05::~COmxGsTest0001Step05()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step05::COmxGsTest0001Step05()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step05);
	}

TVerdict COmxGsTest0001Step05::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	
	// Declare the structs for Parameter tests
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortInput;
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortOutput; 
	OMX_U32 setParam;
	
	/*
	* Set the Frameformat with the OMX_PARAM_PORTDEFINITIONTYPE struct
	* Then compares it against other structs to ensure Framerate is updated
	*/
	omxParamPortInput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortInput.nVersion = TOmxILSpecVersion();
	omxParamPortInput.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);

	omxParamPortOutput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortOutput.nVersion = TOmxILSpecVersion();
	omxParamPortOutput.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
	
	// Confirm default values of buffer count
	if (omxParamPortInput.nBufferCountMin <= 0)
		{
		ERR_PRINTF2(_L("Default buffer count minimun is not set correctly: %d"),omxParamPortInput.nBufferCountMin);
		SetTestStepError(KErrGeneral);
		SetTestStepResult(EFail);
		return TestStepResult();
		}

	if (omxParamPortInput.nBufferCountActual == 0)
		{
		ERR_PRINTF1(_L("Default Buffer count min is set to 0"));
		SetTestStepError(KErrGeneral);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	if (omxParamPortInput.nBufferCountActual < omxParamPortInput.nBufferCountMin)
		{
		ERR_PRINTF2(_L("Default buffer count minimun is not set correctly: %d"),omxParamPortInput.nBufferCountMin);
		SetTestStepError(KErrGeneral);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
    
    // Set nBufferCountActual to below nBufferCountMin
	omxParamPortInput.nBufferCountActual = omxParamPortInput.nBufferCountMin - 1;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorBadParameter);
	
	for (TInt i = 0; i < 5; i++)
		{
		// INFO_PRINTF2(_L("COmxGsTest0001Step05::doTestStepL() Increase nBufferCountActual by %d"),i);
		omxParamPortInput.nBufferCountActual = omxParamPortInput.nBufferCountMin + i;
		setParam = omxParamPortInput.nBufferCountActual;
		
		 // initial settings
        omxParamPortInput.format.video.nFrameWidth = 320;
        omxParamPortInput.format.video.nFrameHeight = 240;
        omxParamPortInput.format.video.nStride = 320*2;
        omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
        omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
        
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);
	
		GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
		CompareU32Param(setParam, omxParamPortInput.nBufferCountActual);
		CompareU32Param(setParam, omxParamPortOutput.nBufferCountActual);
		}

	SetTestStepResult(EPass);
	return TestStepResult();
	}

COmxGsTest0001Step06::~COmxGsTest0001Step06()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step06::COmxGsTest0001Step06()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step06);
	}

TVerdict COmxGsTest0001Step06::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	
	// Declare the structs for Parameter tests
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortInput;
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortOutput;

	/*
	* OMX_PARAM_PORTDEFINITIONTYPE::nBufferAlignment
	*/
	omxParamPortInput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortInput.nVersion = TOmxILSpecVersion();
	omxParamPortInput.nPortIndex = 0;
	omxParamPortOutput.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
	omxParamPortOutput.nVersion = TOmxILSpecVersion();
	omxParamPortOutput.nPortIndex = 0;
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);

	// Set values using OMX_PARAM_PORTDEFINITIONTYPE and confirm stride
	// stride shouldn't be equal to zero. so i=1 .
	for (TInt i = 1; i < 50; i++)
		{
        // initial settings
        omxParamPortInput.format.video.nFrameHeight = 240;
        omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
        omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

        // test starts
		omxParamPortInput.format.video.nFrameWidth = 10 * i;
		TInt bytesPerPixel = COmxILMMBuffer::BytesPerPixel(omxParamPortInput.format.video.eColorFormat);
		TInt stride = bytesPerPixel * omxParamPortInput.format.video.nFrameWidth;
		// The code needs to calculate the stride
		omxParamPortInput.format.video.nStride = stride;

		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorNone);

		GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortOutput,OMX_ErrorNone);
 
		if (omxParamPortOutput.format.video.nStride < stride)
			{
			ERR_PRINTF2(_L("Stride value was not correct: %d"), omxParamPortOutput.format.video.nStride);
			SetTestStepError(KErrGeneral);
			SetTestStepResult(EFail);
			return TestStepResult();
			}
		}

	SetTestStepResult(EPass);
	return TestStepResult();
	}

COmxGsTest0001Step07::~COmxGsTest0001Step07()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step07::COmxGsTest0001Step07()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step07);
	}

TVerdict COmxGsTest0001Step07::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);

    // Declare the structs for Parameter tests
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortTypeInput;
	CreateOmxParamPortDefinitionType(&omxParamPortTypeInput);
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortDefinition;
	CreateOmxParamPortDefinitionType(&omxParamPortDefinition);
	
	// The following tests attempt to set the parameter of the graphics sink component.
	// In each case we call set, which is expected to pass through
	// the component and return OMX_ErrorNone.  After Get is called we perform a chack that the 
	// structs is updated.

	// aren't they all read-only fields?
	
	/*
	omxParamPortTypeInput.nBufferCountMin = omxParamPortTypeInput.nBufferCountMin - 1;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput);
	CompareU32Param(omxParamPortDefinition.nBufferCountMin, omxParamPortTypeInput.nBufferCountMin );
	
	// nBufferSize is read-only value
	omxParamPortTypeInput.nBufferSize = omxParamPortDefinition.nBufferSize + omxParamPortDefinition.nBufferSize;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput);
	CompareU32Param(omxParamPortDefinition.nBufferSize,omxParamPortTypeInput.nBufferSize);
	
	omxParamPortTypeInput.nBufferAlignment = 4;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	CompareU32Param(omxParamPortDefinition.nBufferAlignment,omxParamPortTypeInput.nBufferAlignment);
	
	// nSliceHeight is read-only value
	omxParamPortTypeInput.format.video.nSliceHeight = 20;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortDefinition,OMX_ErrorNone);
	CompareU32Param(omxParamPortDefinition.format.video.nSliceHeight,omxParamPortTypeInput.format.video.nSliceHeight);
	
	// bEnabled is read-only value
	omxParamPortTypeInput.bEnabled = OMX_FALSE;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	CompareBoolParam(omxParamPortTypeInput.bEnabled, omxParamPortDefinition.bEnabled,OMX_IndexParamPortDefinition);

	// bPopulated is read-only value
	omxParamPortTypeInput.bPopulated = OMX_TRUE;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	CompareBoolParam(omxParamPortTypeInput.bPopulated, omxParamPortDefinition.bPopulated,OMX_IndexParamPortDefinition);

	// bBuffersContiguous is read-only value
	omxParamPortTypeInput.bBuffersContiguous = OMX_FALSE;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	CompareBoolParam(omxParamPortTypeInput.bBuffersContiguous, omxParamPortDefinition.bBuffersContiguous,OMX_IndexParamPortDefinition);
	
	// eDomain is read-only value
	omxParamPortTypeInput.eDomain = OMX_PortDomainOther;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	GetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortTypeInput,OMX_ErrorNone);
	CompareU32Param(omxParamPortTypeInput.eDomain, omxParamPortDefinition.eDomain);
	*/
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

COmxGsTest0001Step08::~COmxGsTest0001Step08()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step08::COmxGsTest0001Step08()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step08);
	}

TVerdict COmxGsTest0001Step08::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	OMX_PARAM_PORTDEFINITIONTYPE omxParamPortInput;
	OMX_VIDEO_PARAM_PORTFORMATTYPE omxVideoParamInput;
	CreateOmxParamPortDefinitionType(&omxParamPortInput);
	CreateOmxVideoParamPortFormatType(&omxVideoParamInput);
	
	OMX_COLOR_FORMATTYPE defaultColorFormat = omxParamPortInput.format.video.eColorFormat;
	if (omxVideoParamInput.eColorFormat != omxParamPortInput.format.video.eColorFormat)
		{
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}
	
	// Traverse the array of colour format to confirm support.
	// The enumeration OMX_COLOR_FORMATTYPE starts at 1 and currently finishes at 43
	for (TInt i = 1; i < 44; i++)
		{
		//INFO_PRINTF2(_L("COmxGsTest0001Step08::doTestStepL(): Set OMX_COLOR_FORMATTYPE(%d)"),i);
		omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FORMATTYPE(i);
		omxVideoParamInput.eColorFormat = OMX_COLOR_FORMATTYPE(i);
		omxVideoParamInput.eCompressionFormat = OMX_VIDEO_CodingUnused;
		
        // initial settings
        omxParamPortInput.format.video.nFrameWidth = 320;
        omxParamPortInput.format.video.nFrameHeight = 240;
        omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

        
		TInt check = OMX_COLOR_FORMATTYPE(i);
		if(check == OMX_COLOR_Format16bitRGB565 ||			
			check == OMX_COLOR_FormatYCrYCb ||
			check == OMX_COLOR_FormatCbYCrY ||
			check == OMX_COLOR_Format32bitARGB8888)
			{
			TInt bytesPerPixel = COmxILMMBuffer::BytesPerPixel(omxParamPortInput.format.video.eColorFormat);
			TInt stride = bytesPerPixel * omxParamPortInput.format.video.nFrameWidth;
			omxParamPortInput.format.video.nStride = stride;
			
			SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput);
			SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput);
			}
		else
			{
			SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorUnsupportedSetting);
			SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorUnsupportedSetting);
			}
		
		omxParamPortInput.format.video.eColorFormat = defaultColorFormat;
		omxVideoParamInput.eColorFormat = defaultColorFormat;
		}
	
	// Set CompressionFormat and confirm
	OMX_VIDEO_CODINGTYPE defaultCompressionFormat = omxParamPortInput.format.video.eCompressionFormat;
	if (omxVideoParamInput.eCompressionFormat != omxParamPortInput.format.video.eCompressionFormat)
		{
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}

	for (TInt i = 1; i < 9; i++)
		{
		//INFO_PRINTF2(_L("COmxGsTest0001Step08::doTestStepL(): Set OMX_VIDEO_CODINGTYPE(%d)"),i);
		omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CODINGTYPE(i);
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorUnsupportedSetting);
		omxParamPortInput.format.video.eCompressionFormat = defaultCompressionFormat;
		
		omxVideoParamInput.eCompressionFormat = OMX_VIDEO_CODINGTYPE(i);
		SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorUnsupportedSetting);
		omxVideoParamInput.eCompressionFormat = defaultCompressionFormat;
		}
	
	//INFO_PRINTF1(_L("COmxGsTest0001Step08::doTestStepL(): Set ColorFormat and CompressionFormat"));
	for (TInt i = 0; i < 47; i++)
		{
		//INFO_PRINTF2(_L("COmxGsTest0001Step08::doTestStepL(): Set OMX_COLOR_FORMATTYPE(%d) using OMX_PARAM_PORTDEFINITIONTYPE"),i);
		omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FORMATTYPE(i);
		for (TInt j = 0; j < 9; j++)
			{
			//INFO_PRINTF2(_L("COmxGsTest0001Step08::doTestStepL(): Set OMX_VIDEO_CODINGTYPE(%d) using OMX_PARAM_PORTDEFINITIONTYPE"),j);
			omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CODINGTYPE(j);

			if ((i == OMX_COLOR_Format16bitRGB565 || i == OMX_COLOR_FormatYCrYCb || i == OMX_COLOR_FormatCbYCrY || i == OMX_COLOR_Format32bitARGB8888) && j == 0)
				{
				TInt bytesPerPixel = COmxILMMBuffer::BytesPerPixel(omxParamPortInput.format.video.eColorFormat);
				TInt stride = bytesPerPixel * omxParamPortInput.format.video.nFrameWidth;
				omxParamPortInput.format.video.nStride = stride;
				SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput);
				}
			else
				{
				if ((i == 0 && j != 0) || (i != 0 && j == 0)) // If OMX_COLOR_FORMATTYPE set to OMX_VIDEO_CodingUnused error is OMX_ErrorUnsupportedSetting
					{
					SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorUnsupportedSetting);
					}
				else
					{
					SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorUnsupportedSetting);
					}
				}
			}

		INFO_PRINTF2(_L("COmxGsTest0001Step08::doTestStepL(): Set OMX_COLOR_FORMATTYPE(%d) using OMX_VIDEO_PARAM_PORTFORMATTYPE"),i);
		omxVideoParamInput.eColorFormat = OMX_COLOR_FORMATTYPE(i);
		for (TInt j = 0; j < 9; j++)
			{
            //INFO_PRINTF2(_L("COmxGsTest0001Step08::doTestStepL(): Set OMX_VIDEO_CODINGTYPE(%d) using OMX_VIDEO_PARAM_PORTFORMATTYPE"),j);
			omxVideoParamInput.eCompressionFormat = OMX_VIDEO_CODINGTYPE(j);
			// OMX_COLOR_Format16bitRGB565,OMX_COLOR_FormatYCrYCb, OMX_COLOR_FormatCbYCrY
			if ((i == OMX_COLOR_Format16bitRGB565 || i == OMX_COLOR_FormatYCrYCb || i == OMX_COLOR_FormatCbYCrY || i == OMX_COLOR_Format32bitARGB8888) && j == 0)  
				{
				SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput);
				}
			else
				{
				if ((i == 0 && j != 0) || (i != 0 && j == 0)) // If OMX_COLOR_FORMATTYPE set to OMX_VIDEO_CodingUnused error is OMX_ErrorUnsupportedSetting
					{
					SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorUnsupportedSetting);
					}
				else
					{
					SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorUnsupportedSetting);
					}
				}
			}
		
		// Set values back to default
		omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
		omxVideoParamInput.eColorFormat = OMX_COLOR_FormatCbYCrY;
		omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
		omxVideoParamInput.eCompressionFormat = OMX_VIDEO_CodingUnused;
		}
	
	// Set color format and compression format to Unused
	omxParamPortInput.format.video.eColorFormat = OMX_COLOR_FormatUnused;
	omxVideoParamInput.eColorFormat = OMX_COLOR_FormatUnused;
	omxParamPortInput.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
	omxVideoParamInput.eCompressionFormat = OMX_VIDEO_CodingUnused;
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamPortDefinition,&omxParamPortInput,OMX_ErrorUnsupportedSetting);
	SetParameter(iGraphicSinkCompHandle,OMX_IndexParamVideoPortFormat,&omxVideoParamInput,OMX_ErrorUnsupportedSetting);

	SetTestStepResult(EPass);
	return TestStepResult();
	}

COmxGsTest0001Step09::~COmxGsTest0001Step09()
/**
 * Destructor
 */
	{
	}

COmxGsTest0001Step09::COmxGsTest0001Step09()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0001Step09);
	}

TVerdict COmxGsTest0001Step09::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	// COmxILMMBuffer::BytesPerPixel(OMX_COLOR_FORMATTYPE)
	for (TInt i = 0; i < 47; i++)
		{
		TInt bytesPerPixel = 15;
		bytesPerPixel =  COmxILMMBuffer::BytesPerPixel(OMX_COLOR_FORMATTYPE(i));
		if (bytesPerPixel == 15)
			{
			SetTestStepError(KErrGeneral);
			return TestStepResult();
			}
		}
	
	// COmxILMMBuffer::BytesPerPixel(TUidPixelFormat)
	for (TInt i = 0; i < 50; i++)
		{
		TInt bytesPerPixel = 15;
		bytesPerPixel =  COmxILMMBuffer::BytesPerPixel(TUidPixelFormat(i));
		if (bytesPerPixel == 15)
			{
			SetTestStepError(KErrGeneral);
			return TestStepResult();
			}
		}
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}


COmxGsTest0002Step01::~COmxGsTest0002Step01()
/**
 * Destructor
 */
	{
	}

COmxGsTest0002Step01::COmxGsTest0002Step01()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0002Step01);
	}

TVerdict COmxGsTest0002Step01::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	CGraphicsSinkTestBase::DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0002Step02::~COmxGsTest0002Step02()
/**
 * Destructor
 */
	{
	}

COmxGsTest0002Step02::COmxGsTest0002Step02()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0002Step02);
	}

TVerdict COmxGsTest0002Step02::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	CGraphicsSinkTestBase::DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
 	return TestStepResult();
	}

COmxGsTest0002Step03::~COmxGsTest0002Step03()
/**
 * Destructor
 */
	{
	}

COmxGsTest0002Step03::COmxGsTest0002Step03()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0002Step03);
	}

TVerdict COmxGsTest0002Step03::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0002Step04::~COmxGsTest0002Step04()
/**
 * Destructor
 */
	{
	}

COmxGsTest0002Step04::COmxGsTest0002Step04()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0002Step04);
	}

TVerdict COmxGsTest0002Step04::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	
	// Needed by Allocate buffer
	TInt bytesPerPixel = COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat);
	TInt bufferSize = bytesPerPixel * iOmxParamPortInput.format.video.nFrameWidth * iOmxParamPortInput.format.video.nFrameHeight;
	
	AllocateBuffer(iGraphicSinkCompHandle,&iInputBufferHeader,0,NULL,bufferSize,&iInputBufferHeaders,1,OMX_ErrorIncorrectStateOperation);
	iInputBufferHeaders.Reset();
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	SetTestStepResult(EPass);
 	return TestStepResult();
	}

COmxGsTest0003Step01::~COmxGsTest0003Step01()
/**
 * Destructor
 */
	{
	}

COmxGsTest0003Step01::COmxGsTest0003Step01()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0003Step01);
	}

TVerdict COmxGsTest0003Step01::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateCCameraBuf();
	UseBufferTask();
	if( iError == OMX_ErrorNone ){
	    StartTimer();
	}
	return TestStepResult();
	}

COmxGsTest0003Step02::~COmxGsTest0003Step02()
/**
 * Destructor
 */
	{
	}

COmxGsTest0003Step02::COmxGsTest0003Step02()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0003Step02);
	}

TVerdict COmxGsTest0003Step02::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateCCameraBuf();
	UseBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0003Step03::~COmxGsTest0003Step03()
/**
 * Destructor
 */
	{
	}

COmxGsTest0003Step03::COmxGsTest0003Step03()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0003Step03);
	}

TVerdict COmxGsTest0003Step03::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateCCameraBuf();
	UseBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0003Step04::~COmxGsTest0003Step04()
/**
 * Destructor
 */
	{
	}

COmxGsTest0003Step04::COmxGsTest0003Step04()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0003Step04);
	}


TVerdict COmxGsTest0003Step04::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();

	TInt bytesperpixel = COmxILMMBuffer::BytesPerPixel(iOmxParamPortInput.format.video.eColorFormat);
	// Not a true declaration, just used to test UseBuffer
	iTestChunk.CreateLocal(1024,1024);
	
	
	// Create local buffer then create iCamBuf from it. Used for coverage
	COmxILMMBuffer* cameraBuffer = NULL;
	TRAPD(err, cameraBuffer = COmxILMMBuffer::NewL(iTestChunk));
	if(err)
		{
		ERR_PRINTF1(_L("OOM ERROR"));
		SetTestStepError(err);
		return TestStepResult();
		}
	
	/*
    TRAP(err, iCamBuf = COmxILMMBuffer::CreateL(*cameraBuffer));
    delete cameraBuffer;
    if(err != KErrNone)
        {
        ERR_PRINTF1(_L("Failed to produce COmxILMMBuffer using COmxILMMBuffer::CreateL()"));
        SetTestStepError(err);
        return TestStepResult();
        }
    */
	
	
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
        configSharedChunkMetadata.nHandleId = iTestChunk.Handle();
        configSharedChunkMetadata.nOwnerThreadId = RThread().Id().Id();            
        
        (void) OMX_SetConfig(iGraphicSinkCompHandle,
                          sharedChunkMetadataExtensionIndex,
                          &configSharedChunkMetadata);
        }
    else
        {
        ERR_PRINTF1(_L("Failed to set shared chunk meta-data config."));
        SetTestStepError(err);
        return TestStepResult();
        }
	
	
	iPreviousState = OMX_StateLoaded;

	// UseBuffer with illegal pointer to the memory buffer area to be used
	iOmxErrorType = iGraphicSinkCompHandle->UseBuffer(iGraphicSinkCompHandle,&iOutputBufferHeader,0,cameraBuffer,cameraBuffer->Chunk().Size(),0);
	if (OMX_ErrorBadParameter != iOmxErrorType)
		{
		ERR_PRINTF1(_L("UseBuffer un-expectantly did not return an error"));
		SetTestStepError(PrintOmxError(iOmxErrorType));
		return TestStepResult();
		}
	
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	// UseBuffer with illegal buffer size in bytes
	iOmxErrorType = iGraphicSinkCompHandle->UseBuffer(iGraphicSinkCompHandle,
			//&iOutputBufferHeader,0,iCamBuf,0,iCamBuf->Chunk().Base());
	        &iOutputBufferHeader,0,NULL,0,iTestChunk.Base());
	if (OMX_ErrorBadParameter != iOmxErrorType)
		{
		ERR_PRINTF1(_L("UseBuffer un-expectantly did not return an error"));
		SetTestStepError(PrintOmxError(iOmxErrorType));
		return TestStepResult();
		}
	
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	// UseBuffer with illegal pointer to memory buffer and size
	iOmxErrorType = iGraphicSinkCompHandle->UseBuffer(iGraphicSinkCompHandle,
			//&iOutputBufferHeader,0,iCamBuf,0,0);
	        &iOutputBufferHeader,0,NULL,0,NULL);
	if (OMX_ErrorBadParameter != iOmxErrorType)
		{
		ERR_PRINTF1(_L("UseBuffer un-expectantly did not return an error"));
		SetTestStepError(PrintOmxError(iOmxErrorType));
		return TestStepResult();
		}
	
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	// UseBuffer without calling SendCommand
	iOmxErrorType = iGraphicSinkCompHandle->UseBuffer(iGraphicSinkCompHandle,
			//&iOutputBufferHeader,0,iCamBuf,iCamBuf->Chunk().Size(),iCamBuf->Chunk().Base());
	        &iOutputBufferHeader,0,NULL,iTestChunk.Size(),iTestChunk.Base());
	if (OMX_ErrorIncorrectStateOperation != iOmxErrorType)
		{
		ERR_PRINTF1(_L("UseBuffer un-expectantly did not return an error"));
		SetTestStepError(PrintOmxError(iOmxErrorType));
		return TestStepResult();
		}

	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	// UseBuffer with correct parameters
	SendCommand(iGraphicSinkCompHandle,OMX_CommandStateSet,OMX_StateIdle,0);
/*	
	// change made due to OMXILBufferClass
	iCamBuf->OffsetInfoArray().Append(0);

	iOmxErrorType = iGraphicSinkCompHandle->UseBuffer(iGraphicSinkCompHandle,&iInputBufferHeader,
			0,iCamBuf,iCamBuf->Chunk().Size(),iCamBuf->Chunk().Base() + iCamBuf->OffsetInfoArray()[0]);
	// Confirm UseBuffer returns OMX_ErrorBadParameter
	if (OMX_ErrorBadParameter != iOmxErrorType)
		{
		ERR_PRINTF1(_L("UseBuffer returned incorrect error"));
		SetTestStepError(PrintOmxError(iOmxErrorType));
		return TestStepResult();
		}
		
	// Confirm state unaltered
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
*/
	
	delete cameraBuffer;
	
	//iTestChunk.Close();
	SetTestStepResult(EPass);
	return TestStepResult();
	}

COmxGsTest0004Step01::~COmxGsTest0004Step01()
/**
 * Destructor
 */
	{
	}

COmxGsTest0004Step01::COmxGsTest0004Step01()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0004Step01);
	}

TVerdict COmxGsTest0004Step01::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0004Step02::~COmxGsTest0004Step02()
/**
 * Destructor
 */
	{
	}

COmxGsTest0004Step02::COmxGsTest0004Step02()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0004Step02);
	}


TVerdict COmxGsTest0004Step02::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0004Step03::~COmxGsTest0004Step03()
/**
 * Destructor
 */
	{
	}

COmxGsTest0004Step03::COmxGsTest0004Step03()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0004Step03);
	}

TVerdict COmxGsTest0004Step03::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0004Step04::~COmxGsTest0004Step04()
/**
 * Destructor
 */
	{
	}

COmxGsTest0004Step04::COmxGsTest0004Step04()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0004Step04);
	}

TVerdict COmxGsTest0004Step04::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0004Step05::~COmxGsTest0004Step05()
/**
 * Destructor
 */
	{
	}

COmxGsTest0004Step05::COmxGsTest0004Step05()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0004Step05);
	}

TVerdict COmxGsTest0004Step05::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}


COmxGsTest0004Step06::~COmxGsTest0004Step06()
/**
 * Destructor
 */
	{
	}

COmxGsTest0004Step06::COmxGsTest0004Step06()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0004Step06);
	}

TVerdict COmxGsTest0004Step06::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0005Step01::~COmxGsTest0005Step01()
/**
 * Destructor
 */
	{
	}

COmxGsTest0005Step01::COmxGsTest0005Step01()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0005Step01);
	}

TVerdict COmxGsTest0005Step01::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	iDoEmptyBufferDoneLimit = 10;
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0005Step02::~COmxGsTest0005Step02()
/**
 * Destructor
 */
	{
	}

COmxGsTest0005Step02::COmxGsTest0005Step02()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0005Step02);
	}

TVerdict COmxGsTest0005Step02::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	iDoEmptyBufferDoneLimit = 10;
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0005Step03::~COmxGsTest0005Step03()
/**
 * Destructor
 */
	{
	}

COmxGsTest0005Step03::COmxGsTest0005Step03()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0005Step03);
	}

TVerdict COmxGsTest0005Step03::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	iDoEmptyBufferDoneLimit = 10;
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}


COmxGsTest0005Step04::~COmxGsTest0005Step04()
/**
 * Destructor
 */
	{
	}

COmxGsTest0005Step04::COmxGsTest0005Step04()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0005Step04);
	}

TVerdict COmxGsTest0005Step04::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	iDoEmptyBufferDoneLimit = 10;
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0005Step05::~COmxGsTest0005Step05()
/**
 * Destructor
 */
	{
	}

COmxGsTest0005Step05::COmxGsTest0005Step05()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0005Step05);
	}

TVerdict COmxGsTest0005Step05::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	iDoEmptyBufferDoneLimit = 10;
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0006Step01::~COmxGsTest0006Step01()
/**
 * Destructor
 */
	{
	}

COmxGsTest0006Step01::COmxGsTest0006Step01()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0006Step01);
	}

TVerdict COmxGsTest0006Step01::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0006Step02::~COmxGsTest0006Step02()
/**
 * Destructor
 */
	{
	}

COmxGsTest0006Step02::COmxGsTest0006Step02()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0006Step02);
	}

TVerdict COmxGsTest0006Step02::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0007Step01::~COmxGsTest0007Step01()
/**
 * Destructor
 */
	{
	}

COmxGsTest0007Step01::COmxGsTest0007Step01()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0007Step01);
	}


TVerdict COmxGsTest0007Step01::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	iDoEmptyBufferDoneLimit = 8;
	iExecutingToIdle = EFalse;
	
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0007Step02::~COmxGsTest0007Step02()
/**
 * Destructor
 */
	{
	}

COmxGsTest0007Step02::COmxGsTest0007Step02()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0007Step02);
	}

TVerdict COmxGsTest0007Step02::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	iDoEmptyBufferDoneLimit = 8;
	iExecutingToIdle = EFalse;
	
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}


COmxGsTest0007Step03::~COmxGsTest0007Step03()
/**
 * Destructor
 */
	{
	}

COmxGsTest0007Step03::COmxGsTest0007Step03()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0007Step03);
	}

TVerdict COmxGsTest0007Step03::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	iDoEmptyBufferDoneLimit = 8;
	iExecutingToIdle = EFalse;
	
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0007Step04::~COmxGsTest0007Step04()
/**
 * Destructor
 */
	{
	}

COmxGsTest0007Step04::COmxGsTest0007Step04()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0007Step04);
	}

TVerdict COmxGsTest0007Step04::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	GetState(iGraphicSinkCompHandle,&iOmxStateType,OMX_StateLoaded);
	iDoEmptyBufferDoneLimit = 8;
	iExecutingToIdle = EFalse;
	
	DoROmxGsTestSetup();
	AllocateBufferTask();
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0008Step01::~COmxGsTest0008Step01()
/**
 * Destructor
 */
	{
	}

COmxGsTest0008Step01::COmxGsTest0008Step01()
/**
 * Constructor
 */
	{
	SetTestStepName(KOmxGsTest0008Step01);
	}

TVerdict COmxGsTest0008Step01::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Our implementation only gets called if the base class doTestStepPreambleL() did
 * not leave. That being the case, the current test result value will be EPass.
 */
	{
	// Use set and get config for all different relevant dynamic configurations including negative tests
	OMX_FRAMESIZETYPE setFrameSize;
	OMX_CONFIG_SCALEFACTORTYPE setScaleFactor;
	OMX_CONFIG_RECTTYPE setRecType;
	
	OMX_FRAMESIZETYPE getFrameSize;
	OMX_CONFIG_SCALEFACTORTYPE getScaleFactor;
	OMX_CONFIG_RECTTYPE getRecType;
	
		
	//Testing OMX_IndexConfigCommonScale
	setScaleFactor.nSize = sizeof(OMX_CONFIG_SCALEFACTORTYPE);
	setScaleFactor.nVersion = TOmxILSpecVersion();
	setScaleFactor.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&setScaleFactor);
	setScaleFactor.xWidth = 0x10000;
	setScaleFactor.xHeight = 0x10000;
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&setScaleFactor);
	getScaleFactor.nSize = sizeof(OMX_CONFIG_SCALEFACTORTYPE);
	getScaleFactor.nVersion = TOmxILSpecVersion();
	getScaleFactor.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&getScaleFactor);
	if(setScaleFactor.xWidth != getScaleFactor.xWidth || setScaleFactor.xHeight != getScaleFactor.xHeight)
		{
		ERR_PRINTF1(_L("COmxGsTest0008Step01::doTestStepL(): OMX_IndexConfigCommonScale 1 error"));	
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}
		
	//Code coverage negative testing
	setScaleFactor.nPortIndex = 1;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&setScaleFactor,OMX_ErrorBadPortIndex);
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&setScaleFactor,OMX_ErrorBadPortIndex);
	
		
	//Testing OMX_IndexConfigCommonScale
	setScaleFactor.nSize = sizeof(OMX_CONFIG_SCALEFACTORTYPE);
	setScaleFactor.nVersion = TOmxILSpecVersion();
	setScaleFactor.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&setScaleFactor);
	setScaleFactor.xWidth = 0x20000;
	setScaleFactor.xHeight = 0x20000;
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&setScaleFactor);
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&setScaleFactor);
	getScaleFactor.nSize = sizeof(OMX_CONFIG_SCALEFACTORTYPE);
	getScaleFactor.nVersion = TOmxILSpecVersion();
	getScaleFactor.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonScale,&getScaleFactor);
	if(setScaleFactor.xWidth != getScaleFactor.xWidth || setScaleFactor.xHeight != getScaleFactor.xHeight)
		{
		ERR_PRINTF1(_L("COmxGsTest0008Step01::doTestStepL(): OMX_IndexConfigCommonScale 2 error"));	
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}		
		
	//Testing OMX_IndexConfigCommonOutputSize
	setFrameSize.nSize = sizeof(OMX_FRAMESIZETYPE);
	setFrameSize.nVersion = TOmxILSpecVersion();
	setFrameSize.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputSize,&setFrameSize);
	setFrameSize.nWidth = 320;
	setFrameSize.nHeight = 240;
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputSize,&setFrameSize);
	getFrameSize.nSize = sizeof(OMX_FRAMESIZETYPE);
	getFrameSize.nVersion = TOmxILSpecVersion();
	getFrameSize.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputSize,&getFrameSize);
	if(setFrameSize.nWidth != getFrameSize.nWidth || setFrameSize.nHeight != getFrameSize.nHeight)
		{
		ERR_PRINTF1(_L("COmxGsTest0008Step01::doTestStepL(): OMX_IndexConfigCommonOutputSize 1 error"));	
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}
		
	//Code coverage negative testing
	setFrameSize.nPortIndex = 1;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputSize,&setFrameSize,OMX_ErrorBadPortIndex);
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputSize,&setFrameSize,OMX_ErrorBadPortIndex);
	
		
	//Testing OMX_IndexConfigCommonOutputSize
	setFrameSize.nSize = sizeof(OMX_FRAMESIZETYPE);
	setFrameSize.nVersion = TOmxILSpecVersion();
	setFrameSize.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputSize,&setFrameSize);
	setFrameSize.nWidth = 160;
	setFrameSize.nHeight = 120;
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputSize,&setFrameSize);
	getFrameSize.nSize = sizeof(OMX_FRAMESIZETYPE);
	getFrameSize.nVersion = TOmxILSpecVersion();
	getFrameSize.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputSize,&getFrameSize);
	if(setFrameSize.nWidth != getFrameSize.nWidth || setFrameSize.nHeight != getFrameSize.nHeight)
		{
		ERR_PRINTF1(_L("COmxGsTest0008Step01::doTestStepL(): OMX_IndexConfigCommonOutputSize 2 error"));	
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}	
	
	//Testing OMX_IndexConfigCommonInputCrop
	setRecType.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	setRecType.nVersion = TOmxILSpecVersion();
	setRecType.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonInputCrop,&setRecType);
	setRecType.nLeft = 0x10000;
	setRecType.nTop = 0x10000;
	setRecType.nWidth = 320;
	setRecType.nHeight = 240;
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonInputCrop,&setRecType);
	getRecType.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	getRecType.nVersion = TOmxILSpecVersion();
	getRecType.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonInputCrop,&getRecType);
	if( setRecType.nLeft != getRecType.nLeft 
		|| setRecType.nTop != getRecType.nTop 
		|| setRecType.nWidth != getRecType.nWidth
		|| setRecType.nHeight != getRecType.nHeight )
		{
		ERR_PRINTF1(_L("COmxGsTest0008Step01::doTestStepL(): OMX_IndexConfigCommonInputCrop 1 error"));	
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}
	
	//Testing OMX_IndexConfigCommonOutputCrop
	setRecType.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	setRecType.nVersion = TOmxILSpecVersion();
	setRecType.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputCrop,&setRecType);
	setRecType.nLeft = 0x20000;
	setRecType.nTop = 0x20000;
	setRecType.nWidth = 160;
	setRecType.nHeight = 120;
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputCrop,&setRecType);
	getRecType.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	getRecType.nVersion = TOmxILSpecVersion();
	getRecType.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonOutputCrop,&getRecType);
	if( setRecType.nLeft != getRecType.nLeft 
		|| setRecType.nTop != getRecType.nTop 
		|| setRecType.nWidth != getRecType.nWidth
		|| setRecType.nHeight != getRecType.nHeight )
		{
		ERR_PRINTF1(_L("COmxGsTest0008Step01::doTestStepL(): OMX_IndexConfigCommonOutputCrop 1 error"));	
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}
		
	//Testing OMX_IndexConfigCommonExclusionRect
	setRecType.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	setRecType.nVersion = TOmxILSpecVersion();
	setRecType.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonExclusionRect,&setRecType);
	setRecType.nLeft = 0x20000;
	setRecType.nTop = 0x20000;
	setRecType.nWidth = 320;
	setRecType.nHeight = 240;
	SetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonExclusionRect,&setRecType);
	getRecType.nSize = sizeof(OMX_CONFIG_RECTTYPE);
	getRecType.nVersion = TOmxILSpecVersion();
	getRecType.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonExclusionRect,&getRecType);
	if( setRecType.nLeft != getRecType.nLeft 
		|| setRecType.nTop != getRecType.nTop 
		|| setRecType.nWidth != getRecType.nWidth
		|| setRecType.nHeight != getRecType.nHeight )
		{
		ERR_PRINTF1(_L("COmxGsTest0008Step01::doTestStepL(): OMX_IndexConfigCommonExclusionRect 1 error"));	
		SetTestStepError(KErrGeneral);
		return TestStepResult();
		}
	
	//Negative GetConfig test
	OMX_CONFIG_CONTRASTTYPE conContrastType;
	conContrastType.nSize = sizeof(OMX_CONFIG_CONTRASTTYPE);
	conContrastType.nVersion = TOmxILSpecVersion();
	conContrastType.nPortIndex = 0;
	GetConfig(iGraphicSinkCompHandle,OMX_IndexConfigCommonContrast,&conContrastType,OMX_ErrorUnsupportedIndex);
	
	SetTestStepResult(EPass);
	return TestStepResult();
	}

COmxGsTest0009Step00::~COmxGsTest0009Step00()
	{
	}
 
COmxGsTest0009Step00::COmxGsTest0009Step00()
	{
	SetTestStepName(KOmxGsTest0009Step00);
	}

TVerdict COmxGsTest0009Step00::doTestStepL()
	{
	GetIntFromConfig(ConfigSection(),KMMTestCase,iTestCase);
	GetIntFromConfig(ConfigSection(),KMMTestStep,iTestStep);
	INFO_PRINTF3(_L("COmxGsTest000%dStep0%d::doTestStepL: State Loaded to Executing transition using AllocateBuffer"),iTestCase,iTestStep);
	DoROmxGsTestSetup();
	StartStateTransitionTask();
	
	StartTimer();
	return TestStepResult();
	}

COmxGsTest0010Step00::~COmxGsTest0010Step00()
	{
	}
 
COmxGsTest0010Step00::COmxGsTest0010Step00()
	{
	SetTestStepName(KOmxGsTest0010Step00);
	}

TVerdict COmxGsTest0010Step00::doTestStepL()
	{
	GetIntFromConfig(ConfigSection(),KMMTestCase,iTestCase);
	GetIntFromConfig(ConfigSection(),KMMTestStep,iTestStep);
	GetIntFromConfig(ConfigSection(),_L("REmptyBufferLimit"),iDoEmptyBufferDoneLimit);
	INFO_PRINTF3(_L("COmxGsTest000%dStep0%d::doTestStepL: Empty Buffer Done using AllocateBuffer"),iTestCase,iTestStep);
	DoROmxGsTestSetup();
	StartBufferDoneTask();
	
	StartTimer();
	return TestStepResult();
	}
