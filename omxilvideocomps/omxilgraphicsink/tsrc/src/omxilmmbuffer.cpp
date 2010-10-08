// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//


/**
 @file
 @internalComponent
*/

#include "omxilmmbuffer.h"

EXPORT_C COmxILMMBuffer* COmxILMMBuffer::NewL(const RChunk& aChunk)
	{
	COmxILMMBuffer* self = new (ELeave) COmxILMMBuffer(aChunk);
	return self;
	}
	
EXPORT_C COmxILMMBuffer* COmxILMMBuffer::CreateL(const COmxILMMBuffer& aCamBuf)
	{
	COmxILMMBuffer* self = new (ELeave) COmxILMMBuffer(aCamBuf);
	return self;
	}

EXPORT_C COmxILMMBuffer::~COmxILMMBuffer()
	{	
	iArrayOffsets.Close();
	}
	
COmxILMMBuffer::COmxILMMBuffer(const RChunk& aChunk) 
  : iChunk(const_cast<RChunk&>(aChunk)),
  iChunkOwnerThreadId(RThread().Id())
	{
	// reset offsets
	iArrayOffsets.Reset();
	}

COmxILMMBuffer::COmxILMMBuffer(const COmxILMMBuffer& aCamBuf)
: iSurfaceId(aCamBuf.iSurfaceId),
  iSurfaceInfo(aCamBuf.iSurfaceInfo),
  iChunk(aCamBuf.iChunk),
  iChunkOwnerThreadId(aCamBuf.iChunkOwnerThreadId)
	{
	// reset offsets
	iArrayOffsets.Reset();
	
	// deep copy
	for (TInt i = 0 ; i < aCamBuf.iArrayOffsets.Count() ; i++)
		{
		iArrayOffsets.Append(aCamBuf.iArrayOffsets[i]);
		}
	}
		
EXPORT_C RChunk& COmxILMMBuffer::Chunk()
	{
	return iChunk;
	}

EXPORT_C TSurfaceId& COmxILMMBuffer::SurfaceId()
	{
	return iSurfaceId;
	}

EXPORT_C TThreadId& COmxILMMBuffer::ChunkOwnerThreadId()
	{
	return iChunkOwnerThreadId;
	}

EXPORT_C RSurfaceManager::TSurfaceInfoV01& COmxILMMBuffer::SurfaceInfoV01()
	{
	return iSurfaceInfo;
	}

//EXPORT_C void COmxILMMBuffer::SetChunk(RChunk* aChunk) {iChunk = *aChunk;}
//EXPORT_C void COmxILMMBuffer::SetSurfaceId(TSurfaceId* aSurfaceId) {iSurfaceId = &aSurfacdId;}

EXPORT_C TInt COmxILMMBuffer::BytesPerPixel(OMX_COLOR_FORMATTYPE aPixelFormat)
	{
	switch (aPixelFormat)
		{
		//fall through
		
		//case EUidPixelFormatRGB_565:
		case OMX_COLOR_Format16bitRGB565:
    	//case EUidPixelFormatBGR_565:
    	case OMX_COLOR_Format16bitBGR565:		
		//case EUidPixelFormatARGB_1555:
		case OMX_COLOR_Format16bitARGB1555:
		//case EUidPixelFormatXRGB_1555:
		//case EUidPixelFormatARGB_4444:
		case OMX_COLOR_Format16bitARGB4444:
		//case EUidPixelFormatARGB_8332:
		case OMX_COLOR_Format8bitRGB332:
		//case EUidPixelFormatBGRX_5551:
		//case EUidPixelFormatBGRA_5551:
		//case EUidPixelFormatBGRA_4444:
		//case EUidPixelFormatBGRX_4444:
		//case EUidPixelFormatAP_88:
		//case EUidPixelFormatXRGB_4444:
		//case EUidPixelFormatXBGR_4444:
		//case EUidPixelFormatYUV_422Interleaved:
		//case EUidPixelFormatYUV_422Planar:
		case OMX_COLOR_FormatYUV422Planar:
		//case EUidPixelFormatYUV_422Reversed:
		case OMX_COLOR_FormatYCrYCb:
		//case EUidPixelFormatYUV_422SemiPlanar:
		case OMX_COLOR_FormatYUV422SemiPlanar:
		//case EUidPixelFormatYUV_422InterleavedReversed:
		//case EUidPixelFormatYYUV_422Interleaved:
		case OMX_COLOR_FormatCbYCrY:
		case OMX_COLOR_FormatYCbYCr:
		case OMX_COLOR_FormatRawBayer10bit:
		   	{
			return 2;
			}
			
		//fall through
		//case EUidPixelFormatXRGB_8888:
		//case EUidPixelFormatBGRX_8888:
		//case EUidPixelFormatXBGR_8888:
		//case EUidPixelFormatBGRA_8888:
		case OMX_COLOR_Format32bitBGRA8888:
    	//case EUidPixelFormatARGB_8888:
		case OMX_COLOR_Format32bitARGB8888:
		//case EUidPixelFormatABGR_8888:
		//case EUidPixelFormatARGB_8888_PRE:
		//case EUidPixelFormatABGR_8888_PRE:
		//case EUidPixelFormatBGRA_8888_PRE:
		//case EUidPixelFormatARGB_2101010:
		//case EUidPixelFormatABGR_2101010:
			{
			return 4;
			}
			
		//fall through	
		//case EUidPixelFormatBGR_888:
		case OMX_COLOR_Format24bitBGR888:
		//case EUidPixelFormatRGB_888:
		case OMX_COLOR_Format24bitRGB888:
			{
			return 3;	
			}
			
		default:
			return 0;
		}
	}

EXPORT_C TInt COmxILMMBuffer::BytesPerPixel(TUidPixelFormat aPixelFormat)
	{
	switch (aPixelFormat)
		{
		//fall through
		
		case EUidPixelFormatRGB_565:
    	case EUidPixelFormatBGR_565:
		case EUidPixelFormatARGB_1555:
		case EUidPixelFormatXRGB_1555:
		case EUidPixelFormatARGB_4444:
		case EUidPixelFormatARGB_8332:
		case EUidPixelFormatBGRX_5551:
		case EUidPixelFormatBGRA_5551:
		case EUidPixelFormatBGRA_4444:
		case EUidPixelFormatBGRX_4444:
		case EUidPixelFormatAP_88:
		case EUidPixelFormatXRGB_4444:
		case EUidPixelFormatXBGR_4444:
		case EUidPixelFormatYUV_422Interleaved:
		case EUidPixelFormatYUV_422Planar:
		case EUidPixelFormatYUV_422Reversed:
		case EUidPixelFormatYUV_422SemiPlanar:
		case EUidPixelFormatYUV_422InterleavedReversed:
		case EUidPixelFormatRawBayer10bit:
		   	{
			return 2;
			}
			
		//fall through
		case EUidPixelFormatXRGB_8888:
		case EUidPixelFormatBGRX_8888:
		case EUidPixelFormatXBGR_8888:
		case EUidPixelFormatBGRA_8888:
		case EUidPixelFormatARGB_8888:
		case EUidPixelFormatABGR_8888:
		case EUidPixelFormatARGB_8888_PRE:
		case EUidPixelFormatABGR_8888_PRE:
		case EUidPixelFormatBGRA_8888_PRE:
		case EUidPixelFormatARGB_2101010:
		case EUidPixelFormatABGR_2101010:
			{
			return 4;
			}
			
		//fall through	
		case EUidPixelFormatBGR_888:
		case EUidPixelFormatRGB_888:
			{
			return 3;	
			}
			
		default:
			return 0;
		}
	}

EXPORT_C RArray<TInt>& COmxILMMBuffer::OffsetInfoArray()
	{
	return iArrayOffsets;
	}
