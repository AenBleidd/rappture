/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Boolean Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpBoolean.h"
#include "RpUnits.h"

using namespace Rappture;

Boolean::Boolean (
            const char *path,
            int val
        )
    :   Variable    ()
{
    this->path(path);
    this->def(val);
    this->cur(val);
}

Boolean::Boolean (
            const char *path,
            int val,
            const char *label,
            const char *desc
        )
    :   Variable    ()
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->def(val);
    this->cur(val);
}

// copy constructor
Boolean::Boolean ( const Boolean& o )
    :   Variable(o)
{
    this->def(o.def());
    this->cur(o.cur());

}

// default destructor
Boolean::~Boolean ()
{
}

// -------------------------------------------------------------------- //
