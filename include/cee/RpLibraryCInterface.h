/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Library Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2005  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */


#ifdef __cplusplus
extern "C" {
#endif 

typedef struct RpLibrary RpLibrary;

// lib definition functions
//
RpLibrary*  rpLibrary             (const char* path);
void rpFreeLibrary                (RpLibrary* lib);

// RpLibrary member functions
RpLibrary*  rpElement             (RpLibrary* lib, const char* path);
RpLibrary*  rpElementAsObject     (RpLibrary* lib, const char* path);
const char* rpElementAsType       (RpLibrary* lib, const char* path);
const char* rpElementAsComp       (RpLibrary* lib, const char* path);
const char* rpElementAsId         (RpLibrary* lib, const char* path);

RpLibrary* rpChildren             (RpLibrary* lib,
                                 const char* path,
                                 RpLibrary* childEle);
RpLibrary* rpChildrenByType       (RpLibrary* lib,
                                 const char* path,
                                 RpLibrary* childEle,
                                 const char* type   );
/*
RpLibrary* rpChildrenAsObject     (RpLibrary* lib,
                                 const char* path,
                                 const char* type   );
const char* rpChildrenAsType      (RpLibrary* lib,
                                 const char* path,
                                 const char* type   );
const char* rpChildrenAsComp      (RpLibrary* lib,
                                 const char* path,
                                 const char* type   );
const char* rpChildrenAsId        (RpLibrary* lib,
                                 const char* path,
                                 const char* type   );
 */

RpLibrary*  rpGet                 (RpLibrary* lib, const char* path);
const char* rpGetString           (RpLibrary* lib, const char* path);
double      rpGetDouble           (RpLibrary* lib, const char* path);

void        rpPut                 (RpLibrary* lib,
                                 const char* path,
                                 const char* value,
                                 const char* id,
                                 int append         );
void        rpPutStringId         (RpLibrary* lib,
                                 const char* path,
                                 const char* value,
                                 const char* id,
                                 int append         );
void        rpPutString           (RpLibrary* lib,
                                 const char* path,
                                 const char* value,
                                 int append         );
void        rpPutDoubleId         (RpLibrary* lib,
                                 const char* path,
                                 double value,
                                 const char* id,
                                 int append         );
void        rpPutDouble           (RpLibrary* lib,
                                 const char* path,
                                 double value,
                                 int append         );

const char* rpXml                 (RpLibrary* lib);

const char* rpNodeComp            (RpLibrary* node);
const char* rpNodeType            (RpLibrary* node);
const char* rpNodeId              (RpLibrary* node);

void        rpResult              (RpLibrary* lib);

#ifdef __cplusplus
}
#endif
