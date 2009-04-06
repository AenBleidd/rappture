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

#ifndef RAPPTURE_NUMBER_H
#define RAPPTURE_NUMBER_H

namespace Rappture {

class Number : public Variable
{
    public:

        Number  (  const char *path,
                    const char *units,
                    double val);

        Number  (  const char *path,
                    const char *units,
                    double val,
                    double min,
                    double max,
                    const char *label,
                    const char *desc);

        Number  ( const Number& o );
        virtual ~Number ();

        Accessor<double> def;
        Accessor<double> cur;
        Accessor<double> min;
        Accessor<double> max;
        Accessor<const char *> units;

        // need to add a way to tell user conversion failed
        virtual double convert (const char *to);

        Number& addPreset(  const char *label,
                            const char *desc,
                            double val,
                            const char *units   );

        Number& delPreset(const char *label);

    private:

        // flag tells if user specified min and max values
        int _minmaxSet;

        // hash or linked list of preset values
        Rp_Chain *_presets;

        struct preset{
            Accessor<const char *> label;
            Accessor<const char *> desc;
            Accessor<double> val;
            Accessor<const char *> units;
        };
};

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
