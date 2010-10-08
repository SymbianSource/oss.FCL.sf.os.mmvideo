/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef OMXILGRAPHICSINKTRACE_H__
#define OMXILGRAPHICSINKTRACE_H__

#ifdef SYMBIAN_MULTIMEDIA_TURN_TIMING_TRACING_ON

#include <mmvideoperformance.h>

/*
 * Simple logging macro
 * 
 * identifier - select from enum TTestTraceIdentifier
 * 
 * */
#define GRAPHICSINK_PERFORMANCE_TIMESTAMP_TRACE(identifier) TUTrace::PrintfPrimary(KTestTraceFilterId, 0, 0, "VPT %x", identifier)

#else

#define GRAPHICSINK_PERFORMANCE_TIMESTAMP_TRACE(identifier)

#endif

#endif // OMXILGRAPHICSINKTRACE_H__
