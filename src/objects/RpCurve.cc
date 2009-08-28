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
Curve::xml(size_t indent, size_t tabstop)
{
    size_t l1width = indent + tabstop;
    size_t l2width = indent + (2*tabstop);
    const char *sp = "";

    Path p(path());

    Array1D *tmpAxis = NULL;
    size_t nmemb = 0;

    const double *dataArr[dims()];
    const char *type = propstr("type");

    _tmpBuf.clear();

    _tmpBuf.appendf(
"%9$*6$s<curve id=\"%1$s\">\n\
%9$*7$s<about>\n\
%9$*8$s<group>%2$s</group>\n\
%9$*8$s<label>%3$s</label>\n\
%9$*8$s<description>%4$s</description>\n\
%9$*8$s<type>%5$s</type>\n\
%9$*7$s</about>\n",
        p.id(),group(),label(),desc(),type,indent,l1width,l2width,sp);

    for (size_t dim=0; dim < dims(); dim++) {
        tmpAxis = getNthAxis(dim);
        nmemb = tmpAxis->nmemb();
        dataArr[dim] = tmpAxis->data();
        _tmpBuf.appendf(
"%8$*6$s<%1$s>\n\
%8$*7$s<label>%2$s</label>\n\
%8$*7$s<description>%3$s</description>\n\
%8$*7$s<units>%4$s</units>\n\
%8$*7$s<scale>%5$s</scale>\n\
%8$*6$s</%1$s>\n",
        tmpAxis->name(), tmpAxis->label(), tmpAxis->desc(),
        tmpAxis->units(), tmpAxis->scale(),l1width,l2width,sp);
    }

    _tmpBuf.appendf("%3$*1$s<component>\n%3$*2$s<xy>\n",l1width,l2width,sp);
    for (size_t idx=0; idx < nmemb; idx++) {
        for(size_t dim=0; dim < dims(); dim++) {
            _tmpBuf.appendf("%10g",dataArr[dim][idx]);
        }
        _tmpBuf.append("\n",1);
    }
    _tmpBuf.appendf("%4$*3$s</xy>\n%4$*2$s</component>\n%4$*1$s</curve>",
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
Curve::is() const
{
    // return "curv" in hex
    return 0x63757276;
}


// -------------------------------------------------------------------- //

