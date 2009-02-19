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
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <getopt.h>
#include <errno.h>

// The initial request load for a new renderer.
#define INITIAL_LOAD 100000000.0

// The factor that the load is divided by every second.
#define LOAD_DROP_OFF 2.0

// The broadcast interval (in seconds)
#define BROADCAST_INTERVAL 5

// The load of a remote machine must be less than this factor to
// justify redirection.
#define LOAD_REDIRECT_FACTOR 0.8

// Maxium number of services we support
#define MAX_SERVICES 100

float load = 0;             // The present load average for this system.
int memory_in_use = 0;      // Total memory in use by this system. 
int children = 0;           // Number of children running on this system.
int send_fd;                // The file descriptor we broadcast through.
struct sockaddr_in send_addr;  // The subnet address we broadcast to.
fd_set saved_rfds;          // Descriptors we're reading from.
fd_set pipe_rfds;           // Descriptors that are pipes to children.
fd_set service_rfds[MAX_SERVICES];
int maxCards = 1;
int dispNum = -1;
char displayVar[200];

struct host_info {
    struct in_addr in_addr;
    float          load;
    int            children;
};

struct child_info {
    int   memory;
    int   pipefd;
    float requests;
};

struct host_info host_array[100];
struct child_info child_array[100];

/*
 * min()/max() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define min(x,y) ({				\
	    typeof(x) _x = (x);			\
	    typeof(y) _y = (y);			\
	    (void) (&_x == &_y);		\
	    _x < _y ? _x : _y; })

#define max(x,y) ({				\
	    typeof(x) _x = (x);			\
	    typeof(y) _y = (y);			\
	    (void) (&_x == &_y);		\
	    _x > _y ? _x : _y; })

static int 
find_best_host(void)
{
    int h;
    float best = load;
    int index = -1;
    //printf("My load is %f\n", best);
    for(h=0; h<sizeof(host_array)/sizeof(host_array[0]); h++) {
	if (host_array[h].in_addr.s_addr == 0)
	    continue;
	//printf("%d I think load for %s is %f   ", h,
	//       inet_ntoa(host_array[h].in_addr), host_array[h].load);
	if (host_array[h].children <= children) {
	    if (host_array[h].load < best) {
		//if ((random() % 100) < 75) {
		index = h;
		best = host_array[h].load;
		//}
		//printf(" Better\n");
	    } else {
		//printf(" Worse\n");
	    }
	}
    }

    //printf("I choose %d\n", index);
    return index;
}

static void 
broadcast_load(void)
{
    int msg[2];
    msg[0] = htonl(load);
    msg[1] = htonl(children);
    int status;
    status = sendto(send_fd, &msg, sizeof(msg), 0, (struct sockaddr *)&send_addr,
		    sizeof(send_addr));
    if (status < 0) {
	perror("sendto");
    }
}

static void
close_child(int pipe_fd)
{
    int i;
    for(i=0; i<sizeof(child_array)/sizeof(child_array[0]); i++) {
	if (child_array[i].pipefd == pipe_fd) {
	    children--;
	    memory_in_use -= child_array[i].memory;
	    child_array[i].memory = 0;
	    FD_CLR(child_array[i].pipefd, &saved_rfds);
	    FD_CLR(child_array[i].pipefd, &pipe_rfds);
	    close(child_array[i].pipefd);
	    child_array[i].pipefd = 0;
	    break;
	}
    }
  
    printf("processes=%d, memory=%d, load=%f\n",
	   children, memory_in_use, load);

#ifdef notdef
    broadcast_load();
#endif
}

void note_request(int fd, float value)
{
    int c;
    for(c=0; c < sizeof(child_array)/sizeof(child_array[0]); c++) {
	if (child_array[c].pipefd == fd) {
	    child_array[c].requests += value;
#ifdef DEBUGGING
	    printf("Updating requests from pipefd %d to %f\n",
		   child_array[c].pipefd,
		   child_array[c].requests);
#endif
	    return;
	}
    }
}

static void 
update_load_average(void)
{
    static unsigned int counter;

    load = load / LOAD_DROP_OFF;
    float newload = 0.0;
    int c;
    for(c=0; c < sizeof(child_array)/sizeof(child_array[0]); c++) {
	if (child_array[c].pipefd != 0) {
	    newload += child_array[c].requests * child_array[c].memory;
	    child_array[c].requests = 0;
	}
    }
    load = load + newload;

    if ((counter++ % BROADCAST_INTERVAL) == 0) {
	broadcast_load();
    }
}

volatile int sigalarm_set;

static void 
sigalarm_handler(int signum)
{
    sigalarm_set = 1;
}

static void 
help(const char *argv0)
{
    fprintf(stderr,
	    "Syntax: %s [-d] -b <broadcast port> -l <listen port> -s <subnet> -c 'command'\n",
	    argv0);
    exit(1);
}

static void
clear_service_fd(int fd)
{
    int n;

    for(n = 0; n < MAX_SERVICES; n++) {
	if (FD_ISSET(fd, &service_rfds[n]))
	    FD_CLR(fd, &service_rfds[n]);
    }
}

int 
main(int argc, char *argv[])
{
    char server_command[MAX_SERVICES][1000];
    int nservices = 0;
    int command_argc[MAX_SERVICES];
    char **command_argv[MAX_SERVICES];
    int val;
    int listen_fd[MAX_SERVICES];
    int status;
    struct sockaddr_in listen_addr;
    struct sockaddr_in recv_addr;
    int listen_port[MAX_SERVICES];
    int recv_port = -1;
    int debug_flag = 0;
    int n;

    listen_port[0] = -1;
    server_command[0][0] = 0;

    strcpy(displayVar, "DISPLAY=:0.0");
    if (putenv(displayVar) < 0) {
	perror("putenv");
    }
    while(1) {
	int c;
	int option_index = 0;
	struct option long_options[] = {
	    // name, has_arg, flag, val
	    { 0,0,0,0 },
	};

	c = getopt_long(argc, argv, "+b:c:l:s:x:d", long_options, 
			&option_index);
	if (c == -1)
	    break;

	switch(c) {
	case 'x': /* Number of video cards */
	    maxCards = strtoul(optarg, 0, 0);
	    if ((maxCards < 1) || (maxCards > 10)) {
		fprintf(stderr, "bad number of max videocards specified\n");
		return 1;
	    }
	    break;
	case 'd':
	    debug_flag = 1;
	    break;
	case 'b':
	    recv_port = strtoul(optarg, 0, 0);
	    break;
	case 'c':
	    strncpy(server_command[nservices], optarg, sizeof(server_command[0]));

	    if (listen_port[nservices] == -1) {
		fprintf(stderr,"Must specify -l port before each -c command.\n");
		return 1;
	    }

	    nservices++;
	    listen_port[nservices] = -1;
	    break;
	case 'l':
	    listen_port[nservices] = strtoul(optarg,0,0);
	    break;
	case 's':
	    send_addr.sin_addr.s_addr = htonl(inet_network(optarg));
	    if (send_addr.sin_addr.s_addr == -1) {
		fprintf(stderr,"Invalid subnet broadcast address");
		return 1;
	    }
	    break;
	default:
	    fprintf(stderr,"Don't know what option '%c'.\n", c);
	    return 1;
	}
    }
    if (nservices == 0 ||
	recv_port == -1 ||
	server_command[0][0]=='\0') {
	int i;
	fprintf(stderr, "nservices=%d, recv_port=%d, server_command[0]=%s\n", nservices, recv_port, server_command[0]);
	for (i = 0; i < argc; i++) {
	    fprintf(stderr, "argv[%d]=(%s)\n", i, argv[i]);
	}
	help(argv[0]);
	return 1;
    }

    for(n = 0; n < nservices; n++) {
	// Parse the command arguments...

	command_argc[n]=0;
	command_argv[n] = malloc((command_argc[n]+2) * sizeof(char *));
	command_argv[n][command_argc[n]] = strtok(server_command[n], " \t");
	command_argc[n]++;
	while( (command_argv[n][command_argc[n]] = strtok(NULL, " \t"))) {
	    command_argv[n] = realloc(command_argv[n], (command_argc[n]+2) * sizeof(char *));
	    command_argc[n]++;
	}

	// Create a socket for listening.
	listen_fd[n] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_fd[n] < 0) {
	    perror("socket");
	    exit(1);
	}
  
	// If program is killed, drop the socket address reservation immediately.
	val = 1;
	status = setsockopt(listen_fd[n], SOL_SOCKET, SO_REUSEADDR, &val, 
			    sizeof(val));
	if (status < 0) {
	    perror("setsockopt");
	    // Not fatal.  Keep on going.
	}

	// Bind this address to the socket.
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(listen_port[n]);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	status = bind(listen_fd[n], (struct sockaddr *)&listen_addr,
		      sizeof(listen_addr));
	if (status < 0) {
	    perror("bind");
	    exit(1);
	}

	// Listen on the specified port.
	status = listen(listen_fd[n],5);
	if (status < 0) {
	    perror("listen");
	}
    }

    // Create a socket for broadcast.
    send_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (send_fd < 0) {
	perror("socket");
	exit(1);
    }

    // If program is killed, drop the socket address reservation immediately.
    val = 1;
    status = setsockopt(send_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (status < 0) {
	perror("setsockopt");
	// Not fatal.  Keep on going.
    }

    // We're going to broadcast through this socket.
    val = 1;
    status = setsockopt(send_fd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val));
    if (status < 0) {
	perror("setsockopt");
	// Not fatal.  Keep on going.
    }

    // Bind this address to the socket.
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(recv_port);
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    status = bind(send_fd, (struct sockaddr *)&recv_addr,
		  sizeof(recv_addr));
    if (status < 0) {
	perror("bind");
	exit(1);
    }

    // Set up the address that we broadcast to.
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(recv_port);

    // Set up a signal handler for the alarm interrupt.
    // It doesn't do anything other than interrupt select() below.
    if (signal(SIGALRM, sigalarm_handler) == SIG_ERR) {
	perror("signal SIGALRM");
    }

    struct itimerval itvalue = {
	{1, 0}, {1, 0}
    };
    status = setitimer(ITIMER_REAL, &itvalue, NULL);
    if (status != 0) {
	perror("setitimer");
    }

    // We're ready to go.  Before going into the main loop,
    // broadcast a load announcement to other machines.
#ifdef notdef
    broadcast_load();
#endif
    int maxfd = send_fd;
    FD_ZERO(&saved_rfds);
    FD_ZERO(&pipe_rfds);

    for(n = 0; n < nservices; n++) {
	FD_ZERO(&service_rfds[n]);
	FD_SET(listen_fd[n], &saved_rfds);
	if (listen_fd[n] > maxfd)
	    maxfd = listen_fd[n];
    }

    FD_SET(send_fd, &saved_rfds);

    if (!debug_flag) {
	if ( daemon(0,0) != 0 ) {
	    perror("daemon");
	    exit(1);
	}
    }

    while(1) {
	fd_set rfds = saved_rfds;
      
	status = select(maxfd+1, &rfds, NULL, NULL, 0);
	if (status <= 0) {
	    if (sigalarm_set) {
		update_load_average();
		sigalarm_set = 0;
	    }
	    continue;
	}
      
      
	int accepted = 0;
	for(n = 0; n < nservices; n++) {
	    if (FD_ISSET(listen_fd[n], &rfds)) {
		// Accept a new connection.
		struct sockaddr_in newaddr;
		unsigned int addrlen = sizeof(newaddr);
		int newfd = accept(listen_fd[n], (struct sockaddr *)&newaddr, &addrlen);
		if (newfd < 0) {
		    perror("accept");
		    continue;
		}
	      
		printf("New connection from %s\n", inet_ntoa(newaddr.sin_addr));
		FD_SET(newfd, &saved_rfds);
		maxfd = max(maxfd, newfd);
		FD_SET(newfd, &service_rfds[n]);
		accepted = 1; 
	    }
	}
      
	if (accepted)
	    continue;
      
	if (FD_ISSET(send_fd, &rfds)) {
	    int buffer[1000];
	    struct sockaddr_in peer_addr;
	    unsigned int len = sizeof(peer_addr);
	    status = recvfrom(send_fd, buffer, sizeof(buffer), 0,
			      (struct sockaddr*)&peer_addr, &len);
	    if (status < 0) {
		perror("recvfrom");
		continue;
	    }
	    if (status != 8) {
		fprintf(stderr,"Bogus message from %s\n",
			inet_ntoa(peer_addr.sin_addr));
		continue;
	    }
	    float peer_load = ntohl(buffer[0]);
	    int peer_procs = ntohl(buffer[1]);
	    //printf("Load for %s is %f (%d processes).\n",
	    //       inet_ntoa(peer_addr.sin_addr), peer_load, peer_procs);
	    int h;
	    int free_index=-1;
	    int found = 0;
	    for(h=0; h<sizeof(host_array)/sizeof(host_array[0]); h++) {
		if (host_array[h].in_addr.s_addr == peer_addr.sin_addr.s_addr) {
		    if (host_array[h].children != peer_procs) {
			printf("Load for %s is %f (%d processes).\n",
			       inet_ntoa(peer_addr.sin_addr), peer_load, peer_procs);
		    }
		    host_array[h].load = peer_load;
		    host_array[h].children = peer_procs;
		    found = 1;
		    break;
		}
		if (host_array[h].in_addr.s_addr == 0 && free_index == -1) {
		    free_index = h;
		}
	    }
	    if (!found) {
		host_array[free_index].in_addr.s_addr = peer_addr.sin_addr.s_addr;
		host_array[free_index].load = peer_load;
	    }
	    continue;
	}
      
	int i;
	for(i=0; i< maxfd +1; i++) {
	    if (FD_ISSET(i,&rfds)) {
	      
		// If this is a pipe, get the load.  Update.
		if (FD_ISSET(i,&pipe_rfds)) {
		    float value;
		    status = read(i, &value, sizeof(value));
		    if (status != 4) {
			//fprintf(stderr,"error reading pipe, child ended?\n");
			close_child(i);
			/*close(i);
			  FD_CLR(i, &saved_rfds);
			  FD_CLR(i, &pipe_rfds); */
		    } else {
			note_request(i,value);
		    }
		    continue;
		}
	      
		// This must be a descriptor that we're waiting to from for
		// the memory footprint.  Get it.
		int msg;
		status = read(i, &msg, 4);
		if (status != 4) {
		    fprintf(stderr,"Bad status on read (%d).", status);
		    FD_CLR(i, &saved_rfds);
		    clear_service_fd(i);
		    close(i);
		    continue;
		}
	      
		// find the new memory increment
		int newmemory = ntohl(msg);
	      
#ifdef notdef
		// Find the best host to create a new child on.
		int index = find_best_host();
	      
		// Only redirect if another host's load is significantly less
		// than our own...
		if (index != -1 &&
		    (host_array[index].load < (LOAD_REDIRECT_FACTOR * load))) {
		  
		    // If we're redirecting to another machine, give that
		    // machine an extra boost in our copy of the load
		    // statistics.  This will keep us from sending the very
		    // next job to it.  Eventually, the other machine will
		    // broadcast its real load and we can make an informed
		    // decision as to who redirect to again.
		    host_array[index].load += newmemory * INITIAL_LOAD;
		  
		    // Redirect to another machine.
		    printf("Redirecting to %s\n",
			   inet_ntoa(host_array[index].in_addr));
		    write(i, &host_array[index].in_addr.s_addr, 4);
		    FD_CLR(i, &saved_rfds);
		    clear_service_fd(i);
		    close(i);
		    continue;
		}
#endif 
		memory_in_use += newmemory;
		load += 2*INITIAL_LOAD;
#ifdef notdef
		broadcast_load();
#endif
		printf("Accepted new job with memory %d\n", newmemory);
		//printf("My load is now %f\n", load);
	      
		// accept the connection.
		msg = 0;
		write(i, &msg, 4);
	      
		int pair[2];
		status = pipe(pair);
		if (status != 0) {
		    perror("pipe");
		}
	      
		// Make the child side of the pipe non-blocking...
		status = fcntl(pair[1], F_SETFL, O_NONBLOCK);
		if (status < 0) {
		    perror("fcntl");
		}
		dispNum++;
		if (dispNum >= maxCards) {
		    dispNum = 0;
		}
		// Fork the new process.  Connect i/o to the new socket.
		status = fork();
		if (status < 0) {
		    perror("fork");
		} else if (status == 0) {
		  
		    for(n = 0; n < MAX_SERVICES; n++) {
			if (FD_ISSET(i, &service_rfds[n])) {
			    int status = 0;

			    if (!debug_flag) {
				// disassociate
				status = daemon(0,1);
			    }
			    
			    if (status == 0) { 
				int fd;

				dup2(i, 0);  // stdin
				dup2(i, 1);  // stdout
				dup2(i, 2);  // stderr
				dup2(pair[1],3);
				// read end of pipe moved, and left open to
				// prevent SIGPIPE
				dup2(pair[0],4); 
			      
				for(fd=5; fd<FD_SETSIZE; fd++)
				    close(fd);
			      
				if (maxCards > 1) {
				    displayVar[11] = dispNum + '0';
				}
				execvp(command_argv[n][0], command_argv[n]);
			    }
			    _exit(errno);
			}
		    }
		    _exit(EINVAL);
		  
		} else {
		    int c;
		    // reap initial child which will exit immediately
		    // (grandchild continues)
		    waitpid(status, NULL, 0); 
		    for(c=0; c<sizeof(child_array)/sizeof(child_array[0]); c++) {
			if (child_array[c].pipefd == 0) {
			    child_array[c].memory = newmemory;
			    child_array[c].pipefd = pair[0];
			    child_array[c].requests = INITIAL_LOAD;
			    status = close(pair[1]);
			    if (status != 0) {
				perror("close pair[1]");
			    }
			    FD_SET(pair[0], &saved_rfds);
			    FD_SET(pair[0], &pipe_rfds);
			    maxfd = max(pair[0], maxfd);
			    break;
			}
		    }
		  
		    children++;
		    broadcast_load();
		}
	      
	      
		FD_CLR(i, &saved_rfds);
		clear_service_fd(i);
		close(i);
		break;
	    }
	  
	} // for all connected_fds
      
    } // while(1)
  
}

