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

#include <cstring>
#include <stdlib.h>
#include "RpNumber.h"

#ifndef _RpUNITS_H
    #include "RpUnits.h"
#endif

RpNumber::RpNumber (
            const char *path,
            const char *unit,
            double val
        )
    :   RpVariable  (),
        _default    (val),
        _current    (val),
        _min        (0.0),
        _max        (0.0),
        _minmaxSet  (0.0)
{
    const RpUnits *u = NULL;

    property("path",(const void *)path);

    u = RpUnits::find(std::string(unit));
    if (!u) {
        u = RpUnits::define(unit,NULL);
    }
    property("units",(const void *)unit);

    property("default",(const void *)&_default);
    property("current",(const void *)&_current);
    property("min",(const void *)&_min);
    property("max",(const void *)&_max);
}

RpNumber::RpNumber (
            const char *path,
            const char *unit,
            double val,
            double min,
            double max,
            const char *label,
            const char *desc
        )
    :   RpVariable  (),
        _default    (val),
        _current    (val),
        _min         (min),
        _max         (max),
        _minmaxSet   (0)
{
    const RpUnits *u = NULL;

    property("path",(const void *)path);

    u = RpUnits::find(std::string(unit));
    if (! u) {
        u = RpUnits::define(unit,NULL);
    }
    property("units",(const void *)unit);

    if ((min == 0) && (max == 0)) {
        _minmaxSet = 0;
    }
    else {

        if (min > val) {
            _min = val;
        }

        if (max < val) {
            _max = val;
        }
    }

    property("default",(const void *)&_default);
    property("current",(const void *)&_current);
    property("min",(const void *)&_min);
    property("max",(const void *)&_max);
}

// copy constructor
RpNumber::RpNumber ( const RpNumber& o )
    :   RpVariable(o),
        _default    (o._default),
        _current    (o._current),
        _min        (o._min),
        _max        (o._max),
        _minmaxSet  (o._minmaxSet)
{}

// default destructor
RpNumber::~RpNumber ()
{
    // clean up dynamic memory

}

/**********************************************************************/
// METHOD: units()
/// get / set the units of this number
/**
 * get / set the units property of this object
 */

const char *
RpNumber::units(
    const char *val)
{
    return (const char *) property("units",val);
}


/**********************************************************************/
// METHOD: convert()
/// Convert the number object to another unit from string
/**
 * Store the result as the currentValue.
 */

double
RpNumber::convert(const char *to) {

    const RpUnits* toUnit = NULL;
    const RpUnits* fromUnit = NULL;
    double convertedVal = _current;
    int err = 0;

    // make sure all units functions accept char*'s
    toUnit = RpUnits::find(std::string(to));
    if (!toUnit) {
        // should raise error!
        // conversion not defined because unit does not exist
        return _current;
    }

    fromUnit = RpUnits::find(std::string(units(NULL)));
    if (!fromUnit) {
        // should raise error!
        // conversion not defined because unit does not exist
        return _current;
    }

    // perform the conversion
    convertedVal = fromUnit->convert(toUnit,_default, &err);
    if (!err) {
        _default = convertedVal;
    }

    convertedVal = fromUnit->convert(toUnit,_current, &err);
    if (!err) {
        _current = convertedVal;
    }

    if (err) {
        convertedVal = _current;
    }

    return convertedVal;
}

// -------------------------------------------------------------------- //

