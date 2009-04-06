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

Curve::Curve (const char *path)
    :   Variable    (),
        _axisList    (NULL)
{
    this->path(path);
    this->label("");
    this->desc("");
    this->group("");
}

Curve::Curve (
            const char *path,
            const char *label,
            const char *desc,
            const char *group
        )
    :   Variable    (),
        _axisList    (NULL)
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->group(group);
}

// copy constructor
Curve::Curve ( const Curve& o )
    :   Variable(o)
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

Curve&
Curve::axis(
    const char *label,
    const char *desc,
    const char *units,
    const char *scale,
    double *val,
    size_t size)
{
    Array1D *a = NULL;
    SimpleCharBuffer apath(path());

    apath.append(".");
    apath.append(label);

    a = new Array1D(apath.bytes(),val,size,label,desc,units,scale);
    if (!a) {
        // raise error and exit
    }

    if (_axisList == NULL) {
        _axisList = Rp_ChainCreate();
        if (_axisList == NULL) {
            // raise error and exit
        }
    }

    Rp_ChainAppend(_axisList,a);

    return *this;
}

/*
Curve&
Curve::addAxis(
    const char *path)
{
    return *this;
}
*/

/**********************************************************************/
// METHOD: delAxis()
/// Delete an axis from this object's list of axis
/**
 * Delete an axis from the object
 */

Curve&
Curve::delAxis(const char *label)
{
    Array1D *a = NULL;
    Rp_ChainLink *l = NULL;
    l = __searchAxisList(label);

    if (l == NULL) {
        return *this;
    }

    a = (Array1D *) Rp_ChainGetValue(l);
    delete a;
    Rp_ChainDeleteLink(_axisList,l);

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
    Rappture::Array1D *a = getAxis(label);
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
Curve::getAxis(const char *label) const
{
    Rp_ChainLink *l = NULL;
    l = __searchAxisList(label);

    if (l == NULL) {
        return NULL;
    }

    return (Array1D *) Rp_ChainGetValue(l);
}

Rp_ChainLink *
Curve::__searchAxisList(const char *label) const
{
    if (label == NULL) {
        return NULL;
    }

    if (_axisList == NULL) {
        return NULL;
    }

    Rp_ChainLink *l = NULL;
    Rp_ChainLink *retval = NULL;

    // traverse the list looking for the match
    l = Rp_ChainFirstLink(_axisList);
    while (l != NULL) {
        Array1D *a = (Array1D *) Rp_ChainGetValue(l);
        const char *alabel = a->label();
        if ((*label == *alabel) && (strcmp(alabel,label) == 0)) {
            // we found matching entry, return it
            retval = l;
            break;
        }
        l = Rp_ChainNextLink(l);
    }

    return retval;
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

// -------------------------------------------------------------------- //

