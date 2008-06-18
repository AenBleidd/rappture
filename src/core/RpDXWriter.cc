/*
 * ----------------------------------------------------------------------
 *  Rappture::DXWriter
 *
 *  Rappture DXWriter object for forming dx files nanovis can read
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2008  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cfloat>
#include <RpDXWriter.h>

using namespace Rappture;

DXWriter::DXWriter() :
    _dataBuf(SimpleFloatBuffer()),
    _posBuf(SimpleFloatBuffer()),
    _rank(3),
    _shape(0),
    _positions(NULL),
    _delta(NULL),
    _origin(NULL)
{
    _delta  = (float*) malloc(_rank*_rank*sizeof(float));
    if (_delta == NULL) {
        fprintf(stderr,
            "Error allocating %d bytes, for _delta, inside DXWriter Constructor\n",
            _rank*_rank*sizeof(float));
            return;
    }
    for (size_t j=0; j < _rank; j++) {
        for (size_t i=0; i < _rank; i++) {
            size_t idx = (_rank*j)+i;
            if (j != i) {
                _delta[idx]= 0.0f;
            }
            else {
                _delta[idx] = 1.0f;
            }
        }
    }

    _origin = (float*) malloc(_rank*sizeof(float));
    if (_origin == NULL) {
        fprintf(stderr,
            "Error allocating %d bytes, for _origin, inside DXWriter Constructor\n",
            _rank*sizeof(float));
            return;
    }
    for (size_t i=0; i < _rank; i++) {
        _origin[i] = 0.0f;
    }
}

DXWriter::DXWriter(float* data, size_t nmemb, size_t rank, size_t shape) :
    _dataBuf(SimpleFloatBuffer(data,nmemb)),
    _posBuf(SimpleFloatBuffer()),
    _rank(rank),
    _shape(shape),
    _positions(NULL),
    _delta(NULL),
    _origin(NULL)
{
    _delta  = (float*) malloc(_rank*_rank*sizeof(float));
    if (_delta == NULL) {
        fprintf(stderr,
            "Error allocating %d bytes, for _delta, inside DXWriter Constructor\n",
            _rank*_rank*sizeof(float));
            return;
    }
    for (size_t j=0; j < _rank; j++) {
        for (size_t i=0; i < _rank; i++) {
            size_t idx = (_rank*j)+i;
            if (j != i) {
                _delta[idx] = 0.0f;
            }
            else {
                _delta[idx] = 1.0f;
            }
        }
    }

    _origin = (float*) malloc(_rank*sizeof(float));
    if (_origin == NULL) {
        fprintf(stderr,
            "Error allocating %d bytes, for _origin, inside DXWriter Constructor\n",
            _rank*sizeof(float));
            return;
    }
    for (size_t i=0; i < _rank; i++) {
        _origin[i] = 0.0f;
    }
}

DXWriter::~DXWriter()
{
    if (_positions) {
        free(_positions);
    }

    if (_delta) {
        free(_delta);
    }

    if (_origin) {
        free(_origin);
    }
}

DXWriter&
DXWriter::origin(float* o)
{
    float *tmp = NULL;
    size_t nbytes = 0;

    if (o == NULL) {
        return *this;
    }

    nbytes = _rank*sizeof(float);
    tmp = (float*) malloc(nbytes);
    if (tmp == NULL) {
        fprintf(stderr,"Unable to malloc %d bytes inside DXWriter::origin\n",
            nbytes);
        return *this;
    }
    memcpy((void*)tmp,(void const*)o,nbytes);

    if (_origin) {
        free(_origin);
    }

    _origin = tmp;

    return *this;
}

DXWriter&
DXWriter::delta(float* d)
{
    float *tmp = NULL;
    size_t nbytes = 0;

    if (d == NULL) {
        return *this;
    }

    nbytes = pow(_rank,2)*sizeof(float);
    tmp = (float*) malloc(nbytes);
    if (tmp == NULL) {
        fprintf(stderr,"Unable to malloc %d bytes inside DXWriter::delta\n",
            nbytes);
        return *this;
    }
    memcpy((void*)tmp,(void const*)d,nbytes);

    if (_delta) {
        free(_delta);
    }

    _delta = tmp;

    return *this;
}

DXWriter&
DXWriter::counts(size_t *p)
{
    size_t *tmp = NULL;
    size_t nbytes = 0;

    if (p == NULL) {
        return *this;
    }

    nbytes = _rank*sizeof(size_t);
    tmp = (size_t*) malloc(nbytes);
    if (tmp == NULL) {
        fprintf(stderr,"Unable to malloc %d bytes inside DXWriter::pos\n",
            nbytes);
        return *this;
    }
    memcpy((void*)tmp,(void const*)p,nbytes);

    if (_positions) {
        free(_positions);
    }

    _positions = tmp;

    return *this;
}

DXWriter&
DXWriter::append(float* value)
{
    _dataBuf.append(value,1);
    return *this;
}

DXWriter&
DXWriter::data(float *d, size_t nmemb)
{
    _dataBuf.append(d,nmemb);
    return *this;
}

DXWriter&
DXWriter::pos(float *p, size_t nmemb)
{
    _posBuf.append(p,nmemb);
    return *this;
}

/*
 *  even though dxfile.append() is more expensive,
 *  we don't only use sprintf inside the for loops because
 *  we don't know how many loop iterations there will be
 *  and our static buffer is a constant number of chars.
 */

DXWriter&
DXWriter::write(FILE *stream)
{
    SimpleCharBuffer dxfile;
    char b[80];
    float f = 0.0;

    // expand our original buffer to because we know there are at least
    // 400 characters in even our smallest dx file.
    dxfile.set(512);

    dxfile.append("object 1 class gridpositions counts",35);
    for (size_t i=0; i < _rank; i++) {
        sprintf(b, " %10d",_positions[i]);
        dxfile.append(b,11);
    }

    dxfile.append("\norigin");
    for (size_t i=0; i < _rank; i++) {
        sprintf(b, " %10g",_origin[i]);
        dxfile.append(b,11);
    }

    for (size_t row=0; row < _rank; row++) {
        dxfile.append("\ndelta");
        for (size_t col=0; col < _rank; col++) {
            sprintf(b, " %10g",_delta[(_rank*row)+col]);
            dxfile.append(b,11);
        }
    }

    dxfile.append("\nobject 2 class gridconnections counts", 38);
    for (size_t i=0; i < _rank; i++) {
        sprintf(b, " %10d",_positions[i]);
        dxfile.append(b,11);
    }

    dxfile.append("\nattribute \"element type\" string \"cubes\"\n",41);
    dxfile.append("attribute \"ref\" string \"positions\"\n",35);

    sprintf(b,"object 3 class array type float rank 0 items %d data follows\n",
        _dataBuf.nmemb());
    dxfile.append(b);

    _dataBuf.seek(0,SEEK_SET);
    while (!_dataBuf.eof()) {
        _dataBuf.read(&f,1);
        sprintf(b,"    %10g\n",f);
        dxfile.append(b,15);
    }

    dxfile.append("attribute \"dep\" string \"positions\"\n",35);
    dxfile.append("object \"density\" class field\n",29);
    dxfile.append("component \"positions\" value 1\n",30);
    dxfile.append("component \"connections\" value 2\n",32);
    dxfile.append("component \"data\" value 3\n",25);

    fwrite((const void*)dxfile.bytes(),1,dxfile.size(),stream);
    return *this;
}

size_t
DXWriter::size() const
{
    return _dataBuf.size();
}

size_t
DXWriter::rank(size_t rank)
{
    if (rank > 0) {
        _rank = rank;
    }

    return _rank;
}

size_t
DXWriter::shape(size_t shape)
{
    if (shape > 0) {
        _shape = shape;
    }

    return _shape;
}
