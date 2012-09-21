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

#ifndef RAPPTURE_STRING_H
#define RAPPTURE_STRING_H

namespace Rappture {

class String : public Object
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

    Accessor<const char *> def;
    Accessor<const char *> cur;
    Accessor<size_t> width;
    Accessor<size_t> height;

    const char *xml(size_t indent, size_t tabstop);
    const int is() const;

private:

};

} // namespace Rappture

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif // RAPPTURE_STRING_H
