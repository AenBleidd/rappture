/*
 * ----------------------------------------------------------------------
 *  Rappture 2.0 Number Object Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
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
     _defSet (0),
     _curSet (0),
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
     _defSet (0),
     _curSet (0),
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
      _defSet (0),
      _curSet (0),
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
      _maxSet (o._maxSet),
      _defSet (o._defSet),
      _curSet (o._curSet)
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

int
Number::minset() const
{
    return _minSet;
}

int
Number::maxset() const
{
    return _maxSet;
}

int
Number::defset() const
{
    return _defSet;
}

int
Number::curset() const
{
    return _curSet;
}

/**********************************************************************/
// METHOD: convert()
/// Convert the number object to another unit from string
/**
 * Store the result as the currentValue.
 */

Outcome&
Number::convert(const char *to)
{
    const RpUnits* toUnit = NULL;
    const RpUnits* fromUnit = NULL;

    _status.addContext("Rappture::Number::convert");

    if (to == NULL) {
        return _status;
    }

    if (strcmp(units(),to) == 0) {
        return _status;
    }

    // make sure all units functions accept char*'s
    toUnit = RpUnits::find(std::string(to));
    if (!toUnit) {
        _status.addError("conversion not defined, unit \"%s\" does not exist",to);
        return _status;
    }

    fromUnit = RpUnits::find(std::string(units()));
    if (!fromUnit) {
        _status.addError("conversion not defined, unit \"%s\" does not exist",to);
        return _status;
    }

    // perform the conversion
    int err = 0;
    double convertedVal;

    convertedVal = fromUnit->convert(toUnit,def(), &err);
    if (err) {
        _status.addError("undefined error while converting %s to %s",
            (fromUnit->getUnitsName()).c_str(),
            (toUnit->getUnitsName()).c_str());
    } else {
        def(convertedVal);
    }

    convertedVal = fromUnit->convert(toUnit,cur(), &err);
    if (err) {
        _status.addError("undefined error while converting %s to %s",
            (fromUnit->getUnitsName()).c_str(),
            (toUnit->getUnitsName()).c_str());
    } else {
        cur(convertedVal);
    }

    if (_minSet) {
        convertedVal = fromUnit->convert(toUnit,min(), &err);
        if (err) {
            _status.addError("undefined error while converting %s to %s",
                (fromUnit->getUnitsName()).c_str(),
                (toUnit->getUnitsName()).c_str());
        } else {
            min(convertedVal);
        }
    }

    if (_maxSet) {
        convertedVal = fromUnit->convert(toUnit,max(), &err);
        if (err) {
            _status.addError("undefined error while converting %s to %s",
                (fromUnit->getUnitsName()).c_str(),
                (toUnit->getUnitsName()).c_str());
        } else {
            max(convertedVal);
        }
    }
    return _status;
}

/**********************************************************************/
// METHOD: value()
/// Get the value of this object converted to specified units
/**
 * does not change the value of the object
 * error code is set on trouble
 */

double
Number::value(const char *to) const
{
    const RpUnits* toUnit = NULL;
    const RpUnits* fromUnit = NULL;

    _status.addContext("Rappture::Number::value");

    double val = 0.0;

    if (_defSet) {
        val = def();
    }

    if (_curSet) {
        val = cur();
    }

    if (to == NULL) {
        return val;
    }

    if (strcmp(units(),to) == 0) {
        return val;
    }

    // make sure all units functions accept char*'s
    toUnit = RpUnits::find(std::string(to));
    if (!toUnit) {
        _status.addError("conversion not defined, unit \"%s\" does not exist",to);
        return val;
    }

    fromUnit = RpUnits::find(std::string(units()));
    if (!fromUnit) {
        _status.addError("conversion not defined, unit \"%s\" does not exist",to);
        return val;
    }

    // perform the conversion
    int err = 0;
    double convertedVal = fromUnit->convert(toUnit,val, &err);
    if (err) {
        _status.addError("undefined error while converting %s to %s",
            (fromUnit->getUnitsName()).c_str(),
            (toUnit->getUnitsName()).c_str());
        return cur();
    }

    return convertedVal;
}

/**********************************************************************/
// METHOD: vvalue()
/// Get the value of this object based on provided hints
/**
 * does not change the value of the object
 * currently recognized hints:
 *  units
 * error code is set on trouble
 */

void
Number::vvalue(void *storage, size_t numHints, va_list arg) const
{
    char buf[1024];
    char *hintCopy = NULL;
    size_t hintLen = 0;

    char *hint = NULL;
    const char *hintKey = NULL;
    const char *hintVal = NULL;

    double *ret = (double *) storage;

    *ret = 0.0;

    if (_defSet) {
        *ret = def();
    }

    if (_curSet) {
        *ret = cur();
    }

    while (numHints > 0) {
        numHints--;
        hint = va_arg(arg, char *);
        hintLen = strlen(hint);
        if (hintLen < 1024) {
            hintCopy = buf;
        } else {
            // buf too small, allocate some space
            hintCopy = new char[hintLen];
        }
        strcpy(hintCopy,hint);

        // parse the hint into a key and value
        __hintParser(hintCopy,&hintKey,&hintVal);

        // evaluate the hint key
        if (('u' == *hintKey) &&
            (strcmp("units",hintKey) == 0)) {
            *ret = value(hintVal);

        }

        if (hintCopy != buf) {
            // clean up memory
            delete hintCopy;
        }
    }
    return;
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

    p.type("description");
    Rp_ParserXmlPutF(parser,p.path(),"%s",desc());

    p.del();
    p.type("units");
    Rp_ParserXmlPutF(parser,p.path(),"%s",units());


    if (_minSet) {
        p.type("min");
        Rp_ParserXmlPutF(parser,p.path(),"%g%s",min(),units());
    }

    if (_maxSet) {
        p.type("max");
        Rp_ParserXmlPutF(parser,p.path(),"%g%s",max(),units());
    }

    p.type("default");
    Rp_ParserXmlPutF(parser,p.path(),"%g%s",def(),units());

    p.type("current");
    Rp_ParserXmlPutF(parser,p.path(),"%g%s",cur(),units());

    // process presets
    p.type("preset");
    p.add("label");
    Rp_ChainLink *l = Rp_ChainFirstLink(_presets);
    while (l != NULL) {
        struct preset *presetObj = (struct preset *) Rp_ChainGetValue(l);

        p.type("label");
        Rp_ParserXmlPutF(parser,p.path(),"%s",presetObj->label());
        //p.type("description");
        //Rp_ParserXmlPutF(parser,p.path(),"%s",presetObj->desc());
        // p.type("units");
        // Rp_ParserXmlPutF(parser,p.path(),"%s",presetObj->units());
        p.type("value");
        Rp_ParserXmlPutF(parser,p.path(),"%g%s",presetObj->val(),presetObj->units());

        p.prev();
        p.degree(p.degree()+1);
        p.next();

        l = Rp_ChainNextLink(l);
    }

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


void
Number::__convertFromString(
    const char *val,
    double *ret)
{
    if (val == NULL) {
        return;
    }

    if (ret == NULL) {
        return;
    }

    double numericVal = 0.0;
    if (units()) {
        // convert the provided string into
        // the objects default units
        int err = 0;
        std::string strVal;
        strVal = RpUnits::convert(val,units(),RPUNITS_UNITS_OFF,&err);
        if (err) {
            _status.addError("Unknown error while converting units");
        }

        char *endPtr = NULL;
        numericVal = strtod(strVal.c_str(),&endPtr);
        if (endPtr == strVal.c_str()) {
            // no conversion was performed
            _status.addError("Could not convert \"%s\" into a number",
                strVal.c_str());
        } else if (endPtr == (strVal.c_str()+strVal.length())) {
            *ret = numericVal;
        } else {
            // the whole string could not be converted to a number
            // signal error?
            _status.addError("Could not convert \"%s\" of \"%s\"into a number",
                endPtr, strVal.c_str());
        }
    } else {
        // try to figure out the units
        // store units and return the numeric value to the user
        const char *foundUnits = NULL;
        __valUnitsSplit(val,&numericVal,&foundUnits);
        units(foundUnits);
        *ret = numericVal;
    }

}

void
Number::__valUnitsSplit(
    const char *inStr,
    double *val,
    const char **units)
{
    if (inStr == NULL) {
        return;
    }

    if (val == NULL) {
        return;
    }

    if (units == NULL) {
        return;
    }

    char *endPtr = NULL;
    *val = strtod(inStr,&endPtr);
    if (endPtr == inStr) {
        // no conversion was performed
        _status.addError("Could not convert \"%s\" into a number", inStr);
    } else if (endPtr == (inStr+strlen(inStr))) {
        // the whole string was used in the numeric conversion
        // set units to NULL
        *units = NULL;
    } else {
        // the whole string could not be converted to a number
        // we assume the rest of the string are the units
        *units = endPtr;
    }
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

    if (val == NULL) {
        return;
    }

    __convertFromString(val,&numericVal);

    if (!_status) {
        min(numericVal);
        _minSet = 1;
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

    if (val == NULL) {
        return;
    }

    __convertFromString(val,&numericVal);

    if (!_status) {
        max(numericVal);
        _maxSet = 1;
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

    if (val == NULL) {
        return;
    }

    __convertFromString(val,&numericVal);

    if (!_status) {
        def(numericVal);
        _defSet = 1;
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

    if (val == NULL) {
        return;
    }

    __convertFromString(val,&numericVal);

    if (!_status) {
        cur(numericVal);
        _curSet = 1;
    }

}



// -------------------------------------------------------------------- //

