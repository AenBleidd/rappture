
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>

#include <tcl.h>

#define TRUE	1
#define FALSE	0

#ifndef SERVERSFILE
#define SERVERSFILE  "/opt/hubzero/rappture/render/lib/renderservers.tcl"
#endif

#define ERROR(...)      LogMessage(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#ifdef WANT_TRACE
#define TRACE(...)      LogMessage(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define TRACE(...) 
#endif
#define WARN(...)       LogMessage(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)       LogMessage(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

static const char *syslogLevels[] = {
    "emergency",			/* System is unusable */
    "alert",				/* Action must be taken immediately */
    "critical",				/* Critical conditions */
    "error",				/* Error conditions */
    "warning",				/* Warning conditions */
    "notice",				/* Normal but significant condition */
    "info",				/* Informational */
    "debug",				/* Debug-level messages */
};

/* RenderServer --
 *
 *	Contains information to describe/execute a render server.
 */
typedef struct {
    const char *name;			/* Name of server. */
    int port;				/* Port to listen to. */
    int numCmdArgs;			/* # of args in command.  */
    int numEnvArgs;			/* # of args in environment.  */
    char *const *cmdArgs;		/* Command to execute for server. */
    char *const *envArgs;		/* Environment strings to set. */
    int listenerFd;			/* Descriptor of the listener socket. */
} RenderServer;

static Tcl_HashTable serverTable;	/* Table of render servers
					 * representing services available to
					 * clients.  A new instances is forked
					 * and executed each time a new
					 * request is accepted. */
static int debug = FALSE;

void
LogMessage(int priority, const char *path, int lineNum, const char* fmt, ...)
{
#define MSG_LEN (2047)
    char message[MSG_LEN+1];
    const char *s;
    int length;
    va_list lst;

    va_start(lst, fmt);
    s = strrchr(path, '/');
    if (s == NULL) {
        s = path;
    } else {
        s++;
    }
    length = snprintf(message, MSG_LEN, "nanoscale[%d] %s: %s:%d ", 
		      getpid(), syslogLevels[priority],  s, lineNum);
    length += vsnprintf(message + length, MSG_LEN - length, fmt, lst);
    message[MSG_LEN] = '\0';
    if (debug) {
	fprintf(stderr, "%s\n", message);
    } else {
	syslog(priority, message, length);
    }
}

static void 
Help(const char *program)
{
    fprintf(stderr,
	"Syntax: %s [-d] [-f serversFile] [-x numVideoCards]\n", program);
    exit(1);
}

/* 
 * RegisterServerCmd --
 *
 *	Registers a render server to be run when a client connects
 *	on the designated port. The form of the commands is
 *
 *          register_server <name> <port> <cmd> <environ>
 *
 *	where 
 *
 *	    name	Token for the render server.
 *	    port	Port to listen to accept connections.
 *	    cmd		Command to be run to start the render server.
 *          environ	Name-value pairs of representing environment 
 *			variables.
 *
 *	Note that "cmd" and "environ" are variable and backslash 
 *	substituted.  A listener socket automatically is established on 
 *	the given port to accept client requests.  
 *	
 *	Example:
 *
 *	    register_server myServer 12345 {
 *		 /path/to/myserver arg arg
 *	    } {
 *	         LD_LIBRARY_PATH $libdir/myServer
 *          }
 *
 */
static int
RegisterServerCmd(ClientData clientData, Tcl_Interp *interp, int objc,
		  Tcl_Obj *const *objv)
{
    Tcl_Obj *objPtr;
    const char *serverName;
    int bool, isNew;
    int f;
    int port;
    int numCmdArgs, numEnvArgs;
    char *const *cmdArgs;
    char *const *envArgs;
    struct sockaddr_in addr;
    RenderServer *serverPtr;
    Tcl_HashEntry *hPtr;

    if ((objc < 4) || (objc > 5)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " serverName port cmd ?environ?", 
		(char *)NULL);
	return TCL_ERROR;
    }
    serverName = Tcl_GetString(objv[1]);
    if (Tcl_GetIntFromObj(interp, objv[2], &port) != TCL_OK) {
	return TCL_ERROR;
    }
    hPtr = Tcl_CreateHashEntry(&serverTable, (char *)((long)port), &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "a server is already listening on port ", 
		Tcl_GetString(objv[2]), (char *)NULL);
	return TCL_ERROR;
    }
    objPtr = Tcl_SubstObj(interp, objv[3], 
			  TCL_SUBST_VARIABLES | TCL_SUBST_BACKSLASHES);
    if (Tcl_SplitList(interp, Tcl_GetString(objPtr), &numCmdArgs, 
	(const char ***)&cmdArgs) != TCL_OK) {
	return TCL_ERROR;
    }

    /* Create a socket for listening. */
    f = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (f < 0) {
	Tcl_AppendResult(interp, "can't create listerner socket for \"",
		serverName, "\": ", Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }
  
    /* If the render server instance should be killed, drop the socket address
     * reservation immediately, don't linger. */
    bool = TRUE;
    if (setsockopt(f, SOL_SOCKET, SO_REUSEADDR, &bool, sizeof(bool)) < 0) {
	Tcl_AppendResult(interp, "can't create set socket option for \"",
		serverName, "\": ", Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }

    /* Bind this address to the socket. */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(f, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	Tcl_AppendResult(interp, "can't bind to socket for \"",
		serverName, "\": ", Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }
    /* Listen on the specified port. */
    if (listen(f, 5) < 0) {
	Tcl_AppendResult(interp, "can't listen to socket for \"",
		serverName, "\": ", Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }
    numEnvArgs = 0;
    envArgs = NULL;
    if (objc == 5) {
	objPtr = Tcl_SubstObj(interp, objv[4], 
		TCL_SUBST_VARIABLES | TCL_SUBST_BACKSLASHES);
	if (Tcl_SplitList(interp, Tcl_GetString(objPtr), &numEnvArgs, 
		(const char ***)&envArgs) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (numEnvArgs & 0x1) {
	    Tcl_AppendResult(interp, "odd # elements in enviroment list", 
			     (char *)NULL);
	    return TCL_ERROR;
	}
    }
    serverPtr = malloc(sizeof(RenderServer));
    memset(serverPtr, 0, sizeof(RenderServer));
    if (serverPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate structure for \"",
		serverName, "\": ", Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }
    serverPtr->name = strdup(serverName);
    serverPtr->cmdArgs = cmdArgs;
    serverPtr->numCmdArgs = numCmdArgs;
    serverPtr->listenerFd = f;
    serverPtr->envArgs = envArgs;
    serverPtr->numEnvArgs = numEnvArgs;
    Tcl_SetHashValue(hPtr, serverPtr);
    return TCL_OK;
}

static int 
ParseServersFile(const char *fileName)
{
    Tcl_Interp *interp;

    interp = Tcl_CreateInterp();
    Tcl_MakeSafe(interp);
    Tcl_CreateObjCommand(interp, "register_server", RegisterServerCmd, NULL, 
			 NULL);
    if (Tcl_EvalFile(interp, fileName) != TCL_OK) {
	ERROR("can't add server: %s", Tcl_GetString(Tcl_GetObjResult(interp)));
	return FALSE;
    }
    Tcl_DeleteInterp(interp);
    return TRUE;
}

int 
main(int argc, char **argv)
{
    fd_set serverFds;
    int maxFd;				/* Highest file descriptor in use. */
    char display[200];			/* String used to manage the X 
					 * DISPLAY variable for each render 
					 * server instance. */
    int maxCards;			/* Maximum number of video cards, each
					 * represented by a different X
					 * screen.  */
    int dispNum;			/* Current X display number. */
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    const char *fileName;		/* Path to servers file. */

    dispNum = 0;
    maxCards = 1;
    fileName = SERVERSFILE;
    debug = FALSE;
    strcpy(display, "DISPLAY=:0.0");
    if (putenv(display) < 0) {
	ERROR("can't set DISPLAY variable: %s", strerror(errno));
	exit(1);
    }
    Tcl_InitHashTable(&serverTable, TCL_ONE_WORD_KEYS);

    /* Process command line switches. */
    while (1) {
	int c;
	int option_index = 0;
	struct option long_options[] = {
	    // name, has_arg, flag, val
	    { 0,0,0,0 },
	};

	c = getopt_long(argc, argv, "x:f:d", long_options, &option_index);
	if (c == -1) {
	    break;
	}

	switch(c) {
	case 'x':			/* Number of video cards */
	    maxCards = strtoul(optarg, 0, 0);
	    if ((maxCards < 1) || (maxCards > 10)) {
		fprintf(stderr, "bad number of max videocards specified\n");
		return 1;
	    }
	    break;
	case 'd':			/* Debug  */
	    debug = TRUE;
	    break;

	case 'f':			/* Server file path. */
	    fileName = strdup(optarg);
	    break;

	default:
	    fprintf(stderr,"Don't know what option '%c'.\n", c);
	    Help(argv[0]);
	    exit(1);
	}
    }

    if (!ParseServersFile(fileName)) {
	exit(1);
    }    

    if (serverTable.numEntries == 0) {
	ERROR("no servers designated.");
	exit(1);
    }

    if (!debug) {
	/* Detach this process from the controlling terminal process. The
	 * current directory becomes /tmp and redirect stdin/stdout/stderr to
	 * /dev/null. */
	if (daemon(0,0) < 0) {
	    ERROR("can't daemonize nanoscale: %s", strerror(errno));
	    exit(1);
	}
    }

    /* Build the array of servers listener file descriptors. */
    FD_ZERO(&serverFds);
    maxFd = -1;
    for (hPtr = Tcl_FirstHashEntry(&serverTable, &iter); hPtr != NULL;
	 hPtr = Tcl_NextHashEntry(&iter)) {
	RenderServer *serverPtr;
	
	serverPtr = Tcl_GetHashValue(hPtr);
	FD_SET(serverPtr->listenerFd, &serverFds);
	if (serverPtr->listenerFd > maxFd) {
	    maxFd = serverPtr->listenerFd;
	}
    }

    for (;;) {
	fd_set readFds;

	memcpy(&readFds, &serverFds, sizeof(serverFds));
	if (select(maxFd+1, &readFds, NULL, NULL, 0) <= 0) {
	    break;			/* Error on select. */
	}
	for (hPtr = Tcl_FirstHashEntry(&serverTable, &iter); hPtr != NULL;
	     hPtr = Tcl_NextHashEntry(&iter)) {
	    RenderServer *serverPtr;
	    pid_t child;

	    serverPtr = Tcl_GetHashValue(hPtr);
	    if (!FD_ISSET(serverPtr->listenerFd, &readFds)) {
		continue;		
	    }
	    /* Rotate the display's screen number.  If we have multiple video
	     * cards, try to spread the jobs out among them.  */
	    dispNum++;
	    if (dispNum >= maxCards) {
		dispNum = 0;
	    }
	    /* Fork the new process.  Connect I/O to the new socket. */
	    child = fork();
	    if (child < 0) {
		ERROR("can't fork \"%s\": %s", serverPtr->name, 
		      strerror(errno));
		continue;
	    } else if (child == 0) {
		int i;
		int f;
		socklen_t length;
		struct sockaddr_in newaddr;
		
		INFO("after fork child=%d server=\"%s\"\n", 
		     getpid(), serverPtr->name);
		/* Child process. */
		if (!debug) {
		    /* Detach this child process from the parent nanoscale
		     * process. The current directory becomes /tmp, but don't
		     * redirect stdin/stdout/stderr to /dev/null, we'll use
		     * that to connect to the socket. */
		    if (daemon(0, 1) < 0) {
			ERROR("can't daemonize \"%s\": %s", serverPtr->name, 
			      strerror(errno));
		    }
		}			    

		/* Try to accept the connection and start the server.  */

		/* Accept the new connection. */
		length = sizeof(newaddr);
		f = accept(serverPtr->listenerFd, (struct sockaddr *)&newaddr, 
			   &length);
		if (f < 0) {
		    ERROR("can't accept server \"%s\": %s", serverPtr->name, 
			  strerror(errno));
		    exit(1);
		}
		INFO("child=%d Connecting \"%s\" to %s\n", 
		     getpid(), serverPtr->name, inet_ntoa(newaddr.sin_addr));
		
		dup2(f, 0);		/* Stdin */
		dup2(f, 1);		/* Stdout */
		
		for(i = 3; i <= FD_SETSIZE; i++) {
		    close(i);	/* Close all the other descriptors. */
		}
		/* Set the enviroment, if necessary. */
		if (maxCards > 1) {
		    display[11] = dispNum + '0';
		}
		for (i = 0; i < serverPtr->numEnvArgs; i += 2) {
		    setenv(serverPtr->envArgs[i], serverPtr->envArgs[i+1], 0);
		}
		INFO("%s: client %s, %s on %s", serverPtr->name, 
			inet_ntoa(newaddr.sin_addr), serverPtr->cmdArgs[0], 
			display);
		/* Finally replace the current process with the render server */
		execvp(serverPtr->cmdArgs[0], serverPtr->cmdArgs);
		ERROR("can't execute \"%s\": %s", serverPtr->cmdArgs[0], 
		      strerror(errno));
		exit(1);
	    } else {
		if (!debug) {
		    /* Reap initial child which will exit immediately 
		     * (grandchild continues) */
		    waitpid(child, NULL, 0); 
		}
	    }
	} 
    }
    ERROR("select failed: %s", strerror(errno));
    exit(1);
}
