#include "RpUnits.h"
#include "RpDict.h"
#include "string.h"
#include "RpUnitsFInterface.h"

#define DICT_TEMPLATE <int,std::string>
RpDict DICT_TEMPLATE fortObjDictUnits;

int storeObject_UnitsStr(std::string objectName);
RpUnits* getObject_UnitsInt(int objKey);

int rp_define_unit( char* unitName, 
                    int* basisName, 
                    int unitName_len    ) {

    int result = -1;
    RpUnits* newUnit = NULL;
    RpUnits* basis = NULL;
    std::string basisStrName = "";
    std::string newUnitName = "";
    char* inText = NULL;

    inText = null_terminate(unitName,unitName_len);

    if (basisName && *basisName) {
        basisStrName = fortObjDictUnits.find(*basisName);

        if (basisStrName != "") {
            basis = RpUnits::find(basisStrName);
        }
    }

    // newUnit = RpUnits::define(unitName, basis);
    newUnit = RpUnits::define(inText, basis);
    
    if (newUnit) {
        result = storeObject_UnitsStr(newUnit->getUnitsName());
    }

    if (inText) {
        free(inText);
    }

    return result;
}


int rp_find(char* searchName, int searchName_len)
{

    int result = -1;
    char* inText = NULL;

    inText = null_terminate(searchName,searchName_len);

    // RpUnits* searchUnit = RpUnits::find(searchName);
    RpUnits* searchUnit = RpUnits::find(inText);

    if (searchUnit) {
        result = storeObject_UnitsStr(searchUnit->getUnitsName());
    }

    if (inText) {
        free(inText);
    }

    return result;
}

int rp_make_metric(int* basis)
{
    int result = -1;
    RpUnits* newBasis = NULL;

    if (basis && *basis) {
        newBasis = getObject_UnitsInt(*basis);

        if (newBasis) {
            result = RpUnits::makeMetric(newBasis);
        }
    }

    return result;
}

int rp_get_units(int* unitRefVal, char* retText, int retText_len)
{
    RpUnits* unitObj = NULL;
    std::string unitNameText = "";
    int result = 0;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject_UnitsInt(*unitRefVal);
        if (unitObj) {
            unitNameText = unitObj->getUnits();
            fortranify(unitNameText.c_str(), retText, retText_len);
        }
    }

    return result;
}

int rp_get_units_name(int* unitRefVal, char* retText, int retText_len)
{
    RpUnits* unitObj = NULL;
    std::string unitNameText = "";
    int result = 0;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject_UnitsInt(*unitRefVal);
        if (unitObj) {
            unitNameText = unitObj->getUnitsName();
            fortranify(unitNameText.c_str(), retText, retText_len);
        }
    }
    
    return result;
}

int rp_get_exponent(int* unitRefVal, double* retExponent)
{
    RpUnits* unitObj = NULL;
    int result = 0;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject_UnitsInt(*unitRefVal);
        if (unitObj) {
            *retExponent = unitObj->getExponent();
        }
    }

    return result;
}

int rp_get_basis(int* unitRefVal)
{
    RpUnits* unitObj = NULL;
    RpUnits* basisObj = NULL;
    int result = -1;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject_UnitsInt(*unitRefVal);

        if (unitObj) {
            basisObj = unitObj->getBasis();
            
            if (basisObj) {
                result = storeObject_UnitsStr(basisObj->getUnitsName());
            }
        }
    }

    return result;
}

int rp_units_convert_dbl (  char* fromVal, 
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

int rp_units_convert_str (      char* fromVal, 
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

int rp_units_add_presets ( char* presetName, int presetName_len) {
    
    char* inPresetName = NULL;
    int result = -1;
    
    inPresetName = null_terminate(presetName,presetName_len);

    result = RpUnits::addPresets(inPresetName);

    if (inPresetName) {
        free(inPresetName);
        inPresetName = NULL;
    }

    return result;
}




//**********************************************************************//


int storeObject_UnitsStr(std::string objectName) {

    int retVal = -1;
    int dictNextKey = fortObjDictUnits.size() + 1;
    int newEntry = 0;

    if (objectName != "") {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.
        fortObjDictUnits.set(dictNextKey,objectName, &newEntry); 
    }
    
    retVal = dictNextKey;
    return retVal;
}

RpUnits* getObject_UnitsInt(int objKey) {

    std::string basisName = *(fortObjDictUnits.find(objKey).getValue());

    if (basisName == *(fortObjDictUnits.getNullEntry().getValue())) {
        // basisName = "";
        return NULL;
    }

   return RpUnits::find(basisName);

}
