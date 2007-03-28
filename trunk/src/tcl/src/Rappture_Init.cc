/*
 * ----------------------------------------------------------------------
 *  Rappture_Init
 *
 *  This file contains the function that initializes all of the various
 *  extensions in this package.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILD_rappture
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

EXTERN int Rappture_Init _ANSI_ARGS_((Tcl_Interp *interp));

#include "RpLibraryTclInterface.h"
#include "RpUnitsTclInterface.h"
#include "RpEncodeTclInterface.h"

#ifdef __cplusplus
}
#endif

int
Rappture_Init( Tcl_Interp * interp)
{
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }

    if (Tcl_PkgProvide(interp, "Rappture", PACKAGE_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }

    /*
    if (Rappturelibrary_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    */

    if (RapptureUnits_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }

    if (RapptureEncoding_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }

    return TCL_OK;
}
