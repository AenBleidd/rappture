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
#include "RpObject.h"
#include "RpChain.h"
#include "RpArray1D.h"

#ifndef RAPPTURE_CURVE_H
#define RAPPTURE_CURVE_H

namespace Rappture {

class Curve : public Object
{
    public:

        Curve();

        Curve(const char *path);

        Curve(const char *path, const char *label, const char *desc,
              const char *group);

        Curve(const Curve& o);

        virtual ~Curve();

        Array1D *axis(const char *name, const char *label, const char *desc,
                      const char *units, const char *scale, const double *val,
                      size_t size);

        Curve& delAxis(const char *name);

        size_t data(const char *label, const double **arr) const;

        Array1D *getAxis(const char *name) const;
        Array1D *getNthAxis(size_t n) const;

        // should be a list of groups to add this curve to?
        Accessor <const char *> group;
        size_t dims() const;

        const char *xml(size_t indent, size_t tabstop);
        const int is() const;

        static const char x[];
        static const char y[];

    private:

        // hash or linked list of axis
        Rp_Chain *_axisList;

        Rp_ChainLink *__searchAxisList(const char * name) const;
};


} // namespace Rappture

/*--------------------------------------------------------------------------*/

#endif // RAPPTURE_CURVE_H
