/*
* Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
#include <openmax/il/loader/omxilcomponentif.h>
#include <openmax/il/loader/omxilsymbiancomponentif.h>

#include "comxil3gpdemuxer.h"
#include "comxil3gpdemuxertimeinputport.h"
#include "comxil3gpdemuxeraudiooutputport.h"
#include "comxil3gpdemuxervideooutputport.h"
#include "comxil3gpdemuxerprocessingfunction.h"
#include "comxil3gpdemuxerconfigmanager.h"
#include "omxil3gpdemuxer.hrh"

_LIT8(KSymbianOmxIL3gpDemuxerName, "OMX.SYMBIAN.OTHER.CONTAINER_DEMUXER.3GP");
_LIT8(KSymbianOmxIL3gpDemuxerRole, "container_demuxer");

OMXIL_COMPONENT_ECOM_ENTRYPOINT(KUidSymbianOmxIL3gpDemuxer);

OMX_ERRORTYPE SymbianErrorToOmx(TInt aError);

// Component Entry Point
OMX_ERRORTYPE OMX_ComponentInit(OMX_HANDLETYPE aComponent)
	{
	TInt err = COmxIL3GPDemuxer::CreateComponent(aComponent);
	return SymbianErrorToOmx(err);
	}

TInt COmxIL3GPDemuxer::CreateComponent(OMX_HANDLETYPE hComponent)
	{
	COmxIL3GPDemuxer* self = new COmxIL3GPDemuxer();

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

COmxIL3GPDemuxer::COmxIL3GPDemuxer()
	{
	// nothing to do
	}

void COmxIL3GPDemuxer::ConstructL(OMX_HANDLETYPE hComponent)
	{
	COmxILComponent::ConstructL(hComponent);
	MOmxILCallbackNotificationIf* callbackNotificationIf=CreateCallbackManagerL(COmxILComponent::EOutofContext);
	
	COmxIL3GPDemuxerProcessingFunction* pProcessingFunction = COmxIL3GPDemuxerProcessingFunction::NewL(*callbackNotificationIf);
	RegisterProcessingFunction(pProcessingFunction);
	
	CreatePortManagerL(COmxILComponent::ENonBufferSharingPortManager,
					TOmxILSpecVersion(),         // Component's OMX Version
					1,					         // The number of audio ports in this component
					0, 							 // The starting audio port index
					0,					         // The number of image ports in this component
					0,					         // The starting image port index
					1,					         // The number of video ports in this component
					1,					         // The starting video port index
					1,					         // The number of other ports in this component
					2							 // The starting other port index
					);
	
	RPointerArray<TDesC8> roleList;
	CleanupClosePushL(roleList);
	roleList.AppendL(&KSymbianOmxIL3gpDemuxerRole);
	COmxIL3GPDemuxerConfigManager* configManager = COmxIL3GPDemuxerConfigManager::NewL(KSymbianOmxIL3gpDemuxerName, TOmxILSpecVersion(), roleList, *(static_cast<COmxIL3GPDemuxerProcessingFunction*>(pProcessingFunction)));
	RegisterConfigurationManager(configManager);
	CleanupStack::PopAndDestroy(&roleList);

	static_cast<COmxIL3GPDemuxerProcessingFunction*>(pProcessingFunction)->SetConfigManager(*configManager);
	
	AddAudioOutputPortL();
	AddVideoOutputPortL();
	AddTimeInputPortL();

	InitComponentL();
	}

COmxIL3GPDemuxer::~COmxIL3GPDemuxer()
	{
	}

void COmxIL3GPDemuxer::AddTimeInputPortL()
	{
	TOmxILSpecVersion specVersion;

	TOmxILCommonPortData portData(
			specVersion, 
			EPortIndexTimeInput, 
			OMX_DirInput, 
			1,													// minimum number of buffers
			0,													// minimum buffer size, in bytes
			OMX_PortDomainOther, 
			OMX_TRUE,											// contigious buffers
			4,													// 4-byte alignment
			OMX_BufferSupplyUnspecified, 
			COmxILPort::KBufferMarkPropagationPortNotNeeded
			);

	RArray<OMX_OTHER_FORMATTYPE> supportedOtherFormats;

	CleanupClosePushL(supportedOtherFormats);
	supportedOtherFormats.Append(OMX_OTHER_FormatTime);

	COmxIL3GPDemuxerTimeInputPort* timeInputPort = COmxIL3GPDemuxerTimeInputPort::NewL(portData, supportedOtherFormats,
														*(static_cast<COmxIL3GPDemuxerProcessingFunction*>(GetProcessingFunction())));
	
	CleanupStack::PopAndDestroy(&supportedOtherFormats);
	CleanupStack::PushL(timeInputPort);
	User::LeaveIfError(AddPort(timeInputPort, OMX_DirInput));
	CleanupStack::Pop();//timeInputPort
	}

void COmxIL3GPDemuxer::AddAudioOutputPortL()
	{
	TOmxILSpecVersion specVersion;

	TOmxILCommonPortData portData(
			specVersion,
			EPortIndexAudioOutput, 
			OMX_DirOutput, 
			1,													// minimum number of buffers
			1600,												// minimum buffer size, in bytes
			OMX_PortDomainAudio, 
			OMX_FALSE,											// do not need contigious buffers
			4,													// 4-byte alignment
			OMX_BufferSupplyUnspecified, 
			EPortIndexAudioOutput
			);

	COmxIL3GPDemuxerAudioOutputPort* audioOutputPort = COmxIL3GPDemuxerAudioOutputPort::NewL(portData, *(static_cast<COmxIL3GPDemuxerProcessingFunction*>(GetProcessingFunction())));
	CleanupStack::PushL(audioOutputPort);
	User::LeaveIfError(AddPort(audioOutputPort, OMX_DirOutput));
	CleanupStack::Pop();//audioOutputPort

	static_cast<COmxIL3GPDemuxerProcessingFunction*>(GetProcessingFunction())->SetAudioPort(*audioOutputPort);
	}

void COmxIL3GPDemuxer::AddVideoOutputPortL()
	{
	TOmxILSpecVersion specVersion;

	TOmxILCommonPortData portData(
			specVersion, 
			EPortIndexVideoOutput,
			OMX_DirOutput,
			1,													// minimum number of buffers, we don't need buffers for format detection
			62400,												// minimum buffer size, in bytes, TODO autodetect this
			OMX_PortDomainVideo,
			OMX_FALSE,											// do not need contigious buffers
			4,													// 4-byte alignment
			OMX_BufferSupplyUnspecified,
			EPortIndexVideoOutput
			);

	COmxIL3GPDemuxerVideoOutputPort* videoOutputPort = COmxIL3GPDemuxerVideoOutputPort::NewL(portData, *(static_cast<COmxIL3GPDemuxerProcessingFunction*>(GetProcessingFunction())));
	CleanupStack::PushL(videoOutputPort);
	User::LeaveIfError(AddPort(videoOutputPort, OMX_DirOutput));
	CleanupStack::Pop();//videoOutputPort
	static_cast<COmxIL3GPDemuxerProcessingFunction*>(GetProcessingFunction())->SetVideoPort(*videoOutputPort);
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
