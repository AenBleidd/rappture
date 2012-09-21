/*
 * ----------------------------------------------------------------------
 *  Rappture::DXWriter
 *
 *  Rappture DXWriter object for forming dx files nanovis can read
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
#include <assert.h>
using namespace Rappture;

DXWriter::DXWriter() :
    _dataBuf(SimpleDoubleBuffer()),
    _posBuf(SimpleDoubleBuffer()),
    _rank(3),
    _shape(0),
    _positions(NULL),
    _delta(NULL),
    _origin(NULL)
{
    _delta  = (double*) malloc(_rank*_rank*sizeof(double));
    if (_delta == NULL) {
        fprintf(stderr,
            "Error allocating %lu bytes, for _delta, inside DXWriter Constructor\n",
            (unsigned long)(_rank*_rank*sizeof(double)));
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

    _origin = (double*) malloc(_rank*sizeof(double));
    if (_origin == NULL) {
        fprintf(stderr,
            "Error allocating %lu bytes, for _origin, inside DXWriter Constructor\n",
            (unsigned long)(_rank*sizeof(double)));
            return;
    }
    for (size_t i=0; i < _rank; i++) {
        _origin[i] = 0.0f;
    }
}

DXWriter::DXWriter(double* data, size_t nmemb, size_t rank, size_t shape) :
    _dataBuf(SimpleDoubleBuffer(data,nmemb)),
    _posBuf(SimpleDoubleBuffer()),
    _rank(rank),
    _shape(shape),
    _positions(NULL),
    _delta(NULL),
    _origin(NULL)
{
    _delta  = (double*) malloc(_rank*_rank*sizeof(double));
    if (_delta == NULL) {
        fprintf(stderr,
            "Error allocating %lu bytes, for _delta, inside DXWriter Constructor\n",
            (unsigned long)(_rank*_rank*sizeof(double)));
            return;
    }
    for (size_t j=0; j < _rank; j++) {
        for (size_t i=0; i < _rank; i++) {
            size_t idx = (_rank*j)+i;
            if (j != i) {
                _delta[idx] = 0.0f;
            } else {
                _delta[idx] = 1.0f;
            }
        }
    }

    _origin = (double*) malloc(_rank*sizeof(double));
    if (_origin == NULL) {
        fprintf(stderr,
            "Error allocating %lu bytes, for _origin, inside DXWriter Constructor\n",
            (unsigned long)(_rank*sizeof(double)));
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
DXWriter::origin(double* o)
{
    if (o == NULL) {
        return *this;
    }

    size_t nbytes = _rank * sizeof(double);
    double *tmp = (double*) malloc(nbytes);
    if (tmp == NULL) {
        fprintf(stderr,"Unable to malloc %lu bytes inside DXWriter::origin\n",
            (unsigned long)nbytes);
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
DXWriter::delta(double* d)
{
    if (d == NULL) {
        return *this;
    }

    size_t nbytes = _rank * _rank * sizeof(double);
    double *tmp = (double*)malloc(nbytes);
    if (tmp == NULL) {
        fprintf(stderr,"Unable to malloc %lu bytes inside DXWriter::delta\n",
            (unsigned long)nbytes);
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
    if (p == NULL) {
        return *this;
    }
    size_t nbytes = _rank * sizeof(size_t);
    size_t *tmp = (size_t*) malloc(nbytes);
    if (tmp == NULL) {
        fprintf(stderr,"Unable to malloc %lu bytes inside DXWriter::pos\n",
            (unsigned long)nbytes);
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
DXWriter::append(double* value)
{
    _dataBuf.append(value,1);
    return *this;
}

DXWriter&
DXWriter::data(double *d, size_t nmemb)
{
    _dataBuf.append(d,nmemb);
    return *this;
}

DXWriter&
DXWriter::pos(double *p, size_t nmemb)
{
    _posBuf.append(p,nmemb);
    return *this;
}

DXWriter&
DXWriter::write(FILE *f)
{
    if (f == NULL) {
        fprintf(stderr,"FILE is NULL, cannot write to NULL file inside DXWriter::write\n");
        return *this;
    }
    SimpleCharBuffer dxfile;
    _writeDxToBuffer(dxfile);
    ssize_t nWritten;
    nWritten = fwrite(dxfile.bytes(), 1, dxfile.size(), f);
    assert(nWritten == (ssize_t)dxfile.size());
    return *this;
}

DXWriter&
DXWriter::write(const char* fname)
{
    FILE *fp = NULL;

    if (fname == NULL) {
        fprintf(stderr,"filename is NULL, cannot write to NULL file inside DXWriter::write\n");
        return *this;
    }

    fp = fopen(fname,"wb");
    write(fp);
    fclose(fp);

    return *this;
}

DXWriter&
DXWriter::write(char *str)
{
    SimpleCharBuffer dxfile;
    int sz;

    _writeDxToBuffer(dxfile);
    sz = dxfile.size();
    str = new char[sz+1];
    memcpy(str, dxfile.bytes(), sz);
    str[sz] = '\0';
    return *this;
}

/*
 *  even though dxfile.append() is more expensive,
 *  we don't only use sprintf inside the for loops because
 *  we don't know how many loop iterations there will be
 *  and our static buffer is a constant number of chars.
 */

DXWriter&
DXWriter::_writeDxToBuffer(SimpleCharBuffer &dxfile)
{
    char b[80];
    double f = 0.0;

    // expand our original buffer to 512 characters
    // because we know there are at least
    // 400 characters in even our smallest dx file.
    dxfile.set(512);

    dxfile.append("<ODX>object 1 class gridpositions counts",40);
    for (size_t i=0; i < _rank; i++) {
        sprintf(b, " %10lu", (unsigned long)_positions[i]);
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
        sprintf(b, " %10lu",(unsigned long)_positions[i]);
        dxfile.append(b,11);
    }

    dxfile.append("\nattribute \"element type\" string \"cubes\"\n",41);
    dxfile.append("attribute \"ref\" string \"positions\"\n",35);

    sprintf(b,"object 3 class array type float rank 0 items %lu data follows\n",
        (unsigned long)_dataBuf.nmemb());
    dxfile.append(b);

    _dataBuf.seek(0,SEEK_SET);
    while (!_dataBuf.eof()) {
        _dataBuf.read(&f,1);
        // nanovis and many other progs fail when you send it inf data
        if (!std::isfinite(f)) {
            f = 0.0;
        }
        sprintf(b,"    %10g\n",f);
        // we do not know the length of the string
        // only that it has 10 sigdigs, so we cannot tell
        // append the size of the data
        // dxfile.append(b,15);
        dxfile.append(b);
    }

    dxfile.append("attribute \"dep\" string \"positions\"\n",35);
    dxfile.append("object \"density\" class field\n",29);
    dxfile.append("component \"positions\" value 1\n",30);
    dxfile.append("component \"connections\" value 2\n",32);
    dxfile.append("component \"data\" value 3\n",25);

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
