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

#ifndef TAUDIOFORMAT_H_
#define TAUDIOFORMAT_H_

#include <openmax/il/khronos/v1_x/OMX_Audio.h>

class TAudioFormat
	{

public:

	OMX_AUDIO_CODINGTYPE iCoding;
	OMX_U32 iFramesPerSample;
	OMX_U32 iSampleRate;
	OMX_U32 iAverageBitrate;

	};

#endif /*TAUDIOFORMAT_H_*/
