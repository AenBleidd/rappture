/*
 * ----------------------------------------------------------------------
 *  RpInit
 *
 *  This file contains the function that initializes all of the various
 *  extensions in this package.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>

int RpRlimit_Init _ANSI_ARGS_((Tcl_Interp *interp));

int
Rappturegui_Init(interp)
    Tcl_Interp *interp;  /* interpreter being initialized */
{
    if (RpRlimit_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpRusage_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}
