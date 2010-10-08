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

#include "comxil3gpmuxerprocessingfunction.h"
#include "c3gpmuxer.h"
#include <openmax/il/common/omxilcallbacknotificationif.h>
#include "comxil3gpmuxerconfigmanager.h"
#include "comxil3gpmuxervideoinputport.h"
#include "comxil3gpmuxeraudioinputport.h"
#include "log.h"
#include "c3gpmuxerwriterthreadobserver.h"

COmxIL3GPMuxerProcessingFunction* COmxIL3GPMuxerProcessingFunction::NewL(MOmxILCallbackNotificationIf& aCallbacks)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerProcessingFunction::NewL"));
	COmxIL3GPMuxerProcessingFunction* self = new (ELeave) COmxIL3GPMuxerProcessingFunction(aCallbacks);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

COmxIL3GPMuxerProcessingFunction::COmxIL3GPMuxerProcessingFunction(MOmxILCallbackNotificationIf& aCallbacks) :
 COmxILProcessingFunction(aCallbacks),
 iMuxer(NULL)
 	{
	}

void COmxIL3GPMuxerProcessingFunction::ConstructL()
	{
	}

COmxIL3GPMuxerProcessingFunction::~COmxIL3GPMuxerProcessingFunction()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerProcessingFunction::~COmxIL3GPMuxerProcessingFunction"));
	delete iMuxer;	
	delete iWriterThreadObserver;
	}

void COmxIL3GPMuxerProcessingFunction::SetConfigManager(COmxIL3GPMuxerConfigManager& aConfigManager)
	{
	iConfigManager = &aConfigManager;
	}

OMX_ERRORTYPE COmxIL3GPMuxerProcessingFunction::StateTransitionIndication(TStateIndex aNewState)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerProcessingFunction::StateTransitionIndication : aNewState[%u]"), aNewState);
	OMX_ERRORTYPE omxError = OMX_ErrorNone;
	
	switch(aNewState)
		{
		case EStateExecuting:
			{
			TRAPD(error, iMuxer->StartL(iAudioPort->IsEnabled(), iVideoPort->IsEnabled()));
			if (error != KErrNone)
				{
				omxError = SymbianErrorToOmx(error);
				}
			//Creating Writer thread observer
			TRAP(error, iWriterThreadObserver = C3GPMuxerWriterThreadObserver::NewL(iMuxer));
	        
			if (error != KErrNone)
	                {
	                omxError = SymbianErrorToOmx(error);
	                }
			iWriterThreadObserver->IssueRequest();
			break;
			}

		case ESubStateLoadedToIdle:
			{
			OMX_U32 frameWidth = 0;
			OMX_U32 frameHeight = 0;
			OMX_U32 framerate = 0;
			OMX_U32 bitRate = 0;
			OMX_U32 audioFramerate = 0;
		
			iVideoPort->GetVideoProperties(frameWidth, frameHeight, framerate, bitRate);
			audioFramerate = iAudioPort->GetAudioFrameRate();

			const HBufC* filename = iConfigManager->Filename();
			if(!filename)
				{
				// no file name has been set
				omxError = OMX_ErrorUnsupportedSetting;
				break;
				}

			delete iMuxer;
			iMuxer = NULL;

			TRAPD(error, iMuxer = C3GPMuxer::NewL(iCallbacks)); 
			if (error != KErrNone)
				{
				omxError = SymbianErrorToOmx(error);
				break;
				}

			iMuxer->SetFilename(filename);							
			iMuxer->SetAudioVideoProperties(frameWidth, frameHeight, framerate, bitRate, audioFramerate);

			break;
			}

		case EStateLoaded:
			{
			delete iMuxer;
			iMuxer = NULL;
			break;
			}

		case EStatePause:
			{
			iMuxer->Pause();
			break;
			}
					
		default:
			{
			break;
			}
		}
	
	return omxError;		
	}

OMX_ERRORTYPE COmxIL3GPMuxerProcessingFunction::BufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection)
	{
	DEBUG_PRINTF3(_L8("COmxIL3GPMuxerProcessingFunction::BufferFlushingIndication : aPortIndex[%u]; aDirection[%u]"), aPortIndex, aDirection);
	if (aDirection == OMX_DirInput)
		{
		iMuxer->FlushBuffers(aPortIndex);
		}
	else
		{
		return OMX_ErrorBadParameter;
		}

	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxIL3GPMuxerProcessingFunction::ParamIndication(OMX_INDEXTYPE /*aParamIndex*/, const TAny* /*apComponentParameterStructure*/)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerProcessingFunction::ParamIndication"));
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxIL3GPMuxerProcessingFunction::ConfigIndication(OMX_INDEXTYPE /*aConfigIndex*/, const TAny* /*apComponentConfigStructure*/)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerProcessingFunction::ConfigIndication"));
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxIL3GPMuxerProcessingFunction::BufferIndication(OMX_BUFFERHEADERTYPE* apBufferHeader,
                                                                 OMX_DIRTYPE aDirection)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerProcessingFunction::BufferIndication : aBufferHeader[%X]"), apBufferHeader);
	// Check if the previous buffer processing invalidated the Muxer 
	if (iMuxer->IsMuxerInvalid())
		{	
		return OMX_ErrorInvalidState;
		}	

	TUint32 portIndex;
	if (aDirection == OMX_DirInput)
		{
		portIndex = apBufferHeader->nInputPortIndex;
		}
	else
		{
		return OMX_ErrorBadParameter;
		}
	
	TRAPD(err, iMuxer->ProcessThisBufferL(apBufferHeader, portIndex));
	return SymbianErrorToOmx(err);
	}

OMX_BOOL COmxIL3GPMuxerProcessingFunction::BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE /*aDirection*/)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerProcessingFunction::BufferRemovalIndication : aBufferHeader[%X]"), apBufferHeader);
	if (iMuxer->RemoveBuffer(apBufferHeader))
		{
		return OMX_TRUE;
		}

	return OMX_FALSE;
	}

void COmxIL3GPMuxerProcessingFunction::SetVideoPort(COmxIL3GPMuxerVideoInputPort& aVideoPort)
	{
	iVideoPort = &aVideoPort;
	}

void COmxIL3GPMuxerProcessingFunction::SetAudioPort(COmxIL3GPMuxerAudioInputPort& aAudioPort)
	{
	iAudioPort = &aAudioPort;
	}

OMX_ERRORTYPE COmxIL3GPMuxerProcessingFunction::SymbianErrorToOmx(TInt aError)
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
