
#ifndef _UNIRECT_H
#define _UNIRECT_H
#include <rappture.h>

namespace Rappture {

class Unirect3d {
    size_t _xNum, _yNum, _zNum;
    size_t _nValues;
    int _extents;

    float _xMin, _xMax;
    float _yMin, _yMax;
    float _zMin, _zMax;
    float _vMin, _vMax;

    char *_xUnits;
    char *_yUnits;
    char *_zUnits;
    char *_vUnits;

    float *_values;
    bool _initialized;

public:
    Unirect3d(void) {
	_values = NULL;
	_initialized = false;
	_xNum = _yNum = _zNum = 0;
	_nValues = 0;
	_xUnits = _yUnits = _zUnits = _vUnits = NULL;
    }
    ~Unirect3d(void) {
	if (_values != NULL) {
	    delete [] _values;
	}
	if (_xUnits != NULL) {
	    free(_xUnits);
	}
	if (_yUnits != NULL) {
	    free(_yUnits);
	}
	if (_zUnits != NULL) {
	    free(_zUnits);
	}
	if (_vUnits != NULL) {
	    free(_vUnits);
	}
    }
    size_t xNum(void) {
	return _xNum;
    }
    size_t yNum(void) {
	return _yNum;
    }
    size_t zNum(void) {
	return _zNum;
    }
    float xMin(void) {
	return _xMin;
    }
    float yMin(void) {
	return _yMin;
    }
    float zMin(void) {
	return _zMin;
    }
    float xMax(void) {
	return _xMax;
    }
    float yMax(void) {
	return _yMax;
    }
    float zMax(void) {
	return _zMax;
    }
    const char *xUnits(void) {
	return _xUnits;
    }
    const char *yUnits(void) {
	return _yUnits;
    }
    const char *zUnits(void) {
	return _zUnits;
    }
    const char *vUnits(void) {
	return _vUnits;
    }
    float *values(void) {
	return _values;
    }
    float *SaveValues(void) {
	float *values;
	values = _values;
	_values = NULL;
	_nValues = 0;
	return values;
    }
    size_t nValues(void) {
	return _nValues;
    }
    size_t extents(void) {
	return _extents;
    }
    void extents(size_t extents) {
	_extents = extents;
    }
    int LoadData(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);
    bool isInitialized(void) {
	return _initialized;
    }
};

class Unirect2d {
    size_t _xNum, _yNum;
    size_t _nValues;
    size_t _extents;

    float _xMin, _xMax;
    float _yMin, _yMax;
    float _vMin, _vMax;

    char *_xUnits;
    char *_yUnits;
    char *_vUnits;

    float *_values;
    bool _initialized;

public:
    Unirect2d(void) {
	_values = NULL;
	_initialized = false;
	_xNum = _yNum = 0;
	_nValues = 0;
	_xUnits = _yUnits = _vUnits = NULL;
    }
    ~Unirect2d(void) {
	if (_values != NULL) {
	    delete [] _values;
	}
	if (_xUnits != NULL) {
	    free(_xUnits);
	}
	if (_yUnits != NULL) {
	    free(_yUnits);
	}
	if (_vUnits != NULL) {
	    free(_vUnits);
	}
    }
    size_t xNum(void) {
	return _xNum;
    }
    size_t yNum(void) {
	return _yNum;
    }
    float xMin(void) {
	return _xMin;
    }
    float yMin(void) {
	return _yMin;
    }
    float xMax(void) {
	return _xMax;
    }
    float yMax(void) {
	return _yMax;
    }
    const char *xUnits(void) {
	return _xUnits;
    }
    const char *yUnits(void) {
	return _yUnits;
    }
    const char *vUnits(void) {
	return _vUnits;
    }
    float *values(void) {
	return _values;
    }
    float *acceptValues(void) {
	float *values;
	values = _values;
	_values = NULL;
	_nValues = 0;
	return values;
    }
    size_t nValues(void) {
	return _nValues;
    }
    size_t extents(void) {
	return _extents;
    }
    void extents(size_t extents) {
	_extents = extents;
    }
    int LoadData(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);
    bool isInitialized(void) {
	return _initialized;
    }
};

}

#endif /*_UNIRECT_H*/

