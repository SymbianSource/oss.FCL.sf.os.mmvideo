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

#ifndef COMXILVIDEOSCHEDULEROUTPUTPORT_H_
#define COMXILVIDEOSCHEDULEROUTPUTPORT_H_

#include <openmax/il/common/omxilvideoport.h>



NONSHARABLE_CLASS(COmxILVideoSchedulerOutputPort) : public COmxILVideoPort
	{
public:
	static COmxILVideoSchedulerOutputPort* NewL(const TOmxILCommonPortData& aCommonPortData);

	~COmxILVideoSchedulerOutputPort();

	TUint32 BufferCount() const;
	
	// from COmxILVideoPort	
	OMX_ERRORTYPE GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex, TAny* apComponentParameterStructure) const;
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex, const TAny* apComponentParameterStructure,
															TBool& aUpdateProcessingFunction);
																
protected:
	OMX_ERRORTYPE SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
												TBool& aUpdateProcessingFunction);
	
	TBool IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition) const;

private:
	COmxILVideoSchedulerOutputPort();
	void ConstructL(const TOmxILCommonPortData& aCommonPortData,
	        const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedCodings,
            const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColourFormats);
	
private:
    //Mime Type is not important for uncompressed video but is required for the port definition structure.
	RBuf8 iMimeTypeBuf;
	};

#endif /*COMXILVIDEOSCHEDULEROUTPUTPORT_H_*/
