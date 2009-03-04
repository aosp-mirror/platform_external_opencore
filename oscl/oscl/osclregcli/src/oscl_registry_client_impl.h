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
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//				 Oscl Registry Client Impl

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclutil OSCL Util
 *
 * @{
 */

/*!
 * \file oscl_registry_client_impl.h
 * \brief Client-side implementation of OsclRegistryInterface
 *
 */

#ifndef OSCL_REGISTRY_CLIENT_IMPL_H_INCLUDED
#define OSCL_REGISTRY_CLIENT_IMPL_H_INCLUDED

#include "osclconfig_proc.h"

#include "oscl_registry_serv_impl_global.h"

//client is the same as server
class OsclRegistryClientImpl: public OsclRegistryServImpl
{
};

//access client is also the same as server
class OsclRegistryAccessClientImpl: public OsclRegistryServImpl
{
};



//TLS-based registry
#include "oscl_registry_serv_impl_tls.h"

//client is the same as server
class OsclRegistryClientTlsImpl: public OsclRegistryServTlsImpl
{
};

//access client is also the same as server
class OsclRegistryAccessClientTlsImpl: public OsclRegistryServTlsImpl
{
};

#endif //OSCL_REGISTRY_IMPL_H_INCLUDED
/*! @} */


