/*
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpVariable.h"

#ifndef RAPPTURE_BOOLEAN_H
#define RAPPTURE_BOOLEAN_H

namespace Rappture {

class Boolean : public Variable
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

};

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif
