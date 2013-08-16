
/* 
 * ----------------------------------------------------------------------
 *  RpDxToVtk - 
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
GetUniformFieldValues(Tcl_Interp *interp, int nPoints, int *counts, char **stringPtr, 
                      const char *endPtr, Tcl_Obj *objPtr) 
{
    int i;
    const char *p;
    char mesg[2000];
    double *array, scale, vmin, vmax;
    int iX, iY, iZ;

    p = *stringPtr;
    array = malloc(sizeof(double) * nPoints);
    if (array == NULL) {
        return TCL_ERROR;
    }
    vmin = DBL_MAX, vmax = -DBL_MAX;
    iX = iY = iZ = 0;
    for (i = 0; i < nPoints; i++) {
        double value;
        char *nextPtr;
        int loc;

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
        loc = iZ*counts[0]*counts[1] + iY*counts[0] + iX;
        if (++iZ >= counts[2]) {
            iZ = 0;
            if (++iY >= counts[1]) {
                iY = 0;
                ++iX;
            }
        }
        array[loc] = value;
        if (value < vmin) {
            vmin = value;
        } 
        if (value > vmax) {
            vmax = value;
        }
    }
    scale = 1.0 / (vmax - vmin);
    for (i = 0; i < nPoints; i++) {
#ifdef notdef
        sprintf(mesg, "%g\n", (array[i] - vmin) * scale);
#endif
        sprintf(mesg, "%g\n", array[i]);
        Tcl_AppendToObj(objPtr, mesg, -1);
    }
    free(array);
    *stringPtr = (char *)p;
    return TCL_OK;
}

static int
GetStructuredGridFieldValues(Tcl_Interp *interp, int nPoints, char **stringPtr, 
                             const char *endPtr, Tcl_Obj *objPtr) 
{
    int i;
    const char *p;
    char mesg[2000];
    double *array, scale, vmin, vmax;

    p = *stringPtr;
    array = malloc(sizeof(double) * nPoints);
    if (array == NULL) {
        return TCL_ERROR;
    }
    vmin = DBL_MAX, vmax = -DBL_MAX;
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
#ifdef notdef
        sprintf(mesg, "%g\n", (array[i] - vmin) * scale);
#endif
        sprintf(mesg, "%g\n", array[i]);
        Tcl_AppendToObj(objPtr, mesg, -1);
    }
    free(array);
    *stringPtr = (char *)p;
    return TCL_OK;
}

static int
GetCloudFieldValues(Tcl_Interp *interp, int nXYPoints, int nZPoints, char **stringPtr, 
                    const char *endPtr, Tcl_Obj *objPtr) 
{
    int i;
    const char *p;
    char mesg[2000];
    double *array, scale, vmin, vmax;
    int iXY, iZ;
    int nPoints;

    nPoints = nXYPoints * nZPoints;

    p = *stringPtr;
    array = malloc(sizeof(double) * nPoints);
    if (array == NULL) {
        return TCL_ERROR;
    }
    vmin = DBL_MAX, vmax = -DBL_MAX;
    iXY = iZ = 0;
    for (i = 0; i < nPoints; i++) {
        double value;
        char *nextPtr;
        int loc;

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
        loc = nXYPoints * iZ + iXY;
        if (++iZ >= nZPoints) {
            iZ = 0;
            ++iXY;
        }
        array[loc] = value;
        if (value < vmin) {
            vmin = value;
        } 
        if (value > vmax) {
            vmax = value;
        }
    }
    scale = 1.0 / (vmax - vmin);
    for (i = 0; i < nPoints; i++) {
#ifdef notdef
        sprintf(mesg, "%g\n", (array[i] - vmin) * scale);
#endif
        sprintf(mesg, "%g\n", array[i]);
        Tcl_AppendToObj(objPtr, mesg, -1);
    }
    free(array);
    *stringPtr = (char *)p;
    return TCL_OK;
}

static int
GetPoints(Tcl_Interp *interp, double *array, int nXYPoints,
          char **stringPtr, const char *endPtr) 
{
    int i;
    const char *p;

    p = *stringPtr;
    if (array == NULL) {
        return TCL_ERROR;
    }
    for (i = 0; i < nXYPoints; i++) {
        double x, y, z;
        char *nextPtr;

        if (p >= endPtr) {
            Tcl_AppendResult(interp, "unexpected EOF in reading points",
                             (char *)NULL);
            return TCL_ERROR;
        }
        x = strtod(p, &nextPtr);
        if (nextPtr == p) {
            Tcl_AppendResult(interp, "bad value found in reading points",
                             (char *)NULL);
            return TCL_ERROR;
        }
        p = nextPtr;
        y = strtod(p, &nextPtr);
        if (nextPtr == p) {
            Tcl_AppendResult(interp, "bad value found in reading points",
                             (char *)NULL);
            return TCL_ERROR;
        }
        p = nextPtr;
        z = strtod(p, &nextPtr);
        if (nextPtr == p) {
            Tcl_AppendResult(interp, "bad value found in reading points",
                             (char *)NULL);
            return TCL_ERROR;
        }
        p = nextPtr;

        array[i*2  ] = x;
        array[i*2+1] = y;
    }

    *stringPtr = (char *)p;
    return TCL_OK;
}

/* 
 *  DxToVtk string
 *
 * In DX format:
 *  rank 0 means scalars,
 *  rank 1 means vectors,
 *  rank 2 means matrices,
 *  rank 3 means tensors
 * 
 *  For rank 1, shape is a single number equal to the number of dimensions.
 *  e.g. rank 1 shape 3 means a 3-component vector field
 *
 */

static int
DxToVtkCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv) 
{
    double *points;
    Tcl_Obj *objPtr, *pointsObjPtr, *fieldObjPtr;
    char *p, *pend;
    char *string;
    char mesg[2000];
    double dv0[3], dv1[3], dv2[3];
    double dx, dy, dz;
    double origin[3];
    int count[3];
    int length, nAxes, nPoints, nXYPoints;
    char *name;
    int isUniform;
    int isStructuredGrid;
    int i, ix, iy, iz;

    name = "component";
    points = NULL;
    nAxes = nPoints = nXYPoints = 0;
    dx = dy = dz = 0.0; /* Suppress compiler warning. */
    origin[0] = origin[1] = origin[2] = 0.0; /* May not have an origin line. */
    dv0[0] = dv0[1] = dv0[2] = 0.0;
    dv1[0] = dv1[1] = dv1[2] = 0.0;
    dv2[0] = dv2[1] = dv2[2] = 0.0;
    count[0] = count[1] = count[2] = 0; /* Suppress compiler warning. */
    isUniform = 1; /* Default to expecting uniform grid */
    isStructuredGrid = 0;

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
    fieldObjPtr = Tcl_NewStringObj("", -1);
    for (p = string, pend = p + length; p < pend; /*empty*/) {
        char *line;
        double ddx, ddy, ddz;

        line = GetLine(&p, pend);
        if (line >= pend) {
            break;                        /* EOF */
        }
        if ((line[0] == '#') || (line[0] == '\n')) {
            continue;                        /* Skip blank or comment lines. */
        }
        if (sscanf(line, "object %*d class gridpositions counts %d %d %d", 
                   count, count + 1, count + 2) == 3) {
            isUniform = 1;
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
            if (nAxes == 3) {
                Tcl_AppendResult(interp, "too many delta statements", 
                        (char *)NULL);
                return TCL_ERROR;
            }
            switch (nAxes) {
            case 0:
                dv0[0] = ddx;
                dv0[1] = ddy;
                dv0[2] = ddz;
                break;
            case 1:
                dv1[0] = ddx;
                dv1[1] = ddy;
                dv1[2] = ddz;
                break;
            case 2:
                dv2[0] = ddx;
                dv2[1] = ddy;
                dv2[2] = ddz;
                break;
            default:
                break;
            }

            if (ddx != 0.0) {
                if (ddy != 0.0 || ddz != 0.0) {
                    /* Not axis aligned or skewed grid */
                    isUniform = 0;
                    isStructuredGrid = 1;
                }
                dx = ddx;
            } else if (ddy != 0.0) {
                if (ddx != 0.0 || ddz != 0.0) {
                    /* Not axis aligned or skewed grid */
                    isUniform = 0;
                    isStructuredGrid = 1;
                }
                dy = ddy;
            } else if (ddz != 0.0) {
                if (ddx != 0.0 || ddy != 0.0) {
                    /* Not axis aligned or skewed grid */
                    isUniform = 0;
                    isStructuredGrid = 1;
                }
                dz = ddz;
            }
            nAxes++;
#ifdef notdef
            fprintf(stderr, "found delta %g %g %g\n", ddx, ddy, ddx);
#endif
        } else if (sscanf(line, "object %*d class regulararray count %d", 
                          &count[2]) == 1) {
            // Z grid
        } else if (sscanf(line, "object %*d class array type %*s rank 1 shape 3"
                          " items %d data follows", &nXYPoints) == 1) {
            // This is a 2D point cloud in xy with a uniform zgrid
            isUniform = 0;
#ifdef notdef
            fprintf(stderr, "found class array type shape 3 nPoints=%d\n",
                    nPoints);
#endif
            if (nXYPoints < 0) {
                sprintf(mesg, "bad # points %d", nXYPoints);
                Tcl_AppendResult(interp, mesg, (char *)NULL);
                return TCL_ERROR;
            }
            points = malloc(sizeof(double) * nXYPoints * 2);
            if (GetPoints(interp, points, nXYPoints, &p, pend) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if (sscanf(line, "object %*d class array type %*s rank 0"
                          " %*s %d data follows", &nPoints) == 1) {
#ifdef notdef
            fprintf(stderr, "found class array type rank 0 nPoints=%d\n", 
                nPoints);
#endif
            if ((isUniform || isStructuredGrid) &&
                nPoints != count[0]*count[1]*count[2]) {
                sprintf(mesg, "inconsistent data: expected %d points"
                        " but found %d points", count[0]*count[1]*count[2], 
                        nPoints);
                Tcl_AppendResult(interp, mesg, (char *)NULL);
                return TCL_ERROR;
            } else if (!(isUniform || isStructuredGrid) &&
                       nPoints != nXYPoints * count[2]) {
                sprintf(mesg, "inconsistent data: expected %d points"
                        " but found %d points", nXYPoints * count[2], 
                        nPoints);
                Tcl_AppendResult(interp, mesg, (char *)NULL);
                return TCL_ERROR;
            }
            if (isUniform) {
                if (GetUniformFieldValues(interp, nPoints, count, &p, pend, fieldObjPtr) 
                    != TCL_OK) {
                    return TCL_ERROR;
                }
            } else if (isStructuredGrid) {
                if (GetStructuredGridFieldValues(interp, nPoints, &p, pend, fieldObjPtr) 
                    != TCL_OK) {
                    return TCL_ERROR;
                }
            } else {
                if (GetCloudFieldValues(interp, nXYPoints, count[2], &p, pend, fieldObjPtr) 
                    != TCL_OK) {
                    return TCL_ERROR;
                }
            }
#ifdef notdef
        } else {
            fprintf(stderr, "unknown line (%.80s)\n", line);
#endif
        }
    }

    if (isUniform) {
        objPtr = Tcl_NewStringObj("# vtk DataFile Version 2.0\n", -1);
        Tcl_AppendToObj(objPtr, "Converted from DX file\n", -1);
        Tcl_AppendToObj(objPtr, "ASCII\n", -1);
        Tcl_AppendToObj(objPtr, "DATASET STRUCTURED_POINTS\n", -1);
        sprintf(mesg, "DIMENSIONS %d %d %d\n", count[0], count[1], count[2]);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "ORIGIN %g %g %g\n", origin[0], origin[1], origin[2]);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "SPACING %g %g %g\n", dx, dy, dz);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "POINT_DATA %d\n", nPoints);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "SCALARS %s double 1\n", name);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "LOOKUP_TABLE default\n");
        Tcl_AppendToObj(objPtr, mesg, -1);
        Tcl_AppendObjToObj(objPtr, fieldObjPtr);
    } else if (isStructuredGrid) {
#ifdef notdef
        fprintf(stderr, "dv0 %g %g %g\n", dv0[0], dv0[1], dv0[2]);
        fprintf(stderr, "dv1 %g %g %g\n", dv1[0], dv1[1], dv1[2]);
        fprintf(stderr, "dv2 %g %g %g\n", dv2[0], dv2[1], dv2[2]);
#endif
        objPtr = Tcl_NewStringObj("# vtk DataFile Version 2.0\n", -1);
        Tcl_AppendToObj(objPtr, "Converted from DX file\n", -1);
        Tcl_AppendToObj(objPtr, "ASCII\n", -1);
        Tcl_AppendToObj(objPtr, "DATASET STRUCTURED_GRID\n", -1);
        sprintf(mesg, "DIMENSIONS %d %d %d\n", count[0], count[1], count[2]);
        Tcl_AppendToObj(objPtr, mesg, -1);
        for (iz = 0; iz < count[2]; iz++) {
            for (iy = 0; iy < count[1]; iy++) {
                for (ix = 0; ix < count[0]; ix++) {
                    double x, y, z;
                    x = origin[0] + dv2[0] * iz + dv1[0] * iy + dv0[0] * ix;
                    y = origin[1] + dv2[1] * iz + dv1[1] * iy + dv0[1] * ix;
                    z = origin[2] + dv2[2] * iz + dv1[2] * iy + dv0[2] * ix;
                    sprintf(mesg, "%g %g %g\n", x, y, z);
                    Tcl_AppendToObj(pointsObjPtr, mesg, -1);
                }
            }
        }
        sprintf(mesg, "POINTS %d double\n", nPoints);
        Tcl_AppendToObj(objPtr, mesg, -1);
        Tcl_AppendObjToObj(objPtr, pointsObjPtr);
        sprintf(mesg, "POINT_DATA %d\n", nPoints);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "SCALARS %s double 1\n", name);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "LOOKUP_TABLE default\n");
        Tcl_AppendToObj(objPtr, mesg, -1);
        Tcl_AppendObjToObj(objPtr, fieldObjPtr);
    } else {
        /* Fill points.  Have to wait to do this since origin, delta can come after
         * the point list in the file.
         */
        for (iz = 0; iz < count[2]; iz++) {
            for (i = 0; i < nXYPoints; i++) {
                sprintf(mesg, "%g %g %g\n", points[i*2], points[i*2+1], origin[2] + dz * iz);
                Tcl_AppendToObj(pointsObjPtr, mesg, -1);
            }
        }
        free(points);

        objPtr = Tcl_NewStringObj("# vtk DataFile Version 2.0\n", -1);
        Tcl_AppendToObj(objPtr, "Converted from DX file\n", -1);
        Tcl_AppendToObj(objPtr, "ASCII\n", -1);
        Tcl_AppendToObj(objPtr, "DATASET UNSTRUCTURED_GRID\n", -1);
        sprintf(mesg, "POINTS %d double\n", nPoints);
        Tcl_AppendToObj(objPtr, mesg, -1);
        Tcl_AppendObjToObj(objPtr, pointsObjPtr);
        sprintf(mesg, "POINT_DATA %d\n", nPoints);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "SCALARS %s double 1\n", name);
        Tcl_AppendToObj(objPtr, mesg, -1);
        sprintf(mesg, "LOOKUP_TABLE default\n");
        Tcl_AppendToObj(objPtr, mesg, -1);
        Tcl_AppendObjToObj(objPtr, fieldObjPtr);
    }

    Tcl_DecrRefCount(pointsObjPtr);
    Tcl_DecrRefCount(fieldObjPtr);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  RpDxToVtk_Init --
 *
 *  Invoked when the Rappture GUI library is being initialized
 *  to install the "DxToVtk" command into the interpreter.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interp) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
RpDxToVtk_Init(Tcl_Interp *interp)
{
    /* install the widget command */
    Tcl_CreateObjCommand(interp, "Rappture::DxToVtk", DxToVtkCmd,
        NULL, NULL);
    return TCL_OK;
}
