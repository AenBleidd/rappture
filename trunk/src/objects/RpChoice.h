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
#include "RpChain.h"

#ifndef RAPPTURE_CHOICE_H
#define RAPPTURE_CHOICE_H

namespace Rappture {

class Choice : public Object
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

        const char *xml(size_t indent, size_t tabstop);
        const int is() const;

    private:

        // hash or linked list of preset values
        Rp_Chain *_options;

        typedef struct {
            Accessor<const char *> label;
            Accessor<const char *> desc;
            Accessor<const char *> val;
        } option;
};

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
