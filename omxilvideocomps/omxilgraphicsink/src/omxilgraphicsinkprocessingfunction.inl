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

inline MOmxILCallbackNotificationIf& COmxILGraphicSinkProcessingFunction::GetCallbacks()
    {
    return iCallbacks;
    }

inline TSurfaceConfiguration& COmxILGraphicSinkProcessingFunction::GetSurfaceConfiguration()
    {
    return iGraphicSurfaceSettings.iSurfaceConfig;
    }

inline RPointerArray<OMX_BUFFERHEADERTYPE>& COmxILGraphicSinkProcessingFunction::BuffersToEmpty()
    {
    return iBuffersToEmpty;
    }

inline COmxILGraphicSinkProcessingFunction::TGraphicSurfaceSettings& COmxILGraphicSinkProcessingFunction::GraphicSurfaceSettings()
    {
    return iGraphicSurfaceSettings;
    }

inline OMX_STATETYPE COmxILGraphicSinkProcessingFunction::State()
    {
    return iState;
    }

inline void COmxILGraphicSinkProcessingFunction::SetState(OMX_STATETYPE aState)
    {
    iState = aState;
    }


inline TInt COmxILGraphicSinkProcessingFunction::GetFastCounterFrequency()
    {
    return iFastCounterFrequency;
    }

inline RSurfaceManager& COmxILGraphicSinkProcessingFunction::SurfaceManager()
    {
    return iSurfaceManager;
    }
