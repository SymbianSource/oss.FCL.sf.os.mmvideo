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
 @file
 @internalComponent
*/
 
#include <openmax/il/common/omxilconfigmanager.h>
#include <openmax/il/common/omxilspecversion.h>

#include "omxilgraphicsink.h"
#include "omxilgraphicsinkvpb0port.h"
#include "omxilgraphicsinkprocessingfunction.h"
#include "omxilgraphicsink.hrh"
#include <openmax/il/loader/omxilsymbiancomponentif.h>
#include "log.h"

_LIT8(KSymbianOMXGraphicSinkComponentName, KCompNameSymbianOMXGraphicSink);
_LIT8(KSymbianOMXGraphicSinkRole, KRoleSymbianOMXGraphicSink);

const TUint8 COmxILGraphicSink::iComponentVersionMajor;
const TUint8 COmxILGraphicSink::iComponentVersionMinor;
const TUint8 COmxILGraphicSink::iComponentVersionRevision;
const TUint8 COmxILGraphicSink::iComponentVersionStep;

OMXIL_COMPONENT_ECOM_ENTRYPOINT(KUidSymbianOmxILGraphicSink);

/**
Component Entry Point
@param aComponent The handle of the component to be initialised.
@return KErrNone if successful;
        KErrNoMemory if the driver failed to allocate memory for the new component;
        otherwise one of the other system-wide error codes.
*/
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE aComponent)
	{
	TInt err = COmxILGraphicSink::CreateComponent(aComponent);
	if (err == KErrNone)
		{
		return OMX_ErrorNone;
		}
	else
		{
		// return some problem
		return err == KErrNoMemory ? OMX_ErrorInsufficientResources : OMX_ErrorUndefined;

		}
	}

/**
This function creates a new Graphic sink component.
@param aComponent The handle of the component to be created.
@return KErrNone if successful;
        KErrNoMemory if the driver failed to allocate memory for the new component;
        otherwise one of the other system-wide error codes.
*/
TInt COmxILGraphicSink::CreateComponent(OMX_HANDLETYPE aComponent)
	{
    DEBUG_PRINTF(_L8("COmxILGraphicSink::CreateComponent"));

    COmxILGraphicSink* self = new COmxILGraphicSink();

	if (!self)
		{
		return KErrNoMemory;
		}

	TRAPD(err, self->ConstructL(aComponent));
	
	if(err != KErrNone)
		{
		delete self;
		}

	return err;

	}

/**
Constructor of the class.
*/
COmxILGraphicSink::COmxILGraphicSink()
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSink::COmxILGraphicSink +"));
	}
	
/**
Destructor of the class.
*/
COmxILGraphicSink::~COmxILGraphicSink()
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSink::~COmxILGraphicSink +"));
	}

/**
Second phase construction for the component.
@param aHandle The handle of the component to be created.
*/
void COmxILGraphicSink::ConstructL(OMX_HANDLETYPE aHandle)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSink::ConstructL"));
	// Initialize the data received from IL Core
	COmxILComponent::ConstructL(aHandle);

	// STEP 2: Create the call backs manager...
	MOmxILCallbackNotificationIf* callbackNotificationIf=CreateCallbackManagerL(COmxILComponent::EOutofContext);
	
	// STEP 3: Create the Graphic Sink Processing Function...
	COmxILGraphicSinkProcessingFunction* pGraphicSinkProcessingFunction = COmxILGraphicSinkProcessingFunction::NewL(*callbackNotificationIf);
	RegisterProcessingFunction(pGraphicSinkProcessingFunction);
	
	// STEP 4: Create Port manager...
	CreatePortManagerL(COmxILComponent::ENonBufferSharingPortManager,
		TOmxILSpecVersion(),	// OMX Version
		0, 						// The number of audio ports in this component
		0,						// The starting audio port index
		0,						// The number of image ports in this component
		0,						// The starting image port index
		1,						// The number of video ports in this component
		0,						// The starting video port index
		0,						// The number of other ports in this component
		0						// The starting other port index
		);
	
	COmxILGraphicSinkVPB0Port* pb0Port = ConstructVPB0PortL();
	CleanupStack::PushL(pb0Port);
	User::LeaveIfError(AddPort(pb0Port, OMX_DirInput));
	CleanupStack::Pop(pb0Port);
	SetPortToPF(pb0Port);
	
	// STEP 5: Create the non-port related configuration manager...
	RPointerArray<TDesC8> componentRoles;
	CleanupClosePushL(componentRoles);
	componentRoles.AppendL(&KSymbianOMXGraphicSinkRole);
	COmxILConfigManager* pConfigManager = COmxILConfigManager::NewL(
                                            KSymbianOMXGraphicSinkComponentName,
                                            TOmxILVersion(iComponentVersionMajor,
                                                          iComponentVersionMinor,
                                                          iComponentVersionRevision,
                                                          iComponentVersionStep),
                                            componentRoles);

	CleanupStack::PopAndDestroy(&componentRoles);
	RegisterConfigurationManager(pConfigManager);

	// And finally, let's get everything started
	InitComponentL();
	}

/**
Create the input port VPB0Port for Graphic sink.
@return A pointer to the VPB0Port to be created.
*/
COmxILGraphicSinkVPB0Port* COmxILGraphicSink::ConstructVPB0PortL()
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSink::ConstructVPB0PortL +"));

	OMX_U32 thisPortIndex = 0;
	const TUint32  KBufferCountMin = 2;  	// OMX_U32
	const TUint32  KBufferSizeMin = 1024; 	// OMX_U32
#ifndef ILCOMPONENTCONFORMANCE
	const TUint32  KBufferAlignment = 2;		// OMX_U32
	const OMX_BOOL KBuffersContiguous = OMX_TRUE;
#else
	// conformance suite currently doesn't support allocating contiguous or
	// beyond byte aligned buffers
	const TUint32  KBufferAlignment = 0;		// OMX_U32
	const OMX_BOOL KBuffersContiguous = OMX_FALSE;
#endif
	// These arrays must left empty, to be removed from the video port constructor
	RArray<OMX_VIDEO_CODINGTYPE> supportedVideoFormats;
	RArray<OMX_COLOR_FORMATTYPE> supportedColorFormats;
	CleanupClosePushL(supportedVideoFormats);
	CleanupClosePushL(supportedColorFormats);
	
	COmxILGraphicSinkVPB0Port* vpb0Port = COmxILGraphicSinkVPB0Port::NewL(
		TOmxILCommonPortData(
			TOmxILSpecVersion(),// OMX specification version information
			thisPortIndex,		// Port number the structure applies to
			OMX_DirInput,		// Direction of this port
			KBufferCountMin,		// The minimum number of buffers this port requires
			KBufferSizeMin,		// Minimum size, in bytes, for buffers to be used for this port
			OMX_PortDomainVideo,// Domain of the port
			KBuffersContiguous,	// Buffers contiguous requirement (true or false)
			KBufferAlignment,	// Buffer aligment requirements
			OMX_BufferSupplyInput,	// supplier preference when tunneling between two ports
			COmxILPort::KBufferMarkPropagationPortNotNeeded
			),
		supportedVideoFormats,	// Supported video formats
		supportedColorFormats,	// Supported color formats
		*((COmxILGraphicSinkProcessingFunction *)(GetProcessingFunction()))
		);
	
	CleanupStack::PopAndDestroy(2, &supportedVideoFormats);

	return vpb0Port;
	}

/*
Set the graphic sink port to the graphic sink processing function, 
so the processing function can access the port definition later.
 */
void COmxILGraphicSink::SetPortToPF(COmxILGraphicSinkVPB0Port* aPort)
   {
    ((COmxILGraphicSinkProcessingFunction *)(GetProcessingFunction()))->iGraphicSinkPort = aPort;
   }
