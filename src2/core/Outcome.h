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
#ifndef RAPPTURE_OUTCOME_H
#define RAPPTURE_OUTCOME_H

#include "rappture2.h"

namespace Rappture {

/**
 *  This object represents the result of any Rappture call.  It acts
 *  like a boolean, so it can be tested for success/failure.  But
 *  it can also contain information about failure, including a trace
 *  back of messages indicating the cause.
 */
class Outcome {
public:
    Outcome(const char* errmsg=NULL);
    Outcome(const Outcome& status);
    Outcome& operator=(const Outcome& status);

    virtual Outcome& error(const char* errmsg, int status=1);
    virtual Outcome& clear();

    virtual operator int() const;
    virtual int operator!() const;
    virtual Outcome& operator&=(Outcome status);  // pass-by-value to avoid temp

    virtual std::string remark() const;
    virtual Outcome& addContext(const char *rem);
    virtual std::string context() const;

private:
    /// overall pass/fail status
    int _status;

    /// error message
    Ptr<std::string>_remarkPtr;

    /// stack trace
    Ptr<std::string>_contextPtr;
};

} // namespace Rappture

#endif
