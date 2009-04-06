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

#include "RpVariable.h"
#include "RpHashHelper.h"

using namespace Rappture;

Variable::Variable()
{
    __init();
}

Variable::Variable (
    const char *path,
    const char *label,
    const char *desc,
    const char *hints,
    const char *color   )
{
    this->path(path);
    this->label(label);
    this->desc(desc);
    this->hints(hints);
    this->color(color);
}

Variable::Variable(const Variable& o)
{
    path(o.path());
    label(o.label());
    desc(o.desc());
    hints(o.hints());
    color(o.color());
    icon(o.icon());

    if (o._h != NULL) {
        Rp_HashCopy(_h,o._h,charCpyFxn);
    }
}

Variable::~Variable()
{
    __clear();
}

const void *
Variable::property(
    const char *key,
    const void *val)
{
    const void *r = NULL;
    if (_h == NULL) {
        // hash table does not exist, create it
        _h = (Rp_HashTable*) malloc(sizeof(Rp_HashTable));
        Rp_InitHashTable(_h,RP_STRING_KEYS);
    }

    if (val == NULL) {
        // get the value
        r = Rp_HashSearchNode(_h,key);
    } else {
        // set the value
        Rp_HashRemoveNode(_h,key);
        Rp_HashAddNode(_h,key,val);
        r = val;
    }

    return r;
}

const char *
Variable::propstr(
    const char *key,
    const char *val)
{
    const char *r = NULL;
    char *str = NULL;
    if (_h == NULL) {
        // hash table does not exist, create it
        _h = (Rp_HashTable*) malloc(sizeof(Rp_HashTable));
        Rp_InitHashTable(_h,RP_STRING_KEYS);
    }

    if (val == NULL) {
        // get the value
        r = (const char *) Rp_HashSearchNode(_h,key);
    } else {
        // set the value
        //FIXME: users responsibility to free value with propremove()
        size_t l = strlen(val);
        str = new char[l+1];
        strcpy(str,val);

        // FIXME: possible memory leak here
        // we dont free the removed item,
        // it is user's responsibility to use propremove() to free it
        // we don't know whether to use delete or delete[].
        Rp_HashRemoveNode(_h,key);
        Rp_HashAddNode(_h,key,str);
        r = val;
    }

    return r;
}

void *
Variable::propremove(
    const char *key)
{
    if ((key == NULL) || (_h == NULL)) {
        // key or hash table does not exist
        return NULL;
    }

    return Rp_HashRemoveNode(_h,key);
}

void
Variable::__init()
{
    _h = NULL;
    label("");
    desc("");
    hints("");
    color("");
    // icon(NULL);
    path("");
}

void
Variable::__clear()
{
    // who frees the Accessors?

    if (_h != NULL) {
        Rp_DeleteHashTable(_h);
    }

    __init();
}


// -------------------------------------------------------------------- //

