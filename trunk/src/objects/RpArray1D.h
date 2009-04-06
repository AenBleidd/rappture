/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_ARRAY1D_H
#define RAPPTURE_ARRAY1D_H

#include "RpAccessor.h"
#include "RpSimpleBuffer.h"

namespace Rappture {

class Array1D
{
public:
    Array1D (const char *path);
    Array1D (const char *path, double *val, size_t size);
    Array1D (const char *path,
             double *val,
             size_t size,
             const char *label,
             const char *desc,
             const char *units,
             const char *scale);
    Array1D (const Array1D &o);
    ~Array1D();

    Accessor<const char *> path;
    Accessor<const char *> label;
    Accessor<const char *> desc;
    Accessor<const char *> units;
    Accessor<const char *> scale;

    virtual Array1D& append(double *val, size_t nmemb);
    virtual size_t read(double *val, size_t nmemb);
    virtual size_t nmemb() const;
    virtual double min() const;
    virtual double max() const;
    virtual const double *data() const;

    static const char type[];

private:

    SimpleDoubleBuffer _val;
    double _min;
    double _max;
};

class Array1DUniform
{
public:
    Array1DUniform  (const char *path);
    Array1DUniform  (const char *path, double begin, double end, size_t step);
    Array1DUniform  (const char *path,
                     double begin,
                     double end,
                     size_t step,
                     const char *label,
                     const char *desc,
                     const char *units,
                     const char *scale);
    Array1DUniform  (const Array1DUniform &o);
    ~Array1DUniform ();

    Accessor<const char *> path;
    Accessor<const char *> label;
    Accessor<const char *> desc;
    Accessor<const char *> units;
    Accessor<const char *> scale;
    Accessor<double> begin;
    Accessor<double> end;
    Accessor<size_t> step;

    size_t read(double *val, size_t nmemb);
    const double *data() const;
    virtual size_t nmemb() const;
    static const char type[];

private:
    SimpleDoubleBuffer _val;

};
} // namespace Rappture

#endif // RAPPTURE_ARRAY1D_H
