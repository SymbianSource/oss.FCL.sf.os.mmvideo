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

#include "comxilclockoutputport.h"
#include "comxilclockprocessingfunction.h"
#include "clockpanics.h"

COmxILClockOutputPort* COmxILClockOutputPort::NewL(const TOmxILCommonPortData& aCommonPortData, const RArray<OMX_OTHER_FORMATTYPE>& aSupportedFormats, COmxILClockProcessingFunction& aProcessingFunction)
	{
	COmxILClockOutputPort* self = new(ELeave) COmxILClockOutputPort(aProcessingFunction);
	CleanupStack::PushL(self);
	self->ConstructL(aCommonPortData, aSupportedFormats);
	CleanupStack::Pop(self);
	return self;
	}

COmxILClockOutputPort::COmxILClockOutputPort(COmxILClockProcessingFunction& aProcessingFunction) :
 iProcessingFunction(&aProcessingFunction)
	{
	}

void COmxILClockOutputPort::ConstructL(const TOmxILCommonPortData& aCommonPortData, const RArray<OMX_OTHER_FORMATTYPE>& aSupportedFormats)
	{
	COmxILOtherPort::ConstructL(aCommonPortData, aSupportedFormats);
	}

COmxILClockOutputPort::~COmxILClockOutputPort()
	{
	}
	
OMX_ERRORTYPE COmxILClockOutputPort::GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const
	{
	return COmxILOtherPort::GetLocalOmxParamIndexes(aIndexArray);
	}

OMX_ERRORTYPE COmxILClockOutputPort::GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const
	{
	OMX_ERRORTYPE omxRetValue = COmxILOtherPort::GetLocalOmxConfigIndexes(aIndexArray);
	if (omxRetValue != OMX_ErrorNone)
		{
		return omxRetValue;
		}

	TInt err = aIndexArray.InsertInOrder(OMX_IndexConfigTimeClientStartTime);
	// Note that index duplication is OK.
	if (err == KErrNone || err == KErrAlreadyExists)
		{
		err = aIndexArray.InsertInOrder(OMX_IndexConfigTimeMediaTimeRequest);
		}
				
	if (err != KErrNone && err != KErrAlreadyExists)
		{
		return OMX_ErrorInsufficientResources;
		}

	return OMX_ErrorNone;
	}

OMX_ERRORTYPE COmxILClockOutputPort::GetParameter(OMX_INDEXTYPE aParamIndex,
                                                            TAny* apComponentParameterStructure) const
	{
	return COmxILOtherPort::GetParameter(aParamIndex, apComponentParameterStructure);
	}

OMX_ERRORTYPE COmxILClockOutputPort::SetParameter(OMX_INDEXTYPE aParamIndex,
                                                            const TAny* apComponentParameterStructure,
                                                            TBool& aUpdateProcessingFunction)
	{
	return COmxILOtherPort::SetParameter(aParamIndex, apComponentParameterStructure, aUpdateProcessingFunction);
	}

OMX_ERRORTYPE COmxILClockOutputPort::GetConfig(OMX_INDEXTYPE aConfigIndex,
                                                         TAny* apComponentConfigStructure) const
	{
	return COmxILOtherPort::GetConfig(aConfigIndex, apComponentConfigStructure);
	}

OMX_ERRORTYPE COmxILClockOutputPort::SetConfig(OMX_INDEXTYPE aConfigIndex,
                                                         const TAny* apComponentConfigStructure,
                                                         TBool& aUpdateProcessingFunction)
	{
	OMX_ERRORTYPE error = iProcessingFunction->ProduceRequest(aConfigIndex, CClockSupervisor::ESetConfig, const_cast<TAny*>(apComponentConfigStructure));
	if(error != OMX_ErrorUnsupportedIndex)
		{
		return error;
		}

	// try base class if PF did not support the index
	return COmxILOtherPort::SetConfig(aConfigIndex, apComponentConfigStructure, aUpdateProcessingFunction);
	}

OMX_ERRORTYPE COmxILClockOutputPort::SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& /*aPortDefinition*/,
                                                                TBool& /*aUpdateProcessingFunction*/)
	{
	return OMX_ErrorNone;
	}

TBool COmxILClockOutputPort::IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& /*aPortDefinition*/) const
	{
#ifdef _DEBUG
	// This function only gets called on input ports, but must be implemented because it is pure virtual.
	// Panic if this is ever called.
	Panic(ECompatibilityCheckOnOutput);
#endif
	return ETrue;
	}

/**
 * Returns the number of buffers configured in the port definition.
 */
TInt COmxILClockOutputPort::BufferCount() const
	{
	return GetParamPortDefinition().nBufferCountActual;
	}
