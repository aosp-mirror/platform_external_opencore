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
#ifndef PVMF_AMRENC_NODE_TYPES_H_INCLUDED
#define PVMF_AMRENC_NODE_TYPES_H_INCLUDED

#ifndef OSCL_BASE_H_INCLUDED
#include "oscl_base.h"
#endif
#ifndef PVMF_NODE_INTERFACE_H_INCLUDED
#include "pvmf_node_interface.h"
#endif

///////////////////////////////////////////////
// Port tags
///////////////////////////////////////////////

/** Enumerated list of port tags supported by the node for the port requests.*/
typedef enum
{
    PVMF_AMRENC_NODE_PORT_TYPE_INPUT = 0,
    PVMF_AMRENC_NODE_PORT_TYPE_OUTPUT
} PVMFVideoEncNodePortType;

///////////////////////////////////////////////
// Error Events
///////////////////////////////////////////////

enum PVMFVideoEncNodeErrorEvent
{
    PVMF_AMRENC_NODE_ERROR_ENCODE_ERROR = PVMF_NODE_ERROR_EVENT_LAST
};

///////////////////////////////////////////////
// Information Events
///////////////////////////////////////////////

// Enumerated list of informational event from PVMFVideoEncNode
enum PVMFVideoEncNodeInfoEvent
{
    PVMF_AMRENC_NODE_INFO = PVMF_NODE_INFO_EVENT_LAST
};


#endif // PVMF_AMRENC_NODE_TYPES_H_INCLUDED
