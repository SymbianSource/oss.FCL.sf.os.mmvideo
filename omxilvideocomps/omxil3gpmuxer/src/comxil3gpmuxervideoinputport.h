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

#ifndef COMXIL3GPMUXERVIDEOINPUTPORT_H
#define COMXIL3GPMUXERVIDEOINPUTPORT_H

#include <openmax/il/common/omxilvideoport.h>

class COmxIL3GPMuxerProcessingFunction;

const TInt KMaxVideoBuffers = 20;

NONSHARABLE_CLASS(COmxIL3GPMuxerVideoInputPort) : public COmxILVideoPort
	{
public:
	static COmxIL3GPMuxerVideoInputPort* NewL(const TOmxILCommonPortData& aCommonPortData);
	~COmxIL3GPMuxerVideoInputPort();
	
	void GetVideoProperties(OMX_U32& aFrameWidth, OMX_U32& aFrameHeight, OMX_U32& aFramerate, OMX_U32& aBitRate);
	
	// from COmxILVideoPort
	OMX_ERRORTYPE GetLocalOmxParamIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetLocalOmxConfigIndexes(RArray<TUint>& aIndexArray) const;
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex,
	                           TAny* apComponentParameterStructure) const;
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex,
	                           const TAny* apComponentParameterStructure,
	                           TBool& aUpdateProcessingFunction);

protected:
	OMX_ERRORTYPE SetFormatInPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition,
	                               TBool& aUpdateProcessingFunction);
	TBool IsTunnelledPortCompatible(const OMX_PARAM_PORTDEFINITIONTYPE& aPortDefinition) const;

private:
	COmxIL3GPMuxerVideoInputPort();
	void ConstructL(const TOmxILCommonPortData& aCommonPortData,
	        const RArray<OMX_VIDEO_CODINGTYPE>& aSupportedCodings,
            const RArray<OMX_COLOR_FORMATTYPE>& aSupportedColourFormats);
	};

#endif //COMXIL3GPMUXERVIDEOINPUTPORT_H
