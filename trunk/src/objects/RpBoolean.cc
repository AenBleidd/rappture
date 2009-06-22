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
    :   Object    ()
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
    :   Object    ()
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->def(val);
    this->cur(val);
}

// copy constructor
Boolean::Boolean ( const Boolean& o )
    :   Object(o)
{
    this->def(o.def());
    this->cur(o.cur());

}

// default destructor
Boolean::~Boolean ()
{
}

/**********************************************************************/
// METHOD: xml()
/// view this object's xml
/**
 * View this object as an xml element returned as text.
 */

const char *
Boolean::xml()
{
    Path p(path());
    _tmpBuf.clear();

    // FIXME: boolean should print yes/no

    _tmpBuf.appendf(
"<boolean id=\"%s\">\n\
    <about>\n\
        <label>%s</label>\n\
        <description>%s</description>\n\
    </about>\n\
    <default>%i</default>\n\
    <current>%i</current>\n\
</boolean>",
       p.id(),label(),desc(),def(),cur());

    return _tmpBuf.bytes();
}

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
Boolean::is() const
{
    // return "bool" in hex
    return 0x626F6F6C;
}

// -------------------------------------------------------------------- //
