/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Common Rappture Dictionary Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick S. Kearney, Purdue University
 *  Copyright (c) 2005
 *  Purdue Research Foundation, West Lafayette, IN
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

#define DICT_TEMPLATE_L <int,RpLibrary*>
#define DICT_TEMPLATE_U <int,std::string>

// global declaration of library and units dictionaries
// for languages that cannot return rappture objects.
// (fortran, matlab, ...)

extern RpDict DICT_TEMPLATE_L ObjDict_Lib;
extern RpDict DICT_TEMPLATE_U ObjDictUnits;


int storeObject_Lib(RpLibrary* objectName);
int storeObject_UnitsStr(std::string objectName);

RpLibrary* getObject_Lib(int objKey);
RpUnits* getObject_UnitsStr(int objKey);

#ifdef __cplusplus
}
#endif

#endif // _RpBINDINGS_DICT_H
