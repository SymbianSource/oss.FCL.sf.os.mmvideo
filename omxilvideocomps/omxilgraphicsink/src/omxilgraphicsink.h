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

#ifndef OMXILGRAPHICSINK_H
#define OMXILGRAPHICSINK_H

#include <openmax/il/common/omxilcomponent.h>

class COmxILGraphicSinkVPB0Port;
class COmxILGraphicSinkProcessingFunction;

/**
OpenMAX IL based graphics sink component class
*/
class COmxILGraphicSink : public COmxILComponent
	{
public:
	/** The major version number of component. */
	static const TUint8 iComponentVersionMajor	  = OMX_VERSION_MAJOR;
	/** The minor version number of component. */
	static const TUint8 iComponentVersionMinor	  = OMX_VERSION_MINOR;
	/** The revision version number of component. */
	static const TUint8 iComponentVersionRevision = OMX_VERSION_REVISION;
	/** The step version number of component. */
	static const TUint8 iComponentVersionStep	  = OMX_VERSION_STEP;
	
public:
	static TInt CreateComponent(OMX_HANDLETYPE aComponent);
	~COmxILGraphicSink();

private:
	COmxILGraphicSink();
	void ConstructL(OMX_HANDLETYPE aHandle);
	
	COmxILGraphicSinkVPB0Port* ConstructVPB0PortL();
	void SetPortToPF(COmxILGraphicSinkVPB0Port* aPort);
	};

#endif // OMXILGRAPHICSINK_H
