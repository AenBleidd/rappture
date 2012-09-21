/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Scatter Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpScatter.h"

using namespace Rappture;

Scatter::Scatter ()
    : Curve()
{
    this->path("");
    this->label("");
    this->desc("");
    this->group("");
    propstr("type","scatter");
}

Scatter::Scatter (const char *path)
    : Curve()
{
    this->path(path);
    this->label("");
    this->desc("");
    this->group("");
    propstr("type","scatter");
}

Scatter::Scatter (const char *path, const char *label, const char *desc,
              const char *group)
    : Curve()
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->group(group);
    propstr("type","scatter");
}

// copy constructor
Scatter::Scatter ( const Scatter& o )
    :   Curve(o)
{
    this->path(o.path());
    this->label(o.label());
    this->desc(o.desc());
    this->group(o.group());

    // need to copy _axisList
}

// default destructor
Scatter::~Scatter ()
{
    // clean up dynamic memory
    // unallocate the _axisList?

}

/**********************************************************************/
// METHOD: xml()
/// Return the xml of the object
/**
 * Return the xml of the object
 */

/*
const char *
Scatter::xml(size_t indent, size_t tabstop)
{
// FIXME: the xml function should just read an array
// of path/value or path/function pairs, get the value by executing
// the function if needed, and place the data in an xml tree.
    Path p(path());

    Array1D *tmpAxis = NULL;
    size_t nmemb = 0;

    const double *dataArr[dims()];

    _tmpBuf.clear();

    _tmpBuf.appendf(
"<curve id=\"%s\">\n\
    <about>\n\
        <group>%s</group>\n\
        <label>%s</label>\n\
        <description>%s</description>\n\
        <type>scatter</type>\n\
    </about>\n", p.id(),group(),label(),desc());

    for (size_t dim=0; dim < dims(); dim++) {
        tmpAxis = getNthAxis(dim);
        nmemb = tmpAxis->nmemb();
        dataArr[dim] = tmpAxis->data();
        _tmpBuf.appendf(
"    <%s>\n\
        <label>%s</label>\n\
        <description>%s</description>\n\
        <units>%s</units>\n\
        <scale>%s</scale>\n\
    </%s>\n",
        tmpAxis->name(), tmpAxis->label(), tmpAxis->desc(),
        tmpAxis->units(), tmpAxis->scale(), tmpAxis->name());
    }

    _tmpBuf.append("    <component>\n        <xy>\n");
    for (size_t idx=0; idx < nmemb; idx++) {
        for(size_t dim=0; dim < dims(); dim++) {
            _tmpBuf.appendf("%10g",dataArr[dim][idx]);
        }
        _tmpBuf.append("\n",1);
    }
    _tmpBuf.append("        </xy>\n    </component>\n</curve>");
    _tmpBuf.append("\0",1);

    return _tmpBuf.bytes();
}
*/

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
Scatter::is() const
{
    // return "scat" in hex
    return 0x73636174;
}


// -------------------------------------------------------------------- //

