/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Common Rappture Dictionary Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#include "RpBindingsDict.h"

RpDict DICT_TEMPLATE_L ObjDict_Lib;
RpDict DICT_TEMPLATE_U ObjDictUnits;

int
storeObject_Lib(RpLibrary* objectName) {

    int retVal = -1;
    int dictKey = ObjDict_Lib.size() + 1;
    int newEntry = 0;

    if (objectName) {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.
        ObjDict_Lib.set(dictKey,objectName, &newEntry); 
    }

    retVal = dictKey;
    return retVal;
}

RpLibrary*
getObject_Lib(int objKey) {

    RpLibrary* retVal = *(ObjDict_Lib.find(objKey).getValue());

    if (retVal == *(ObjDict_Lib.getNullEntry().getValue())) {
        retVal = NULL;
    }

   return retVal;

}

void
cleanLibDict () {
    // clean up the dictionary

    RpDictEntry DICT_TEMPLATE_L *hPtr;
    // RpDictIterator DICT_TEMPLATE iter(fortObjDict_Lib);
    // should rp_quit clean up the dict or some function in RpBindingsCommon.h
    RpDictIterator DICT_TEMPLATE_L iter(ObjDict_Lib);

    hPtr = iter.first();

    while (hPtr) {
        // Py_DECREF(*(hPtr->getValue()));
        hPtr->erase();
        hPtr = iter.next();
    }

    // if (fortObjDict_Lib.size()) {
    if (ObjDict_Lib.size()) {
        // probably want to change the warning sometime
        // printf("\nWARNING: internal dictionary is not empty..deleting\n");
    }

}

int
storeObject_UnitsStr(std::string objectName) {

    int retVal = -1;
    int dictNextKey = ObjDictUnits.size() + 1;
    int newEntry = 0;

    if (objectName != "") {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.
        ObjDictUnits.set(dictNextKey,objectName, &newEntry);
    }

    retVal = dictNextKey;
    return retVal;
}

RpUnits*
getObject_UnitsStr(int objKey) {

    std::string basisName = *(ObjDictUnits.find(objKey).getValue());

    if (basisName == *(ObjDictUnits.getNullEntry().getValue())) {
        // basisName = "";
        return NULL;
    }

   return RpUnits::find(basisName);

}

void
cleanUnitsDict () {
    // clean up the dictionary

    RpDictEntry DICT_TEMPLATE_U *hPtr;
    // RpDictIterator DICT_TEMPLATE iter(fortObjDict_Lib);
    // should rp_quit clean up the dict or some function in RpBindingsCommon.h
    RpDictIterator DICT_TEMPLATE_U iter(ObjDictUnits);

    hPtr = iter.first();

    while (hPtr) {
        // Py_DECREF(*(hPtr->getValue()));
        hPtr->erase();
        hPtr = iter.next();
    }

    // if (fortObjDict_Lib.size()) {
    if (ObjDictUnits.size()) {
        // probably want to change the warning sometime
        // printf("\nWARNING: internal dictionary is not empty..deleting\n");
    }

}
