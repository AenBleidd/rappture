
#include <tcl.h>
#include <Unirect.h>

extern int GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	float *valuePtr);
extern int GetAxisFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *indexPtr);

int 
Rappture::Unirect3d::LoadData(Tcl_Interp *interp, int objc, 
			      Tcl_Obj *const *objv)
{
    int num[3], nValues;
    float min[3], max[3];
    float *values;
    const char *units[4], *order;


    if ((objc & 0x01) == 0) {
        Tcl_AppendResult(interp, Tcl_GetString(objv[0]), ": ",
                "wrong number of arguments: should be key-value pairs",
                (char *)NULL);
        return TCL_ERROR;
    }

    /* Default order is  z, y, x. */
    int axis1, axis2, axis3;
    axis1 = 2; 			/* Z-axis */
    axis2 = 1; 			/* Y-axis */
    axis3 = 0;			/* X-axis */

    int extents = 1;
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
            values = new float[nValues];
            int j;
            for (j = 0; j < nValues; j++) {
                if (GetFloatFromObj(interp, vobj[j], values + j)!=TCL_OK) {
                    return TCL_ERROR;
		}
	    }
        } else if ((c == 'u') && (strcmp(string, "units") == 0)) {
            vUnits_ = strdup(Tcl_GetString(objv[i+1]));
        } else if ((c == 'o') && (strcmp(string, "order") == 0)) {
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
    if (values == NULL) {
        Tcl_AppendResult(interp, "missing \"values\" key", (char *)NULL);
        return TCL_ERROR;
    }
    if (nValues != (num[0] * num[1] * num[2] * extents)) {
        Tcl_AppendResult(interp, 
		"wrong number of values: must be xnum*ynum*znum*extents", 
			 (char *)NULL);
        return TCL_ERROR;
    }
    
    fprintf(stderr, "generating %dx%dx%dx%d = %d points\n", 
	    num[0], num[1], num[2], extents, num[0]*num[1]*num[2]*extents);

    if ((axis1 != 2) || (axis2 != 1) || (axis3 != 0)) {
	// Reorder the data into x, y, z where x varies fastest and so on.
	int z;
	float *data, *dp;

	dp = data = new float[nValues];
	for (z = 0; z < num[0]; z++) {
	    int y;

	    for (y = 0; y < num[1]; y++) {
		int x;

		for (x = 0; x < num[2]; x++) {
		    int i, v;
		    
		    /* Compute the index from the data's described ordering. */
		    i = ((z*num[axis2]*num[axis3]) + (y*num[axis3]) + x) * 3;
		    for(v = 0; v < extents; v++) {
			dp[v] = values[i+v];
		    }
		    dp += extents;
		}
	    }
	}
	delete [] values;
	values = data;
    }

    values_ = values;
    nValues_ = nValues;
    extents_ = extents;
    if (units[3] != NULL) {
	vUnits_ = strdup(units[3]);
    }
    xMin_ = min[axis3];
    xMax_ = max[axis3];
    xNum_ = num[axis3];
    if (units[axis3] != NULL) {
	xUnits_ = strdup(units[axis3]);
    }
    yMin_ = min[axis2];
    yMax_ = max[axis2];
    yNum_ = num[axis2];
    if (units[axis2] != NULL) {
	yUnits_ = strdup(units[axis2]);
    }
    zMin_ = min[axis1];
    zMax_ = max[axis1];
    zNum_ = num[axis1];
    if (units[axis1] != NULL) {
	zUnits_ = strdup(units[axis1]);
    }
    initialized_ = true;
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
    axis[0] = 1; 			/* X-axis */
    axis[1] = 0;			/* Y-axis */

    extents_ = 1;
    xNum_ = yNum_ = nValues_ = 0;
    xMin_ = yMin_ = xMax_ = yMax_ = 0.0f;
    if (xUnits_ != NULL) {
	free(xUnits_);
    }
    if (yUnits_ != NULL) {
	free(yUnits_);
    }
    if (vUnits_ != NULL) {
	free(vUnits_);
    }
    xUnits_ = yUnits_ = vUnits_ = NULL;
    if (values_ != NULL) {
	delete [] values_;
    }
    values_ = NULL;

    int i;
    for (i = 1; i < objc; i += 2) {
        const char *string;
        char c;

        string = Tcl_GetString(objv[i]);
        c = string[0];
        if ((c == 'x') && (strcmp(string, "xmin") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], &xMin_) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'x') && (strcmp(string, "xmax") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], &xMax_) != TCL_OK) {
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
	    xNum_ = n;
        } else if ((c == 'x') && (strcmp(string, "xunits") == 0)) {
            xUnits_ = strdup(Tcl_GetString(objv[i+1]));
        } else if ((c == 'y') && (strcmp(string, "ymin") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], &yMin_) != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'y') && (strcmp(string, "ymax") == 0)) {
            if (GetFloatFromObj(interp, objv[i+1], &yMax_) != TCL_OK) {
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
	    yNum_ = n;
        } else if ((c == 'y') && (strcmp(string, "yunits") == 0)) {
            yUnits_ = strdup(Tcl_GetString(objv[i+1]));
        } else if ((c == 'v') && (strcmp(string, "values") == 0)) {
            Tcl_Obj **vobj;
	    int n;

            if (Tcl_ListObjGetElements(interp, objv[i+1], &n, &vobj) != TCL_OK){
                return TCL_ERROR;
            }
	    if (n <= 0) {
                Tcl_AppendResult(interp, "empty values list : must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
	    }
	    nValues_ = n;
            values_ = new float[nValues_];
            size_t j;
            for (j = 0; j < nValues_; j++) {
                if (GetFloatFromObj(interp, vobj[j], values_ + j)!=TCL_OK) {
                    return TCL_ERROR;
		}
	    }
        } else if ((c == 'u') && (strcmp(string, "units") == 0)) {
            vUnits_ = strdup(Tcl_GetString(objv[i+1]));
        } else if ((c == 'e') && (strcmp(string, "extents") == 0)) {
	    int n;

	    if (Tcl_GetIntFromObj(interp, objv[i+1], &n) != TCL_OK) {
                return TCL_ERROR;
            }
	    if (n <= 0) {
                Tcl_AppendResult(interp, "bad extents value: must be > 0",
                                 (char *)NULL);
                return TCL_ERROR;
            }
	    extents_ = n;
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
    if (values_ == NULL) {
        Tcl_AppendResult(interp, "missing \"values\" key", (char *)NULL);
        return TCL_ERROR;
    }
    if (nValues_ != (xNum_ * yNum_ * extents_)) {
        Tcl_AppendResult(interp, 
		"wrong number of values: must be xnum*ynum*extents", 
			 (char *)NULL);
        return TCL_ERROR;
    }
    
    fprintf(stderr, "generating %dx%dx%d = %d points\n", 
	    xNum_, yNum_, extents_, xNum_ * yNum_ * extents_);

#ifndef notdef
    if ((axis[0] != 1) || (axis[1] != 0)) {
	fprintf(stderr, "reordering data\n");
	// Reorder the data into x, y where x varies fastest and so on.
	size_t y;
	float *data, *dp;

	dp = data = new float[nValues_];
	for (y = 0; y < yNum_; y++) {
	    size_t x;

	    for (x = 0; x < xNum_; x++) {
		size_t i, v;
		    
		/* Compute the index from the data's described ordering. */
		i = (y + (yNum_ * x)) * extents_;
		for(v = 0; v < extents_; v++) {
		    dp[v] = values_[i+v];
		}
		dp += extents_;
	    }
	}
	delete [] values_;
	values_ = data;
    }
#endif
    initialized_ = true;
    return TCL_OK;
}
