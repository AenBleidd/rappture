/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Implementation of the ServerSocket class
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
#include "ServerSocket.h"
#include "SocketException.h"

ServerSocket::ServerSocket ( int port )
{
  connected = false;
  if ( ! Socket::create() )
    {
      throw SocketException ( "Could not create server socket." );
    }

  if ( ! Socket::bind ( port ) )
    {
      throw SocketException ( "Could not bind to port." );
    }

  if ( ! Socket::listen() )
    {
      throw SocketException ( "Could not listen to socket." );
    }

}

ServerSocket::~ServerSocket()
{
  connected = false;
}


const ServerSocket& ServerSocket::operator << ( const std::string& s ) const
{
  if ( ! Socket::send ( s ) )
    {
      throw SocketException ( "Could not write to socket." );
    }

  return *this;

}


const ServerSocket& ServerSocket::operator >> ( std::string& s ) const
{
  if ( ! Socket::recv ( s ) )
    {
      throw SocketException ( "Could not read from socket." );
    }

  return *this;
}

bool ServerSocket::send (char* s, int size) const
{
  //printf("ServerSocket::send: 0\n");
  bool ret = Socket::send (s, size);
  //printf("ServerSocket::send: 1\n");
  if (!ret)
    {
  //printf("ServerSocket::send: 2\n");
      throw SocketException ( "Could not write to socket." );
    }

  return ret;
}

int ServerSocket::recv ( char* s, int size) const
{
  bool ret = Socket::recv (s, size);
  if (!ret)
    {
      throw SocketException ( "Could not read from socket." );
    }

  return ret;
}

bool ServerSocket::accept ( ServerSocket& sock )
{
/*
  if ( ! Socket::accept ( sock ) )
    {
      throw SocketException ( "Could not accept socket." );
    }
*/
   bool ret =  Socket::accept( sock );
   sock.set_connected(ret);
   return ret;
}

void ServerSocket::set_non_blocking(bool val){
	Socket::set_non_blocking(val);
}

bool ServerSocket::is_connected(){
	return connected;
}

bool ServerSocket::set_connected(bool val){
	return connected = val;
}
