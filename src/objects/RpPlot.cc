/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Plot Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpPlot.h"

using namespace Rappture;

const char Plot::format[]  = "RAPPTURE::PLOT::FORMAT";
const char Plot::id[]      = "RAPPTURE::PLOT::ID";
const char Plot::xaxis[]   = "xaxis";
const char Plot::yaxis[]   = "yaxis";
const char Plot::creator[] = "RAPPTURE::PLOT::CREATOR";

/*
const char *Plot::creator[] = {
    "plot",
    "user"
};
*/

Plot::Plot ()
    :   Object    (),
        _curveList  (NULL)
{
    // this->path(autopath());
    this->name("");
    this->path("run");
    this->label("");
    this->desc("");
}

// copy constructor
Plot::Plot ( const Plot& o )
    :   Object(o)
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
        Curve * c = (Curve *) Rp_ChainGetValue(l);
        c->propremove(Plot::format);
        c->propremove(Plot::id);
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

    c->name(name);

    // can't use "xaxis" kinda strings here have to allocate it forreal
    c->axis(Plot::xaxis,"","","","",x,nPts);
    c->axis(Plot::yaxis,"","","","",y,nPts);
    c->propstr(Plot::format,fmt);
    c->propstr(Plot::creator,"plot");

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
// METHOD: add()
/// Add an xy curve to the object
/**
 * Add an xy curve to the object.
 * returns curve id
 */

Plot&
Plot::add(
    Curve *c,
    const char *name)
{
    if (c == NULL) {
       // raise memory error and exit
       return *this;
    }

    c->propstr(Plot::id,name);

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

/**********************************************************************/
// METHOD: getNthCurve()
/// Return the Nth Curve object
/**
 * Return the Nth Curve
 */

Curve *
Plot::getNthCurve(size_t n) const
{
    Rp_ChainLink *l = NULL;
    l = Rp_ChainGetNthLink(_curveList,n);

    if (l == NULL) {
        return NULL;
    }

    return (Curve *) Rp_ChainGetValue(l);
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
        const char *cname = c->propstr(Plot::id);
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


/**********************************************************************/
// METHOD: configureFromTree(ClientData p)
/// configure the object from a Rappture1.1 based tree
/**
 * Configure the object from a Rappture1.1 based tree
 */

// FIXME: this is an incomplete definition of how to
//        configure a plot object from a tree because there
//        currently is no xml description for plot objects
void
Plot::__configureFromTree(ClientData c)
{
    Rp_ParserXml *p = (Rp_ParserXml *)c;
    if (p == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_TreeNode node = Rp_ParserXmlElement(p,NULL);

    Rappture::Path pathObj(Rp_ParserXmlNodePath(p,node));

    path(pathObj.parent());
    name(Rp_ParserXmlNodeId(p,node));

    return;
}

/**********************************************************************/
// METHOD: dumpToTree(ClientData p)
/// dump the object to a Rappture1.1 based tree
/**
 * Dump the object to a Rappture1.1 based tree
 */

void
Plot::__dumpToTree(ClientData c)
{
    if (c == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_ParserXml *parser = (Rp_ParserXml *)c;

    Path p;

    p.parent(path());
    p.last();

    Rp_ChainLink *l = NULL;

    l = Rp_ChainFirstLink(_curveList);
    while (l != NULL) {
        Curve *c = (Curve *) Rp_ChainGetValue(l);

        //find who created the curve
        const char *ccreator = c->propstr(Plot::creator);
        if ((ccreator != NULL) &&
            (*ccreator == 'p') &&
            (strcmp(ccreator,"plot") == 0)) {
            // FIXME: check fields to see if the user specified value
            // plot defined curve, use plot's labels in curve's xml
            c->group(label());
            c->label(label());
            c->desc(desc());


            const char *xlabel = propstr("xlabel");
            const char *xdesc  = propstr("xdesc");
            const char *xunits = propstr("xunits");
            const char *xscale = propstr("xscale");
            const char *ylabel = propstr("ylabel");
            const char *ydesc  = propstr("ydesc");
            const char *yunits = propstr("yunits");
            const char *yscale = propstr("yscale");

            if (xlabel || xdesc || xunits || xscale) {
                Array1D *cxaxis = c->getAxis(Plot::xaxis);
                cxaxis->label(xlabel);
                cxaxis->desc(xdesc);
                cxaxis->units(xunits);
                cxaxis->scale(xscale);
            }

            if (ylabel || ydesc || yunits || yscale) {
                Array1D *cyaxis = c->getAxis(Plot::yaxis);
                cyaxis->label(ylabel);
                cyaxis->desc(ydesc);
                cyaxis->units(yunits);
                cyaxis->scale(yscale);
            }

        }

        c->dump(RPCONFIG_TREE,parser);
        l = Rp_ChainNextLink(l);
    }
}

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
Plot::is() const
{
    // return "plot" in hex
    return 0x706C6F74;
}

// -------------------------------------------------------------------- //
