/*
* Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
 * @file
 * @internalTechnology
 */

#ifndef GRAPHICSINKTESTSTEPS_H
#define GRAPHICSINKTESTSTEPS_H

#include "graphicsinktestbase.h"

// Test step names

_LIT(KOmxGsTest0001Step01, "MMVIDEO-OMX-GS-001-01-HP");
_LIT(KOmxGsTest0001Step02, "MMVIDEO-OMX-GS-001-02-HP");
_LIT(KOmxGsTest0001Step03,	"MMVIDEO-OMX-GS-001-03-HP");
_LIT(KOmxGsTest0001Step04,	"MMVIDEO-OMX-GS-001-04-HP");
_LIT(KOmxGsTest0001Step05,	"MMVIDEO-OMX-GS-001-05-HP");
_LIT(KOmxGsTest0001Step06,	"MMVIDEO-OMX-GS-001-06-HP");
_LIT(KOmxGsTest0001Step07,	"MMVIDEO-OMX-GS-001-07-HP");
_LIT(KOmxGsTest0001Step08,	"MMVIDEO-OMX-GS-001-08-HP");
_LIT(KOmxGsTest0001Step09,	"MMVIDEO-OMX-GS-001-09-HP");
_LIT(KOmxGsTest0001Step00,	"MMVIDEO-OMX-GS-001-00-HP");

_LIT(KOmxGsTest0002Step01,	"MMVIDEO-OMX-GS-002-01-HP");
_LIT(KOmxGsTest0002Step02,	"MMVIDEO-OMX-GS-002-02-HP");
_LIT(KOmxGsTest0002Step03,	"MMVIDEO-OMX-GS-002-03-HP");
_LIT(KOmxGsTest0002Step04,	"MMVIDEO-OMX-GS-002-04-HP");

_LIT(KOmxGsTest0003Step01,	"MMVIDEO-OMX-GS-003-01-HP");
_LIT(KOmxGsTest0003Step02,	"MMVIDEO-OMX-GS-003-02-HP");
_LIT(KOmxGsTest0003Step03,	"MMVIDEO-OMX-GS-003-03-HP");
_LIT(KOmxGsTest0003Step04,	"MMVIDEO-OMX-GS-003-04-HP");

_LIT(KOmxGsTest0004Step01,	"MMVIDEO-OMX-GS-004-01-HP");
_LIT(KOmxGsTest0004Step02,	"MMVIDEO-OMX-GS-004-02-HP");
_LIT(KOmxGsTest0004Step03,	"MMVIDEO-OMX-GS-004-03-HP");
_LIT(KOmxGsTest0004Step04,	"MMVIDEO-OMX-GS-004-04-HP");
_LIT(KOmxGsTest0004Step05,	"MMVIDEO-OMX-GS-004-05-HP");
_LIT(KOmxGsTest0004Step06,	"MMVIDEO-OMX-GS-004-06-HP");

_LIT(KOmxGsTest0005Step01,	"MMVIDEO-OMX-GS-005-01-HP");
_LIT(KOmxGsTest0005Step02,	"MMVIDEO-OMX-GS-005-02-HP");
_LIT(KOmxGsTest0005Step03,	"MMVIDEO-OMX-GS-005-03-HP");
_LIT(KOmxGsTest0005Step04,	"MMVIDEO-OMX-GS-005-04-HP");
_LIT(KOmxGsTest0005Step05,	"MMVIDEO-OMX-GS-005-05-HP");

_LIT(KOmxGsTest0006Step01,	"MMVIDEO-OMX-GS-006-01-HP");
_LIT(KOmxGsTest0006Step02,	"MMVIDEO-OMX-GS-006-02-HP");

_LIT(KOmxGsTest0007Step01,	"MMVIDEO-OMX-GS-007-01-HP");
_LIT(KOmxGsTest0007Step02,	"MMVIDEO-OMX-GS-007-02-HP");
_LIT(KOmxGsTest0007Step03,	"MMVIDEO-OMX-GS-007-03-HP");
_LIT(KOmxGsTest0007Step04,	"MMVIDEO-OMX-GS-007-04-HP");

_LIT(KOmxGsTest0008Step01,	"MMVIDEO-OMX-GS-008-01-HP");

_LIT(KOmxGsTest0009Step00,	"MMVIDEO-OMX-GS-009-00-HP");
_LIT(KOmxGsTest0010Step00,	"MMVIDEO-OMX-GS-010-00-HP");

// Test step declarations, each step is contained within a seperate class
// Macro to save re-defining similar classes

#define NEW_GS_CLASS(TEST_NUM) \
	class C##TEST_NUM \
		: public CGraphicsSinkTestBase \
		{ \
	public: \
		C##TEST_NUM(); \
		~C##TEST_NUM(); \
		TVerdict doTestStepL(); \
	private: \
		}; 


// Now use the macro to declare all the test steps

NEW_GS_CLASS(OmxGsTest0001Step00)
NEW_GS_CLASS(OmxGsTest0001Step01)
NEW_GS_CLASS(OmxGsTest0001Step02)
NEW_GS_CLASS(OmxGsTest0001Step03)
NEW_GS_CLASS(OmxGsTest0001Step04)
NEW_GS_CLASS(OmxGsTest0001Step05)
NEW_GS_CLASS(OmxGsTest0001Step06)
NEW_GS_CLASS(OmxGsTest0001Step07)
NEW_GS_CLASS(OmxGsTest0001Step08)
NEW_GS_CLASS(OmxGsTest0001Step09)

NEW_GS_CLASS(OmxGsTest0002Step01)
NEW_GS_CLASS(OmxGsTest0002Step02)
NEW_GS_CLASS(OmxGsTest0002Step03)
NEW_GS_CLASS(OmxGsTest0002Step04)

NEW_GS_CLASS(OmxGsTest0003Step01)
NEW_GS_CLASS(OmxGsTest0003Step02)
NEW_GS_CLASS(OmxGsTest0003Step03)
NEW_GS_CLASS(OmxGsTest0003Step04)

NEW_GS_CLASS(OmxGsTest0004Step01)
NEW_GS_CLASS(OmxGsTest0004Step02)
NEW_GS_CLASS(OmxGsTest0004Step03)
NEW_GS_CLASS(OmxGsTest0004Step04)
NEW_GS_CLASS(OmxGsTest0004Step05)
NEW_GS_CLASS(OmxGsTest0004Step06)

NEW_GS_CLASS(OmxGsTest0005Step01)
NEW_GS_CLASS(OmxGsTest0005Step02)
NEW_GS_CLASS(OmxGsTest0005Step03)
NEW_GS_CLASS(OmxGsTest0005Step04)
NEW_GS_CLASS(OmxGsTest0005Step05)

NEW_GS_CLASS(OmxGsTest0006Step01)
NEW_GS_CLASS(OmxGsTest0006Step02)

NEW_GS_CLASS(OmxGsTest0007Step01)
NEW_GS_CLASS(OmxGsTest0007Step02)
NEW_GS_CLASS(OmxGsTest0007Step03)
NEW_GS_CLASS(OmxGsTest0007Step04)

NEW_GS_CLASS(OmxGsTest0008Step01)

NEW_GS_CLASS(OmxGsTest0009Step00)
NEW_GS_CLASS(OmxGsTest0010Step00)


#endif //GRAPHICSINKTESTSTEPS_H
