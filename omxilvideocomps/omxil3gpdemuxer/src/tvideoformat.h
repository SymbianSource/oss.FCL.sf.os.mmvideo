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

#ifndef TVIDEOFORMAT_H_
#define TVIDEOFORMAT_H_

#include <openmax/il/khronos/v1_x/OMX_Video.h>

class TVideoFormat
	{
public:
	OMX_VIDEO_CODINGTYPE iCoding;
	union
		{
		OMX_VIDEO_H263PROFILETYPE h263;
		OMX_VIDEO_AVCPROFILETYPE avc;
		OMX_VIDEO_MPEG4PROFILETYPE mpeg4;
		} iProfile;

	union
		{
		OMX_VIDEO_H263LEVELTYPE h263;
		OMX_VIDEO_AVCLEVELTYPE avc;
		OMX_VIDEO_MPEG4LEVELTYPE mpeg4;
		} iLevel;
	};

#endif /*TVIDEOFORMAT_H_*/
