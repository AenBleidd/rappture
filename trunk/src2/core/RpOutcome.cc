/*
 * ----------------------------------------------------------------------
 *  Rappture::Outcome
 *    This object represents the result of any Rappture call.  It acts
 *    like a boolean, so it can be tested for success/failure.  But
 *    it can also contain information about failure, including a trace
 *    back of messages indicating the cause.
 *
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpOutcome.h"

using namespace Rappture;

Outcome::Outcome(const char *errmsg)
  : _status(0),
    _remarkPtr(NULL),
    _contextPtr(NULL)
{
    if (errmsg) {
        error(errmsg);
    }
}

Outcome::Outcome(const Outcome& oc)
  : _status(oc._status),
    _remarkPtr(oc._remarkPtr),
    _contextPtr(oc._contextPtr)
{
}

Outcome&
Outcome::operator=(const Outcome& oc)
{
    _status = oc._status;
    _remarkPtr = oc._remarkPtr;
    _contextPtr = oc._contextPtr;
    return *this;
}

Outcome&
Outcome::error(const char* errmsg, int status)
{
    _status = status;
    _remarkPtr = Ptr<std::string>(new std::string(errmsg));
    _contextPtr.clear();
    return *this;
}

Outcome&
Outcome::clear()
{
    _status = 0;
    _remarkPtr.clear();
    _contextPtr.clear();
    return *this;
}

Outcome::operator int() const
{
    return _status;
}

int
Outcome::operator!() const
{
    return (_status == 0);
}

Outcome&
Outcome::operator&=(Outcome oc)
{
    _status &= oc._status;
    if (!oc._contextPtr.isNull()) {
        _remarkPtr = oc._remarkPtr;
        _contextPtr = oc._contextPtr;
    }
    return *this;
}

std::string
Outcome::remark() const
{
    if (!_remarkPtr.isNull()) {
        return _remarkPtr->data();
    }
    return "";
}

Outcome&
Outcome::addContext(const char *rem)
{
    if (_contextPtr.isNull()) {
        _contextPtr = new std::string();
    }
    _contextPtr->append(rem);
    _contextPtr->append("\n");
    return *this;
}

std::string
Outcome::context() const
{
    if (!_contextPtr.isNull()) {
        return _contextPtr->data();
    }
    return "";
}
