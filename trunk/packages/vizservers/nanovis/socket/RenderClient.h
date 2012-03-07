/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * RenderClient.h: server with OpenRenderer engine
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
#ifndef _RENDER_CLIENT_H_
#define _RENDER_CLIENT_H_

#include <iostream>
#include <string>

#include "ClientSocket.h"
#include "SocketException.h"

class RenderClient{
	
private:
	int socket_num;
	std::string host;

public:
	ClientSocket* client_socket;
	char* screen_buffer;
	int screen_size; //units of byte
	RenderClient();
	RenderClient(std::string& remote_host, int port_num);
	void send(std::string& msg);
	void receive(std::string& msg);

	bool receive(char* data, int size);
	bool send(char* data, int size);
};

#endif
