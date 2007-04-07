/*
 * ======================================================================
 *  Rappture::Utils
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2007  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <iostream>
#include "RpUtils.h"

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

using namespace Rappture::Utils;

int progress(int percent, const char* text)
{
    int retVal = 0;
    if (text != NULL) {
        std::cout << "=RAPPTURE-PROGRESS=>" << percent << " " << text << std::endl;
    }
    else {
        std::cout << "=RAPPTURE-PROGRESS=>" << percent << std::endl;
    }
    return retVal;
}


#ifdef __cplusplus
}
#endif // ifdef __cplusplus
