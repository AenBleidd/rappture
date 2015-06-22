/*
 * ----------------------------------------------------------------------
 *  INTERFACE: C Rappture Buffer Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef _RAPPTURE_BUFFER_C_H
#define _RAPPTURE_BUFFER_C_H

#include "RpOutcomeCInterface.h"

#ifdef __cplusplus
extern "C" {
#endif // ifdef __cplusplus

typedef struct {
    void* _buf;
    /*
    const char* (*bytes)();
    unsigned int (*size)();
    // void (*clear)();
    int (*append)(const char*, int);
    int (*read)(const char*, int);
    int (*seek)(int, int);
    int (*tell)();
    // void (*rewind);
    bool (*load)(Outcome &result, const char*);
    bool (*dump)(Outcome &result, const char*);
    bool (*encode)(Outcome &result, bool, bool);
    bool (*decode)(Outcome &result, bool, bool);
    */
} RapptureBuffer;

int RapptureBufferInit(RapptureBuffer* buf);
int RapptureBufferNew(RapptureBuffer* buf);
int RapptureBufferFree(RapptureBuffer* buf);
const char* RapptureBufferBytes(RapptureBuffer* buf);
unsigned int RapptureBufferSize(RapptureBuffer* buf);
int RapptureBufferAppend(RapptureBuffer* buf, const char* bytes, int size);
int RapptureBufferRead(RapptureBuffer* buf, const char* bytes, int size);
int RapptureBufferSeek(RapptureBuffer* buf, int offset, int whence);
int RapptureBufferTell(RapptureBuffer* buf);
RapptureOutcome RapptureBufferLoad(RapptureBuffer* buf, const char* filename);
RapptureOutcome RapptureBufferDump(RapptureBuffer* buf, const char* filename);
RapptureOutcome RapptureBufferEncode(RapptureBuffer* buf, int compress,
                                     int base64);
RapptureOutcome RapptureBufferDecode(RapptureBuffer* buf,
                                     int decompress, int base64);

#ifdef __cplusplus
}
#endif // ifdef __cplusplus

#endif // ifndef _RAPPTURE_BUFFER_C_H
