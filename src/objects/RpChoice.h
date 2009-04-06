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

#ifndef RAPPTURE_CHOICE_H
#define RAPPTURE_CHOICE_H

namespace Rappture {

class Choice : public Variable
{
    public:

        Choice  (   const char *path,
                    const char *val );

        Choice  (   const char *path,
                    const char *val,
                    const char *label,
                    const char *desc);

        Choice  (   const Choice& o );
        virtual ~Choice ();

        Accessor<const char *> def;
        Accessor<const char *> cur;

        Choice& addOption ( const char *label,
                            const char *desc,
                            const char *val );

        Choice& delOption ( const char *label);

    private:

        // hash or linked list of preset values
        Rp_Chain *_options;

        struct option{
            Accessor<const char *> label;
            Accessor<const char *> desc;
            Accessor<const char *> val;
        };
};

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
