/*
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_VARIABLE_H
#define RAPPTURE_VARIABLE_H

#include "RpInt.h"
#include "RpHash.h"
#include "RpAccessor.h"
#include "RpBuffer.h"

namespace Rappture {

class Variable
{
    public:
        Variable ();
        Variable (  const char *path,
                    const char *label,
                    const char *desc,
                    const char *hints,
                    const char *color   );
        Variable (const Variable& o);
        virtual ~Variable();

        Accessor<const char *> path;
        Accessor<const char *> label;
        Accessor<const char *> desc;
        Accessor<const char *> hints;
        Accessor<const char *> color;
        Accessor<Buffer> icon;

        // these functions are not completely improper use proof
        // user is responsible for calling propremove() on any item
        // they put into the hash table.
        const void *property (const char *key, const void *val);
        const char *propstr (const char *key, const char *val);
        void *propremove (const char *key);

    private:

        /// hash table holding other object properties
        Rp_HashTable *_h;

        /// initialize the object, set the data members to 0 or null
        void __init();

        /// close out the object, freeing its memory
        void __clear();
};

} // RAPPTURE_VARIABLE_H

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
