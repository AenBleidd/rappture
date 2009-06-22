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

using namespace Rappture;

Number::Number()
   : Object (),
     _minSet (0),
     _maxSet (0),
     _presets (NULL)
{
    this->path("");
    this->label("");
    this->desc("");
    this->def(0.0);
    this->cur(0.0);
    this->min(0.0);
    this->max(0.0);
    // need to set this to the None unit
    // this->units(units);
}

Number::Number(const char *path, const char *units, double val)
   : Object (),
     _minSet (0),
     _maxSet (0),
     _presets (NULL)
{
    const RpUnits *u = NULL;

    this->path(path);
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

Number::Number(const char *path, const char *units, double val,
               double min, double max, const char *label,
               const char *desc)
    : Object (),
      _minSet (0),
      _maxSet (0),
      _presets (NULL)
{
    const RpUnits *u = NULL;

    this->path(path);
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

    // need to copy _presets
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
// METHOD: xml()
/// view this object's xml
/**
 * View this object as an xml element returned as text.
 */

const char *
Number::xml()
{
    Path p(path());
    _tmpBuf.clear();

    _tmpBuf.appendf(
"<number id='%s'>\n\
    <about>\n\
        <label>%s</label>\n\
        <description>%s</description>\n\
    </about>\n\
    <units>%s</units>\n",
       p.id(),label(),desc(),units());

    if (_minSet) {
        _tmpBuf.appendf("    <min>%g%s</min>\n", min(),units());
    }
    if (_maxSet) {
        _tmpBuf.appendf("    <max>%g%s</max>\n", max(),units());
    }

    _tmpBuf.appendf(
"    <default>%g%s</default>\n\
    <current>%g%s</current>\n\
</number>",
       def(),units(),cur(),units());

    return _tmpBuf.bytes();
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


// -------------------------------------------------------------------- //

