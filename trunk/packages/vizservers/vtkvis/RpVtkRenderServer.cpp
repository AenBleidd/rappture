/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <unistd.h>
#include <signal.h>

#ifdef WANT_TRACE
#include <sys/time.h>
#endif

#include "Trace.h"
#include "RpVtkRenderServer.h"
#include "RpVtkRendererCmd.h"
#include "RpVtkRenderer.h"
#include "PPMWriter.h"
#include "TGAWriter.h"

using namespace Rappture::VtkVis;

int Rappture::VtkVis::g_fdIn = STDIN_FILENO; ///< Input file descriptor
int Rappture::VtkVis::g_fdOut = STDOUT_FILENO; ///< Output file descriptor
FILE *Rappture::VtkVis::g_fIn = stdin; ///< Input file handle
FILE *Rappture::VtkVis::g_fOut = stdout; ///< Output file handle
FILE *Rappture::VtkVis::g_fLog = NULL; ///< Trace logging file handle
Renderer *Rappture::VtkVis::g_renderer = NULL; ///< Main render worker

#define ELAPSED_TIME(t1, t2) \
    ((t1).tv_sec == (t2).tv_sec ? (((t2).tv_usec - (t1).tv_usec)/1.0e+3f) : \
     (((t2).tv_sec - (t1).tv_sec))*1.0e+3f + (float)((t2).tv_usec - (t1).tv_usec)/1.0e+3f)

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
#endif

#else
    if (g_renderer->getCameraMode() == Renderer::IMAGE) {
        double xywh[4];
        g_renderer->getScreenWorldCoords(xywh);
        std::ostringstream oss;
        oss.precision(12);
        // Send upper left and lower right corners as bbox
        oss << "nv>image -type image -bbox {"
            << std::scientific
            << xywh[0] << " "
            << (xywh[1] + xywh[3]) << " "
            << (xywh[0] + xywh[2]) << " "
            << xywh[1] << "} -bytes";

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
#endif
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
#endif
    }
#endif
}

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

int
main(int argc, char *argv[])
{
    // Ignore SIGPIPE.  **Is this needed? **
    signal(SIGPIPE, SIG_IGN);
    initService();
    InitLog();

    TRACE("Starting VTKVis Server");

    g_fIn = stdin;
    g_fOut = stdout;
    g_fdIn = fileno(stdin);
    g_fdOut = fileno(stdout);

    g_renderer = new Renderer();
    vtkSmartPointer<vtkUnsignedCharArray> imgData = 
        vtkSmartPointer<vtkUnsignedCharArray>::New();

    Tcl_Interp *interp = initTcl();

    int ret = 0;

    while (1) {
        ret = processCommands(interp, g_fIn, g_fOut);
        if (ret < 0)
            break;

        if (g_renderer->render()) {
            TRACE("Rendering new frame");
            g_renderer->getRenderedFrame(imgData);
            writeFrame(g_fdOut, imgData);
        } else {
            TRACE("No render required");
        }

        if (feof(g_fIn))
            break;
    }

    exitTcl(interp);
    interp = NULL;

    delete g_renderer;
    g_renderer = NULL;

    TRACE("Exiting VTKVis Server");

    CloseLog();
    exitService();

    return ret;
}

