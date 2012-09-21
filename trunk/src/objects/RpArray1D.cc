/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Array1D Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <limits>
#include "RpArray1D.h"

using namespace Rappture;

const char Array1D::type[] = "RAPPTURE_AXIS_TYPE_IRREGULAR";

Array1D::Array1D()
    : Object(),
      _min(std::numeric_limits<double>::max()),
      _max(std::numeric_limits<double>::min())
{
    name("");
    label("");
    desc("");
    units("");
    scale("linear");
}

Array1D::Array1D(const double *val, size_t size)
    : Object(),
      _min(std::numeric_limits<double>::max()),
      _max(std::numeric_limits<double>::min())
{
    name("");
    label("");
    desc("");
    units("");
    scale("linear");
    append(val,size);
}

// copy constructor
Array1D::Array1D(const Array1D& o)
    : _val(o._val),
      _min(o._min),
      _max(o._max)
{
    name(o.name());
    label(o.label());
    desc(o.desc());
    units(o.units());
    scale(o.scale());
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
Array1D::append(const double *val, size_t nmemb)
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
// METHOD: clear()
/// clear data values from the object
/**
 * Clear data values from the object
 */

Array1D&
Array1D::clear()
{
    _val.clear();
    return *this;
}

/**********************************************************************/
// METHOD: read()
/// Read values from the axis object into a memory location
/**
 * Read values from the axis object into a memory location
 */

size_t
Array1D::read(double *val, size_t nmemb)
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

/**********************************************************************/
// METHOD: xml()
/// Return the xml text representation of this object
/**
 * Return the xml text representation of this object
 */

const char *
Array1D::xml(size_t indent, size_t tabstop)
{
    return "";
}

/**********************************************************************/
// METHOD: xml()
/// Return the xml text representation of this object
/**
 * Return the xml text representation of this object
 */

const int
Array1D::is() const
{
    // return "ar1d" in hex
    return 0x61723164;
}

// -------------------------------------------------------------------- //

