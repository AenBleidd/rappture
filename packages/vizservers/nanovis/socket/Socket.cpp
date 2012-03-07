/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Implementation of the Socket class.
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
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "Socket.h"

ssize_t writen(int fd, void *vptr, size_t n) ;
ssize_t readn(int fd, void *vptr, size_t n) ;

void error(int status, int err, const char *fmt, ... ){
  va_list ap;
  va_start(ap, fmt);
  //fprintf(stderr, "%s: ", program_name);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  if(err)
    fprintf(stderr, ":%s (%d)\n", strerror(err), err);
  if(status)
    exit(status);
}

void 
set_address(char *hname, char *sname, struct sockaddr_in *sap, char *protocol)
{
    struct servent *sp = NULL;
    struct hostent *hp;
    char *endptr;
    short port;
    
    bzero(sap, sizeof(*sap));
    sap->sin_family = AF_INET;
    
    if (hname!=NULL) {
	if (!inet_aton(hname, &sap->sin_addr)) {
	    hp = gethostbyname(hname);
	    if(hp==NULL) {
		error(1, 0, "unknown host: %s\n", hname);
	    }
	    sap->sin_addr = *(struct in_addr *) hp->h_addr;
	}
    } else {
	sap->sin_addr.s_addr = htonl(INADDR_ANY);
    }
    port = strtol(sname, &endptr, 0);
    if (*endptr=='\0') {
	sap->sin_port = htons(port);
    } else {
	sp = getservbyname(sname, protocol);
    }
    if (sp == NULL) {
	error(1, 0, "unknown service: %s\n", sname);
    }
}

void parse_GET_string(char *_str, char keys[256][256], char values[256][256], int *count)
{
  char str[1024];
  strcpy(str, _str);
  char * pch_amp;

  //printf ("Splitting string \"%s\" in tokens:\n",str);
  pch_amp = strtok (str,"&");
  //return;
  int k=0;
  while (pch_amp != NULL)
  {
    char keyval[128];
    strcpy(keyval, pch_amp);
   
    char *valptr = strchr(keyval, '=');
   
    if (valptr!=NULL)
    {
        *valptr='\0';
    valptr++;
    
        strcpy(keys[k], keyval);
        strcpy(values[k], valptr);
    }
    else
    {
        strcpy(keys[k], keyval);
        strcpy(values[k], "");
    }
    //printf("Key - :%s:, Value - :%s:\n", keys[k], values[k]);
    pch_amp = strtok (NULL, "&");
    k++;
  }
  *count = k;
}



Socket::Socket() :
  m_sock ( -1 )
{

  memset ( &m_addr,
	   0,
	   sizeof ( m_addr ) );

}

Socket::~Socket()
{
  if ( is_valid() )
    ::close ( m_sock );
}

bool Socket::create()
{
  m_sock = socket ( AF_INET,
		    SOCK_STREAM,
		    0 );

  if ( ! is_valid() )
    return false;


  // TIME_WAIT - argh
  int on = 1;
  if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
    return false;


  return true;

}



bool Socket::bind ( const int port )
{

  if ( ! is_valid() )
    {
      return false;
    }

  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons ( port );

  int bind_return = ::bind ( m_sock,
			     ( struct sockaddr * ) &m_addr,
			     sizeof ( m_addr ) );


  if ( bind_return == -1 )
    {
      return false;
    }

  return true;
}


bool Socket::listen() const
{
  if ( ! is_valid() )
    {
      return false;
    }

  //printf("socket:listen:1\n");
  //fflush(stdout);
  int listen_return = ::listen ( m_sock, MAXCONNECTIONS );
  //printf("socket:listen:2\n");
  //fflush(stdout);


  if ( listen_return == -1 )
    {
      return false;
    }

  return true;
}


bool Socket::accept ( Socket& new_socket ) const
{
  int addr_length = sizeof ( m_addr );
  new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );

  if ( new_socket.m_sock <= 0 )
    return false;
  else
    return true;
}


bool Socket::send ( const std::string s ) const
{
  int status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
  if ( status == -1 )
    {
      return false;
    }
  else
    {
      return true;
    }
}


bool Socket::send (char* s, int size) const
{
  //int status = ::send ( m_sock, s, size, MSG_NOSIGNAL );

  int status = writen(m_sock, s, size);
  if ( status == -1 )
    {
      return false;
    }
  else
    {
      return true;
    }
}


int Socket::recv ( std::string& s ) const
{
  char buf [ MAXRECV + 1 ];

  s = "";

  memset ( buf, 0, MAXRECV + 1 );
  int status = ::recv ( m_sock, buf, MAXRECV, 0 );

  if ( status == -1 )
    {
      std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
      return 0;
    }
  else if ( status == 0 )
    {
      return 0;
    }
  else
    {
      s = buf;
      return status;
    }
}


int Socket::recv (char* s, int size) const
{

  //int status = ::recv ( m_sock, s, size, 0 );
  int status = readn( m_sock, s, size);

  if ( status == -1 )
    {
      std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
      return 0;
    }
  else if ( status == 0 )
    {
      return 0;
    }
  else
    {
      return status;
    }
}


bool Socket::connect ( const std::string host, const int port )
{
  if ( ! is_valid() ) return false;

  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons ( port );

  int status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

  if ( errno == EAFNOSUPPORT ) return false;

  status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

  if ( status == 0 )
    return true;
  else
    return false;
}

void Socket::set_non_blocking ( const bool b )
{

  int opts;

  opts = fcntl ( m_sock,
		 F_GETFL );

  if ( opts < 0 )
    {
      return;
    }

  if ( b )
    opts = ( opts | O_NONBLOCK );
  else
    opts = ( opts & ~O_NONBLOCK );

  fcntl ( m_sock,
	  F_SETFL,opts );

}

ssize_t readn(int fd, void *vptr, size_t n) 
{
  size_t nleft;
  ssize_t nread;
  char* ptr;

  ptr = (char*) vptr;
  nleft = n;

  while(nleft > 0){
    if((nread=read(fd, ptr, nleft))<0){
      if(errno==EINTR)
	nread = 0;
      else
	return(-1);
    }
    else if(nread==0){
      fprintf(stderr, "socket::readn  nread==0\n");
      break;
    }

    nleft -= nread;
    ptr += nread;
  }

  return (n-nleft);	//return n>=0
}


ssize_t writen(int fd, void *vptr, size_t n) 
{
  size_t nleft;
  ssize_t nwritten;
  const char* ptr;

  ptr = (char*) vptr;
  nleft = n;

  while(nleft > 0){
    if((nwritten=write(fd, ptr, nleft))<=0){
      if(errno==EINTR)
	nwritten = 0;
      else
	return(-1);
    }
    nleft -= nwritten;
    ptr += nwritten;
  }

  return (n);
}
