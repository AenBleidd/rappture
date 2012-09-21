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
#include "RpChainHelper.h"
#include "RpAxisMarker.h"

#ifndef RAPPTURE_HISTOGRAM_H
#define RAPPTURE_HISTOGRAM_H

namespace Rappture {

class Histogram : public Curve
{
    public:

        Histogram();

        Histogram(double *h, size_t npts);

        Histogram(double *h, size_t npts, double *bins, size_t nbins);

        Histogram(double *h, size_t npts, double min, double max,
                  size_t nbins);

        Histogram(double *h, size_t npts, double min, double max,
                  double step);

        Histogram(const Histogram& o);

        virtual ~Histogram();

        Histogram& xaxis(const char *label, const char *desc,
                         const char *units);

        Histogram& xaxis(const char *label, const char *desc,
                         const char *units, const double *bins,
                         size_t nBins);

        Histogram& xaxis(const char *label, const char *desc,
                         const char *units, double min, double max,
                         size_t nBins);

        Histogram& xaxis(const char *label, const char *desc,
                         const char *units, double min, double max,
                         double step);

        Histogram& yaxis(const char *label, const char *desc,
                         const char *units);

        Histogram& yaxis(const char *label, const char *desc,
                         const char *units, const double *vals,
                         size_t nPts);

        Histogram& binWidths(const size_t *widths, size_t nbins);

        Histogram& marker(const char *axisName, double at,
                          const char *label, const char *style);

        // should be a list of groups to add this curve to?
        Accessor <const char *> group;

        static const char x[];
        static const char y[];

        size_t nbins(void) const;

        const char *xml(size_t indent, size_t tabstop);
        const int is(void) const;

        friend int axisMarkerCpyFxn (void **to, void *from);

    protected:

        Rp_Chain *_markerList;

};

} // namespace Rappture

/*--------------------------------------------------------------------------*/

#endif // RAPPTURE_HISTOGRAM_H
