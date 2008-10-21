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
 *
 * Release: 050926
 *
 ***************************************************************************/
#ifndef _M4VIO_H_
#define _M4VIO_H_

#define BIT_BUFF_SIZE 8024000
#define TRUE 1
#define FALSE 0

#include "oscl_types.h"

int32 m4v_getVideoHeader(int32 layer, uint8 *buf, int32 max_size);
int32 m4v_getNextVideoSample(int32 layer_id, uint8 **buf, int32 max_buffer_size, uint32 *ts);

#endif
