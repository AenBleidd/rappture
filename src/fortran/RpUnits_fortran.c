// #include "../include/RpUnitsCInterface.h"
// #include "../include/RpDictCFInterface.h"
// #include <stdio.h>

#include "RpUnits.h"
#include "RpDict.h"
#include "string.h"

#ifdef COMPNAME_ADD1UNDERSCORE
#   define rp_define_unit       rp_define_unit_
#   define rp_find              rp_find_
#   define rp_make_metric       rp_make_metric_
#   define rp_get_units         rp_get_units_
#   define rp_get_units_name    rp_get_units_name_
#   define rp_get_exponent      rp_get_exponent_
#   define rp_get_basis         rp_get_basis_
#elif defined(COMPNAME_ADD2UNDERSCORE)
#   define rp_define_unit       rp_define_unit__
#   define rp_find              rp_find__
#   define rp_make_metric       rp_make_metric__
#   define rp_get_units         rp_get_units__
#   define rp_get_units_name    rp_get_units_name__
#   define rp_get_exponent      rp_get_exponent__
#   define rp_get_basis         rp_get_basis__
#elif defined(COMPNAME_NOCHANGE)
#   define rp_define_unit       rp_define_unit
#   define rp_find              rp_find
#   define rp_make_metric       rp_make_metric
#   define rp_get_units         rp_get_units
#   define rp_get_units_name    rp_get_units_name
#   define rp_get_exponent      rp_get_exponent
#   define rp_get_basis         rp_get_basis
#elif defined(COMPNAME_UPPERCASE)
#   define rp_define_unit       RP_DEFINE_UNIT
#   define rp_find              RP_FIND
#   define rp_make_metric       RP_MAKE_METRIC
#   define rp_get_units         RP_GET_UNITS
#   define rp_get_units_name    RP_GET_UNITS_NAME 
#   define rp_get_exponent      RP_GET_EXPONENT
#   define rp_get_basis         RP_GET_BASIS
#endif

extern "C"
int rp_define_unit(char* unitName, int* basisName, int unitName_len);

extern "C"
int rp_find(char* searchName, int searchName_len);

extern "C"
int rp_make_metric(int* basis);

extern "C"
void rp_get_units(int* unitRefVal, char* retText, int retText_len);

extern "C"
void rp_get_units_name(int* unitRefVal, char* retText, int retText_len);

extern "C"
double rp_get_exponent(int* unitRefVal);

extern "C"
int rp_get_basis(int* unitRefVal);








char* null_terminate(char* inStr, int len);
int storeObject(std::string objectName);
RpUnits* getObject(int objKey);




#define DICT_TEMPLATE <int,std::string>
// RpDict<int,std::string>fortObjDict;
RpDict DICT_TEMPLATE fortObjDict;

int rp_define_unit( char* unitName, 
                    int* basisName, 
                    int unitName_len    ) {

    int retVal = -1;
    RpUnits* newUnit = NULL;
    RpUnits* basis = NULL;
    std::string basisStrName = "";
    std::string newUnitName = "";
    char* inText = NULL;

    inText = null_terminate(unitName,unitName_len);

    if (basisName && *basisName) {
        basisStrName = fortObjDict.find(*basisName);

        if (basisStrName != "") {
            basis = RpUnits::find(basisStrName);
        }
    }

    // newUnit = RpUnits::define(unitName, basis);
    newUnit = RpUnits::define(inText, basis);
    
    if (newUnit) {
        retVal = storeObject(newUnit->getUnitsName());
    }

    if (inText) {
        free(inText);
    }

    return retVal;
}


int rp_find(char* searchName, int searchName_len)
{

    int retVal = -1;
    char* inText = NULL;

    inText = null_terminate(searchName,searchName_len);

    // RpUnits* searchUnit = RpUnits::find(searchName);
    RpUnits* searchUnit = RpUnits::find(inText);

    if (searchUnit) {
        retVal = storeObject(searchUnit->getUnitsName());
    }

    if (inText) {
        free(inText);
    }

    return retVal;
}

int rp_make_metric(int* basis)
{
    int retVal = -1;
    RpUnits* newBasis = NULL;

    if (basis && *basis) {
        newBasis = getObject(*basis);

        if (newBasis) {
            retVal = RpUnits::makeMetric(newBasis);
        }
    }

    return retVal;
}

void rp_get_units(int* unitRefVal, char* retText, int retText_len)
{
    int length_in = 0;
    int length_out = 0;
    int i = 0;
    RpUnits* unitObj = NULL;
    std::string unitNameText = "";

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject(*unitRefVal);
        if (unitObj) {
            unitNameText = unitObj->getUnits();

            length_in = unitNameText.length();
            length_out = retText_len;

            strncpy(retText, unitNameText.c_str(), length_out);

            // fortran-ify the string
            if (length_in < length_out) {
                for (i = length_in; i < length_out; i++) {
                    retText[i] = ' ';
                }
            }
                
        }
    }
}

void rp_get_units_name(int* unitRefVal, char* retText, int retText_len)
{
    int length_in = 0;
    int length_out = 0;
    int i = 0;
    RpUnits* unitObj = NULL;
    std::string unitNameText = "";

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject(*unitRefVal);
        if (unitObj) {
            unitNameText = unitObj->getUnitsName();

            length_in = unitNameText.length();
            length_out = retText_len;

            strncpy(retText, unitNameText.c_str(), length_out);

            // fortran-ify the string
            if (length_in < length_out) {
                for (i = length_in; i < length_out; i++) {
                    retText[i] = ' ';
                }
            }
                
        }
    }

}

double rp_get_exponent(int* unitRefVal)
{
    RpUnits* unitObj = NULL;
    double retVal = 0;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject(*unitRefVal);
        if (unitObj) {
            retVal = unitObj->getExponent();
        }
    }

    return retVal;
}

int rp_get_basis(int* unitRefVal)
{
    RpUnits* unitObj = NULL;
    RpUnits* basisObj = NULL;
    int retVal = -1;

    if (unitRefVal && *unitRefVal) {
        unitObj = getObject(*unitRefVal);

        if (unitObj) {
            basisObj = unitObj->getBasis();
            
            if (basisObj) {
                retVal = storeObject(basisObj->getUnitsName());
            }
        }
    }

    return retVal;
}











int storeObject(std::string objectName) {

    int retVal = -1;
    int dictNextKey = fortObjDict.size() + 1;
    int newEntry = 0;

    if (objectName != "") {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.
        fortObjDict.set(dictNextKey,objectName, &newEntry); 
    }
    
    retVal = dictNextKey;
    return retVal;
}

RpUnits* getObject(int objKey) {

    std::string basisName = *(fortObjDict.find(objKey).getValue());

    if (basisName == *(fortObjDict.getNullEntry().getValue())) {
        // basisName = "";
        return NULL;
    }

   return RpUnits::find(basisName);

}

/* fix buffer overflow possibility*/
char* null_terminate(char* inStr, int len)
{
    int retVal = 0;
    int origLen = len;
    char* newStr = NULL;

    if (inStr) {

        while ((len > 0) && (isspace(*(inStr+len-1)))) {
            // dont strip off newlines
            if ( (*(inStr+len-1) == '\f')
              || (*(inStr+len-1) == '\n')
              || (*(inStr+len-1) == '\r')
              || (*(inStr+len-1) == '\t')
              || (*(inStr+len-1) == '\v') )
            {
                break;
            }
            len--;
        }

        newStr = (char*) calloc(len+1,(sizeof(char)));
        strncpy(newStr,inStr,len);
        *(newStr+len) = '\0';

        retVal++;
    }

    // return retVal;

    return newStr;
}
