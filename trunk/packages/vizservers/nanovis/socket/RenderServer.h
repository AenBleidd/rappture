/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * RenderServer.h: server with OpenRenderer engine
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#ifndef _RENDER_SERVER_H_
#define _RENDER_SERVER_H_

#include "ServerSocket.h"
#include "SocketException.h"
#include <string>


class RenderServer{
	
private:
	ServerSocket* server_socket;
	ServerSocket open_socket;
	int socket_num;

public:
	RenderServer();
	RenderServer(int port_num);
	bool listen(std::string& data);
	bool send(std::string& data);
	bool send(char* data, int size); //send raw bytes
};

#endif
