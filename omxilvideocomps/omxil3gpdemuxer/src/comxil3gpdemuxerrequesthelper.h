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

#include <e32base.h>
#include <openmax/il/common/omxilprocessingfunction.h>

#ifndef COMXIL3GPDEMUXERREQUESTHELPER_H
#define COMXIL3GPDEMUXERREQUESTHELPER_H

NONSHARABLE_CLASS(COmxIL3GPDemuxerRequestHelper) : public CActive
    {
public:
    static COmxIL3GPDemuxerRequestHelper * NewL(COmxILProcessingFunction* aProcessingFunction);
    ~COmxIL3GPDemuxerRequestHelper();
    void RunL();
    void DoCancel();
    
    OMX_ERRORTYPE StateTransitionIndication(TStateIndex aNewState);
    OMX_ERRORTYPE BufferFlushingIndication(TUint32 aPortIndex, OMX_DIRTYPE aDirection);
    OMX_BOOL BufferRemovalIndication(OMX_BUFFERHEADERTYPE* apBufferHeader, OMX_DIRTYPE aDirection);
    
private:
    COmxIL3GPDemuxerRequestHelper(COmxILProcessingFunction* aProcessingFunction);
    void ConstructL();

    void DoRequest();
    void DoStateTransitionIndication();
    void DoBufferFlushingIndication();
    void DoBufferRemovalIndication();
    
private:
    enum TEC3GPDemuxerFunctionCode
        {
        EC3GPDemuxerFunctionCodeNone,
        EC3GPDemuxerFunctionCodeStateTransitionIndication,
        EC3GPDemuxerFunctionCodeBufferFlushingIndication,
        EC3GPDemuxerFunctionCodeBufferRemovalIndication
        
        // ... more to be added when required
        };
    
private:
    COmxILProcessingFunction* iProcessingFunction; // not owned
    TEC3GPDemuxerFunctionCode iFunctionCode;
    
    TStateIndex iNewState;
    OMX_ERRORTYPE iOMXError;
    TUint32 iPortIndex;
    OMX_DIRTYPE iDirection;
    OMX_BUFFERHEADERTYPE* ipBufferHeader; // not owned
    OMX_BOOL iOMXBool;
    
    TThreadId iConstructionThreadId;
    TRequestStatus* iCallingStatus; // not owned
    TThreadId iCallingThreadId;
    };

#endif //COMXIL3GPDEMUXERREQUESTHELPER_H
