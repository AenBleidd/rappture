/*
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <errno.h>
#include "RpVariable.h"
#include "RpChain.h"
#include "RpArray1D.h"

#ifndef RAPPTURE_CURVE_H
#define RAPPTURE_CURVE_H

namespace Rappture {

class Curve : public Variable
{
    public:

        Curve  (const char *path);

        Curve  (const char *path,
                const char *label,
                const char *desc,
                const char *group);

        Curve  (const Curve& o);

        virtual ~Curve  ();

        Curve& axis (const char *label,
                     const char *desc,
                     const char *units,
                     const char *scale,
                     double *val,
                     size_t size);

        Curve& delAxis (const char *label);

        size_t data (const char *label,
                     const double **arr) const;

        Array1D *getAxis    ( const char *label) const;
        Array1D *getNthAxis ( size_t n) const;

        /*
        Array1D *firstAxis ();
        Array1D *lastAxis  ();
        Array1D *nextAxis  (   const char *prevLabel);
        Array1D *prevAxis  (   const char *prevLabel);
        */

        Accessor <const char *> group;
        size_t dims() const;

    private:

        // hash or linked list of preset values
        Rp_Chain *_axisList;

        Rp_ChainLink *__searchAxisList(const char * label) const;
};

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif // RAPPTURE_CURVE_H
