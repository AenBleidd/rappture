/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */

#include <cassert>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <csignal>

#include <fcntl.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/uio.h> // for readv/writev
#include <unistd.h>

#include <sstream>

#include <tcl.h>

#include <GL/glew.h>
#include <GL/glut.h>

#include <util/FilePath.h>

#include "nanovis.h"
#include "nanovisServer.h"
#include "define.h"
#include "Command.h"
#include "PPMWriter.h"
#include "ReadBuffer.h"
#include "Shader.h"
#ifdef USE_THREADS
#include <pthread.h>
#include "ResponseQueue.h"
#endif
#include "Trace.h"

using namespace nv;
using namespace nv::util;

Stats nv::g_stats;
int nv::g_statsFile = -1; ///< Stats output file descriptor.

int nv::g_fdIn = STDIN_FILENO;     ///< Input file descriptor
int nv::g_fdOut = STDOUT_FILENO;   ///< Output file descriptor
FILE *nv::g_fOut = NULL;           ///< Output file handle
FILE *nv::g_fLog = NULL;           ///< Trace logging file handle
ReadBuffer *nv::g_inBufPtr = NULL; ///< Socket read buffer
#ifdef USE_THREADS
ResponseQueue *nv::g_queue = NULL;
#endif

#ifdef USE_THREADS

static void
queueFrame(ResponseQueue *queue, unsigned char *imgData)
{
    queuePPM(queue, "nv>image -type image -bytes",
             imgData,
             NanoVis::winWidth,
             NanoVis::winHeight);
}

#else

static void
writeFrame(int fd, unsigned char *imgData)
{
    writePPM(fd, "nv>image -type image -bytes",
             imgData,
             NanoVis::winWidth,
             NanoVis::winHeight);
}

#endif /*USE_THREADS*/

static int
sendAck()
{
    std::ostringstream oss;
    oss << "nv>ok -token " << g_stats.nCommands <<  "\n";
    int nBytes = oss.str().length();

    TRACE("Sending OK for commands through %lu", g_stats.nCommands);
#ifdef USE_THREADS
    queueResponse(oss.str().c_str(), nBytes, Response::VOLATILE, Response::OK);
#else
    if (write(g_fdOut, oss.str().c_str(), nBytes) < 0) {
        ERROR("write failed: %s", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

#ifdef KEEPSTATS

#ifndef STATSDIR
#define STATSDIR	"/var/tmp/visservers"
#endif  /*STATSDIR*/

int
nv::getStatsFile(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_DString ds;
    Tcl_Obj **objv;
    int objc;
    int i;
    char fileName[33];
    const char *path;
    md5_state_t state;
    md5_byte_t digest[16];
    char *string;
    int length;

    if ((objPtr == NULL) || (g_statsFile >= 0)) {
        return g_statsFile;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return -1;
    }
    Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewStringObj("pid", 3));
    Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewIntObj(getpid()));
    string = Tcl_GetStringFromObj(objPtr, &length);

    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)string, length);
    md5_finish(&state, digest);
    for (i = 0; i < 16; i++) {
        sprintf(fileName + i * 2, "%02x", digest[i]);
    }
    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, STATSDIR, -1);
    Tcl_DStringAppend(&ds, "/", 1);
    Tcl_DStringAppend(&ds, fileName, 32);
    path = Tcl_DStringValue(&ds);

    g_statsFile = open(path, O_EXCL | O_CREAT | O_WRONLY, 0600);
    Tcl_DStringFree(&ds);
    if (g_statsFile < 0) {
        ERROR("can't open \"%s\": %s", fileName, strerror(errno));
        return -1;
    }
    return g_statsFile;
}

int
nv::writeToStatsFile(int f, const char *s, size_t length)
{
    if (f >= 0) {
        ssize_t numWritten;

        numWritten = write(f, s, length);
        if (numWritten == (ssize_t)length) {
            close(dup(f));
        }
    }
    return 0;
}

static int
serverStats(int code) 
{
    double start, finish;
    char buf[BUFSIZ];
    Tcl_DString ds;
    int result;
    int f;

    {
        struct timeval tv;

        /* Get ending time.  */
        gettimeofday(&tv, NULL);
        finish = CVT2SECS(tv);
        tv = g_stats.start;
        start = CVT2SECS(tv);
    }
    /* 
     * Session information:
     *   - Name of render server
     *   - Process ID
     *   - Hostname where server is running
     *   - Start date of session
     *   - Start date of session in seconds
     *   - Number of frames returned
     *   - Number of bytes total returned (in frames)
     *   - Number of commands received
     *   - Total elapsed time of all commands
     *   - Total elapsed time of session
     *   - Exit code of vizserver
     *   - User time 
     *   - System time
     *   - User time of children 
     *   - System time of children
     */ 

    Tcl_DStringInit(&ds);
    
    Tcl_DStringAppendElement(&ds, "render_stop");
    /* renderer */
    Tcl_DStringAppendElement(&ds, "renderer");
    Tcl_DStringAppendElement(&ds, "nanovis");
    /* pid */
    Tcl_DStringAppendElement(&ds, "pid");
    sprintf(buf, "%d", getpid());
    Tcl_DStringAppendElement(&ds, buf);
    /* host */
    Tcl_DStringAppendElement(&ds, "host");
    gethostname(buf, BUFSIZ-1);
    buf[BUFSIZ-1] = '\0';
    Tcl_DStringAppendElement(&ds, buf);
    /* date */
    Tcl_DStringAppendElement(&ds, "date");
    strcpy(buf, ctime(&g_stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_DStringAppendElement(&ds, buf);
    /* date_secs */
    Tcl_DStringAppendElement(&ds, "date_secs");
    sprintf(buf, "%ld", g_stats.start.tv_sec);
    Tcl_DStringAppendElement(&ds, buf);
    /* num_frames */
    Tcl_DStringAppendElement(&ds, "num_frames");
    sprintf(buf, "%lu", (unsigned long int)g_stats.nFrames);
    Tcl_DStringAppendElement(&ds, buf);
    /* frame_bytes */
    Tcl_DStringAppendElement(&ds, "frame_bytes");
    sprintf(buf, "%lu", (unsigned long int)g_stats.nFrameBytes);
    Tcl_DStringAppendElement(&ds, buf);
    /* num_commands */
    Tcl_DStringAppendElement(&ds, "num_commands");
    sprintf(buf, "%lu", (unsigned long int)g_stats.nCommands);
    Tcl_DStringAppendElement(&ds, buf);
    /* cmd_time */
    Tcl_DStringAppendElement(&ds, "cmd_time");
    sprintf(buf, "%g", g_stats.cmdTime);
    Tcl_DStringAppendElement(&ds, buf);
    /* session_time */
    Tcl_DStringAppendElement(&ds, "session_time");
    sprintf(buf, "%g", finish - start);
    Tcl_DStringAppendElement(&ds, buf);
    /* status */
    Tcl_DStringAppendElement(&ds, "status");
    sprintf(buf, "%d", code);
    Tcl_DStringAppendElement(&ds, buf);
    {
        long clocksPerSec = sysconf(_SC_CLK_TCK);
        double clockRes = 1.0 / clocksPerSec;
        struct tms tms;

        memset(&tms, 0, sizeof(tms));
        times(&tms);
        /* utime */
        Tcl_DStringAppendElement(&ds, "utime");
        sprintf(buf, "%g", tms.tms_utime * clockRes);
        Tcl_DStringAppendElement(&ds, buf);
        /* stime */
        Tcl_DStringAppendElement(&ds, "stime");
        sprintf(buf, "%g", tms.tms_stime * clockRes);
        Tcl_DStringAppendElement(&ds, buf);
        /* cutime */
        Tcl_DStringAppendElement(&ds, "cutime");
        sprintf(buf, "%g", tms.tms_cutime * clockRes);
        Tcl_DStringAppendElement(&ds, buf);
        /* cstime */
        Tcl_DStringAppendElement(&ds, "cstime");
        sprintf(buf, "%g", tms.tms_cstime * clockRes);
        Tcl_DStringAppendElement(&ds, buf);
    }
    Tcl_DStringAppend(&ds, "\n", -1);
    f = getStatsFile(NULL, NULL);
    result = writeToStatsFile(f, Tcl_DStringValue(&ds), 
                              Tcl_DStringLength(&ds));
    close(f);
    Tcl_DStringFree(&ds);
    return result;
}

#endif

/**
 * \brief Send a command with data payload
 *
 * data pointer is freed on completion of the response
 */
void
nv::sendDataToClient(const char *command, char *data, size_t dlen)
{
#ifdef USE_THREADS
    char *buf = (char *)malloc(strlen(command) + dlen);
    memcpy(buf, command, strlen(command));
    memcpy(buf + strlen(command), data, dlen);
    queueResponse(buf, strlen(command) + dlen, Response::DYNAMIC);
#else
    size_t numRecords = 2;
    struct iovec *iov = new iovec[numRecords];

    // Write the nanovisviewer command, then the image header and data.
    // Command
    iov[0].iov_base = const_cast<char *>(command);
    iov[0].iov_len = strlen(command);
    // Data
    iov[1].iov_base = data;
    iov[1].iov_len = dlen;
    if (writev(nv::g_fdOut, iov, numRecords) < 0) {
        ERROR("write failed: %s", strerror(errno));
    }
    delete [] iov;
    free(data);
#endif
}

static void
initService()
{
    g_fOut = fdopen(g_fdOut, "w");
    // If running without socket, use stdout for debugging
    if (g_fOut == NULL && g_fdOut != STDOUT_FILENO) {
        g_fdOut = STDOUT_FILENO;
        g_fOut = fdopen(g_fdOut, "w");
    }

    const char* user = getenv("USER");
    char* logName = NULL;
    int logNameLen = 0;

    if (user == NULL) {
        logNameLen = 20+1;
        logName = (char *)calloc(logNameLen, sizeof(char));
        strncpy(logName, "/tmp/nanovis_log.txt", logNameLen);
    } else {
        logNameLen = 17+1+strlen(user);
        logName = (char *)calloc(logNameLen, sizeof(char));
        strncpy(logName, "/tmp/nanovis_log_", logNameLen);
        strncat(logName, user, strlen(user));
    }

    // open log and map stderr to log file
    g_fLog = fopen(logName, "w");
    dup2(fileno(g_fLog), STDERR_FILENO);
    // If we are writing to socket, map stdout to log
    if (g_fdOut != STDOUT_FILENO) {
        dup2(fileno(g_fLog), STDOUT_FILENO);
    }

    fflush(stdout);

    // clean up malloc'd memory
    if (logName != NULL) {
        free(logName);
    }
}

static void
exitService(int code)
{
    TRACE("Enter: %d", code);

    NanoVis::removeAllData();

    Shader::exit();

    //close log file
    if (g_fLog != NULL) {
        fclose(g_fLog);
        g_fLog = NULL;
    }

#ifdef KEEPSTATS
    serverStats(code);
#endif
    closelog();

    exit(code);
}

static void
idle()
{
    TRACE("Enter");

    glutSetWindow(NanoVis::renderWindow);

    if (processCommands(NanoVis::interp, g_inBufPtr, g_fdOut) < 0) {
        exitService(1);
    }

    NanoVis::update();
    NanoVis::bindOffscreenBuffer();
    NanoVis::render();
    bool renderedSomething = true;
    if (renderedSomething) {
        TRACE("Rendering new frame");
        NanoVis::readScreen();
#ifdef USE_THREADS
        queueFrame(g_queue, NanoVis::screenBuffer);
#else
        writeFrame(g_fdOut, NanoVis::screenBuffer);
#endif
        g_stats.nFrames++;
        g_stats.nFrameBytes += NanoVis::winWidth * NanoVis::winHeight * 3;
    } else {
        TRACE("No render required");
        sendAck();
    }

    if (g_inBufPtr->status() == ReadBuffer::ENDFILE) {
        exitService(0);
    }

    TRACE("Leave");
}

#ifdef USE_THREADS

static void *
writerThread(void *clientData)
{
    ResponseQueue *queue = (ResponseQueue *)clientData;

    TRACE("Starting writer thread");
    for (;;) {
        Response *response = queue->dequeue();
        if (response == NULL)
            continue;
        if (fwrite(response->message(), sizeof(char), response->length(),
                   g_fOut) != response->length()) {
            ERROR("short write while trying to write %ld bytes", 
                  response->length());
        }
        fflush(g_fOut);
        TRACE("Wrote response of type %d", response->type());
        delete response;
        if (feof(g_fOut))
            break;
    }    
    return NULL;
}

#endif  /*USE_THREADS*/

static
void shaderErrorCallback(void)
{
    if (!Shader::printErrorInfo()) {
        TRACE("Shader error, exiting...");
        exitService(1);
    }
}

static
void reshape(int width, int height)
{
    NanoVis::resizeOffscreenBuffer(width, height);
}

int 
main(int argc, char **argv)
{
    // Ignore SIGPIPE.  **Is this needed? **
    signal(SIGPIPE, SIG_IGN);

    const char *resourcePath = NULL;
    while (1) {
        static struct option long_options[] = {
            {"debug",   no_argument,       NULL, 'd'},
            {"path",    required_argument, NULL, 'p'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, "dp:i:o:", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'd':
            NanoVis::debugFlag = true;
            break;
        case 'p':
            resourcePath = optarg;
            break;
        case 'i': {
            int fd = atoi(optarg);
            if (fd >=0 && fd < 5) {
                g_fdIn = fd;
            }
        }
            break;
        case 'o': {
            int fd = atoi(optarg);
            if (fd >=0 && fd < 5) {
                g_fdOut = fd;
            }
        }
            break;
        case '?':
            break;
        default:
            return 1;
        }
    }

    initService();
    initLog();

    memset(&g_stats, 0, sizeof(g_stats));
    gettimeofday(&g_stats.start, NULL);

    TRACE("Starting NanoVis Server");
    if (NanoVis::debugFlag) {
        TRACE("Debugging on");
    }

    // Sanity check: log descriptor can't be used for client IO
    if (fileno(g_fLog) == g_fdIn) {
        ERROR("Invalid input file descriptor");
        return 1;
    }
    if (fileno(g_fLog) == g_fdOut) {
        ERROR("Invalid output file descriptor");
        return 1;
    }
    TRACE("File descriptors: in %d out %d log %d", g_fdIn, g_fdOut, fileno(g_fLog));

    /* This synchronizes the client with the server, so that the client 
     * doesn't start writing commands before the server is ready. It could
     * also be used to supply information about the server (version, memory
     * size, etc). */
    fprintf(g_fOut, "NanoVis %s (build %s)\n", NANOVIS_VERSION_STRING, SVN_VERSION);
    fflush(g_fOut);

    g_inBufPtr = new ReadBuffer(g_fdIn, 1<<12);

    Tcl_Interp *interp = Tcl_CreateInterp();
    ClientData clientData = NULL;
#ifdef USE_THREADS
    g_queue = new ResponseQueue();
    clientData = (ClientData)g_queue;
    initTcl(interp, clientData);

    pthread_t writerThreadId;
    if (pthread_create(&writerThreadId, NULL, &writerThread, g_queue) < 0) {
        ERROR("Can't create writer thread: %s", strerror(errno));
    }
#else
    initTcl(interp, clientData);
#endif
    NanoVis::interp = interp;

    /* Initialize GLUT here so it can parse and remove GLUT-specific 
     * command-line options before we parse the command-line below. */
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(NanoVis::winWidth, NanoVis::winHeight);
    glutInitWindowPosition(10, 10);
    NanoVis::renderWindow = glutCreateWindow("nanovis");

    glutIdleFunc(idle);
    glutDisplayFunc(NanoVis::render);
    glutReshapeFunc(reshape);

    char *newResourcePath = NULL;
    if (resourcePath == NULL) {
        char *p;

        // See if we can derive the path from the location of the program.
        // Assume program is in the form <path>/bin/nanovis.
        resourcePath = argv[0];
        p = strrchr((char *)resourcePath, '/');
        if (p != NULL) {
            *p = '\0';
            p = strrchr((char *)resourcePath, '/');
        }
        if (p == NULL) {
            ERROR("Resource path not specified, and could not determine a valid path");
            return 1;
        }
        *p = '\0';
        newResourcePath = new char[(strlen(resourcePath) + 15) * 2 + 1];
        sprintf(newResourcePath, "%s/lib/shaders:%s/lib/resources", resourcePath, resourcePath);
        resourcePath = newResourcePath;
        TRACE("No resource path specified, using: %s", resourcePath);
    } else {
        TRACE("Resource path: '%s'", resourcePath);
    }

    FilePath::getInstance()->setWorkingDirectory(argc, (const char**) argv);

    if (!NanoVis::init(resourcePath)) {
        exitService(1);
    }
    if (newResourcePath != NULL) {
        delete [] newResourcePath;
    }

    Shader::init();
    // Override callback with one that cleans up server on exit
    Shader::setErrorCallback(shaderErrorCallback);

    if (!NanoVis::initGL()) {
        exitService(1);
    }

    if (!NanoVis::resizeOffscreenBuffer(NanoVis::winWidth, NanoVis::winHeight)) {
        exitService(1);
    }

    glutMainLoop();

    exitService(0);
}
