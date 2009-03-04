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

//               O S C L _ U T F 8 C O N V

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

/*! \addtogroup osclutil OSCL Util
 *
 * @{
 */


/** \file oscl_utf8conv.h
    \brief Utilities to convert unicode to utf8.
*/


/********************************************************************************
							UTF-8 Bit Distribution

UTF-16									1st Byte 2nd Byte 3rd Byte 4th Byte
-------- -------- -------- --------     -------- -------- -------- --------
00000000 0xxxxxxx						0xxxxxxx
00000yyy yyxxxxxx						110yyyyy 10xxxxxx
zzzzyyyy yyxxxxxx						1110zzzz 10yyyyyy 10xxxxxx
110110ww wwzzzzyy 110111yy yyxxxxxx	    11110uuu 10uuzzzz 10yyyyyy 10xxxxxx

NOTE:
 uuuuu = wwww+1 (to account for addition of 0x10000 as in Section 3.7, Surrogates)

**********************************************************************************/


#ifndef OSCL_UTF8CONV_H
#define OSCL_UTF8CONV_H

#ifndef OSCL_BASE_INCLUDED_H
#include "oscl_base.h"
#endif

/**********************************************************************************/
/*                                                                                */
/* Function:     oscl_UnicodeToUTF8                                                    */
/* Description:  Convert UTF8 byte sequence to Unicode string		    	  */
/*                                                                                */
/* Parameters:   szSrc - UTF8 byte sequence to be converted			  */
/*				 nSrcLen - Length of szSrc                        */
/*               strDest - unicode char buffer for				  */
/*               nDestLen - size (in characters) of buffer			  */
/*										  */
/* Returns:      On success, the number of characters in the destination buffer   */
/*               0 on failure due to insufficient buffer size			  */
/*                                                                                */
/* History:      Created  {DATE]  {BY} {NAME} {PRODUCT REV}                       */
/*               Modified {DATE]  {BY} {NAME} {PRODUCT REV}                       */
/*                                                                                */
/**********************************************************************************/

OSCL_IMPORT_REF int32 oscl_UTF8ToUnicode(const char *input, int32 inLength, oscl_wchar *output, int32 outLength);


/**********************************************************************************/
/*                                                                                */
/* Function:     oscl_UnicodeToUTF8                                                    */
/* Description:  Convert Unicode string to UTF8 byte sequence		    	  */
/*                                                                                *//*                                                                                */
/* Parameters:   szSrc - Unicode string to be converted				  */
/*				 nSrcLen - Length of szSrc                        */
/*               strDest - char buffer for UTF8 text				  */
/*               nDestLen - size (in characters) of buffer			  */
/*										  */
/* Returns:      On success, the number of bytes in the destination buffer        */
/*               0 on failure due to insufficient buffer size			  */
/*                                                                                */
/* History:      Created  {DATE]  {BY} {NAME} {PRODUCT REV}                       */
/*               Modified {DATE]  {BY} {NAME} {PRODUCT REV}                       */
/*                                                                                */
/**********************************************************************************/

OSCL_IMPORT_REF int32 oscl_UnicodeToUTF8(const oscl_wchar *input, int32 inLength, char *output, int32 outLength);

#endif /* OSCL_UTF8CONV_H */

/*! @} */
