/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_ARRAY1DUNIFORM_H
#define RAPPTURE_ARRAY1DUNIFORM_H

#include "RpArray1D.h"

namespace Rappture {

class Array1DUniform : public Array1D
{
public:
    Array1DUniform();
    Array1DUniform(double min, double max, double step);
    Array1DUniform(double min, double max, size_t nmemb);
    Array1DUniform(const Array1DUniform &o);
    virtual ~Array1DUniform ();

    void min(double min);
    void max(double max);
    double step(void) const;
    void step(double step);

    static const char type[];

protected:
    double _step;

private:
    double __calcStepFromNmemb(size_t nmemb);
    size_t __calcNmembFromStep(double step);
    void __fillBuffer();

};
} // namespace Rappture

#endif // RAPPTURE_ARRAY1DUNIFORM_H
