/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Plot Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpPlot.h"

using namespace Rappture;

const char Plot::format[] = "RAPPTURE::PLOT::FORMAT";
const char Plot::id[]     = "RAPPTURE::PLOT::ID";
const char Plot::xaxis[]  = "RAPPTURE::PLOT::XAXIS";
const char Plot::yaxis[]  = "RAPPTURE::PLOT::YAXIS";

Plot::Plot ()
    :   Variable    (),
        _curveList  (NULL)
{
    // this->path(autopath());
    this->path("");
    this->label("");
    this->desc("");
}

// copy constructor
Plot::Plot ( const Plot& o )
    :   Variable(o)
{
    _curveList = Rp_ChainCreate();
    Rp_ChainCopy(this->_curveList,o._curveList,&__curveCopyFxn);
    // need to copy _curveList
}

// default destructor
Plot::~Plot ()
{
    // remove the properties plot put in each curve
    // unallocate _curveList

    Rp_ChainLink *l = Rp_ChainFirstLink(_curveList);
    while (l != NULL) {
        char *str = NULL;
        Curve * c = (Curve *) Rp_ChainGetValue(l);

        str = (char *) c->propremove(Plot::format);
        delete str;

        str = (char *) c->propremove(Plot::id);
        delete str;

        delete c;
        c = NULL;
        l = Rp_ChainNextLink(l);
    }

    Rp_ChainDestroy(_curveList);
}

/**********************************************************************/
// METHOD: add()
/// Add an xy curve to the object
/**
 * Add an xy curve to the object.
 * returns curve id
 */

Plot&
Plot::add(
    size_t nPts,
    double *x,
    double *y,
    const char *fmt,
    const char *name)
{
    // Curve *c = Curve(autopath(),"","",__group());
    Curve *c = new Curve("","","","");

    if (c == NULL) {
       // raise memory error and exit
       return *this;
    }

    // can't use "xaxis" kinda strings here have to allocate it forreal
    c->axis(xaxis,"xdesc","xunits","xcale",x,nPts);
    c->axis(yaxis,"ydesc","yunits","ycale",y,nPts);
    c->propstr(format,fmt);
    c->propstr(id,name);

    if (_curveList == NULL) {
        _curveList = Rp_ChainCreate();
        if (_curveList == NULL) {
            // raise error and exit
        }
    }

    Rp_ChainAppend(_curveList,c);

    return *this;
}


/**********************************************************************/
// METHOD: count()
/// return number of curves in this object
/**
 * count of the number of curves contained in this object
 * returns curve count
 */

size_t
Plot::count() const
{
    // return (size_t) Rp_ChainGetLength(_curveList);
    return Rp_ChainGetLength(_curveList);
}

/**********************************************************************/
// METHOD: curve()
/// Retrieve an xy curve from the object
/**
 * Get a named xy curve.
 * returns pointer to Rappture::Curve
 */

Curve *
Plot::curve(
    const char *name) const
{
    Curve *c = NULL;
    Rp_ChainLink *l = NULL;

    l = __searchCurveList(name);
    if (l != NULL) {
        c = (Curve *) Rp_ChainGetValue(l);
    }
    return c;
}

Rp_ChainLink *
Plot::__searchCurveList(const char *name) const
{
    if (name == NULL) {
        return NULL;
    }

    if (_curveList == NULL) {
        return NULL;
    }

    Rp_ChainLink *l = NULL;
    Rp_ChainLink *retval = NULL;

    // traverse the list looking for the match
    l = Rp_ChainFirstLink(_curveList);
    while (l != NULL) {
        Curve *c = (Curve *) Rp_ChainGetValue(l);
        const char *cname = (const char *) c->property(id,NULL);
        if (cname != NULL) {
            if((*cname == *name) && (strcmp(cname,name) == 0)) {
                // we found matching entry, return it
                retval = l;
                break;
            }
        }
        l = Rp_ChainNextLink(l);
    }

    return retval;
}

int
Plot::__curveCopyFxn(void **to, void *from)
{
    if (from == NULL) {
        // return error, improper function call
        return -1;
    }

    Curve *c = new Curve(*((Curve *)from));
    *to = (void *) c;

    return 0;
}

// -------------------------------------------------------------------- //
