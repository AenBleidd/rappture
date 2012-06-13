/*
 * ----------------------------------------------------------------------
 *  RapptureGUI_Init
 *
 *  This file contains the function that initializes all of the various
 *  extensions in this package.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <tcl.h>
#include <tk.h>

#ifdef BUILD_rappture
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

extern Tcl_AppInitProc Rappturegui_Init;
extern Tcl_AppInitProc RpCanvHotspot_Init;
extern Tcl_AppInitProc RpCanvPlacard_Init;
extern Tcl_AppInitProc RpConvertDxToVtk_Init;
extern Tcl_AppInitProc RpDiffview_Init;

#ifdef BUILD_Rappture
__declspec( dllexport )
#endif

int
Rappturegui_Init( Tcl_Interp * interp)
{
    if (Tcl_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }
    if (Tk_InitStubs(interp, "8.4", 0) == NULL) {
        return TCL_ERROR;
    }
    if (RpDiffview_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpCanvPlacard_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpConvertDxToVtk_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    if (RpCanvHotspot_Init(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

