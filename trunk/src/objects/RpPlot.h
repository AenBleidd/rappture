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
#include "RpChainHelper.h"
#include "RpCurve.h"

#ifndef RAPPTURE_PLOT_H
#define RAPPTURE_PLOT_H

namespace Rappture {

class Plot : public Variable
{
    public:

        Plot ();

        Plot ( const Plot& o );
        virtual ~Plot ();

        Plot& add ( size_t nPts,
                    double *x,
                    double *y,
                    const char *fmt,
                    const char *name);

        // count the number of curves in the object
        size_t count() const;

        // retrieve a curve from the object
        Curve *curve (const char* name) const;

        static const char format[];
        static const char id[];
        static const char xaxis[];
        static const char yaxis[];

    private:

        // hash or linked list of preset values
        Rp_Chain *_curveList;

        Rp_ChainLink *__searchCurveList(const char *name) const;
        static int __curveCopyFxn(void **to, void *from);
};

} // namespace Rappture

#endif // RAPPTURE_PLOT_H
