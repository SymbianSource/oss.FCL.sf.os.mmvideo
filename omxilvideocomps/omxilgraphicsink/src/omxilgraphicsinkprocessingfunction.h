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

#ifndef OMXILGRAPHICSINKPROCESSINGFUNCTION_H
#define OMXILGRAPHICSINKPROCESSINGFUNCTION_H

#include <e32msgqueue.h>
#include <openmax/il/common/omxilprocessingfunction.h>
#include <openmax/il/extensions/omxilsymbianvideographicsinkextensions.h>
#include <openmax/il/khronos/v1_x/OMX_Video.h>
#include <graphics/surface.h>
#include <graphics/surfacemanager.h>
#include <graphics/surfaceupdateclient.h>
#include <graphics/surfaceconfiguration.h>
#include "mmfbuffershared.h"


class COmxILGraphicSinkVPB0Port;
/**
The class for GraphicSink processing functions. This provides the main processing engine behind the graphic sink.
*/
class COmxILGraphicSinkProcessingFunction :
	public COmxILProcessingFunction
	{
private:
    class TGraphicSurfaceSettings
        {
    public:
        TGraphicSurfaceSettings() : iSurfaceAttributes(iSurfaceAttributesBuf()) {};
        
    public:
        // Surface configuration
        TSurfaceConfiguration iSurfaceConfig;
        // A package for surface attribute.
        RSurfaceManager::TSurfaceCreationAttributesBuf iSurfaceAttributesBuf;
        // A surface attribute.
        RSurfaceManager::TSurfaceCreationAttributes& iSurfaceAttributes;
        } iGraphicSurfaceSettings;

public:
	static COmxILGraphicSinkProcessingFunction* NewL(
			MOmxILCallbackNotificationIf& aCallbacks);

	~COmxILGraphicSinkProcessingFunction();

	OMX_ERRORTYPE StateTransitionIndication(TStateIndex aNewState);

	OMX_ERRORTYPE BufferFlushingIndication(TUint32 aPortIndex,
										   OMX_DIRTYPE aDirection);

	OMX_ERRORTYPE ParamIndication(OMX_INDEXTYPE aParamIndex,
									  const TAny* apComponentParameterStructure);

	OMX_ERRORTYPE ConfigIndication(OMX_INDEXTYPE aConfigIndex,
									   const TAny* apComponentConfigStructure);

	OMX_ERRORTYPE BufferIndication(
		OMX_BUFFERHEADERTYPE* apBufferHeader,
		OMX_DIRTYPE aDirection);

	OMX_BOOL BufferRemovalIndication(
		OMX_BUFFERHEADERTYPE* apBufferHeader,
		OMX_DIRTYPE aDirection);
	
	void CreateBufferL(OMX_U8*& aPortSpecificBuffer, OMX_PTR& aPortPrivate, OMX_U32 aBufferCountActual);
	void DestroyBuffer(OMX_PTR apPortPrivate);
	void InitBufferL(OMX_U32 aSizeBytes, OMX_U8* apBuffer, OMX_U32 aBufferCountActual);
	void DeInitBuffer();
	void WaitForTransitionToPauseToFinish();
	void TransitionToPauseFinished();

	TUidPixelFormat ConvertPixelFormat(OMX_COLOR_FORMATTYPE aColorFormat);
	
	// Access private member (read and write)
	inline MOmxILCallbackNotificationIf& GetCallbacks();
	inline TSurfaceConfiguration& GetSurfaceConfiguration();
	inline RPointerArray<OMX_BUFFERHEADERTYPE>& BuffersToEmpty();
	inline TGraphicSurfaceSettings& GraphicSurfaceSettings();
	inline OMX_STATETYPE State();
	inline void SetState(OMX_STATETYPE aState);
	inline RSurfaceManager& SurfaceManager();
	inline TInt GetFastCounterFrequency();

	void SetSharedChunkBufConfig(TMMSharedChunkBufConfig aSharedChunkBufConfig);
    void GetSharedChunkMetadata(OMX_U32& aHandleId, OMX_U64& aThreadId) const;

public:
	//Not own, just points to related graphic sink port, so set it to public data member.
    COmxILGraphicSinkVPB0Port* iGraphicSinkPort;
private:
	COmxILGraphicSinkProcessingFunction(MOmxILCallbackNotificationIf& aCallbacks);
	void ConstructL();
	
	void InitSurfaceAttributes();

private:
	RPointerArray<OMX_BUFFERHEADERTYPE> iBuffersToEmpty;
	OMX_BUFFERHEADERTYPE* iBufferOnScreen;
	RMutex iBufferMutex;
	OMX_STATETYPE iState;
	TInt iFastCounterFrequency;
    // A surface manager.
    RSurfaceManager iSurfaceManager;
    TThreadId iOwnerThreadId;
    CActiveSchedulerWait* iTransitionToPauseWait;
    RSemaphore iTransitionToPauseWaitSemaphore;

	/**
	 * Mediates access to the RSurfaceUpdateSession.
	 */
	class CGraphicSurfaceAccess : public CActive
		{
	public:
		static CGraphicSurfaceAccess* NewL(COmxILGraphicSinkProcessingFunction& aParent);
		~CGraphicSurfaceAccess();

		// from CActive
		void RunL();
		void DoCancel();

		TInt OpenDevice();
		void CloseDevice();
		TInt Execute();
		TInt Pause();
		TInt Stop();
		
		void CreateBufferL(OMX_U8*& aPortSpecificBuffer, OMX_PTR& aPortPrivate, OMX_U32 aBufferCountActual);
		void InitBufferL(OMX_U32 aSizeBytes, OMX_U8* apBuffer, OMX_U32 aBufferCountActual);
		
		TInt ProcessNextBuffer();
		
		void ResetSurfaceId();
		void CloseChunk();

	public:
	    TInt iSharedChunkHandleId;
	    TUint64 iSharedChunkThreadId;
	    TMMSharedChunkBufConfig iSharedChunkBufConfig;
        TBool iIsLocalChunk;

	private:
		CGraphicSurfaceAccess(COmxILGraphicSinkProcessingFunction& aParent);
		
	public:
	    TInt iBufferIdGenerator;
	    
	private:
		OMX_BUFFERHEADERTYPE* iCurrentBuffer;
		COmxILGraphicSinkProcessingFunction& iParent;
		
		// Memory chunk
		RChunk iChunk;
		
        #ifdef ILCOMPONENTCONFORMANCE
		// These are added for the ILComponentConformance Suite
		// Handles copy of data in case of Non-buffer supplier, while running IL Component conformance suite
		TBool iIsBufferSupplier;
		// Conformance suite does not handle chunks therefore offsets from base chunk address are held in this array
		RArray<TInt> iArrayOffsets;
        #endif
		
		// Surface Id
		TSurfaceId iSurfaceId;
		// A surface update session.
		RSurfaceUpdateSession iSurfaceUpdateSession;
		// Time stamp of the display notification event
		TTimeStamp iTimeStamp;
		// An indicator for first frame displayed
		TBool iFirstFrameDisplayed;

		RArray<TInt> iOffsetArray;

		} *iGraphicSurfaceAccess;

	/**
	 * Serializes operations into the 'main' thread hosting the Active Scheduler.
	 */
	class CPFHelper : public CActive
		{
	public:
		static CPFHelper* NewL(COmxILGraphicSinkProcessingFunction& aParent, CGraphicSurfaceAccess& aGraphicSurfaceAccess);
		~CPFHelper();

		// from CActive
		void RunL();
		void DoCancel();

		OMX_ERRORTYPE OpenDevice();
		OMX_ERRORTYPE CloseDevice();
		OMX_ERRORTYPE ExecuteAsync();
		OMX_ERRORTYPE StopAsync();
		OMX_ERRORTYPE StopSync();
		OMX_ERRORTYPE Pause();
		OMX_ERRORTYPE BufferIndication();

		enum TMessageType 
			{
			EOpenDevice,
			ECloseDevice,
			EExecuteCommand,
			EStopCommand,
			EPauseCommand,
			EBufferIndication
			};
		
		RMsgQueue<TMessageType> iMsgQueue;
	
	private:
		CPFHelper(COmxILGraphicSinkProcessingFunction& aParent, CGraphicSurfaceAccess& aGraphicSurfaceAccess);
		void ConstructL();
		
		TInt ProcessQueue();
		
		OMX_ERRORTYPE ConvertError(TInt aError);

	private:
		static const TInt KMaxMsgQueueEntries = 25;

		COmxILGraphicSinkProcessingFunction& iParent;
		CGraphicSurfaceAccess& iGraphicSurfaceAccess;
		} *iPFHelper;
	};

#include "omxilgraphicsinkprocessingfunction.inl"

#endif // OMXILGRAPHICSINKPROCESSINGFUNCTION_H

