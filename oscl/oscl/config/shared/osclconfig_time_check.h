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

//osclconfig: this build configuration file is for win32
#ifndef OSCLCONFIG_TIME_CHECK_H_INCLUDED
#define OSCLCONFIG_TIME_CHECK_H_INCLUDED



/**
OsclBasicTimeStruct type should be defined to the platform-specific
time of day type.
*/
typedef OsclBasicTimeStruct __Validate__BasicTimeStruct__;

/**
OsclBasicDateTimeStruct type should be defined to the platform-specific
date + time type.
*/
typedef OsclBasicDateTimeStruct __Validate__BasicTimeDateStruct__;

#endif //OSCLCONFIG_TIME_CHECK_H_INCLUDED


