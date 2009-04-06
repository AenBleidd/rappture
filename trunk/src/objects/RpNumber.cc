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

using namespace Rappture;

Number::Number (
            const char *path,
            const char *units,
            double val
        )
    :   Variable    (),
        _minmaxSet  (0),
        _presets    (NULL)
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

Number::Number (
            const char *path,
            const char *units,
            double val,
            double min,
            double max,
            const char *label,
            const char *desc
        )
    :   Variable    (),
        _minmaxSet  (0),
        _presets    (NULL)
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
        _minmaxSet = 0;
    }
    else {

        if (min > val) {
            this->min(val);
        }

        if (max < val) {
            this->max(val);
        }
    }
}

// copy constructor
Number::Number ( const Number& o )
    :   Variable(o),
        _minmaxSet  (o._minmaxSet)
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

/**********************************************************************/
// METHOD: convert()
/// Convert the number object to another unit from string
/**
 * Store the result as the currentValue.
 */

double
Number::convert(const char *to) {

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

    if (err) {
        convertedVal = cur();
    }

    return convertedVal;
}

/**********************************************************************/
// METHOD: addPreset()
/// Add a preset / suggessted value to the object
/**
 * Add a preset value to the object. Currently all
 * labels must be unique.
 */

Number&
Number::addPreset(
    const char *label,
    const char *desc,
    double val,
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

// -------------------------------------------------------------------- //

