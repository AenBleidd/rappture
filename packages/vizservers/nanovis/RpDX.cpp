/*
 * ----------------------------------------------------------------------
 *  Rappture::DX
 *
 *  Rappture DX object for file reading and interacting
 *  with libDX and friends.
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "nvconf.h"
#ifdef HAVE_DX_DX_H 
#include "RpDX.h"
#undef ERROR
#include "Trace.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

using namespace Rappture;

DX::DX(Outcome &result, const char* filename) :
    _dataMin(FLT_MAX),
    _dataMax(-FLT_MAX),
    _nzero_min(FLT_MAX),
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
    Array dxpos;
    Array dxdata;
    Array dxgrid;
    // category and type are probably not needed
    // we keep them around in case they hold useful info for the future
    // we could replace them with NULL inthe DXGetArrayInfo fxn call
    Category category;
    Type type;

    if (filename == NULL) {
        result.addError("filename is NULL");
        return;
    }
    // open the file with libdx
    TRACE("Calling DXImportDX(%s)\n", filename);
    DXenable_locks(0);
    _dxobj = DXImportDX((char*)filename,NULL,NULL,NULL,NULL);
    if (_dxobj == NULL) {
        result.addError("can't import DX file \"%s\"", filename);
        return;
    }

    // parse out the positions array 

    // FIXME: nanowire will need a different way to parse out the positions
    //        array since it uses a productarray to store its positions.
    //        Possibly use DXGetProductArray().
    dxpos = (Array) DXGetComponentValue((Field) _dxobj, (char *)"positions");
    if (dxpos == NULL) {
        result.addError("can't get component value of \"positions\"");
        return;
    }
    DXGetArrayInfo(dxpos, &_n, &type, &category, &_rank, &_shape);

    TRACE("_n = %d\n",_n);
    if (type != TYPE_FLOAT) {
        result.addError("\"positions\" is not type float (type=%d)\n", type);
        return;
    }
    TRACE("_rank = %d\n",_rank);
    TRACE("_shape = %d\n",_shape);

    float* pos = NULL;
    pos = (float*) DXGetArrayData(dxpos);
    if (pos == NULL) {
        result.addError("DXGetArrayData failed to return positions array");
        return;
    }

    // first call to get the number of axis needed
    dxgrid = (Array) DXGetComponentValue((Field) _dxobj, (char *)"connections");
    if (DXQueryGridConnections(dxgrid, &_numAxis, NULL) == NULL) {
        // raise error, data is not a regular grid and we cannot handle it
        result.addError("DX says our grid is not regular, we cannot handle this data");
        return;
    }

    _positions = new float[_n*_numAxis];
    if (_positions == NULL) {
        // malloc failed, raise error
        result.addError("malloc of _positions array failed");
        return;
    }
    memcpy(_positions,pos,sizeof(float)*_n*_numAxis);

    _axisLen = new int[_numAxis];
    if (_axisLen == NULL) {
        // malloc failed, raise error
        result.addError("malloc of _axisLen array failed");
        return;
    }
    memset(_axisLen, 0, _numAxis);

    _delta = new float[_numAxis*_numAxis];
    if (_delta == NULL) {
        result.addError("malloc of _delta array failed");
        return;
    }
    memset(_delta, 0, _numAxis*_numAxis);

    _origin = new float[_numAxis];
    if (_origin == NULL) {
        result.addError("malloc of _origin array failed");
        return;
    }
    memset(_origin, 0, _numAxis);

    _max = new float[_numAxis];
    if (_max == NULL) {
        result.addError("malloc of _max array failed");
        return;
    }
    memset(_max, 0, _numAxis);

    __findPosMax();

    // parse out the gridconnections (length of each axis) array
    // dxgrid = (Array) DXQueryGridConnections(dxpos, &_numAxis, _axisLen);
    DXQueryGridPositions(dxpos, NULL, _axisLen, _origin, _delta);

    TRACE("_max = [%g,%g,%g]\n",_max[0],_max[1],_max[2]);
    TRACE("_delta = [%g,%g,%g]\n",_delta[0],_delta[1],_delta[2]);
    TRACE("         [%g,%g,%g]\n",_delta[3],_delta[4],_delta[5]);
    TRACE("         [%g,%g,%g]\n",_delta[6],_delta[7],_delta[8]);
    TRACE("_origin = [%g,%g,%g]\n",_origin[0],_origin[1],_origin[2]);
    TRACE("_axisLen = [%i,%i,%i]\n",_axisLen[0],_axisLen[1],_axisLen[2]);

    // grab the data array from the dx object and store it in _data
    dxdata = (Array) DXGetComponentValue((Field) _dxobj, (char *)"data");
    DXGetArrayInfo(dxdata, NULL, &type, NULL, NULL, NULL);
    _data = new float[_n];
    if (_data == NULL) {
        result.addError("malloc of _data array failed");
        return;
    }

    switch (type) {
    case TYPE_FLOAT:
        float *float_data;

        float_data = (float *)DXGetArrayData(dxdata);
        memcpy(_data, float_data, sizeof(float)*_n);
        break;

    case TYPE_DOUBLE:
        double *double_data;
        double_data = (double *)DXGetArrayData(dxdata);
        for (int i = 0; i < _n; i++) {
            _data[i] = double_data[i];
        }
	break;

    default:
        result.addError("don't know how to handle data of type %d\n", type);
        return;
    }

    // print debug info
    for (int lcv = 0, pt = 0; lcv < _n; lcv +=3, pt+=9) {
        TRACE("(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n",
            _positions[pt],_positions[pt+1],_positions[pt+2], _data[lcv],
            _positions[pt+3],_positions[pt+4],_positions[pt+5],_data[lcv+1],
            _positions[pt+6],_positions[pt+7],_positions[pt+8],_data[lcv+2]);
    }
    __collectDataStats();
}

DX::~DX()
{
    delete[] _axisLen;
    delete[] _delta;
    delete[] _origin;
    delete[] _max;
    delete[] _data;
    delete[] _positions;
}

void
DX::__findPosMax()
{
    int lcv = 0;

    // initialize the max array and delta array
    // max holds the maximum value found for each index
    for (lcv = 0; lcv < _numAxis; lcv++) {
        _max[lcv] = _positions[lcv];
    }

    for (lcv=lcv; lcv < _n*_numAxis; lcv++) {
        int maxIdx = lcv%_numAxis;
        if (_positions[lcv] > _max[maxIdx]) {
            _max[maxIdx] = _positions[lcv];
        }
    }
}

void
DX::__collectDataStats()
{
    _dataMin = FLT_MAX;
    _dataMax = -FLT_MAX;
    _nzero_min = FLT_MAX;

    // populate _dataMin, _dataMax, _nzero_min
    for (int lcv = 0; lcv < _n; lcv++) {
        if (_data[lcv] < _dataMin) {
            _dataMin = _data[lcv];
        }
        if (_data[lcv] > _dataMax) {
            _dataMax = _data[lcv];
        }
        if ((_data[lcv] > 0.0f) && (_data[lcv] < _nzero_min)) {
            _nzero_min = _data[lcv];
        }
    }
    if (_nzero_min == FLT_MAX) {
        ERROR("could not find a positive minimum value\n");
    }
}

/*
 * getInterpPos()
 *
 * creates a new grid of positions which can be used for interpolation.
 * we create the positions array using the function DXMakeGridPositionsV
 * which creates an n-dimensional grid of regularly spaced positions.
 * This function overwrites the original positions array
 */
void
DX::__getInterpPos()
{
    Array dxpos;
    float* pos = NULL;

    // gather the positions we want to interpolate over
    dxpos = DXMakeGridPositionsV(_numAxis, _axisLen, _origin, _delta);
    DXGetArrayInfo(dxpos, &_n, NULL, NULL, &_rank, &_shape);
    pos = (float*) DXGetArrayData(dxpos);
    if (pos == NULL) {
        TRACE("DXGetArrayData failed to return positions array\n");
    }

    if (_positions != NULL) {
        delete[] _positions;
    }
    _positions = new float[_n*_numAxis];
    if (_positions == NULL) {
        // malloc failed, raise error
        TRACE("malloc of _axisLen array failed");
    }
    memcpy(_positions,pos,sizeof(float)*_n*_numAxis);

    pos = NULL;
    DXDelete((object*)dxpos);
}

/*
 * getInterpData()
 *
 * this function interpolates over a positions array to produce data for each
 * point in the positions array. we use the position data stored in _positions
 * array.
 */
void
DX::__getInterpData()
{
    int pts = _n;
    int interppts = pts;
    Interpolator interpolator;

    _data = new float[_n];
    if (_data == NULL) {
        // malloc failed, raise error
        TRACE("malloc of _data array failed");
    }
    memset(_data,0,_n);

    // build the interpolator and interpolate
    TRACE("creating DXNewInterpolator...\n");
    interpolator = DXNewInterpolator(_dxobj,INTERP_INIT_IMMEDIATE,-1.0);
    TRACE("_rank = %i\n",_rank);
    TRACE("_shape = %i\n",_shape);
    TRACE("_n = %i\n",_n);
    TRACE("start interppts = %i\n",interppts);
    DXInterpolate(interpolator,&interppts,_positions,_data);
    TRACE("interppts = %i\n",interppts);

    // print debug info
    for (int lcv = 0, pt = 0; lcv < pts; lcv+=3,pt+=9) {
        TRACE("(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n(%f,%f,%f)|->% 8e\n",
            _positions[pt],_positions[pt+1],_positions[pt+2], _data[lcv],
            _positions[pt+3],_positions[pt+4],_positions[pt+5],_data[lcv+1],
            _positions[pt+6],_positions[pt+7],_positions[pt+8],_data[lcv+2]);
    }

    __collectDataStats();
}

/*
 * interpolate()
 *
 * generate a positions array with optional new axis length and
 * interpolate to get the new data values at each point in
 * the positions array. this function currently only works if you
 * do not change the axis length (i.e. newAxisLen == NULL).
 */
DX&
DX::interpolate(int* newAxisLen)
{
    TRACE("----begin interpolation----\n");
    if (newAxisLen != NULL) {
        for (int i = 0; i < _numAxis; i++) {
            _axisLen[i] = newAxisLen[i];
        }
    }
    __getInterpPos();
    __getInterpData();
    TRACE("----end interpolation----\n");
    return *this;
}

#endif /*HAVE_DX_DX_H*/
