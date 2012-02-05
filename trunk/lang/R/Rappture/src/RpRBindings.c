/*
 * ----------------------------------------------------------------------
 *  INTERFACE: R Rappture Bindings
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2011  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpLibraryRInterface.h"
#include "RpUnitsRInterface.h"
#include "RpUtilsRInterface.h"

#include <R_ext/Rdynload.h>

static const R_CallMethodDef CallEntries[] = {
    {"RPRLib", (DL_FUNC) &RPRLib, 1},
    {"RPRLibGetString", (DL_FUNC) &RPRLibGetString, 2},
    {"RPRLibGetDouble", (DL_FUNC) &RPRLibGetDouble, 2},
    {"RPRLibGetInteger", (DL_FUNC) &RPRLibGetInteger, 2},
    {"RPRLibGetBoolean", (DL_FUNC) &RPRLibGetBoolean, 2},
    {"RPRLibGetFile", (DL_FUNC) &RPRLibGetFile, 3},
    {"RPRLibPutString", (DL_FUNC) &RPRLibPutString, 4},
//    {"RPRLibPutData", (DL_FUNC) &RPRLibPutData, 5},
    {"RPRLibPutDouble", (DL_FUNC) &RPRLibPutDouble, 4},
    {"RPRLibPutFile", (DL_FUNC) &RPRLibPutFile, 5},
    {"RPRLibResult", (DL_FUNC) &RPRLibResult, 1},
    {"RPRUnitsConvertDouble", (DL_FUNC) &RPRUnitsConvertDouble, 2},
    {"RPRUnitsConvertString", (DL_FUNC) &RPRUnitsConvertString, 3},
    {"RPRUtilsProgress", (DL_FUNC) &RPRUtilsProgress, 2},
    {NULL, NULL, 0}
};

void R_init_Rappture(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}


#ifdef __cplusplus
}
#endif // ifdef __cplusplus

