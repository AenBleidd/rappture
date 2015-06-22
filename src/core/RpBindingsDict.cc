/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Common Rappture Dictionary Source
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpBindingsDict.h"

RpDict DICT_TEMPLATE_L ObjDict_Lib;
RpDict DICT_TEMPLATE_U ObjDictUnits;
RpDict DICT_TEMPLATE_V ObjDict_Void;

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
    bool ci = false;

    if (objectName) {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.

        if (dictKey == 0) {
            dictKey = ObjDict_Lib.size() + 1;
        }
        long int dictKey_long = dictKey;
        ObjDict_Lib.set(dictKey_long,objectName,NULL,&newEntry,ci);
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

    long int objKey_long = objKey;
    libEntry = &(ObjDict_Lib.find(objKey_long));


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
    RpDictEntry DICT_TEMPLATE_L *hPtr;
    // should rp_quit clean up the dict or some function in RpBindingsCommon.h
    RpDictIterator DICT_TEMPLATE_L iter(ObjDict_Lib);

    hPtr = iter.first();

    while (hPtr) {
        hPtr->erase();
        hPtr = iter.next();
    }

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
    bool ci = false;

    if (objectName != "") {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.
        long int dictNextKey_long = dictNextKey;
        ObjDictUnits.set(dictNextKey_long,objectName,NULL,&newEntry,ci);
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

    RpDictEntry DICT_TEMPLATE_U* unitEntry = &(ObjDictUnits.getNullEntry());
    RpDictEntry DICT_TEMPLATE_U* nullEntry = &(ObjDictUnits.getNullEntry());

    long int objKey_long = objKey;
    unitEntry = &(ObjDictUnits.find(objKey_long));


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
    RpDictEntry DICT_TEMPLATE_U *hPtr;
    // should rp_quit clean up the dict or some function in RpBindingsCommon.h
    RpDictIterator DICT_TEMPLATE_U iter(ObjDictUnits);

    hPtr = iter.first();

    while (hPtr) {
        hPtr->erase();
        hPtr = iter.next();
    }

    if (ObjDictUnits.size()) {
        // probably want to change the warning sometime
        // printf("\nWARNING: internal dictionary is not empty..deleting\n");
    }
}

/**********************************************************************/
// FUNCTION: storeObject_Void()
/// Store an object into the general dictionary.
/**
 * This function stores a void* object pointed to by 'objectName'
 * into the general dictionary. This is helpful for writing bindings
 * for languages that can not accept pointers to provide back to the
 * function's caller.
 *
 * Returns the key of the object in the dictionary
 * On Error, returns 0 (which also means nothing can be stored at 0)
 */

size_t
storeObject_Void(void* objectName, size_t key) {

    size_t retVal = 0;
    size_t dictKey = key;
    int newEntry = 0;
    bool ci = false;

    if (objectName) {
        // dictionary returns a reference to the inserted value
        // no error checking to make sure it was successful in entering
        // the new entry.

        if (dictKey == 0) {
            dictKey = ObjDict_Void.size() + 1;
        }
        ObjDict_Void.set(dictKey,objectName,NULL,&newEntry,ci);
        retVal = dictKey;
    }

    return retVal;
}

/**********************************************************************/
// FUNCTION: getObject_Void()
/// Get an object from the general dictionary.
/**
 * This function retrieves the void* object associated with the key
 * 'objKey' from the general dictionary and returns its address to the
 * caller. This is helpful for writing bindings for languages that can
 * not accept pointers to provide back to the function's caller.
 *
 * Returns the address of the void* object in the dictionary
 */

void*
getObject_Void(size_t objKey) {

    RpDictEntry DICT_TEMPLATE_V* voidEntry = &(ObjDict_Void.getNullEntry());
    RpDictEntry DICT_TEMPLATE_V* nullEntry = &(ObjDict_Void.getNullEntry());

    voidEntry = &(ObjDict_Void.find(objKey));

    if ( (!voidEntry->isValid()) || (voidEntry == nullEntry) ) {
        return NULL;
    }

   return *(voidEntry->getValue());

}

/**********************************************************************/
// FUNCTION: cleanVoidDict()
/// Clean the library dictionary, removing all entries in the dictionary
/**
 * This function removes all entries from the library dictionary.
 *
 * \sa {storeObject_Lib,getObject_Lib}
 */

void
cleanVoidDict () {
    RpDictEntry DICT_TEMPLATE_V *hPtr;
    // should rp_quit clean up the dict or some function in RpBindingsCommon.h
    RpDictIterator DICT_TEMPLATE_V iter(ObjDict_Void);

    hPtr = iter.first();

    while (hPtr) {
        hPtr->erase();
        hPtr = iter.next();
    }

    if (ObjDict_Lib.size()) {
        // probably want to change the warning sometime
        // printf("\nWARNING: internal dictionary is not empty..deleting\n");
    }

}

