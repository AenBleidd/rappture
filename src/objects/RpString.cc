/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 String Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpString.h"

using namespace Rappture;

String::String (
            const char *path,
            const char *val
        )
    :   Variable    ()
{
    this->path(path);
    this->label("");
    this->desc("");
    this->hints("");
    this->def(val);
    this->cur(val);
    this->width(0);
    this->height(0);
}

String::String (
            const char *path,
            const char *val,
            const char *label,
            const char *desc,
            const char *hints,
            size_t width,
            size_t height
        )
    :   Variable    ()
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->hints(hints);
    this->def(val);
    this->cur(val);
    this->width(width);
    this->height(height);
}

// copy constructor
String::String ( const String& o )
    :   Variable(o)
{
    this->hints(o.hints());
    this->def(o.def());
    this->cur(o.cur());
    this->width(o.width());
    this->height(o.height());
}

// default destructor
String::~String ()
{
    // clean up dynamic memory
}

// -------------------------------------------------------------------- //

