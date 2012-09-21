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
#include "ClientSocket.h"
#include "SocketException.h"

ClientSocket::ClientSocket(std::string host, int port)
{
    if (!Socket::create()) {
        throw SocketException("Could not create client socket.");
    }

    if (!Socket::connect(host, port)) {
        throw SocketException("Could not bind to port.");
    }
}

const ClientSocket& ClientSocket::operator <<(const std::string& s) const
{
    if (!Socket::send(s)) {
        throw SocketException("Could not write to socket.");
    }

    return *this;
}

const ClientSocket& ClientSocket::operator >>(std::string& s) const
{
    if (!Socket::recv(s)) {
        throw SocketException("Could not read from socket.");
    }

    return *this;
}

void ClientSocket::set_non_blocking(bool val)
{
    Socket::set_non_blocking(val);
}

bool ClientSocket::send(char* s, int size) const
{
    bool ret = Socket::send(s, size);
    if (!ret) {
        throw SocketException("Could not write to socket.");
    }

    return ret;
}

int ClientSocket::recv(char *s, int size) const
{
    bool ret = Socket::recv (s, size);
    if (!ret) {
        throw SocketException("Could not read from socket.");
    }

    return ret;
}
