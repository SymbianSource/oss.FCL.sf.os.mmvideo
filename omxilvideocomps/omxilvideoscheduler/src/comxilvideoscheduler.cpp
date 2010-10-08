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

#include <openmax/il/common/omxilspecversion.h>
#include <openmax/il/common/omxilconfigmanager.h>
#include <openmax/il/loader/omxilcomponentif.h>
#include <openmax/il/loader/omxilsymbiancomponentif.h>
#include <openmax/il/common/omxilclientclockport.h>

#include "comxilvideoscheduler.h"
#include "comxilvideoschedulerinputport.h"
#include "comxilvideoscheduleroutputport.h"
#include "comxilvideoschedulerpf.h"
#include "omxilvideoscheduler.hrh"

const OMX_U32 KInputPortIndex = 0;
const OMX_U32 KOutputPortIndex = 1;
const OMX_U32 KClockPortIndex = 2;

_LIT8(KSymbianOmxILVideoSchedulerName, "OMX.SYMBIAN.VIDEO.VIDEOSCHEDULER");
_LIT8(KSymbianOmxILVideoSchedulerRole, "video_scheduler.binary");	

OMXIL_COMPONENT_ECOM_ENTRYPOINT(KUidSymbianOmxILVideoScheduler);

OMX_ERRORTYPE SymbianErrorToOmx(TInt aError);

// Component Entry Point
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE aComponent)
	{
	TInt err = COmxILVideoScheduler::CreateComponent(aComponent);
	return SymbianErrorToOmx(err);
	}

TInt COmxILVideoScheduler::CreateComponent(OMX_HANDLETYPE aComponent)
	{
	COmxILVideoScheduler* self = new COmxILVideoScheduler();

	if (!self)
		{
		return KErrNoMemory;
		}

	TRAPD(err, self->ConstructL(aComponent));
	if(err)
		{
		delete self;
		}
	return err;
	}

COmxILVideoScheduler::COmxILVideoScheduler()
	{
	// nothing to do
	}

void COmxILVideoScheduler::ConstructL(OMX_HANDLETYPE hComponent)
	{
	// STEP 1: Initialize the data received from the IL Core
    COmxILComponent::ConstructL(hComponent);
    
	// STEP 2: Create the callback manager
	// In context callback manager is used since we need output buffers to be received at the sink
	// component immediately after we decide to send them.
	// Also, returning clock buffers imediately avoids clock buffer starvation 
    MOmxILCallbackNotificationIf* callbackNotificationIf = CreateCallbackManagerL(COmxILComponent::EOutofContext);
 
    
	// STEP 3: Create the Processing Function...
    COmxILVideoSchedulerPF* pProcessingFunction = COmxILVideoSchedulerPF::NewL(*callbackNotificationIf, *this, GetHandle());
	RegisterProcessingFunction(pProcessingFunction);
	
	// STEP 4: Create Port manager...
	CreatePortManagerL(COmxILComponent::ENonBufferSharingPortManager,
		TOmxILSpecVersion(),	        // OMX Version
		0,						// The number of audio ports in this component
		0,						// The starting audio port index
		0,						// The number of image ports in this component
		0,						// The starting image port index
		2,						// The number of video ports in this component
		0,						// The starting video port index
		1,						// The number of other ports in this component
		2						// The starting other port index
		);

	// create the input port
	AddInputVideoPortL();
	AddOutputVideoPortL();
	AddClockPortL();
	
	// STEP 5: Create the non-port related configuration manager...
	RPointerArray<TDesC8> roleList;
	CleanupClosePushL(roleList);
	roleList.AppendL(&KSymbianOmxILVideoSchedulerRole);
	COmxILConfigManager* pConfigManager = COmxILConfigManager::NewL(
			KSymbianOmxILVideoSchedulerName,
			TOmxILSpecVersion(),
			roleList);
	CleanupStack::PopAndDestroy();
	RegisterConfigurationManager(pConfigManager);
	
	// And finally, let's get everything started
	InitComponentL();
	}

COmxILVideoScheduler::~COmxILVideoScheduler()
	{
	}

void COmxILVideoScheduler::AddInputVideoPortL()
	{
	TOmxILSpecVersion specVersion;

	TOmxILCommonPortData portData(
			specVersion,
			KInputPortIndex, 
			OMX_DirInput, 
			1,											// minimum number of buffers
			1,											// minimum buffer size, in bytes
			OMX_PortDomainVideo,
			OMX_FALSE,					// do not need contigious buffers
			1,							// 1-byte alignment
			OMX_BufferSupplyInput,
			KOutputPortIndex);

	iVideoInputPort = COmxILVideoSchedulerInputPort::NewL(portData);
	CleanupStack::PushL(iVideoInputPort);
	User::LeaveIfError(AddPort(iVideoInputPort, OMX_DirInput));
	CleanupStack::Pop();
	}
	
void COmxILVideoScheduler::AddOutputVideoPortL()
	{
	TOmxILSpecVersion specVersion;

	TOmxILCommonPortData portData(
			specVersion,
			KOutputPortIndex, 
			OMX_DirOutput, 
			1,											// minimum number of buffers
			1,											// minimum buffer size, in bytes
			OMX_PortDomainVideo,
			OMX_FALSE,					// do not need contigious buffers
			1,							// 1-byte alignment
			OMX_BufferSupplyInput,
			COmxILPort::KBufferMarkPropagationPortNotNeeded);
			
	iVideoOutputPort = COmxILVideoSchedulerOutputPort::NewL(portData);
	CleanupStack::PushL(iVideoOutputPort);
	User::LeaveIfError(AddPort(iVideoOutputPort, OMX_DirOutput));
	CleanupStack::Pop();
	}	
	
void COmxILVideoScheduler::AddClockPortL()
	{
	TOmxILSpecVersion specVersion;

	TOmxILCommonPortData portData(
			specVersion,
			KClockPortIndex, 
			OMX_DirInput, 
			4,											// minimum number of buffers
			sizeof(OMX_TIME_MEDIATIMETYPE),				// minimum buffer size, in bytes
			OMX_PortDomainOther,
			OMX_TRUE,									// contigious buffers
			4,											// 4-byte alignment
			OMX_BufferSupplyOutput,
			COmxILPort::KBufferMarkPropagationPortNotNeeded);

	RArray<OMX_OTHER_FORMATTYPE> array;
	CleanupClosePushL(array);
	array.AppendL(OMX_OTHER_FormatTime);
	iClockPort = COmxILClientClockPort::NewL(portData, array);
	CleanupStack::PopAndDestroy(&array);
	CleanupStack::PushL(iClockPort);
	User::LeaveIfError(AddPort(iClockPort, OMX_DirInput));
	CleanupStack::Pop();
	}

/** Returns the maximum number of buffers configured on a port. */
TUint32 COmxILVideoScheduler::BufferCount() const
	{
	// due to buffer copying we do not need the same number of buffers on each port,
	// so to be safe the maximum count is used when allocating queues.
	// when buffer sharing is added, the buffer counts must be checked to be the same on
	// the Loaded->Idle phase.
	TUint32 in = iVideoInputPort->BufferCount();
	TUint32 out = iVideoOutputPort->BufferCount();
	return in > out ? in : out;
	}

OMX_ERRORTYPE COmxILVideoScheduler::MediaTimeRequest(TAny* apPrivate, OMX_TICKS aMediaTime, OMX_TICKS aOffset)
	{
	return iClockPort->MediaTimeRequest(apPrivate, aMediaTime, aOffset);
	}		

OMX_ERRORTYPE COmxILVideoScheduler::GetWallTime(OMX_TICKS& aWallTime)
	{
	return iClockPort->GetWallTime(aWallTime);
	}			

OMX_ERRORTYPE COmxILVideoScheduler::SetVideoStartTime(OMX_TICKS aStartTime)
	{
	return iClockPort->SetStartTime(aStartTime);
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
