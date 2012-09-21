/*
 * ======================================================================
 *  Rappture::Outcome
 *
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "RpOutcome.h"
using namespace Rappture;

/**
 *  Create a negative outcome, with the given error message.
 */
Outcome::Outcome(const char *errmsg) :
    _status(0)
{
    if (errmsg) {
        error(errmsg);
    }
}

/// Copy constructor
Outcome::Outcome(const Outcome& oc) :
    _status(oc._status),
    _remark(oc._remark),
    _context(oc._context)
{
}

/// Assignment operator
Outcome&
Outcome::operator=(const Outcome& oc)
{
    _status = oc._status;
    _remark = oc._remark;
    _context = oc._context;
    return *this;
}

/// Destructor
Outcome::~Outcome()
{}

/**
 *  Assign an error condition to the outcome.
 */
Outcome&
Outcome::error(const char* errmsg, int status)
{
    _status = status;
    _remark = errmsg;
    return *this;
}

Outcome&
Outcome::addError(const char* format, ...)
{
    char stackSpace[1024];
    va_list lst;
    size_t n;
    char *bufPtr;

    va_start(lst, format);
    bufPtr = stackSpace;
    n = vsnprintf(bufPtr, 1024, format, lst);
    if (n >= 1024) {
        bufPtr = (char *)malloc(n);
        vsnprintf(bufPtr, n, format, lst);
    }
    if (_remark == "") {
        _remark = _context;
        _remark.append(":\n");
        _remark.append(bufPtr);
    } else {
        _remark.append("\n");
        _remark.append(bufPtr);
    }
    /* fprintf(stderr, "Outcome: %s\n", _remark.c_str()); */
    _status = 1;                /* Set to error */
    if (bufPtr != stackSpace) {
        free(bufPtr);
    }
    return *this;
}

/**
 *  Clear the status of this outcome.
 */
Outcome&
Outcome::clear()
{
    _status = 0;
    _remark.clear();
    _context.clear();
    return *this;
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
    _remark = oc._remark;
    _context = oc._context;
    return *this;
}

/**
 *  Query the error remark from an outcome.
 */
const char *
Outcome::remark() const
{
    return _remark.c_str();
}

/**
 *  Add information to the context stack for an outcome.
 */
Outcome&
Outcome::addContext(const char *rem)
{
    // FIXME: There should be a stack of contexts
    _context = rem;
    _context.append("\n");
    return *this;
}

#ifdef soon
/**
 *  Push context information on to the context stack for an outcome.
 */
void
Outcome::pushContext(const char* format, ...)
{
    char stackSpace[1024];
    va_list lst;
    size_t n;
    char *bufPtr;

    va_start(lst, format);
    bufPtr = stackSpace;
    n = vsnprintf(bufPtr, 1024, format, lst);
    if (n >= 1024) {
        bufPtr = (char *)malloc(n);
        vsnprintf(bufPtr, n, format, lst);
    }
    _contexts.push_front(bufPtr);
}

/**
 *  Pop the last context from the stack.
 */
void
Outcome::popContext(void)
{
    _contexts.pop_front();
}

void
Outcome::printContext(void)
{
    list<const char *>::interator iter;

    for (iter = _contexts.begin(); iter != _contexts.end(); iter++) {
        fprintf(stderr, "Called from %s\n", *iter);
    }
}

#endif /*soon*/

/**
 *  Query the context stack from an outcome.
 */

const char *
Outcome::context() const
{
    return _context.c_str();
}
