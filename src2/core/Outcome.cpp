/*
 * ======================================================================
 *  Rappture::Outcome
 *
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include "RpOutcome.h"

using namespace Rappture;

/**
 *  Create a negative outcome, with the given error message.
 */
Outcome::Outcome(const char *errmsg)
  : _status(0),
    _remarkPtr(NULL),
    _contextPtr(NULL)
{
    if (errmsg) {
        error(errmsg);
    }
}

/// Copy constructor
Outcome::Outcome(const Outcome& oc)
  : _status(oc._status),
    _remarkPtr(oc._remarkPtr),
    _contextPtr(oc._contextPtr)
{
}

/// Assignment operator
Outcome&
Outcome::operator=(const Outcome& oc)
{
    _status = oc._status;
    _remarkPtr = oc._remarkPtr;
    _contextPtr = oc._contextPtr;
    return *this;
}

/**
 *  Assign an error condition to the outcome.
 */
Outcome&
Outcome::error(const char* errmsg, int status)
{
    _status = status;
    _remarkPtr = Ptr<std::string>(new std::string(errmsg));
    _contextPtr.clear();
}

/**
 *  Clear the status of this outcome.
 */
Outcome&
Outcome::clear()
{
    _status = 0;
    _remarkPtr.clear();
    _contextPtr.clear();
}

/**
 *  Returns the status of this outcome as an integer.
 *  As in Unix systems, 0 = okay.
 */
Outcome::operator int() const
{
    return _status;
}

/**
 *  For !error tests.
 */
int
Outcome::operator!() const
{
    return (_status == 0);
}

/**
 *  Use this to concatenate many different outcomes.
 */
Outcome&
Outcome::operator&=(Outcome oc)
{
    _status &= oc._status;
    if (!oc._contextPtr.isNull()) {
        _remarkPtr = oc._remarkPtr;
        _contextPtr = oc._contextPtr;
    }
}

/**
 *  Query the error remark from an outcome.
 */
std::string
Outcome::remark() const
{
    if (!_remarkPtr.isNull()) {
        return _remarkPtr->data();
    }
    return "";
}

/**
 *  Add information to the context stack for an outcome.
 */
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

/**
 *  Query the context stack from an outcome.
 */
std::string
Outcome::context() const
{
    if (!_contextPtr.isNull()) {
        return _contextPtr->data();
    }
    return "";
}
