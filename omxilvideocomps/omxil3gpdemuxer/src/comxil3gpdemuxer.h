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

#ifndef COMXIL3GPDEMUXER_H
#define COMXIL3GPDEMUXER_H

#include <openmax/il/common/omxilcomponent.h>

class COmxIL3GPDemuxerTimeInputPort;
class COmxIL3GPDemuxerAudioOutputPort;
class COmxIL3GPDemuxerVideoOutputPort;

NONSHARABLE_CLASS(COmxIL3GPDemuxer) : public COmxILComponent
	{
public:
	enum TPortIndex
		{
		EPortIndexAudioOutput,
		EPortIndexVideoOutput,
		EPortIndexTimeInput,   // time port currently not used
		EPortIndexMax    // Must be last
		};

public:
	static TInt CreateComponent(OMX_HANDLETYPE hComponent);
	~COmxIL3GPDemuxer();

private:
	COmxIL3GPDemuxer();
	void ConstructL(OMX_HANDLETYPE hComponent);

private:
	void AddTimeInputPortL();
	void AddAudioOutputPortL();
	void AddVideoOutputPortL();
	};
	
#endif //COMXIL3GPDEMUXER_H
