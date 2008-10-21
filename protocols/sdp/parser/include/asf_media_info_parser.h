/* ------------------------------------------------------------------
 * Copyright (C) 2008 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
/*																			*/
/*	=====================================================================	*/
/*	File: asfMediaInfoParser.h												*/
/*	Description:															*/
/*																			*/
/*																			*/
/*	Rev:																	*/
/*	Created: 12/20/02														*/
/*	=====================================================================	*/
/*																			*/
/*	Revision History:														*/
/*																			*/
/*	Rev:																	*/
/*	Date:																	*/
/*	Description:															*/
/*																			*/
/* //////////////////////////////////////////////////////////////////////// */

#ifndef ASF_MEDIAINFO_PARSER_H
#define ASF_MEDIAINFO_PARSER_H
#include "base_media_info_parser.h"
#include "asf_media_info.h"

SDP_ERROR_CODE asfMediaInfoParser(const char *buff, const int index, SDPInfo *sdp);
uint32		getTypeSpecificDataLength(HeaderObject  *_pHeaderObject, uint8 streamNumber);
uint8*		getTypeSpecificData(HeaderObject  *_pHeaderObject, uint8 streamNumber) ;
#endif //ASF_MEDIAINFO_PARSER_H
