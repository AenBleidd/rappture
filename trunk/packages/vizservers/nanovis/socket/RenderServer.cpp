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
#include "RenderServer.h"

RenderServer::RenderServer()
{
}

RenderServer::RenderServer(int port_num)
{
    socket_num = port_num;

    //init socket server
    std::cout << "server up and  running....\n";

    try {
        // Create the socket
        server_socket = new ServerSocket(socket_num);
        //server_socket->set_non_blocking(true);
    } catch (SocketException& e) {
        std::cout << "Exception was caught:" << e.description() << "\nExiting.\n";
    }
}

bool  RenderServer::listen(std::string& data)
{
    if (!open_socket.is_connected()) {
        if (server_socket->accept(open_socket)) {
            fprintf(stderr, "server: connection accepted\n");
            try {
                //std::string data;
                open_socket >> data;
                std::cout << "server: msg received - " << data << "\n";

                open_socket << data;
                return false;
            } catch (SocketException&) { 
                return false;
            }
        }
    } else {
        try {
            //std::string data;
            open_socket >> data;
            //std::cout << "server: msg received - " << data << "\n";

            //open_socket << data;
            return true;
        } catch (SocketException&) {
            return false;
        }
    }
}

bool RenderServer::send(std::string& data)
{
    if (!open_socket.is_connected())
        return false;

    try {
        open_socket << data;
        return true;
    } catch (SocketException&) {
        return false;
    }
}

bool RenderServer::send(char* s, int size)
{
    if (!open_socket.is_connected())
        return false;

    try {
        open_socket.send(s, size);
        return true;
    } catch (SocketException&) {
        return false;
    }
}
