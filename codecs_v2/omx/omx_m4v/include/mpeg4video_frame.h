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
/*

	=====================================================================
	File: mpeg4VideoDecoder.h
	Description: MPEG-4 video decoder class header.
	Rev:   1.0 A
	Created: April 11, 2000
	=====================================================================

	Revision History:
		Rev:	NONE
		Date:
		Author:
		Description:

/////////////////////////////////////////////////////////////////////////
*/

#include "mp4dec_api.h"

/* //////////////////////////////////////////////////////////////////////

	=====================================================================
	File: mpeg4VideoDecoder.h
	Description: MPEG-4 video decoder application class.
	Rev:   1.0 A
	Created: April 11, 2000
	=====================================================================

	Revision History:
		Rev:	NONE
		Date:
		Author:
		Description:

/////////////////////////////////////////////////////////////////////////
*/
int InitializeVideoDecode(int32 *width, int32 *height,
                          unsigned char **buffer, int32 *size, int32 *max_size, int postproc);

void DisplayVideoFrame(unsigned char *frame, unsigned char *ppvBits);
int EndVideoDecode();

