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

#ifndef COMXILCLOCKOUTPUTPORT_H
#define COMXILCLOCKOUTPUTPORT_H

#include <openmax/il/common/omxilotherport.h>



class COmxILClockProcessingFunction;

class COmxILClockOutputPort : public COmxILOtherPort
	{
public:
	static COmxILClockOutputPort* NewL(const TOmxILCommonPortData& aCommonPortData, const RArray<OMX_OTHER_FORMATTYPE>& aSupportedFormats, COmxILClockProcessingFunction& aProcessingFunction);
	~COmxILClockOutputPort();

	TInt BufferCount() const;

	// from COmxILOtherPort
	OMX_ERRORTYPE GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex,
	                           TAny* apComponentParameterStructure) const;
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex,
	                           const TAny* apComponentParameterStructure,
	                           TBool& aUpdateProcessingFunction);
	OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE aConfigIndex,
                          TAny* apComponentConfigStructure) const;
	OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE aConfigIndex,
                          const TAny* apComponentConfigStructure,
                          TBool& aUpdateProcessingFunction);

protected:
	OMX_ERRORTYPE SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
	                               TBool& aUpdateProcessingFunction);
	TBool IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition) const;
	
private:
	COmxILClockOutputPort(COmxILClockProcessingFunction& aProcessingFunction);
	void ConstructL(const TOmxILCommonPortData& aCommonPortData, const RArray<OMX_OTHER_FORMATTYPE>& aSupportedFormats);
	
private:
	COmxILClockProcessingFunction* iProcessingFunction;	// not owned
	};

#endif //COMXILCLOCKOUTPUTPORT_H
