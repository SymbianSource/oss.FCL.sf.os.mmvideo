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

#include <uri8.h>

#include "comxil3gpmuxerconfigmanager.h"
#include "log.h"

COmxIL3GPMuxerConfigManager* COmxIL3GPMuxerConfigManager::NewL(
		const TDesC8& aComponentName,
		const OMX_VERSIONTYPE& aComponentVersion,
		const RPointerArray<TDesC8>& aComponentRoleList)
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerConfigManager::NewL"));
	COmxIL3GPMuxerConfigManager* self = new(ELeave) COmxIL3GPMuxerConfigManager();
	CleanupStack::PushL(self);
	self->ConstructL(aComponentName, aComponentVersion, aComponentRoleList);
	CleanupStack::Pop(self);
	return self;
	}

COmxIL3GPMuxerConfigManager::COmxIL3GPMuxerConfigManager()
	{
	// nothing to do
	}

void COmxIL3GPMuxerConfigManager::ConstructL(const TDesC8& aComponentName,
                                               const OMX_VERSIONTYPE& aComponentVersion,
                                               const RPointerArray<TDesC8>& aComponentRoleList)
	{
	COmxILConfigManager::ConstructL(aComponentName, aComponentVersion, aComponentRoleList);
	}

COmxIL3GPMuxerConfigManager::~COmxIL3GPMuxerConfigManager()
	{
	DEBUG_PRINTF(_L8("COmxIL3GPMuxerConfigManager::~COmxIL3GPMuxerConfigManager"));
	delete iUri;
	delete iFilename;
	}

OMX_ERRORTYPE COmxIL3GPMuxerConfigManager::GetParameter(OMX_INDEXTYPE aParamIndex,
			   TAny* apComponentParameterStructure) const
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerConfigManager::GetParameter : aParamIndex[%u]"), aParamIndex);
 	// try the base class first
	OMX_ERRORTYPE error = COmxILConfigManager::GetParameter(aParamIndex, apComponentParameterStructure);
	if(error != OMX_ErrorUnsupportedIndex)
		{
		return error;
		}
	
	if(aParamIndex == OMX_IndexParamContentURI)
		{
		if(!iUri)
			{
			// content URI has not yet been set
			return OMX_ErrorUnsupportedSetting;	// TODO check return code
			}
		OMX_PARAM_CONTENTURITYPE* apContentURI = reinterpret_cast<OMX_PARAM_CONTENTURITYPE*>(apComponentParameterStructure);
		TPtr8 aDestDes(apContentURI->contentURI, apContentURI->nSize - _FOFF(OMX_PARAM_CONTENTURITYPE, contentURI));
		// using >= as a byte has to be left for the null terminator
		if(iUri->Length() >= aDestDes.MaxLength())
			{
			// not enough room in supplied struct to copy URI
			return OMX_ErrorOverflow;	// TODO check return code
			}
		else
			{
			aDestDes = *iUri;
			aDestDes.Append('\0');
			return OMX_ErrorNone;
			}
		}
	return OMX_ErrorUnsupportedIndex;
	}

OMX_ERRORTYPE COmxIL3GPMuxerConfigManager::SetParameter(OMX_INDEXTYPE aParamIndex,
                                                          const TAny* aComponentParameterStructure,
                                                          OMX_BOOL aInitTime)
	{
	DEBUG_PRINTF2(_L8("COmxIL3GPMuxerConfigManager::SetParameter : aParamIndex[%u]"), aParamIndex);
	// try the base class first
	OMX_ERRORTYPE error = COmxILConfigManager::SetParameter(aParamIndex, aComponentParameterStructure, aInitTime);
	if(error != OMX_ErrorUnsupportedIndex)
		{
		return error;
		}
	
	if(aParamIndex == OMX_IndexParamContentURI)
		{
		const OMX_PARAM_CONTENTURITYPE* contentUri = reinterpret_cast<const OMX_PARAM_CONTENTURITYPE*>(aComponentParameterStructure);
		TPtrC8 aUriDes(contentUri->contentURI);

		// validate URI by converting to filename
		TUriParser8 parser;
		if(parser.Parse(aUriDes) != KErrNone)
			{
			return OMX_ErrorBadParameter;
			}

		HBufC* newFilename = NULL;
		TInt error;
		TRAP(error, newFilename = parser.GetFileNameL());
		if(error != KErrNone)
			{
			return OMX_ErrorBadParameter;
			}

		// retain a copy of the original URI for GetParameter
		HBufC8* newUri = HBufC8::New(aUriDes.Length());
		if(!newUri)
			{
			delete newFilename;
			return OMX_ErrorInsufficientResources;
			}
		*newUri = aUriDes;

		delete iUri;
		iUri = newUri;
		delete iFilename;
		iFilename = newFilename;

		return OMX_ErrorNone;
		}

	return OMX_ErrorUnsupportedIndex;
	}

const HBufC* COmxIL3GPMuxerConfigManager::Filename() const
	{
	return iFilename;
	}
