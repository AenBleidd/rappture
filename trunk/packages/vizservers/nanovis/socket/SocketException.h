/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef SOCKETEXCEPTION_H
#define SOCKETEXCEPTION_H

#include <string>

class SocketException
{
public:
    SocketException (std::string s) :
        m_s (s)
    {}

    ~SocketException()
    {}

    std::string description()
    {
        return m_s;
    }

private:
    std::string m_s;
};

#endif
