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

#ifndef COMXIL3GPDEMUXERTIMEINPUTPORT_H
#define COMXIL3GPDEMUXERTIMEINPUTPORT_H

#include <openmax/il/common/omxilotherport.h>

class COmxIL3GPDemuxerProcessingFunction;

const TInt KMaxTimeBuffers = 4;

NONSHARABLE_CLASS(COmxIL3GPDemuxerTimeInputPort) : public COmxILOtherPort
	{
public:
	static COmxIL3GPDemuxerTimeInputPort* NewL(const TOmxILCommonPortData& aCommonPortData,
											   const RArray<OMX_OTHER_FORMATTYPE>& aSupportedOtherFormats,
	                                           COmxIL3GPDemuxerProcessingFunction& aProcessingFunction);
	~COmxIL3GPDemuxerTimeInputPort();

	// from COmxILOtherPort
	OMX_ERRORTYPE GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex,
	                           TAny* apComponentParameterStructure) const;
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex,
	                           const TAny* apComponentParameterStructure,
	                           TBool& aUpdateProcessingFunction);

protected:
	// from COmxILOtherPort
	OMX_ERRORTYPE SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
	                               TBool& aUpdateProcessingFunction);
	TBool IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition) const;

private:
	COmxIL3GPDemuxerTimeInputPort(COmxIL3GPDemuxerProcessingFunction& aProcessingFunction);

private:
	COmxIL3GPDemuxerProcessingFunction* iProcessingFunction;	// not owned
	};

#endif //COMXIL3GPDEMUXERTIMEINPUTPORT_H
