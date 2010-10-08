/*
* Copyright (c) 2008 - 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "log.h"
#include "comxil3gpdemuxerprocessingfunction.h"
#include "comxil3gpdemuxerconfigmanager.h"
#include "comxil3gpdemuxervideooutputport.h"
#include "comxil3gpdemuxeraudiooutputport.h"
#include "c3gpdemuxer.h"
#include <openmax/il/common/omxilcallbacknotificationif.h>


COmxIL3GPDemuxerProcessingFunction* COmxIL3GPDemuxerProcessingFunction::NewL(MOmxILCallbackNotificationIf& aCallbacks)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerProcessingFunction::NewL"));

	COmxIL3GPDemuxerProcessingFunction* self = new (ELeave) COmxIL3GPDemuxerProcessingFunction(aCallbacks);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

COmxIL3GPDemuxerProcessingFunction::COmxIL3GPDemuxerProcessingFunction(MOmxILCallbackNotificationIf& aCallbacks) :
 COmxILProcessingFunction(aCallbacks)
 	{
	}

void COmxIL3GPDemuxerProcessingFunction::ConstructL()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerProcessingFunction::ConstructL"));

	iDemuxer = C3GPDemuxer::NewL(iCallbacks);
	
	// this is used in the Idle->Executing transition to issue the port changed events in the correct order
	// we create the callback now so it cannot fail later on
	iStreamDetectCallback = new(ELeave) CAsyncCallBack(TCallBack(StreamDetectCallBack, this), CAsyncCallBack::EPriorityHigh);

    iRequestHelper = COmxIL3GPDemuxerRequestHelper::NewL(this);
	}

COmxIL3GPDemuxerProcessingFunction::~COmxIL3GPDemuxerProcessingFunction()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerProcessingFunction::~COmxIL3GPDemuxerProcessingFunction"));

	delete iDemuxer;
	delete iStreamDetectCallback;
	delete iRequestHelper;
	}

void COmxIL3GPDemuxerProcessingFunction::SetConfigManager(COmxIL3GPDemuxerConfigManager& aConfigManager)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPDemuxerProcessingFunction::SetConfigManager"));

	iConfigManager = &aConfigManager;	
	iConfigManager->SetDemuxer(*iDemuxer);	
	}

OMX_ERRORTYPE COmxIL3GPDemuxerProcessingFunction::StateTransitionIndication(TStateIndex aNewState)
	{
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerProcessingFunction::StateTransitionIndication"));
    
    return iRequestHelper->StateTransitionIndication(aNewState);
	}

OMX_ERRORTYPE COmxIL3GPDemuxerProcessingFunction::DoStateTransitionIndication(TStateIndex aNewState)
    {
    DEBUG_PRINTF(_L8("COmxIL3GPDemuxerProcessingFunction::DoStateTransitionIndication"));
    
    OMX_ERRORTYPE omxError = OMX_ErrorNone;

    switch(aNewState)
        {
        case EStateExecuting:
            {
            DEBUG_PRINTF(_L8("StateTransitionIndication : OMX_StateExecuting"));

            // Cause PortFormatDetected and PortSettingsChanged with
            // the format detected from the file.
            
            // However, we want this to occur after the idle->executing
            // completion notification, which FsmTransition() will queue
            // after this function completes. So we use a CAsyncCallBack
            // to create the events at some time after FsmTransition()
            // completes. A high priority is chosen to avoid being starved
            // by other busy AOs, in particular the callback manager(s).
            
            if(!iStreamsDetected)
                {
                iStreamDetectCallback->CallBack();
                }
            else
                {
                iDemuxer->Start();
                iExecuting = ETrue;
                }

            break;
            }

        case ESubStateLoadedToIdle:
            {
            DEBUG_PRINTF(_L8("StateTransitionIndication : OMX_StateLoadedToIdle"));

            const HBufC* filename = iConfigManager->Filename();
            if(filename)
                {
                // get the maximum buffer count for a port
                omxError = iDemuxer->AcquireResources(*filename);
                }
            else
                {
                // no file name has been set
                omxError = OMX_ErrorUnsupportedSetting;
                }
            break;
            }

        case EStateIdle:
            {
            DEBUG_PRINTF(_L8("StateTransitionIndication : OMX_StateIdle"));

            if (iExecuting)
                {
                iExecuting = EFalse;
                iDemuxer->Stop();
                }
            break;
            }

        case EStateLoaded:
            {
            DEBUG_PRINTF(_L8("StateTransitionIndication : OMX_StateLoaded"));

            iDemuxer->ReleaseResources();
            iStreamsDetected = EFalse;
            break;
            }

        case EStatePause:
            {
            DEBUG_PRINTF(_L8("StateTransitionIndication : OMX_StatePause"));

            iDemuxer->Pause();
            break;
            }

        default:
            {
            break;
            }
        }
    
    return omxError;        
    }

OMX_ERRORTYPE COmxIL3GPDemuxerProcessingFunction::BufferFlushingIndication(TUint32 aPortIndex,
                                                                           OMX_DIRTYPE aDirection)
    {
    DEBUG_PRINTF2(_L8("COmxIL3GPDemuxerProcessingFunction::BufferFlushingIndication : aPortIndex[%d]"), aPortIndex);
    
    return iRequestHelper->BufferFlushingIndication(aPortIndex, aDirection);
    }

OMX_ERRORTYPE COmxIL3GPDemuxerProcessingFunction::DoBufferFlushingIndication(TUint32 aPortIndex,
                                                                           OMX_DIRTYPE /*aDirection*/)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPDemuxerProcessingFunction::DoBufferFlushingIndication : aPortIndex[%d]"), aPortIndex);

	iDemuxer->FlushBuffers(aPortIndex);
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxIL3GPDemuxerProcessingFunction::ParamIndication(OMX_INDEXTYPE /*aParamIndex*/,
                                                                      const TAny* /*apComponentParameterStructure*/)
	{
	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxIL3GPDemuxerProcessingFunction::ConfigIndication(OMX_INDEXTYPE /*aConfigIndex*/,
                                                                       const TAny* /*apComponentConfigStructure*/)
	{
	return OMX_ErrorNone;
	}
 
OMX_ERRORTYPE COmxIL3GPDemuxerProcessingFunction::BufferIndication(OMX_BUFFERHEADERTYPE* apBufferHeader,
                                                                   OMX_DIRTYPE /*aDirection*/)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPDemuxerProcessingFunction::BufferIndication >: [%X]"), apBufferHeader);

	// Check if the previous buffer processing invalidated the demuxer 
	if (iDemuxer->Invalid())
		{	
		return OMX_ErrorInvalidState;
		}	
	
	TUint32 portIndex;
	portIndex = apBufferHeader->nOutputPortIndex;

	if (portIndex >= COmxIL3GPDemuxer::EPortIndexMax)
		{
		return OMX_ErrorBadPortIndex;
		}

	TRAPD(err, iDemuxer->ProcessThisBufferL(apBufferHeader, portIndex));

	DEBUG_PRINTF2(_L8("COmxIL3GPDemuxerProcessingFunction::BufferIndication :< size [%d]"), apBufferHeader->nFilledLen);

	if (err == KErrNone)
		{
		return OMX_ErrorNone;
		}
	else
		{
		return OMX_ErrorUndefined;		
		}	
	}

OMX_BOOL COmxIL3GPDemuxerProcessingFunction::BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader,
                                                                   OMX_DIRTYPE aDirection)
    {
    DEBUG_PRINTF2(_L8("COmxIL3GPDemuxerProcessingFunction::BufferRemovalIndication >: [%X]"), apBufferHeader);
    
    return iRequestHelper->BufferRemovalIndication(apBufferHeader, aDirection);
    }

OMX_BOOL COmxIL3GPDemuxerProcessingFunction::DoBufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPDemuxerProcessingFunction::DoBufferRemovalIndication : BUFFER [%X]"), apBufferHeader);

	if (iDemuxer->RemoveBuffer(apBufferHeader, aDirection))
		{
		return OMX_TRUE;
		}

	return OMX_FALSE;
	}

OMX_U32 COmxIL3GPDemuxerProcessingFunction::NumAvailableStreams(COmxIL3GPDemuxer::TPortIndex aPortType)
	{
	switch(aPortType)
		{
	case COmxIL3GPDemuxer::EPortIndexVideoOutput:
			{
			TSize frameSize;
			TVideoFormat format;
			TBool videoAvailable = iDemuxer->GetVideoFormat(frameSize, format);
			return videoAvailable ? 1 : 0;
			}
			
	case COmxIL3GPDemuxer::EPortIndexAudioOutput:
		{
		TAudioFormat format;
		return iDemuxer->GetAudioFormat(format) ? 1 : 0;
		}

	default:
		User::Invariant();
		return 0;
		}
	}

OMX_U32 COmxIL3GPDemuxerProcessingFunction::ActiveStream(COmxIL3GPDemuxer::TPortIndex /*aPortType*/)
	{
	//TODO
	return 0;
	}

OMX_ERRORTYPE COmxIL3GPDemuxerProcessingFunction::SetActiveStream(COmxIL3GPDemuxer::TPortIndex aPortType, OMX_U32 aActiveStream)
	{
	switch(aPortType)
		{
	case COmxIL3GPDemuxer::EPortIndexVideoOutput:
			{
			TSize frameSize;
			TVideoFormat format;
				TBool videoAvailable = iDemuxer->GetVideoFormat(frameSize, format);
			if(videoAvailable && aActiveStream == 0)
				{
				return OMX_ErrorNone;
				}
			else
				{
				return OMX_ErrorUnsupportedSetting;
				}
			}
	case COmxIL3GPDemuxer::EPortIndexAudioOutput:
		// TODO
//ZK		return OMX_ErrorNotImplemented;
		return OMX_ErrorNone;
		
	default:
		return OMX_ErrorUnsupportedIndex;
		}
	}

void COmxIL3GPDemuxerProcessingFunction::SetVideoPort(COmxIL3GPDemuxerVideoOutputPort& aVideoPort)
	{
	iVideoPort = &aVideoPort;
	}

void COmxIL3GPDemuxerProcessingFunction::SetAudioPort(COmxIL3GPDemuxerAudioOutputPort& aAudioPort)
	{
	iAudioPort = &aAudioPort;
	}

TInt COmxIL3GPDemuxerProcessingFunction::StreamDetectCallBack(TAny* aPtr)
	{
	reinterpret_cast<COmxIL3GPDemuxerProcessingFunction*>(aPtr)->DoStreamDetect();
	return KErrNone;
	}

void COmxIL3GPDemuxerProcessingFunction::DoStreamDetect()
	{
	OMX_ERRORTYPE error = iDemuxer->DetectStreams();
	if(error)
		{
		iCallbacks.EventNotification(OMX_EventError, (TUint32)OMX_ErrorFormatNotDetected, 0, NULL);
		return;
		}

	// video format has been discovered
	// check against port definition
	// if autodetect, cause a PortSettingsChanged event
	// if not autodetect and detected format is different, cause an error event
	TSize size;
	TVideoFormat format;
	TBool videoAvailable = iDemuxer->GetVideoFormat(size, format);
	// errors from EventNotification are ignored - these will be
	// OMX_ErrorInsufficientResources if the callback could not be
	// created or added to the queue. In this case, we probably want
	// the callback manager to send an out-of-band error event to the
	// IL client but this does not happen in 1509 yet
	if(format.iCoding == OMX_VIDEO_CodingMax)
		{
		// format could not be mapped, and so is unsupported
		iCallbacks.EventNotification(OMX_EventError, (TUint32) OMX_ErrorFormatNotDetected, 0, NULL);
		// only send FormatNotDetected once for the component, not for both ports
		return;
		}
	else
		{
		if (iVideoPort->IsEnabled())
			{
			iStreamsDetected = ETrue;
			if (iVideoPort->VideoFormat() == OMX_VIDEO_CodingAutoDetect)
				{
				if(videoAvailable)
					{
					iVideoPort->FormatDetected(size, format);
					}
				// TODO spec doesn't say what the params should be for PortFormatDetected
				iCallbacks.EventNotification(OMX_EventPortFormatDetected, COmxIL3GPDemuxer::EPortIndexVideoOutput, 0, NULL);
				iCallbacks.EventNotification(OMX_EventPortSettingsChanged, OMX_IndexParamPortDefinition, COmxIL3GPDemuxer::EPortIndexVideoOutput, NULL);
				}
			else
				{
				if (iVideoPort->VideoFormat() != format.iCoding)
					{
					iStreamsDetected = EFalse;
					iCallbacks.EventNotification(OMX_EventError, (TUint32)OMX_ErrorFormatNotDetected, 0, NULL);
					return;
					}
				}
			}
		}

	TAudioFormat audioFormat;
	TBool audioAvailable = iDemuxer->GetAudioFormat(audioFormat);

	if(audioFormat.iCoding == OMX_AUDIO_CodingMax)
		{
		// format could not be mapped, and so is unsupported
		iCallbacks.EventNotification(OMX_EventError, (TUint32)OMX_ErrorFormatNotDetected, 0, NULL);
		return;
		}
	else
		{
		if (iAudioPort->IsEnabled())
			{
			iStreamsDetected = ETrue;
			if (iAudioPort->AudioFormat() == OMX_AUDIO_CodingAutoDetect)
				{
				if (audioAvailable)
					{
					iAudioPort->FormatDetected(audioFormat);
					}
				iCallbacks.EventNotification(OMX_EventPortFormatDetected, COmxIL3GPDemuxer::EPortIndexAudioOutput, 0, NULL);
				iCallbacks.EventNotification(OMX_EventPortSettingsChanged, OMX_IndexParamPortDefinition, COmxIL3GPDemuxer::EPortIndexAudioOutput, NULL);
				}
			else
				{
				if (iAudioPort->AudioFormat() != audioFormat.iCoding)
					{
					iStreamsDetected = EFalse;
					iCallbacks.EventNotification(OMX_EventError, (TUint32)OMX_ErrorFormatNotDetected, 0, NULL);
					return;
					}
				}
			}
		}
	//Only start the demuxer if one of its ports is enabled.
	if (iStreamsDetected)
		{
		iDemuxer->Start();
		iExecuting = ETrue;
		}
	}

