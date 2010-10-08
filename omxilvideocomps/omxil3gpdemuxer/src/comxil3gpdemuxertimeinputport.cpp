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

#include "comxil3gpdemuxertimeinputport.h"
#include "comxil3gpdemuxerprocessingfunction.h"

COmxIL3GPDemuxerTimeInputPort* COmxIL3GPDemuxerTimeInputPort::NewL(const TOmxILCommonPortData& aCommonPortData, 
													 			   const RArray<OMX_OTHER_FORMATTYPE>& aSupportedOtherFormats,
                                                                   COmxIL3GPDemuxerProcessingFunction& aProcessingFunction)
	{
	COmxIL3GPDemuxerTimeInputPort* tip = new(ELeave) COmxIL3GPDemuxerTimeInputPort(aProcessingFunction);
	CleanupStack::PushL(tip);
	tip->ConstructL(aCommonPortData, aSupportedOtherFormats);
	CleanupStack::Pop(tip);
	return tip;
	}

COmxIL3GPDemuxerTimeInputPort::COmxIL3GPDemuxerTimeInputPort(COmxIL3GPDemuxerProcessingFunction& aProcessingFunction) :
 iProcessingFunction(&aProcessingFunction)
	{
	}

COmxIL3GPDemuxerTimeInputPort::~COmxIL3GPDemuxerTimeInputPort()
	{
	}
	
OMX_ERRORTYPE COmxIL3GPDemuxerTimeInputPort::GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILOtherPort::GetLocalOmxParamIndexes(aIndexArray);
	}

OMX_ERRORTYPE COmxIL3GPDemuxerTimeInputPort::GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILOtherPort::GetLocalOmxConfigIndexes(aIndexArray);
	}

OMX_ERRORTYPE COmxIL3GPDemuxerTimeInputPort::GetParameter(OMX_INDEXTYPE aParamIndex,
                                                          TAny* apComponentParameterStructure) const
	{
	return COmxILOtherPort::GetParameter(aParamIndex, apComponentParameterStructure);
	}

OMX_ERRORTYPE COmxIL3GPDemuxerTimeInputPort::SetParameter(OMX_INDEXTYPE aParamIndex,
                                                          const TAny* apComponentParameterStructure,
                                                          TBool& aUpdateProcessingFunction)
	{
	return COmxILOtherPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);
	}

OMX_ERRORTYPE COmxIL3GPDemuxerTimeInputPort::SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
                                                              TBool& /*aUpdateProcessingFunction*/)
	{
	if (aPortDefinition.nBufferCountActual > KMaxTimeBuffers)
		{
		return OMX_ErrorBadParameter;
		}

	// Just ignore this. It has to be a time format.
	return OMX_ErrorNone;
	}

TBool COmxIL3GPDemuxerTimeInputPort::IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& /*aPortDefinition*/) const
	{
	return ETrue;
	}
