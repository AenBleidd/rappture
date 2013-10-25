/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cfloat>
#include <cerrno>
#include <string>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <tcl.h>

#include "Trace.h"
#include "CmdProc.h"
#include "ReadBuffer.h"
#include "Types.h"
#include "RendererCmd.h"
#include "RenderServer.h"
#include "Renderer.h"
#include "PPMWriter.h"
#include "TGAWriter.h"
#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

using namespace GeoVis;

static int lastCmdStatus;

ssize_t
GeoVis::queueResponse(const void *bytes, size_t len, 
                      Response::AllocationType allocType,
                      Response::ResponseType type)
{
#ifdef USE_THREADS
    Response *response = new Response(type);
    response->setMessage((unsigned char *)bytes, len, allocType);
    g_queue->enqueue(response);
    return (ssize_t)len;
#else
    return SocketWrite(bytes, len);
#endif
}

static ssize_t
SocketWrite(const void *bytes, size_t len)
{
    size_t ofs = 0;
    ssize_t bytesWritten;
    while ((bytesWritten = write(g_fdOut, (const char *)bytes + ofs, len - ofs)) > 0) {
        ofs += bytesWritten;
        if (ofs == len)
            break;
    }
    if (bytesWritten < 0) {
        ERROR("write: %s", strerror(errno));
    }
    return bytesWritten;
}

static bool
SocketRead(char *bytes, size_t len)
{
    ReadBuffer::BufferStatus status;
    status = g_inBufPtr->followingData((unsigned char *)bytes, len);
    TRACE("followingData status: %d", status);
    return (status == ReadBuffer::OK);
}

static int
ExecuteCommand(Tcl_Interp *interp, Tcl_DString *dsPtr) 
{
    int result;
#ifdef WANT_TRACE
    char *str = Tcl_DStringValue(dsPtr);
    std::string cmd(str);
    cmd.erase(cmd.find_last_not_of(" \n\r\t")+1);
    TRACE("command %lu: '%s'", g_stats.nCommands+1, cmd.c_str());
#endif
    lastCmdStatus = TCL_OK;
    result = Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), 
                        Tcl_DStringLength(dsPtr), 
                        TCL_EVAL_DIRECT | TCL_EVAL_GLOBAL);
    Tcl_DStringSetLength(dsPtr, 0);
    if (lastCmdStatus == TCL_BREAK) {
        return TCL_BREAK;
    }
    lastCmdStatus = result;
    if (result != TCL_OK) {
        TRACE("Error: %d", result);
    }
    return result;
}

static int
GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, bool *boolPtr)
{
    int value;

    if (Tcl_GetBooleanFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    *boolPtr = (bool)value;
    return TCL_OK;
}

static int
GetFloatFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, float *valuePtr)
{
    double value;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    *valuePtr = (float)value;
    return TCL_OK;
}

static int
CameraOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double quat[4];

    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setCameraOrientation(quat);
    return TCL_OK;
}

static int
CameraPanOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double x, y;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->panCamera(x, y);
    return TCL_OK;
}

static int
CameraResetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *string = Tcl_GetString(objv[2]);
        char c = string[0];
        if ((c != 'a') || (strcmp(string, "all") != 0)) {
            Tcl_AppendResult(interp, "bad camera reset option \"", string,
                         "\": should be all", (char*)NULL);
            return TCL_ERROR;
        }
        g_renderer->resetCamera(true);
    } else {
        g_renderer->resetCamera(false);
    }
    return TCL_OK;
}

static int
CameraRotateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double x, y;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->rotateCamera(x, y);
    return TCL_OK;
}

static int
CameraZoomOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double z;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &z) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->zoomCamera(z);
    return TCL_OK;
}

static Rappture::CmdSpec cameraOps[] = {
    {"orient", 1, CameraOrientOp, 6, 6, "qw qx qy qz"},
    {"pan",    1, CameraPanOp, 4, 4, "panX panY"},
    {"reset",  2, CameraResetOp, 2, 3, "?all?"},
    {"rotate", 2, CameraRotateOp, 4, 4, "azimuth elevation"},
    {"zoom",   1, CameraZoomOp, 3, 3, "zoomAmount"}
};
static int nCameraOps = NumCmdSpecs(cameraOps);

static int
CameraCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nCameraOps, cameraOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
ClientInfoCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Tcl_DString ds;
    Tcl_Obj *objPtr, *listObjPtr, **items;
    int numItems;
    char buf[BUFSIZ];
    const char *string;
    int length;
    int result;
    static bool first = true;

    /* Use the initial client key value pairs as the parts for a generating
     * a unique file name. */
    int fd = GeoVis::getStatsFile(interp, objv[1]);
    if (fd < 0) {
	Tcl_AppendResult(interp, "can't open stats file: ", 
                         Tcl_PosixError(interp), (char *)NULL);
	return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_IncrRefCount(listObjPtr);
    if (first) {
        first = false;
        objPtr = Tcl_NewStringObj("render_start", 12);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        /* server */
        objPtr = Tcl_NewStringObj("server", 6);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("geovis", 6);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        /* pid */
        objPtr = Tcl_NewStringObj("pid", 3);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(getpid()));
        /* machine */
        objPtr = Tcl_NewStringObj("machine", 7);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        gethostname(buf, BUFSIZ-1);
        buf[BUFSIZ-1] = '\0';
        objPtr = Tcl_NewStringObj(buf, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    } else {
        objPtr = Tcl_NewStringObj("render_info", 11);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    /* date */
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("date", 4));
    strcpy(buf, ctime(&GeoVis::g_stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(buf, -1));
    /* date_secs */
    Tcl_ListObjAppendElement(interp, listObjPtr, 
                             Tcl_NewStringObj("date_secs", 9));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
                             Tcl_NewLongObj(GeoVis::g_stats.start.tv_sec));
    /* Client arguments. */
    if (Tcl_ListObjGetElements(interp, objv[1], &numItems, &items) != TCL_OK) {
	return TCL_ERROR;
    }
    for (int i = 0; i < numItems; i++) {
        Tcl_ListObjAppendElement(interp, listObjPtr, items[i]);
    }
    Tcl_DStringInit(&ds);
    string = Tcl_GetStringFromObj(listObjPtr, &length);
    Tcl_DStringAppend(&ds, string, length);
    Tcl_DStringAppend(&ds, "\n", 1);
    result = GeoVis::writeToStatsFile(fd, Tcl_DStringValue(&ds), 
                                      Tcl_DStringLength(&ds));
    Tcl_DStringFree(&ds);
    Tcl_DecrRefCount(listObjPtr);
    return result;
}

static int
ImageFlushCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    lastCmdStatus = TCL_BREAK;
    return TCL_OK;
}

static int
RendererLoadOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    g_renderer->loadEarthFile(Tcl_GetString(objv[2]));
    return TCL_OK;
}

static int
RendererRenderOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    g_renderer->eventuallyRender();
    return TCL_OK;
}

static Rappture::CmdSpec rendererOps[] = {
    {"load",       1, RendererLoadOp, 3, 3, "path"},
    {"render",     1, RendererRenderOp, 2, 2, ""},
};
static int nRendererOps = NumCmdSpecs(rendererOps);

static int
RendererCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nRendererOps, rendererOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
ScreenBgColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    float color[3];

    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setBackgroundColor(color);
    return TCL_OK;
}

static int
ScreenSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    int width, height;

    if (Tcl_GetIntFromObj(interp, objv[2], &width) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &height) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setWindowSize(width, height);
    return TCL_OK;
}

static Rappture::CmdSpec screenOps[] = {
    {"bgcolor", 1, ScreenBgColorOp, 5, 5, "r g b"},
    {"size", 1, ScreenSizeOp, 4, 4, "width height"}
};
static int nScreenOps = NumCmdSpecs(screenOps);

static int
ScreenCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nScreenOps, screenOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/**
 * \brief Execute commands from client in Tcl interpreter
 * 
 * In this threaded model, the select call is for event compression.  We
 * want to execute render server commands as long as they keep coming.  
 * This lets us execute a stream of many commands but render once.  This 
 * benefits camera movements, screen resizing, and opacity changes 
 * (using a slider on the client).  The down side is you don't render
 * until there's a lull in the command stream.  If the client needs an
 * image, it can issue an "imgflush" command.  That breaks us out of the
 * read loop.
 */
int
GeoVis::processCommands(Tcl_Interp *interp,
                        ClientData clientData,
                        ReadBuffer *inBufPtr, 
                        int fdOut)
{
    int ret = 1;
    int status = TCL_OK;

    Tcl_DString command;
    Tcl_DStringInit(&command);
    fd_set readFds;
    struct timeval tv, *tvPtr;

    FD_ZERO(&readFds);
    FD_SET(inBufPtr->file(), &readFds);
    tvPtr = NULL;                       /* Wait for the first read. This is so
                                         * that we don't spin when no data is
                                         * available. */
    while (inBufPtr->isLineAvailable() || 
           (select(1, &readFds, NULL, NULL, tvPtr) > 0)) {
        size_t numBytes;
        unsigned char *buffer;

        /* A short read is treated as an error here because we assume that we
         * will always get commands line by line. */
        if (inBufPtr->getLine(&numBytes, &buffer) != ReadBuffer::OK) {
            /* Terminate the server if we can't communicate with the client
             * anymore. */
            if (inBufPtr->status() == ReadBuffer::ENDFILE) {
                TRACE("Exiting server on EOF from client");
                return -1;
            } else {
                ERROR("Exiting server, failed to read from client: %s",
                      strerror(errno));
                return -1;
            }
        }
        Tcl_DStringAppend(&command, (char *)buffer, numBytes);
        if (Tcl_CommandComplete(Tcl_DStringValue(&command))) {
            struct timeval start, finish;
            gettimeofday(&start, NULL);
            status = ExecuteCommand(interp, &command);
            gettimeofday(&finish, NULL);
            g_stats.cmdTime += (MSECS_ELAPSED(start, finish) / 1.0e+3);
            g_stats.nCommands++;
            if (status == TCL_BREAK) {
                return 1;               /* This was caused by a "imgflush"
                                         * command. Break out of the read loop
                                         * and allow a new image to be
                                         * rendered. */
            } else { //if (status != TCL_OK) {
                ret = 0;
                if (handleError(interp, clientData, status, fdOut) < 0) {
                    return -1;
                }
            }
        }

        tv.tv_sec = tv.tv_usec = 0L;    /* On successive reads, we break out
                                         * if no data is available. */
        FD_SET(inBufPtr->file(), &readFds);
        tvPtr = &tv;
    }

    return ret;
}

/**
 * \brief Send error message to client socket
 */
int
GeoVis::handleError(Tcl_Interp *interp,
                    ClientData clientData,
                    int status, int fdOut)
{
    const char *string;
    int nBytes;

    if (status != TCL_OK) {
        string = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
        nBytes = strlen(string);
        if (nBytes > 0) {
            TRACE("status=%d errorInfo=(%s)", status, string);

            std::ostringstream oss;
            oss << "nv>viserror -type internal_error -token " << g_stats.nCommands << " -bytes " << nBytes << "\n" << string;
            nBytes = oss.str().length();

            if (queueResponse(oss.str().c_str(), nBytes, Response::VOLATILE, Response::ERROR) < 0) {
                return -1;
            }
        }
    }

    string = getUserMessages();
    nBytes = strlen(string);
    if (nBytes > 0) {
        TRACE("userError=(%s)", string);

        std::ostringstream oss;
        oss << "nv>viserror -type error -token " << g_stats.nCommands << " -bytes " << nBytes << "\n" << string;
        nBytes = oss.str().length();

        if (queueResponse(oss.str().c_str(), nBytes, Response::VOLATILE, Response::ERROR) < 0) {
            return -1;
        }

        clearUserMessages();
    }

    return 0;
}

/**
 * \brief Create Tcl interpreter and add commands
 *
 * \return The initialized Tcl interpreter
 */
void
GeoVis::initTcl(Tcl_Interp *interp, ClientData clientData)
{
    Tcl_MakeSafe(interp);
    Tcl_CreateObjCommand(interp, "camera",         CameraCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "clientinfo",     ClientInfoCmd,     clientData, NULL);
    Tcl_CreateObjCommand(interp, "imgflush",       ImageFlushCmd,     clientData, NULL);
    Tcl_CreateObjCommand(interp, "renderer",       RendererCmd,       clientData, NULL);
    Tcl_CreateObjCommand(interp, "screen",         ScreenCmd,         clientData, NULL);
}

/**
 * \brief Delete Tcl commands and interpreter
 */
void GeoVis::exitTcl(Tcl_Interp *interp)
{
    Tcl_DeleteCommand(interp, "camera");
    Tcl_DeleteCommand(interp, "clientinfo");
    Tcl_DeleteCommand(interp, "imgflush");
    Tcl_DeleteCommand(interp, "renderer");
    Tcl_DeleteCommand(interp, "screen");

    Tcl_DeleteInterp(interp);
}
