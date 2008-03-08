/*
 * ----------------------------------------------------------------------
 *  Rappture::DX
 *
 *  Rappture DX object for file reading and interacting
 *  with libDX and friends.
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2008  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpDX.h"
#include <math.h>

#ifdef __cplusplus
    extern "C" {
#endif // ifdex __cplusplus

using namespace Rappture;

DX::DX(const char* filename)
    : _dataMin(0),
      _dataMax(0),
      _nzero_min(0),
      _numAxis(0),
      _axisLen(NULL),
      _data(NULL),
      _n(0),
      _rank(0),
      _shape(0),
      _positions(NULL),
      _delta(NULL),
      _max(NULL),
      _origin(NULL)
{
    Array dxarr;
    // category and type are probably not needed
    // we keep them around in case they hold useful info for the future
    // we could replace them with NULL inthe DXGetArrayInfo fxn call
    Category category;
    Type type;

    if (filename == NULL) {
        // error
    }

    // open the file with libdx
    fprintf(stdout, "Calling DXImportDX(%s)\n", filename);
    fflush(stdout);
    _dxobj = DXImportDX((char*)filename,NULL,NULL,NULL,NULL);

    fprintf(stdout, "dxobj=%x\n", (unsigned int)_dxobj);
    fflush(stdout);

    // parse out the positions array
    dxarr = (Array) DXGetComponentValue((Field) _dxobj, (char *)"positions");
    DXGetArrayInfo(dxarr, &_n, &type, &category, &_rank, &_shape);

    _positions = (float*) DXGetArrayData(dxarr);
    if (_positions == NULL) {
        fprintf(stdout, "DXGetArrayData failed to return positions array\n");
        fflush(stdout);
    }

    _numAxis = _rank*_shape;
    _axisLen = new int[_numAxis];
    if (_axisLen == NULL) {
        // malloc failed, raise error
        fprintf(stdout, "malloc of _axisLen array failed");
        fflush(stdout);
    }

    _delta = new float[_numAxis];
    if (_delta == NULL) {
        // malloc failed, raise error
        fprintf(stdout, "malloc of _delta array failed");
        fflush(stdout);
    }

    _max = new float[_numAxis];
    if (_max == NULL) {
        // malloc failed, raise error
        fprintf(stdout, "malloc of _max array failed");
        fflush(stdout);
    }

    __findPosDeltaMax();

    for (int lcv = 0; lcv < _numAxis; lcv++) {
        if (_delta[lcv] == 0) {
            // div by 0, raise error
            fprintf(stdout, "delta is 0, can't divide by zero\n");
            fflush(stdout);
        }
        // FIXME: find a way to grab the number of points per axis in dx
        // this is a horrible way to find the number of points
        // per axis and only works when each axis has equal number of pts
        _axisLen[lcv] = (int)ceil(pow((double)_n,(double)(1.0/_numAxis)));

    }

    fprintf(stdout, "_max = [%lg,%lg,%lg]\n",_max[0],_max[1],_max[2]);
    fprintf(stdout, "_delta = [%lg,%lg,%lg]\n",_delta[0],_delta[1],_delta[2]);
    fprintf(stdout, "_axisLen = [%i,%i,%i]\n",_axisLen[0],_axisLen[1],_axisLen[2]);
    fflush(stdout);

    _data = new float[_n];
    if (_data == NULL) {
        // malloc failed, raise error
        fprintf(stdout, "malloc of _data array failed");
        fflush(stdout);
    }

    __getInterpData2();

}

DX::~DX()
{
    delete[] _axisLen;
    delete[] _delta;
    delete[] _max;
    delete[] _data;
    delete[] _positions;
}

void
DX::__findPosDeltaMax()
{
    int lcv = 0;

    // initialize the max array and delta array
    // max holds the maximum value found for each index
    // delta holds the difference between each entry's value
    for (lcv = 0; lcv < _numAxis; lcv++) {
        _max[lcv] = _positions[lcv];
        _delta[lcv] = _positions[lcv];
    }

    for (lcv=lcv; lcv < _n*_numAxis; lcv++) {
        if (_positions[lcv] > _max[lcv%_numAxis]) {
            _max[lcv%_numAxis] = _positions[lcv];
        }
        if (_delta[lcv%_numAxis] == _positions[lcv%_numAxis]) {
            if (_positions[lcv] != _positions[lcv-_numAxis]) {
                _delta[lcv%_numAxis] = _positions[lcv] - _positions[lcv-_numAxis];
            }
        }
    }
}

void
DX::__getInterpData2()
{
    int pts = _n;
    int interppts = pts;
    Interpolator interpolator;

    // build the interpolator and interpolate
    fprintf(stdout, "creating DXNewInterpolator...\n");
    fflush(stdout);
    interpolator = DXNewInterpolator(_dxobj,INTERP_INIT_IMMEDIATE,-1.0);
    fprintf(stdout, "DXNewInterpolator=%x\n", (unsigned int)interpolator);
    fprintf(stdout,"_rank = %i\n",_rank);
    fprintf(stdout,"_shape = %i\n",_shape);
    fprintf(stdout,"_n = %i\n",_n);
    fprintf(stdout,"start interppts = %i\n",interppts);
    fflush(stdout);
    DXInterpolate(interpolator,&interppts,_positions,_data);
    fprintf(stdout,"interppts = %i\n",interppts);
    fflush(stdout);

    // print debug info
    for (int lcv = 0, pt = 0; lcv < pts; lcv+=3,pt+=9) {
        fprintf(stdout,
            "(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n",
            _positions[pt],_positions[pt+1],_positions[pt+2], _data[lcv],
            _positions[pt+3],_positions[pt+4],_positions[pt+5],_data[lcv+1],
            _positions[pt+6],_positions[pt+7],_positions[pt+8],_data[lcv+2]);
        fflush(stdout);
    }

    for (int lcv = 0; lcv < pts; lcv=lcv+3) {
        if (_data[lcv] < _dataMin) {
            _dataMin = _data[lcv];
            if (_dataMin != 0) {
                _nzero_min = _dataMin;
            }
        }
        if (_data[lcv] > _dataMax) {
            _dataMax = _data[lcv];
        }
    }
}


int
DX::n() const
{
    return _n;
}

int
DX::rank() const
{
    return _rank;
}

int
DX::shape() const
{
    return _shape;
}

const float*
DX::delta() const
{
    return _delta;
}

const float*
DX::max() const
{
    return _max;
}

const float*
DX::origin() const
{
    return _origin;
}

const float*
DX::positions() const
{
    return _positions;
}

const int*
DX::axisLen() const
{
    return _axisLen;
}

const float*
DX::data() const
{
    return _data;
}

float
DX::dataMin() const
{
    return _dataMin;
}

float
DX::dataMax() const
{
    return _dataMax;
}

float
DX::nzero_min() const
{
    return _nzero_min;
}

#ifdef __cplusplus
    }
#endif // ifdef __cplusplus
