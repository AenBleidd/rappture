/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Boolean Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
Boolean::xml(size_t indent, size_t tabstop)
{
    size_t l1width = indent + tabstop;
    size_t l2width = indent + (2*tabstop);
    const char *sp = "";

    Path p(path());
    _tmpBuf.clear();

    // FIXME: boolean should print yes/no

    _tmpBuf.appendf(
"%9$*6$s<boolean id=\"%1$s\">\n\
%9$*7$s<about>\n\
%9$*8$s<label>%2$s</label>\n\
%9$*8$s<description>%3$s</description>\n\
%9$*7$s</about>\n\
%9$*7$s<default>%4$i</default>\n\
%9$*7$s<current>%5$i</current>\n\
%9$*6$s</boolean>",
       p.id(),label(),desc(),def(),cur(),indent,l1width,l2width,sp);

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
