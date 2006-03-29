#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "clientlib.h"

int connect_renderer(char *hostname, int connect_port, int memory)
{
  int val;
  int status;
  struct sockaddr_in connect_addr;
  int connect_fd = -1;

  struct hostent *hostent = gethostbyname(hostname);
  if (hostent == NULL) {
    fprintf(stderr, "Unable to resolve '%s'\n", hostname);
    return -1;
  }

  if (hostent->h_addrtype != AF_INET) {
    fprintf(stderr, "Don't know how to deal with address family for '%s'\n",
            hostname);
    return -1;
  }

  int addr = *(int *) hostent->h_addr_list[0];

 restart:

  // Bind this address to the socket.
  connect_addr.sin_family = AF_INET;
  connect_addr.sin_port = htons(connect_port);
  connect_addr.sin_addr.s_addr = addr; //htonl(inet_network(addr));

  int msg;
  do {
    // Create a socket for connecting...
    connect_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connect_fd < 0) {
      perror("socket");
      return -1;
    }

    // If program is killed, drop the socket address reservation immediately.
    val = 1;
    status = setsockopt(connect_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (status < 0) {
      perror("setsockopt");
      // Not fatal.  Keep on going.
    }

    // Connect.
    status = connect(connect_fd, (struct sockaddr *)&connect_addr,
                     sizeof(connect_addr));
    if (status < 0) {
      perror("connect");
      goto restart;
    }

    msg = htonl(memory);
    status = write(connect_fd, &msg, sizeof(msg));
    if (status <= 0) {
      perror("write");
      return -1;
    }

    status = read(connect_fd, &msg, sizeof(msg));
    if (status <= 0) {
      perror("read");
      return -1;
    }

    if (msg != 0) {
      connect_addr.sin_addr.s_addr = msg;
      fprintf(stderr,"Connection redirected to %s\n",
              inet_ntoa(connect_addr.sin_addr));
      status = close(connect_fd);
      if (status < 0) {
        perror("shutdown");
      }
    }
  } while(msg != 0);

  return connect_fd;
}

