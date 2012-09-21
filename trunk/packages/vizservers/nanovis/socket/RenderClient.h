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
#ifndef RENDER_CLIENT_H
#define RENDER_CLIENT_H

#include <iostream>
#include <string>

#include "ClientSocket.h"
#include "SocketException.h"

class RenderClient
{
public:
    RenderClient();
    RenderClient(std::string& remote_host, int port_num);

    void send(std::string& msg);
    void receive(std::string& msg);

    bool receive(char *data, int size);
    bool send(char *data, int size);

    ClientSocket *client_socket;
    char *screen_buffer;
    int screen_size; //units of byte

private:
    int socket_num;
    std::string host;
};

#endif
