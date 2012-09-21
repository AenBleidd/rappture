/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Units Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "RpUnits.h"
#include "RpUnitsCInterface.h"

const RpUnits*
rpDefineUnit(const char* unitSymbol, const RpUnits* basis) {

    return RpUnits::define(unitSymbol, basis);
}

const RpUnits*
rpDefineConv(  const RpUnits* fromUnit,
               const RpUnits* toUnit,
               double (*convForwFxnPtr)(double),
               double (*convBackFxnPtr)(double)    ) {

    return RpUnits::define(fromUnit,toUnit,convForwFxnPtr,convBackFxnPtr);
}

const RpUnits*
rpFind ( const char* key ) {

    return RpUnits::find(key);
}

const char*
rpGetUnits ( const RpUnits* unit ) {

    static std::string retVal;
    retVal = unit->getUnits();
    return retVal.c_str();
}

const char*
rpGetUnitsName ( const RpUnits* unit ) {

    static std::string retVal;
    retVal = unit->getUnitsName();
    return retVal.c_str();
}

double
rpGetExponent ( const RpUnits* unit ) {

    return unit->getExponent();
}

const RpUnits*
rpGetBasis ( const RpUnits* unit ) {

    return unit->getBasis();
}

const char*
rpConvert (   const char* fromVal,
               const char* toUnitsName,
               int showUnits,
               int* result ) {

    static std::string retVal;
    const char *empty = "";

    if (fromVal == NULL) {
        return retVal.c_str();
    }

    if (toUnitsName == NULL) {
        toUnitsName = empty;
    }

    retVal = RpUnits::convert(fromVal,toUnitsName,showUnits,result);
    return retVal.c_str();
}

const char*
rpConvertStr (   const char* fromVal,
                  const char* toUnitsName,
                  int showUnits,
                  int* result ) {

    static std::string retVal;
    const char *empty = "";

    if (fromVal == NULL) {
        return retVal.c_str();
    }

    if (toUnitsName == NULL) {
        toUnitsName = empty;
    }

    retVal = RpUnits::convert(fromVal,toUnitsName,showUnits,result);
    return retVal.c_str();
}

const char*
rpConvert_ObjStr ( const RpUnits* fromUnits,
                   const RpUnits* toUnits,
                   double val,
                   int showUnits,
                   int* result ) {

    static std::string retVal;
    retVal = fromUnits->convert(toUnits,val,showUnits,result);
    return retVal.c_str();
}

double
rpConvertDbl (    const char* fromVal,
                  const char* toUnitsName,
                  int* result ) {

    std::string convStr;
    double retVal = 0.0;
    const char *empty = "";

    if (fromVal == NULL) {
        return retVal;
    }

    if (toUnitsName == NULL) {
        toUnitsName = empty;
    }

    convStr = RpUnits::convert(fromVal,toUnitsName,0,result);

    if (!convStr.empty()) {
        retVal = atof(convStr.c_str());
    }

    return retVal;
}

double
rpConvert_ObjDbl (   const RpUnits* fromUnits,
                     const RpUnits* toUnits,
                     double val,
                     int* result ) {

    return fromUnits->convert(toUnits,val,result);
}

int rpAddPresets ( const char* presetName ) {

    return RpUnits::addPresets(presetName);
}

