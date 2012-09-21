/*
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <errno.h>
#include "RpCurve.h"

#ifndef RAPPTURE_SCATTER_H
#define RAPPTURE_SCATTER_H

namespace Rappture {

class Scatter : public Curve
{
    public:

        Scatter();

        Scatter(const char *path);

        Scatter(const char *path,
                const char *label,
                const char *desc,
                const char *group);

        Scatter(const Scatter& o);

        virtual ~Scatter();

        // const char *xml();
        const int is() const;

    protected:
};


} // namespace Rappture

/*--------------------------------------------------------------------------*/

#endif // RAPPTURE_SCATTER_H
