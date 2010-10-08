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

#ifndef COMXILCLOCKCONFIGMANAGER_H
#define COMXILCLOCKCONFIGMANAGER_H

#include <openmax/il/common/omxilconfigmanager.h>



// Forward declarations
class COmxILClockProcessingFunction;

class COmxILClockConfigManager : public COmxILConfigManager
	{
public:
	static COmxILClockConfigManager* NewL(
		const TDesC8& aComponentName,
		const OMX_VERSIONTYPE& aComponentVersion,
		const RPointerArray<TDesC8>& aComponentRoles,
		COmxILClockProcessingFunction& aProcessingFunction);
	
	~COmxILClockConfigManager();

	// from COmxILConfigManager
	OMX_ERRORTYPE GetConfig(OMX_INDEXTYPE aConfigIndex,
								TAny* apComponentConfigStructure) const;
	OMX_ERRORTYPE SetConfig(OMX_INDEXTYPE aConfigIndex,
								const TAny* apComponentConfigStructure);

private:
	COmxILClockConfigManager(COmxILClockProcessingFunction& aProcessingFunction);
	
	void ConstructL(const TDesC8& aComponentName,
	                const OMX_VERSIONTYPE& aComponentVersion,
	                const RPointerArray<TDesC8>& aComponentRoles);

private:
	COmxILClockProcessingFunction* iProcessingFunction;	// not owned
	};

#endif //COMXILCLOCKCONFIGMANAGER_H
