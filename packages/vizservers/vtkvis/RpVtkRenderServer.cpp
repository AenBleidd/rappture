/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <signal.h>

#ifdef WANT_TRACE
#include <sys/time.h>
#endif

#include <string>
#include <sstream>

#include "Trace.h"
#include "ReadBuffer.h"
#include "RpVtkRenderServer.h"
#include "RpVtkRendererCmd.h"
#include "RpVtkRenderer.h"
#include "PPMWriter.h"
#include "TGAWriter.h"
#ifdef USE_THREADS
#include <pthread.h>
#include "ResponseQueue.h"
#endif 

using namespace Rappture::VtkVis;

int Rappture::VtkVis::g_fdIn = STDIN_FILENO; ///< Input file descriptor
int Rappture::VtkVis::g_fdOut = STDOUT_FILENO; ///< Output file descriptor
FILE *Rappture::VtkVis::g_fOut = stdout; ///< Output file handle
FILE *Rappture::VtkVis::g_fLog = NULL; ///< Trace logging file handle
Renderer *Rappture::VtkVis::g_renderer = NULL; ///< Main render worker
ReadBuffer *Rappture::VtkVis::g_inBufPtr = NULL; ///< Socket read buffer

#define ELAPSED_TIME(t1, t2) \
    ((t1).tv_sec == (t2).tv_sec ? (((t2).tv_usec - (t1).tv_usec)/1.0e+3f) : \
     (((t2).tv_sec - (t1).tv_sec))*1.0e+3f + (float)((t2).tv_usec - (t1).tv_usec)/1.0e+3f)

#ifdef USE_THREADS
static void
queueFrame(ResponseQueue *queue, vtkUnsignedCharArray *imgData)
{
#ifdef DEBUG
    if (g_renderer->getCameraMode() == Renderer::IMAGE) {
        double xywh[4];
        g_renderer->getScreenWorldCoords(xywh);
        TRACE("Image bbox: %g %g %g %g", 
              xywh[0], 
              (xywh[1] + xywh[3]), 
              (xywh[0] + xywh[2]), 
              xywh[1]);
    }

#ifdef RENDER_TARGA
    writeTGAFile("/tmp/frame.tga",
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL);
#else
    writeTGAFile("/tmp/frame.tga",
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL,
                 true);
#endif  /*RENDER_TARGA*/

#else
    if (g_renderer->getCameraMode() == Renderer::IMAGE) {
        double xywh[4];
        g_renderer->getCameraZoomRegion(xywh);
        std::ostringstream oss;
        oss.precision(12);
        // Send upper left and lower right corners as bbox
        oss << "nv>image -type image -bbox {"
            << std::scientific
            << xywh[0] << " "
            << xywh[1] << " "
            << xywh[2] << " "
            << xywh[3] << "} -bytes";

#ifdef RENDER_TARGA
        queueTGA(queue, oss.str().c_str(),
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL);
#else
        queuePPM(queue, oss.str().c_str(),
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight());
#endif  /*RENDER_TARGA*/
    } else {
#ifdef RENDER_TARGA
        queueTGA(queue, "nv>image -type image -bytes",
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL);
#else
        queuePPM(queue, "nv>image -type image -bytes",
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight());
#endif  /*RENDER_TARGA*/
    }
#endif  /*DEBUG*/
}

#else

static void
writeFrame(int fd, vtkUnsignedCharArray *imgData)
{
#ifdef DEBUG
    if (g_renderer->getCameraMode() == Renderer::IMAGE) {
        double xywh[4];
        g_renderer->getScreenWorldCoords(xywh);
        TRACE("Image bbox: %g %g %g %g", 
              xywh[0], 
              (xywh[1] + xywh[3]), 
              (xywh[0] + xywh[2]), 
              xywh[1]);
    }

#ifdef RENDER_TARGA
    writeTGAFile("/tmp/frame.tga",
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL);
#else
    writeTGAFile("/tmp/frame.tga",
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL,
                 true);
#endif  /*RENDER_TARGA*/

#else
    if (g_renderer->getCameraMode() == Renderer::IMAGE) {
        double xywh[4];
        g_renderer->getCameraZoomRegion(xywh);
        std::ostringstream oss;
        oss.precision(12);
        // Send upper left and lower right corners as bbox
        oss << "nv>image -type image -bbox {"
            << std::scientific
            << xywh[0] << " "
            << xywh[1] << " "
            << xywh[2] << " "
            << xywh[3] << "} -bytes";

#ifdef RENDER_TARGA
        writeTGA(fd, oss.str().c_str(),
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL);
#else
        writePPM(fd, oss.str().c_str(),
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight());
#endif  /*RENDER_TARGA*/
    } else {
#ifdef RENDER_TARGA
        writeTGA(fd, "nv>image -type image -bytes",
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight(),
                 TARGA_BYTES_PER_PIXEL);
#else
        writePPM(fd, "nv>image -type image -bytes",
                 imgData->GetPointer(0),
                 g_renderer->getWindowWidth(),
                 g_renderer->getWindowHeight());
#endif  /*RENDER_TARGA*/
    }
#endif  /*DEBUG*/
}
#endif /*USE_THREADS*/

static void
initService()
{
    const char *user = getenv("USER");
    char *logName = NULL;
    int logNameLen = 0;

    if (user == NULL) {
        logNameLen = 19+1;
        logName = (char *)calloc(logNameLen, sizeof(char));
        strncpy(logName, "/tmp/vtkvis_log.txt", logNameLen);
    }
    else {
        logNameLen = 16+strlen(user)+4+1;
        logName = (char *)calloc(logNameLen, sizeof(char));
        strncpy(logName, "/tmp/vtkvis_log_", logNameLen);
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
}

static void
exitService()
{
    // close log file
    if (g_fLog != NULL) {
        fclose(g_fLog);
	g_fLog = NULL;
    }
}

#ifdef USE_THREADS
static void *
readerThread(void *clientData)
{
    ResponseQueue *queue = (ResponseQueue *)clientData;
    Tcl_Interp *interp;

    TRACE("Starting reader thread");
    interp = (Tcl_Interp *)queue->clientData();
    vtkSmartPointer<vtkUnsignedCharArray> imgData = 
        vtkSmartPointer<vtkUnsignedCharArray>::New();
    for (;;) {
        if (processCommands(interp, g_inBufPtr, g_fdOut) < 0)
            break;

        if (g_renderer->render()) {
            TRACE("Rendering new frame");
            g_renderer->getRenderedFrame(imgData);
            queueFrame(queue, imgData);
        } else {
            TRACE("No render required");
        }

        if (g_inBufPtr->status() == ReadBuffer::ENDFILE)
            break;
    }    
    return NULL;
}

static void *
writerThread(void *clientData)
{
    ResponseQueue *queue = (ResponseQueue *)clientData;

    TRACE("Starting writer thread");
    for (;;) {
        Response *response;

        response = queue->dequeue(); 
        if (fwrite(response->message(), sizeof(char), response->length(),
                   g_fOut) != response->length()) {
            ERROR("short write while trying to write %ld bytes", 
                  response->length());
        }
        fflush(g_fOut);
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

    TRACE("Starting VTKVis Server");

    /* This synchronizes the client with the server, so that the client 
     * doesn't start writing commands before the server is ready. It could
     * also be used to supply information about the server (version, memory
     * size, etc). */
    fprintf(g_fOut, "VtkVis 1.0\n");
    fflush(g_fOut);

    g_renderer = new Renderer();
    g_inBufPtr = new ReadBuffer(g_fdIn, 1<<12);

    Tcl_Interp *interp = Tcl_CreateInterp();

#ifdef USE_THREADS
    ResponseQueue *queue = new ResponseQueue((void *)interp);
    initTcl(interp, (ClientData)queue);

    pthread_t readerThreadId, writerThreadId;
    if (pthread_create(&readerThreadId, NULL, &readerThread, queue) < 0) {
        ERROR("Can't create reader thread: %s", strerror(errno));
    }
    if (pthread_create(&writerThreadId, NULL, &writerThread, queue) < 0) {
        ERROR("Can't create writer thread: %s", strerror(errno));
    }
    if (pthread_join(readerThreadId, NULL) < 0) {
        ERROR("Can't join reader thread: %s", strerror(errno));
    } else {
        TRACE("Reader thread exited");
    }
    // Writer thread is probably blocked on sem_wait, so cancel instead
    // of joining
    if (pthread_cancel(writerThreadId) < 0) {
        ERROR("Can't cancel writer thread: %s", strerror(errno));
    } else {
        TRACE("Cancelled writer thread");
    }

    TRACE("Deleting ResponseQueue");
    delete queue;
    queue = NULL;
#else 
    initTcl(interp, (ClientData)NULL);

    vtkSmartPointer<vtkUnsignedCharArray> imgData = 
        vtkSmartPointer<vtkUnsignedCharArray>::New();
    for (;;) {
        if (processCommands(interp, g_inBufPtr, g_fdOut) < 0)
            break;

        if (g_renderer->render()) {
            TRACE("Rendering new frame");
            g_renderer->getRenderedFrame(imgData);
            writeFrame(g_fdOut, imgData);
        } else {
            TRACE("No render required");
        }

        if (g_inBufPtr->status() == ReadBuffer::ENDFILE)
            break;
    }
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

    TRACE("Exiting VTKVis Server");

    closeLog();
    exitService();

    return 0;
}
