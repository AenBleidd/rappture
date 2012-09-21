/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Array1DUniform Object Source
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
#include <cmath>
#include "RpArray1DUniform.h"

using namespace Rappture;

const char Array1DUniform::type[] = "RAPPTURE_AXIS_TYPE_UNIFORM";

Array1DUniform::Array1DUniform()
    : Array1D(),
      _step(0)
{}

Array1DUniform::Array1DUniform(double min, double max, double step)
    : Array1D(),
      _step(step)
{
    this->min(min);
    this->max(max);
    __fillBuffer();
}

Array1DUniform::Array1DUniform(double min, double max, size_t nmemb)
    : Array1D(),
      _step(0)
{
    this->min(min);
    this->max(max);
    this->step(__calcStepFromNmemb(nmemb));
    __fillBuffer();
}

// copy constructor
Array1DUniform::Array1DUniform ( const Array1DUniform& o )
    : Array1D(o),
      _step(o._step)
{}

// default destructor
Array1DUniform::~Array1DUniform ()
{
    // clean up dynamic memory
}

/**********************************************************************/
// METHOD: min()
/// Set the min value and refill buffer
/**
 * Set the min value and refill buffer
 */

void
Array1DUniform::min(double min)
{
    _min = min;
    clear();
    __fillBuffer();
    return;
}

/**********************************************************************/
// METHOD: max()
/// Set the max value and refill buffer
/**
 * Set the max value and refill buffer
 */

void
Array1DUniform::max(double max)
{
    _max = max;
    clear();
    __fillBuffer();
    return;
}

/**********************************************************************/
// METHOD: step()
/// Get the step value
/**
 * Get the step value
 */

double
Array1DUniform::step(void) const
{
    return _step;
}

/**********************************************************************/
// METHOD: step()
/// Set the step value and refill buffer
/**
 * Set the step value and refill buffer
 */

void
Array1DUniform::step(double step)
{
    _step = step;
    clear();
    __fillBuffer();
    return;
}

double
Array1DUniform::__calcStepFromNmemb(size_t nmemb)
{
    double newstep = 0.0;

    if (nmemb != 0) {
        newstep = (Array1D::max()-Array1D::min()+1)/nmemb;
    }

    return newstep;
}

size_t
Array1DUniform::__calcNmembFromStep(double step)
{
    size_t nmemb = 0;

    if (step != 0) {
        nmemb = (size_t) round((Array1D::max()-Array1D::min()+1)/step);
    }

    return nmemb;
}

void
Array1DUniform::__fillBuffer()
{
    size_t bufSize = 0;

    if (_step == 0) {
        return;
    }

    bufSize = __calcNmembFromStep(_step);
    _val.set(bufSize);

    for(double i=_min; i <= _max; i=i+_step) {
        _val.append(&i,1);
    }

    return;
}

// -------------------------------------------------------------------- //

