
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

/* 
 *  ConvertDxToVtk string
 */

static int
ConvertDxToVtkCmd(ClientData clientData, Tcl_Interp *interp, int objc,
		  Tcl_Obj *const *objv) 
{
    Tcl_Obj *objPtr;
    char *p, *pend;
    char *string;
    char mesg[2000];
    double delta[3];
    double origin[3];
    double xMax, yMax, zMax, xMin, yMin, zMin;
    double maxValue[3], minValue[3];
    int count[3];
    int length, nComponents, nValues, nPoints;
    int xNum, yNum, zNum;
    size_t ix;

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
    for (p = string, pend = p + length; p < pend; /*empty*/) {
	char *line;
	double ddx, ddy, ddz;

	line = GetLine(&p, pend);
        if (line == pend) {
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
	} else if (sscanf(line, "origin %lg %lg %lg", origin, origin + 1, 
		origin + 2) == 3) {
	    /* Found origin. */
	} else if (sscanf(line, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
	    if (nComponents == 3) {
		Tcl_AppendResult(interp, "too many delta statements", 
			(char *)NULL);
		return TCL_ERROR;
	    }
	    delta[nComponents] = sqrt((ddx * ddx) + (ddy * ddy) + (ddz * ddz));
	    nComponents++;
	} else if (sscanf(line, "object %*d class array type %*s shape 3"
		" rank 1 items %d data follows", &nPoints) == 1) {
	    if (nPoints < 0) {
		sprintf(mesg, "bad # points %d", nPoints);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
            }	
	    printf("#points=%d\n", nPoints);
	    if (nPoints != count[0]*count[1]*count[2]) {
		sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", count[0]*count[1]*count[2], 
			nPoints);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
	    }
	    break;
	} else if (sscanf(line, "object %*d class array type %*s rank 0"
		" times %d data follows", &nPoints) == 1) {
	    if (nPoints != count[0]*count[1]*count[2]) {
		sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", count[0]*count[1]*count[2], 
			nPoints);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
	    }
	    break;
	}
    }
    if (nPoints != count[0]*count[1]*count[2]) {
	sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", count[0]*count[1]*count[2], 
			nPoints);
	Tcl_AppendResult(interp, mesg, (char *)NULL);
	return TCL_ERROR;
    }

    objPtr = Tcl_NewStringObj("vtk DataFile Version 2.0\n", -1);
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

    minValue[0] = minValue[1] = minValue[2] = DBL_MAX;
    maxValue[0] = maxValue[1] = maxValue[2] = -DBL_MAX;
    xMin = origin[0], yMin = origin[1], zMin = origin[2];
    xNum = count[0], yNum = count[1], zNum = count[2];
    xMax = origin[0] + delta[0] * count[0];
    yMax = origin[1] + delta[1] * count[1];
    zMax = origin[2] + delta[2] * count[2];
    nValues = 0;
    for (ix = 0; ix < xNum; ix++) {
	size_t iy;

	for (iy = 0; iy < yNum; iy++) {
	    size_t iz;

	    for (iz = 0; iz < zNum; iz++) {
		const char *line;
		double vx, vy, vz;

		if ((p == pend) || (nValues > (size_t)nPoints)) {
		    break;
		}
		line = GetLine(&p, pend);
		if ((line[0] == '#') || (line[0] == '\n')) {
		    continue;		/* Skip blank or comment lines. */
		}
		if (sscanf(line, "%lg %lg %lg", &vx, &vy, &vz) == 3) {
		    if (vx < minValue[0]) {
			minValue[0] = vx;
		    } else if (vx > maxValue[0]) {
			maxValue[0] = vx;
		    }
		    if (vy < minValue[1]) {
			minValue[1] = vy;
		    } else if (vy > maxValue[1]) {
			maxValue[1] = vy;
		    }
		    if (vz < minValue[2]) {
			minValue[2] = vz;
		    } else if (vz > maxValue[2]) {
			maxValue[2] = vz;
		    }
		    sprintf(mesg, "%g %g %g\n", vx, vy, vz);
		    Tcl_AppendToObj(objPtr, mesg, -1);
		    nValues++;
		}
	    }
	}
    }
    /* Make sure that we read all of the expected points. */
    if (nValues != (size_t)nPoints) {
	sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", nPoints, nValues);
	Tcl_AppendResult(interp, mesg, (char *)NULL);
	Tcl_DecrRefCount(objPtr);
	return TCL_ERROR;
    }
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
