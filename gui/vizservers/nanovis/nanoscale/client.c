#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <getopt.h>

/*
 * min()/max() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define min(x,y) ({ \
        typeof(x) _x = (x);     \
        typeof(y) _y = (y);     \
        (void) (&_x == &_y);            \
        _x < _y ? _x : _y; })

#define max(x,y) ({ \
        typeof(x) _x = (x);     \
        typeof(y) _y = (y);     \
        (void) (&_x == &_y);            \
        _x > _y ? _x : _y; })

void help(const char *argv0)
{
  fprintf(stderr,
          "Syntax: %s addr:port load\n",
          argv0);
  exit(1);
}

int connect_fd = -1;

void activity(int signum)
{
  write(connect_fd, "hello\n\r", 7);
  write(1,".",1);
}


int main(int argc, char *argv[])
{
  int val;
  int status;
  struct sockaddr_in connect_addr;
  int connect_port = -1;

  if (argc != 3) {
    help(argv[0]);
  }

  char addr[100];
  if (sscanf(argv[1], "%100[^:\n]:%d\n", addr, &connect_port) != 2) {
    help(argv[0]);
  }
  int memory = strtoul(argv[2],0,0);

 restart:

  // Bind this address to the socket.
  connect_addr.sin_family = AF_INET;
  connect_addr.sin_port = htons(connect_port);
  connect_addr.sin_addr.s_addr = htonl(inet_network(addr));

  int msg;
  do {
    // Create a socket for connecting...
    connect_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connect_fd < 0) {
      perror("socket");
      exit(1);
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
      exit(1);
    }

    status = read(connect_fd, &msg, sizeof(msg));
    if (status <= 0) {
      perror("read");
      exit(1);
    }

    if (msg != 0) {
      connect_addr.sin_addr.s_addr = msg;
      printf("Connection redirected to %s\n",
             inet_ntoa(connect_addr.sin_addr));
      status = close(connect_fd);
      if (status < 0) {
        perror("shutdown");
      }
    }
  } while(msg != 0);

  printf("Connection accepted.\n");

#ifdef DEBUGGING
  if (signal(SIGALRM, activity) == SIG_ERR) {
    perror("signal");
  }

  struct itimerval itvalue = {
    {0,500000}, {0,500000}
  };
  status = setitimer(ITIMER_REAL, &itvalue, NULL);
#endif

  fd_set rfds;
  FD_ZERO(&rfds);
  while(1) {
    char buffer[100];
    FD_SET(0,&rfds);
    FD_SET(connect_fd,&rfds);
    status = select(connect_fd+1, &rfds, NULL, NULL, NULL);
    if (FD_ISSET(0,&rfds)) {
      status = read(0,buffer,sizeof(buffer));
      if (status <= 0) {
        fprintf(stderr,"Goodbye\n");
        exit(0);
      } else {
        status = write(connect_fd, buffer, status);
        if (status < 0) {
          perror("write");
        }
      }
    }

    else if (FD_ISSET(connect_fd,&rfds)) {
      status = read(connect_fd,buffer,sizeof(buffer));
      if (status <= 0) {
        fprintf(stderr,"Server closed connection\n");
        exit(0);
      } else {
        status = write(1, buffer, status);
        if (status < 0) {
          perror("write");
        }
      }
    }

    else {
      fprintf(stderr,"Strange case for select: %d\n", status);
      exit(1);
    }
  }

  return 0;
}

