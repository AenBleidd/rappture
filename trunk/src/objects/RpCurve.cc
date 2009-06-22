/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Curve Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpCurve.h"

using namespace Rappture;

/*
const char Curve::format[]  = "RAPPTURE::CURVE::FORMAT";
const char Curve::id[]      = "RAPPTURE::CURVE::ID";
const char Curve::creator[] = "RAPPTURE::CURVE::CREATOR";
*/
const char Curve::x[]   = "xaxis";
const char Curve::y[]   = "yaxis";

Curve::Curve ()
    : Object (),
      _axisList (NULL)
{
    this->path("");
    this->label("");
    this->desc("");
    this->group("");
}

Curve::Curve (const char *path)
    : Object (),
      _axisList (NULL)
{
    this->path(path);
    this->label("");
    this->desc("");
    this->group("");
}

Curve::Curve (const char *path, const char *label, const char *desc,
              const char *group)
    : Object (),
      _axisList (NULL)
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->group(group);
}

// copy constructor
Curve::Curve ( const Curve& o )
    :   Object(o)
{
    this->path(o.path());
    this->label(o.label());
    this->desc(o.desc());
    this->group(o.group());

    // need to copy _axisList
}

// default destructor
Curve::~Curve ()
{
    // clean up dynamic memory
    // unallocate the _axisList?

}

/**********************************************************************/
// METHOD: axis()
/// Add an axis vector to the object
/**
 * Add an axis vector to the object.
 * Axis label must be unique.
 */

Array1D *
Curve::axis(
    const char *name,
    const char *label,
    const char *desc,
    const char *units,
    const char *scale,
    const double *val,
    size_t size)
{
    Array1D *a = NULL;

    a = new Array1D(val,size);
    if (a == NULL) {
        // raise error and exit
        return NULL;
    }
    a->name(name);
    a->label(label);
    a->desc(desc);
    a->units(units);
    a->scale(scale);

    if (_axisList == NULL) {
        _axisList = Rp_ChainCreate();
        if (_axisList == NULL) {
            // raise error and exit
            delete a;
            return NULL;
        }
    }

    Rp_ChainAppend(_axisList,a);

    return a;
}

/**********************************************************************/
// METHOD: delAxis()
/// Delete an axis from this object's list of axis
/**
 * Delete an axis from the object
 */

Curve&
Curve::delAxis(const char *name)
{
    Array1D *a = NULL;
    Rp_ChainLink *l = NULL;
    l = __searchAxisList(name);

    if (l != NULL) {
        a = (Array1D *) Rp_ChainGetValue(l);
        delete a;
        Rp_ChainDeleteLink(_axisList,l);
    }

    return *this;
}

/**********************************************************************/
// METHOD: data()
/// Return an Axis object's data based on its label
/**
 * Return an Axis object's data based on its label
 */

size_t
Curve::data(
    const char *label,
    const double **arr) const
{
    if (arr == NULL) {
        // arr should not be null
        return 0;
    }

    size_t ret = 0;
    Array1D *a = getAxis(label);
    if (a != NULL) {
        *arr = a->data();
        ret = a->nmemb();
    }
    return ret;
}

/**********************************************************************/
// METHOD: getAxis()
/// Return an Axis object based on its label
/**
 * Return an Axis object based on its label
 */

Array1D *
Curve::getAxis(const char *name) const
{
    Rp_ChainLink *l = NULL;
    l = __searchAxisList(name);

    if (l == NULL) {
        return NULL;
    }

    return (Array1D *) Rp_ChainGetValue(l);
}

Rp_ChainLink *
Curve::__searchAxisList(const char *name) const
{
    if (name == NULL) {
        return NULL;
    }

    if (_axisList == NULL) {
        return NULL;
    }

    Rp_ChainLink *l = NULL;
    Path p;

    // traverse the list looking for the match
    l = Rp_ChainFirstLink(_axisList);
    while (l != NULL) {
        Array1D *a = (Array1D *) Rp_ChainGetValue(l);
        const char *aname = a->name();
        if ((*name == *aname) && (strcmp(name,aname) == 0)) {
            // we found matching entry, return it
            break;
        }
        l = Rp_ChainNextLink(l);
    }

    return l;
}

/**********************************************************************/
// METHOD: getNthAxis()
/// Return the Nth Axis object
/**
 * Return the Nth Axis
 */

Array1D *
Curve::getNthAxis(size_t n) const
{
    Rp_ChainLink *l = NULL;
    l = Rp_ChainGetNthLink(_axisList,n);

    if (l == NULL) {
        return NULL;
    }

    return (Array1D *) Rp_ChainGetValue(l);
}

/**********************************************************************/
// METHOD: dims()
/// Return the dimensionality of the object
/**
 * Return the dimensionality of the object
 */

size_t
Curve::dims() const
{
    return (size_t) Rp_ChainGetLength(_axisList);
}

/**********************************************************************/
// METHOD: xml()
/// Return the xml of the object
/**
 * Return the xml of the object
 */

const char *
Curve::xml()
{
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

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
Curve::is() const
{
    // return "curv" in hex
    return 0x63757276;
}


// -------------------------------------------------------------------- //

