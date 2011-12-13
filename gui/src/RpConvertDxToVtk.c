
/* 
 * ----------------------------------------------------------------------
 *  RpConvertDxToVtk - 
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2011  Purdue Research Foundation
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
#include <float.h>
#include "tcl.h"

static INLINE char *
SkipSpaces(char *string) 
{
    while (isspace(*string)) {
	string++;
    }
    return string;
}

static INLINE char *
GetLine(char **stringPtr, const char *endPtr) 
{
    char *line, *p;

    line = SkipSpaces(*stringPtr);
    for (p = line; p < endPtr; p++) {
	if (*p == '\n') {
	    *stringPtr = p + 1;
	    return line;
	}
    }
    *stringPtr = p;
    return line;
}

static int
GetPoints(Tcl_Interp *interp, int nPoints, char **stringPtr, 
	  const char *endPtr, Tcl_Obj *objPtr) 
{
    int nValues;
    int i;
    const char *p;
    char mesg[2000];
    float *array, scale, vmin, vmax;

    nValues = 0;
    p = *stringPtr;
    array = malloc(sizeof(float) * nPoints);
    if (array == NULL) {
	return TCL_ERROR;
    }
    vmin = FLT_MAX, vmax = -FLT_MAX;
    for (i = 0; i < nPoints; i++) {
	double value;
	char *nextPtr;
		
	if (p >= endPtr) {
	    Tcl_AppendResult(interp, "unexpected EOF in reading points",
			     (char *)NULL);
	    return TCL_ERROR;
	}
	value = strtod(p, &nextPtr);
	if (nextPtr == p) {
	    Tcl_AppendResult(interp, "bad value found in reading points",
			     (char *)NULL);
	    return TCL_ERROR;
	}
	p = nextPtr;
	array[i] = value;
	if (value < vmin) {
	    vmin = value;
	} 
	if (value > vmax) {
	    vmax = value;
	}
    }
    scale = 1.0 / (vmax - vmin);
    for (i = 0; i < nPoints; i++) {
	sprintf(mesg, "%g\n", (array[i] - vmin) * scale);
	Tcl_AppendToObj(objPtr, mesg, -1);
    }
    free(array);
    *stringPtr = p;
    return TCL_OK;
}

/* 
 *  ConvertDxToVtk string
 */

static int
ConvertDxToVtkCmd(ClientData clientData, Tcl_Interp *interp, int objc,
		  Tcl_Obj *const *objv) 
{
    Tcl_Obj *objPtr, *pointsObjPtr;
    char *p, *pend;
    char *string;
    char mesg[2000];
    double delta[3];
    double origin[3];
    int count[3];
    int length, nComponents, nValues, nPoints;
    char *name;

    name = "myScalars";
    nComponents = nPoints = 0;
    delta[0] = delta[1] = delta[2] = 0.0; /* Suppress compiler warning. */
    origin[0] = origin[1] = origin[2] = 0.0; /* May not have an origin line. */
    count[0] = count[1] = count[2] = 0; /* Suppress compiler warning. */

    if (objc != 2) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
			 Tcl_GetString(objv[0]), " string\"", (char *)NULL);
	return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objv[1], &length);
    if (strncmp("<ODX>", string, 5) == 0) {
	string += 5;
	length -= 5;
    }
    pointsObjPtr = Tcl_NewStringObj("", -1);
    for (p = string, pend = p + length; p < pend; /*empty*/) {
	char *line;
	double ddx, ddy, ddz;

	line = GetLine(&p, pend);
        if (line >= pend) {
	    break;			/* EOF */
	}
        if ((line[0] == '#') || (line[0] == '\n')) {
	    continue;			/* Skip blank or comment lines. */
	}
	if (sscanf(line, "object %*d class gridpositions counts %d %d %d", 
		   count, count + 1, count + 2) == 3) {
	    if ((count[0] < 0) || (count[1] < 0) || (count[2] < 0)) {
		sprintf(mesg, "invalid grid size: x=%d, y=%d, z=%d", 
			count[0], count[1], count[2]);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
	    }
#ifdef notdef
	    fprintf(stderr, "found gridpositions counts %d %d %d\n",
		    count[0], count[1], count[2]);
#endif
	} else if (sscanf(line, "origin %lg %lg %lg", origin, origin + 1, 
		origin + 2) == 3) {
	    /* Found origin. */
#ifdef notdef
	    fprintf(stderr, "found origin %g %g %g\n",
		    origin[0], origin[1], origin[2]);
#endif
	} else if (sscanf(line, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
	    if (nComponents == 3) {
		Tcl_AppendResult(interp, "too many delta statements", 
			(char *)NULL);
		return TCL_ERROR;
	    }
	    delta[nComponents] = sqrt((ddx * ddx) + (ddy * ddy) + (ddz * ddz));
	    nComponents++;
#ifdef notdef
	    fprintf(stderr, "found delta %g %g %g\n", ddx, ddy, ddx);
#endif
	} else if (sscanf(line, "object %*d class array type %*s shape 3"
		" rank 1 items %d data follows", &nPoints) == 1) {
	    fprintf(stderr, "found class array type shape 3 nPoints=%d\n",
		    nPoints);
	    if (nPoints < 0) {
		sprintf(mesg, "bad # points %d", nPoints);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
            }	
	    if (nPoints != count[0]*count[1]*count[2]) {
		sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", count[0]*count[1]*count[2], 
			nPoints);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
	    }
	    if (GetPoints(interp, nPoints, &p, pend, pointsObjPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	} else if (sscanf(line, "object %*d class array type %*s rank 0"
		" items %d data follows", &nPoints) == 1) {
#ifdef notdef
	    fprintf(stderr, "found class array type rank 0 nPoints=%d\n", 
		nPoints);
#endif
	    if (nPoints != count[0]*count[1]*count[2]) {
		sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", count[0]*count[1]*count[2], 
			nPoints);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
	    }
	    if (GetPoints(interp, nPoints, &p, pend, pointsObjPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
#ifdef notdef
	} else {
	    fprintf(stderr, "unknown line (%.80s)\n", line);
#endif
	}
    }
    if (nPoints != count[0]*count[1]*count[2]) {
	sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", count[0]*count[1]*count[2], 
			nPoints);
	Tcl_AppendResult(interp, mesg, (char *)NULL);
	return TCL_ERROR;
    }

    objPtr = Tcl_NewStringObj("# vtk DataFile Version 2.0\n", -1);
    Tcl_AppendToObj(objPtr, "Converted from DX file\n", -1);
    Tcl_AppendToObj(objPtr, "ASCII\n", -1);
    Tcl_AppendToObj(objPtr, "DATASET STRUCTURED_POINTS\n", -1);
    sprintf(mesg, "DIMENSIONS %d %d %d\n", count[0], count[1], count[2]);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "ORIGIN %g %g %g\n", origin[0], origin[1], origin[2]);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "SPACING %g %g %g\n", delta[0], delta[1], delta[2]);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "POINT_DATA %d\n", nPoints);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "SCALARS %s float 1\n", name);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "LOOKUP_TABLE default\n");
    Tcl_AppendToObj(objPtr, mesg, -1);
    Tcl_AppendObjToObj(objPtr, pointsObjPtr);
    Tcl_DecrRefCount(pointsObjPtr);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpConvertDxToVtk_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "ConvertDxToVtk" command into the interpreter.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpConvertDxToVtk_Init(Tcl_Interp *interp)
{
    /* install the widget command */
    Tcl_CreateObjCommand(interp, "Rappture::ConvertDxToVtk", ConvertDxToVtkCmd,
        NULL, NULL);
    return TCL_OK;
}
