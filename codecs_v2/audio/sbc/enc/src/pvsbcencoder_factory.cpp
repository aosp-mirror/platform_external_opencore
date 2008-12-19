/***************************************************************************
 * This Software has been authored by or modified by PacketVideo Corporation.
 * Title and ownership, including all intellectual
 * property rights in and to the Software shall remain with PacketVideo
 * Corporation. The Software is protected by the patent and copyright laws of
 * the United States and by international treaty.
 *
 * No part of this software may be modified, reproduced or distributed without
 * the prior written consent of PacketVideo Corporation.
 *
 * Copyright (c) 1998, 2007, PacketVideo Corporation. All Rights Reserved.
 *
 * Release: NJ_SRCHREL_071018
 *
 ***************************************************************************/
/**
 * @file pvsbcencoder_factory.cpp
 * @brief Singleton factory for CPVSbcEncoder
 */

#include "oscl_base.h"

#include "pvsbcencoder.h"
#include "pvsbcencoder_factory.h"

#include "oscl_error_codes.h"
#include "oscl_exception.h"

// Use default DLL entry point for Symbian
#include "oscl_dll.h"

OSCL_DLL_ENTRY_POINT_DEFAULT()


////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF PVSbcEncoderInterface* PVSbcEncoderFactory::CreatePVSbcEncoder()
{
	PVSbcEncoderInterface* sbcenc = NULL;
	sbcenc = CPVSbcEncoder::New();
	if (sbcenc==NULL)
	{
		OSCL_LEAVE(OsclErrNoMemory);
	}
	return sbcenc;
}

////////////////////////////////////////////////////////////////////////////
OSCL_EXPORT_REF bool PVSbcEncoderFactory::DeletePVSbcEncoder(PVSbcEncoderInterface* aSbcEnc)
{
	if(aSbcEnc)
	{
		OSCL_DELETE(aSbcEnc);
		return true;
	}

	return false;
}

