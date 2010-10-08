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

#include "log.h"

#include "omxilgraphicsinkprocessingfunction.h"
#include "omxilgraphicsinktrace.h"
#include "omxilgraphicsinkpanics.h"
#include "omxilgraphicsinkvpb0port.h"
#include <hal.h>
#include <graphics/suerror.h>
#include "omxilgraphicsinkextensionsindexes.h"
#include <openmax/il/shai/OMX_Symbian_ComponentExt.h>
#include <openmax/il/common/omxilcallbacknotificationif.h>

// Constant numbers
#ifndef __WINSCW__
const TInt KRefGfxAlignment = RSurfaceManager::EPageAligned;
#else
const TInt KRefGfxAlignment = 2;
#endif
static const TBool KRefGfxContiguous = ETrue;
static const TInt KSurfaceUpdateNumOfMessageSlots = 4;
static const TUint32 KNullTickCount = 0xFFFFFFFF;


/**
Create a new processing function object.

@param 	aCallbacks The callback manager interface for processing function.

@return A pointer to the processing function object to be created.
*/
COmxILGraphicSinkProcessingFunction*
COmxILGraphicSinkProcessingFunction::NewL(MOmxILCallbackNotificationIf& aCallbacks)
	{
	COmxILGraphicSinkProcessingFunction* self =
		new (ELeave)COmxILGraphicSinkProcessingFunction(aCallbacks);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

/**
Second phase construction for the processing function. Loads the device driver for surface manager and initializes the surface attributes.
*/
void
COmxILGraphicSinkProcessingFunction::ConstructL()
	{	
    iTransitionToPauseWait = new(ELeave) CActiveSchedulerWait();
    User::LeaveIfError(iTransitionToPauseWaitSemaphore.CreateLocal(0));

    //record the ID of the creator thread for later use
    iOwnerThreadId = RThread().Id();

	iGraphicSurfaceAccess = CGraphicSurfaceAccess::NewL(*this);	        
	iPFHelper = CPFHelper::NewL(*this, *iGraphicSurfaceAccess);
	
	User::LeaveIfError(iBufferMutex.CreateLocal());
	InitSurfaceAttributes();
    iState = OMX_StateLoaded;
	}

/**
Constructor of the class.

@param 	aCallbacks The callback manager interface for processing function.
*/
COmxILGraphicSinkProcessingFunction::COmxILGraphicSinkProcessingFunction(
		MOmxILCallbackNotificationIf& aCallbacks)
	:
	COmxILProcessingFunction(aCallbacks)
	{
	HAL::Get(HALData::EFastCounterFrequency,iFastCounterFrequency);
	}

/**
Destructor of the class.
*/
COmxILGraphicSinkProcessingFunction::~COmxILGraphicSinkProcessingFunction()
	{
    delete iTransitionToPauseWait;
    iTransitionToPauseWaitSemaphore.Close();
    
	// Check in case the driver has not been closed. That would happen in
	// an scenario where the component is deleted while being in
	// OMX_StateExecuting state.
	if(iPFHelper &&
	   (iState == OMX_StateInvalid  ||
	    iState == OMX_StateExecuting ||
	    iState == OMX_StatePause))
		{
		// Ignore error if the following call fails
		iPFHelper->StopSync();
		}

	delete iPFHelper;
	delete iGraphicSurfaceAccess;
	
	iSurfaceManager.Close();
	
	// Buffer headers are not owned by the processing function
	iBuffersToEmpty.Close();
	
	iBufferMutex.Close();
	}

/**
This method provides the state change information within the processing function so that appropriate action can be taken. 
This state change information is provided by the FSM on behalf of the IL Client.

@param 	aNewState The new state of FSM.

@return  OMX_ErrorNone if successful
         OMX_ErrorInsufficientResources if fail to start GraphicSink frame acceptor
         OMX_ErrorIncorrectStateTransition if unsupported state
         Any other OpenMAX IL wide error code
*/
OMX_ERRORTYPE
COmxILGraphicSinkProcessingFunction::StateTransitionIndication(TStateIndex aNewState)
	{
	switch(aNewState)
		{
	case EStateExecuting:
		{
		return iPFHelper->ExecuteAsync();
		}
	case EStateInvalid:
		{
		return iPFHelper->StopAsync();
		}
	case EStatePause:
		{
		// must be done immediately
		OMX_ERRORTYPE omxErr = iPFHelper->Pause();
		if(omxErr == OMX_ErrorNone)
			{
			WaitForTransitionToPauseToFinish();
			}
	    return omxErr;
		}
	case EStateIdle:
		{
		iBufferMutex.Wait();
		iBuffersToEmpty.Reset();
		iBufferMutex.Signal();
		iState = OMX_StateIdle;
		return OMX_ErrorNone;
		}
	case EStateLoaded:
	case EStateWaitForResources:
		{
		return iPFHelper->StopAsync();
		}
	case ESubStateLoadedToIdle:
		{
	    // Open a channel to the surface manager logical device driver. 
	    TInt err = iSurfaceManager.Open();
	    if ( err != KErrNone)
	        {
	        return OMX_ErrorHardware;
	        }
	    
	    if (iPFHelper->OpenDevice() != KErrNone)
	        {
            return OMX_ErrorInsufficientResources;
	        }
	    /*
	    if (iGraphicSinkPort->ValidateStride() != OMX_ErrorNone)
	        {
            return OMX_ErrorUnsupportedSetting;
	        }
	    return OMX_ErrorNone;
	    */
	    return iGraphicSinkPort->ValidateStride();
		}
	case ESubStateIdleToLoaded:
		{
		return iPFHelper->CloseDevice();
		}
	case ESubStateExecutingToIdle:
		{
		// must be done immediately
		return iPFHelper->StopAsync();
		}
	case ESubStatePauseToIdle:
		{
		// Ignore these transitions...
		return OMX_ErrorNone;
		}
	default:
		{
		return OMX_ErrorIncorrectStateTransition;
		}
		};
	}

/**
Flushes all the buffers retained by the processing function and sends it to either IL Client or the Tunelled component, as the case may be.

@param 	aPortIndex Port index used to flush buffers from a given port of the component.
@param  aDirection This describes the direction of the port.
		
@return OMX_ErrorNone if successful;
		Any other OpenMAX IL wide error code;
*/
OMX_ERRORTYPE
COmxILGraphicSinkProcessingFunction::BufferFlushingIndication(
	TUint32 aPortIndex,
	OMX_DIRTYPE aDirection)
	{
	iBufferMutex.Wait();
    if ((aPortIndex == OMX_ALL && aDirection == OMX_DirMax) ||
		(aPortIndex == 0 && aDirection == OMX_DirInput))
	{
	    // Send BufferDone notifications for each buffer...
		if(iBufferOnScreen)
			{
			iCallbacks.BufferDoneNotification(iBufferOnScreen, 0, OMX_DirInput);
			iBufferOnScreen = NULL;
			}
		const TUint bufferCount = iBuffersToEmpty.Count();
		OMX_BUFFERHEADERTYPE* pBufferHeader = 0;
		for (TUint i=0; i<bufferCount; i++)
			{
			pBufferHeader = iBuffersToEmpty[i];
			pBufferHeader->nTickCount = KNullTickCount;
			iCallbacks.
				BufferDoneNotification(
					pBufferHeader,
					pBufferHeader->nInputPortIndex,
					OMX_DirInput
					);
			}
		// Empty buffer lists...
		iBuffersToEmpty.Reset();

		iBufferMutex.Signal();
		return OMX_ErrorNone;
	}
	else
		{
		iBufferMutex.Signal();
		return OMX_ErrorBadParameter;
		}
	}

/**
Update the structure corresponding to the given index which belongs to the static parameters list.

@param 	aParamIndex The index representing the desired structure to be updated.
@param  aComponentParameterStructure A pointer to structure which has the desired settings that will be used to update the Processing function.
		
@return OMX_ErrorNone if successful;
		OMX_ErrorUnsupportedIndex if unsupported index;
		OMX_ErrorUnsupportedSetting if pixel format is EUidPixelFormatUnknown;
		Any other OpenMAX IL wide error code;
*/
OMX_ERRORTYPE
COmxILGraphicSinkProcessingFunction::ParamIndication(
	OMX_INDEXTYPE aParamIndex,
	const TAny* apComponentParameterStructure)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkProcessingFunction::PortParamIndication"));
	
	switch(aParamIndex)
		{
		case OMX_IndexParamPortDefinition:
			{
			const OMX_PARAM_PORTDEFINITIONTYPE* portDefinition = static_cast<const OMX_PARAM_PORTDEFINITIONTYPE*>(apComponentParameterStructure);
			
			//
			// All the fields in SurfaceAttribute structure should be updated.
			//
			iGraphicSurfaceSettings.iSurfaceAttributes.iSize.iWidth = portDefinition->format.video.nFrameWidth;
			iGraphicSurfaceSettings.iSurfaceAttributes.iSize.iHeight = portDefinition->format.video.nFrameHeight;
			
			// Need to convert OMX Color Format to TUidPixelFormat.
			TUidPixelFormat pixelFormat = ConvertPixelFormat(portDefinition->format.video.eColorFormat);
			if(pixelFormat == EUidPixelFormatUnknown)
				{
                return OMX_ErrorUnsupportedSetting;
				}
			else
				{
                iGraphicSurfaceSettings.iSurfaceAttributes.iPixelFormat = pixelFormat;
				}
			
			iGraphicSurfaceSettings.iSurfaceAttributes.iBuffers = portDefinition->nBufferCountActual;
			iGraphicSurfaceSettings.iSurfaceAttributes.iStride = portDefinition->format.video.nStride;
			break;
			}
		case OMX_IndexParamVideoPortFormat:
			{
			const OMX_VIDEO_PARAM_PORTFORMATTYPE* videoPortFormat = static_cast<const OMX_VIDEO_PARAM_PORTFORMATTYPE*>(apComponentParameterStructure);

			// only OMX_COLOR_FORMATTYPE eColorFormat to be used for SurfaceAttributes.iPixelFormat		
			TUidPixelFormat pixelFormat = ConvertPixelFormat(videoPortFormat->eColorFormat);
			if(pixelFormat == EUidPixelFormatUnknown)
				{
				return OMX_ErrorUnsupportedSetting;
				}
			else
				{
                iGraphicSurfaceSettings.iSurfaceAttributes.iPixelFormat = pixelFormat;
				}
			break;
			}
		default:
			{
			return OMX_ErrorUnsupportedIndex;
			}
		}
	return OMX_ErrorNone;
	}

/**
Update the structure corresponding to the given index which belongs to the dynamic configuration list.

@param 	aConfigIndex The index representing the desired structure to be updated.
@param  aComponentConfigStructure A pointer to structure which has the desired settings that will be used to update the Processing function.
		
@return OMX_ErrorNone if successful;
		OMX_ErrorUnsupportedIndex if unsupported index;
		OMX_ErrorUnsupportedSetting if SurfaceConfiguration returns error;
		Any other OpenMAX IL wide error code;
*/
OMX_ERRORTYPE
COmxILGraphicSinkProcessingFunction::ConfigIndication(OMX_INDEXTYPE aConfigIndex,
													  const TAny* apComponentConfigStructure)
	{
	DEBUG_PRINTF(_L8("COmxILGraphicSinkProcessingFunction::ConfigIndication"));

	TInt err = KErrNone;
	
	switch(aConfigIndex)
		{
	    case OMX_SymbianIndexConfigSharedChunkMetadata:
            {           
            const OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE*
                pSharedChunkMetadata
                = static_cast<
                const OMX_SYMBIAN_CONFIG_SHAREDCHUNKMETADATATYPE*>(
                    apComponentConfigStructure);
 
            iGraphicSurfaceAccess->iSharedChunkHandleId = pSharedChunkMetadata->nHandleId;
            iGraphicSurfaceAccess->iSharedChunkThreadId = pSharedChunkMetadata->nOwnerThreadId;
            
            iGraphicSurfaceAccess->iIsLocalChunk = EFalse;
            break;
            }
		case OMX_IndexConfigCommonScale:
			{
			const OMX_CONFIG_SCALEFACTORTYPE* scaleFactor = static_cast<const OMX_CONFIG_SCALEFACTORTYPE*>(apComponentConfigStructure);

			err = iGraphicSurfaceSettings.iSurfaceConfig.SetExtent(TRect(TSize(scaleFactor->xWidth, scaleFactor->xHeight)));
			if(err != KErrNone)
				{
				return OMX_ErrorUnsupportedSetting;
				}

			break;
			}
		case OMX_IndexConfigCommonOutputSize:
			{
			const OMX_FRAMESIZETYPE* frameSize = static_cast<const OMX_FRAMESIZETYPE*>(apComponentConfigStructure);

			err = iGraphicSurfaceSettings.iSurfaceConfig.SetExtent(TRect(TSize(frameSize->nWidth, frameSize->nHeight)));
			if(err != KErrNone)
				{
				return OMX_ErrorUnsupportedSetting;
				}
			
			break;
			}
		case OMX_IndexConfigCommonInputCrop:
		case OMX_IndexConfigCommonOutputCrop:
		case OMX_IndexConfigCommonExclusionRect:
			{
			const OMX_CONFIG_RECTTYPE* rec = static_cast<const OMX_CONFIG_RECTTYPE*>(apComponentConfigStructure);

			err = iGraphicSurfaceSettings.iSurfaceConfig.SetExtent(TRect(TPoint(rec->nTop, rec->nLeft), TSize(rec->nWidth, rec->nHeight)));
			if(err != KErrNone)
				{
				return OMX_ErrorUnsupportedSetting;
				}
						
			break;
			}
		default:
			{
			return OMX_ErrorUnsupportedIndex;
			}
		}
	
	return OMX_ErrorNone;
	}

void COmxILGraphicSinkProcessingFunction::SetSharedChunkBufConfig(TMMSharedChunkBufConfig aSharedChunkBufConfig)
    {
    iGraphicSurfaceAccess->iSharedChunkBufConfig = aSharedChunkBufConfig;
    }

void COmxILGraphicSinkProcessingFunction::GetSharedChunkMetadata(
    OMX_U32& aHandleId,
    OMX_U64& aThreadId) const
    {
    aHandleId = iGraphicSurfaceAccess->iSharedChunkHandleId;
    aThreadId = iGraphicSurfaceAccess->iSharedChunkThreadId;
    }

/**
This method is invoked whenever the component is requested to emtpy/display the contents of the buffers passed as function arguments.

@param 	apBufferHeader A pointer to buffer header.
@param  aDirection provides the direction either input or output. This can be used as a further check whether buffers received are valid or not.
		
@return OMX_ErrorNone if successful;
		Any other OpenMAX IL wide error code;
*/
OMX_ERRORTYPE
COmxILGraphicSinkProcessingFunction::BufferIndication(
	OMX_BUFFERHEADERTYPE* apBufferHeader,
	OMX_DIRTYPE aDirection)
	{
    if (aDirection != OMX_DirInput)
        {
        return OMX_ErrorBadParameter;
        }
    //The nTickCount is just internal here, stored temporarily. So it is count not time period.
    apBufferHeader->nTickCount = User::FastCounter();
    iBufferMutex.Wait();
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    if (iBuffersToEmpty.Append(apBufferHeader) != KErrNone)
        {
        apBufferHeader->nTickCount = KNullTickCount;
        ret = OMX_ErrorInsufficientResources;
        }
    else if (iState != OMX_StateExecuting)
        {
        // If Component not in an executing state delay processing buffer
        ret = OMX_ErrorNone;
        }
    else if (iPFHelper->BufferIndication() != KErrNone)
        {
        apBufferHeader->nTickCount = KNullTickCount;
        ret = OMX_ErrorInsufficientResources;
        }
    iBufferMutex.Signal();
    return ret;
	}

/**
This method is used to check whether the required buffer is held by the processing function or not.

@param 	apBufferHeader A pointer to buffer header being searched.
@param  aDirection provides the direction either input or output. This can be used as a further check whether buffers received are valid or not.
		
@return OMX_TRUE if find the buffer;
		OMX_FALSE if fail to find the buffer;
*/
OMX_BOOL
COmxILGraphicSinkProcessingFunction::BufferRemovalIndication(
	OMX_BUFFERHEADERTYPE* apBufferHeader,
	OMX_DIRTYPE /* aDirection */)
	{
    TBool headerDeletionResult = ETrue;
    
	TInt headerIndexInArray = KErrNotFound;
	iBufferMutex.Wait();
	if (KErrNotFound !=
		(headerIndexInArray =
		 iBuffersToEmpty.Find(apBufferHeader)))
		{
		iBuffersToEmpty.Remove(headerIndexInArray);
		}
	else if(iBufferOnScreen == apBufferHeader)
		{
		iBufferOnScreen = NULL;
		}
	else
		{
		headerDeletionResult = EFalse;
		}
	iBufferMutex.Signal();
    return (headerDeletionResult ? OMX_TRUE : OMX_FALSE);
	}

/**
This method creates COmxILMMBuffer class object. Also creates surface and allocates the chunks based on the number of buffers, 
maps the surface in the given process. It also allocates the resources like creating the message queue and other necessary C 
class objects. This method gets called when the component acts as buffer supplier.

@param 	aPortSpecificBuffer gives the starting address of the specific buffer.
@param  aPortPrivate gives the private data which is COmxILMMBuffer pointer in this case.
		
@leave  OMX_ErrorNone if successful;
@leave  OMX_ErrorInsufficientResources if function returns KErrNoMemory;
@leave  OMX_ErrorBadParameter if function returns errors except KErrNoMemory;
@leave  Any other OpenMAX IL wide error code;
*/
void COmxILGraphicSinkProcessingFunction::CreateBufferL(OMX_U8*& aPortSpecificBuffer, OMX_PTR& aPortPrivate, OMX_U32 aBufferCountActual)
	{
	iGraphicSurfaceAccess->CreateBufferL(aPortSpecificBuffer, aPortPrivate, aBufferCountActual);
	}

/**
Destroy MM buffer, close surface, and deallocate other resources like message queue and C class objects. This is called when component
acts as buffer supplier.
@param  apPortPrivate gives the private data which is COmxILMMBuffer pointer in this case.
*/
void COmxILGraphicSinkProcessingFunction::DestroyBuffer(OMX_PTR /*apPortPrivate*/)
	{
    if( iGraphicSurfaceAccess )
        {
        iGraphicSurfaceAccess->iBufferIdGenerator--;
         // to reset surface id in case client requests different settings.
         if(iGraphicSurfaceAccess->iBufferIdGenerator == 0)
             {
             iGraphicSurfaceAccess->ResetSurfaceId();        
             iGraphicSurfaceAccess->CloseChunk();
             }   
        }
	}

/**
Creates the surface by utilizing the buffers passed via application private data. It then maps the surface in the given process. 
It also allocates the resources like creating the message queue and other necessary C class objects. This method gets called when 
the component acts as non buffer supplier.

@param aSizeBytes The size of buffer.
@param apBuffer gives the starting address of the specific buffer.
@param aAppPrivate provides the private data which is COmxILMMBuffer pointer in this case and holds details of buffers already allocated.
@param aBufferCountActual The actual number of buffers.

@leave  OMX_ErrorNone if successful;
@leave  OMX_ErrorInsufficientResources if aPortPrivate is null or functions return KErrNoMemory;
@leave	OMX_ErrorBadParameter if functions return errors except KErrNoMemory;
@leave	Any other OpenMAX IL wide error code;
*/
void COmxILGraphicSinkProcessingFunction::InitBufferL(OMX_U32 aSizeBytes, OMX_U8* apBuffer, OMX_U32 aBufferCountActual)
	{
	iGraphicSurfaceAccess->InitBufferL(aSizeBytes, apBuffer, aBufferCountActual);
	}

/**
Deallocate resources like message queue and C class objects. This is called when component acts as non buffer supplier.
*/
void COmxILGraphicSinkProcessingFunction::DeInitBuffer()
	{
	// to reset surface id in case client requests different settings.
	if(iGraphicSurfaceAccess)
		{
		iGraphicSurfaceAccess->ResetSurfaceId(); 
		iGraphicSurfaceAccess->CloseChunk();
		}
	}

/**
Initialise surface attribute structure.
*/	
void COmxILGraphicSinkProcessingFunction::InitSurfaceAttributes()
	{
	RSurfaceManager::TSurfaceCreationAttributes* attr = &iGraphicSurfaceSettings.iSurfaceAttributes;
	
	attr->iAlignment = KRefGfxAlignment;
	attr->iContiguous = KRefGfxContiguous;
	attr->iCacheAttrib = RSurfaceManager::ENotCached;
	attr->iMappable = ETrue;
	}

TUidPixelFormat COmxILGraphicSinkProcessingFunction::ConvertPixelFormat(OMX_COLOR_FORMATTYPE aColorFormat)
	{
	switch(aColorFormat)
		{
		// OMX "Planar" formats not currently supported by GraphicSink since data comes in more than one buffer.
		// "PackedPlanar" formats can be added easily provided the GCE backend supports them.

		case OMX_COLOR_Format16bitRGB565:
			return EUidPixelFormatRGB_565;

		case OMX_COLOR_Format32bitARGB8888:
			return EUidPixelFormatARGB_8888;

		case OMX_COLOR_FormatYCrYCb:
			return EUidPixelFormatYUV_422Reversed;
		
		case OMX_COLOR_FormatCbYCrY:
			return EUidPixelFormatYUV_422Interleaved;

		// Need to map color format to Symbian pixel format.
		default:
			{
			return EUidPixelFormatUnknown;
			}
		}
	}

void COmxILGraphicSinkProcessingFunction::WaitForTransitionToPauseToFinish()
    {
    if(RThread().Id() == iOwnerThreadId)
        {
        //if the owner thread is the same thread as the one created the active objects in this processing function
        //then we can wait by using CActiveSchedulerWait
        DEBUG_PRINTF(_L8("GraphicSinkProcessingFunction::WaitForTransitionToPauseToFinish - blocking transition to pause with active scheduler wait now"));
        iTransitionToPauseWait->Start();
        }
    else
        {
        //if this is a thread different from the creator thread then semaphore is needed to block this thread until the transition
        //to paused state completes
        DEBUG_PRINTF(_L8("GraphicSinkProcessingFunction::WaitForTransitionToPauseToFinish - blocking thread with semaphore now"));
        iTransitionToPauseWaitSemaphore.Wait();
        }
    }

void COmxILGraphicSinkProcessingFunction::TransitionToPauseFinished()
    {
    if(iTransitionToPauseWait->IsStarted())
        {
        DEBUG_PRINTF(_L8("GraphicSinkProcessingFunction::TransitionToPauseFinished - unblocking transition to pause (active scheduler wait) now"));
        iTransitionToPauseWait->AsyncStop();
        }
    else
        {
        DEBUG_PRINTF(_L8("GraphicSinkProcessingFunction::TransitionToPauseFinished - unblocking transition to pause (semaphore) now"));
        iTransitionToPauseWaitSemaphore.Signal();
        }
    }

COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess* COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::NewL(COmxILGraphicSinkProcessingFunction& aParent)
	{
	return new (ELeave) CGraphicSurfaceAccess(aParent);
	}

COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::CGraphicSurfaceAccess(COmxILGraphicSinkProcessingFunction& aParent)
: CActive(EPriorityStandard),
  iIsLocalChunk(ETrue),
  iParent(aParent)
	{
	CActiveScheduler::Add(this);
	iSurfaceId = TSurfaceId::CreateNullId();
	iOffsetArray.Reset();
	}

void COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::CloseChunk()
    {
    if(iChunk.Handle())
        {
        iChunk.Close();
        }
    }

COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::~CGraphicSurfaceAccess()
	{
	Cancel();
	
	CloseChunk();

	if (!iSurfaceId.IsNull())
	    {
	    iParent.SurfaceManager().CloseSurface(iSurfaceId);	// ignore the error
	    }
    #ifdef ILCOMPONENTCONFORMANCE
	iArrayOffsets.Close();
    #endif

	iOffsetArray.Close();
	iSurfaceUpdateSession.Close();

	}

void COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::RunL()
	{
	// The buffer is not on the list implies that they have already been flushed/spotted 
	// via BufferFlushingIndication/BufferRemovalIndication
	iParent.iBufferMutex.Wait();
	TInt index = iParent.BuffersToEmpty().Find(iCurrentBuffer);
	if (KErrNotFound != index)
		{
		switch(iStatus.Int())
			{
			case KErrNone: 
				{
				// Consumed all data completely and setting nFilledLen to zero.
				iCurrentBuffer->nFilledLen = 0;
				break;
				}
			case KErrCancel:
			default:
				{
				// Leave actual value of iCurrentBuffer->nFilledLen
				DEBUG_PRINTF2(_L8("COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::RunL() err = %d"), iStatus.Int());
				}
			};

		if(iStatus.Int() != KErrNone)
		    {
		    iCurrentBuffer->nTickCount = KNullTickCount;
		    }
		else
			{
			TUint32 currentTickCount = User::FastCounter();

			// On some hardware boards, tick count decrements as time increases so
			// need to check which way the counter is going
			if (currentTickCount >= iCurrentBuffer->nTickCount)
				{
				iCurrentBuffer->nTickCount = (currentTickCount - iCurrentBuffer->nTickCount) / iParent.GetFastCounterFrequency();
				}
			else
				{
				iCurrentBuffer->nTickCount = (iCurrentBuffer->nTickCount - currentTickCount) / iParent.GetFastCounterFrequency();
				}
			}

        
        if(iCurrentBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            {
            iParent.GetCallbacks().EventNotification(OMX_EventBufferFlag, iCurrentBuffer->nInputPortIndex, iCurrentBuffer->nFlags, NULL);
            }

		iCurrentBuffer->nFilledLen = 0;
		iCurrentBuffer->nFlags = 0;
		iCurrentBuffer->nOffset = 0;
		iCurrentBuffer->nTimeStamp = 0;

		// now sending back to framework..
		if(iParent.iBufferOnScreen)
			{
			// Add error handling?
			iParent.GetCallbacks().BufferDoneNotification(iParent.iBufferOnScreen, iParent.iBufferOnScreen->nInputPortIndex,OMX_DirInput);
			}
		iParent.iBufferOnScreen = iCurrentBuffer;

		iParent.BuffersToEmpty().Remove(index);
		iCurrentBuffer = NULL;
		
		// check if any more buffers to be consumed..
		if (ProcessNextBuffer() != KErrNone)
			{
			iParent.GetCallbacks().ErrorEventNotification(OMX_ErrorInsufficientResources);
			}
		}
	iParent.iBufferMutex.Signal();
	}

void COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::CreateBufferL(OMX_U8*& aPortSpecificBuffer, OMX_PTR& aPortPrivate, OMX_U32 aBufferCountActual)
	{
	if(iSurfaceId.IsNull())
		{
		// race condition on nBufferCountActual
		if(aBufferCountActual != iParent.GraphicSurfaceSettings().iSurfaceAttributes.iBuffers)
		    {
		    iParent.GraphicSurfaceSettings().iSurfaceAttributes.iBuffers = aBufferCountActual;
		    }

		User::LeaveIfError(iParent.SurfaceManager().CreateSurface(iParent.GraphicSurfaceSettings().iSurfaceAttributesBuf, iSurfaceId));
		
		RChunk chunk;
		CleanupClosePushL(chunk);
		User::LeaveIfError(iParent.SurfaceManager().MapSurface(iSurfaceId, chunk));
		
		//We need to change the chunk handle to be shared by the whole process.
		RThread thread;
		CleanupClosePushL(thread);
        iChunk.SetHandle(chunk.Handle());
        User::LeaveIfError(iChunk.Duplicate(thread));
		CleanupStack::PopAndDestroy(2, &chunk);

        // for SetConfig(OMX_SYMBIAN_CONFIG_SHARED_CHUNK_METADATA)
        iSharedChunkHandleId = iChunk.Handle();
        iSharedChunkThreadId = RThread().Id().Id();

		switch(iParent.iGraphicSurfaceSettings.iSurfaceAttributes.iPixelFormat)
			{
		case EUidPixelFormatYUV_422Reversed:
			{
			// fill buffer 0 with black
			TUint32* data = reinterpret_cast<TUint32*>(iChunk.Base());
			TInt numPixelPairs = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iStride * iParent.GraphicSurfaceSettings().iSurfaceAttributes.iSize.iHeight / 4;
			for(TInt offset = 0; offset < numPixelPairs; offset++)
				{
				data[offset] = 0x80108010;
				}
			}
			break;
		case EUidPixelFormatYUV_422Interleaved:
			{
			// fill buffer 0 with black
			TUint32* data = reinterpret_cast<TUint32*>(iChunk.Base());
			TInt numPixelPairs = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iStride * iParent.GraphicSurfaceSettings().iSurfaceAttributes.iSize.iHeight / 4;
			for(TInt offset = 0; offset < numPixelPairs; offset++)
				{
				data[offset] = 0x10801080;
				}
			}
			break;
		case EUidPixelFormatRGB_565:
		case EUidPixelFormatARGB_8888:
			Mem::FillZ(iChunk.Base(), iParent.GraphicSurfaceSettings().iSurfaceAttributes.iStride * iParent.GraphicSurfaceSettings().iSurfaceAttributes.iSize.iHeight);
			break;
		default:
#ifdef _DEBUG
		    // Panic in a debug build. It will make people think about how the error should be handled.
		    Panic(EUndefinedPixelFormat);
#endif
			break;
			}
		
		// Now, GFX needs to make sure that TSurfaceConfiguration has valid surface id
		// so that IL Client can use RSurfaceManager::SetBackroundSurface()
		User::LeaveIfError(iParent.GraphicSurfaceSettings().iSurfaceConfig.SetSurfaceId(iSurfaceId));
		iParent.iCallbacks.EventNotification(OMX_EventPortSettingsChanged, OMX_NokiaIndexParamGraphicSurfaceConfig, 0, NULL);
		
		chunk.Close();
        #ifdef ILCOMPONENTCONFORMANCE
		iIsBufferSupplier = ETrue;
        #endif
		}
	
	ASSERT(iChunk.Handle());
	

	RSurfaceManager::TInfoBuf surfaceInfoBuf;
	RSurfaceManager::TSurfaceInfoV01& surfaceInfo (surfaceInfoBuf());
	User::LeaveIfError(iParent.SurfaceManager().SurfaceInfo(iSurfaceId, surfaceInfoBuf));
    for (TInt i = 0 ; i < surfaceInfo.iBuffers ; i++)
        {
        TInt offset = 0;
        User::LeaveIfError(iParent.SurfaceManager().GetBufferOffset(iSurfaceId, i, offset));

        if(iBufferIdGenerator == 0)
            {
            iOffsetArray.AppendL(offset);
            }
        }

    aPortSpecificBuffer = iChunk.Base() + iOffsetArray[iBufferIdGenerator];
    aPortPrivate = NULL;
	
	iBufferIdGenerator++;
	}


#ifndef ILCOMPONENTCONFORMANCE
void COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::InitBufferL(OMX_U32 aSizeBytes, OMX_U8* apBuffer, OMX_U32 aBufferCountActual)
{
    // open chunk at the beginning
    if(iChunk.Handle() == NULL)
        {
        // only support chunk extension otherwise error
        if(iSharedChunkThreadId == NULL || iSharedChunkHandleId == NULL)
            {
            DEBUG_PRINTF(_L8("COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::InitBufferL handles not valie"));
            User::Leave(KErrBadHandle);
            }
        
        // race condition on nBufferCountActual
        if(aBufferCountActual != iParent.GraphicSurfaceSettings().iSurfaceAttributes.iBuffers)
            {
            iParent.GraphicSurfaceSettings().iSurfaceAttributes.iBuffers = aBufferCountActual;
            }
        
        DEBUG_PRINTF2(_L8("COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::InitBufferL iSharedChunkThreadId = %Lu"), iSharedChunkThreadId);
        DEBUG_PRINTF2(_L8("COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::InitBufferL iSharedChunkHandleId = %Lu"), iSharedChunkHandleId);

        RThread chunkOwnerThread;
        User::LeaveIfError(chunkOwnerThread.Open(TThreadId(iSharedChunkThreadId)));
        CleanupClosePushL(chunkOwnerThread);
                
        iChunk.SetHandle(iSharedChunkHandleId);
        User::LeaveIfError(iChunk.Duplicate(chunkOwnerThread));
        CleanupStack::PopAndDestroy(&chunkOwnerThread);
        }
    
    // differ creating surface id with chunk at last call
    if(iSurfaceId.IsNull() && (((aBufferCountActual - 1) == iOffsetArray.Count())))
        {
		// Buffer size must be > 0!
		if( aSizeBytes == 0 )
			{
			User::Leave( KErrArgument );
			}

        // Update surface attributes using the buffer size supplied by the client.
		// The supplied buffer size is used to create the graphics surface and must
		// therefore be large enough to accommodate any meta-data too.
        iParent.GraphicSurfaceSettings().iSurfaceAttributes.iOffsetBetweenBuffers = aSizeBytes;
            
        TInt err = KErrGeneral;
        // create surface id with chunk
        err = iParent.SurfaceManager().CreateSurface(iParent.GraphicSurfaceSettings().iSurfaceAttributesBuf, iSurfaceId, iChunk);
        if(err != KErrNone)
            {
            DEBUG_PRINTF2(_L8("COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::InitBufferL createsurface err = %d"), err);
            User::Leave(err);
            }

        // Now, GFX needs to make sure that TSurfaceConfiguration has valid surface id
        err = iParent.GraphicSurfaceSettings().iSurfaceConfig.SetSurfaceId(iSurfaceId);	   
        if(err != KErrNone)
            {
            DEBUG_PRINTF2(_L8("COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::InitBufferL SetSurfaceId err = %d"), err);
            User::Leave(err);
            }
      
        RSurfaceManager::TInfoBuf surfaceInfoBuf;
        RSurfaceManager::TSurfaceInfoV01& surfaceInfo (surfaceInfoBuf());
        surfaceInfo.iSize = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iSize;
        surfaceInfo.iBuffers = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iBuffers;
        surfaceInfo.iPixelFormat = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iPixelFormat;
        surfaceInfo.iStride = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iStride;
        surfaceInfo.iContiguous = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iContiguous;
        surfaceInfo.iCacheAttrib = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iCacheAttrib;
        surfaceInfo.iMappable = iParent.GraphicSurfaceSettings().iSurfaceAttributes.iMappable;
        
        err = iParent.SurfaceManager().SurfaceInfo(iSurfaceId, surfaceInfoBuf);
        if(err != KErrNone)
            {
            DEBUG_PRINTF2(_L8("COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::InitBufferL SurfaceInfo err = %d"), err);  
            User::Leave(err);
            }

        // everything is fine and now ready to rock and roll...
        iParent.iCallbacks.EventNotification(OMX_EventPortSettingsChanged, OMX_NokiaIndexParamGraphicSurfaceConfig, 0, NULL);
        } 
    
    // save offsets
    TInt offset = apBuffer - iChunk.Base();
    iOffsetArray.AppendL(offset);
	}

#else 

void COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::InitBufferL(OMX_U32 /*aSizeBytes*/, OMX_U8* /*apBuffer*/, OMX_U32 aBufferCountActual)
    {
    if(iSurfaceId.IsNull())
        {
        User::LeaveIfError(iParent.SurfaceManager().CreateSurface(iParent.GraphicSurfaceSettings().iSurfaceAttributesBuf, iSurfaceId));
        
        RChunk chunk;
        User::LeaveIfError(iParent.SurfaceManager().MapSurface(iSurfaceId, chunk));
        CleanupClosePushL(chunk);
        
        //We need to change the chunk handle to be shared by the whole process.
        RThread thread;
        CleanupClosePushL(thread);
        iChunk.SetHandle(chunk.Handle());
        User::LeaveIfError(iChunk.Duplicate(thread));
        CleanupStack::PopAndDestroy(2, &chunk);
            
        // Now, GFX needs to make sure that TSurfaceConfiguration has valid surface id
        // so that IL Client can use RSurfaceManager::SetBackroundSurface()
        User::LeaveIfError(iParent.GraphicSurfaceSettings().iSurfaceConfig.SetSurfaceId(iSurfaceId));
        iParent.iCallbacks.EventNotification(OMX_EventPortSettingsChanged, OMX_NokiaIndexParamGraphicSurfaceConfig, 0, NULL);
        
        iIsBufferSupplier = EFalse;
        
        for(TInt i = 0 ; i < aBufferCountActual ; i++)
            {
            TInt offset = 0;
            User::LeaveIfError(iParent.SurfaceManager().GetBufferOffset(iSurfaceId, i, offset));
            iArrayOffsets.AppendL(offset);
            }   
        }

    }
#endif //ILCOMPONENTCONFORMANCE

TInt COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::ProcessNextBuffer()
    {
    __ASSERT_DEBUG(iParent.iBufferMutex.IsHeld(), User::Invariant());
    
    TInt err = KErrNone;
    
    if ((iParent.BuffersToEmpty().Count()>0) && !IsActive() && iParent.State() == OMX_StateExecuting)
        {
        iCurrentBuffer = iParent.BuffersToEmpty()[0];
        TInt bufferId = KErrNotFound;

        if (iCurrentBuffer->nFilledLen == 0)
            {
            // Nothing in the buffer so no need to display it. The buffer might have a flag
            // that needs processing though. Self complete to keep the state machine running.
            iStatus = KRequestPending;
            SetActive();
            TRequestStatus* status = &iStatus;
            User::RequestComplete(status, KErrNone);
            return KErrNone;
            }

#ifdef ILCOMPONENTCONFORMANCE
        if (iIsBufferSupplier)
            {
#endif
			TInt offset = iCurrentBuffer->pBuffer - iChunk.Base();
			bufferId = iOffsetArray.Find(offset);
#ifdef ILCOMPONENTCONFORMANCE
            }
        else
            {
            // Copy data from IL Conformance Suite in to Chunk address at specific address.
            TPtr8 ptr(iChunk.Base(),iCurrentBuffer->nFilledLen ,iCurrentBuffer->nAllocLen);
            ptr.Copy(iCurrentBuffer->pBuffer + iCurrentBuffer->nOffset, iCurrentBuffer->nFilledLen);
            // isn't nOffset likely going to be 0? better to map buffer pointer to buffer id directly
            bufferId = iArrayOffsets.Find(iCurrentBuffer->nOffset);
            // nOffset is not ideal for identifying buffer area since it's likely each buffer header
            // will have a unique pBuffer and nOffset == 0. We could calculate buffer ID by finding
            // the buffer header on the buffer header array in the port, but this isn't how bufferID
            // is calculated in the rest of the code. (we could just store the buffer ID on the
            // pInputPortPrivate but we have a habit of trying to export GS specific info into COmxILMMBuffer)
            __ASSERT_ALWAYS(bufferId == 0, User::Invariant());
            }
#endif  

        if(KErrNotFound == bufferId)
            {
            // An error here means that the buffer will not be displayed and RunL() will not be
            // invoked. However the buffer might have a flag that needs processing. Self complete
            // to keep the state machine running.
            iStatus = KRequestPending;
            SetActive();
            TRequestStatus* status = &iStatus;
            User::RequestComplete(status, KErrNotFound);
            return KErrNotFound;
            }

		// due to COmxMMILBuffer dependency. to be removed 
        if (iIsLocalChunk)
            {
            // copy data into local chunk
            TPtr8 ptr(iCurrentBuffer->pBuffer,iCurrentBuffer->nFilledLen ,iCurrentBuffer->nAllocLen);
            ptr.Copy(iChunk.Base() + offset, iCurrentBuffer->nFilledLen);
            }

        iSurfaceUpdateSession.NotifyWhenDisplayed(iStatus, iTimeStamp);
        SetActive();

        err = iSurfaceUpdateSession.SubmitUpdate(KAllScreens, iSurfaceId, bufferId);
        if(err)
            {
            // An error here means that the buffer will not be displayed and RunL() will not be
            // invoked. However the buffer might have a flag that needs processing. Self complete
            // to keep the state machine running.
            iStatus = KRequestPending;
            SetActive();
            TRequestStatus* status = &iStatus;
            User::RequestComplete(status, err);
            }
        }
    
    return err;
    }

void COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::DoCancel()
	{
    if (iSurfaceUpdateSession.Handle() != KNullHandle)
        {
        iSurfaceUpdateSession.CancelAllUpdateNotifications();
        }
    
	iCurrentBuffer = NULL;
	}

TInt COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::OpenDevice()
    {
    TInt err = iSurfaceUpdateSession.Connect(KSurfaceUpdateNumOfMessageSlots);
    
    if (err == KErrNotFound || err != KErrNone)
        {
#ifdef __WINSCW__
        DEBUG_PRINTF(_L8("Make sure SYMBIAN_GRAPHICS_USE_GCE ON is specified in epoc.ini"));
#else
        DEBUG_PRINTF(_L8("Make sure SYMBIAN_GRAPHICS_USE_GCE is defined in ROM build"));
#endif
        }
    
    return err;
    }

void COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::CloseDevice()
    {
    iSurfaceUpdateSession.CancelAllUpdateNotifications();
    
    if (!iSurfaceId.IsNull())
        {
        iParent.SurfaceManager().CloseSurface(iSurfaceId);  // ignore the error
        }
    
    iSurfaceUpdateSession.Close();
    // RSurface::Open() happened in context of caller.
    iParent.SurfaceManager().Close();

    iOffsetArray.Reset();
    }

TInt COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::Execute()
	{
	iParent.SetState(OMX_StateExecuting);
	iFirstFrameDisplayed = EFalse;
	TInt r = ProcessNextBuffer();
	return r;
	}

TInt COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::Pause()
	{
	iParent.SetState(OMX_StatePause);
	iFirstFrameDisplayed = EFalse;
	Cancel();
	iParent.TransitionToPauseFinished();
	return KErrNone;
	}

TInt COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::Stop()
	{
	if(iParent.State() == OMX_StateExecuting || iParent.State() == OMX_StatePause)
		{
		// Cancel and flush the device driver
		Cancel();
		iParent.SetState(OMX_StateIdle);
		iFirstFrameDisplayed = EFalse;
		}
	return KErrNone;
	}

void COmxILGraphicSinkProcessingFunction::CGraphicSurfaceAccess::ResetSurfaceId()
	{
	iSurfaceId = TSurfaceId::CreateNullId();
	}

COmxILGraphicSinkProcessingFunction::CPFHelper* COmxILGraphicSinkProcessingFunction::CPFHelper::NewL(COmxILGraphicSinkProcessingFunction& aParent, CGraphicSurfaceAccess& aGraphicSurfaceAccess)
	{
	CPFHelper* self = new (ELeave) CPFHelper(aParent, aGraphicSurfaceAccess);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

COmxILGraphicSinkProcessingFunction::CPFHelper::CPFHelper(COmxILGraphicSinkProcessingFunction& aParent, CGraphicSurfaceAccess& aSurfaceAccess)
: CActive(EPriorityUserInput),
  iParent(aParent),
  iGraphicSurfaceAccess(aSurfaceAccess)
	{
	CActiveScheduler::Add(this);
	}
	
void COmxILGraphicSinkProcessingFunction::CPFHelper::ConstructL()
	{
	User::LeaveIfError(iMsgQueue.CreateLocal(KMaxMsgQueueEntries));
	SetActive();
	iMsgQueue.NotifyDataAvailable(iStatus);
	}
	
COmxILGraphicSinkProcessingFunction::CPFHelper::~CPFHelper()
	{
	Cancel(); 
	iMsgQueue.Close();
	}

void COmxILGraphicSinkProcessingFunction::CPFHelper::RunL()
	{
	iParent.iBufferMutex.Wait();
	if (ProcessQueue() != KErrNone)
		{
		iParent.GetCallbacks().ErrorEventNotification(OMX_ErrorInsufficientResources);
		}
	
	// setup for next callbacks		
	SetActive();
	iMsgQueue.NotifyDataAvailable(iStatus);
	iParent.iBufferMutex.Signal();
	}

void COmxILGraphicSinkProcessingFunction::CPFHelper::DoCancel()
	{
	if (iMsgQueue.Handle())
		{
		ProcessQueue();	// Ignore the error?
		iMsgQueue.CancelDataAvailable();
		}
	}

TInt COmxILGraphicSinkProcessingFunction::CPFHelper::ProcessQueue()
	{
	TMessageType msg;
	TInt err = iMsgQueue.Receive(msg);
	while (err == KErrNone)
		{
		switch (msg)
			{
			case EOpenDevice:
			    {
			    err = iGraphicSurfaceAccess.OpenDevice();
			    break;
			    }
			case ECloseDevice:
                {
                iGraphicSurfaceAccess.CloseDevice();
                break;
                }
			
			case EExecuteCommand:
				{
				err = iGraphicSurfaceAccess.Execute();	
				break;
				}				

			case EStopCommand:
				{
				err = iGraphicSurfaceAccess.Stop();	
				break;
				}
				
			case EPauseCommand:
				{
				err = iGraphicSurfaceAccess.Pause();
				break;
				}
				
			case EBufferIndication:
				{
				// Add buffer to list waiting to process.
				// While we could send zero length buffers straight back, this would cause a
				// problem if that buffer has a flag on it that needs processing. So we still
				// pass them on and the graphics surface access code will not display them.
				if (iParent.State() == OMX_StateExecuting || iParent.State() == OMX_StatePause)
					{
					err = iGraphicSurfaceAccess.ProcessNextBuffer();
					}
				break;
				}
			default:
				{
				DEBUG_PRINTF2(_L8("\nMsqQue >> %d"),msg);
				break;
				}					
			}
		
		if (err)
			{
			break;
			}
		
		err = iMsgQueue.Receive(msg);
		}
	
    if ( err  == KErrUnderflow)
        {
        err = KErrNone;
        }
    
	return err;
	}

OMX_ERRORTYPE COmxILGraphicSinkProcessingFunction::CPFHelper::OpenDevice()
    {
    TMessageType message;
    message = EOpenDevice;
    return ConvertError(iMsgQueue.Send(message));
    }

OMX_ERRORTYPE COmxILGraphicSinkProcessingFunction::CPFHelper::CloseDevice()
    {
    TMessageType message;
    message = ECloseDevice;
    return ConvertError(iMsgQueue.Send(message));
    }

OMX_ERRORTYPE COmxILGraphicSinkProcessingFunction::CPFHelper::ExecuteAsync()
	{
	TMessageType message;
	message = EExecuteCommand;
    return ConvertError(iMsgQueue.Send(message));
	}
	
OMX_ERRORTYPE COmxILGraphicSinkProcessingFunction::CPFHelper::StopAsync()
	{
	TMessageType message;
	message = EStopCommand;
    return ConvertError(iMsgQueue.Send(message));
	}

OMX_ERRORTYPE COmxILGraphicSinkProcessingFunction::CPFHelper::StopSync()
	{
	// Cancel to process the existing queue before handling this command
	if (IsActive())
	    {
	    Cancel();
	    }
	
	TInt err = iGraphicSurfaceAccess.Stop();
	
	// setup for next callbacks		
	SetActive();
	iMsgQueue.NotifyDataAvailable(iStatus);
	
    return ConvertError(err);
	}

OMX_ERRORTYPE COmxILGraphicSinkProcessingFunction::CPFHelper::Pause()
    {
    TMessageType message;
    message = EPauseCommand;
    return ConvertError(iMsgQueue.Send(message));
    }
	
OMX_ERRORTYPE COmxILGraphicSinkProcessingFunction::CPFHelper::BufferIndication()
	{
	TMessageType message;
	message = EBufferIndication;
    return ConvertError(iMsgQueue.Send(message));
	}

OMX_ERRORTYPE COmxILGraphicSinkProcessingFunction::CPFHelper::ConvertError(TInt aError)
    {
    if(aError == KErrNone)
        {
        return OMX_ErrorNone;
        }
    else if(aError == KErrOverflow)
        {
        return OMX_ErrorInsufficientResources; 
        }

    // default
    return OMX_ErrorUndefined;
    }
