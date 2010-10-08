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

#ifndef COMXILVIDEOSCHEDULERINPUTPORT_H_
#define COMXILVIDEOSCHEDULERINPUTPORT_H_

#include <openmax/il/common/omxilvideoport.h>
#include <openmax/il/extensions/omxildroppedframeeventextension.h>

NONSHARABLE_CLASS(COmxILVideoSchedulerInputPort) : public COmxILVideoPort
	{
public:
	static COmxILVideoSchedulerInputPort* NewL(const TOmxILCommonPortData& aCommonPortData);

	~COmxILVideoSchedulerInputPort();

	TUint32 BufferCount() const;
	
	// from COmxILVideoPort	
	OMX_ERRORTYPE GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex, TAny* apComponentParameterStructure) const;
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex, const TAny* apComponentParameterStructure,
															TBool& aUpdateProcessingFunction);
	OMX_ERRORTYPE GetExtensionIndex(OMX_STRING aParameterName, OMX_INDEXTYPE* apIndexType) const;
protected:
	OMX_ERRORTYPE SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
												TBool& aUpdateProcessingFunction);
	
	TBool IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition) const;
	
private:
    COmxILVideoSchedulerInputPort();

	void ConstructL(const TOmxILCommonPortData& aCommonPortData,
	        const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedCodings,
            const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColourFormats);
		
private:
	RBuf8 iMimeTypeBuf;
	// Extension to provide a structure for generating the video scheduler dropping frame error
	OMX_NOKIA_PARAM_DROPPEDFRAMEEVENT iParamVideoScheDropFrame;
	};

#endif /*COMXILVIDEOSCHEDULERINPUTPORT_H_*/
