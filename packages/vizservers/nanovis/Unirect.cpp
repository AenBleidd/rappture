/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <float.h>
#include <tcl.h>

#include <RpField1D.h>
#include <RpFieldRect3D.h>

#include "Unirect.h"
#include "Trace.h"

extern int GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                           float *valuePtr);

extern int GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *indexPtr);

static inline char *
skipspaces(char *string) 
{
    while (isspace(*string)) {
        string++;
    }
    return string;
}

static inline char *
getline(char **stringPtr, char *endPtr) 
{
    char *line, *p;

    line = skipspaces(*stringPtr);
    for (p = line; p < endPtr; p++) {
        if (*p == '\n') {
            *p++ = '\0';
            *stringPtr = p;
            return line;
        }
    }
    *stringPtr = p;
    return line;
}

int
Rappture::Unirect2d::ParseBuffer(Tcl_Interp *interp, Rappture::Buffer &buf)
{
    Tcl_Obj *objPtr;
    objPtr = Tcl_NewStringObj(buf.bytes(), buf.size());
    Tcl_Obj **objv;
    int objc;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    int result;
    result = LoadData(interp, objc, objv);
    Tcl_DecrRefCount(objPtr);
    if ((result != TCL_OK) || (!isInitialized())) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

int
Rappture::Unirect3d::ParseBuffer(Tcl_Interp *interp, Rappture::Buffer &buf)
{
    Tcl_Obj *objPtr;
    objPtr = Tcl_NewStringObj(buf.bytes(), buf.size());
    Tcl_Obj **objv;
    int objc;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    int result;
    result = LoadData(interp, objc, objv);
    Tcl_DecrRefCount(objPtr);
    if ((result != TCL_OK) || (!isInitialized())) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

int 
Rappture::Unirect3d::LoadData(Tcl_Interp *interp, int objc, 
                              Tcl_Obj *const *objv)
{
    int num[3], nValues;
    float min[3], max[3];
    float *values;
    const char *units[4], *order;


    if ((objc-1) & 0x01) {
        Tcl_AppendResult(interp, Tcl_GetString(objv[0]), ": ",
                "wrong # of arguments: should be key-value pairs",
                (char *)NULL);
        return TCL_ERROR;
    }

    /* Default order is  z, y, x. */
    int axis1, axis2, axis3;
    axis1 = 0; 			/* X-axis */
    axis2 = 1; 			/* Y-axis */
    axis3 = 2;			/* Z-axis */

    values = NULL;
    num[0] = num[1] = num[2] = nValues = 0;
    min[0] = min[1] = min[2] = max[0] = max[1] = max[2] = 0.0f;
    order = units[0] = units[1] = units[2] = units[3] = NULL;

    int i;
    for (i = 1; i < objc; i += 2) {
        const char *string;
        char c;

        string = Tcl_GetString(objv[i]);
        c = string[0];
        if ((c == 'x') && (strcmp(string, "xmin") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], min+2) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'x') && (strcmp(string, "xmax") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], max+2) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'x') && (strcmp(string, "xnum") == 0)) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], num+2) != TCL_OK) {
                return TCL_ERROR;
            }
            if (num[2] <= 0) {
                Tcl_AppendResult(interp, "bad xnum value: must be > 0",
                     (char *)NULL);
                return TCL_ERROR;
            }
        } else if ((c == 'x') && (strcmp(string, "xunits") == 0)) {
            units[2] = Tcl_GetString(objv[i+1]);
        } else if ((c == 'y') && (strcmp(string, "ymin") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], min+1) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'y') && (strcmp(string, "ymax") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], max+1) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'y') && (strcmp(string, "ynum") == 0)) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], num+1) != TCL_OK) {
                return TCL_ERROR;
            }
            if (num[1] <= 0) {
                Tcl_AppendResult(interp, "bad ynum value: must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
        } else if ((c == 'y') && (strcmp(string, "yunits") == 0)) {
            units[1] = Tcl_GetString(objv[i+1]);
        } else if ((c == 'z') && (strcmp(string, "zmin") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], min) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'z') && (strcmp(string, "zmax") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], max) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'z') && (strcmp(string, "znum") == 0)) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], num) != TCL_OK) {
                return TCL_ERROR;
            }
            if (num[0] <= 0) {
                Tcl_AppendResult(interp, "bad znum value: must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
        } else if ((c == 'z') && (strcmp(string, "zunits") == 0)) {
            units[0] = Tcl_GetString(objv[i+1]);
        } else if ((c == 'v') && (strcmp(string, "values") == 0)) {
            Tcl_Obj **vobj;

            if (Tcl_ListObjGetElements(interp, objv[i+1], &nValues, &vobj)
                != TCL_OK) {
                return TCL_ERROR;
            }
            _values = (float *)realloc(_values, sizeof(float) * nValues);
            int j;
            for (j = 0; j < nValues; j++) {
                if (GetFloatFromObj(interp, vobj[j], _values + j)!=TCL_OK) {
                    return TCL_ERROR;
                }
            }
        } else if ((c == 'u') && (strcmp(string, "units") == 0)) {
            _vUnits = strdup(Tcl_GetString(objv[i+1]));
        } else if ((c == 'c') && (strcmp(string, "components") == 0)) {
            int n;

            if (Tcl_GetIntFromObj(interp, objv[i+1], &n) != TCL_OK) {
                return TCL_ERROR;
            }
            if (n <= 0) {
                Tcl_AppendResult(interp, "bad extents value: must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
            _nComponents = n;
        } else if ((c == 'a') && (strcmp(string, "axisorder") == 0)) {
            Tcl_Obj **axes;
            int n;

            if (Tcl_ListObjGetElements(interp, objv[i+1], &n, &axes) 
                != TCL_OK) {
                return TCL_ERROR;
            }
            if (n != 3) {
                return TCL_ERROR;
            }
            if ((GetAxisFromObj(interp, axes[0], &axis1) != TCL_OK) ||
                (GetAxisFromObj(interp, axes[1], &axis2) != TCL_OK) ||
                (GetAxisFromObj(interp, axes[2], &axis3) != TCL_OK)) {
                return TCL_ERROR;
            }
        } else {
            Tcl_AppendResult(interp, "unknown key \"", string,
                (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (_values == NULL) {
        Tcl_AppendResult(interp, "missing \"values\" key", (char *)NULL);
        return TCL_ERROR;
    }
    if ((size_t)nValues != (num[0] * num[1] * num[2] * _nComponents)) {
        TRACE("num[0]=%d num[1]=%d num[2]=%d ncomponents=%d nValues=%d\n",
               num[0], num[1], num[2], _nComponents, nValues);
        Tcl_AppendResult(interp, 
                "wrong # of values: must be xnum*ynum*znum*extents", 
                         (char *)NULL);
       return TCL_ERROR;
    }
    
#ifdef notdef
    if ((axis1 != 0) || (axis2 != 1) || (axis3 != 2)) {
        // Reorder the data into x, y, z where x varies fastest and so on.
        int z;
        float *data, *dp;

        dp = data = new float[nValues];
        for (z = 0; z < num[0]; z++) {
            int y;

            for (y = 0; y < num[1]; y++) {
                int x;

                for (x = 0; x < num[2]; x++) {
                    int i;
                    
                    /* Compute the index from the data's described ordering. */
                    i = ((z*num[axis2]*num[axis3]) + (y*num[axis3]) + x) * 3;
                    for(size_t v = 0; v < _nComponents; v++) {
                        dp[v] = values[i+v];
                    }
                    dp += _nComponents;
                }
            }
        }
        delete [] values;
        values = data;
    }
#endif
    _nValues = nValues;
    if (units[3] != NULL) {
        if (_vUnits != NULL) {
            free(_vUnits);
        }
        _vUnits = strdup(units[3]);
    }
    _xMin = min[axis3];
    _xMax = max[axis3];
    _xNum = num[axis3];
    if (units[axis3] != NULL) {
        if (_xUnits != NULL) {
            free(_xUnits);
        }
        _xUnits = strdup(units[axis3]);
    }
    _yMin = min[axis2];
    _yMax = max[axis2];
    _yNum = num[axis2];
    if (units[axis2] != NULL) {
        if (_yUnits != NULL) {
            free(_yUnits);
        }
        _yUnits = strdup(units[axis2]);
    }
    _zMin = min[axis1];
    _zMax = max[axis1];
    _zNum = num[axis1];
    if (units[axis1] != NULL) {
        if (_zUnits != NULL) {
            free(_zUnits);
        }
        _zUnits = strdup(units[axis1]);
    }
    _initialized = true;
#ifdef notdef
    { 
        FILE *f;
        f = fopen("/tmp/unirect3d.txt", "w");
        fprintf(f, "unirect3d xmin %g xmax %g xnum %d ", _xMin, _xMax, _xNum);
        fprintf(f, "ymin %g ymax %g ynum %d ", _yMin, _yMax, _yNum);
        fprintf(f, "zmin %g zmax %g znum %d ", _zMin, _zMax, _zNum);
        fprintf(f, "components %d values {\n",  _nComponents);
        for (size_t i = 0; i < _nValues; i+= 3) {
            fprintf(f, "%g %g %g\n", _values[i], _values[i+1], _values[i+2]);
        }
        fprintf(f, "}\n");
        fclose(f);
    }
#endif
    return TCL_OK;
}


int 
Rappture::Unirect2d::LoadData(Tcl_Interp *interp, int objc, 
                              Tcl_Obj *const *objv)
{
    if ((objc & 0x01) == 0) {
        Tcl_AppendResult(interp, Tcl_GetString(objv[0]), ": ",
                "wrong number of arguments: should be key-value pairs",
                (char *)NULL);
        return TCL_ERROR;
    }

    int axis[2];
    axis[0] = 1;                         /* X-axis */
    axis[1] = 0;			/* Y-axis */

    _xNum = _yNum = _nValues = 0;
    _xMin = _yMin = _xMax = _yMax = 0.0f;
    if (_xUnits != NULL) {
        free(_xUnits);
    }
    if (_yUnits != NULL) {
        free(_yUnits);
    }
    if (_vUnits != NULL) {
        free(_vUnits);
    }
    _xUnits = _yUnits = _vUnits = NULL;
    _nValues = 0;

    int i;
    for (i = 1; i < objc; i += 2) {
        const char *string;
        char c;

        string = Tcl_GetString(objv[i]);
        c = string[0];
        if ((c == 'x') && (strcmp(string, "xmin") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], &_xMin) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'x') && (strcmp(string, "xmax") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], &_xMax) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'x') && (strcmp(string, "xnum") == 0)) {
            int n;
            if (Tcl_GetIntFromObj(interp, objv[i+1], &n) != TCL_OK) {
                return TCL_ERROR;
            }
            if (n <= 0) {
                Tcl_AppendResult(interp, "bad xnum value: must be > 0",
                     (char *)NULL);
                return TCL_ERROR;
            }
            _xNum = n;
        } else if ((c == 'x') && (strcmp(string, "xunits") == 0)) {
            _xUnits = strdup(Tcl_GetString(objv[i+1]));
        } else if ((c == 'y') && (strcmp(string, "ymin") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], &_yMin) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'y') && (strcmp(string, "ymax") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], &_yMax) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'y') && (strcmp(string, "ynum") == 0)) {
            int n;
            if (Tcl_GetIntFromObj(interp, objv[i+1], &n) != TCL_OK) {
                return TCL_ERROR;
            }
            if (n <= 0) {
                Tcl_AppendResult(interp, "bad ynum value: must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
            _yNum = n;
        } else if ((c == 'y') && (strcmp(string, "yunits") == 0)) {
            _yUnits = strdup(Tcl_GetString(objv[i+1]));
        } else if ((c == 'v') && (strcmp(string, "values") == 0)) {
            Tcl_Obj **vobj;
            int n;

            Tcl_IncrRefCount(objv[i+1]);
            if (Tcl_ListObjGetElements(interp, objv[i+1], &n, &vobj) != TCL_OK){
                return TCL_ERROR;
            }
            if (n <= 0) {
                Tcl_AppendResult(interp, "empty values list : must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
            _nValues = n;
            _values = (float *)realloc(_values, sizeof(float) * _nValues);
            size_t j;
            for (j = 0; j < _nValues; j++) {
                if (GetFloatFromObj(interp, vobj[j], _values + j)!=TCL_OK) {
                    return TCL_ERROR;
                }
            }
            Tcl_DecrRefCount(objv[i+1]);
        } else if ((c == 'u') && (strcmp(string, "units") == 0)) {
            _vUnits = strdup(Tcl_GetString(objv[i+1]));
        } else if ((c == 'c') && (strcmp(string, "components") == 0)) {
            int n;

            if (Tcl_GetIntFromObj(interp, objv[i+1], &n) != TCL_OK) {
                return TCL_ERROR;
            }
            if (n <= 0) {
                Tcl_AppendResult(interp, "bad extents value: must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
            _nComponents = n;
        } else if ((c == 'a') && (strcmp(string, "axisorder") == 0)) {
            Tcl_Obj **order;
            int n;

            if (Tcl_ListObjGetElements(interp, objv[i+1], &n, &order) 
                != TCL_OK) {
                return TCL_ERROR;
            }
            if (n != 2) {
                Tcl_AppendResult(interp, 
                        "wrong # of axes defined for ordering the data",
                        (char *)NULL);
                return TCL_ERROR;
            }
            if ((GetAxisFromObj(interp, order[0], axis) != TCL_OK) ||
                (GetAxisFromObj(interp, order[1], axis+1) != TCL_OK)) {
                return TCL_ERROR;
            }
        } else {
            Tcl_AppendResult(interp, "unknown key \"", string,
                (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (_nValues == 0) {
        Tcl_AppendResult(interp, "missing \"values\" key", (char *)NULL);
        return TCL_ERROR;
    }
    if (_nValues != (_xNum * _yNum * _nComponents)) {
        Tcl_AppendResult(interp, 
                "wrong number of values: must be xnum*ynum*components", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    
    if ((axis[0] != 1) || (axis[1] != 0)) {
        TRACE("reordering data\n");
        /* Reorder the data into x, y where x varies fastest and so on. */
        size_t y;
        float *dp;

        dp = _values = (float *)realloc(_values, sizeof(float) * _nValues);
        for (y = 0; y < _yNum; y++) {
            size_t x;

            for (x = 0; x < _xNum; x++) {
                size_t i, v;
                    
                /* Compute the index from the data's described ordering. */
                i = (y + (_yNum * x)) * _nComponents;
                for(v = 0; v < _nComponents; v++) {
                    dp[v] = _values[i+v];
                }
                dp += _nComponents;
            }
        }
    }
    _initialized = true;
    return TCL_OK;
}


bool
Rappture::Unirect3d::ImportDx(Rappture::Outcome &result, size_t nComponents,
                              size_t length, char *string) 
{
    int nx, ny, nz, npts;
    double x0, y0, z0, dx, dy, dz, ddx, ddy, ddz;
    char *p, *pend;

    dx = dy = dz = 0.0;			/* Suppress compiler warning. */
    x0 = y0 = z0 = 0.0;			/* May not have an origin line. */
    nx = ny = nz = npts = 0;		/* Suppress compiler warning. */
    for (p = string, pend = p + length; p < pend; /*empty*/) {
        char *line;

        line = getline(&p, pend);
        if (line == pend) {
            break;                        /* EOF */
        }
        if ((line[0] == '#') || (line == '\0')) {
            continue;                        /* Skip blank or comment lines. */
        }
        if (sscanf(line, "object %*d class gridpositions counts %d %d %d", 
                   &nx, &ny, &nz) == 3) {
            if ((nx < 0) || (ny < 0) || (nz < 0)) {
                result.addError("invalid grid size: x=%d, y=%d, z=%d", 
                        nx, ny, nz);
                return false;
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
                result.addError("bad # points %d", npts);
                return false;
            }
            TRACE("#points=%d\n", npts);
            if (npts != nx*ny*nz) {
                result.addError("inconsistent data: expected %d points"
                                " but found %d points", nx*ny*nz, npts);
                return false;
            }
            break;
        } else if (sscanf(line, "object %*d class array type %*s rank 0"
                " times %d data follows", &npts) == 1) {
            if (npts != nx*ny*nz) {
                result.addError("inconsistent data: expected %d points"
                                " but found %d points", nx*ny*nz, npts);
                return false;
            }
            break;
        }
    }
    if (npts != nx*ny*nz) {
        result.addError("inconsistent data: expected %d points"
                        " but found %d points", nx*ny*nz, npts);
        return false;
    }

    _initialized = false;
    _xValueMin = _yValueMin = _zValueMin = FLT_MAX;
    _xValueMax = _yValueMax = _zValueMax = -FLT_MAX;
    _xMin = x0, _yMin = y0, _zMin = z0;
    _xNum = nx, _yNum = ny, _zNum = nz;
    _xMax = _xMin + dx * _xNum;
    _yMax = _yMin + dy * _yNum;
    _zMax = _zMin + dz * _zNum;
    _nComponents = nComponents;

    _values = (float *)realloc(_values, sizeof(float) * npts * _nComponents);
    _nValues = 0;
    for (size_t ix = 0; ix < _xNum; ix++) {
        for (size_t iy = 0; iy < _yNum; iy++) {
            for (size_t iz = 0; iz < _zNum; iz++) {
                char *line;
                if ((p == pend) || (_nValues > (size_t)npts)) {
                    break;
                }
                line = getline(&p, pend);
                if ((line[0] == '#') || (line[0] == '\0')) {
                    continue;                /* Skip blank or comment lines. */
                }
                double vx, vy, vz;
                if (sscanf(line, "%lg %lg %lg", &vx, &vy, &vz) == 3) {
                    int nindex = (iz*nx*ny + iy*nx + ix) * 3;
                    if (vx < _xValueMin) {
                        _xValueMin = vx;
                    } else if (vx > _xValueMax) {
                        _xValueMax = vx;
                    }
                    if (vy < _yValueMin) {
                        _yValueMin = vy;
                    } else if (vy > _yValueMax) {
                        _yValueMax = vy;
                    }
                    if (vz < _zValueMin) {
                        _zValueMin = vz;
                    } else if (vz > _zValueMax) {
                        _zValueMax = vz;
                    }
                    _values[nindex] = vx;
                    _values[nindex+1] = vy;
                    _values[nindex+2] = vz;
                    _nValues++;
                }
            }
        }
    }
    /* Make sure that we read all of the expected points. */
    if (_nValues != (size_t)npts) {
        result.addError("inconsistent data: expected %d points"
                        " but found %d points", npts, _nValues);
        free(_values);
        _values = NULL;
        return false;
    }
    _nValues *= _nComponents;
    _initialized = true;
#ifdef notdef
    { 
        FILE *f;
        f = fopen("/tmp/dx.txt", "w");
        fprintf(f, "unirect3d xmin %g xmax %g xnum %d ", _xMin, _xMax, _xNum);
        fprintf(f, "ymin %g ymax %g ynum %d ", _yMin, _yMax, _yNum);
        fprintf(f, "zmin %g zmax %g znum %d ", _zMin, _zMax, _zNum);
        fprintf(f, "components %d values {\n",  _nComponents);
        for (size_t i = 0; i < _nValues; i+= 3) {
            fprintf(f, "%g %g %g\n", _values[i], _values[i+1], _values[i+2]);
        }
        fprintf(f, "}\n");
        fclose(f);
    }
#endif
    return true;
}


bool
Rappture::Unirect3d::Resample(Rappture::Outcome &result, size_t nSamples)
{
    Rappture::Mesh1D xgrid(_xMin, _xMax, _xNum);
    Rappture::Mesh1D ygrid(_yMin, _yMax, _yNum);
    Rappture::Mesh1D zgrid(_zMin, _zMax, _zNum);
    Rappture::FieldRect3D xfield(xgrid, ygrid, zgrid);
    Rappture::FieldRect3D yfield(xgrid, ygrid, zgrid);
    Rappture::FieldRect3D zfield(xgrid, ygrid, zgrid);

    size_t i, j;
    for (i = 0, j = 0; i < _nValues; i += _nComponents, j++) {
        double vx, vy, vz;

        vx = _values[i];
        vy = _values[i+1];
        vz = _values[i+2];
        
        xfield.define(j, vx);
        yfield.define(j, vy);
        zfield.define(j, vz);
    }
    // Figure out a good mesh spacing
    double dx, dy, dz;
    double lx, ly, lz;
    lx = xfield.rangeMax(Rappture::xaxis) - xfield.rangeMin(Rappture::xaxis);
    ly = xfield.rangeMax(Rappture::yaxis) - xfield.rangeMin(Rappture::yaxis);
    lz = xfield.rangeMax(Rappture::zaxis) - xfield.rangeMin(Rappture::zaxis);

    double dmin;
    dmin = pow((lx*ly*lz)/((nSamples-1)*(nSamples-1)*(nSamples-1)), 0.333);

    /* Recompute new number of points for each axis. */
    _xNum = (size_t)ceil(lx/dmin);
    _yNum = (size_t)ceil(ly/dmin);
    _zNum = (size_t)ceil(lz/dmin);

#ifndef HAVE_NPOT_TEXTURES
    // must be an even power of 2 for older cards
    _xNum = (int)pow(2.0, ceil(log10((double)_xNum)/log10(2.0)));
    _yNum = (int)pow(2.0, ceil(log10((double)_yNum)/log10(2.0)));
    _zNum = (int)pow(2.0, ceil(log10((double)_zNum)/log10(2.0)));
#endif

    dx = lx/(double)(_xNum-1);
    dy = ly/(double)(_yNum-1);
    dz = lz/(double)(_zNum-1);

    TRACE("lx:%lf ly:%lf lz:%lf dmin:%lf dx:%lf dy:%lf dz:%lf\n", lx, ly, lz, dmin, dx, dy, dz);

    size_t n = _nComponents * _xNum * _yNum * _zNum;
    _values = (float *)realloc(_values, sizeof(float) * n);
    memset(_values, 0, sizeof(float) * n);

    // Generate the uniformly sampled rectangle that we need for a volume
    float *destPtr = _values;
    for (size_t i = 0; i < _zNum; i++) {
        double z;

        z = _zMin + (i * dx);
        for (size_t j = 0; j < _yNum; j++) {
            double y;
                
            y = _yMin + (j * dy);
            for (size_t k = 0; k < _xNum; k++) {
                double x;

                x = _xMin + (k * dz);
                destPtr[0] = xfield.value(x, y, z);
                destPtr[1] = yfield.value(x, y, z);
                destPtr[2] = zfield.value(x, y, z);
            }
        }
    }
    _nValues = _xNum * _yNum * _zNum * _nComponents;
    return true;
}

void
Rappture::Unirect3d::GetVectorRange()
{
    assert(_nComponents == 3);
    TRACE("GetVectorRange\n");
    _magMax = -DBL_MAX, _magMin = DBL_MAX;
    size_t i;
    for (i = 0; i < _nValues; i += _nComponents) {
        double vx, vy, vz, vm;

        vx = _values[i];
        vy = _values[i+1];
        vz = _values[i+2];
                    
        vm = sqrt(vx*vx + vy*vy + vz*vz);
        if (vm > _magMax) {
            _magMax = vm;
        }
        if (vm < _magMin) {
            _magMin = vm;
        }
    }
}

bool 
Rappture::Unirect3d::Convert(Rappture::Unirect2d *dataPtr)
{
    _initialized = false;

    _xValueMin = dataPtr->xValueMin();
    _yValueMin = dataPtr->yValueMin();
    _zValueMin = 0.0;
    _xMin = dataPtr->xMin();
    _yMin = dataPtr->yMin();
    _zMin = 0.0;
    _xMax = dataPtr->xMax();
    _yMax = dataPtr->yMax();
    _zMax = 0.0;
    _xNum = dataPtr->xNum();
    _yNum = dataPtr->yNum();
    _zNum = 1;
    switch (dataPtr->nComponents()) {
    case 1:
    case 3:
        _values = (float *)realloc(_values, sizeof(float) * dataPtr->nValues());
        if (_values == NULL) {
            return false;
        }
        memcpy(_values, dataPtr->values(), dataPtr->nValues());
        _nValues = dataPtr->nValues();
        _nComponents = dataPtr->nComponents();
        break;
    case 2:
        float *values;
        _nValues = 3 * _xNum * _yNum * _zNum;
        _values = (float *)realloc(_values, sizeof(float) * _nValues);
        if (_values == NULL) {
            return false;
        }
        values = dataPtr->values();
        size_t i, j;
        for (j = i = 0; i < dataPtr->nValues(); i += 2, j+= 3) {
            _values[j] = values[i];
            _values[j+1] = values[i+1];
            _values[j+2] = 0.0f;
        }
        _nComponents = 3;
        break;
    }
    return true;
}
