
#ifndef _UNIRECT_H
#define _UNIRECT_H
#include <rappture.h>

namespace Rappture {

class Unirect3d {
    size_t xNum_, yNum_, zNum_;
    size_t nValues_;
    int extents_;

    float xMin_, xMax_;
    float yMin_, yMax_;
    float zMin_, zMax_;
    float vMin_, vMax_;

    char *xUnits_;
    char *yUnits_;
    char *zUnits_;
    char *vUnits_;

    float *values_;
    bool initialized_;

public:
    Unirect3d(float xMin, float xMax, size_t xNum, 
	      float yMin, float yMax, size_t yNum, 
	      float zMin, float zMax, size_t zNum, 
	      size_t nValues, float *values) {
	xMax_ = xMax;
	xMin_ = xMin;
	xNum_ = xNum;
	yMax_ = yMax;
	yMin_ = yMin;
	yNum_ = yNum;
	zMax_ = zMax;
	zMin_ = zMin;
	zNum_ = zNum;
	nValues_ = nValues;
	values_ = values;
	initialized_ = true;
    }

    Unirect3d(void) {
	values_ = NULL;
	initialized_ = false;
	xNum_ = yNum_ = zNum_ = 0;
	nValues_ = 0;
	xUnits_ = yUnits_ = zUnits_ = vUnits_ = NULL;
    }
    ~Unirect3d(void) {
	if (values_ != NULL) {
	    delete [] values_;
	}
	if (xUnits_ != NULL) {
	    free(xUnits_);
	}
	if (yUnits_ != NULL) {
	    free(yUnits_);
	}
	if (zUnits_ != NULL) {
	    free(zUnits_);
	}
	if (vUnits_ != NULL) {
	    free(vUnits_);
	}
    }
    size_t xNum(void) {
	return xNum_;
    }
    size_t yNum(void) {
	return yNum_;
    }
    size_t zNum(void) {
	return zNum_;
    }
    float xMin(void) {
	return xMin_;
    }
    float yMin(void) {
	return yMin_;
    }
    float zMin(void) {
	return zMin_;
    }
    float xMax(void) {
	return xMax_;
    }
    float yMax(void) {
	return yMax_;
    }
    float zMax(void) {
	return zMax_;
    }
    const char *xUnits(void) {
	return xUnits_;
    }
    const char *yUnits(void) {
	return yUnits_;
    }
    const char *zUnits(void) {
	return zUnits_;
    }
    const char *vUnits(void) {
	return vUnits_;
    }
    float *values(void) {
	return values_;
    }
    float *SaveValues(void) {
	float *values;
	values = values_;
	values_ = NULL;
	nValues_ = 0;
	return values;
    }
    size_t nValues(void) {
	return nValues_;
    }
    size_t extents(void) {
	return extents_;
    }
    void extents(size_t extents) {
	extents_ = extents;
    }
    int LoadData(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);
    bool isInitialized(void) {
	return initialized_;
    }
};

class Unirect2d {
    size_t xNum_, yNum_;
    size_t nValues_;
    size_t extents_;

    float xMin_, xMax_;
    float yMin_, yMax_;
    float vMin_, vMax_;

    char *xUnits_;
    char *yUnits_;
    char *vUnits_;

    float *values_;
    bool initialized_;

public:
    Unirect2d(void) {
	values_ = NULL;
	initialized_ = false;
	xNum_ = yNum_ = 0;
	nValues_ = 0;
	xUnits_ = yUnits_ = vUnits_ = NULL;
    }
    ~Unirect2d(void) {
	if (values_ != NULL) {
	    delete [] values_;
	}
	if (xUnits_ != NULL) {
	    free(xUnits_);
	}
	if (yUnits_ != NULL) {
	    free(yUnits_);
	}
	if (vUnits_ != NULL) {
	    free(vUnits_);
	}
    }
    size_t xNum(void) {
	return xNum_;
    }
    size_t yNum(void) {
	return yNum_;
    }
    float xMin(void) {
	return xMin_;
    }
    float yMin(void) {
	return yMin_;
    }
    float xMax(void) {
	return xMax_;
    }
    float yMax(void) {
	return yMax_;
    }
    const char *xUnits(void) {
	return xUnits_;
    }
    const char *yUnits(void) {
	return yUnits_;
    }
    const char *vUnits(void) {
	return vUnits_;
    }
    float *values(void) {
	return values_;
    }
    float *acceptValues(void) {
	float *values;
	values = values_;
	values_ = NULL;
	nValues_ = 0;
	return values;
    }
    size_t nValues(void) {
	return nValues_;
    }
    size_t extents(void) {
	return extents_;
    }
    void extents(size_t extents) {
	extents_ = extents;
    }
    int LoadData(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);
    bool isInitialized(void) {
	return initialized_;
    }
};

}

#endif /*_UNIRECT_H*/

