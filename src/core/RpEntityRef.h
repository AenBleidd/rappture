/*
 * ----------------------------------------------------------------------
 *  Rappture Library Entity Reference Translation Header
 *
 *  Begin Character Entity Translator
 *
 *  The next section of code implements routines used to translate
 *  character entity references into their corresponding strings.
 *
 *  Examples:
 *
 *        &amp;          "&"
 *        &lt;           "<"
 *        &gt;           ">"
 *        &nbsp;         " "
 *
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *  Also see below text for additional information on usage and redistribution
 *
 * ======================================================================
 */

#ifndef _RpENTITYREF_H
#define _RpENTITYREF_H

#include "RpBuffer.h"

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

namespace Rappture {

class EntityRef
{
    public:
        const char* encode (const char* in, unsigned int len);
        const char* decode (const char* in, unsigned int len);
        int size();

        EntityRef () {};

        ~EntityRef() {};

    private:
        Buffer _bout;
};

} // namespace Rappture

#ifdef __cplusplus
    } // extern c
#endif // ifdef __cplusplus

#endif
