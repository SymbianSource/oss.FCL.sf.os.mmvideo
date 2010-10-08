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

#include "comxilclockconfigmanager.h"
#include "comxilclockprocessingfunction.h"
#include "clocksupervisor.h"



COmxILClockConfigManager* COmxILClockConfigManager::NewL(
		const TDesC8& aComponentName,
		const OMX_VERSIONTYPE& aComponentVersion,		
		const RPointerArray<TDesC8>& aComponentRoleList,
		COmxILClockProcessingFunction& aProcessingFunction)
	{
	COmxILClockConfigManager* self = new(ELeave) COmxILClockConfigManager(aProcessingFunction);
	CleanupStack::PushL(self);
	self->ConstructL(aComponentName, aComponentVersion, aComponentRoleList);
	CleanupStack::Pop(self);
	return self;
	}

COmxILClockConfigManager::COmxILClockConfigManager(COmxILClockProcessingFunction& aProcessingFunction):
  iProcessingFunction(&aProcessingFunction)
	{
	// nothing to do
	}

void COmxILClockConfigManager::ConstructL(const TDesC8& aComponentName,
                                               const OMX_VERSIONTYPE& aComponentVersion,
                                               const RPointerArray<TDesC8>& aComponentRoleList)
	{
	COmxILConfigManager::ConstructL(aComponentName, aComponentVersion, aComponentRoleList);
	}

COmxILClockConfigManager::~COmxILClockConfigManager()
	{
	}
	
OMX_ERRORTYPE COmxILClockConfigManager::GetConfig(OMX_INDEXTYPE aConfigIndex,
															TAny* apComponentConfigStructure) const
	{
	OMX_ERRORTYPE error = iProcessingFunction->ProduceRequest(aConfigIndex, CClockSupervisor::EGetConfig, apComponentConfigStructure);
	if(error != OMX_ErrorUnsupportedIndex)
		{
		return error;
		}

	// try base class if PF did not support the index
	return COmxILConfigManager::GetConfig(aConfigIndex, apComponentConfigStructure);
	}

OMX_ERRORTYPE COmxILClockConfigManager::SetConfig(OMX_INDEXTYPE aConfigIndex,
																const TAny* apComponentConfigStructure)
	{
	OMX_ERRORTYPE error = iProcessingFunction->ProduceRequest(aConfigIndex, CClockSupervisor::ESetConfig, const_cast<TAny*>(apComponentConfigStructure));
	if(error != OMX_ErrorUnsupportedIndex)
		{
		return error;
		}

	// try base class if PF did not support the index
	return COmxILConfigManager::SetConfig(aConfigIndex, apComponentConfigStructure);
	}

