/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 AxisMarker Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpAxisMarker.h"

using namespace Rappture;

AxisMarker::AxisMarker()
    : Object(),
      _axisName(NULL),
      _style(NULL),
      _at(0.0)
{}

AxisMarker::AxisMarker(const char *axisName, const char *label,
                       const char *style, double at)
    : Object(),
      _axisName(NULL),
      _style(NULL),
      _at(0.0)
{
    this->axisName(axisName);
    this->label(label);
    this->style(style);
    this->at(at);
}

AxisMarker::AxisMarker(const AxisMarker &o)
    : Object(),
      _axisName(NULL),
      _style(NULL),
      _at(0.0)
{
    this->axisName(o.axisName());
    this->label(o.label());
    this->style(o.style());
    this->at(o.at());
}

AxisMarker::~AxisMarker()
{
    if (_axisName != NULL) {
        delete[] _axisName;
    }

    if (_style != NULL) {
        delete[] _style;
    }
}


void
AxisMarker::axisName (const char *a)
{
    size_t len = 0;

    if (a == NULL) {
        return;
    }

    if (_axisName != NULL) {
        delete[] _axisName;
    }

    len = strlen(a);
    char *tmp = new char[len+1];

    strncpy(tmp,a,len+1);

    _axisName = tmp;

    return;
}

const char *
AxisMarker::axisName (void) const
{
    return _axisName;
}

void
AxisMarker::style (const char *s)
{
    size_t len = 0;

    if (s == NULL) {
        return;
    }

    if (_style != NULL) {
        delete[] _style;
    }

    len = strlen(s);
    char *tmp = new char[len+1];

    strncpy(tmp,s,len+1);

    _style = tmp;

    return;
}

const char *
AxisMarker::style (void) const
{
    return _style;
}

void
AxisMarker::at (double a)
{
    _at = a;
    return;
}

double
AxisMarker::at (void) const
{
    return _at;
}

/**********************************************************************/
// METHOD: xml()
/// Return the xml of the object
/**
 * Return the xml of the object
 */

const char *
AxisMarker::xml(size_t indent, size_t tabstop)
{
    size_t l1width = indent + tabstop;
    const char *sp = "";

    _tmpBuf.clear();

    _tmpBuf.appendf(
"%6$*4$s<marker>\n\
%6$*5$s<at>%1$g</at>\n\
%6$*5$s<label>%2$s</label>\n\
%6$*5$s<style>%3$s</style>\n\
%6$*4$s<marker>\n",
        _at,label(),_style,indent,l1width,sp);

    _tmpBuf.append("\0",1);

    return _tmpBuf.bytes();
}

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
AxisMarker::is() const
{
    // return "mark" in hex
    return 0x6D61726B;
}

// -------------------------------------------------------------------- //

