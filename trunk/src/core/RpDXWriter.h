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
#ifndef RAPPTURE_DX_WRITER_H
#define RAPPTURE_DX_WRITER_H

#include <RpOutcome.h>
#include <RpSimpleBuffer.h>

namespace Rappture {

class DXWriter {
public:
    DXWriter();
    DXWriter(float* data, size_t nmemb, size_t rank, size_t shape);
    DXWriter(const DXWriter& rpdx);
    DXWriter& operator=(const DXWriter& rpdx);
    virtual ~DXWriter();

    virtual DXWriter& origin(float* o);
    virtual DXWriter& delta(float* d);
    virtual DXWriter& counts(size_t *p);

    virtual DXWriter& append(float* value);

    virtual DXWriter& data(float *d, size_t nmemb=1);
    virtual DXWriter& pos(float *p, size_t nmemb=1);
    virtual DXWriter& write(FILE *stream=NULL);
    virtual size_t size() const;

    virtual size_t rank(size_t rank=0);
    virtual size_t shape(size_t shape=0);
protected:

private:

    SimpleFloatBuffer _dataBuf;
    SimpleFloatBuffer _posBuf;

    size_t _rank;          // number of dimensions in each item
    size_t _shape;         // array of the extents of each dimension

    size_t* _positions;    // array holding the number of x,y,z points
    float* _delta;      // array holding deltas of the uniform mesh
    float* _origin;     // array holding coord of origin
};

} // namespace Rappture

#endif // RAPPTURE_DX_WRITER_H
