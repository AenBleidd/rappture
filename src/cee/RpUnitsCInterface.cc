/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Units Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpUnits.h"
#include "RpUnitsCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

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

int
rpMakeMetric(const RpUnits* basis) {

    return RpUnits::makeMetric(basis);
}

const char*
rpConvert (   const char* fromVal,
               const char* toUnitsName,
               int showUnits,
               int* result ) {

    static std::string retVal;
    retVal = RpUnits::convert(fromVal,toUnitsName,showUnits,result);
    return retVal.c_str();
}

const char*
rpConvertStr (   const char* fromVal,
                  const char* toUnitsName,
                  int showUnits,
                  int* result ) {

    static std::string retVal;
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

#ifdef __cplusplus
}
#endif
