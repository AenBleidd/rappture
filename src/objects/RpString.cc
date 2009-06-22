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
    :   Object    ()
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
    :   Object    ()
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
    :   Object(o)
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

/**********************************************************************/
// METHOD: xml()
/// view this object's xml
/**
 * View this object as an xml element returned as text.
 */

const char *
String::xml()
{
    Path p(path());
    _tmpBuf.clear();

    _tmpBuf.appendf(
"<string id='%s'>\n\
    <about>\n\
        <label>%s</label>\n\
        <description>%s</description>\n\
        <hints>%s</hints>\n\
    </about>\n\
    <size>%ix%i</size>\n\
    <default>%s</default>\n\
    <current>%s</current>\n\
</string>\n",
       p.id(),label(),desc(),hints(),width(),height(),def(),cur());

    return _tmpBuf.bytes();
}

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
String::is() const
{
    // return "stri" in hex
    return 0x73747269;
}

// -------------------------------------------------------------------- //

