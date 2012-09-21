/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Library Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */


#ifndef _RAPPTURE_LIBRARY_C_H
#define _RAPPTURE_LIBRARY_C_H

#include "RpBufferCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

typedef struct RpLibrary RpLibrary;

// lib definition functions
RpLibrary* rpLibrary  (const char* path);
int rpFreeLibrary     (RpLibrary** lib);

// RpLibrary member functions
RpLibrary* rpElement  (RpLibrary* lib, const char* path);
RpLibrary* rpElementAsObject (RpLibrary* lib, const char* path);
int rpElementAsType   (RpLibrary* lib, const char* path, const char** retCStr);
int rpElementAsComp   (RpLibrary* lib, const char* path, const char** retCStr);
int rpElementAsId     (RpLibrary* lib, const char* path, const char** retCStr);

RpLibrary* rpChildren (RpLibrary* lib,
                            const char* path,
                            RpLibrary* childEle  );
RpLibrary* rpChildrenByType  (RpLibrary* lib,
                            const char* path,
                            RpLibrary* childEle,
                            const char* type     );

int rpGet             (RpLibrary* lib, const char* path, const char** retCStr);
int rpGetString       (RpLibrary* lib, const char* path, const char** retCStr);
int rpGetDouble       (RpLibrary* lib, const char* path, double* retDVal);

int rpGetData         (RpLibrary* lib, const char* path, RapptureBuffer* retBuf);

int rpPut             (RpLibrary* lib,
                            const char* path,
                            const char* value,
                            const char* id,
                            unsigned int append         );

int rpPutString       (RpLibrary* lib,
                            const char* path,
                            const char* value,
                            unsigned int append         );

int rpPutDouble       (RpLibrary* lib,
                            const char* path,
                            double value,
                            unsigned int append         );

int rpPutData         (RpLibrary* lib,
                            const char* path,
                            const char* bytes,
                            int nBytes,
                            unsigned int append         );

int rpPutFile         (RpLibrary* lib,
                            const char* path,
                            const char* fileName,
                            unsigned int binary,
                            unsigned int append         );

int rpXml             (RpLibrary* lib, const char** retCStr);

int rpNodeComp        (RpLibrary* node, const char** retCStr);
int rpNodeType        (RpLibrary* node, const char** retCStr);
int rpNodeId          (RpLibrary* node, const char** retCStr);

int rpResult          (RpLibrary* lib);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // ifndef _RAPPTURE_LIBRARY_C_H
