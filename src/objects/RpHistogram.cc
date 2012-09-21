/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Histogram Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpHistogram.h"
#include "RpArray1DUniform.h"

using namespace Rappture;

const char Histogram::x[] = "xaxis";
const char Histogram::y[] = "yaxis";

Histogram::Histogram()
    : Curve(),
      _markerList(NULL)
{}

Histogram::Histogram(double *h, size_t npts)
    : Curve(),
      _markerList(NULL)
{
    Array1D *y = axis(Histogram::y,"","","","",h,npts);
    Array1DUniform bins(y->min(),y->max(),y->nmemb());
    axis(Histogram::x,"","","","",bins.data(),bins.nmemb());
}

Histogram::Histogram(double *h, size_t npts, double *bins, size_t nbins)
    : Curve(),
      _markerList(NULL)
{
    axis(Histogram::x,"","","","",bins,nbins);
    axis(Histogram::y,"","","","",h,npts);
}

Histogram::Histogram(double *h, size_t npts, double min, double max,
                     size_t nbins)
    : Curve(),
      _markerList(NULL)
{
    Array1DUniform bins(min,max,nbins);
    axis(Histogram::x,"","","","",bins.data(),bins.nmemb());
    axis(Histogram::y,"","","","",h,npts);
}

Histogram::Histogram(double *h, size_t npts, double min, double max,
                     double step)
    : Curve(),
      _markerList(NULL)
{
    Array1DUniform bins(min,max,step);
    axis(Histogram::x,"","","","",bins.data(),bins.nmemb());
    axis(Histogram::y,"","","","",h,npts);
}

// copy constructor
Histogram::Histogram(const Histogram& o)
    : Curve(o),
      _markerList(NULL)
{
    group(o.group());

    if (o._markerList != NULL) {
        Rp_ChainLink *l = Rp_ChainFirstLink(o._markerList);
        while (l != NULL) {
            AxisMarker *a = (AxisMarker *) Rp_ChainGetValue(l);
            AxisMarker *newAM = new AxisMarker(*a);
            Rp_ChainAppend(_markerList,(void *)newAM);
            l = Rp_ChainNextLink(l);
        }
    }

}

// default destructor
Histogram::~Histogram ()
{
    // clean up dynamic memory
    // unallocate the _axisList?

}

/**********************************************************************/
// METHOD: xaxis()
/// Define the xaxis properties
/**
 * Define the xaxis properties.
 */

Histogram&
Histogram::xaxis(const char *label, const char *desc, const char *units)
{
    Array1D *a = getAxis(Histogram::x);

    if (a == NULL) {
        // couldn't find xaxis array, create one
        a = axis(Histogram::x,label,desc,units,"linear",NULL,0);
    } else {
        a->label(label);
        a->desc(desc);
        a->units(units);
    }

    return *this;
}

/**********************************************************************/
// METHOD: xaxis()
/// Define the xaxis properties
/**
 * Define the xaxis properties.
 */

Histogram&
Histogram::xaxis(const char *label, const char *desc,
                 const char *units, const double *bins, size_t nBins)
{
    Array1D *a = getAxis(Histogram::x);

    if (a == NULL) {
        // couldn't find xaxis array, create one
        a = axis(Histogram::x,label,desc,units,"linear",bins,nBins);
    } else {
        a->label(label);
        a->desc(desc);
        a->units(units);
        a->clear();
        a->append(bins,nBins);
    }

    return *this;
}

/**********************************************************************/
// METHOD: xaxis()
/// Define the xaxis properties
/**
 * Define the xaxis properties.
 */

Histogram&
Histogram::xaxis(const char *label, const char *desc,
                 const char *units, double min, double max,
                 size_t nBins)
{
    Array1D *a = getAxis(Histogram::x);
    Array1DUniform tmp(min,max,nBins);

    if (a == NULL) {
        // couldn't find xaxis array, create one
        a = axis(Histogram::x,label,desc,units,"linear",
                 tmp.data(),tmp.nmemb());
    } else {
        a->label(label);
        a->desc(desc);
        a->units(units);
        a->clear();
        a->append(tmp.data(),tmp.nmemb());
    }

    return *this;
}

/**********************************************************************/
// METHOD: xaxis()
/// Define the xaxis properties
/**
 * Define the xaxis properties.
 */

Histogram&
Histogram::xaxis(const char *label, const char *desc,
                 const char *units, double min, double max,
                 double step)
{
    Array1D *a = getAxis(Histogram::x);
    Array1DUniform tmp(min,max,step);

    if (a == NULL) {
        // couldn't find xaxis array, create one
        a = axis(Histogram::x,label,desc,units,"linear",
                 tmp.data(),tmp.nmemb());
    } else {
        a->label(label);
        a->desc(desc);
        a->units(units);
        a->clear();
        a->append(tmp.data(),tmp.nmemb());
    }

    return *this;
}

/**********************************************************************/
// METHOD: yaxis()
/// Define the yaxis properties
/**
 * Define the yaxis properties.
 */

Histogram&
Histogram::yaxis(const char *label, const char *desc, const char *units)
{
    Array1D *a = getAxis(Histogram::y);

    if (a == NULL) {
        // couldn't find yaxis array, create one
        a = axis(Histogram::y,label,desc,units,"linear",NULL,0);
    } else {
        a->label(label);
        a->desc(desc);
        a->units(units);
    }

    return *this;
}

/**********************************************************************/
// METHOD: yaxis()
/// Define the yaxis properties
/**
 * Define the yaxis properties.
 */

Histogram&
Histogram::yaxis(const char *label, const char *desc,
                 const char *units, const double *vals,
                 size_t nPts)
{
    Array1D *a = getAxis(Histogram::y);

    if (a == NULL) {
        // couldn't find xaxis array, create one
        a = axis(Histogram::y,label,desc,units,"linear",vals,nPts);
    } else {
        a->label(label);
        a->desc(desc);
        a->units(units);
        a->clear();
        a->append(vals,nPts);
    }

    return *this;
}

/**********************************************************************/
// METHOD: binWidths()
/// Define the bin widths
/**
 * Define the bin widths
 */

Histogram&
Histogram::binWidths(const size_t *widths, size_t nbins)
{
    // FIXME: we can probably find a better way to do this evenatually
    // Dont know how to get an array of size_t's in the axis list yet
    // maybe we store it separately, or Templatize Array1D

    SimpleDoubleBuffer b;
    if (widths == NULL) {
        return *this;
    }

    b.set(nbins);
    for (size_t i=0; i < nbins; i++) {
        double val = static_cast<double>(*(widths+i));
        b.append(&val,1);
    }
    axis("binwidths","","","","",b.bytes(),b.nmemb());

    return *this;
}

/**********************************************************************/
// METHOD: marker()
/// Define a new marker
/**
 * Define a new marker
 */

Histogram&
Histogram::marker(const char *axisName, double at, const char *label,
                  const char *style)
{
    AxisMarker *m = new AxisMarker(axisName,label,style,at);

    if (_markerList == NULL) {
        _markerList = Rp_ChainCreate();
        if (_markerList == NULL) {
            delete m;
            return *this;
        }
    }

    Rp_ChainAppend(_markerList,(void*)m);

    return *this;
}

/**********************************************************************/
// METHOD: xml()
/// Return the xml of the object
/**
 * Return the xml of the object
 */

const char *
Histogram::xml(size_t indent, size_t tabstop)
{
    size_t l1width = indent + tabstop;
    size_t l2width = indent + (2*tabstop);
    const char *sp = "";

    Path p(path());

    Array1D *tmpAxis = NULL;
    size_t nmemb = 0;

    const double *dataArr[dims()];

    _tmpBuf.clear();

    _tmpBuf.appendf(
"%8$*5$s<histogram id=\"%1$s\">\n\
%8$*6$s<about>\n\
%8$*7$s<group>%2$s</group>\n\
%8$*7$s<label>%3$s</label>\n\
%8$*7$s<description>%4$s</description>\n\
%8$*6$s</about>\n",
        p.id(),group(),label(),desc(),indent,l1width,l2width,sp);

    for (size_t dim=0; dim < dims(); dim++) {
        tmpAxis = getNthAxis(dim);
        nmemb = tmpAxis->nmemb();
        dataArr[dim] = tmpAxis->data();
        if (strcmp("binwidths",tmpAxis->name()) == 0) {
            continue;
        }
        _tmpBuf.appendf(
"%8$*6$s<%1$s>\n\
%8$*7$s<label>%2$s</label>\n\
%8$*7$s<description>%3$s</description>\n\
%8$*7$s<units>%4$s</units>\n\
%8$*7$s<scale>%5$s</scale>\n",
        tmpAxis->name(), tmpAxis->label(), tmpAxis->desc(),
        tmpAxis->units(), tmpAxis->scale(),l1width,l2width,sp);

        if (_markerList != NULL) {
            Rp_ChainLink *l = Rp_ChainFirstLink(_markerList);
            while (l != NULL) {
                AxisMarker *m = (AxisMarker *) Rp_ChainGetValue(l);
                if (strcmp(tmpAxis->name(),m->axisName()) == 0) {
                    _tmpBuf.append(m->xml(indent+tabstop,tabstop));
                }
                l = Rp_ChainNextLink(l);
            }
        }
        _tmpBuf.appendf("%3$*2$s</%1$s>\n",tmpAxis->name(),l1width,sp);
    }

    _tmpBuf.appendf("%3$*1$s<component>\n%3$*2$s<xhw>\n",l1width,l2width,sp);
    for (size_t idx=0; idx < nmemb; idx++) {
        for (size_t dim=0; dim < dims(); dim++) {
            _tmpBuf.appendf("%10g",dataArr[dim][idx]);
        }
        _tmpBuf.append("\n",1);
    }
    _tmpBuf.appendf("%4$*3$s</xhw>\n%4$*2$s</component>\n%4$*1$s</curve>",
        indent,l1width,l2width,sp);

    return _tmpBuf.bytes();
}

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
Histogram::is() const
{
    // return "hist" in hex
    return 0x68697374;
}

/**********************************************************************/
// FUNCTION: axisMarkerCpyFxn()
/// Copy an axisMarker object
/**
 * Copy an axisMarker object
 */

/*
int
axisMarkerCpyFxn(void **to, void *from)
{
    int retVal = 0;
    size_t len = 0;
    char *tmp = NULL;
    Histogram::axisMarker *totmp = NULL;

    if (from == NULL) {
        return -1;
    }

    totmp = new Historgram::axisMarker();

    len = strlen(from->axisName);
    tmp = new char[len+1];
    strncpy(tmp,from->axisName,len+1);
    totmp->axisName = tmp;

    len = strlen(from->label);
    tmp = new char[len+1];
    strncpy(tmp,from->label,len+1);
    totmp->label = tmp;

    len = strlen(from->style);
    tmp = new char[len+1];
    strncpy(tmp,from->style,len+1);
    totmp->style = tmp;

    totmp->at = at;

    *to = totmp;

    return retVal;
}
*/

// -------------------------------------------------------------------- //

