/*
 * ----------------------------------------------------------------------
 *  Rappture_Init
 *
 *  This file contains the function that initializes all of the various
 *  extensions in this package.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <tcl.h>
#include "config.h"

#ifdef BUILD_rappture
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

extern Tcl_AppInitProc Rappture_Init;
extern Tcl_AppInitProc RpRlimit_Init;
extern Tcl_AppInitProc RpRusage_Init;
extern Tcl_AppInitProc RpSignal_Init;
extern Tcl_AppInitProc RpSlice_Init;
extern Tcl_AppInitProc RpSysinfo_Init;
extern Tcl_AppInitProc RpDaemon_Init;
extern Tcl_AppInitProc RpCurses_Init;

#ifdef notdef
extern Tcl_AppInitProc RpLibrary_Init;
#endif
extern Tcl_AppInitProc RpUnits_Init;
extern Tcl_AppInitProc RpEncoding_Init;
extern Tcl_AppInitProc RpUtils_Init;

#ifdef BUILD_Rappture
__declspec( dllexport )
#endif
int
Rappture_Init( Tcl_Interp * interp)
{
    fprintf(stderr, "Rappture_init\n");
#ifdef _WIN32
    rpWinInitJob();
#endif
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "Rappture", RAPPTURE_VERSION) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_SetVar(interp, "::Rappture::version", RAPPTURE_VERSION,
                   TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_SetVar(interp, "::Rappture::build", SVN_VERSION,
                   TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL) {
        return TCL_ERROR;
    }
#ifdef notdef
    if (RpLibrary_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
#endif
    if (RpUnits_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpEncoding_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpUtils_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpRlimit_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpRusage_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpSignal_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpSlice_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpSysinfo_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpDaemon_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpCurses_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

