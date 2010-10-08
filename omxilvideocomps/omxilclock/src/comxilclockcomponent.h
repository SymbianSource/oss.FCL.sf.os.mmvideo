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

#ifndef COMXILCLOCKCOMPONENT_H
#define COMXILCLOCKCOMPONENT_H

#include <openmax/il/common/omxilcomponent.h>



class COmxILClockOutputPort;

class COmxILClockComponent : public COmxILComponent
	{
public:
	static TInt CreateComponent(OMX_HANDLETYPE hComponent);
	~COmxILClockComponent();

	TBool PortEnabled(TInt aPortIndex) const;
	TInt PortBufferCount(TInt aPortIndex) const;

private:
	COmxILClockComponent();
	void ConstructL(OMX_HANDLETYPE hComponent);

private:
	void AddPortL();

private:
	RPointerArray<COmxILClockOutputPort> iPorts;	
	};
	
#endif //COMXILCLOCKOUTPUTPORT_H
