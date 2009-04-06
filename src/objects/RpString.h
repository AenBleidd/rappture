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

#ifndef RAPPTURE_STRING_H
#define RAPPTURE_STRING_H

namespace Rappture {

class String : public Variable
{
public:

    String  ( const char *path,
              const char *val);

    String  ( const char *path,
              const char *val,
              const char *label,
              const char *desc,
              const char *hints,
              size_t width,
              size_t height);

    String  ( const String& o );
    virtual ~String ();

    Accessor<const char *> hints;
    Accessor<const char *> def;
    Accessor<const char *> cur;
    Accessor<size_t> width;
    Accessor<size_t> height;

private:

};

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif // RAPPTURE_STRING_H
