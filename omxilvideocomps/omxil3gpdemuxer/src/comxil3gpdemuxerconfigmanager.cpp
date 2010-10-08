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

#include "comxil3gpdemuxerprocessingfunction.h"
#include "comxil3gpdemuxerconfigmanager.h"
#include "c3gpdemuxer.h"



COmxIL3GPDemuxerConfigManager* COmxIL3GPDemuxerConfigManager::NewL(
		const TDesC8& aComponentName,
		const OMX_VERSIONTYPE& aComponentVersion,		
		const RPointerArray<TDesC8>& aComponentRoleList,
		COmxIL3GPDemuxerProcessingFunction& aPf)
	{
	COmxIL3GPDemuxerConfigManager* self = new(ELeave) COmxIL3GPDemuxerConfigManager(aPf);
	CleanupStack::PushL(self);
	self->ConstructL(aComponentName, aComponentVersion, aComponentRoleList);
	CleanupStack::Pop(self);
	return self;
	}

COmxIL3GPDemuxerConfigManager::COmxIL3GPDemuxerConfigManager(COmxIL3GPDemuxerProcessingFunction& aPf):
 iSeekMode(OMX_TIME_SeekModeFast),
 iPf(aPf)
	{
	// nothing to do
	}

void COmxIL3GPDemuxerConfigManager::ConstructL(const TDesC8& aComponentName,
                                               const OMX_VERSIONTYPE& aComponentVersion,
                                               const RPointerArray<TDesC8>& aComponentRoleList)
	{
	COmxILConfigManager::ConstructL(aComponentName, aComponentVersion, aComponentRoleList);
	}

COmxIL3GPDemuxerConfigManager::~COmxIL3GPDemuxerConfigManager()
	{
	delete iUri;
	delete iFilename;
	}

OMX_ERRORTYPE COmxIL3GPDemuxerConfigManager::GetParameter(OMX_INDEXTYPE aParamIndex,
                                                          TAny* aComponentParameterStructure) const
	{
	// try the base class first
	OMX_ERRORTYPE error = COmxILConfigManager::GetParameter(aParamIndex, aComponentParameterStructure);
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
		OMX_PARAM_CONTENTURITYPE* contentUri = reinterpret_cast<OMX_PARAM_CONTENTURITYPE*>(aComponentParameterStructure);
		TPtr8 destDes(contentUri->contentURI, contentUri->nSize - _FOFF(OMX_PARAM_CONTENTURITYPE, contentURI));
		// using >= as a byte has to be left for the null terminator
		if(iUri->Length() >= destDes.MaxLength())
			{
			// not enough room in supplied struct to copy URI
			return OMX_ErrorOverflow;	// TODO check return code
			}
		else
			{
			destDes = *iUri;
			destDes.Append('\0');
			return OMX_ErrorNone;
			}
		}
	else if (aParamIndex == OMX_IndexConfigMetadataItem)
		{
		if(!iDemuxer)
			{
			return OMX_ErrorNotReady;	
			}

		OMX_CONFIG_METADATAITEMTYPE* metaData = reinterpret_cast<OMX_CONFIG_METADATAITEMTYPE*>(aComponentParameterStructure);
		OMX_ERRORTYPE omxError = OMX_ErrorNone;
		TRAPD(error, omxError = iDemuxer->GetMetadataL(metaData));
		if (error != KErrNone)
			{
			return OMX_ErrorUndefined;
			}
		else
			{
			return omxError;
			}	
		}
	
	return OMX_ErrorUnsupportedIndex;
	}

OMX_ERRORTYPE COmxIL3GPDemuxerConfigManager::SetParameter(OMX_INDEXTYPE aParamIndex,
                                                          const TAny* aComponentParameterStructure,
                                                          OMX_BOOL aInitTime)
	{
	// try the base class first
	OMX_ERRORTYPE error = COmxILConfigManager::SetParameter(aParamIndex, aComponentParameterStructure, aInitTime);
	if(error != OMX_ErrorUnsupportedIndex)
		{
		return error;
		}
	
	if(aParamIndex == OMX_IndexParamContentURI)
		{
		const OMX_PARAM_CONTENTURITYPE* contentUri = reinterpret_cast<const OMX_PARAM_CONTENTURITYPE*>(aComponentParameterStructure);
		TPtrC8 uriDes(contentUri->contentURI);

		// validate URI by converting to filename
		TUriParser8 parser;
		if(parser.Parse(uriDes) != KErrNone)
			{
			return OMX_ErrorBadParameter;
			}

		HBufC* newFilename = NULL;
		TInt error;
		TRAP(error, newFilename = parser.GetFileNameL());
		if(error == KErrNoMemory)
			{
			return OMX_ErrorInsufficientResources;
			}
		else if(error != KErrNone)
			{
			return OMX_ErrorBadParameter;
			}

		// retain a copy of the original URI for GetParameter
		HBufC8* newUri = HBufC8::New(uriDes.Length());
		if(!newUri)
			{
			delete newFilename;
			return OMX_ErrorInsufficientResources;
			}
		*newUri = uriDes;

		delete iUri;
		iUri = newUri;
		delete iFilename;
		iFilename = newFilename;

		return iPf.ParamIndication(aParamIndex, aComponentParameterStructure);
		}

	return OMX_ErrorUnsupportedIndex;
	}

OMX_ERRORTYPE COmxIL3GPDemuxerConfigManager::GetConfig(OMX_INDEXTYPE aConfigIndex,
                                                       TAny* apComponentConfigStructure) const
	{
	switch (aConfigIndex)
		{
		case OMX_IndexConfigTimePosition:
			{
			if(!iDemuxer)
			    {
			    return OMX_ErrorNotReady;   
			    }
			OMX_TIME_CONFIG_TIMESTAMPTYPE* timestampType = reinterpret_cast<OMX_TIME_CONFIG_TIMESTAMPTYPE*>(apComponentConfigStructure);
			return iDemuxer->GetVideoTimestamp(timestampType->nTimestamp);
			}

		case OMX_IndexConfigTimeSeekMode:
			{
			OMX_TIME_CONFIG_SEEKMODETYPE* seekModeType = reinterpret_cast<OMX_TIME_CONFIG_SEEKMODETYPE*>(apComponentConfigStructure);
			seekModeType->eType = iSeekMode;
			return OMX_ErrorNone;
			}

		default:
			{
			return COmxILConfigManager::GetConfig(aConfigIndex, apComponentConfigStructure);
			}
		}
	}

OMX_ERRORTYPE COmxIL3GPDemuxerConfigManager::SetConfig(OMX_INDEXTYPE aConfigIndex,
                                                       const TAny* apComponentConfigStructure)
	{
	switch (aConfigIndex)
		{
		case OMX_IndexConfigTimePosition:
			{
			const OMX_TIME_CONFIG_TIMESTAMPTYPE* timestampType = reinterpret_cast<const OMX_TIME_CONFIG_TIMESTAMPTYPE*>(apComponentConfigStructure);
			if (OMX_TIME_SeekModeAccurate == iSeekMode)
			    {
			    return OMX_ErrorUnsupportedSetting; //Accurate mode is not supported currently.
			    }
			//Currently only the fast mode is supported so not using iSeekMode as parameter.
			iDemuxer->Seek(timestampType->nTimestamp, OMX_TIME_SeekModeFast);
			return iPf.ConfigIndication(aConfigIndex, apComponentConfigStructure);
			}

		case OMX_IndexConfigTimeSeekMode:
			{
			const OMX_TIME_CONFIG_SEEKMODETYPE* seekModeType = reinterpret_cast<const OMX_TIME_CONFIG_SEEKMODETYPE*>(apComponentConfigStructure);
			iSeekMode = seekModeType->eType;
			return iPf.ConfigIndication(aConfigIndex, apComponentConfigStructure);
			}

		default:
			{
			return COmxILConfigManager::SetConfig(aConfigIndex, apComponentConfigStructure);
			}
		}
	}

const HBufC* COmxIL3GPDemuxerConfigManager::Filename() const
	{
	return iFilename;
	}
	
void COmxIL3GPDemuxerConfigManager::SetDemuxer(C3GPDemuxer& aDemuxer)
	{
	iDemuxer = &aDemuxer;
	}
