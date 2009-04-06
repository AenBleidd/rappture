/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Array1D Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <limits>
#include "RpArray1D.h"

using namespace Rappture;

const char Array1D::type[]        = "RAPPTURE_AXIS_TYPE_IRREGULAR";
const char Array1DUniform::type[] = "RAPPTURE_AXIS_TYPE_UNIFORM";

Array1D::Array1D (const char *path)
    : _val(SimpleDoubleBuffer()),
      _min(std::numeric_limits<double>::max()),
      _max(std::numeric_limits<double>::min())
{
    this->path(path);
    this->label("");
    this->desc("");
    this->units("");
    this->scale("linear");
}

Array1D::Array1D (
            const char *path,
            double *val,
            size_t size
        )
    : _val(SimpleDoubleBuffer()),
      _min(std::numeric_limits<double>::max()),
      _max(std::numeric_limits<double>::min())
{
    this->path(path);
    this->label("");
    this->desc("");
    this->units("");
    this->scale("linear");
    append(val,size);
}

Array1D::Array1D (
            const char *path,
            double *val,
            size_t size,
            const char *label,
            const char *desc,
            const char *units,
            const char *scale
        )
    : _val(SimpleDoubleBuffer()),
      _min(std::numeric_limits<double>::max()),
      _max(std::numeric_limits<double>::min())
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->units(units);
    this->scale(scale);
    append(val,size);
}

// copy constructor
Array1D::Array1D ( const Array1D& o )
    : _val(o._val),
      _min(o._min),
      _max(o._max)
{
    this->path(o.path());
    this->label(o.label());
    this->desc(o.desc());
    this->units(o.units());
    this->scale(o.scale());
}

// default destructor
Array1D::~Array1D ()
{
    // clean up dynamic memory
}

/**********************************************************************/
// METHOD: append()
/// Append value to the axis
/**
 * Append value to the axis object.
 */

Array1D&
Array1D::append(
    double *val,
    size_t nmemb)
{
    double nmin = _min;
    double nmax = _max;

    for (size_t i = 0; i < nmemb; i++) {
        if (val[i] < nmin) {
            nmin = val[i];
        }
        if (val[i] > nmax) {
            nmax = val[i];
        }
    }

    _val.append(val,nmemb);

    _min = nmin;
    _max = nmax;

    return *this;
}

/**********************************************************************/
// METHOD: read()
/// Read values from the axis object into a memory location
/**
 * Read values from the axis object into a memory location
 */

size_t
Array1D::read(
    double *val,
    size_t nmemb)
{
    return _val.read(val,nmemb);
}

/**********************************************************************/
// METHOD: nmemb()
/// Return the number of members in the axis object
/**
 * Return the number of members in the axis object
 */

size_t
Array1D::nmemb() const
{
    return _val.nmemb();
}

/**********************************************************************/
// METHOD: min()
/// Return the min value of the object
/**
 * Return the min value of the object
 */

double
Array1D::min() const
{
    return _min;
}

/**********************************************************************/
// METHOD: max()
/// Return the max value of the object
/**
 * Return the max value of the object
 */

double
Array1D::max() const
{
    return _max;
}

/**********************************************************************/
// METHOD: data()
/// Return the actual data pointer
/**
 * Return the actual data pointer
 */

const double *
Array1D::data() const
{
    return _val.bytes();
}

// -------------------------------------------------------------------- //

