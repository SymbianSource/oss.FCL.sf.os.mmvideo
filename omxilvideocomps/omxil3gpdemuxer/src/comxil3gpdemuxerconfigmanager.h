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

#ifndef COMXIL3GPDEMUXERCONFIGMANAGER_H
#define COMXIL3GPDEMUXERCONFIGMANAGER_H

#include <openmax/il/common/omxilconfigmanager.h>

// Forward declarations
class C3GPDemuxer;


NONSHARABLE_CLASS(COmxIL3GPDemuxerConfigManager) : public COmxILConfigManager
	{
public:
	static COmxIL3GPDemuxerConfigManager* NewL(
		const TDesC8& aComponentName,
		const OMX_VERSIONTYPE& aComponentVersion,
		const RPointerArray<TDesC8>& aComponentRoles,
		COmxIL3GPDemuxerProcessingFunction& aPf);
	
	~COmxIL3GPDemuxerConfigManager();

	// from COmxILConfigManager
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex,
	                           TAny* aComponentParameterStructure) const;
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex,
	                           const TAny* aComponentParameterStructure,
	                           OMX_BOOL aInitTime = OMX_TRUE);
	OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE aConfigIndex,
	                        TAny* apComponentConfigStructure) const;
	OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE aConfigIndex,
	                        const TAny* apComponentConfigStructure);

	/** can return NULL if parameter has not been set. */
	const HBufC* Filename() const;	
	void SetDemuxer(C3GPDemuxer& aDemuxer);

private:
	COmxIL3GPDemuxerConfigManager(COmxIL3GPDemuxerProcessingFunction& aPf);
	
	void ConstructL(const TDesC8& aComponentName,
	                const OMX_VERSIONTYPE& aComponentVersion,
	                const RPointerArray<TDesC8>& aComponentRoles);

private:
	HBufC8* iUri;
	HBufC* iFilename;
	C3GPDemuxer* iDemuxer;  // Not owned
	OMX_TIME_SEEKMODETYPE iSeekMode;
	COmxIL3GPDemuxerProcessingFunction& iPf;
	};

#endif //COMXIL3GPDEMUXERCONFIGMANAGER_H
