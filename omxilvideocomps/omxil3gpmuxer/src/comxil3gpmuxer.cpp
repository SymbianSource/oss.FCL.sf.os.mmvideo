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

#include <openmax/il/loader/omxilsymbiancomponentif.h>

#include "omxil3gpmuxer.hrh"
#include "comxil3gpmuxer.h"
#include "comxil3gpmuxerprocessingfunction.h"
#include "comxil3gpmuxerconfigmanager.h"
#include "comxil3gpmuxervideoinputport.h"
#include "comxil3gpmuxeraudioinputport.h"
#include "log.h"

_LIT8(KSymbianOmxIL3gpMuxerName, "OMX.SYMBIAN.OTHER.CONTAINER_MUXER.3GP");
_LIT8(KSymbianOmxIL3gpMuxerRole, "container_muxer");

static const TInt KMinAudioBufferCount = 1;
// at least 2 video buffers must be used since the difference in two buffer timestamps
// is used to calculate the frame duration passed to 3gplibrary
static const TInt KMinVideoBufferCount = 2;

OMXIL_COMPONENT_ECOM_ENTRYPOINT(KUidSymbianOmxIL3gpMuxer);

OMX_ERRORTYPE SymbianErrorToOmx(TInt aError);

// Component Entry Point
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE aComponent)
	{
	TInt err = COmxIL3GPMuxer::CreateComponent(aComponent);
	return SymbianErrorToOmx(err);
	}

TInt COmxIL3GPMuxer::CreateComponent(OMX_HANDLETYPE hComponent)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxer::CreateComponent"));
	COmxIL3GPMuxer* self = new COmxIL3GPMuxer();

	if (!self)
		{
		return KErrNoMemory;
		}

	TRAPD(err, self->ConstructL(hComponent));
	if(err)
		{
		delete self;
		}
	return err;
	}

COmxIL3GPMuxer::COmxIL3GPMuxer()
	{
	}

void COmxIL3GPMuxer::ConstructL(OMX_HANDLETYPE hComponent)
	{
	// STEP 1: Initialize the data received from the IL Core
    COmxILComponent::ConstructL(hComponent);
    
	// STEP 2: Create the call backs holder...
    MOmxILCallbackNotificationIf* callbackNotificationIf=CreateCallbackManagerL(COmxILComponent::EOutofContext);

	// STEP 3: Create the 3gpmuxer-specific Processing Function...
    COmxIL3GPMuxerProcessingFunction* pProcessingFunction = COmxIL3GPMuxerProcessingFunction::NewL(*callbackNotificationIf);
    RegisterProcessingFunction(pProcessingFunction);
    
	// STEP 4: Create Port manager...
    CreatePortManagerL(COmxILComponent::ENonBufferSharingPortManager,
		TOmxILSpecVersion(),	        // OMX Version
		1,						// The number of audio ports in this component
		0,						// The starting audio port index
		0,						// The number of image ports in this component
		0,						// The starting image port index
		1,						// The number of video ports in this component
		1,						// The starting video port index
		0,						// The number of other ports in this component
		0						// The starting other port index
		);

	// STEP 5: Create the non-port related configuration manager...
	RPointerArray<TDesC8> roleList;
	CleanupClosePushL(roleList);
	roleList.AppendL(&KSymbianOmxIL3gpMuxerRole);
	COmxIL3GPMuxerConfigManager* configManager = COmxIL3GPMuxerConfigManager::NewL(KSymbianOmxIL3gpMuxerName, TOmxILSpecVersion(), roleList);
	RegisterConfigurationManager(configManager);
	CleanupStack::PopAndDestroy();

	static_cast<COmxIL3GPMuxerProcessingFunction*>(pProcessingFunction)->SetConfigManager(*configManager);
	
	// create the input port and add it to the port manager...
	AddAudioInputPortL();
	AddVideoInputPortL();

	// And finally, let's get everything started
	InitComponentL();
	}

COmxIL3GPMuxer::~COmxIL3GPMuxer()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxer::~COmxIL3GPMuxer"));
	}

void COmxIL3GPMuxer::AddVideoInputPortL()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxer::AddVideoInputPortL"));
	TOmxILSpecVersion omxVersion;
	TOmxILCommonPortData portData(
			omxVersion, 
			EPortIndexVideoInput,
			OMX_DirInput,
			KMinVideoBufferCount,
			20480,												// minimum buffer size, in bytes, TODO autodetect this
			OMX_PortDomainVideo,
			OMX_TRUE,
			4,													// 4-byte alignment
			OMX_BufferSupplyUnspecified,
			COmxILPort::KBufferMarkPropagationPortNotNeeded
			);
	
	COmxIL3GPMuxerVideoInputPort* videoInputPort = COmxIL3GPMuxerVideoInputPort::NewL(portData);
	CleanupStack::PushL(videoInputPort);
	User::LeaveIfError(AddPort(videoInputPort, OMX_DirInput));
	CleanupStack::Pop();
	static_cast<COmxIL3GPMuxerProcessingFunction*>(GetProcessingFunction())->SetVideoPort(*videoInputPort);
	}

void COmxIL3GPMuxer::AddAudioInputPortL()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxer::AddAudioInputPortL"));
	TOmxILSpecVersion omxVersion;
	TOmxILCommonPortData portData(
			omxVersion, 
			EPortIndexAudioInput,
			OMX_DirInput,
			KMinAudioBufferCount,
			20480,												// minimum buffer size, in bytes, TODO autodetect this
			OMX_PortDomainAudio,
			OMX_TRUE,
			4,													// 4-byte alignment
			OMX_BufferSupplyUnspecified,
			COmxILPort::KBufferMarkPropagationPortNotNeeded
			);
	
	COmxIL3GPMuxerAudioInputPort* audioInputPort = COmxIL3GPMuxerAudioInputPort::NewL(portData);
	CleanupStack::PushL(audioInputPort);
	User::LeaveIfError(AddPort(audioInputPort, OMX_DirInput));
	CleanupStack::Pop();
	static_cast<COmxIL3GPMuxerProcessingFunction*>(GetProcessingFunction())->SetAudioPort(*audioInputPort);
	}

OMX_ERRORTYPE SymbianErrorToOmx(TInt aError)
	{
	switch(aError)
		{
	case KErrNone:
		return OMX_ErrorNone;
	case KErrNoMemory:
		return OMX_ErrorInsufficientResources;
	default:
		return OMX_ErrorUndefined;
		}
	}
