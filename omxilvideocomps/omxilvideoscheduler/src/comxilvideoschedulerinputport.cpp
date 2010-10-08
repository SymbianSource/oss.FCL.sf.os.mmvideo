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

#include "comxilvideoschedulerinputport.h"
#include "omxilvideoschedulerextensionsindexes.h"


COmxILVideoSchedulerInputPort* COmxILVideoSchedulerInputPort::NewL(const TOmxILCommonPortData& aCommonPortData)
	{
	// TODO these arrays must left empty, to be removed from the video port constructor
	RArray<OMX_VIDEO_CODINGTYPE> supportedCodings;
	RArray<OMX_COLOR_FORMATTYPE> supportedColorFormats;
	
	CleanupClosePushL(supportedCodings);
	CleanupClosePushL(supportedColorFormats);

	COmxILVideoSchedulerInputPort* self = new(ELeave) COmxILVideoSchedulerInputPort();
	CleanupStack::PushL(self);
	self->ConstructL(aCommonPortData, supportedCodings, supportedColorFormats);
	CleanupStack::Pop(self);
	
	CleanupStack::PopAndDestroy(2, &supportedCodings);
	
	return self;
	}


COmxILVideoSchedulerInputPort::COmxILVideoSchedulerInputPort()
    {
    }

void COmxILVideoSchedulerInputPort::ConstructL(const TOmxILCommonPortData& aCommonPortData,
                                    const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedCodings,
                                    const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColourFormats)
	{
	COmxILVideoPort::ConstructL(aCommonPortData, aSupportedCodings, aSupportedColourFormats);
	// Port definition mime type. Mime type is not relevant for uncompressed video frames
 	iMimeTypeBuf.CreateL(KNullDesC8(), KNullDesC8().Length() + 1);
 	TUint8* pTUint = const_cast<TUint8*>(iMimeTypeBuf.PtrZ());
 	GetParamPortDefinition().format.video.cMIMEType = reinterpret_cast<OMX_STRING>(pTUint);

 	iParamVideoScheDropFrame.nSize = sizeof(OMX_NOKIA_PARAM_DROPPEDFRAMEEVENT);
 	iParamVideoScheDropFrame.nVersion = TOmxILSpecVersion();
 	iParamVideoScheDropFrame.nPortIndex = GetParamPortDefinition().nPortIndex;
 	iParamVideoScheDropFrame.bEnabled = OMX_FALSE;

	GetSupportedVideoFormats().AppendL(OMX_VIDEO_CodingUnused);
	GetSupportedColorFormats().AppendL(OMX_COLOR_FormatUnused);	 
	}

COmxILVideoSchedulerInputPort::~COmxILVideoSchedulerInputPort()
	{
	iMimeTypeBuf.Close();
	}

OMX_ERRORTYPE COmxILVideoSchedulerInputPort::GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const
	{
	OMX_ERRORTYPE omxRetValue = COmxILVideoPort::GetLocalOmxParamIndexes(aIndexArray);
	if(omxRetValue != OMX_ErrorNone)
	    {
	    return omxRetValue; 
	    }

	TInt err = aIndexArray.InsertInOrder(OMX_NokiaIndexParamDroppedFrameEvent);

	if (err != KErrNone && err != KErrAlreadyExists)
	    {
	    return OMX_ErrorInsufficientResources;
	    }

	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxILVideoSchedulerInputPort::GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILVideoPort::GetLocalOmxConfigIndexes(aIndexArray);
	}

/**
This method provides the current values for the parameters present in the structure represented by the given index.

@param  aParamIndex The specific param index for which the current parameter values are required.
        apComponentParameterStructure The pointer to the structure which will be updated to provide the current parameter values.
        
@return OMX_ErrorNone if successful;
        OMX_ErrorNoMore if no more formats;
        OMX_ErrorUnsupportedSetting if unsupported setting is passed;
*/
OMX_ERRORTYPE COmxILVideoSchedulerInputPort::GetParameter(OMX_INDEXTYPE aParamIndex,TAny* apComponentParameterStructure) const
    {
    if(aParamIndex == OMX_NokiaIndexParamDroppedFrameEvent)
        {
        OMX_NOKIA_PARAM_DROPPEDFRAMEEVENT* dropFrame = static_cast<OMX_NOKIA_PARAM_DROPPEDFRAMEEVENT*>(apComponentParameterStructure);
        *dropFrame = iParamVideoScheDropFrame;
        return OMX_ErrorNone;
        }
    else
        {
         // Try the parent's indexes
        return COmxILVideoPort::GetParameter(aParamIndex,apComponentParameterStructure);
        }
    }

/**
This method sets the values of the parameters present in the structure represented by the given index.

@param  aParamIndex The specific param index for which the parameter values have to be set.
        apComponentParameterStructure The pointer to the structure which will provide the desired parameter values to be set.
        aUpdateProcessingFunction informs whether the processing function needs to be updated.
        
@return OMX_ErrorNone if successful;
        OMX_ErrorUnsupportedSetting if non-zero framerate value;
        OMX_ErrorUnsupportedIndex if request OMX_NokiaIndexParamDroppedFrameEvent index;
*/
OMX_ERRORTYPE COmxILVideoSchedulerInputPort::SetParameter(OMX_INDEXTYPE aParamIndex,const TAny* apComponentParameterStructure, TBool& aUpdateProcessingFunction)
	{    
	if(aParamIndex == OMX_NokiaIndexParamDroppedFrameEvent)
		{
		const OMX_NOKIA_PARAM_DROPPEDFRAMEEVENT *portParameterStructure = static_cast<const OMX_NOKIA_PARAM_DROPPEDFRAMEEVENT*>(apComponentParameterStructure);
		if (iParamVideoScheDropFrame.bEnabled != portParameterStructure->bEnabled)
			{
			iParamVideoScheDropFrame.bEnabled = portParameterStructure->bEnabled;
			aUpdateProcessingFunction = ETrue;
			}
		return OMX_ErrorNone;
		}

	// Try the parent's indexes
	return COmxILVideoPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);
	}


OMX_ERRORTYPE COmxILVideoSchedulerInputPort::SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
													TBool& /*aUpdateProcessingFunction*/)
	{
	GetParamPortDefinition().format.video = aPortDefinition.format.video;
 	TUint8* pTUint = const_cast<TUint8*>(iMimeTypeBuf.PtrZ());
 	GetParamPortDefinition().format.video.cMIMEType = reinterpret_cast<OMX_STRING>(pTUint);
 	GetParamPortDefinition().format.video.nSliceHeight = GetParamPortDefinition().format.video.nFrameHeight;
 	GetParamPortDefinition().nBufferSize = GetParamPortDefinition().format.video.nStride * GetParamPortDefinition().format.video.nSliceHeight;
	return OMX_ErrorNone;
	}

TBool COmxILVideoSchedulerInputPort::IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& /*aPortDefinition*/) const
	{
	// TODO (The base class should do this checking) 
	return ETrue;	
	}

/** Returns the number of buffers configured on this port. */
TUint32 COmxILVideoSchedulerInputPort::BufferCount() const
	{
	return GetParamPortDefinition().nBufferCountActual;
	}

/**
This method provides the index type represented by the given parameter name.

@param  aParameterName The name of extention parameter to be retrieved.
        apIndexType The pointer which will retrieve the required index.
        
@return OMX_ErrorNone if successful;
        OMX_ErrorUnsupportedIndex if unsupported parameter name is passed;
*/  
OMX_ERRORTYPE COmxILVideoSchedulerInputPort::GetExtensionIndex(OMX_STRING aParameterName,   OMX_INDEXTYPE* apIndexType) const
    {
    TPtrC8 receivedParameterName(const_cast<const TUint8*>(reinterpret_cast<TUint8*>(aParameterName)));

    TPtrC8 parameterName(reinterpret_cast<const TUint8*>(sOmxNokiaIndexParamDroppedFrameEvent));
    
    if (receivedParameterName == parameterName)
        {
        *apIndexType = static_cast<OMX_INDEXTYPE>(OMX_NokiaIndexParamDroppedFrameEvent);
        return OMX_ErrorNone;
        }
    
    *apIndexType = OMX_IndexMax;
    return OMX_ErrorUnsupportedIndex;
    }

