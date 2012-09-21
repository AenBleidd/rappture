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
#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include "Socket.h"

class ServerSocket : private Socket
{
public:
    ServerSocket(int port);
    ServerSocket()
    {}
    virtual ~ServerSocket();

    const ServerSocket& operator <<(const std::string&) const;
    const ServerSocket& operator >>(std::string&) const;

    bool send(char* s, int size) const;
    int recv(char* s, int size) const;

    bool accept(ServerSocket&);

    void set_non_blocking(bool val);
    bool is_connected();
    bool set_connected(bool val);

private:
    bool connected;
};

#endif
