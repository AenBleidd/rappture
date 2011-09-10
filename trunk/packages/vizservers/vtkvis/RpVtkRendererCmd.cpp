/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <tcl.h>

#include "Trace.h"
#include "CmdProc.h"
#include "RpVtkRendererCmd.h"
#include "RpVtkRenderServer.h"
#include "PPMWriter.h"
#include "TGAWriter.h"

using namespace Rappture::VtkVis;

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

static size_t
SocketRead(void *bytes, size_t len)
{
#ifdef notdef
    size_t ofs = 0;
    ssize_t bytesRead = 0;
    while ((bytesRead = read(g_fdIn, bytes + ofs, len - ofs)) > 0) {
        ofs += bytesRead;
        if (ofs == len)
            break;
    }
    TRACE("bytesRead: %lu", ofs);
    return ofs;
#else
    size_t bytesRead = fread(bytes, 1, len, g_fIn);
    TRACE("bytesRead: %lu", bytesRead);
    return bytesRead;
#endif
}

static int
ExecuteCommand(Tcl_Interp *interp, Tcl_DString *dsPtr) 
{
    int result;

    result = Tcl_Eval(interp, Tcl_DStringValue(dsPtr));
    Tcl_DStringSetLength(dsPtr, 0);

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
AxisColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double color[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setAxesColor(color);
    return TCL_OK;
}

static int
AxisFlyModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    Renderer::AxesFlyMode mode;
    if ((c == 's') && (strcmp(string, "static_edges") == 0)) {
        mode = Renderer::FLY_STATIC_EDGES;
    } else if ((c == 's') && (strcmp(string, "static_triad") == 0)) {
        mode = Renderer::FLY_STATIC_TRIAD;
    } else if ((c == 'o') && (strcmp(string, "outer_edges") == 0)) {
        mode = Renderer::FLY_OUTER_EDGES;
    } else if ((c == 'f') && (strcmp(string, "furthest_triad") == 0)) {
        mode = Renderer::FLY_FURTHEST_TRIAD;
    } else if ((c == 'c') && (strcmp(string, "closest_triad") == 0)) {
        mode = Renderer::FLY_CLOSEST_TRIAD;
    } else {
        Tcl_AppendResult(interp, "bad axis flymode option \"", string,
                         "\": should be static_edges, static_triad, outer_edges, furthest_triad, or closest_triad", (char*)NULL);
        return TCL_ERROR;
    }
    g_renderer->setAxesFlyMode(mode);
    return TCL_OK;
}

static int
AxisGridOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisGridVisibility(Renderer::X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisGridVisibility(Renderer::Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisGridVisibility(Renderer::Z_AXIS, visible);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesGridVisibility(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisLabelsVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisLabelVisibility(Renderer::X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisLabelVisibility(Renderer::Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisLabelVisibility(Renderer::Z_AXIS, visible);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesLabelVisibility(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisNameOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    const char *title = Tcl_GetString(objv[3]);
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisTitle(Renderer::X_AXIS, title);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisTitle(Renderer::Y_AXIS, title);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisTitle(Renderer::Z_AXIS, title);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName title", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisTicksVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisTickVisibility(Renderer::X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisTickVisibility(Renderer::Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisTickVisibility(Renderer::Z_AXIS, visible);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesTickVisibility(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisTickPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'i') && (strcmp(string, "inside") == 0)) {
        g_renderer->setAxesTickPosition(Renderer::TICKS_INSIDE);
    } else if ((c == 'o') && (strcmp(string, "outside") == 0)) {
        g_renderer->setAxesTickPosition(Renderer::TICKS_OUTSIDE);
    } else if ((c == 'b') && (strcmp(string, "both") == 0)) {
        g_renderer->setAxesTickPosition(Renderer::TICKS_BOTH);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be inside, outside or both", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisUnitsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    const char *units = Tcl_GetString(objv[3]);
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisUnits(Renderer::X_AXIS, units);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisUnits(Renderer::Y_AXIS, units);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisUnits(Renderer::Z_AXIS, units);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName units", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisVisibility(Renderer::X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisVisibility(Renderer::Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisVisibility(Renderer::Z_AXIS, visible);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesVisibility(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static Rappture::CmdSpec axisOps[] = {
    {"color",   1, AxisColorOp, 5, 5, "r g b"},
    {"flymode", 1, AxisFlyModeOp, 3, 3, "mode"},
    {"grid",    1, AxisGridOp, 4, 4, "axis bool"},
    {"labels",  1, AxisLabelsVisibleOp, 4, 4, "axis bool"},
    {"name",    1, AxisNameOp, 4, 4, "axis title"},
    {"tickpos", 2, AxisTickPositionOp, 3, 3, "position"},
    {"ticks",   2, AxisTicksVisibleOp, 4, 4, "axis bool"},
    {"units",   1, AxisUnitsOp, 4, 4, "axis units"},
    {"visible", 1, AxisVisibleOp, 4, 4, "axis bool"}
};
static int nAxisOps = NumCmdSpecs(axisOps);

static int
AxisCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nAxisOps, axisOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
CameraModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Renderer::CameraMode mode;
    const char *string = Tcl_GetString(objv[2]);
    if ((strcmp(string, "persp") == 0)) {
        mode = Renderer::PERSPECTIVE;
    } else if ((strcmp(string, "ortho") == 0)) {
        mode = Renderer::ORTHO;
    } else if ((strcmp(string, "image") == 0)) {
        mode = Renderer::IMAGE;
    } else {
        Tcl_AppendResult(interp, "bad camera mode option \"", string,
                         "\": should be persp, ortho or image", (char*)NULL);
        return TCL_ERROR;
    }
    g_renderer->setCameraMode(mode);
    return TCL_OK;
}

static int
CameraGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double pos[3];
    double focalPt[3];
    double viewUp[3];

    g_renderer->getCameraOrientationAndPosition(pos, focalPt, viewUp);

    char buf[256];
    snprintf(buf, sizeof(buf), "nv>camera set %.12e %.12e %.12e %.12e %.12e %.12e %.12e %.12e %.12e\n", 
             pos[0], pos[1], pos[2], focalPt[0], focalPt[1], focalPt[2], viewUp[0], viewUp[1], viewUp[2]);

    ssize_t bytesWritten = SocketWrite(buf, strlen(buf));

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
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
CameraOrthoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    const char *string = Tcl_GetString(objv[2]);

    if (string[0] == 'p' && strcmp(string, "pixel") == 0) {
        int x, y, width, height;
        if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK ||
            Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK ||
            Tcl_GetIntFromObj(interp, objv[5], &width) != TCL_OK ||
            Tcl_GetIntFromObj(interp, objv[6], &height) != TCL_OK) {
            return TCL_ERROR;
        }
        g_renderer->setCameraZoomRegionPixels(x, y, width, height);
    } else if (string[0] == 'w' && strcmp(string, "world") == 0) {
        double x, y, width, height;
        if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
            Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK ||
            Tcl_GetDoubleFromObj(interp, objv[5], &width) != TCL_OK ||
            Tcl_GetDoubleFromObj(interp, objv[6], &height) != TCL_OK) {
            return TCL_ERROR;
        }
        g_renderer->setCameraZoomRegion(x, y, width, height);
    } else {
        Tcl_AppendResult(interp, "bad camera ortho option \"", string,
                         "\": should be pixel or world", (char*)NULL);
        return TCL_ERROR;
    }

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
    double yaw, pitch, roll;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &yaw) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pitch) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &roll) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->rotateCamera(yaw, pitch, roll);
    return TCL_OK;
}

static int
CameraSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double pos[3];
    double focalPt[3];
    double viewUp[3];

    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &focalPt[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[6], &focalPt[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[7], &focalPt[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[8], &viewUp[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[9], &viewUp[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[10], &viewUp[2]) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setCameraOrientationAndPosition(pos, focalPt, viewUp);
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
    {"get", 1, CameraGetOp, 2, 2, ""},
    {"mode", 1, CameraModeOp, 3, 3, "mode"},
    {"orient", 3, CameraOrientOp, 6, 6, "qw qx qy qz"},
    {"ortho", 1, CameraOrthoOp, 7, 7, "coordMode x y width height"},
    {"pan", 1, CameraPanOp, 4, 4, "panX panY"},
    {"reset", 2, CameraResetOp, 2, 3, "?all?"},
    {"rotate", 2, CameraRotateOp, 5, 5, "angle angle angle"},
    {"set", 1, CameraSetOp, 11, 11, "posX posY posZ focalPtX focalPtY focalPtZ viewUpX viewUpY viewUpZ"},
    {"zoom", 1, CameraZoomOp, 3, 3, "zoomAmount"}
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
ColorMapAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[2]);
    int cmapc, omapc;
    Tcl_Obj **cmapv = NULL;
    Tcl_Obj **omapv = NULL;

    if (Tcl_ListObjGetElements(interp, objv[3], &cmapc, &cmapv) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((cmapc % 4) != 0) {
        Tcl_AppendResult(interp, "wrong # elements in colormap: should be ",
                         "{ value r g b ... }", (char*)NULL);
        return TCL_ERROR;
    }

    ColorMap *colorMap = new ColorMap(name);
    colorMap->setNumberOfTableEntries(256);

    for (int i = 0; i < cmapc; i += 4) {
        double val[4];
        for (int j = 0; j < 4; j++) {
            if (Tcl_GetDoubleFromObj(interp, cmapv[i+j], &val[j]) != TCL_OK) {
                delete colorMap;
                return TCL_ERROR;
            }
            if ((val[j] < 0.0) || (val[j] > 1.0)) {
                Tcl_AppendResult(interp, "bad colormap value \"",
                                 Tcl_GetString(cmapv[i+j]),
                                 "\": should be in the range [0,1]", (char*)NULL);
                delete colorMap;
                return TCL_ERROR;
            }
        }
        ColorMap::ControlPoint cp;
        cp.value = val[0];
        for (int c = 0; c < 3; c++) {
            cp.color[c] = val[c+1];
        }
        colorMap->addControlPoint(cp);
    }

    if (Tcl_ListObjGetElements(interp, objv[4], &omapc, &omapv) != TCL_OK) {
        delete colorMap;
        return TCL_ERROR;
    }
    if ((omapc % 2) != 0) {
        Tcl_AppendResult(interp, "wrong # elements in opacitymap: should be ",
                         "{ value alpha ... }", (char*)NULL);
        delete colorMap;
        return TCL_ERROR;
    }
    for (int i = 0; i < omapc; i += 2) {
        double val[2];
        for (int j = 0; j < 2; j++) {
            if (Tcl_GetDoubleFromObj(interp, omapv[i+j], &val[j]) != TCL_OK) {
                delete colorMap;
                return TCL_ERROR;
            }
            if ((val[j] < 0.0) || (val[j] > 1.0)) {
                Tcl_AppendResult(interp, "bad opacitymap value \"",
                                 Tcl_GetString(omapv[i+j]),
                                 "\": should be in the range [0,1]", (char*)NULL);
                delete colorMap;
                return TCL_ERROR;
            }
        }
        ColorMap::OpacityControlPoint ocp;
        ocp.value = val[0];
        ocp.alpha = val[1];
        colorMap->addOpacityControlPoint(ocp);
    }

    colorMap->build();
    g_renderer->addColorMap(name, colorMap);
    return TCL_OK;
}

static int
ColorMapDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteColorMap(name);
    } else {
        g_renderer->deleteColorMap("all");
    }
    return TCL_OK;
}

static Rappture::CmdSpec colorMapOps[] = {
    { "add",    1, ColorMapAddOp,    5, 5, "colorMapName colormap alphamap"},
    { "delete", 1, ColorMapDeleteOp, 2, 3, "?colorMapName?"}
};
static int nColorMapOps = NumCmdSpecs(colorMapOps);

static int
ColorMapCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nColorMapOps, colorMapOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
Contour2DAddContourListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    std::vector<double> contourList;

    int clistc;
    Tcl_Obj **clistv;

    if (Tcl_ListObjGetElements(interp, objv[3], &clistc, &clistv) != TCL_OK) {
        return TCL_ERROR;
    }

    for (int i = 0; i < clistc; i++) {
        double val;
        if (Tcl_GetDoubleFromObj(interp, clistv[i], &val) != TCL_OK) {
            return TCL_ERROR;
        }
        contourList.push_back(val);
    }

    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        if (!g_renderer->addContour2D(name, contourList)) {
            Tcl_AppendResult(interp, "Failed to create contour2d", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addContour2D("all", contourList)) {
            Tcl_AppendResult(interp, "Failed to create contour2d for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
Contour2DAddNumContoursOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    int numContours;
    if (Tcl_GetIntFromObj(interp, objv[3], &numContours) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        if (!g_renderer->addContour2D(name, numContours)) {
            Tcl_AppendResult(interp, "Failed to create contour2d", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
       if (!g_renderer->addContour2D("all", numContours)) {
            Tcl_AppendResult(interp, "Failed to create contour2d for one or more data sets", (char*)NULL);
            return TCL_ERROR;
       }
    }
    return TCL_OK;
}

static Rappture::CmdSpec contour2dAddOps[] = {
    {"contourlist", 1, Contour2DAddContourListOp, 4, 5, "contourList ?dataSetName?"},
    {"numcontours", 1, Contour2DAddNumContoursOp, 4, 5, "numContours ?dataSetName?"}
};
static int nContour2dAddOps = NumCmdSpecs(contour2dAddOps);

static int
Contour2DAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nContour2dAddOps, contour2dAddOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
Contour2DDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteContour2D(name);
    } else {
        g_renderer->deleteContour2D("all");
    }
    return TCL_OK;
}

static int
Contour2DLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour2DLighting(name, state);
    } else {
        g_renderer->setContour2DLighting("all", state);
    }
    return TCL_OK;
}

static int
Contour2DLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setContour2DColor(name, color);
    } else {
        g_renderer->setContour2DColor("all", color);
    }
    return TCL_OK;
}

static int
Contour2DLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour2DEdgeWidth(name, width);
    } else {
        g_renderer->setContour2DEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
Contour2DOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour2DOpacity(name, opacity);
    } else {
        g_renderer->setContour2DOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
Contour2DOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setContour2DOrientation(name, quat);
    } else {
        g_renderer->setContour2DOrientation("all", quat);
    }
    return TCL_OK;
}

static int
Contour2DPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setContour2DPosition(name, pos);
    } else {
        g_renderer->setContour2DPosition("all", pos);
    }
    return TCL_OK;
}

static int
Contour2DScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setContour2DScale(name, scale);
    } else {
        g_renderer->setContour2DScale("all", scale);
    }
    return TCL_OK;
}

static int
Contour2DVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour2DVisibility(name, state);
    } else {
        g_renderer->setContour2DVisibility("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec contour2dOps[] = {
    {"add",       1, Contour2DAddOp, 4, 5, "oper value ?dataSetName?"},
    {"color",     1, Contour2DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"delete",    1, Contour2DDeleteOp, 2, 3, "?dataSetName?"},
    {"lighting",  3, Contour2DLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, Contour2DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, Contour2DLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   2, Contour2DOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",    2, Contour2DOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       1, Contour2DPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",     1, Contour2DScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",   1, Contour2DVisibleOp, 3, 4, "bool ?dataSetName?"}
};
static int nContour2dOps = NumCmdSpecs(contour2dOps);

static int
Contour2DCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nContour2dOps, contour2dOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
Contour3DAddContourListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    std::vector<double> contourList;

    int clistc;
    Tcl_Obj **clistv;

    if (Tcl_ListObjGetElements(interp, objv[3], &clistc, &clistv) != TCL_OK) {
        return TCL_ERROR;
    }

    for (int i = 0; i < clistc; i++) {
        double val;
        if (Tcl_GetDoubleFromObj(interp, clistv[i], &val) != TCL_OK) {
            return TCL_ERROR;
        }
        contourList.push_back(val);
    }

    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        if (!g_renderer->addContour3D(name, contourList)) {
            Tcl_AppendResult(interp, "Failed to create contour3d", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addContour3D("all", contourList)) {
            Tcl_AppendResult(interp, "Failed to create contour3d for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
Contour3DAddNumContoursOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    int numContours;
    if (Tcl_GetIntFromObj(interp, objv[3], &numContours) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        if (!g_renderer->addContour3D(name, numContours)) {
            Tcl_AppendResult(interp, "Failed to create contour3d", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addContour3D("all", numContours)) {
            Tcl_AppendResult(interp, "Failed to create contour3d for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static Rappture::CmdSpec contour3dAddOps[] = {
    {"contourlist", 1, Contour3DAddContourListOp, 4, 5, "contourList ?dataSetName?"},
    {"numcontours", 1, Contour3DAddNumContoursOp, 4, 5, "numContours ?dataSetName?"}
};
static int nContour3dAddOps = NumCmdSpecs(contour3dAddOps);

static int
Contour3DAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nContour3dAddOps, contour3dAddOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
Contour3DColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setContour3DColor(name, color);
    } else {
        g_renderer->setContour3DColor("all", color);
    }
    return TCL_OK;
}

static int
Contour3DColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setContour3DColorMap(dataSetName, colorMapName);
    } else {
        g_renderer->setContour3DColorMap("all", colorMapName);
    }
    return TCL_OK;
}

static int
Contour3DDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteContour3D(name);
    } else {
        g_renderer->deleteContour3D("all");
    }
    return TCL_OK;
}

static int
Contour3DEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour3DEdgeVisibility(name, state);
    } else {
        g_renderer->setContour3DEdgeVisibility("all", state);
    }
    return TCL_OK;
}

static int
Contour3DLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour3DLighting(name, state);
    } else {
        g_renderer->setContour3DLighting("all", state);
    }
    return TCL_OK;
}

static int
Contour3DLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setContour3DEdgeColor(name, color);
    } else {
        g_renderer->setContour3DEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
Contour3DLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour3DEdgeWidth(name, width);
    } else {
        g_renderer->setContour3DEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
Contour3DOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour3DOpacity(name, opacity);
    } else {
        g_renderer->setContour3DOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
Contour3DOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setContour3DOrientation(name, quat);
    } else {
        g_renderer->setContour3DOrientation("all", quat);
    }
    return TCL_OK;
}

static int
Contour3DPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setContour3DPosition(name, pos);
    } else {
        g_renderer->setContour3DPosition("all", pos);
    }
    return TCL_OK;
}

static int
Contour3DScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setContour3DScale(name, scale);
    } else {
        g_renderer->setContour3DScale("all", scale);
    }
    return TCL_OK;
}

static int
Contour3DVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour3DVisibility(name, state);
    } else {
        g_renderer->setContour3DVisibility("all", state);
    }
    return TCL_OK;
}

static int
Contour3DWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour3DWireframe(name, state);
    } else {
        g_renderer->setContour3DWireframe("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec contour3dOps[] = {
    {"add",       1, Contour3DAddOp, 4, 5, "oper value ?dataSetName?"},
    {"ccolor",    2, Contour3DColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  2, Contour3DColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",    1, Contour3DDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, Contour3DEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",  3, Contour3DLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, Contour3DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, Contour3DLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   2, Contour3DOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",    2, Contour3DOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       1, Contour3DPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",     1, Contour3DScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",   1, Contour3DVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe", 1, Contour3DWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nContour3dOps = NumCmdSpecs(contour3dOps);

static int
Contour3DCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nContour3dOps, contour3dOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
DataSetActiveScalarsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    const char *scalarName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        if (!g_renderer->setDataSetActiveScalars(name, scalarName)) {
            Tcl_AppendResult(interp, "scalar \"", scalarName,
                         "\" not found", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->setDataSetActiveScalars("all", scalarName)) {
            Tcl_AppendResult(interp, "scalar \"", scalarName,
                         "\" not found in one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
DataSetActiveVectorsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    const char *vectorName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        if (!g_renderer->setDataSetActiveVectors(name, vectorName)) {
            Tcl_AppendResult(interp, "vector \"", vectorName,
                         "\" not found", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->setDataSetActiveVectors("all", vectorName)) {
            Tcl_AppendResult(interp, "vector \"", vectorName,
                         "\" not found in one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
DataSetAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[2]);
    const char *string = Tcl_GetString(objv[3]);
    char c = string[0];
    if ((c != 'd') || (strcmp(string, "data") != 0)) {
        Tcl_AppendResult(interp, "bad dataset option \"", string,
                         "\": should be data", (char*)NULL);
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[4]);
    c = string[0];
    if ((c != 'f') || (strcmp(string, "follows") != 0)) {
        Tcl_AppendResult(interp, "bad dataset option \"", string,
                         "\": should be follows", (char*)NULL);
        return TCL_ERROR;
    }
    int nbytes = 0;
    if (Tcl_GetIntFromObj(interp, objv[5], &nbytes) != TCL_OK ||
        nbytes < 0) {
        return TCL_ERROR;
    }
    char *data = (char *)malloc(nbytes);
    size_t bytesRead = SocketRead(data, nbytes);
    if (bytesRead < 0) {
        free(data);
        return TCL_ERROR;
    }
    g_renderer->addDataSet(name);
    g_renderer->setData(name, data, nbytes);
    free(data);
    return TCL_OK;
}

static int
DataSetDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        TRACE("Deleting dataset %s", name);
        g_renderer->deleteDataSet(name);
    } else {
        g_renderer->deleteDataSet("all");
    }
    return TCL_OK;
}

static int
DataSetGetScalarPixelOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[5]);
    int x, y;
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }
    double value;
    if (!g_renderer->getScalarValueAtPixel(name, x, y, &value)) {
        Tcl_AppendResult(interp, "Pixel out of dataset bounds or no scalar data available", (char*)NULL);
        return TCL_ERROR;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "nv>dataset scalar pixel %d %d %g %s\n", x, y, value, name);

    ssize_t bytesWritten = SocketWrite(buf, strlen(buf));

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
DataSetGetScalarWorldOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[6]);
    double x, y, z;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &z) != TCL_OK) {
        return TCL_ERROR;
    }
    double value;
    if (!g_renderer->getScalarValue(name, x, y, z, &value)) {
        Tcl_AppendResult(interp, "Coordinate out of dataset bounds or no scalar data available", (char*)NULL);
        return TCL_ERROR;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "nv>dataset scalar world %g %g %g %g %s\n", x, y, z, value, name);

    ssize_t bytesWritten = SocketWrite(buf, strlen(buf));

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static Rappture::CmdSpec dataSetGetScalarOps[] = {
    {"pixel", 1, DataSetGetScalarPixelOp, 6, 6, "x y dataSetName"},
    {"world", 1, DataSetGetScalarWorldOp, 7, 7, "x y z dataSetName"}
};
static int nDataSetGetScalarOps = NumCmdSpecs(dataSetGetScalarOps);

static int
DataSetGetScalarOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nDataSetGetScalarOps, dataSetGetScalarOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
DataSetGetVectorPixelOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[5]);
    int x, y;
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }
    double value[3];
    if (!g_renderer->getVectorValueAtPixel(name, x, y, value)) {
        Tcl_AppendResult(interp, "Pixel out of dataset bounds or no vector data available", (char*)NULL);
        return TCL_ERROR;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "nv>dataset vector pixel %d %d %g %g %g %s\n", x, y,
             value[0], value[1], value[2], name);

    ssize_t bytesWritten = SocketWrite(buf, strlen(buf));

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
DataSetGetVectorWorldOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[6]);
    double x, y, z;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &z) != TCL_OK) {
        return TCL_ERROR;
    }
    double value[3];
    if (!g_renderer->getVectorValue(name, x, y, z, value)) {
        Tcl_AppendResult(interp, "Coordinate out of dataset bounds or no vector data available", (char*)NULL);
        return TCL_ERROR;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "nv>dataset vector world %g %g %g %g %g %g %s\n", x, y, z,
             value[0], value[1], value[2], name);

    ssize_t bytesWritten = SocketWrite(buf, strlen(buf));

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static Rappture::CmdSpec dataSetGetVectorOps[] = {
    {"pixel", 1, DataSetGetVectorPixelOp, 6, 6, "x y dataSetName"},
    {"world", 1, DataSetGetVectorWorldOp, 7, 7, "x y z dataSetName"}
};
static int nDataSetGetVectorOps = NumCmdSpecs(dataSetGetVectorOps);

static int
DataSetGetVectorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nDataSetGetVectorOps, dataSetGetVectorOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
DataSetMapRangeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    const char *value = Tcl_GetString(objv[2]);
    if (strcmp(value, "all") == 0) {
        g_renderer->setUseCumulativeDataRange(true);
    } else if (strcmp(value, "visible") == 0) {
        g_renderer->setUseCumulativeDataRange(true, true);
    } else if (strcmp(value, "separate") == 0) {
        g_renderer->setUseCumulativeDataRange(false);
    } else {
        Tcl_AppendResult(interp, "bad maprange option \"", value,
                         "\": should be all, visible, or separate", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
DataSetNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    std::vector<std::string> dataSets;
    g_renderer->getDataSetNames(dataSets);
    std::ostringstream oss;
    size_t len = 0;
    oss << "nv>dataset names {";
    len += 18;
    for (size_t i = 0; i < dataSets.size(); i++) {
        oss << "\"" << dataSets[i] << "\"";
        len += 2 + dataSets[i].length();
        if (i < dataSets.size() - 1) {
            oss << " ";
            len++;
        }
    }
    oss << "}\n";
    len += 2;

    size_t bytesWritten = SocketWrite(oss.str().c_str(), len);

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
DataSetOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setDataSetOpacity(name, opacity);
    } else {
        g_renderer->setDataSetOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
DataSetOutlineOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setDataSetShowBounds(name, state);
    } else {
        g_renderer->setDataSetShowBounds("all", state);
    }
    return TCL_OK;
}

static int
DataSetOutlineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setDataSetOutlineColor(name, color);
    } else {
        g_renderer->setDataSetOutlineColor("all", color);
    }
    return TCL_OK;
}

static int
DataSetVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setDataSetVisibility(name, state);
    } else {
        g_renderer->setDataSetVisibility("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec dataSetOps[] = {
    {"add",       1, DataSetAddOp, 6, 6, "name data follows nBytes"},
    {"color",     1, DataSetOutlineColorOp, 5, 6, "r g b ?name?"},
    {"delete",    1, DataSetDeleteOp, 2, 3, "?name?"},
    {"getscalar", 4, DataSetGetScalarOp, 6, 7, "oper x y ?z? name"},
    {"getvector", 4, DataSetGetVectorOp, 6, 7, "oper x y ?z? name"},
    {"maprange",  1, DataSetMapRangeOp, 3, 3, "value"},
    {"names",     1, DataSetNamesOp, 2, 2, ""},
    {"opacity",   2, DataSetOpacityOp, 3, 4, "value ?name?"},
    {"outline",   2, DataSetOutlineOp, 3, 4, "bool ?name?"},
    {"scalar",    1, DataSetActiveScalarsOp, 3, 4, "scalarName ?name?"},
    {"vector",    2, DataSetActiveVectorsOp, 3, 4, "vectorName ?name?"},
    {"visible",   2, DataSetVisibleOp, 3, 4, "bool ?name?"}
};
static int nDataSetOps = NumCmdSpecs(dataSetOps);

static int
DataSetCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nDataSetOps, dataSetOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
GlyphsAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Glyphs::GlyphShape shape;

    const char *shapeOpt = Tcl_GetString(objv[2]);
    if (shapeOpt[0] == 'a' && strcmp(shapeOpt, "arrow") == 0) {
        shape = Glyphs::ARROW;
    } else if (shapeOpt[0] == 'c' && strcmp(shapeOpt, "cone") == 0) {
        shape = Glyphs::CONE;
    } else if (shapeOpt[0] == 'c' && strcmp(shapeOpt, "cube") == 0) {
        shape = Glyphs::CUBE;
    } else if (shapeOpt[0] == 'c' && strcmp(shapeOpt, "cylinder") == 0) {
        shape = Glyphs::CYLINDER;
    } else if (shapeOpt[0] == 'd' && strcmp(shapeOpt, "dodecahedron") == 0) {
        shape = Glyphs::DODECAHEDRON;
    } else if (shapeOpt[0] == 'i' && strcmp(shapeOpt, "icosahedron") == 0) {
        shape = Glyphs::ICOSAHEDRON;
    } else if (shapeOpt[0] == 'l' && strcmp(shapeOpt, "line") == 0) {
        shape = Glyphs::LINE;
    } else if (shapeOpt[0] == 'o' && strcmp(shapeOpt, "octahedron") == 0) {
        shape = Glyphs::OCTAHEDRON;
    } else if (shapeOpt[0] == 's' && strcmp(shapeOpt, "sphere") == 0) {
        shape = Glyphs::SPHERE;
    } else if (shapeOpt[0] == 't' && strcmp(shapeOpt, "tetrahedron") == 0) {
        shape = Glyphs::TETRAHEDRON;
    } else {
        Tcl_AppendResult(interp, "bad shape option \"", shapeOpt,
                         "\": should be one of: 'arrow', 'cone', 'cube', 'cylinder', 'dodecahedron', 'icosahedron', 'octahedron', 'sphere', 'tetrahedron'", (char*)NULL);
        return TCL_ERROR;
    }

    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        if (!g_renderer->addGlyphs(name, shape)) {
            Tcl_AppendResult(interp, "Failed to create glyphs", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGlyphs("all", shape)) {
            Tcl_AppendResult(interp, "Failed to create glyphs for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
GlyphsColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGlyphsColor(name, color);
    } else {
        g_renderer->setGlyphsColor("all", color);
    }
    return TCL_OK;
}

static int
GlyphsColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsColorMap(dataSetName, colorMapName);
    } else {
        g_renderer->setGlyphsColorMap("all", colorMapName);
    }
    return TCL_OK;
}

static int
GlyphsColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Glyphs::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'c' && strcmp(str, "ccolor") == 0) {
        mode = Glyphs::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = Glyphs::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = Glyphs::COLOR_BY_VECTOR_MAGNITUDE;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'ccolor'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsColorMode(name, mode);
    } else {
        g_renderer->setGlyphsColorMode("all", mode);
    }
    return TCL_OK;
}

static int
GlyphsDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGlyphs(name);
    } else {
        g_renderer->deleteGlyphs("all");
    }
    return TCL_OK;
}

static int
GlyphsEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsEdgeVisibility(name, state);
    } else {
        g_renderer->setGlyphsEdgeVisibility("all", state);
    }
    return TCL_OK;
}

static int
GlyphsLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsLighting(name, state);
    } else {
        g_renderer->setGlyphsLighting("all", state);
    }
    return TCL_OK;
}

static int
GlyphsLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGlyphsEdgeColor(name, color);
    } else {
        g_renderer->setGlyphsEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
GlyphsLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsEdgeWidth(name, width);
    } else {
        g_renderer->setGlyphsEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
GlyphsNormalizeScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    bool normalize;
    if (GetBooleanFromObj(interp, objv[2], &normalize) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsNormalizeScale(name, normalize);
    } else {
        g_renderer->setGlyphsNormalizeScale("all", normalize);
    }
    return TCL_OK;
}

static int
GlyphsOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsOpacity(name, opacity);
    } else {
        g_renderer->setGlyphsOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
GlyphsOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setGlyphsOrientation(name, quat);
    } else {
        g_renderer->setGlyphsOrientation("all", quat);
    }
    return TCL_OK;
}

static int
GlyphsPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGlyphsPosition(name, pos);
    } else {
        g_renderer->setGlyphsPosition("all", pos);
    }
    return TCL_OK;
}

static int
GlyphsScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGlyphsScale(name, scale);
    } else {
        g_renderer->setGlyphsScale("all", scale);
    }
    return TCL_OK;
}

static int
GlyphsScaleFactorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double scale;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsScaleFactor(name, scale);
    } else {
        g_renderer->setGlyphsScaleFactor("all", scale);
    }
    return TCL_OK;
}

static int
GlyphsScalingModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    Glyphs::ScalingMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = Glyphs::SCALE_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = Glyphs::SCALE_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vcomp") == 0) {
        mode = Glyphs::SCALE_BY_VECTOR_COMPONENTS;
    } else if (str[0] == 'o' && strcmp(str, "off") == 0) {
        mode = Glyphs::SCALING_OFF;
    } else {
        Tcl_AppendResult(interp, "bad scaling mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vcomp', 'off'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsScalingMode(name, mode);
    } else {
        g_renderer->setGlyphsScalingMode("all", mode);
    }
    return TCL_OK;
}

static int
GlyphsShapeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Glyphs::GlyphShape shape;

    const char *shapeOpt = Tcl_GetString(objv[2]);
    if (shapeOpt[0] == 'a' && strcmp(shapeOpt, "arrow") == 0) {
        shape = Glyphs::ARROW;
    } else if (shapeOpt[0] == 'c' && strcmp(shapeOpt, "cone") == 0) {
        shape = Glyphs::CONE;
    } else if (shapeOpt[0] == 'c' && strcmp(shapeOpt, "cube") == 0) {
        shape = Glyphs::CUBE;
    } else if (shapeOpt[0] == 'c' && strcmp(shapeOpt, "cylinder") == 0) {
        shape = Glyphs::CYLINDER;
    } else if (shapeOpt[0] == 'd' && strcmp(shapeOpt, "dodecahedron") == 0) {
        shape = Glyphs::DODECAHEDRON;
    } else if (shapeOpt[0] == 'i' && strcmp(shapeOpt, "icosahedron") == 0) {
        shape = Glyphs::ICOSAHEDRON;
    } else if (shapeOpt[0] == 'l' && strcmp(shapeOpt, "line") == 0) {
        shape = Glyphs::LINE;
    } else if (shapeOpt[0] == 'o' && strcmp(shapeOpt, "octahedron") == 0) {
        shape = Glyphs::OCTAHEDRON;
    } else if (shapeOpt[0] == 's' && strcmp(shapeOpt, "sphere") == 0) {
        shape = Glyphs::SPHERE;
    } else if (shapeOpt[0] == 't' && strcmp(shapeOpt, "tetrahedron") == 0) {
        shape = Glyphs::TETRAHEDRON;
    } else {
        Tcl_AppendResult(interp, "bad shape option \"", shapeOpt,
                         "\": should be one of: 'arrow', 'cone', 'cube', 'cylinder', 'dodecahedron', 'icosahedron', 'line', 'octahedron', 'sphere', 'tetrahedron'", (char*)NULL);
        return TCL_ERROR;
    }

    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsShape(name, shape);
    } else {
        g_renderer->setGlyphsShape("all", shape);
    }
    return TCL_OK;
}

static int
GlyphsVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsVisibility(name, state);
    } else {
        g_renderer->setGlyphsVisibility("all", state);
    }
    return TCL_OK;
}

static int
GlyphsWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsWireframe(name, state);
    } else {
        g_renderer->setGlyphsWireframe("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec glyphsOps[] = {
    {"add",       1, GlyphsAddOp, 3, 4, "shape ?dataSetNme?"},
    {"ccolor",    2, GlyphsColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  7, GlyphsColorMapOp, 3, 4, "colorMapName ?dataSetNme?"},
    {"colormode", 7, GlyphsColorModeOp, 3, 4, "mode ?dataSetNme?"},
    {"delete",    1, GlyphsDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, GlyphsEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"gscale",    1, GlyphsScaleFactorOp, 3, 4, "scaleFactor ?dataSetName?"},
    {"lighting",  3, GlyphsLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, GlyphsLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, GlyphsLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"normscale", 1, GlyphsNormalizeScaleOp, 3, 4, "bool ?dataSetName?"},
    {"opacity",   2, GlyphsOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",    2, GlyphsOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       1, GlyphsPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",     2, GlyphsScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"shape",     2, GlyphsShapeOp, 3, 4, "shapeVal ?dataSetName?"},
    {"smode",     2, GlyphsScalingModeOp, 3, 4, "mode ?dataSetNme?"},
    {"visible",   1, GlyphsVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe", 1, GlyphsWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nGlyphsOps = NumCmdSpecs(glyphsOps);

static int
GlyphsCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nGlyphsOps, glyphsOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
HeightMapAddContourListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    std::vector<double> contourList;
    double heightScale;

    int clistc;
    Tcl_Obj **clistv;

    if (Tcl_ListObjGetElements(interp, objv[3], &clistc, &clistv) != TCL_OK) {
        return TCL_ERROR;
    }

    for (int i = 0; i < clistc; i++) {
        double val;
        if (Tcl_GetDoubleFromObj(interp, clistv[i], &val) != TCL_OK) {
            return TCL_ERROR;
        }
        contourList.push_back(val);
    }

    if (Tcl_GetDoubleFromObj(interp, objv[4], &heightScale) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        if (!g_renderer->addHeightMap(name, contourList, heightScale)) {
            Tcl_AppendResult(interp, "Failed to create heightmap", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addHeightMap("all", contourList, heightScale)) {
            Tcl_AppendResult(interp, "Failed to create heightmap for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
HeightMapAddNumContoursOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    int numContours;
    double heightScale;
    if (Tcl_GetIntFromObj(interp, objv[3], &numContours) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[4], &heightScale) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        if (!g_renderer->addHeightMap(name, numContours, heightScale)) {
            Tcl_AppendResult(interp, "Failed to create heightmap", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addHeightMap("all", numContours, heightScale)) {
            Tcl_AppendResult(interp, "Failed to create heightmap for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static Rappture::CmdSpec heightmapAddOps[] = {
    {"contourlist", 1, HeightMapAddContourListOp, 5, 6, "contourList heightscale ?dataSetName?"},
    {"numcontours", 1, HeightMapAddNumContoursOp, 5, 6, "numContours heightscale ?dataSetName?"}
};
static int nHeightmapAddOps = NumCmdSpecs(heightmapAddOps);

static int
HeightMapAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nHeightmapAddOps, heightmapAddOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
HeightMapColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapColorMap(dataSetName, colorMapName);
    } else {
        g_renderer->setHeightMapColorMap("all", colorMapName);
    }
    return TCL_OK;
}

static int
HeightMapContourLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                            Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setHeightMapContourEdgeColor(name, color);
    } else {
        g_renderer->setHeightMapContourEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
HeightMapContourLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                            Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapContourEdgeWidth(name, width);
    } else {
        g_renderer->setHeightMapContourEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
HeightMapContourListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    std::vector<double> contourList;

    int clistc;
    Tcl_Obj **clistv;

    if (Tcl_ListObjGetElements(interp, objv[2], &clistc, &clistv) != TCL_OK) {
        return TCL_ERROR;
    }

    for (int i = 0; i < clistc; i++) {
        double val;
        if (Tcl_GetDoubleFromObj(interp, clistv[i], &val) != TCL_OK) {
            return TCL_ERROR;
        }
        contourList.push_back(val);
    }

    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapContourList(name, contourList);
    } else {
        g_renderer->setHeightMapContourList("all", contourList);
    }
    return TCL_OK;
}

static int
HeightMapContourLineVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapContourLineVisibility(name, state);
    } else {
        g_renderer->setHeightMapContourLineVisibility("all", state);
    }
    return TCL_OK;
}

static int
HeightMapContourSurfaceVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapContourSurfaceVisibility(name, state);
    } else {
        g_renderer->setHeightMapContourSurfaceVisibility("all", state);
    }
    return TCL_OK;
}

static int
HeightMapDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteHeightMap(name);
    } else {
        g_renderer->deleteHeightMap("all");
    }
    return TCL_OK;
}

static int
HeightMapEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapEdgeVisibility(name, state);
    } else {
        g_renderer->setHeightMapEdgeVisibility("all", state);
    }
    return TCL_OK;
}

static int
HeightMapHeightScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double scale;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapHeightScale(name, scale);
    } else {
        g_renderer->setHeightMapHeightScale("all", scale);
    }
    return TCL_OK;
}

static int
HeightMapLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapLighting(name, state);
    } else {
        g_renderer->setHeightMapLighting("all", state);
    }
    return TCL_OK;
}

static int
HeightMapLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setHeightMapEdgeColor(name, color);
    } else {
        g_renderer->setHeightMapEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
HeightMapLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapEdgeWidth(name, width);
    } else {
        g_renderer->setHeightMapEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
HeightMapNumContoursOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    int numContours;

    if (Tcl_GetIntFromObj(interp, objv[2], &numContours) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapNumContours(name, numContours);
    } else {
        g_renderer->setHeightMapNumContours("all", numContours);
    }
    return TCL_OK;
}

static int
HeightMapOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapOpacity(name, opacity);
    } else {
        g_renderer->setHeightMapOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
HeightMapOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setHeightMapOrientation(name, quat);
    } else {
        g_renderer->setHeightMapOrientation("all", quat);
    }
    return TCL_OK;
}

static int
HeightMapPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setHeightMapPosition(name, pos);
    } else {
        g_renderer->setHeightMapPosition("all", pos);
    }
    return TCL_OK;
}

static int
HeightMapScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setHeightMapScale(name, scale);
    } else {
        g_renderer->setHeightMapScale("all", scale);
    }
    return TCL_OK;
}

static int
HeightMapVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapVisibility(name, state);
    } else {
        g_renderer->setHeightMapVisibility("all", state);
    }
    return TCL_OK;
}

static int
HeightMapVolumeSliceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double ratio;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &ratio) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    HeightMap::Axis axis;
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        axis = HeightMap::X_AXIS;
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        axis = HeightMap::Y_AXIS;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        axis = HeightMap::Z_AXIS;
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName ratio", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setHeightMapVolumeSlice(name, axis, ratio);
    } else {
        g_renderer->setHeightMapVolumeSlice("all", axis, ratio);
    }
    return TCL_OK;
}

static int
HeightMapWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapWireframe(name, state);
    } else {
        g_renderer->setHeightMapWireframe("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec heightmapOps[] = {
    {"add",          1, HeightMapAddOp, 5, 6, "oper value ?dataSetName?"},
    {"colormap",     2, HeightMapColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"contourlist",  2, HeightMapContourListOp, 3, 4, "contourList ?dataSetName?"},
    {"delete",       1, HeightMapDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, HeightMapEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"heightscale",  1, HeightMapHeightScaleOp, 3, 4, "value ?dataSetName?"},
    {"isolinecolor", 8, HeightMapContourLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"isolines",     8, HeightMapContourLineVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"isolinewidth", 8, HeightMapContourLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"lighting",     3, HeightMapLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, HeightMapLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, HeightMapLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"numcontours",  1, HeightMapNumContoursOp, 3, 4, "numContours ?dataSetName?"},
    {"opacity",      2, HeightMapOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, HeightMapOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, HeightMapPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",        1, HeightMapScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"surface",      2, HeightMapContourSurfaceVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"visible",      2, HeightMapVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"volumeslice",  2, HeightMapVolumeSliceOp, 4, 5, "axis ratio ?dataSetName?"},
    {"wireframe",    1, HeightMapWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nHeightmapOps = NumCmdSpecs(heightmapOps);

static int
HeightMapCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nHeightmapOps, heightmapOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
LegendCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    if (objc < 7) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " colormapName legendType title width height numLabels ?dataSetName?\"", (char*)NULL);
        return TCL_ERROR;
    }
    const char *colorMapName = Tcl_GetString(objv[1]);
    const char *typeStr = Tcl_GetString(objv[2]);
    Renderer::LegendType type;
    if (typeStr[0] == 's' && strcmp(typeStr, "scalar") == 0) {
        type = Renderer::ACTIVE_SCALAR;
    } else if (typeStr[0] == 'v' && strcmp(typeStr, "vmag") == 0) {
        type = Renderer::ACTIVE_VECTOR_MAGNITUDE;
    } else if (typeStr[0] == 'v' && strcmp(typeStr, "vx") == 0) {
        type = Renderer::ACTIVE_VECTOR_X;
    } else if (typeStr[0] == 'v' && strcmp(typeStr, "vy") == 0) {
        type = Renderer::ACTIVE_VECTOR_Y;
    } else if (typeStr[0] == 'v' && strcmp(typeStr, "vz") == 0) {
        type = Renderer::ACTIVE_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "Bad legendType option \"",
                         typeStr, "\", should be one of 'scalar', 'vmag', 'vx', 'vy', 'vz'", (char*)NULL);
        return TCL_ERROR;
    }

    std::string title(Tcl_GetString(objv[3]));
    int width, height, numLabels;

    if (Tcl_GetIntFromObj(interp, objv[4], &width) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[5], &height) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[6], &numLabels) != TCL_OK) {
        return TCL_ERROR;
    }

    vtkSmartPointer<vtkUnsignedCharArray> imgData = 
        vtkSmartPointer<vtkUnsignedCharArray>::New();

    double range[2];

    if (objc == 8) {
        const char *dataSetName = Tcl_GetString(objv[7]);
        if (!g_renderer->renderColorMap(colorMapName, dataSetName, type, title,
                                        range, width, height, numLabels, imgData)) {
            Tcl_AppendResult(interp, "Color map \"",
                             colorMapName, "\" or dataset \"",
                             dataSetName, "\" was not found", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->renderColorMap(colorMapName, "all", type, title,
                                        range, width, height, numLabels, imgData)) {
            Tcl_AppendResult(interp, "Color map \"",
                             colorMapName, "\" was not found", (char*)NULL);
            return TCL_ERROR;
        }
    }

#ifdef DEBUG
    writeTGAFile("/tmp/legend.tga", imgData->GetPointer(0), width, height,
                 TARGA_BYTES_PER_PIXEL);
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nv>legend {%s} {%s} %g %g",
             colorMapName, title.c_str(), range[0], range[1]);
#ifdef RENDER_TARGA
    writeTGA(g_fdOut, cmd, imgData->GetPointer(0), width, height,
                 TARGA_BYTES_PER_PIXEL);
#else
    writePPM(g_fdOut, cmd, imgData->GetPointer(0), width, height);
#endif
#endif

    return TCL_OK;
}

static int
LICAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addLIC(name)) {
            Tcl_AppendResult(interp, "Failed to create lic", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addLIC("all")) {
            Tcl_AppendResult(interp, "Failed to create lic for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
LICColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setLICColorMap(dataSetName, colorMapName);
    } else {
        g_renderer->setLICColorMap("all", colorMapName);
    }
    return TCL_OK;
}

static int
LICDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteLIC(name);
    } else {
        g_renderer->deleteLIC("all");
    }
    return TCL_OK;
}

static int
LICEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setLICEdgeVisibility(name, state);
    } else {
        g_renderer->setLICEdgeVisibility("all", state);
    }
    return TCL_OK;
}

static int
LICLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setLICLighting(name, state);
    } else {
        g_renderer->setLICLighting("all", state);
    }
    return TCL_OK;
}

static int
LICLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setLICEdgeColor(name, color);
    } else {
        g_renderer->setLICEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
LICLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setLICEdgeWidth(name, width);
    } else {
        g_renderer->setLICEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
LICOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setLICOpacity(name, opacity);
    } else {
        g_renderer->setLICOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
LICOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setLICOrientation(name, quat);
    } else {
        g_renderer->setLICOrientation("all", quat);
    }
    return TCL_OK;
}

static int
LICPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setLICPosition(name, pos);
    } else {
        g_renderer->setLICPosition("all", pos);
    }
    return TCL_OK;
}

static int
LICScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setLICScale(name, scale);
    } else {
        g_renderer->setLICScale("all", scale);
    }
    return TCL_OK;
}

static int
LICVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setLICVisibility(name, state);
    } else {
        g_renderer->setLICVisibility("all", state);
    }
    return TCL_OK;
}

static int
LICVolumeSliceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double ratio;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &ratio) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    LIC::Axis axis;
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        axis = LIC::X_AXIS;
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        axis = LIC::Y_AXIS;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        axis = LIC::Z_AXIS;
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName ratio", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setLICVolumeSlice(name, axis, ratio);
    } else {
        g_renderer->setLICVolumeSlice("all", axis, ratio);
    }
    return TCL_OK;
}

static Rappture::CmdSpec licOps[] = {
    {"add",         1, LICAddOp, 2, 3, "?dataSetName?"},
    {"colormap",    1, LICColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",      1, LICDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",       1, LICEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",    3, LICLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",   5, LICLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",   5, LICLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",     2, LICOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",      2, LICOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",         1, LICPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",       1, LICScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",     2, LICVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"volumeslice", 2, LICVolumeSliceOp, 4, 5, "axis ratio ?dataSetName?"}
};
static int nLICOps = NumCmdSpecs(licOps);

static int
LICCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nLICOps, licOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
MoleculeAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addMolecule(name)) {
            Tcl_AppendResult(interp, "Failed to create molecule", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addMolecule("all")) {
            Tcl_AppendResult(interp, "Failed to create molecule for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
MoleculeAtomVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeAtomVisibility(name, state);
    } else {
        g_renderer->setMoleculeAtomVisibility("all", state);
    }
    return TCL_OK;
}

static int
MoleculeAtomScalingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    Molecule::AtomScaling scaling;
    const char *scalingOpt = Tcl_GetString(objv[2]);
    if (scalingOpt[0] == 'v' && strcmp(scalingOpt, "van_der_waals") == 0) {
        scaling = Molecule::VAN_DER_WAALS_RADIUS;
    } else if (scalingOpt[0] == 'c' && strcmp(scalingOpt, "covalent") == 0) {
        scaling = Molecule::COVALENT_RADIUS;
    } else if (scalingOpt[0] == 'a' && strcmp(scalingOpt, "atomic") == 0) {
        scaling = Molecule::ATOMIC_RADIUS;
    } else if (scalingOpt[0] == 'n' && strcmp(scalingOpt, "none") == 0) {
        scaling = Molecule::NO_ATOM_SCALING;
    } else {
        Tcl_AppendResult(interp, "bad atomscale option \"", scalingOpt,
                         "\": should be van_der_waals, covalent, atomic, or none", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeAtomScaling(name, scaling);
    } else {
        g_renderer->setMoleculeAtomScaling("all", scaling);
    }
    return TCL_OK;
}

static int
MoleculeBondVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeBondVisibility(name, state);
    } else {
        g_renderer->setMoleculeBondVisibility("all", state);
    }
    return TCL_OK;
}

static int
MoleculeColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeColorMap(dataSetName, colorMapName);
    } else {
        g_renderer->setMoleculeColorMap("all", colorMapName);
    }
    return TCL_OK;
}

static int
MoleculeDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteMolecule(name);
    } else {
        g_renderer->deleteMolecule("all");
    }
    return TCL_OK;
}

static int
MoleculeEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeEdgeVisibility(name, state);
    } else {
        g_renderer->setMoleculeEdgeVisibility("all", state);
    }
    return TCL_OK;
}

static int
MoleculeLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeLighting(name, state);
    } else {
        g_renderer->setMoleculeLighting("all", state);
    }
    return TCL_OK;
}

static int
MoleculeLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setMoleculeEdgeColor(name, color);
    } else {
        g_renderer->setMoleculeEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
MoleculeLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeEdgeWidth(name, width);
    } else {
        g_renderer->setMoleculeEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
MoleculeOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeOpacity(name, opacity);
    } else {
        g_renderer->setMoleculeOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
MoleculeOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setMoleculeOrientation(name, quat);
    } else {
        g_renderer->setMoleculeOrientation("all", quat);
    }
    return TCL_OK;
}

static int
MoleculePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setMoleculePosition(name, pos);
    } else {
        g_renderer->setMoleculePosition("all", pos);
    }
    return TCL_OK;
}

static int
MoleculeScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setMoleculeScale(name, scale);
    } else {
        g_renderer->setMoleculeScale("all", scale);
    }
    return TCL_OK;
}

static int
MoleculeVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeVisibility(name, state);
    } else {
        g_renderer->setMoleculeVisibility("all", state);
    }
    return TCL_OK;
}

static int
MoleculeWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeWireframe(name, state);
    } else {
        g_renderer->setMoleculeWireframe("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec moleculeOps[] = {
    {"add",        2, MoleculeAddOp, 2, 3, "?dataSetName?"},
    {"atoms",      2, MoleculeAtomVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"bonds",      2, MoleculeBondVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"colormap",   1, MoleculeColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",     1, MoleculeDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",      1, MoleculeEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",   3, MoleculeLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",  5, MoleculeLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",  5, MoleculeLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",    2, MoleculeOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",     2, MoleculeOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",        1, MoleculePositionOp, 5, 6, "x y z ?dataSetName?"},
    {"rscale",     1, MoleculeAtomScalingOp, 3, 4, "scaling ?dataSetName?"},
    {"scale",      1, MoleculeScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",    1, MoleculeVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe",  1, MoleculeWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nMoleculeOps = NumCmdSpecs(moleculeOps);

static int
MoleculeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nMoleculeOps, moleculeOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
PolyDataAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addPolyData(name)) {
            Tcl_AppendResult(interp, "Failed to create polydata", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addPolyData("all")) {
            Tcl_AppendResult(interp, "Failed to create polydata for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
PolyDataDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deletePolyData(name);
    } else {
        g_renderer->deletePolyData("all");
    }
    return TCL_OK;
}

static int
PolyDataColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setPolyDataColor(name, color);
    } else {
        g_renderer->setPolyDataColor("all", color);
    }
    return TCL_OK;
}

static int
PolyDataEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPolyDataEdgeVisibility(name, state);
    } else {
        g_renderer->setPolyDataEdgeVisibility("all", state);
    }
    return TCL_OK;
}

static int
PolyDataLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPolyDataLighting(name, state);
    } else {
        g_renderer->setPolyDataLighting("all", state);
    }
    return TCL_OK;
}

static int
PolyDataLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setPolyDataEdgeColor(name, color);
    } else {
        g_renderer->setPolyDataEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
PolyDataLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPolyDataEdgeWidth(name, width);
    } else {
        g_renderer->setPolyDataEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
PolyDataOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPolyDataOpacity(name, opacity);
    } else {
        g_renderer->setPolyDataOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
PolyDataOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setPolyDataOrientation(name, quat);
    } else {
        g_renderer->setPolyDataOrientation("all", quat);
    }
    return TCL_OK;
}

static int
PolyDataPointSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    float size;
    if (GetFloatFromObj(interp, objv[2], &size) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPolyDataPointSize(name, size);
    } else {
        g_renderer->setPolyDataPointSize("all", size);
    }
    return TCL_OK;
}

static int
PolyDataPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setPolyDataPosition(name, pos);
    } else {
        g_renderer->setPolyDataPosition("all", pos);
    }
    return TCL_OK;
}

static int
PolyDataScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setPolyDataScale(name, scale);
    } else {
        g_renderer->setPolyDataScale("all", scale);
    }
    return TCL_OK;
}

static int
PolyDataVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPolyDataVisibility(name, state);
    } else {
        g_renderer->setPolyDataVisibility("all", state);
    }
    return TCL_OK;
}

static int
PolyDataWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPolyDataWireframe(name, state);
    } else {
        g_renderer->setPolyDataWireframe("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec polyDataOps[] = {
    {"add",       1, PolyDataAddOp, 2, 3, "?dataSetName?"},
    {"color",     1, PolyDataColorOp, 5, 6, "r g b ?dataSetName?"},
    {"delete",    1, PolyDataDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, PolyDataEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",  3, PolyDataLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, PolyDataLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, PolyDataLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   2, PolyDataOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",    2, PolyDataOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       2, PolyDataPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"ptsize",    2, PolyDataPointSizeOp, 3, 4, "size ?dataSetName?"},
    {"scale",     1, PolyDataScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",   1, PolyDataVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe", 1, PolyDataWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nPolyDataOps = NumCmdSpecs(polyDataOps);

static int
PolyDataCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nPolyDataOps, polyDataOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
PseudoColorAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addPseudoColor(name)) {
            Tcl_AppendResult(interp, "Failed to create pseudocolor", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addPseudoColor("all")) {
            Tcl_AppendResult(interp, "Failed to create pseudocolor for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
PseudoColorColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setPseudoColorColor(name, color);
    } else {
        g_renderer->setPseudoColorColor("all", color);
    }
    return TCL_OK;
}

static int
PseudoColorColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorColorMap(dataSetName, colorMapName);
    } else {
        g_renderer->setPseudoColorColorMap("all", colorMapName);
    }
    return TCL_OK;
}

static int
PseudoColorColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    PseudoColor::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'c' && strcmp(str, "ccolor") == 0) {
        mode = PseudoColor::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = PseudoColor::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = PseudoColor::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = PseudoColor::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = PseudoColor::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = PseudoColor::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorColorMode(name, mode);
    } else {
        g_renderer->setPseudoColorColorMode("all", mode);
    }
    return TCL_OK;
}

static int
PseudoColorDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deletePseudoColor(name);
    } else {
        g_renderer->deletePseudoColor("all");
    }
    return TCL_OK;
}

static int
PseudoColorEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                            Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorEdgeVisibility(name, state);
    } else {
        g_renderer->setPseudoColorEdgeVisibility("all", state);
    }
    return TCL_OK;
}

static int
PseudoColorLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorLighting(name, state);
    } else {
        g_renderer->setPseudoColorLighting("all", state);
    }
    return TCL_OK;
}

static int
PseudoColorLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setPseudoColorEdgeColor(name, color);
    } else {
        g_renderer->setPseudoColorEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
PseudoColorLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorEdgeWidth(name, width);
    } else {
        g_renderer->setPseudoColorEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
PseudoColorOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorOpacity(name, opacity);
    } else {
        g_renderer->setPseudoColorOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
PseudoColorOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setPseudoColorOrientation(name, quat);
    } else {
        g_renderer->setPseudoColorOrientation("all", quat);
    }
    return TCL_OK;
}

static int
PseudoColorPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setPseudoColorPosition(name, pos);
    } else {
        g_renderer->setPseudoColorPosition("all", pos);
    }
    return TCL_OK;
}

static int
PseudoColorScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setPseudoColorScale(name, scale);
    } else {
        g_renderer->setPseudoColorScale("all", scale);
    }
    return TCL_OK;
}

static int
PseudoColorVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorVisibility(name, state);
    } else {
        g_renderer->setPseudoColorVisibility("all", state);
    }
    return TCL_OK;
}

static int
PseudoColorWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorWireframe(name, state);
    } else {
        g_renderer->setPseudoColorWireframe("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec pseudoColorOps[] = {
    {"add",       1, PseudoColorAddOp, 2, 3, "?dataSetName?"},
    {"ccolor",    2, PseudoColorColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  7, PseudoColorColorMapOp, 3, 4, "colorMapName ?dataSetNme?"},
    {"colormode", 7, PseudoColorColorModeOp, 3, 4, "mode ?dataSetNme?"},
    {"delete",    1, PseudoColorDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, PseudoColorEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",  3, PseudoColorLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, PseudoColorLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, PseudoColorLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   2, PseudoColorOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",    2, PseudoColorOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       1, PseudoColorPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",     1, PseudoColorScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",   1, PseudoColorVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe", 1, PseudoColorWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nPseudoColorOps = NumCmdSpecs(pseudoColorOps);

static int
PseudoColorCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nPseudoColorOps, pseudoColorOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
RendererClipPlaneOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    Renderer::Axis axis;
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        axis = Renderer::X_AXIS;
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        axis = Renderer::Y_AXIS;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        axis = Renderer::Z_AXIS;
    } else {
        Tcl_AppendResult(interp, "bad clipplane option \"", string,
                         "\": should be axisName ratio direction", (char*)NULL);
        return TCL_ERROR;
    }
    double ratio;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &ratio) != TCL_OK) {
        return TCL_ERROR;
    }
    int direction;
    if (Tcl_GetIntFromObj(interp, objv[4], &direction) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setClipPlane(axis, ratio, direction);
    return TCL_OK;
}

static int
RendererDepthPeelingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setUseDepthPeeling(state);
    return TCL_OK;
}

static int
RendererTwoSidedLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                           Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setUseTwoSidedLighting(state);
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
    {"clipplane",  1, RendererClipPlaneOp, 5, 5, "axis ratio direction"},
    {"depthpeel",  1, RendererDepthPeelingOp, 3, 3, "bool"},
    {"light2side", 1, RendererTwoSidedLightingOp, 3, 3, "bool"},
    {"render",     1, RendererRenderOp, 2, 2, ""}
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

static int
StreamlinesAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addStreamlines(name)) {
            Tcl_AppendResult(interp, "Failed to create streamlines", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addStreamlines("all")) {
            Tcl_AppendResult(interp, "Failed to create streamlines for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
StreamlinesColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setStreamlinesColor(name, color);
    } else {
        g_renderer->setStreamlinesColor("all", color);
    }
    return TCL_OK;
}

static int
StreamlinesColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesColorMap(dataSetName, colorMapName);
    } else {
        g_renderer->setStreamlinesColorMap("all", colorMapName);
    }
    return TCL_OK;
}

static int
StreamlinesColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    Streamlines::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = Streamlines::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = Streamlines::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = Streamlines::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = Streamlines::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = Streamlines::COLOR_BY_VECTOR_Z;
    } else if (str[0] == 'c' && strcmp(str, "ccolor") == 0) {
        mode = Streamlines::COLOR_CONSTANT;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesColorMode(dataSetName, mode);
    } else {
        g_renderer->setStreamlinesColorMode("all", mode);
    }
    return TCL_OK;
}

static int
StreamlinesDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteStreamlines(name);
    } else {
        g_renderer->deleteStreamlines("all");
    }
    return TCL_OK;
}

static int
StreamlinesEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                            Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesEdgeVisibility(name, state);
    } else {
        g_renderer->setStreamlinesEdgeVisibility("all", state);
    }
    return TCL_OK;
}

static int
StreamlinesLengthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double length;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &length) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesLength(name, length);
    } else {
        g_renderer->setStreamlinesLength("all", length);
    }
    return TCL_OK;
}

static int
StreamlinesLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesLighting(name, state);
    } else {
        g_renderer->setStreamlinesLighting("all", state);
    }
    return TCL_OK;
}

static int
StreamlinesLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setStreamlinesEdgeColor(name, color);
    } else {
        g_renderer->setStreamlinesEdgeColor("all", color);
    }
    return TCL_OK;
}

static int
StreamlinesLinesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->setStreamlinesTypeToLines(name);
    } else {
        g_renderer->setStreamlinesTypeToLines("all");
    }
    return TCL_OK;
}

static int
StreamlinesLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesEdgeWidth(name, width);
    } else {
        g_renderer->setStreamlinesEdgeWidth("all", width);
    }
    return TCL_OK;
}

static int
StreamlinesOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesOpacity(name, opacity);
    } else {
        g_renderer->setStreamlinesOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
StreamlinesOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setStreamlinesOrientation(name, quat);
    } else {
        g_renderer->setStreamlinesOrientation("all", quat);
    }
    return TCL_OK;
}

static int
StreamlinesPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setStreamlinesPosition(name, pos);
    } else {
        g_renderer->setStreamlinesPosition("all", pos);
    }
    return TCL_OK;
}

static int
StreamlinesScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setStreamlinesScale(name, scale);
    } else {
        g_renderer->setStreamlinesScale("all", scale);
    }
    return TCL_OK;
}

static int
StreamlinesRibbonsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    double width, angle;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &angle) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setStreamlinesTypeToRibbons(name, width, angle);
    } else {
        g_renderer->setStreamlinesTypeToRibbons("all", width, angle);
    }
    return TCL_OK;
}

static int
StreamlinesSeedColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setStreamlinesSeedColor(name, color);
    } else {
        g_renderer->setStreamlinesSeedColor("all", color);
    }
    return TCL_OK;
}

static int
StreamlinesSeedDiskOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double center[3], normal[3], radius, innerRadius;
    int numPoints;
    for (int i = 0; i < 3; i++) {
        if (Tcl_GetDoubleFromObj(interp, objv[3+i], &center[i]) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetDoubleFromObj(interp, objv[6+i], &normal[i]) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (Tcl_GetDoubleFromObj(interp, objv[9], &radius) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[10], &innerRadius) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[11], &numPoints) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 13) {
        const char *name = Tcl_GetString(objv[12]);
        g_renderer->setStreamlinesSeedToDisk(name, center, normal, radius, innerRadius, numPoints);
    } else {
        g_renderer->setStreamlinesSeedToDisk("all", center, normal, radius, innerRadius, numPoints);
    }
    return TCL_OK;
}

static int
StreamlinesSeedFilledMeshOp(ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *const *objv)
{
    int numPoints = 0;
    if (Tcl_GetIntFromObj(interp, objv[3], &numPoints) != TCL_OK ||
        numPoints < 1) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[4]);
    char c = string[0];
    if ((c != 'd') || (strcmp(string, "data") != 0)) {
        Tcl_AppendResult(interp, "bad streamlines seed fmesh option \"",
                         string, "\": should be data", (char*)NULL);
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[5]);
    c = string[0];
    if ((c != 'f') || (strcmp(string, "follows") != 0)) {
        Tcl_AppendResult(interp, "bad streamlines seed fmesh option \"",
                         string, "\": should be follows", (char*)NULL);
        return TCL_ERROR;
    }
    int nbytes = 0;
    if (Tcl_GetIntFromObj(interp, objv[6], &nbytes) != TCL_OK ||
        nbytes < 0) {
        return TCL_ERROR;
    }
    char *data = (char *)malloc(nbytes);
    size_t bytesRead = SocketRead(data, nbytes);
    if (bytesRead < 0) {
        free(data);
        return TCL_ERROR;
    }
    if (objc == 8) {
        const char *name = Tcl_GetString(objv[7]);
        if (!g_renderer->setStreamlinesSeedToFilledMesh(name, data, nbytes,
                                                        numPoints)) {
            free(data);
            Tcl_AppendResult(interp, "Couldn't read mesh data or streamlines not found", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->setStreamlinesSeedToFilledMesh("all", data, nbytes,
                                                        numPoints)) {
            free(data);
            Tcl_AppendResult(interp, "Couldn't read mesh data or streamlines not found", (char*)NULL);
            return TCL_ERROR;
        }
    }
    free(data);
    return TCL_OK;
}

static int
StreamlinesSeedMeshPointsOp(ClientData clientData, Tcl_Interp *interp,
                            int objc, Tcl_Obj *const *objv)
{
    const char *string = Tcl_GetString(objv[3]);
    char c = string[0];
    if ((c != 'd') || (strcmp(string, "data") != 0)) {
        Tcl_AppendResult(interp, "bad streamlines seed fmesh option \"",
                         string, "\": should be data", (char*)NULL);
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[4]);
    c = string[0];
    if ((c != 'f') || (strcmp(string, "follows") != 0)) {
        Tcl_AppendResult(interp, "bad streamlines seed fmesh option \"",
                         string, "\": should be follows", (char*)NULL);
        return TCL_ERROR;
    }
    int nbytes = 0;
    if (Tcl_GetIntFromObj(interp, objv[5], &nbytes) != TCL_OK ||
        nbytes < 0) {
        return TCL_ERROR;
    }
    char *data = (char *)malloc(nbytes);
    size_t bytesRead = SocketRead(data, nbytes);
    if (bytesRead < 0) {
        free(data);
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        if (!g_renderer->setStreamlinesSeedToMeshPoints(name, data, nbytes)) {
            free(data);
            Tcl_AppendResult(interp, "Couldn't read mesh data or streamlines not found", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->setStreamlinesSeedToMeshPoints("all", data, nbytes)) {
            free(data);
            Tcl_AppendResult(interp, "Couldn't read mesh data or streamlines not found", (char*)NULL);
            return TCL_ERROR;
        }
    }
    free(data);
    return TCL_OK;
}

static int
StreamlinesSeedPointsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesSeedToMeshPoints(name);
    } else {
        g_renderer->setStreamlinesSeedToMeshPoints("all");
    }
    return TCL_OK;
}

static int
StreamlinesSeedPolygonOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    double center[3], normal[3], angle, radius;
    int numSides;
    for (int i = 0; i < 3; i++) {
        if (Tcl_GetDoubleFromObj(interp, objv[3+i], &center[i]) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetDoubleFromObj(interp, objv[6+i], &normal[i]) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (Tcl_GetDoubleFromObj(interp, objv[9], &angle) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[10], &radius) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[11], &numSides) != TCL_OK) {
        return TCL_ERROR;
    }
    if (numSides < 3) {
        Tcl_AppendResult(interp, "numSides must be 3 or greater", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 13) {
        const char *name = Tcl_GetString(objv[12]);
        g_renderer->setStreamlinesSeedToPolygon(name, center, normal, angle, radius, numSides);
    } else {
        g_renderer->setStreamlinesSeedToPolygon("all", center, normal, angle, radius, numSides);
    }
    return TCL_OK;
}

static int
StreamlinesSeedFilledPolygonOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                               Tcl_Obj *const *objv)
{
    double center[3], normal[3], angle, radius;
    int numSides, numPoints;
    for (int i = 0; i < 3; i++) {
        if (Tcl_GetDoubleFromObj(interp, objv[3+i], &center[i]) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetDoubleFromObj(interp, objv[6+i], &normal[i]) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (Tcl_GetDoubleFromObj(interp, objv[9], &angle) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[10], &radius) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[11], &numSides) != TCL_OK) {
        return TCL_ERROR;
    }
    if (numSides < 3) {
        Tcl_AppendResult(interp, "numSides must be 3 or greater", (char*)NULL);
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[12], &numPoints) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 14) {
        const char *name = Tcl_GetString(objv[13]);
        g_renderer->setStreamlinesSeedToFilledPolygon(name, center, normal, angle,
                                                      radius, numSides, numPoints);
    } else {
        g_renderer->setStreamlinesSeedToFilledPolygon("all", center, normal, angle,
                                                      radius, numSides, numPoints);
    }
    return TCL_OK;
}

static int
StreamlinesSeedRakeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double start[3], end[3];
    int numPoints;
    for (int i = 0; i < 3; i++) {
        if (Tcl_GetDoubleFromObj(interp, objv[3+i], &start[i]) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetDoubleFromObj(interp, objv[6+i], &end[i]) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (Tcl_GetIntFromObj(interp, objv[9], &numPoints) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 11) {
        const char *name = Tcl_GetString(objv[10]);
        g_renderer->setStreamlinesSeedToRake(name, start, end, numPoints);
    } else {
        g_renderer->setStreamlinesSeedToRake("all", start, end, numPoints);
    }
    return TCL_OK;
}

static int
StreamlinesSeedRandomOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    int numPoints;
    if (Tcl_GetIntFromObj(interp, objv[3], &numPoints) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setStreamlinesSeedToFilledMesh(name, numPoints);
    } else {
        g_renderer->setStreamlinesSeedToFilledMesh("all", numPoints);
    }
    return TCL_OK;
}

static int
StreamlinesSeedVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setStreamlinesSeedVisibility(name, state);
    } else {
        g_renderer->setStreamlinesSeedVisibility("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec streamlinesSeedOps[] = {
    {"color",   1, StreamlinesSeedColorOp, 6, 7, "r g b ?dataSetName?"},
    {"disk",    1, StreamlinesSeedDiskOp, 12, 13, "centerX centerY centerZ normalX normalY normalZ radius innerRadius numPoints ?dataSetName?"},
    {"fmesh",   2, StreamlinesSeedFilledMeshOp, 7, 8, "numPoints data follows nbytes ?dataSetName?"},
    {"fpoly",   2, StreamlinesSeedFilledPolygonOp, 13, 14, "centerX centerY centerZ normalX normalY normalZ angle radius numSides numPoints ?dataSetName?"},
    {"mesh",    1, StreamlinesSeedMeshPointsOp, 6, 7, "data follows nbytes ?dataSetName?"},
    {"points",  3, StreamlinesSeedPointsOp, 3, 4, "?dataSetName?"},
    {"polygon", 3, StreamlinesSeedPolygonOp, 12, 13, "centerX centerY centerZ normalX normalY normalZ angle radius numSides ?dataSetName?"},
    {"rake",    3, StreamlinesSeedRakeOp, 10, 11, "startX startY startZ endX endY endZ numPoints ?dataSetName?"},
    {"random",  3, StreamlinesSeedRandomOp, 4, 5, "numPoints ?dataSetName?"},
    {"visible", 1, StreamlinesSeedVisibleOp, 4, 5, "bool ?dataSetName?"}
};
static int nStreamlinesSeedOps = NumCmdSpecs(streamlinesSeedOps);

static int
StreamlinesSeedOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nStreamlinesSeedOps, streamlinesSeedOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
StreamlinesTubesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    int numSides;
    double radius;
    if (Tcl_GetIntFromObj(interp, objv[2], &numSides) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &radius) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setStreamlinesTypeToTubes(name, numSides, radius);
    } else {
        g_renderer->setStreamlinesTypeToTubes("all", numSides, radius);
    }
    return TCL_OK;
}

static int
StreamlinesVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesVisibility(name, state);
    } else {
        g_renderer->setStreamlinesVisibility("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec streamlinesOps[] = {
    {"add",       1, StreamlinesAddOp, 2, 3, "?dataSetName?"},
    {"ccolor",    1, StreamlinesColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  7, StreamlinesColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode", 7, StreamlinesColorModeOp, 3, 4, "mode ?dataSetNme?"},
    {"delete",    1, StreamlinesDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, StreamlinesEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"length",    2, StreamlinesLengthOp, 3, 4, "length ?dataSetName?"},
    {"lighting",  3, StreamlinesLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, StreamlinesLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"lines",     5, StreamlinesLinesOp, 2, 3, "?dataSetName?"},
    {"linewidth", 5, StreamlinesLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   2, StreamlinesOpacityOp, 3, 4, "val ?dataSetName?"},
    {"orient",    2, StreamlinesOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       1, StreamlinesPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"ribbons",   1, StreamlinesRibbonsOp, 4, 5, "width angle ?dataSetName?"},
    {"scale",     2, StreamlinesScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"seed",      2, StreamlinesSeedOp, 3, 14, "op params... ?dataSetName?"},
    {"tubes",     1, StreamlinesTubesOp, 4, 5, "numSides radius ?dataSetName?"},
    {"visible",   1, StreamlinesVisibleOp, 3, 4, "bool ?dataSetName?"}
};
static int nStreamlinesOps = NumCmdSpecs(streamlinesOps);

static int
StreamlinesCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nStreamlinesOps, streamlinesOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
VolumeAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addVolume(name)) {
            Tcl_AppendResult(interp, "Failed to create volume", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addVolume("all")) {
            Tcl_AppendResult(interp, "Failed to create volume for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
VolumeColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setVolumeColorMap(dataSetName, colorMapName);
    } else {
        g_renderer->setVolumeColorMap("all", colorMapName);
    }
    return TCL_OK;
}

static int
VolumeDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteVolume(name);
    } else {
        g_renderer->deleteVolume("all");
    }
    return TCL_OK;
}

static int
VolumeLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setVolumeLighting(name, state);
    } else {
        g_renderer->setVolumeLighting("all", state);
    }
    return TCL_OK;
}

static int
VolumeOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setVolumeOpacity(name, opacity);
    } else {
        g_renderer->setVolumeOpacity("all", opacity);
    }
    return TCL_OK;
}

static int
VolumeOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double quat[4];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quat[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &quat[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &quat[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &quat[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setVolumeOrientation(name, quat);
    } else {
        g_renderer->setVolumeOrientation("all", quat);
    }
    return TCL_OK;
}

static int
VolumePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double pos[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &pos[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &pos[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &pos[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setVolumePosition(name, pos);
    } else {
        g_renderer->setVolumePosition("all", pos);
    }
    return TCL_OK;
}

static int
VolumeScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double scale[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &scale[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &scale[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setVolumeScale(name, scale);
    } else {
        g_renderer->setVolumeScale("all", scale);
    }
    return TCL_OK;
}

static int
VolumeShadingAmbientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double coeff;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &coeff) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setVolumeAmbient(name, coeff);
    } else {
        g_renderer->setVolumeAmbient("all", coeff);
    }
    return TCL_OK;
}

static int
VolumeShadingDiffuseOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double coeff;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &coeff) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setVolumeDiffuse(name, coeff);
    } else {
        g_renderer->setVolumeDiffuse("all", coeff);
    }
    return TCL_OK;
}

static int
VolumeShadingSpecularOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    double coeff, power;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &coeff) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &power) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setVolumeSpecular(name, coeff, power);
    } else {
        g_renderer->setVolumeSpecular("all", coeff, power);
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeShadingOps[] = {
    {"ambient",  1, VolumeShadingAmbientOp, 4, 5, "coeff ?dataSetName?"},
    {"diffuse",  1, VolumeShadingDiffuseOp, 4, 5, "coeff ?dataSetName?"},
    {"specular", 1, VolumeShadingSpecularOp, 5, 6, "coeff power ?dataSetName?"}
};
static int nVolumeShadingOps = NumCmdSpecs(volumeShadingOps);

static int
VolumeShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeShadingOps, volumeShadingOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
VolumeVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setVolumeVisibility(name, state);
    } else {
        g_renderer->setVolumeVisibility("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeOps[] = {
    {"add",      1, VolumeAddOp, 2, 3, "?dataSetName?"},
    {"colormap", 1, VolumeColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",   1, VolumeDeleteOp, 2, 3, "?dataSetName?"},
    {"lighting", 1, VolumeLightingOp, 3, 4, "bool ?dataSetName?"},
    {"opacity",  2, VolumeOpacityOp, 3, 4, "val ?dataSetName?"},
    {"orient",   2, VolumeOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",      1, VolumePositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",    2, VolumeScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"shading",  2, VolumeShadingOp, 4, 6, "oper val ?dataSetName?"},
    {"visible",  1, VolumeVisibleOp, 3, 4, "bool ?dataSetName?"}
};
static int nVolumeOps = NumCmdSpecs(volumeOps);

static int
VolumeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nVolumeOps, volumeOps,
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/**
 * \brief Execute commands from client in Tcl interpreter
 */
int
Rappture::VtkVis::processCommands(Tcl_Interp *interp, FILE *fin, FILE *fout)
{
    Tcl_DString cmdbuffer;
    Tcl_DStringInit(&cmdbuffer);

    int fdIn = fileno(fin);
    int fdOut = fileno(fout);
    int flags = fcntl(fdIn, F_GETFL, 0);
    fcntl(fdIn, F_SETFL, flags & ~O_NONBLOCK);

    int status = TCL_OK;
    int nCommands = 0;
    bool isComplete = false;
    while ((!feof(fin)) && (status == TCL_OK)) {
        while (!feof(fin)) {
            int c = fgetc(fin);
            if (c <= 0) {
                if (errno == EWOULDBLOCK) {
                    break;
                }
                if (feof(fin))
                    return 1;
                else
                    return c;
            }
            char ch = (char)c;
            Tcl_DStringAppend(&cmdbuffer, &ch, 1);
            if (ch == '\n') {
                isComplete = Tcl_CommandComplete(Tcl_DStringValue(&cmdbuffer));
                if (isComplete) {
                    break;
                }
            }
        }
        // no command? then we're done for now
        if (Tcl_DStringLength(&cmdbuffer) == 0) {
            break;
        }
        if (isComplete) {
            // back to original flags during command evaluation...
            fcntl(fdIn, F_SETFL, flags & ~O_NONBLOCK);
            TRACE("command: '%s'", Tcl_DStringValue(&cmdbuffer));
            status = ExecuteCommand(interp, &cmdbuffer);
            // non-blocking for next read -- we might not get anything
            fcntl(fdIn, F_SETFL, flags | O_NONBLOCK);
            isComplete = false;
	    nCommands++;
        }
    }
    fcntl(fdIn, F_SETFL, flags);

    if (status != TCL_OK) {
        const char *string;
        int nBytes;

        string = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
        TRACE("ERROR errorInfo=(%s)", string);

        nBytes = strlen(string);
        struct iovec iov[3];
        iov[0].iov_base = (char *)"VtkVis Server Error: ";
        iov[0].iov_len = strlen((char *)iov[0].iov_base);
        iov[1].iov_base = (char *)string;
        iov[1].iov_len = nBytes;
        iov[2].iov_base = (char *)"\n";
        iov[2].iov_len = strlen((char *)iov[2].iov_base);
        if (writev(fdOut, iov, 3) < 0) {
	    ERROR("write failed: %s", strerror(errno));
	}
        return 0;
    }

    return 1;
}

/**
 * \brief Create Tcl interpreter and add commands
 *
 * \return The initialized Tcl interpreter
 */
Tcl_Interp *
Rappture::VtkVis::initTcl()
{
    Tcl_Interp *interp;
    interp = Tcl_CreateInterp();

    Tcl_MakeSafe(interp);

    Tcl_CreateObjCommand(interp, "axis",        AxisCmd,        NULL, NULL);
    Tcl_CreateObjCommand(interp, "camera",      CameraCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "colormap",    ColorMapCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "contour2d",   Contour2DCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "contour3d",   Contour3DCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "dataset",     DataSetCmd,     NULL, NULL);
    Tcl_CreateObjCommand(interp, "glyphs",      GlyphsCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "heightmap",   HeightMapCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "legend",      LegendCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "lic",         LICCmd,         NULL, NULL);
    Tcl_CreateObjCommand(interp, "molecule",    MoleculeCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "polydata",    PolyDataCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "pseudocolor", PseudoColorCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "renderer",    RendererCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "screen",      ScreenCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "streamlines", StreamlinesCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "volume",      VolumeCmd,      NULL, NULL);
    return interp;
}

/**
 * \brief Delete Tcl commands and interpreter
 *
 */
void Rappture::VtkVis::exitTcl(Tcl_Interp *interp)
{

    Tcl_DeleteCommand(interp, "axis");
    Tcl_DeleteCommand(interp, "camera");
    Tcl_DeleteCommand(interp, "colormap");
    Tcl_DeleteCommand(interp, "contour2d");
    Tcl_DeleteCommand(interp, "contour3d");
    Tcl_DeleteCommand(interp, "dataset");
    Tcl_DeleteCommand(interp, "glyphs");
    Tcl_DeleteCommand(interp, "heightmap");
    Tcl_DeleteCommand(interp, "legend");
    Tcl_DeleteCommand(interp, "lic");
    Tcl_DeleteCommand(interp, "molecule");
    Tcl_DeleteCommand(interp, "polydata");
    Tcl_DeleteCommand(interp, "pseudocolor");
    Tcl_DeleteCommand(interp, "renderer");
    Tcl_DeleteCommand(interp, "screen");
    Tcl_DeleteCommand(interp, "streamlines");
    Tcl_DeleteCommand(interp, "volume");

    Tcl_DeleteInterp(interp);
}
