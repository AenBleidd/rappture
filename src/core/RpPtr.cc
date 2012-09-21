/*
 * ======================================================================
 *  Rappture::Ptr<type>
 *
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <stdlib.h>
#include <RpPtr.h>
#include <assert.h>

using namespace Rappture;

PtrCore::PtrCore(void* ptr)
  : _ptr(ptr),
    _refcount(1)
{
}

PtrCore::~PtrCore()
{
    assert(_refcount <= 0);
}

void*
PtrCore::pointer() const
{
    return _ptr;
}

void
PtrCore::attach()
{
    _refcount++;
}

void*
PtrCore::detach()
{
    if (--_refcount <= 0 && _ptr != NULL) {
        return _ptr;
    }
    return NULL;
}
