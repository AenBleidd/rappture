/*
 * ----------------------------------------------------------------------
 *  RpVariable.cc
 *
 *   Rappture 2.0 Variable member functions
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <cstring>
#include <stdlib.h>
#include "RpVariable.h"
#include "RpHashHelper.h"

RpVariable::RpVariable()
{
    __init();
}

RpVariable::RpVariable(const RpVariable& o)
    :
        _path (o._path)
{
    if (o._h != NULL) {
        Rp_HashCopy(_h,o._h,charCpyFxn);
    }
}

RpVariable::~RpVariable()
{
    __close();
}

const char *
RpVariable::label(
    const char *val)
{
    return (const char *) property("label",val);
}

const char *
RpVariable::desc(
    const char *val)
{
    return (const char *) property("desc",val);
}

const char *
RpVariable::hints(
    const char *val)
{
    return (const char *) property("hints",val);
}

const char *
RpVariable::color(
    const char *val)
{
    return (const char *) property("color",val);
}

const char *
RpVariable::icon(
    const char *val)
{
    return (const char *) property("icon",val);
}

const char *
RpVariable::path(
    const char *val)
{
    return (const char *) property("path",val);
}

const void *
RpVariable::property(
    const char *key,
    const void *val)
{
    const void *r = NULL;
    if (_h == NULL) {
        // hash table does not exist, create it
        _h = (Rp_HashTable*) malloc(sizeof(Rp_HashTable));
        Rp_InitHashTable(_h,RP_STRING_KEYS);
        return NULL;
    }

    if (val == NULL) {
        // get the value
        r = Rp_HashSearchNode(_h,key);
    } else {
        //set the value
        Rp_HashRemoveNode(_h,key);
        Rp_HashAddNode(_h,key,val);
        r = val;
    }

    return r;
}

void
RpVariable::__init()
{
    _path = NULL;
    _h = NULL;
}

void
RpVariable::__close()
{
    if (_path != NULL) {
        free((void *)_path);
    }

    if (_h != NULL) {
        Rp_DeleteHashTable(_h);
    }

    __init();
}


// -------------------------------------------------------------------- //

