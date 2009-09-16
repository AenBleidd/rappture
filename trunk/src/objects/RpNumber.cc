/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Number Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpNumber.h"
#include "RpUnits.h"
#include "RpSimpleBuffer.h"
#include "RpUnitsCInterface.h"
#include "RpPath.h"

using namespace Rappture;

Number::Number()
   : Object (),
     _minSet (0),
     _maxSet (0),
     _presets (NULL)
{
    // FIXME: empty names should be autoname'd
    this->name("");
    this->path("run");
    this->label("");
    this->desc("");
    this->def(0.0);
    this->cur(0.0);
    this->min(0.0);
    this->max(0.0);
    // FIXME: empty units should be set to the None unit
    // this->units(units);
}

Number::Number(const char *name, const char *units, double val)
   : Object (),
     _minSet (0),
     _maxSet (0),
     _presets (NULL)
{
    const RpUnits *u = NULL;

    this->name(name);
    this->path("run");
    this->label("");
    this->desc("");
    this->def(val);
    this->cur(val);
    this->min(0.0);
    this->max(0.0);

    u = RpUnits::find(std::string(units));
    if (!u) {
        u = RpUnits::define(units,NULL);
    }
    this->units(units);
}

Number::Number(const char *name, const char *units, double val,
               double min, double max, const char *label,
               const char *desc)
    : Object (),
      _minSet (0),
      _maxSet (0),
      _presets (NULL)
{
    const RpUnits *u = NULL;

    this->name(name);
    this->path("run");
    this->label(label);
    this->desc(desc);
    this->def(val);
    this->cur(val);
    this->min(min);
    this->max(max);

    u = RpUnits::find(std::string(units));
    if (! u) {
        u = RpUnits::define(units,NULL);
    }
    this->units(units);

    if ((min == 0) && (max == 0)) {
        _minSet = 0;
        _maxSet = 0;
    }
    else {

        _minSet = 1;
        if (min > val) {
            this->min(val);
        }

        _maxSet = 1;
        if (max < val) {
            this->max(val);
        }
    }
}

// copy constructor
Number::Number ( const Number& o )
    : Object (o),
      _minSet (o._minSet),
      _maxSet (o._maxSet)
{
    this->def(o.def());
    this->cur(o.cur());
    this->min(o.min());
    this->max(o.max());
    this->units(o.units());

    // FIXME: need to copy _presets
}

// default destructor
Number::~Number ()
{
    // clean up dynamic memory
}

const char *
Number::units(void) const
{
    return propstr("units");
}

void
Number::units(const char *p)
{
    propstr("units",p);
}

/**********************************************************************/
// METHOD: convert()
/// Convert the number object to another unit from string
/**
 * Store the result as the currentValue.
 */

int
Number::convert(const char *to)
{
    const RpUnits* toUnit = NULL;
    const RpUnits* fromUnit = NULL;
    double convertedVal = cur();
    int err = 0;

    // make sure all units functions accept char*'s
    toUnit = RpUnits::find(std::string(to));
    if (!toUnit) {
        // should raise error!
        // conversion not defined because unit does not exist
        return convertedVal;
    }

    fromUnit = RpUnits::find(std::string(units()));
    if (!fromUnit) {
        // should raise error!
        // conversion not defined because unit does not exist
        return convertedVal;
    }

    // perform the conversion
    convertedVal = fromUnit->convert(toUnit,def(), &err);
    if (!err) {
        def(convertedVal);
    }

    convertedVal = fromUnit->convert(toUnit,cur(), &err);
    if (!err) {
        cur(convertedVal);
    }

    return err;
}

/**********************************************************************/
// METHOD: value()
/// Get the value of this object converted to specified units
/**
 * does not change the value of the object
 * error code is returned
 */

int
Number::value(const char *to, double *value) const
{
    return 1;
}

/**********************************************************************/
// METHOD: addPreset()
/// Add a preset / suggessted value to the object
/**
 * Add a preset value to the object. Currently all
 * labels must be unique.
 */

Number&
Number::addPreset(const char *label, const char *desc, double val,
                  const char *units)
{
    preset *p = NULL;

    p = new preset;
    if (!p) {
        // raise error and exit
    }

    p->label(label);
    p->desc(desc);
    p->val(val);
    p->units(units);

    if (_presets == NULL) {
        _presets = Rp_ChainCreate();
        if (_presets == NULL) {
            // raise error and exit
        }
    }

    Rp_ChainAppend(_presets,p);

    return *this;
}

Number&
Number::addPreset(const char *label, const char *desc, const char *val)
{
    double valval = 0.0;
    const char *valunits = "";
    char *endptr = NULL;
    int result = 0;

    std::string vstr = RpUnits::convert(val,"",RPUNITS_UNITS_OFF,&result);
    if (result) {
        // probably shouldnt trust this result
        fprintf(stderr,"error in RpUnits::convert in addPreset\n");
    }
    size_t len = vstr.length();
    valunits = val+len;

    valval = strtod(val,&endptr);
    if ( (endptr == val) || (endptr != valunits) ) {
        // error? strtod was not able to find the same
        // units location as RpUnits::convert
        fprintf(stderr,"error while parsing units in addPreset\n");
    }

    return addPreset(label,desc,valval,valunits);
}

/**********************************************************************/
// METHOD: delPreset()
/// Delete a preset / suggessted value from the object
/**
 * Delete a preset value from the object.
 */

Number&
Number::delPreset(const char *label)
{
    if (label == NULL) {
        return *this;
    }

    if (_presets == NULL) {
        return *this;
    }

    preset *p = NULL;
    const char *plabel = NULL;
    Rp_ChainLink *l = NULL;

    // traverse the list looking for the matching preset
    l = Rp_ChainFirstLink(_presets);
    while (l != NULL) {
        p = (preset *) Rp_ChainGetValue(l);
        plabel = p->label();
        if ((*label == *plabel) && (strcmp(plabel,label) == 0)) {
            // we found matching entry, remove it
            if (p) {
                delete p;
                Rp_ChainDeleteLink(_presets,l);
            }
            break;
        }
    }


    return *this;
}

/**********************************************************************/
// METHOD: configure(ClientData c)
/// construct a number object from the provided tree
/**
 * construct a number object from the provided tree
 */

void
Number::configure(size_t as, ClientData c)
{
    if (as == RPCONFIG_XML) {
        __configureFromXml(c);
    } else if (as == RPCONFIG_TREE) {
        __configureFromTree(c);
    }
}

/**********************************************************************/
// METHOD: configureFromXml(const char *xmltext)
/// configure the object based on Rappture1.1 xmltext
/**
 * Configure the object based on the provided xml
 */

void
Number::__configureFromXml(ClientData c)
{
    const char *xmltext = (const char *)c;
    if (xmltext == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_ParserXml *p = Rp_ParserXmlCreate();
    Rp_ParserXmlParse(p, xmltext);
    configure(RPCONFIG_TREE, p);

    return;
}

void
Number::__configureFromTree(ClientData c)
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

    pathObj.clear();
    pathObj.add("about");
    pathObj.add("label");
    label(Rp_ParserXmlGet(p,pathObj.path()));
    pathObj.type("description");
    desc(Rp_ParserXmlGet(p,pathObj.path()));
    units(Rp_ParserXmlGet(p,"units"));
    minFromStr(Rp_ParserXmlGet(p,"min"));
    maxFromStr(Rp_ParserXmlGet(p,"max"));
    defFromStr(Rp_ParserXmlGet(p,"default"));
    curFromStr(Rp_ParserXmlGet(p,"current"));

    // collect info about the preset values
    Rp_Chain *childChain = Rp_ChainCreate();
    Rp_ParserXmlChildren(p,NULL,"preset",childChain);
    Rp_ChainLink *l = Rp_ChainFirstLink(childChain);
    while (l != NULL) {
        Rp_TreeNode presetNode = (Rp_TreeNode) Rp_ChainGetValue(l);
        Rp_ParserXmlBaseNode(p,presetNode);

        const char *presetlabel = Rp_ParserXmlGet(p,"label");
        const char *presetdesc = Rp_ParserXmlGet(p,"description");
        const char *presetvalue = Rp_ParserXmlGet(p,"value");
        addPreset(presetlabel,presetdesc,presetvalue);


        l = Rp_ChainNextLink(l);
    }

    Rp_ChainDestroy(childChain);

    // return the base node to the tree root
    Rp_ParserXmlBaseNode(p,NULL);

    return;
}

/**********************************************************************/
// METHOD: dump(size_t as, void *p)
/// construct a number object from the provided tree
/**
 * construct a number object from the provided tree
 */

void
Number::dump(size_t as, ClientData p)
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
Number::__dumpToXml(ClientData c)
{
    if (c == NULL) {
        // FIXME: setup error
        return;
    }

    ClientDataXml *d = (ClientDataXml *)c;
    Rp_ParserXml *parser = Rp_ParserXmlCreate();
    __dumpToTree(parser);
    _tmpBuf.appendf("%s",Rp_ParserXmlXml(parser));
    d->retStr = _tmpBuf.bytes();
    Rp_ParserXmlDestroy(&parser);
}

/**********************************************************************/
// METHOD: dumpToTree(ClientData p)
/// dump the object to a Rappture1.1 based tree
/**
 * Dump the object to a Rappture1.1 based tree
 */

void
Number::__dumpToTree(ClientData c)
{
    if (c == NULL) {
        // FIXME: setup error
        return;
    }

    Rp_ParserXml *parser = (Rp_ParserXml *)c;

    Path p;

    p.parent(path());
    p.last();

    p.add("number");
    p.id(name());

    p.add("about");

    p.add("label");
    Rp_ParserXmlPutF(parser,p.path(),"%s",label());

    p.del();
    p.add("description");
    Rp_ParserXmlPutF(parser,p.path(),"%s",desc());

    p.del();
    p.del();
    p.add("units");
    Rp_ParserXmlPutF(parser,p.path(),"%s",units());


    if (_minSet) {
        p.del();
        p.add("min");
        Rp_ParserXmlPutF(parser,p.path(),"%g",min());
    }

    if (_maxSet) {
        p.del();
        p.add("max");
        Rp_ParserXmlPutF(parser,p.path(),"%g",max());
    }

    p.del();
    p.add("default");
    Rp_ParserXmlPutF(parser,p.path(),"%g",def());

    p.del();
    p.add("current");
    Rp_ParserXmlPutF(parser,p.path(),"%g",cur());

    // still need to add presets
    return;
}

/**********************************************************************/
// METHOD: is()
/// what kind of object is this
/**
 * return hex value telling what kind of object this is.
 */

const int
Number::is() const
{
    // return "numb" in hex
    return 0x6E756D62;
}


/**********************************************************************/
// METHOD: minFromStr()
/// xml helper function to receive min value as a string
/**
 * convert string to value and units and store as min
 */

void
Number::minFromStr(
    const char *val)
{
    double numericVal = 0;
    int err = 0;

    numericVal = rpConvertDbl(val,units(),&err);

    if (!err) {
        min(numericVal);
    } else {
        // FIXME: add error code
    }
}

/**********************************************************************/
// METHOD: maxFromStr()
/// xml helper function to receive max value as a string
/**
 * convert string to value and units and store as max
 */

void
Number::maxFromStr(
    const char *val)
{
    double numericVal = 0;
    int err = 0;

    numericVal = rpConvertDbl(val,units(),&err);

    if (!err) {
        max(numericVal);
    } else {
        // FIXME: add error code
    }
}

/**********************************************************************/
// METHOD: defFromStr()
/// xml helper function to receive default value as a string
/**
 * convert string to value and units and store as default
 */

void
Number::defFromStr(
    const char *val)
{
    double numericVal = 0;
    int err = 0;

    numericVal = rpConvertDbl(val,units(),&err);

    if (!err) {
        def(numericVal);
    } else {
        // FIXME: add error code
    }
}

/**********************************************************************/
// METHOD: currFromStr()
/// xml helper function to receive current value as a string
/**
 * convert string to value and units and store as current
 */

void
Number::curFromStr(
    const char *val)
{
    double numericVal = 0;
    int err = 0;

    numericVal = rpConvertDbl(val,units(),&err);

    if (!err) {
        cur(numericVal);
    } else {
        // FIXME: add error code
    }
}



// -------------------------------------------------------------------- //

