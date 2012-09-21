/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Units Source
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
#include "string.h"
#include "RpUnitsFInterface.h"
#include "RpUnitsFStubs.c"
#include "RpBindingsDict.h"

extern "C" int
rp_define_unit( char* unitName,
                    int* basisName,
                    int unitName_len    ) {

    int result = -1;
    const RpUnits* newUnit = NULL;
    const RpUnits* basis = NULL;
    std::string basisStrName = "";
    std::string newUnitName = "";
    char* inText = NULL;

    inText = null_terminate(unitName,unitName_len);

    if (basisName && *basisName) {
	long key = (long)*basisName;
        basisStrName = ObjDictUnits.find(key);

        if (basisStrName != "") {
            basis = RpUnits::find(basisStrName);
        }
    }

    newUnit = RpUnits::define(inText, basis);

    if (newUnit) {
        result = storeObject_UnitsStr(newUnit->getUnitsName());
    }

    if (inText) {
        free(inText);
    }

    return result;
}


int
rp_find(char* searchName, int searchName_len)
{

    int result = -1;
    char* inText = NULL;

    inText = null_terminate(searchName,searchName_len);

    const RpUnits* searchUnit = RpUnits::find(inText);

    if (searchUnit) {
        result = storeObject_UnitsStr(searchUnit->getUnitsName());
    }

    if (inText) {
        free(inText);
    }

    return result;
}

int
rp_get_units(int* unitRefVal, char* retText, int retText_len)
{
    const RpUnits* unitObj = NULL;
    std::string unitNameText = "";
    int result = 1;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject_UnitsStr(*unitRefVal);
        if (unitObj) {
            unitNameText = unitObj->getUnits();
            fortranify(unitNameText.c_str(), retText, retText_len);
            result = 0;
        }
    }

    return result;
}

int
rp_get_units_name(int* unitRefVal, char* retText, int retText_len)
{
    const RpUnits* unitObj = NULL;
    std::string unitNameText = "";
    int result = 1;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject_UnitsStr(*unitRefVal);
        if (unitObj) {
            unitNameText = unitObj->getUnitsName();
            fortranify(unitNameText.c_str(), retText, retText_len);
            result = 0;
        }
    }

    return result;
}

int
rp_get_exponent(int* unitRefVal, double* retExponent)
{
    const RpUnits* unitObj = NULL;
    int result = 1;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject_UnitsStr(*unitRefVal);
        if (unitObj) {
            *retExponent = unitObj->getExponent();
            result = 0;
        }
    }

    return result;
}

int
rp_get_basis(int* unitRefVal)
{
    const RpUnits* unitObj = NULL;
    const RpUnits* basisObj = NULL;
    int result = -1;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject_UnitsStr(*unitRefVal);

        if (unitObj) {
            basisObj = unitObj->getBasis();

            if (basisObj) {
                result = storeObject_UnitsStr(basisObj->getUnitsName());
            }
        }
    }

    return result;
}

int
rp_units_convert_dbl (  char* fromVal,
                        char* toUnitsName,
                        double* convResult,
                        int fromVal_len,
                        int toUnitsName_len ) {

    char* inFromVal = NULL;
    char* inToUnitsName = NULL;
    int result = -1;
    int showUnits = 0;
    std::string convStr = "";

    // prepare string for c
    inFromVal = null_terminate(fromVal,fromVal_len);
    inToUnitsName = null_terminate(toUnitsName,toUnitsName_len);

    if (inFromVal && inToUnitsName) {
        // the call to RpUnits::convert() populates the result flag
        convStr = RpUnits::convert(inFromVal,inToUnitsName,showUnits,&result);

        if (!convStr.empty()) {
            *convResult = atof(convStr.c_str());
        }
    }

    // clean up memory
    if (inFromVal) {
        free(inFromVal);
        inFromVal = NULL;
    }

    if (inToUnitsName) {
        free(inToUnitsName);
        inToUnitsName = NULL;
    }

    return result;
}

int
rp_units_convert_str (      char* fromVal,
                            char* toUnitsName,
                            char* retText,
                            int fromVal_len,
                            int toUnitsName_len,
                            int retText_len     ) {

    char* inFromVal = NULL;
    char* inToUnitsName = NULL;
    std::string convStr = "";
    int showUnits = 1;
    int result = -1;

    // prepare string for c
    inFromVal = null_terminate(fromVal,fromVal_len);
    inToUnitsName = null_terminate(toUnitsName,toUnitsName_len);

    if (inFromVal && inToUnitsName) {

        // do the conversion
        convStr = RpUnits::convert(inFromVal,inToUnitsName,showUnits,&result);

        if (!convStr.empty()) {
            // prepare string for fortran
            fortranify(convStr.c_str(), retText, retText_len);
        }
    }

    // clean up out memory
    if (inFromVal) {
        free(inFromVal);
        inFromVal = NULL;
    }

    if (inToUnitsName) {
        free(inToUnitsName);
        inToUnitsName = NULL;
    }

    // return the same result from the RpUnits::convert call.
    return result;
}
