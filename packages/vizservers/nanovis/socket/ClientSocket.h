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
#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include "Socket.h"

class ClientSocket : private Socket
{
public:
    ClientSocket(std::string host, int port);
    virtual ~ClientSocket()
    {}

    const ClientSocket& operator <<(const std::string&) const;
    const ClientSocket& operator >>(std::string&) const;

    bool send(char* s, int size) const;
    int recv(char* s, int size) const;

    void set_non_blocking(bool val);
};

#endif
