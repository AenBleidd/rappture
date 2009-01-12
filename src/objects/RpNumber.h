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

#ifndef _RpVARIABLE_H
    #include "RpVariable.h"
#endif

#ifndef _RpNUMBER_H
#define _RpNUMBER_H

class RpNumber : public RpVariable
{
    public:

        RpNumber (  const char *path,
                    const char *unit,
                    double val);

        RpNumber (  const char *path,
                    const char *unit,
                    double val,
                    double min,
                    double max,
                    const char *label,
                    const char *desc);

        RpNumber ( const RpNumber& o );
        virtual ~RpNumber ();

        virtual const double defaultValue    (double val);
        virtual const double currentValue    (double val);
        virtual const double min             (double val);
        virtual const double max             (double val);
        virtual const char *units            (const char *val);
        // need to add a way to tell user conversion failed
        virtual double convert               (const char *to);

    private:

        double _default;
        double _current;
        double _min;
        double _max;

        // flag tells if user specified min and max values
        int _minmaxSet;
};

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
