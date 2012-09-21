/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Curve Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
    this->name("");
    this->path("run");
    this->label("");
    this->desc("");
    this->group("");
}

Curve::Curve (const char *name)
    : Object (),
      _axisList (NULL)
{
    this->name(name);
    this->path("run");
    this->label("");
    this->desc("");
    this->group("");
}

Curve::Curve (const char *name, const char *label, const char *desc,
              const char *group)
    : Object (),
      _axisList (NULL)
{
    this->name(name);
    this->path("run");
    this->label(label);
    this->desc(desc);
    this->group(group);
}

// copy constructor
Curve::Curve ( const Curve& o )
    :   Object(o)
{
    this->name(o.name());
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
// METHOD: configure(Rp_ParserXml *p)
/// construct a number object from the provided tree
/**
 * construct a number object from the provided tree
 */

void
Curve::configure(size_t as, void *p)
{
    if (as == RPCONFIG_XML) {
        __configureFromXml((const char *)p);
    } else if (as == RPCONFIG_TREE) {
        __configureFromTree((Rp_ParserXml *)p);
    }
}

/**********************************************************************/
// METHOD: configureFromXml(const char *xmltext)
/// configure the object based on Rappture1.1 xmltext
/**
 * Configure the object based on the provided xml
 */

void
Curve::__configureFromXml(const char *xmltext)
{
    if (xmltext == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_ParserXml *p = Rp_ParserXmlCreate();
    Rp_ParserXmlParse(p, xmltext);
    configure(RPCONFIG_TREE, p);

    return;
}


// This is really, __configureFromTree_1.0
// because it only allows for an xaxis and yaxis
// and component.xy formated data
void
Curve::__configureFromTree(Rp_ParserXml *p)
{
    if (p == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_TreeNode node = Rp_ParserXmlElement(p,NULL);

    Rappture::Path pathObj(Rp_ParserXmlNodePath(p,node));

    path(pathObj.parent());
    name(Rp_ParserXmlNodeId(p,node));

    pathObj.clear();
    pathObj.add("about");
    pathObj.add("label");
    label(Rp_ParserXmlGet(p,pathObj.path()));

    pathObj.del();
    pathObj.add("description");
    desc(Rp_ParserXmlGet(p,pathObj.path()));


    Array1D *xaxis = axis(Curve::x,"","","","",NULL,0);
    pathObj.del();
    pathObj.del();
    pathObj.add(Curve::x);
    pathObj.add("label");
    xaxis->label(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.del();
    pathObj.add("description");
    xaxis->desc(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.del();
    pathObj.add("units");
    xaxis->units(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.del();
    pathObj.add("scale");
    xaxis->scale(Rp_ParserXmlGet(p,pathObj.path()));

    Array1D *yaxis = axis(Curve::y,"","","","",NULL,0);
    pathObj.del();
    pathObj.del();
    pathObj.add(Curve::y);
    pathObj.add("label");
    yaxis->label(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.del();
    pathObj.add("description");
    yaxis->desc(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.del();
    pathObj.add("units");
    yaxis->units(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.del();
    pathObj.add("scale");
    yaxis->scale(Rp_ParserXmlGet(p,pathObj.path()));

    pathObj.del();
    pathObj.del();
    pathObj.add("component");
    pathObj.add("xy");
    const char *values = Rp_ParserXmlGet(p,pathObj.path());

    double x,y;
    int n;
    while (sscanf(values,"%lf%lf%n",&x,&y,&n) == 2) {
        xaxis->append(&x,1);
        yaxis->append(&y,1);
        values += n;
    }

    return;
}

/**********************************************************************/
// METHOD: dump(size_t as, void *p)
/// construct a number object from the provided tree
/**
 * construct a number object from the provided tree
 */

void
Curve::dump(size_t as, ClientData p)
{
    if (as == RPCONFIG_XML) {
        __dumpToXml(p);
    } else if (as == RPCONFIG_TREE) {
        __dumpToTree(p);
    }
}

/**********************************************************************/
// METHOD: dumpToXml(ClientData p)
/// configure the object based on Rappture1.1 xmltext
/**
 * Configure the object based on the provided xml
 */

void
Curve::__dumpToXml(ClientData c)
{
    if (c == NULL) {
        // FIXME: setup error
        return;
    }

    ClientDataXml *d = (ClientDataXml *)c;
    Rp_ParserXml *parser = Rp_ParserXmlCreate();
    __dumpToTree(parser);
    const char *xmltext = Rp_ParserXmlXml(parser);
    _tmpBuf.appendf("%s",xmltext);
    Rp_ParserXmlDestroy(&parser);
    d->retStr = _tmpBuf.bytes();
}

/**********************************************************************/
// METHOD: dumpToTree(ClientData p)
/// dump the object to a Rappture1.1 based tree
/**
 * Dump the object to a Rappture1.1 based tree
 */

void
Curve::__dumpToTree(ClientData c)
{
    if (c == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_ParserXml *parser = (Rp_ParserXml *)c;

    Path p;

    p.parent(path());
    p.last();

    p.add("curve");
    p.id(name());

    p.add("about");

    p.add("group");
    Rp_ParserXmlPutF(parser,p.path(),"%s",group());

    p.del();
    p.add("label");
    Rp_ParserXmlPutF(parser,p.path(),"%s",label());

    p.del();
    p.add("description");
    Rp_ParserXmlPutF(parser,p.path(),"%s",desc());

    p.del();
    p.add("type");
    Rp_ParserXmlPutF(parser,p.path(),"%s",propstr("type"));

    p.del();
    p.del();

    const double *dataArr[dims()];
    size_t nmemb = 0;
    for (size_t dim = 0; dim < dims(); dim++) {
        Array1D *a = getNthAxis(dim);
        nmemb = a->nmemb();
        dataArr[dim] = a->data();
        p.add(a->name());

        p.add("label");
        Rp_ParserXmlPutF(parser,p.path(),"%s",a->label());
        p.del();

        p.add("description");
        Rp_ParserXmlPutF(parser,p.path(),"%s",a->desc());
        p.del();

        p.add("units");
        Rp_ParserXmlPutF(parser,p.path(),"%s",a->units());
        p.del();

        p.add("scale");
        Rp_ParserXmlPutF(parser,p.path(),"%s",a->scale());
        p.del();

        p.del();
    }

    SimpleCharBuffer tmpBuf;
    for (size_t idx=0; idx < nmemb; idx++) {
        for(size_t dim=0; dim < dims(); dim++) {
            tmpBuf.appendf("%10g",dataArr[dim][idx]);
        }
        tmpBuf.appendf("\n");
    }
    p.add("component");
    p.add("xy");
    Rp_ParserXmlPutF(parser,p.path(),"%s",tmpBuf.bytes());

    return;
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

