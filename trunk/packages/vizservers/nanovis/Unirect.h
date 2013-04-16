/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */
#ifndef RAPPTURE_UNIRECT_H
#define RAPPTURE_UNIRECT_H

#include <float.h>

#include <tcl.h>

#include <rappture.h>

#include <vrmath/Vector3f.h>

#include "Trace.h"

namespace Rappture {

class Unirect2d;

class Unirect3d
{
public:
    Unirect3d(float xMin, float xMax, size_t xNum,
              float yMin, float yMax, size_t yNum,
              float zMin, float zMax, size_t zNum,
              size_t nValues, float *values, size_t nComponents) :
        _xNum(xNum), _yNum(yNum), _zNum(zNum), 
        _nValues(nValues), 
        _nComponents(nComponents),
        _xMin(xMin), _xMax(xMax), 
        _yMin(yMin), _yMax(yMax), 
        _zMin(zMin), _zMax(zMax),
        _xValueMin(FLT_MAX), _xValueMax(-FLT_MAX),
        _yValueMin(FLT_MAX), _yValueMax(-FLT_MAX),
        _zValueMin(FLT_MAX), _zValueMax(-FLT_MAX),
        _magMin(DBL_MAX), _magMax(-DBL_MAX),
        _xUnits(NULL), _yUnits(NULL), _zUnits(NULL), _vUnits(NULL),
        _values(NULL),
        _initialized(false)
    {
        _initialized = true;
    }

    Unirect3d(size_t nComponents = 1) :
        _nValues(0), 
        _nComponents(nComponents),
        _xValueMin(FLT_MAX), _xValueMax(-FLT_MAX),
        _yValueMin(FLT_MAX), _yValueMax(-FLT_MAX),
        _zValueMin(FLT_MAX), _zValueMax(-FLT_MAX),
        _magMin(DBL_MAX), _magMax(-DBL_MAX),
        _xUnits(NULL), _yUnits(NULL), _zUnits(NULL), _vUnits(NULL),
        _values(NULL),
        _initialized(false)
    {
        _nComponents = nComponents;
    }

    ~Unirect3d()
    {
        if (_values != NULL) {
            free(_values);
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

    size_t xNum()
    {
        return _xNum;
    }

    size_t yNum()
    {
        return _yNum;
    }

    size_t zNum()
    {
        return _zNum;
    }

    float xMin()
    {
        return _xMin;
    }

    float yMin()
    {
        return _yMin;
    }

    float zMin()
    {
        return _zMin;
    }

    float xMax()
    {
        return _xMax;
    }

    float yMax()
    {
        return _yMax;
    }

    float zMax()
    {
        return _zMax;
    }

    float xValueMin()
    {
        return _xValueMin;
    }

    float yValueMin()
    {
        return _yValueMin;
    }

    float zValueMin()
    {
        return _zValueMin;
    }

    float xValueMax()
    {
        return _xValueMax;
    }

    float yValueMax()
    {
        return _yValueMax;
    }

    float zValueMax()
    {
        return _zValueMax;
    }

    size_t nComponents()
    {
        return _nComponents;
    }

    const char *xUnits()
    {
        return _xUnits;
    }

    const char *yUnits()
    {
        return _yUnits;
    }
    const char *zUnits()
    {
        return _zUnits;
    }

    const char *vUnits()
    {
        return _vUnits;
    }

    const float *values()
    {
        return _values;
    }

    double magMin()
    {
        if (_magMin == DBL_MAX) {
            getVectorRange();
        }
        return _magMin;
    }

    double magMax()
    {
        if (_magMax == -DBL_MAX) {
            getVectorRange();
        }
        return _magMax;
    }

    void getBounds(vrmath::Vector3f& bboxMin,
                   vrmath::Vector3f& bboxMax) const
    {
        bboxMin.set(_xMin, _yMin, _zMin);
        bboxMax.set(_xMax, _yMax, _zMax);
    }

    const float *SaveValues()
    {
        float *values;
        values = _values;
        _values = NULL;
        _nValues = 0;
        return values;
    }

    size_t nValues()
    {
        return _nValues;
    }

    int loadData(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);

    int parseBuffer(Tcl_Interp *interp, Rappture::Buffer &buf);

    bool importDx(Rappture::Outcome &result, size_t nComponents, 
                  size_t length, char *string);

    bool convert(Unirect2d *dataPtr);

    bool resample(Rappture::Outcome &context, size_t nSamples = 30);

    bool isInitialized()
    {
        return _initialized;
    }

private:
    void getVectorRange();

    size_t _xNum, _yNum, _zNum;
    size_t _nValues;
    size_t _nComponents;
    float _xMin, _xMax, _yMin, _yMax, _zMin, _zMax;
    float _xValueMin, _xValueMax;
    float _yValueMin, _yValueMax;
    float _zValueMin, _zValueMax;
    double _magMin, _magMax;                /* Range of magnitudes of vector
                                         * data. */
    char *_xUnits;
    char *_yUnits;
    char *_zUnits;
    char *_vUnits;

    float *_values;
    bool _initialized;
};

class Unirect2d
{
public:
    Unirect2d(size_t nComponents = 1) :
        _xNum(0), _yNum(0),
        _nValues(0),
        _nComponents(nComponents),
        _xUnits(NULL), _yUnits(NULL), _vUnits(NULL),
        _values(NULL),
        _initialized(false)
    {
    }

    ~Unirect2d()
    {
        if (_values != NULL) {
            free(_values);
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

    size_t xNum()
    {
        return _xNum;
    }

    size_t yNum()
    {
        return _yNum;
    }

    float xMin()
    {
        return _xMin;
    }

    float yMin()
    {
        return _yMin;
    }

    float xMax()
    {
        return _xMax;
    }

    float yMax()
    {
        return _yMax;
    }

    const char *xUnits()
    {
        return _xUnits;
    }

    const char *yUnits()
    {
        return _yUnits;
    }

    const char *vUnits()
    {
        return _vUnits;
    }

    float *values()
    {
        return _values;
    }

    float xValueMin()
    {
        return _xValueMin;
    }

    float yValueMin()
    {
        return _yValueMin;
    }

    float xValueMax()
    {
        return _xValueMax;
    }

    float yValueMax()
    {
        return _yValueMax;
    }

    size_t nComponents()
    {
        return _nComponents;
    }

    void getBounds(vrmath::Vector3f& bboxMin,
                   vrmath::Vector3f& bboxMax) const
    {
        bboxMin.set(_xMin, _yMin, 0);
        bboxMax.set(_xMax, _yMax, 0);
    }

    float *transferValues()
    {
        float *values;
        values = _values;
        _values = NULL;
        _nValues = 0;
        return values;
    }

    size_t nValues()
    {
        return _nValues;
    }

    int loadData(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv);

    int parseBuffer(Tcl_Interp *interp, Rappture::Buffer &buf);

    bool isInitialized()
    {
        return _initialized;
    }

private:
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
};

}

#endif
