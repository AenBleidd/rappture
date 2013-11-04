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
#include "PPMWriter.h"
#include "TGAWriter.h"
#ifdef USE_THREADS
#include <pthread.h>
#include "ResponseQueue.h"
#ifdef USE_READ_THREAD
#include "CommandQueue.h"
#endif
#endif 
#include <md5.h>

using namespace GeoVis;

Stats GeoVis::g_stats;

int GeoVis::g_statsFile = -1; ///< Stats output file descriptor.
int GeoVis::g_fdIn = STDIN_FILENO; ///< Input file descriptor
int GeoVis::g_fdOut = STDOUT_FILENO; ///< Output file descriptor
FILE *GeoVis::g_fOut = stdout; ///< Output file handle
FILE *GeoVis::g_fLog = NULL; ///< Trace logging file handle
Renderer *GeoVis::g_renderer = NULL; ///< Main render worker
ReadBuffer *GeoVis::g_inBufPtr = NULL; ///< Socket read buffer
#ifdef USE_THREADS
ResponseQueue *GeoVis::g_outQueue = NULL;
#ifdef USE_READER_THREAD
CommandQueue *GeoVis::g_inQueue = NULL;
#endif
#endif

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

int
GeoVis::getStatsFile(Tcl_Interp *interp, Tcl_Obj *objPtr)
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
GeoVis::writeToStatsFile(int f, const char *s, size_t length)
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
     *   - Number of data sets loaded from client
     *   - Number of data bytes total loaded from client
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
    Tcl_DStringAppendElement(&ds, "geovis");
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
    /* num_data_sets */
    Tcl_DStringAppendElement(&ds, "num_data_sets");
    sprintf(buf, "%lu", (unsigned long int)g_stats.nDataSets);
    Tcl_DStringAppendElement(&ds, buf);
    /* data_set_bytes */
    Tcl_DStringAppendElement(&ds, "data_set_bytes");
    sprintf(buf, "%lu", (unsigned long int)g_stats.nDataBytes);
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

static void
initService()
{
    TRACE("Enter");

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
    close(STDERR_FILENO);
    dup2(fileno(g_fLog), STDERR_FILENO);
    // flush junk
    fflush(stderr);

    // clean up malloc'd memory
    if (logName != NULL) {
        free(logName);
    }

    TRACE("Leave");
}

static void
exitService()
{
    TRACE("Enter");

    serverStats(0);

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
    initService();
    initLog();

    memset(&g_stats, 0, sizeof(g_stats));
    gettimeofday(&g_stats.start, NULL);

    TRACE("Starting GeoVis Server");

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
            }
            if (imgData->s() == g_renderer->getWindowWidth() &&
                imgData->t() == g_renderer->getWindowHeight()) {
#ifdef USE_THREADS
                queueFrame(g_outQueue, imgData->data());
#else
                writeFrame(g_fdOut, imgData->data());
#endif
            }
            g_stats.nFrames++;
            g_stats.nFrameBytes += imgData->s() * imgData->t() * 3;
        } else {
            //TRACE("No render required");
            if (cmdStatus > 1) {
                sendAck();
            }
        }

        double x, y, z;
        if (g_renderer->getMousePoint(&x, &y, &z)) {
            // send coords to client
            size_t length;
            char mesg[256];

            length = snprintf(mesg, sizeof(mesg),
                              "nv>dataset coords %g %g %g\n", x, y, z);

            queueResponse(mesg, length, Response::VOLATILE);
        }

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
