/*
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpObject.h"

#ifndef RAPPTURE_BOOLEAN_H
#define RAPPTURE_BOOLEAN_H

namespace Rappture {

class Boolean : public Object
{
    public:

        Boolean  (  const char *path,
                    int val);

        Boolean  (  const char *path,
                    int val,
                    const char *label,
                    const char *desc);

        Boolean  ( const Boolean& o );
        virtual ~Boolean ();

        Accessor<int> def;
        Accessor<int> cur;

        const char *xml(size_t indent, size_t tabstop);
        const int is() const;
};

} // namespace Rappture

/*--------------------------------------------------------------------------*/

#endif
