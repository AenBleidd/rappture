/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Common Rappture Dictionary Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RpBINDINGS_DICT_H
#define _RpBINDINGS_DICT_H

#include "RpDict.h"
#include "RpUnits.h"

#ifdef __cplusplus
extern "C" {
#endif

// forward declarations so we can compile the object file
class RpLibrary;
/*
class RpUnits;
*/

#define DICT_TEMPLATE_L <long,RpLibrary*>
#define DICT_TEMPLATE_U <long,std::string>
#define DICT_TEMPLATE_V <size_t,void*>

// global declaration of library and units dictionaries
// for languages that cannot return rappture objects.
// (fortran, matlab, ...)

extern RpDict DICT_TEMPLATE_L ObjDict_Lib;
extern RpDict DICT_TEMPLATE_U ObjDictUnits;
extern RpDict DICT_TEMPLATE_V ObjDict_Void;


int storeObject_Lib(RpLibrary* objectName, int key=0);
int storeObject_UnitsStr(std::string objectName);
size_t storeObject_Void(void* objectName, size_t key=0);

RpLibrary* getObject_Lib(int objKey);
const RpUnits* getObject_UnitsStr(int objKey);
void* getObject_Void(size_t objKey);

void cleanLibDict();
void cleanUnitsDict();
void cleanVoidDict();

#ifdef __cplusplus
}
#endif

#endif // _RpBINDINGS_DICT_H
