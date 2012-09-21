/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_ARRAY1D_H
#define RAPPTURE_ARRAY1D_H

#include "RpAccessor.h"
#include "RpSimpleBuffer.h"
#include "RpObject.h"

namespace Rappture {

class Array1D : public Object
{
public:

    Array1D();
    Array1D (const double *val, size_t size);
    Array1D (const Array1D &o);
    virtual ~Array1D();

    Accessor<const char *> name;
    Accessor<const char *> units;
    Accessor<const char *> scale;

    virtual Array1D& append(const double *val, size_t nmemb);
    virtual Array1D& clear();
    virtual size_t read(double *val, size_t nmemb);
    virtual size_t nmemb() const;
    virtual double min() const;
    virtual double max() const;
    virtual const double *data() const;

    static const char type[];

    const char *xml(size_t indent, size_t tabstop);
    const int is(void) const;

protected:

    SimpleDoubleBuffer _val;
    double _min;
    double _max;
};

} // namespace Rappture

#endif // RAPPTURE_ARRAY1D_H
