/*
 * ----------------------------------------------------------------------
 *  Rappture::units
 *
 *  This is an interface to the rappture units module.
 *  It allows you to convert between units and format values.
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <tcl.h>
#include "RpUtils.h"
#include "stdlib.h"
#include "string.h"

extern "C" Tcl_AppInitProc RpUtils_Init;
static Tcl_CmdProc RpTclUtilsProgress;

/**********************************************************************/
// FUNCTION: RpUtils_Init()
/// Initializes the Rappture Utils module and commands defined below
/**
 * Called in RapptureGUI_Init() to initialize the Rappture Utils module.
 * Initialized commands include:
 * ::Rappture::progress
 */

int
RpUtils_Init(Tcl_Interp *interp)
{

    Tcl_CreateCommand(interp, "::Rappture::Utils::progress",
        RpTclUtilsProgress, (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

    return TCL_OK;
}

/**********************************************************************/
// FUNCTION: RpTclUtilsProgress()
/// Rappture::progress function in Tcl, used to print progress statements in gui.
/**
 * Full function call:
 * ::Rappture::progress <value> ?-mesg mesg?
 *
 */

int
RpTclUtilsProgress  (   ClientData cdata,
                        Tcl_Interp *interp,
                        int argc,
                        const char *argv[]  )
{
    int err                   = 0;  // err code for validate()
    int nextarg               = 1;
    long int val              = 0; // value
    const char* mesg          = NULL; // error mesg text

    Tcl_ResetResult(interp);

    // parse through command line options
    if ( (argc != 2) && (argc != 4) ) {
        Tcl_AppendResult(interp,
            "wrong # args: should be \"",argv[0]," <value> ?-mesg message?\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    val = (int) strtol(argv[nextarg++],NULL,10);

    if (argc == 4) {
        if (*(argv[nextarg]) == '-') {
            if (strncmp("-mesg",argv[nextarg],5) == 0) {
                nextarg++;
                mesg = argv[nextarg];
            }
        }
    }

    err = Rappture::Utils::progress((int)val,mesg);

    if (err != 0) {
        Tcl_AppendResult(interp,
            "error while printing progress to stdout",
            (char*)NULL);
        return TCL_ERROR;
    }

    return TCL_OK;
}
