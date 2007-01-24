/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Common Rappture Dictionary Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpBindingsDict.h"

RpDict DICT_TEMPLATE_L ObjDict_Lib;
RpDict DICT_TEMPLATE_U ObjDictUnits;

/**********************************************************************/
// FUNCTION: storeObject_Lib()
/// Store an object into the library dictionary.
/**
 * This function stores the RpLibrary object pointed to by 'objectName'
 * into the library dictionary. This is helpful for writing bindings
 * for languages that can not accept pointers to provide back to the
 * function's caller.
 *
 * Returns the key of the object in the dictionary 
 * On Error, returns 0 (which also means nothing can be stored at 0)
 */

int
storeObject_Lib(RpLibrary* objectName, int key) {

    int retVal = 0;
    int dictKey = key;
    int newEntry = 0;

    if (objectName) {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.

        if (dictKey == 0) {
            dictKey = ObjDict_Lib.size() + 1;
        }
        ObjDict_Lib.set(dictKey,objectName, NULL,&newEntry);
        retVal = dictKey;
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: getObject_Lib()
/// Get an object from the library dictionary.
/**
 * This function retrieves the RpLibrary object associated with the key
 * 'objKey' from the library dictionary and returns its address to the
 * caller. This is helpful for writing bindings for languages that can
 * not accept pointers to provide back to the function's caller.
 *
 * Returns the address of the RpLibrary object in the dictionary
 */

RpLibrary*
getObject_Lib(int objKey) {


    RpDictEntry DICT_TEMPLATE_L* libEntry = &(ObjDict_Lib.getNullEntry());
    RpDictEntry DICT_TEMPLATE_L* nullEntry = &(ObjDict_Lib.getNullEntry());

    libEntry = &(ObjDict_Lib.find(objKey));


    if ( (!libEntry->isValid()) || (libEntry == nullEntry) ) {
        return NULL;
    }

   return *(libEntry->getValue());

}

/**********************************************************************/
// FUNCTION: cleanLibDict()
/// Clean the library dictionary, removing all entries in the dictionary
/**
 * This function removes all entries from the library dictionary.
 *
 * \sa {storeObject_Lib,getObject_Lib}
 */

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

/**********************************************************************/
// FUNCTION: storeObject_UnitsStr()
/// Store an object into the UnitsStr dictionary.
/**
 * This function stores the RpUnits names specified by 'objectName'
 * into the UnitsStr dictionary. This is helpful for writing bindings
 * for languages that can not accept pointers to provide back to the 
 * function's caller.
 *
 * Returns the key of the object in the dictionary
 */

int
storeObject_UnitsStr(std::string objectName) {

    int retVal = -1;
    int dictNextKey = ObjDictUnits.size() + 1;
    int newEntry = 0;

    if (objectName != "") {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.
        ObjDictUnits.set(dictNextKey,objectName, NULL, &newEntry);
        retVal = dictNextKey;
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: getObject_UnitsStr()
/// Get an object from the UnitsStr dictionary.
/**
 * This function retrieves the RpUnits name referenced to by 'objKey'
 * from the UnitsStr dictionary. This is helpful for writing bindings
 * for languages that can not accept pointers to provide back to the 
 * function's caller.
 *
 * Returns the key of the object in the dictionary
 */

const RpUnits*
getObject_UnitsStr(int objKey) {

    /*
    std::string basisName = *(ObjDictUnits.find(objKey).getValue());

    if (basisName == *(ObjDictUnits.getNullEntry().getValue())) {
        // basisName = "";
        return NULL;
    }

    return RpUnits::find(basisName);
    */

    RpDictEntry DICT_TEMPLATE_U* unitEntry = &(ObjDictUnits.getNullEntry());
    RpDictEntry DICT_TEMPLATE_U* nullEntry = &(ObjDictUnits.getNullEntry());

    unitEntry = &(ObjDictUnits.find(objKey));


    if ( (!unitEntry->isValid()) || (unitEntry == nullEntry) ) {
        return NULL;
    }

    return RpUnits::find(*(unitEntry->getValue()));

}

/**********************************************************************/
// FUNCTION: cleanUnitsDict()
/// Clean the UnitsStr dictionary, removing all entries.
/**
 * This function removes all entries from the UnitsStr dictionary
 *
 * \sa {storeObject_UnitsStr,getObject_UnitsStr}
 */

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
