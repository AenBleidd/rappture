
/* 
 * ----------------------------------------------------------------------
 *  RpReadPoints - 
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <float.h>
#include "tcl.h"

#define UCHAR(c) ((unsigned char) (c))

static INLINE const char *
SkipSpaces(const char *s, const char *endPtr) 
{
    while ((s < endPtr) && ((*s == ' ') || (*s == '\t'))) {
	s++;
    }
    return s;
}

static INLINE const char *
GetLine(Tcl_DString *dsPtr, const char *s, const char *endPtr) 
{
    const char *line, *p;
    Tcl_DStringSetLength(dsPtr, 0);
    line = SkipSpaces(s, endPtr);
    for (p = line; p < endPtr; p++) {
	if (*p == '\n') {
	    if (p == line) {
		line++;
		continue;
	    }
	    Tcl_DStringAppend(dsPtr, line, p - line);
	    return p + 1;
	}
    }
    Tcl_DStringAppend(dsPtr, line, p - line);
    return p;
}

int
GetDouble(Tcl_Interp *interp, const char *s, double *valuePtr)
{
    char *end;
    double d;
    
    errno = 0;
    d = strtod(s, &end); /* INTL: TCL source. */
    if (end == s) {
	badDouble:
        if (interp != (Tcl_Interp *) NULL) {
            Tcl_AppendResult(interp,
		"expected floating-point number but got \"", s, "\"", 
		(char *) NULL);
        }
	return TCL_ERROR;
    }
    if (errno != 0 && (d == HUGE_VAL || d == -HUGE_VAL || d == 0)) {
        if (interp != (Tcl_Interp *) NULL) {
	    char msg[64 + TCL_INTEGER_SPACE];
	
	    sprintf(msg, "unknown floating-point error, errno = %d", errno);
	    Tcl_AppendToObj(Tcl_GetObjResult(interp), msg, -1);
	    Tcl_SetErrorCode(interp, "ARITH", "UNKNOWN", msg, (char *) NULL);
        }
	return TCL_ERROR;
    }
    while ((*end != 0) && isspace(UCHAR(*end))) { /* INTL: ISO space. */
	end++;
    }
    if (*end != 0) {
	goto badDouble;
    }
    *valuePtr = d;
    return TCL_OK;
}
    
/* 
 *  ReadPoints string dimVar pointsVar
 */
static int
ReadPoints(ClientData clientData, Tcl_Interp *interp, int objc,
	   Tcl_Obj *const *objv) 
{
    Tcl_Obj *listObjPtr;
    const char *p, *pend;
    const char *string;
    int count, length, dim;
    Tcl_DString ds;

    if (objc != 4) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		Tcl_GetString(objv[0]), " string dimVar pointsVar\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    dim = 0;
    string = Tcl_GetStringFromObj(objv[1], &length);
    count = 0;
    Tcl_DStringInit(&ds);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (p = string, pend = p + length; p < pend; /*empty*/) {
	const char *line;
	int i, argc;
	const char **argv;
	char saved;
	int result;

	p = GetLine(&ds, p, pend);
	if (Tcl_DStringLength(&ds) == 0) {
	    break;			/* EOF */
	}
	result = Tcl_SplitList(interp, Tcl_DStringValue(&ds), &argc, &argv);
	if (result != TCL_OK) {
	    goto error;
	}
	if (argc == 0) {
	    Tcl_Free((char *)argv);
	    goto error;
	}
	if (dim == 0) {
	    dim = argc;
	}
	if (dim != argc) {
	    Tcl_AppendResult(interp, "wrong # of elements on line \"", 
			line, "\"", (char *)NULL);
	    Tcl_Free((char *)argv);
	    goto error;
	}
	for (i = 0; i < argc; i++) {
	    double d;

	    if (GetDouble(interp, argv[i], &d) != TCL_OK) {
		Tcl_Free((char *)argv);
		goto error;
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(d));
	    count++;
	}
	Tcl_Free((char *)argv);
    }
    Tcl_DStringFree(&ds);
    if (Tcl_ObjSetVar2(interp, objv[2], NULL, Tcl_NewIntObj(dim),
		      TCL_LEAVE_ERR_MSG) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_ObjSetVar2(interp, objv[3], NULL, listObjPtr, 
		      TCL_LEAVE_ERR_MSG) == NULL) {
	return TCL_ERROR;
    }
    return TCL_OK;
 error:
    Tcl_DStringFree(&ds);
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  RpReadPoints_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "ConvertDxToVtk" command into the interpreter.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpReadPoints_Init(Tcl_Interp *interp)
{
    /* install the widget command */
    Tcl_CreateObjCommand(interp, "Rappture::ReadPoints", ReadPoints,
        NULL, NULL);
    return TCL_OK;
}
