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
    {"name",    1, AxisNameOp, 4, 4, "axis title"},
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
CameraGetOrientationOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double pos[3];
    double focalPt[3];
    double viewUp[3];

    g_renderer->getCameraOrientationAndPosition(pos, focalPt, viewUp);

    char buf[256];
    snprintf(buf, sizeof(buf), "nv>camera orient %.12e %.12e %.12e %.12e %.12e %.12e %.12e %.12e %.12e", 
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
CameraOrientationOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
    {"get", 1, CameraGetOrientationOp, 2, 2, ""},
    {"mode", 1, CameraModeOp, 3, 3, "mode"},
    {"orient", 3, CameraOrientationOp, 6, 6, "qw qx qy qz"},
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
        g_renderer->setContourList(name, contourList);
    } else {
        g_renderer->addContour2D("all");
        g_renderer->setContourList("all", contourList);
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
        g_renderer->setContours(name, numContours);
    } else {
        g_renderer->addContour2D("all");
        g_renderer->setContours("all", numContours);
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
        g_renderer->setContourLighting(name, state);
    } else {
        g_renderer->setContourLighting("all", state);
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
        g_renderer->setContourEdgeColor(name, color);
    } else {
        g_renderer->setContourEdgeColor("all", color);
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
        g_renderer->setContourEdgeWidth(name, width);
    } else {
        g_renderer->setContourEdgeWidth("all", width);
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
        g_renderer->setContourOpacity(name, opacity);
    } else {
        g_renderer->setContourOpacity("all", opacity);
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
        g_renderer->setContourVisibility(name, state);
    } else {
        g_renderer->setContourVisibility("all", state);
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
        return TCL_ERROR;
    }
#else
    size_t bytesRead = fread(data, 1, nbytes, g_fIn);
    TRACE("bytesRead: %d '%c'", bytesRead, data[0]);
    if (bytesRead < (size_t)nbytes) {
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
    writeTGAFile("/tmp/legend.tga", imgData->GetPointer(0), width, height);
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nv>legend %s", name);
    writePPM(g_fdOut, cmd, imgData->GetPointer(0), width, height);
#endif

    return TCL_OK;
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

static Rappture::CmdSpec pseudoColorOps[] = {
    {"add",       1, PseudoColorAddOp, 2, 3, "?dataSetName?"},
    {"colormap",  1, PseudoColorColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",    1, PseudoColorDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, PseudoColorEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",  3, PseudoColorLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, PseudoColorLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, PseudoColorLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   1, PseudoColorOpacityOp, 3, 4, "value ?dataSetName?"},
    {"visible",   1, PseudoColorVisibleOp, 3, 4, "bool ?dataSetName?"}
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
    {"opacity",   1, PolyDataOpacityOp, 3, 4, "value ?dataSetName?"},
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
    /*
    Tcl_MakeSafe(interp);
    */
    Tcl_CreateObjCommand(interp, "axis",        AxisCmd,        NULL, NULL);
    Tcl_CreateObjCommand(interp, "camera",      CameraCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "colormap",    ColorMapCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "contour2d",   Contour2DCmd,   NULL, NULL);
    Tcl_CreateObjCommand(interp, "dataset",     DataSetCmd,     NULL, NULL);
    Tcl_CreateObjCommand(interp, "legend",      LegendCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "polydata",    PolyDataCmd,    NULL, NULL);
    Tcl_CreateObjCommand(interp, "pseudocolor", PseudoColorCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "screen",      ScreenCmd,      NULL, NULL);
    return interp;
}
