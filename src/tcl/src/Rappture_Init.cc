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

int Rappture_Init _ANSI_ARGS_((Tcl_Interp *interp));

#include "RpLibraryTclInterface.h"
#include "RpUnitsTclInterface.h"

#ifdef __cplusplus
}
#endif

int
Rappture_Init( Tcl_Interp * interp)
{
    if (Rappturelibrary_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Rapptureunits_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }

    return TCL_OK;
}
