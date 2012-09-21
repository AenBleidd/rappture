/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 String Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
String::xml(size_t indent, size_t tabstop)
{
    size_t l1width = indent + tabstop;
    size_t l2width = indent + (2*tabstop);
    const char *sp = "";

    Path p(path());
    _tmpBuf.clear();

    _tmpBuf.appendf(
"%12$*9$s<string id='%1$s'>\n\
%12$*10$s<about>\n\
%12$*11$s<label>%2$s</label>\n\
%12$*11$s<description>%3$s</description>\n\
%12$*11$s<hints>%4$s</hints>\n\
%12$*10$s</about>\n\
%12$*10$s<size>%5$ix%6$i</size>\n\
%12$*10$s<default>%7$s</default>\n\
%12$*10$s<current>%8$s</current>\n\
%12$*9$s</string>\n",
       p.id(),label(),desc(),hints(),width(),height(),def(),cur(),
       indent, l1width, l2width, sp);

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

