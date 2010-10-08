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

#ifndef COMXIL3GPMUXERCONFIGMANAGER_H
#define COMXIL3GPMUXERCONFIGMANAGER_H

#include <openmax/il/common/omxilconfigmanager.h>

NONSHARABLE_CLASS(COmxIL3GPMuxerConfigManager) : public COmxILConfigManager
	{
public:
	static COmxIL3GPMuxerConfigManager* NewL(
		const TDesC8& aComponentName,
		const OMX_VERSIONTYPE& aComponentVersion,
		const RPointerArray<TDesC8>& aComponentRoles);
		
	~COmxIL3GPMuxerConfigManager();
	
	OMX_ERRORTYPE GetParameter(OMX_INDEXTYPE aParamIndex,
			   TAny* apComponentParameterStructure) const;
	
	OMX_ERRORTYPE SetParameter(OMX_INDEXTYPE aParamIndex,
	                           const TAny* aComponentParameterStructure,
	                           OMX_BOOL aInitTime = OMX_TRUE);
	
	/** can return NULL if parameter has not been set. */
	const HBufC* Filename() const;
	
private:
	COmxIL3GPMuxerConfigManager();

	void ConstructL(const TDesC8& aComponentName,
	                const OMX_VERSIONTYPE& aComponentVersion,
	                const RPointerArray<TDesC8>& aComponentRoles);	

private:
	HBufC8* iUri;
	HBufC* iFilename;
	};

#endif //COMXIL3GPMUXERCONFIGMANAGER_H
