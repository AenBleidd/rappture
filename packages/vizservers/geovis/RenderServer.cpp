/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/times.h>

#include <string>
#include <sstream>

#include <tcl.h>

#include "Trace.h"
#include "ReadBuffer.h"
#include "RenderServer.h"
#include "RendererCmd.h"
#include "Renderer.h"
#include "Stats.h"
#include "PPMWriter.h"
#include "TGAWriter.h"
#ifdef USE_THREADS
#include <pthread.h>
#include "ResponseQueue.h"
#ifdef USE_READ_THREAD
#include "CommandQueue.h"
#endif
#endif 

using namespace GeoVis;

Stats GeoVis::g_stats;

int GeoVis::g_statsFile = -1; ///< Stats output file descriptor.
int GeoVis::g_fdIn = STDIN_FILENO; ///< Input file descriptor
int GeoVis::g_fdOut = STDOUT_FILENO; ///< Output file descriptor
FILE *GeoVis::g_fOut = NULL; ///< Output file handle
FILE *GeoVis::g_fLog = NULL; ///< Trace logging file handle
Renderer *GeoVis::g_renderer = NULL; ///< Main render worker
ReadBuffer *GeoVis::g_inBufPtr = NULL; ///< Socket read buffer
#ifdef USE_THREADS
ResponseQueue *GeoVis::g_outQueue = NULL;
#ifdef USE_READER_THREAD
CommandQueue *GeoVis::g_inQueue = NULL;
#endif
#endif

static int
queueViewpoint()
{
    osgEarth::Viewpoint view = g_renderer->getViewpoint();

    std::ostringstream oss;
    size_t len = 0;
    oss << "nv>camera get "
        << view.x() << " "
        << view.y() << " "
        << view.z() << " "
        << view.getHeading() << " "
        << view.getPitch() << " "
        << view.getRange()
        << " {" << ((view.getSRS() == NULL) ? "" : view.getSRS()->getHorizInitString()) << "}"
        << " {" << ((view.getSRS() == NULL) ? "" : view.getSRS()->getVertInitString()) << "}"
        << "\n";
    std::string ostr = oss.str();
    len = ostr.size();
#ifdef USE_THREADS
    queueResponse(ostr.c_str(), len, Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(ostr.c_str(), len);

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif /*USE_THREADS*/
    return TCL_OK;
}

#ifdef USE_THREADS
static void
queueFrame(ResponseQueue *queue, const unsigned char *imgData)
{
#ifdef DEBUG_WRITE_FRAME_FILE

#ifdef RENDER_TARGA
    writeTGAFile("/tmp/frame.tga",
                 imgData,
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL);
#else
    writeTGAFile("/tmp/frame.tga",
                 imgData,
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL,
                 true);
#endif  /*RENDER_TARGA*/

#else
 
#ifdef RENDER_TARGA
    queueTGA(queue, "nv>image -type image -bytes",
             imgData,
             g_renderer->getWindowWidth(),
             g_renderer->getWindowHeight(),
             TARGA_BYTES_PER_PIXEL);
#else
    queuePPM(queue, "nv>image -type image -bytes",
             imgData,
             g_renderer->getWindowWidth(),
             g_renderer->getWindowHeight());
#endif  /*RENDER_TARGA*/
#endif  /*DEBUG_WRITE_FRAME_FILE*/
}

#else

static void
writeFrame(int fd, const unsigned char *imgData)
{
#ifdef DEBUG_WRITE_FRAME_FILE

#ifdef RENDER_TARGA
    writeTGAFile("/tmp/frame.tga",
                 imgData,
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL);
#else
    writeTGAFile("/tmp/frame.tga",
                 imgData,
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL,
                 true);
#endif  /*RENDER_TARGA*/

#else

#ifdef RENDER_TARGA
    writeTGA(fd, "nv>image -type image -bytes",
             imgData,
             g_renderer->getWindowWidth(),
             g_renderer->getWindowHeight(),
             TARGA_BYTES_PER_PIXEL);
#else
    writePPM(fd, "nv>image -type image -bytes",
             imgData,
             g_renderer->getWindowWidth(),
             g_renderer->getWindowHeight());
#endif  /*RENDER_TARGA*/
#endif  /*DEBUG_WRITE_FRAME_FILE*/
}
#endif /*USE_THREADS*/

static int
sendAck()
{
    std::ostringstream oss;
    oss << "nv>ok -token " << g_stats.nCommands << "\n";
    std::string ostr = oss.str();
    int nBytes = ostr.length();

    TRACE("Sending OK for commands through %lu", g_stats.nCommands);
#ifdef USE_THREADS
    queueResponse(ostr.c_str(), nBytes, Response::VOLATILE, Response::OK);
#else
    if (write(g_fdOut, ostr.c_str(), nBytes) < 0) {
        ERROR("write failed: %s", strerror(errno));
        return -1;
    }
#endif
    return 0;
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

    const char *user = getenv("USER");
    char *logName = NULL;
    int logNameLen = 0;

    if (user == NULL) {
        logNameLen = 19+1;
        logName = (char *)calloc(logNameLen, sizeof(char));
        strncpy(logName, "/tmp/geovis_log.txt", logNameLen);
    } else {
        logNameLen = 16+strlen(user)+4+1;
        logName = (char *)calloc(logNameLen, sizeof(char));
        strncpy(logName, "/tmp/geovis_log_", logNameLen);
        strncat(logName, user, strlen(user));
        strncat(logName, ".txt", 4);
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
exitService()
{
    TRACE("Enter");

    serverStats(g_stats, 0);

    // close log file
    if (g_fLog != NULL) {
        fclose(g_fLog);
	g_fLog = NULL;
    }
}

#ifdef USE_THREADS

#ifdef USE_READ_THREAD
static void *
readerThread(void *clientData)
{
    Tcl_Interp *interp = (Tcl_Interp *)clientData;

    TRACE("Starting reader thread");

    queueCommands(interp, NULL, g_inBufPtr);

    return NULL;
}
#endif

static void *
writerThread(void *clientData)
{
    ResponseQueue *queue = (ResponseQueue *)clientData;

    TRACE("Starting writer thread");
    for (;;) {
        Response *response = queue->dequeue();
        if (response == NULL)
            continue;
        if (fwrite(response->message(), sizeof(unsigned char), response->length(),
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

int
main(int argc, char *argv[])
{
    // Ignore SIGPIPE.  **Is this needed? **
    signal(SIGPIPE, SIG_IGN);

    //const char *resourcePath = NULL;
    while (1) {
        int c = getopt(argc, argv, "p:i:o:");
        if (c == -1) {
            break;
        }
        switch (c) {
        case 'p':
            //resourcePath = optarg;
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

    TRACE("Starting GeoVis Server");

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
    fprintf(g_fOut, "GeoVis %s (build %s)\n", GEOVIS_VERSION_STRING, SVN_VERSION);
    fflush(g_fOut);

    g_renderer = new Renderer();
    g_inBufPtr = new ReadBuffer(g_fdIn, 1<<12);

    Tcl_Interp *interp = Tcl_CreateInterp();

#ifdef USE_THREADS
    g_outQueue = new ResponseQueue();

    pthread_t writerThreadId;
    if (pthread_create(&writerThreadId, NULL, &writerThread, g_outQueue) < 0) {
        ERROR("Can't create writer thread: %s", strerror(errno));
    }
#endif
    initTcl(interp, NULL);

    osg::ref_ptr<osg::Image> imgData;

    // Start main server loop
    for (;;) {
        long timeout = g_renderer->getTimeout();
        int cmdStatus = processCommands(interp, NULL, g_inBufPtr, g_fdOut, timeout);
        if (cmdStatus < 0)
            break;

        if (g_renderer->render()) {
            TRACE("Rendered new frame");
            imgData = g_renderer->getRenderedFrame();
            if (imgData == NULL) {
                ERROR("Empty image");
            } else {
                TRACE("Image: %d x %d", imgData->s(), imgData->t());

                if (imgData->s() == g_renderer->getWindowWidth() &&
                    imgData->t() == g_renderer->getWindowHeight()) {
                    queueViewpoint();
#ifdef USE_THREADS
                    queueFrame(g_outQueue, imgData->data());
#else
                    writeFrame(g_fdOut, imgData->data());
#endif
                }
                g_stats.nFrames++;
                g_stats.nFrameBytes += imgData->s() * imgData->t() * 3;
            }
        } else {
            //TRACE("No render required");
            if (cmdStatus > 1) {
                sendAck();
            }
        }

#if 0
        double x, y, z;
        if (g_renderer->getMousePoint(&x, &y, &z)) {
            // send coords to client
            size_t length;
            char mesg[256];

            length = snprintf(mesg, sizeof(mesg),
                              "nv>map coords %g %g %g\n", x, y, z);

            queueResponse(mesg, length, Response::VOLATILE);
        }
#endif

        if (g_inBufPtr->status() == ReadBuffer::ENDFILE)
            break;
    }
#ifdef USE_THREADS
    // Writer thread is probably blocked on sem_wait, so cancel instead
    // of joining
    if (pthread_cancel(writerThreadId) < 0) {
        ERROR("Can't cancel writer thread: %s", strerror(errno));
    } else {
        TRACE("Cancelled writer thread");
    }

    TRACE("Deleting ResponseQueue");
    delete g_outQueue;
    g_outQueue = NULL;
#endif

    TRACE("Stopping Tcl interpreter");
    exitTcl(interp);
    interp = NULL;

    TRACE("Deleting ReadBuffer");
    delete g_inBufPtr;
    g_inBufPtr = NULL;

    TRACE("Deleting renderer");
    delete g_renderer;
    g_renderer = NULL;

    TRACE("Exiting GeoVis Server");

    closeLog();
    exitService();

    return 0;
}
