/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ======================================================================
 *  Copyright (c) 2004-2014  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#define _GNU_SOURCE
#include <sys/socket.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>

#include <tcl.h>

#include "config.h"

#define TRUE    1
#define FALSE   0

#ifndef SERVERSFILE
#define SERVERSFILE  "/opt/hubzero/rappture/render/lib/renderservers.tcl"
#endif

#ifndef LOGDIR
#define LOGDIR          "/tmp"
#endif

#define ERROR(...)      SysLog(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#define TRACE(...)      SysLog(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define WARN(...)       SysLog(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)       SysLog(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

static const char *syslogLevels[] = {
    "emergency",                        /* System is unusable */
    "alert",                            /* Action must be taken immediately */
    "critical",                         /* Critical conditions */
    "error",                            /* Error conditions */
    "warning",                          /* Warning conditions */
    "notice",                           /* Normal but significant condition */
    "info",                             /* Informational */
    "debug",                            /* Debug-level messages */
};

/* RenderServer --
 *
 *      Contains information to describe/execute a render server.
 */
typedef struct {
    const char *name;                   /* Name of server. */
    int port;                           /* Port to listen to. */
    int numCmdArgs;                     /* # of args in command.  */
    int numEnvArgs;                     /* # of args in environment.  */
    char *const *cmdArgs;               /* Command to execute for
                                         * server. */
    char *const *envArgs;               /* Environment strings to set. */
    int listenerFd;                     /* Descriptor of the listener
                                           socket. */
    int inputFd;                        /* Descriptor to dup input side of
                                         * server socket. */
    int outputFd;                       /* Descriptor to dup output side of
                                         * server socket. */
    int logStdout;                      /* Redirect server stdout to a
                                           file. */
    int logStderr;                      /* Redirect server stderr to a
                                           file. */
    int combineLogs;                    /* Combine server stdout/stderr in
                                         * same file. */
} RenderServer;

static Tcl_HashTable serverTable;       /* Table of render servers
                                         * representing services available to
                                         * clients.  A new instance is forked
                                         * and executed each time a new
                                         * request is accepted. */
static int debug = FALSE;
static pid_t serverPid;

static void
SysLog(int priority, const char *path, int lineNum, const char* fmt, ...)
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
    length = snprintf(message, MSG_LEN, "nanoscale (%d %d) %s: %s:%d ",
        serverPid, getpid(), syslogLevels[priority],  s, lineNum);
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

static RenderServer *
NewServer(Tcl_Interp *interp, const char *name)
{
    RenderServer *serverPtr;

    serverPtr = malloc(sizeof(RenderServer));
    memset(serverPtr, 0, sizeof(RenderServer));
    if (serverPtr == NULL) {
        Tcl_AppendResult(interp, "can't allocate structure for \"",
                name, "\": ", Tcl_PosixError(interp), (char *)NULL);
        return NULL;
    }
    serverPtr->name = strdup(name);
    serverPtr->outputFd = STDOUT_FILENO;
    serverPtr->inputFd = STDIN_FILENO;
    serverPtr->combineLogs = TRUE;
    serverPtr->logStdout = TRUE;
    serverPtr->logStderr = TRUE;
    return serverPtr;
}

static int
ParseSwitches(Tcl_Interp *interp, RenderServer *serverPtr, int *objcPtr,
              Tcl_Obj ***objvPtr)
{
    int i, objc;
    Tcl_Obj **objv;

    objc = *objcPtr;
    objv = *objvPtr;
    for (i = 3; i < objc; i += 2) {
        const char *string;
        char c;

        string = Tcl_GetString(objv[i]);
        if (string[0] != '-') {
            break;
        }
        c = string[1];
        if ((c == 'i') && (strcmp(string, "-input") == 0)) {
            int f;

            if (Tcl_GetIntFromObj(interp, objv[i+1], &f) != TCL_OK) {
                return TCL_ERROR;
            }
            serverPtr->inputFd = f;
        } else if ((c == 'o') && (strcmp(string, "-output") == 0)) {
            int f;

            if (Tcl_GetIntFromObj(interp, objv[i+1], &f) != TCL_OK) {
                return TCL_ERROR;
            }
            serverPtr->outputFd = f;
        } else if ((c == 'l') && (strcmp(string, "-logstdout") == 0)) {
            int state;

            if (Tcl_GetBooleanFromObj(interp, objv[i+1], &state) != TCL_OK) {
                return TCL_ERROR;
            }
            serverPtr->logStdout = state;
        } else if ((c == 'l') && (strcmp(string, "-logstderr") == 0)) {
            int state;

            if (Tcl_GetBooleanFromObj(interp, objv[i+1], &state) != TCL_OK) {
                return TCL_ERROR;
            }
            serverPtr->logStderr = state;
        } else if ((c == 'c') && (strcmp(string, "-combinelogs") == 0)) {
            int state;

            if (Tcl_GetBooleanFromObj(interp, objv[i+1], &state) != TCL_OK) {
                return TCL_ERROR;
            }
            serverPtr->combineLogs = state;
        } else {
            Tcl_AppendResult(interp, "unknown switch \"", string, "\"",
                             (char *)NULL);
            return TCL_ERROR;
        }
    }
    *objcPtr = objc - (i - 3);
    *objvPtr = objv + (i - 3);
    return TCL_OK;
}

/* 
 * RegisterServerCmd --
 *
 *      Registers a render server to be run when a client connects
 *      on the designated port. The form of the commands is
 *
 *          register_server <name> <port> <cmd> <environ>
 *
 *      where
 *
 *          name        Token for the render server.
 *          port        Port to listen to accept connections.
 *          cmd         Command to be run to start the render server.
 *          environ     Name-value pairs of representing environment
 *                      variables.
 *
 *      Note that "cmd" and "environ" are variable and backslash
 *      substituted.  A listener socket automatically is established on
 *      the given port to accept client requests.
 *
 *      Example:
 *
 *          register_server myServer 12345 ?switches? {
 *               /path/to/myserver arg arg
 *          } {
 *               LD_LIBRARY_PATH $libdir/myServer
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

    if (objc < 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " serverName port ?flags? cmd ?environ?",
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
    serverPtr = NewServer(interp, serverName);
    if (serverPtr == NULL) {
        return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, serverPtr);
    if (ParseSwitches(interp, serverPtr, &objc, (Tcl_Obj ***)&objv) != TCL_OK) {
        goto error;
    }
    objPtr = Tcl_SubstObj(interp, objv[3],
                          TCL_SUBST_VARIABLES | TCL_SUBST_BACKSLASHES);
    if (Tcl_SplitList(interp, Tcl_GetString(objPtr), &numCmdArgs,
        (const char ***)&cmdArgs) != TCL_OK) {
        goto error;
    }
    serverPtr->cmdArgs = cmdArgs;
    serverPtr->numCmdArgs = numCmdArgs;

    numEnvArgs = 0;
    envArgs = NULL;
    if (objc == 5) {
        objPtr = Tcl_SubstObj(interp, objv[4],
                TCL_SUBST_VARIABLES | TCL_SUBST_BACKSLASHES);
        if (Tcl_SplitList(interp, Tcl_GetString(objPtr), &numEnvArgs,
                (const char ***)&envArgs) != TCL_OK) {
            goto error;
        }
        if (numEnvArgs & 0x1) {
            Tcl_AppendResult(interp, "odd # elements in enviroment list",
                             (char *)NULL);
            goto error;
        }
    }
    serverPtr->envArgs = envArgs;
    serverPtr->numEnvArgs = numEnvArgs;

    /* Create a socket for listening. */
    f = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (f < 0) {
        Tcl_AppendResult(interp, "can't create listerner socket for \"",
                serverPtr->name, "\": ", Tcl_PosixError(interp), (char *)NULL);
        goto error;
    }

    /* If the render server instance should be killed, drop the socket address
     * reservation immediately, don't linger. */
    bool = TRUE;
    if (setsockopt(f, SOL_SOCKET, SO_REUSEADDR, &bool, sizeof(bool)) < 0) {
        Tcl_AppendResult(interp, "can't create set socket option for \"",
                serverPtr->name, "\": ", Tcl_PosixError(interp), (char *)NULL);
        goto error;
    }

    /* Bind this address to the socket. */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(f, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        Tcl_AppendResult(interp, "can't bind to socket for \"",
                serverPtr->name, "\": ", Tcl_PosixError(interp), (char *)NULL);
        goto error;
    }
    /* Listen on the specified port. */
    if (listen(f, 5) < 0) {
        Tcl_AppendResult(interp, "can't listen to socket for \"",
                serverPtr->name, "\": ", Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    serverPtr->listenerFd = f;

    return TCL_OK;
 error:
    free(serverPtr);
    return TCL_ERROR;
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
        ERROR("Can't add server: %s", Tcl_GetString(Tcl_GetObjResult(interp)));
        return FALSE;
    }
    Tcl_DeleteInterp(interp);
    return TRUE;
}

int
main(int argc, char **argv)
{
#ifdef SA_NOCLDWAIT
    struct sigaction action;
#endif
    fd_set serverFds;
    int maxFd;                          /* Highest file descriptor in use. */
    char display[200];                  /* String used to manage the X
                                         * DISPLAY variable for each render
                                         * server instance. */
    int maxCards;                       /* Maximum number of video cards, each
                                         * represented by a different X
                                         * screen.  */
    int screenNum;                      /* Current X screen number. */
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch iter;
    const char *fileName;               /* Path to servers file. */

    serverPid = getpid();
    screenNum = 0;
    maxCards = 1;
    fileName = SERVERSFILE;
    debug = FALSE;

    strcpy(display, ":0.0");
    Tcl_InitHashTable(&serverTable, TCL_ONE_WORD_KEYS);

    /* Process command line switches. */
    for (;;) {
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
        case 'x':                       /* Number of video cards */
            maxCards = strtoul(optarg, 0, 0);
            if ((maxCards < 1) || (maxCards > 10)) {
                fprintf(stderr, "Bad number of max videocards specified\n");
                return 1;
            }
            break;
        case 'd':                       /* Debug  */
            debug = TRUE;
            break;

        case 'f':                       /* Server file path. */
            fileName = strdup(optarg);
            break;

        default:
            fprintf(stderr,"Don't know what option '%c'.\n", c);
            Help(argv[0]);
            exit(1);
        }
    }

    if (!debug) {
        /* Detach this process from the controlling terminal process. The
         * current directory becomes /tmp and redirect stdin/stdout/stderr to
         * /dev/null. */
        if (daemon(0,0) < 0) {
            ERROR("Can't daemonize nanoscale: %s", strerror(errno));
            exit(1);
        }
    }
    serverPid = getpid();
    if (!ParseServersFile(fileName)) {
        exit(1);
    }

    if (serverTable.numEntries == 0) {
        ERROR("No servers designated.");
        exit(1);
    }
    signal(SIGPIPE, SIG_IGN);
#ifdef SA_NOCLDWAIT
    memset(&action, 0, sizeof(action));
    action.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &action, 0);
#else
    signal(SIGCHLD, SIG_IGN);
#endif

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

        /* Reset using the array of server file descriptors. */
        memcpy(&readFds, &serverFds, sizeof(serverFds));
        if (select(maxFd+1, &readFds, NULL, NULL, 0) <= 0) {
            ERROR("Select failed: %s", strerror(errno));
            break;                      /* Error on select. */
        }
        for (hPtr = Tcl_FirstHashEntry(&serverTable, &iter); hPtr != NULL;
             hPtr = Tcl_NextHashEntry(&iter)) {
            RenderServer *serverPtr;
            pid_t child;
            int sock;
            socklen_t length;
            struct sockaddr_in newaddr;

            serverPtr = Tcl_GetHashValue(hPtr);
            if (!FD_ISSET(serverPtr->listenerFd, &readFds)) {
                continue;
            }
            /* Rotate the display's screen number.  If we have multiple video
             * cards, try to spread the jobs out among them.  */
            screenNum++;
            if (screenNum >= maxCards) {
                screenNum = 0;
            }
            /* Accept the new connection. */
            length = sizeof(newaddr);
#ifdef HAVE_ACCEPT4
            sock = accept4(serverPtr->listenerFd, (struct sockaddr *)&newaddr,
                           &length, SOCK_CLOEXEC);
#else
            sock = accept(serverPtr->listenerFd, (struct sockaddr *)&newaddr,
                          &length);
#endif
            if (sock < 0) {
                ERROR("Can't accept server \"%s\": %s", serverPtr->name,
                      strerror(errno));
                exit(1);
            }
#ifndef HAVE_ACCEPT4
            int flags = fcntl(sock, F_GETFD);
            flags |= FD_CLOEXEC;
            if (fcntl(sock, F_SETFD, flags) < 0) {
                ERROR("Can't set FD_CLOEXEC on socket \"%s\": %s",
                        serverPtr->name, strerror(errno));
                exit(1);
            }
#endif
            INFO("Connecting \"%s\" to %s\n", serverPtr->name,
                 inet_ntoa(newaddr.sin_addr));

            /* Fork the new process.  Connect I/O to the new socket. */
            child = fork();
            if (child < 0) {
                ERROR("Can't fork \"%s\": %s", serverPtr->name,
                      strerror(errno));
                continue;
            }
            if (child == 0) {           /* Child process. */
                int i;

                if ((!debug) && (setsid() < 0)) {
                    ERROR("Can't setsid \"%s\": %s", serverPtr->name,
                          strerror(errno));
                    exit(1);
                }
                if ((!debug) && ((chdir("/")) < 0)) {
                    ERROR("Can't change to root directory for \"%s\": %s",
                          serverPtr->name, strerror(errno));
                    exit(1);
                }
                if (serverPtr->combineLogs) {
                    char path[BUFSIZ];
                    int newFd;

                    sprintf(path, "%s/%s-%d.log", LOGDIR,
                        serverPtr->name, getpid());
                    if (serverPtr->logStdout) {
                        newFd = open(path, O_WRONLY | O_CREAT| O_TRUNC, 0600);
                    } else {
                        newFd = open("/dev/null", O_WRONLY, 0600);
                    }
                    if (newFd < 0) {
                        ERROR("%s: can't open \"%s\": %s", serverPtr->name,
                              path, strerror(errno));
                        exit(1);
                    }
                    if (dup2(newFd, 1) < 0) {
                        ERROR("%s: can't dup stdout to \"%s\": %s",
                              serverPtr->name, path, strerror(errno));
                        exit(1);
                    }
                    if (dup2(newFd, 2) < 0) {
                        ERROR("%s: can't dup stderr to \"%s\": %s",
                              serverPtr->name, path, strerror(errno));
                        exit(1);
                    }
                } else {
                    char path[BUFSIZ];
                    int newFd;

                    sprintf(path, "%s/%s-%d.stdout", LOGDIR,
                        serverPtr->name, getpid());
                    if (serverPtr->logStdout) {
                        newFd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                    } else {
                        newFd = open("/dev/null", O_WRONLY, 0600);
                    }
                    if (newFd < 0) {
                        ERROR("%s: can't open \"%s\": %s", serverPtr->name,
                              path, strerror(errno));
                        exit(1);
                    }
                    if (dup2(newFd, 1) < 0) {
                        ERROR("%s: can't dup stdout to \"%s\": %s",
                              serverPtr->name, path, strerror(errno));
                        exit(1);
                    }
                    sprintf(path, "%s/%s-%d.stderr", LOGDIR,
                        serverPtr->name, getpid());
                    if (serverPtr->logStderr) {
                        newFd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
                    } else {
                        newFd = open("/dev/null", O_WRONLY, 0600);
                    }
                    if (newFd < 0) {
                        ERROR("%s: can't open \"%s\": %s", serverPtr->name,
                              path, strerror(errno));
                        exit(1);
                    }
                    if (dup2(newFd, 2) < 0) {
                        ERROR("%s: can't dup stderr to \"%s\": %s",
                              serverPtr->name, path, strerror(errno));
                        exit(1);
                    }
                }
                /* Dup the socket to descriptors, e.g. 3 and 4 */
                if (dup2(sock, serverPtr->inputFd) < 0)  { /* input */
                    ERROR("%s: can't dup socket to fd %d: %s",
                          serverPtr->name, serverPtr->inputFd,
                          strerror(errno));
                    exit(1);
                }
                /* If using a single descriptor for input and output, we don't
                 * need to call dup2 again
                 */
                if (serverPtr->outputFd != serverPtr->inputFd) {
                    if (dup2(sock, serverPtr->outputFd) < 0) { /* output */
                        ERROR("%s: can't dup socket to fd %d: %s",
                              serverPtr->name, serverPtr->outputFd,
                              strerror(errno));
                        exit(1);
                    }
                }
                /* Close all the other descriptors. */
                for (i = 3; i <= FD_SETSIZE; i++) {
                    if (i != serverPtr->inputFd &&
                        i != serverPtr->outputFd) {
                        close(i);
                    }
                }

                /* Set the screen number in the DISPLAY variable. */
                display[3] = screenNum + '0';
                setenv("DISPLAY", display, 1);
                /* Set the enviroment, if necessary. */
                for (i = 0; i < serverPtr->numEnvArgs; i += 2) {
                    setenv(serverPtr->envArgs[i], serverPtr->envArgs[i+1], 1);
                }
                { 
                    char *cmd;

                    cmd = Tcl_Merge(serverPtr->numCmdArgs,
                                    (const char *const *)serverPtr->cmdArgs);
                    INFO("Executing %s: client=%s, \"%s\" in=%d out=%d on DISPLAY=%s",
                         serverPtr->name, inet_ntoa(newaddr.sin_addr),
                         cmd, serverPtr->inputFd, serverPtr->outputFd, display);
                    /* Replace the current process with the render server. */
                    execvp(serverPtr->cmdArgs[0], serverPtr->cmdArgs);
                    ERROR("Can't execute \"%s\": %s", cmd, strerror(errno));
                    Tcl_Free(cmd);
                    exit(1);
                }
            } else {
                close(sock);
            }
        }
    }
    exit(1);
}
