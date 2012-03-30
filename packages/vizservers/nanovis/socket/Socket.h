/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef SOCKET_H 
#define SOCKET_H 

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <iostream>

const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
//const int MAXRECV =500;
//const int MAXRECV = 512*512*4*sizeof(float)+1;
const int MAXRECV = 512*512*4+1;

void error(int status, int err, const char *fmt, ... );
void set_address(char *hname, char *sname, struct sockaddr_in *sap, char *protocol);
void parse_GET_string(char *_str, char keys[256][256], char values[256][256], int *count);

ssize_t readn(int fd, void *vptr, size_t n);
ssize_t writen(int fd, void *vptr, size_t n);

class Socket
{
public:
    Socket();
    virtual ~Socket();

    // Server initialization
    bool create();
    bool bind(const int port);
    bool listen() const;
    bool accept(Socket&) const;

    // Client initialization
    bool connect(const std::string host, const int port);

    // Data Transimission
    bool send(const std::string) const;
    bool send(char* s, int size) const;
    int recv(std::string&) const;
    int recv(char* s, int size) const;

    void set_non_blocking(const bool);

    bool is_valid() const {
        return m_sock != -1;
    }

    int m_sock;

private:
    sockaddr_in m_addr;
};



#endif
