
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
GetLine(char **stringPtr, char *endPtr) 
{
    char *line, *p;

    line = SkipSpaces(*stringPtr);
    for (p = line; p < endPtr; p++) {
	if (*p == '\n') {
	    p++;
	    *stringPtr = p;
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
    int nx, ny, nz, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char *p, *pend;
    int length;
    double xValueMin, yValueMin, zValueMin;
    double xValueMax, yValueMax, zValueMax;
    double xMin, yMin, zMin;
    int xNum, yNum, zNum;
    double xMax, yMax, zMax;
    int nComponents, nValues;
    char *string;
    Tcl_Obj *objPtr;
    char mesg[2000];
    size_t ix;

    dx = dy = dz = 0.0;			/* Suppress compiler warning. */
    x0 = y0 = z0 = 0.0;			/* May not have an origin line. */
    nx = ny = nz = npts = 0;		/* Suppress compiler warning. */

    if (objc != 2) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
			 Tcl_GetString(objv[0]), " string\"", (char *)NULL);
	return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objv[1], &length);
    for (p = string, pend = p + length; p < pend; /*empty*/) {
	char *line;

	line = GetLine(&p, pend);
        if (line == pend) {
	    break;			/* EOF */
	}
        if ((line[0] == '#') || (line[0] == '\n')) {
	    continue;			/* Skip blank or comment lines. */
	}
	if (sscanf(line, "object %*d class gridpositions counts %d %d %d", 
		   &nx, &ny, &nz) == 3) {
	    if ((nx < 0) || (ny < 0) || (nz < 0)) {
		sprintf(mesg, "invalid grid size: x=%d, y=%d, z=%d", 
			nx, ny, nz);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
	    }
	} else if (sscanf(line, "origin %lg %lg %lg", &x0, &y0, &z0) == 3) {
	    /* Found origin. */
	} else if (sscanf(line, "delta %lg %lg %lg", &ddx, &ddy, &ddz) == 3) {
	    /* Found one of the delta lines. */
	    if (ddx != 0.0) {
		dx = ddx;
	    } else if (ddy != 0.0) {
		dy = ddy;
	    } else if (ddz != 0.0) {
		dz = ddz;
	    }
	} else if (sscanf(line, "object %*d class array type %*s shape 3"
		" rank 1 items %d data follows", &npts) == 1) {
	    if (npts < 0) {
		sprintf(mesg, "bad # points %d", npts);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
            }	
	    printf("#points=%d\n", npts);
	    if (npts != nx*ny*nz) {
		sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", nx*ny*nz, npts);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
	    }
	    break;
	} else if (sscanf(line, "object %*d class array type %*s rank 0"
		" times %d data follows", &npts) == 1) {
	    if (npts != nx*ny*nz) {
		sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", nx*ny*nz, npts);
		Tcl_AppendResult(interp, mesg, (char *)NULL);
		return TCL_ERROR;
	    }
	    break;
	}
    }
    if (npts != nx*ny*nz) {
	sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", nx*ny*nz, npts);
	Tcl_AppendResult(interp, mesg, (char *)NULL);
	return TCL_ERROR;
    }

    objPtr = Tcl_NewStringObj("vtk DataFile Version 2.0\n", -1);
    Tcl_AppendToObj(objPtr, "Converted from DX file\n", -1);
    Tcl_AppendToObj(objPtr, "ASCII\n", -1);
    Tcl_AppendToObj(objPtr, "DATASET STRUCTURED_POINTS\n", -1);
    sprintf(mesg, "DIMENSIONS %d %d %d\n", nx, ny, nz);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "ORIGIN %g %g %g\n", x0, y0, z0);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "SPACING %g %g %g\n", dx, dy, dx);
    Tcl_AppendToObj(objPtr, mesg, -1);
    sprintf(mesg, "POINT_DATA %d\n", npts);
    Tcl_AppendToObj(objPtr, mesg, -1);

    xValueMin = yValueMin = zValueMin = FLT_MAX;
    xValueMax = yValueMax = zValueMax = -FLT_MAX;
    xMin = x0, yMin = y0, zMin = z0;
    xNum = nx, yNum = ny, zNum = nz;
    xMax = xMin + dx * xNum;
    yMax = yMin + dy * yNum;
    zMax = zMin + dz * zNum;
    nValues = 0;
    for (ix = 0; ix < nx; ix++) {
	size_t iy;

	for (iy = 0; iy < ny; iy++) {
	    size_t iz;

	    for (iz = 0; iz < nz; iz++) {
		char *line;
		if ((p == pend) || (nValues > (size_t)npts)) {
		    break;
		}
		line = GetLine(&p, pend);
		if ((line[0] == '#') || (line[0] == '\n')) {
		    continue;		/* Skip blank or comment lines. */
		}
		double vx, vy, vz;
		if (sscanf(line, "%lg %lg %lg", &vx, &vy, &vz) == 3) {
		    int nindex = (iz*nx*ny + iy*nx + ix) * 3;
		    if (vx < xValueMin) {
			xValueMin = vx;
		    } else if (vx > xValueMax) {
			xValueMax = vx;
		    }
		    if (vy < yValueMin) {
			yValueMin = vy;
		    } else if (vy > yValueMax) {
			yValueMax = vy;
		    }
		    if (vz < zValueMin) {
			zValueMin = vz;
		    } else if (vz > zValueMax) {
			zValueMax = vz;
		    }
		    sprintf(mesg, "%g %g %g\n", vx, vy, vz);
		    Tcl_AppendToObj(objPtr, mesg, -1);
		    nValues++;
		}
	    }
	}
    }
    /* Make sure that we read all of the expected points. */
    if (nValues != (size_t)npts) {
	sprintf(mesg, "inconsistent data: expected %d points"
			" but found %d points", npts, nValues);
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
