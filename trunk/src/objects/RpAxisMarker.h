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

#include "RpObject.h"

#ifndef RAPPTURE_AXISMARKER_H
#define RAPPTURE_AXISMARKER_H

namespace Rappture {

class AxisMarker : public Object
{
    public:

        AxisMarker();

        AxisMarker(const char *axisName, const char *label,
                   const char *style, double at);

        AxisMarker(const AxisMarker& o);

        virtual ~AxisMarker();

        void axisName (const char *a);
        const char *axisName (void) const;

        void style (const char *s);
        const char *style (void) const;

        void at (double a);
        double at (void) const;

        const char *xml(size_t indent, size_t tabstop);
        const int is(void) const;

    protected:

        const char *_axisName;
        const char *_style;
        double _at;
};

} // namespace Rappture

/*--------------------------------------------------------------------------*/

#endif // RAPPTURE_AXISMARKER_H
