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

static int
ExecuteCommand(Tcl_Interp *interp, Tcl_DString *dsPtr) 
{
    int result;

    result = Tcl_Eval(interp, Tcl_DStringValue(dsPtr));
    Tcl_DStringSetLength(dsPtr, 0);

    return result;
}

static bool
GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, bool *boolPtr)
{
    int value;

    if (Tcl_GetBooleanFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    *boolPtr = (bool)value;
    return TCL_OK;
}

int
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
    snprintf(buf, sizeof(buf), "nv>camera set %.12e %.12e %.12e %.12e %.12e %.12e %.12e %.12e %.12e", 
             pos[0], pos[1], pos[2], focalPt[0], focalPt[1], focalPt[2], viewUp[0], viewUp[1], viewUp[2]);
    ssize_t bytesWritten;
    size_t len = strlen(buf);
    size_t ofs = 0;
    while ((bytesWritten = write(g_fdOut, buf + ofs, len - ofs)) > 0) {
        ofs += bytesWritten;
        if (ofs == len)
            break;
    }
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
    double x, y, width, height;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &width) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &height) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setCameraZoomRegion(x, y, width, height);
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
    {"ortho", 1, CameraOrthoOp, 6, 6, "x y width height"},
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
        g_renderer->addContour2D(name);
        g_renderer->setContour2DContourList(name, contourList);
    } else {
        g_renderer->addContour2D("all");
        g_renderer->setContour2DContourList("all", contourList);
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
        g_renderer->addContour2D(name);
        g_renderer->setContour2DContours(name, numContours);
    } else {
        g_renderer->addContour2D("all");
        g_renderer->setContour2DContours("all", numContours);
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
        g_renderer->setContour2DEdgeColor(name, color);
    } else {
        g_renderer->setContour2DEdgeColor("all", color);
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
    {"delete",    1, Contour2DDeleteOp, 2, 3, "?dataSetName?"},
    {"lighting",  3, Contour2DLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, Contour2DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, Contour2DLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   1, Contour2DOpacityOp, 3, 4, "value ?dataSetName?"},
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
        g_renderer->addContour3D(name);
        g_renderer->setContour3DContourList(name, contourList);
    } else {
        g_renderer->addContour3D("all");
        g_renderer->setContour3DContourList("all", contourList);
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
        g_renderer->addContour3D(name);
        g_renderer->setContour3DContours(name, numContours);
    } else {
        g_renderer->addContour3D("all");
        g_renderer->setContour3DContours("all", numContours);
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
    {"color",     6, Contour3DColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  6, Contour3DColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",    1, Contour3DDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, Contour3DEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",  3, Contour3DLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, Contour3DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, Contour3DLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   1, Contour3DOpacityOp, 3, 4, "value ?dataSetName?"},
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
#ifdef notdef
    size_t ofs = 0;
    ssize_t bytesRead = 0;
    while ((bytesRead = read(g_fdIn, data + ofs, nbytes - ofs)) > 0) {
        ofs += bytesRead;
        if (ofs == nbytes)
            break;
    }
    TRACE("bytesRead: %d", ofs);
    if (bytesRead < 0) {
        free(data);
        return TCL_ERROR;
    }
#else
    size_t bytesRead = fread(data, 1, nbytes, g_fIn);
    TRACE("bytesRead: %d '%c'", bytesRead, data[0]);
    if (bytesRead < (size_t)nbytes) {
        free(data);
        return TCL_ERROR;
    }
#endif
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
DataSetGetValuePixelOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[5]);
    int x, y;
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }
    double value = g_renderer->getDataValueAtPixel(name, x, y);
    char buf[128];
    snprintf(buf, sizeof(buf), "nv>dataset value pixel %d %d %.12e", x, y, value);
    ssize_t bytesWritten;
    size_t len = strlen(buf);
    size_t ofs = 0;
    while ((bytesWritten = write(g_fdOut, buf + ofs, len - ofs)) > 0) {
        ofs += bytesWritten;
        if (ofs == len)
            break;
    }
    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
DataSetGetValueWorldOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[6]);
    double x, y, z;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &z) != TCL_OK) {
        return TCL_ERROR;
    }
    double value = g_renderer->getDataValue(name, x, y, z);
    char buf[128];
    snprintf(buf, sizeof(buf), "nv>dataset value world %.12e %.12e %.12e %.12e", x, y, z, value);
    ssize_t bytesWritten;
    size_t len = strlen(buf);
    size_t ofs = 0;
    while ((bytesWritten = write(g_fdOut, buf + ofs, len - ofs)) > 0) {
        ofs += bytesWritten;
        if (ofs == len)
            break;
    }
    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static Rappture::CmdSpec dataSetGetValueOps[] = {
    {"pixel", 1, DataSetGetValuePixelOp, 6, 6, "x y dataSetName"},
    {"world", 1, DataSetGetValueWorldOp, 7, 7, "x y z dataSetName"}
};
static int nDataSetGetValueOps = NumCmdSpecs(dataSetGetValueOps);

static int
DataSetGetValueOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nDataSetGetValueOps, dataSetGetValueOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
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
        g_renderer->setOpacity(name, opacity);
    } else {
        g_renderer->setOpacity("all", opacity);
    }
    return TCL_OK;
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
        return TCL_ERROR;
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
        g_renderer->setVisibility(name, state);
    } else {
        g_renderer->setVisibility("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec dataSetOps[] = {
    {"add",      1, DataSetAddOp, 6, 6, "name data follows nBytes"},
    {"delete",   1, DataSetDeleteOp, 2, 3, "?name?"},
    {"getvalue", 1, DataSetGetValueOp, 6, 7, "oper x y ?z? name"},
    {"maprange", 1, DataSetMapRangeOp, 3, 3, "value"},
    {"opacity",  1, DataSetOpacityOp, 3, 4, "value ?name?"},
    {"visible",  1, DataSetVisibleOp, 3, 4, "bool ?name?"}
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
        g_renderer->addGlyphs(name);
        g_renderer->setGlyphsShape(name, shape);
    } else {
        g_renderer->addGlyphs("all");
        g_renderer->setGlyphsShape("all", shape);
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
GlyphsScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
    {"colormap",  1, GlyphsColorMapOp, 3, 4, "colorMapName ?dataSetNme?"},
    {"delete",    1, GlyphsDeleteOp, 2, 3, "?dataSetName?"},
    {"lighting",  1, GlyphsLightingOp, 3, 4, "bool ?dataSetName?"},
    {"opacity",   1, GlyphsOpacityOp, 3, 4, "value ?dataSetName?"},
    {"scale",     2, GlyphsScaleOp, 3, 4, "scaleFactor ?dataSetName?"},
    {"shape",     2, GlyphsShapeOp, 3, 4, "shapeVal ?dataSetName?"},
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
        g_renderer->addHeightMap(name);
        g_renderer->setHeightMapContourList(name, contourList);
    } else {
        g_renderer->addHeightMap("all");
        g_renderer->setHeightMapContourList("all", contourList);
    }
    return TCL_OK;
}

static int
HeightMapAddNumContoursOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    int numContours;
    if (Tcl_GetIntFromObj(interp, objv[3], &numContours) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->addHeightMap(name);
        g_renderer->setHeightMapContours(name, numContours);
    } else {
        g_renderer->addHeightMap("all");
        g_renderer->setHeightMapContours("all", numContours);
    }
    return TCL_OK;
}

static Rappture::CmdSpec heightmapAddOps[] = {
    {"contourlist", 1, HeightMapAddContourListOp, 4, 5, "contourList ?dataSetName?"},
    {"numcontours", 1, HeightMapAddNumContoursOp, 4, 5, "numContours ?dataSetName?"}
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
HeightMapContourVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapContourVisibility(name, state);
    } else {
        g_renderer->setHeightMapContourVisibility("all", state);
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

static Rappture::CmdSpec heightmapOps[] = {
    {"add",          1, HeightMapAddOp, 4, 5, "oper value ?dataSetName?"},
    {"colormap",     1, HeightMapColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",       1, HeightMapDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, HeightMapEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"heightscale",  1, HeightMapHeightScaleOp, 3, 4, "value ?dataSetName?"},
    {"isolinecolor", 8, HeightMapContourLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"isolines",     8, HeightMapContourVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"isolinewidth", 8, HeightMapContourLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"lighting",     3, HeightMapLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, HeightMapLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, HeightMapLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",      2, HeightMapOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, HeightMapOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, HeightMapPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",        1, HeightMapScaleOp, 5, 6, "sx sy sz  ?dataSetName?"},
    {"visible",      2, HeightMapVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"volumeslice",  2, HeightMapVolumeSliceOp, 4, 5, "axis ratio ?dataSetName?"}
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
    if (objc < 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " colormapName title width height ?dataSetName?\"", (char*)NULL);
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[1]);
    const char *title = Tcl_GetString(objv[2]);
    int width, height;

    if (Tcl_GetIntFromObj(interp, objv[3], &width) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[4], &height) != TCL_OK) {
        return TCL_ERROR;
    }

    vtkSmartPointer<vtkUnsignedCharArray> imgData = 
        vtkSmartPointer<vtkUnsignedCharArray>::New();

    if (objc == 6) {
        const char *dataSetName = Tcl_GetString(objv[5]);
        if (!g_renderer->renderColorMap(name, dataSetName, title, width, height, imgData)) {
            Tcl_AppendResult(interp, "Color map \"",
                             name, "\" was not found", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->renderColorMap(name, "all", title, width, height, imgData)) {
            Tcl_AppendResult(interp, "Color map \"",
                             name, "\" was not found", (char*)NULL);
            return TCL_ERROR;
        }
    }

#ifdef DEBUG
    writeTGAFile("/tmp/legend.tga", imgData->GetPointer(0), width, height,
                 TARGA_BYTES_PER_PIXEL);
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nv>legend %s", name);
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
        g_renderer->addLIC(name);
    } else {
        g_renderer->addLIC("all");
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
    {"opacity",     1, LICOpacityOp, 3, 4, "value ?dataSetName?"},
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
        g_renderer->addMolecule(name);
    } else {
        g_renderer->addMolecule("all");
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
    {"scale",      1, MoleculeScaleOp, 5, 6, "sx sy sz  ?dataSetName?"},
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
        g_renderer->addPolyData(name);
    } else {
        g_renderer->addPolyData("all");
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
    {"pos",       1, PolyDataPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",     1, PolyDataScaleOp, 5, 6, "sx sy sz  ?dataSetName?"},
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
        g_renderer->addPseudoColor(name);
    } else {
        g_renderer->addPseudoColor("all");
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
    {"colormap",  1, PseudoColorColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",    1, PseudoColorDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, PseudoColorEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",  3, PseudoColorLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, PseudoColorLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, PseudoColorLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   1, PseudoColorOpacityOp, 3, 4, "value ?dataSetName?"},
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
RendererRenderOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    g_renderer->eventuallyRender();
    return TCL_OK;
}

static Rappture::CmdSpec rendererOps[] = {
    {"clipplane", 1, RendererClipPlaneOp, 5, 5, "axis ratio direction"},
    {"depthpeel", 1, RendererDepthPeelingOp, 3, 3, "bool"},
    {"render",    1, RendererRenderOp, 2, 2, ""}
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
        g_renderer->addStreamlines(name);
    } else {
        g_renderer->addStreamlines("all");
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
StreamlinesSeedPolygonOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    double center[3], normal[3], radius;
    int numSides;
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
    if (Tcl_GetIntFromObj(interp, objv[10], &numSides) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 12) {
        const char *name = Tcl_GetString(objv[11]);
        g_renderer->setStreamlinesSeedToPolygon(name, center, normal, radius, numSides);
    } else {
        g_renderer->setStreamlinesSeedToPolygon("all", center, normal, radius, numSides);
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
        g_renderer->setStreamlinesSeedToRandomPoints(name, numPoints);
    } else {
        g_renderer->setStreamlinesSeedToRandomPoints("all", numPoints);
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
    {"polygon", 1, StreamlinesSeedPolygonOp, 11, 12, "centerX centerY centerZ normalX normalY normalZ radius numSides ?dataSetName?"},
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
    {"colormap",  1, StreamlinesColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",    1, StreamlinesDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, StreamlinesEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"length",    2, StreamlinesLengthOp, 3, 4, "length ?dataSetName?"},
    {"lighting",  3, StreamlinesLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, StreamlinesLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"lines",     5, StreamlinesLinesOp, 2, 3, "?dataSetName?"},
    {"linewidth", 5, StreamlinesLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   1, StreamlinesOpacityOp, 3, 4, "val ?dataSetName?"},
    {"ribbons",   1, StreamlinesRibbonsOp, 4, 5, "width angle ?dataSetName?"},
    {"seed",      1, StreamlinesSeedOp, 4, 11, "op params... ?dataSetName?"},
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
        g_renderer->addVolume(name);
    } else {
        g_renderer->addVolume("all");
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
    {"opacity",  1, VolumeOpacityOp, 3, 4, "val ?dataSetName?"},
    {"shading",  1, VolumeShadingOp, 4, 6, "oper val ?dataSetName?"},
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
        iov[2].iov_len = 1;
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
