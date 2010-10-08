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

class COmxIL3GPMuxerVideoInputPort;
class COmxIL3GPMuxerAudioInputPort;

NONSHARABLE_CLASS(COmxIL3GPMuxer) : public COmxILComponent
	{
public:
	enum TPortIndex
		{
		EPortIndexAudioInput = 0,
		EPortIndexVideoInput,
		EPortIndexMax  // Must be last
		};
public:
	static TInt CreateComponent(OMX_HANDLETYPE hComponent);
	~COmxIL3GPMuxer();
	
private:
	COmxIL3GPMuxer();
	void ConstructL(OMX_HANDLETYPE hComponent);

private:
	void AddVideoInputPortL();
	void AddAudioInputPortL();
	};

#endif //COMXIL3GPDEMUXER_H
