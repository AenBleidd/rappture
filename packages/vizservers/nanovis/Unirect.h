
#ifndef _UNIRECT_H
#define _UNIRECT_H
#include <rappture.h>

namespace Rappture {

class Unirect3d {
    size_t _xNum, _yNum, _zNum;
    size_t _nValues;
    size_t _nComponents;
    float _xMin, _xMax, _yMin, _yMax, _zMin, _zMax;
    float _xValueMin, _xValueMax;
    float _yValueMin, _yValueMax;
    float _zValueMin, _zValueMax;
    char *_xUnits;
    char *_yUnits;
    char *_zUnits;
    char *_vUnits;

    float *_values;
    bool _initialized;

public:
    Unirect3d(float xMin, float xMax, size_t xNum, 
	      float yMin, float yMax, size_t yNum, 
	      float zMin, float zMax, size_t zNum, 
	      size_t nValues, float *values, size_t nComponents) 
    {
	SetValues(xMin, xMax, xNum, yMin, yMax, yNum, zMin, zMax, zNum, 
		  nValues, values, nComponents);
    }

    Unirect3d(size_t nComponents = 1) {
	_values = NULL;
	_initialized = false;
	_xNum = _yNum = _zNum = 0;
	_nValues = 0;
	_xUnits = _yUnits = _zUnits = _vUnits = NULL;
	_nComponents = nComponents;
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
    void SetValues(float xMin, float xMax, size_t xNum, 
		   float yMin, float yMax, size_t yNum, 
		   float zMin, float zMax, size_t zNum, 
		   size_t nValues, float *values, size_t nComponents) {
	_xMax = xMax;
	_xMin = xMin;
	_xNum = xNum;
	_yMax = yMax;
	_yMin = yMin;
	_yNum = yNum;
	_zMax = zMax;
	_zMin = zMin;
	_zNum = zNum;
	_nValues = nValues;
	_values = values;
	_initialized = true;
	_nComponents = nComponents;
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
    float xValueMin(void) {
	return _xValueMin;
    }
    float yValueMin(void) {
	return _yValueMin;
    }
    float zValueMin(void) {
	return _zValueMin;
    }
    float xValueMax(void) {
	return _xValueMax;
    }
    float yValueMax(void) {
	return _yValueMax;
    }
    float zValueMax(void) {
	return _zValueMax;
    }
    size_t nComponents(void) {
	return _nComponents;
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
    const float *values(void) {
	return _values;
    }
    const float *SaveValues(void) {
	float *values;
	values = _values;
	_values = NULL;
	_nValues = 0;
	return values;
    }
    size_t nValues(void) {
	return _nValues;
    }
    int LoadData(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);

    bool ReadVectorDataFromDx(Rappture::Outcome &result, size_t length, 
	char *string);
    bool Resample(Rappture::Outcome &context, int nSamples = 30);
    bool isInitialized(void) {
	return _initialized;
    }
};

class Unirect2d {
    size_t _xNum, _yNum;
    size_t _nValues;
    size_t _nComponents;

    float _xMin, _xMax;
    float _yMin, _yMax;
    float _xValueMin, _xValueMax;
    float _yValueMin, _yValueMax;

    char *_xUnits;
    char *_yUnits;
    char *_vUnits;

    float *_values;
    bool _initialized;

public:
    Unirect2d(size_t nComponents = 1) {
	_values = NULL;
	_initialized = false;
	_xNum = _yNum = 0;
	_nValues = 0;
	_xUnits = _yUnits = _vUnits = NULL;
	_nComponents = nComponents;
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
    int LoadData(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);
    bool isInitialized(void) {
	return _initialized;
    }
};

}

#endif /*_UNIRECT_H*/

