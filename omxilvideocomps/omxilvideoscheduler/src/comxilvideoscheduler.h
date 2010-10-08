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

#ifndef COMXILVIDEOSCHEDULER_H_
#define COMXILVIDEOSCHEDULER_H_

#include <openmax/il/common/omxilcomponent.h>

class COmxILVideoSchedulerInputPort;
class COmxILVideoSchedulerOutputPort;
class COmxILClientClockPort;


NONSHARABLE_CLASS(COmxILVideoScheduler) : public COmxILComponent
	{
public:
	static TInt CreateComponent(OMX_HANDLETYPE hComponent);
	~COmxILVideoScheduler();
	
	TUint32 BufferCount() const;	
	OMX_ERRORTYPE MediaTimeRequest(TAny* apPrivate, OMX_TICKS aMediaTime, OMX_TICKS aOffset);		
	OMX_ERRORTYPE GetWallTime(OMX_TICKS& aWallTime);			
	OMX_ERRORTYPE SetVideoStartTime(OMX_TICKS aStartTime);
	
private:
	COmxILVideoScheduler();
	void ConstructL(OMX_HANDLETYPE aComponent);
	
	void AddOutputVideoPortL();
	void AddInputVideoPortL();
	void AddClockPortL();
	
private:	
	COmxILVideoSchedulerOutputPort* iVideoOutputPort; //not owned	
	COmxILVideoSchedulerInputPort* iVideoInputPort;//not owned
	COmxILClientClockPort* iClockPort;	//not owned
	};
	
#endif /*COMXILVIDEOSCHEDULER_H_*/
