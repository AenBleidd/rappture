/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
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
#include <unistd.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <tcl.h>

#include "Trace.h"
#include "CmdProc.h"
#include "ReadBuffer.h"
#include "RpTypes.h"
#include "RpVtkRendererCmd.h"
#include "RpVtkRenderServer.h"
#include "RpVtkRenderer.h"
#include "RpVtkRendererGraphicsObjs.h"
#include "PPMWriter.h"
#include "TGAWriter.h"
#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

using namespace Rappture::VtkVis;

static int lastCmdStatus;

#ifdef USE_THREADS
static void
QueueResponse(ClientData clientData, const void *bytes, size_t len, 
              Response::AllocationType allocType)
{
    ResponseQueue *queue = (ResponseQueue *)clientData;

    Response *response;

    response = new Response(Response::DATA);
    response->setMessage((unsigned char *)bytes, len, allocType);
    queue->enqueue(response);
}
#else

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

#endif  /*USE_THREADS*/

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

    TRACE("command: '%s'", Tcl_DStringValue(dsPtr));
    lastCmdStatus = TCL_OK;
    result = Tcl_EvalEx(interp, Tcl_DStringValue(dsPtr), 
                        Tcl_DStringLength(dsPtr), 
                        TCL_EVAL_DIRECT | TCL_EVAL_GLOBAL);
    Tcl_DStringSetLength(dsPtr, 0);
    if (lastCmdStatus == TCL_BREAK) {
        return TCL_BREAK;
    }
    lastCmdStatus = result;
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
        g_renderer->setAxisGridVisibility(X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisGridVisibility(Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisGridVisibility(Z_AXIS, visible);
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
        g_renderer->setAxisLabelVisibility(X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisLabelVisibility(Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisLabelVisibility(Z_AXIS, visible);
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
    if (strlen(title) > 30) {
        Tcl_AppendResult(interp, "axis name \"", title,
                         "\" is too long", (char*)NULL);
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisTitle(X_AXIS, title);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisTitle(Y_AXIS, title);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisTitle(Z_AXIS, title);
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
        g_renderer->setAxisTickVisibility(X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisTickVisibility(Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisTickVisibility(Z_AXIS, visible);
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
    if (strlen(units) > 10) {
        Tcl_AppendResult(interp, "axis units name \"", units,
                         "\" is too long", (char*)NULL);
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisUnits(X_AXIS, units);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisUnits(Y_AXIS, units);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisUnits(Z_AXIS, units);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxisUnits(X_AXIS, units);
        g_renderer->setAxisUnits(Y_AXIS, units);
        g_renderer->setAxisUnits(Z_AXIS, units);
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
        g_renderer->setAxisVisibility(X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisVisibility(Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisVisibility(Z_AXIS, visible);
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
BoxAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[2]);
    if (!g_renderer->addGraphicsObject<Box>(name)) {
        Tcl_AppendResult(interp, "Failed to create box", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
BoxDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Box>(name);
    } else {
        g_renderer->deleteGraphicsObject<Box>("all");
    }
    return TCL_OK;
}

static int
BoxColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Box>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Box>("all", color);
    }
    return TCL_OK;
}

static int
BoxEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Box>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Box>("all", state);
    }
    return TCL_OK;
}

static int
BoxLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Box>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Box>("all", state);
    }
    return TCL_OK;
}

static int
BoxLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Box>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Box>("all", color);
    }
    return TCL_OK;
}

static int
BoxLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Box>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Box>("all", width);
    }
    return TCL_OK;
}

static int
BoxMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double ambient, diffuse, specCoeff, specPower;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &ambient) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &diffuse) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &specCoeff) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &specPower) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setGraphicsObjectAmbient<Box>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Box>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Box>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Box>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Box>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Box>("all", specCoeff, specPower);
    }
    return TCL_OK;
}

static int
BoxOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Box>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Box>("all", opacity);
    }
    return TCL_OK;
}

static int
BoxOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Box>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Box>("all", quat);
    }
    return TCL_OK;
}

static int
BoxPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Box>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Box>("all", pos);
    }
    return TCL_OK;
}

static int
BoxScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Box>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Box>("all", scale);
    }
    return TCL_OK;
}

static int
BoxVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Box>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Box>("all", state);
    }
    return TCL_OK;
}

static int
BoxWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Box>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Box>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec boxOps[] = {
    {"add",       1, BoxAddOp, 3, 3, "name"},
    {"color",     1, BoxColorOp, 5, 6, "r g b ?name?"},
    {"delete",    1, BoxDeleteOp, 2, 3, "?name?"},
    {"edges",     1, BoxEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"lighting",  3, BoxLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, BoxLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, BoxLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, BoxMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, BoxOpacityOp, 3, 4, "value ?name?"},
    {"orient",    2, BoxOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"pos",       2, BoxPositionOp, 5, 6, "x y z ?name?"},
    {"scale",     1, BoxScaleOp, 5, 6, "sx sy sz ?name?"},
    {"visible",   1, BoxVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, BoxWireframeOp, 3, 4, "bool ?name?"}
};
static int nBoxOps = NumCmdSpecs(boxOps);

static int
BoxCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nBoxOps, boxOps,
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

#ifdef USE_THREADS
    QueueResponse(clientData, buf, strlen(buf), Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(buf, strlen(buf));
    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif
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
    {"get",    1, CameraGetOp, 2, 2, ""},
    {"mode",   1, CameraModeOp, 3, 3, "mode"},
    {"orient", 3, CameraOrientOp, 6, 6, "qw qx qy qz"},
    {"ortho",  1, CameraOrthoOp, 7, 7, "coordMode x y width height"},
    {"pan",    1, CameraPanOp, 4, 4, "panX panY"},
    {"reset",  2, CameraResetOp, 2, 3, "?all?"},
    {"rotate", 2, CameraRotateOp, 5, 5, "angle angle angle"},
    {"set",    1, CameraSetOp, 11, 11, "posX posY posZ focalPtX focalPtY focalPtZ viewUpX viewUpY viewUpZ"},
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
        g_renderer->deleteGraphicsObject<Contour2D>(name);
    } else {
        g_renderer->deleteGraphicsObject<Contour2D>("all");
    }
    return TCL_OK;
}

static int
Contour2DColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectColorMap<Contour2D>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Contour2D>("all", colorMapName);
    }
    return TCL_OK;
}

static int
Contour2DColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    Contour2D::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'c' && strcmp(str, "ccolor") == 0) {
        mode = Contour2D::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = Contour2D::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = Contour2D::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = Contour2D::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = Contour2D::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = Contour2D::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == Contour2D::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setContour2DColorMode(name, mode, fieldName);
    } else {
        g_renderer->setContour2DColorMode("all", mode, fieldName);
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
        g_renderer->setGraphicsObjectLighting<Contour2D>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Contour2D>("all", state);
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
        g_renderer->setGraphicsObjectColor<Contour2D>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Contour2D>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<Contour2D>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Contour2D>("all", width);
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
        g_renderer->setGraphicsObjectOpacity<Contour2D>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Contour2D>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<Contour2D>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Contour2D>("all", quat);
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
        g_renderer->setGraphicsObjectPosition<Contour2D>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Contour2D>("all", pos);
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
        g_renderer->setGraphicsObjectScale<Contour2D>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Contour2D>("all", scale);
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
        g_renderer->setGraphicsObjectVisibility<Contour2D>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Contour2D>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec contour2dOps[] = {
    {"add",       1, Contour2DAddOp, 4, 5, "oper value ?dataSetName?"},
    {"ccolor",    2, Contour2DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  7, Contour2DColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode", 7, Contour2DColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
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
        g_renderer->setGraphicsObjectColor<Contour3D>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Contour3D>("all", color);
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
        g_renderer->setGraphicsObjectColorMap<Contour3D>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Contour3D>("all", colorMapName);
    }
    return TCL_OK;
}

static int
Contour3DDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Contour3D>(name);
    } else {
        g_renderer->deleteGraphicsObject<Contour3D>("all");
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
        g_renderer->setGraphicsObjectEdgeVisibility<Contour3D>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Contour3D>("all", state);
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
        g_renderer->setGraphicsObjectLighting<Contour3D>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Contour3D>("all", state);
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
        g_renderer->setGraphicsObjectEdgeColor<Contour3D>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Contour3D>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<Contour3D>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Contour3D>("all", width);
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
        g_renderer->setGraphicsObjectOpacity<Contour3D>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Contour3D>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<Contour3D>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Contour3D>("all", quat);
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
        g_renderer->setGraphicsObjectPosition<Contour3D>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Contour3D>("all", pos);
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
        g_renderer->setGraphicsObjectScale<Contour3D>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Contour3D>("all", scale);
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
        g_renderer->setGraphicsObjectVisibility<Contour3D>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Contour3D>("all", state);
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
        g_renderer->setGraphicsObjectWireframe<Contour3D>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Contour3D>("all", state);
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
CutplaneAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addGraphicsObject<Cutplane>(name)) {
            Tcl_AppendResult(interp, "Failed to create cutplane", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<Cutplane>("all")) {
            Tcl_AppendResult(interp, "Failed to create cutplane for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
CutplaneColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Cutplane>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Cutplane>("all", color);
    }
    return TCL_OK;
}

static int
CutplaneColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectColorMap<Cutplane>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Cutplane>("all", colorMapName);
    }
    return TCL_OK;
}

static int
CutplaneColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    Cutplane::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = Cutplane::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = Cutplane::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = Cutplane::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = Cutplane::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = Cutplane::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setCutplaneColorMode(name, mode, fieldName);
    } else {
        g_renderer->setCutplaneColorMode("all", mode, fieldName);
    }
    return TCL_OK;
}

static int
CutplaneDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Cutplane>(name);
    } else {
        g_renderer->deleteGraphicsObject<Cutplane>("all");
    }
    return TCL_OK;
}

static int
CutplaneEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Cutplane>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Cutplane>("all", state);
    }
    return TCL_OK;
}

static int
CutplaneLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Cutplane>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Cutplane>("all", state);
    }
    return TCL_OK;
}

static int
CutplaneLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Cutplane>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Cutplane>("all", color);
    }
    return TCL_OK;
}

static int
CutplaneLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Cutplane>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Cutplane>("all", width);
    }
    return TCL_OK;
}

static int
CutplaneOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Cutplane>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Cutplane>("all", opacity);
    }
    return TCL_OK;
}

static int
CutplaneOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Cutplane>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Cutplane>("all", quat);
    }
    return TCL_OK;
}

static int
CutplaneOutlineOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setCutplaneOutlineVisibility(name, state);
    } else {
        g_renderer->setCutplaneOutlineVisibility("all", state);
    }
    return TCL_OK;
}

static int
CutplanePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Cutplane>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Cutplane>("all", pos);
    }
    return TCL_OK;
}

static int
CutplaneScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Cutplane>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Cutplane>("all", scale);
    }
    return TCL_OK;
}

static int
CutplaneSliceVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    Axis axis;
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        axis = X_AXIS;
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        axis = Y_AXIS;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        axis = Z_AXIS;
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName bool", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setCutplaneSliceVisibility(name, axis, state);
    } else {
        g_renderer->setCutplaneSliceVisibility("all", axis, state);
    }
    return TCL_OK;
}

static int
CutplaneVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Cutplane>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Cutplane>("all", state);
    }
    return TCL_OK;
}

static int
CutplaneVolumeSliceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double ratio;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &ratio) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    Axis axis;
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        axis = X_AXIS;
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        axis = Y_AXIS;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        axis = Z_AXIS;
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName ratio", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setGraphicsObjectVolumeSlice<Cutplane>(name, axis, ratio);
    } else {
        g_renderer->setGraphicsObjectVolumeSlice<Cutplane>("all", axis, ratio);
    }
    return TCL_OK;
}

static int
CutplaneWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Cutplane>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Cutplane>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec cutplaneOps[] = {
    {"add",          2, CutplaneAddOp, 2, 3, "oper value ?dataSetName?"},
    {"axis",         2, CutplaneSliceVisibilityOp, 4, 5, "axis bool ?dataSetName?"},
    {"ccolor",       2, CutplaneColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, CutplaneColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    7, CutplaneColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"delete",       1, CutplaneDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, CutplaneEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",     3, CutplaneLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, CutplaneLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, CutplaneLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",      2, CutplaneOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, CutplaneOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"outline",      2, CutplaneOutlineOp, 3, 4, "bool ?dataSetName?"},
    {"pos",          1, CutplanePositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",        2, CutplaneScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"slice",        2, CutplaneVolumeSliceOp, 4, 5, "axis ratio ?dataSetName?"},
    {"visible",      1, CutplaneVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe",    1, CutplaneWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nCutplaneOps = NumCmdSpecs(cutplaneOps);

static int
CutplaneCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nCutplaneOps, cutplaneOps,
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
    if (!SocketRead(data, nbytes)) {
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
    int length;

    length = snprintf(buf, sizeof(buf), "nv>dataset scalar pixel %d %d %g %s\n",
                      x, y, value, name);

#ifdef USE_THREADS
    QueueResponse(clientData, buf, length, Response::VOLATILE);
#else
    ssize_t bytesWritten = SocketWrite(buf, length);

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif
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
    int length;

    length = snprintf(buf, sizeof(buf), 
                      "nv>dataset scalar world %g %g %g %g %s\n",
                      x, y, z, value, name);

#ifdef USE_THREADS
    QueueResponse(clientData, buf, length, Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(buf, length);
    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif
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
    int length; 
    length = snprintf(buf, sizeof(buf), 
                      "nv>dataset vector pixel %d %d %g %g %g %s\n",
                      x, y,
                      value[0], value[1], value[2], name);

#ifdef USE_THREADS
    QueueResponse(clientData, buf, length, Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(buf, length);

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif /*USE_THREADS*/
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
    int length;
    length = snprintf(buf, sizeof(buf), 
                      "nv>dataset vector world %g %g %g %g %g %g %s\n",
                      x, y, z,
                      value[0], value[1], value[2], name);
#ifdef USE_THREADS
    QueueResponse(clientData, buf, length, Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(buf, length);

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif /*USE_THREADS*/
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
#ifdef USE_THREADS
    QueueResponse(clientData, oss.str().c_str(), len, Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(oss.str().c_str(), len);

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif /*USE_THREADS*/
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
        g_renderer->setGraphicsObjectColor<Glyphs>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Glyphs>("all", color);
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
        g_renderer->setGraphicsObjectColorMap<Glyphs>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Glyphs>("all", colorMapName);
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
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = Glyphs::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = Glyphs::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = Glyphs::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == Glyphs::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setGlyphsColorMode(name, mode, fieldName);
    } else {
        g_renderer->setGlyphsColorMode("all", mode, fieldName);
    }
    return TCL_OK;
}

static int
GlyphsDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Glyphs>(name);
    } else {
        g_renderer->deleteGraphicsObject<Glyphs>("all");
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
        g_renderer->setGraphicsObjectEdgeVisibility<Glyphs>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Glyphs>("all", state);
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
        g_renderer->setGraphicsObjectLighting<Glyphs>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Glyphs>("all", state);
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
        g_renderer->setGraphicsObjectEdgeColor<Glyphs>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Glyphs>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<Glyphs>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Glyphs>("all", width);
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
        g_renderer->setGraphicsObjectOpacity<Glyphs>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Glyphs>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<Glyphs>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Glyphs>("all", quat);
    }
    return TCL_OK;
}

static int
GlyphsOrientGlyphsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setGlyphsOrientMode(name, state, fieldName);
    } else {
        g_renderer->setGlyphsOrientMode("all", state, fieldName);
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
        g_renderer->setGraphicsObjectPosition<Glyphs>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Glyphs>("all", pos);
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
        g_renderer->setGraphicsObjectScale<Glyphs>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Glyphs>("all", scale);
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
    const char *fieldName = Tcl_GetString(objv[3]);
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setGlyphsScalingMode(name, mode, fieldName);
    } else {
        g_renderer->setGlyphsScalingMode("all", mode, fieldName);
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
        g_renderer->setGraphicsObjectVisibility<Glyphs>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Glyphs>("all", state);
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
        g_renderer->setGraphicsObjectWireframe<Glyphs>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Glyphs>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec glyphsOps[] = {
    {"add",       1, GlyphsAddOp, 3, 4, "shape ?dataSetName?"},
    {"ccolor",    2, GlyphsColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  7, GlyphsColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode", 7, GlyphsColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"delete",    1, GlyphsDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, GlyphsEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"gorient",   2, GlyphsOrientGlyphsOp, 4, 5, "bool fieldName ?dataSetName?"},
    {"gscale",    2, GlyphsScaleFactorOp, 3, 4, "scaleFactor ?dataSetName?"},
    {"lighting",  3, GlyphsLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, GlyphsLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, GlyphsLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"normscale", 1, GlyphsNormalizeScaleOp, 3, 4, "bool ?dataSetName?"},
    {"opacity",   2, GlyphsOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",    2, GlyphsOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       1, GlyphsPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",     2, GlyphsScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"shape",     2, GlyphsShapeOp, 3, 4, "shapeVal ?dataSetName?"},
    {"smode",     2, GlyphsScalingModeOp, 4, 5, "mode fieldName ?dataSetName?"},
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
        g_renderer->setGraphicsObjectColorMap<HeightMap>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<HeightMap>("all", colorMapName);
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
        g_renderer->deleteGraphicsObject<HeightMap>(name);
    } else {
        g_renderer->deleteGraphicsObject<HeightMap>("all");
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
        g_renderer->setGraphicsObjectEdgeVisibility<HeightMap>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<HeightMap>("all", state);
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
        g_renderer->setGraphicsObjectLighting<HeightMap>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<HeightMap>("all", state);
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
        g_renderer->setGraphicsObjectEdgeColor<HeightMap>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<HeightMap>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<HeightMap>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<HeightMap>("all", width);
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
        g_renderer->setGraphicsObjectOpacity<HeightMap>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<HeightMap>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<HeightMap>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<HeightMap>("all", quat);
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
        g_renderer->setGraphicsObjectPosition<HeightMap>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<HeightMap>("all", pos);
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
        g_renderer->setGraphicsObjectScale<HeightMap>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<HeightMap>("all", scale);
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
        g_renderer->setGraphicsObjectVisibility<HeightMap>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<HeightMap>("all", state);
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
    Axis axis;
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        axis = X_AXIS;
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        axis = Y_AXIS;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        axis = Z_AXIS;
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName ratio", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setGraphicsObjectVolumeSlice<HeightMap>(name, axis, ratio);
    } else {
        g_renderer->setGraphicsObjectVolumeSlice<HeightMap>("all", axis, ratio);
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
        g_renderer->setGraphicsObjectWireframe<HeightMap>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<HeightMap>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec heightmapOps[] = {
    {"add",          1, HeightMapAddOp, 5, 6, "oper value ?dataSetName?"},
    {"colormap",     3, HeightMapColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"contourlist",  3, HeightMapContourListOp, 3, 4, "contourList ?dataSetName?"},
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
    {"scale",        2, HeightMapScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"slice",        2, HeightMapVolumeSliceOp, 4, 5, "axis ratio ?dataSetName?"},
    {"surface",      2, HeightMapContourSurfaceVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"visible",      1, HeightMapVisibleOp, 3, 4, "bool ?dataSetName?"},
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
ImageFlushCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    lastCmdStatus = TCL_BREAK;
    return TCL_OK;
}

static int
LegendCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    if (objc < 8) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " colormapName legendType fieldName title width height numLabels ?dataSetName?\"", (char*)NULL);
        return TCL_ERROR;
    }
    const char *colorMapName = Tcl_GetString(objv[1]);
    const char *typeStr = Tcl_GetString(objv[2]);
    Renderer::LegendType legendType;
    if (typeStr[0] == 's' && strcmp(typeStr, "scalar") == 0) {
        legendType = Renderer::LEGEND_SCALAR;
    } else if (typeStr[0] == 'v' && strcmp(typeStr, "vmag") == 0) {
        legendType = Renderer::LEGEND_VECTOR_MAGNITUDE;
    } else if (typeStr[0] == 'v' && strcmp(typeStr, "vx") == 0) {
        legendType = Renderer::LEGEND_VECTOR_X;
    } else if (typeStr[0] == 'v' && strcmp(typeStr, "vy") == 0) {
        legendType = Renderer::LEGEND_VECTOR_Y;
    } else if (typeStr[0] == 'v' && strcmp(typeStr, "vz") == 0) {
        legendType = Renderer::LEGEND_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "Bad legendType option \"",
                         typeStr, "\", should be one of 'scalar', 'vmag', 'vx', 'vy', 'vz'", (char*)NULL);
        return TCL_ERROR;
    }

    const char *fieldName = Tcl_GetString(objv[3]);

    std::string title(Tcl_GetString(objv[4]));
    int width, height, numLabels;

    if (Tcl_GetIntFromObj(interp, objv[5], &width) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[6], &height) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[7], &numLabels) != TCL_OK) {
        return TCL_ERROR;
    }

    vtkSmartPointer<vtkUnsignedCharArray> imgData = 
        vtkSmartPointer<vtkUnsignedCharArray>::New();

    double range[2];
    range[0] = DBL_MAX;
    range[1] = -DBL_MAX;

    if (objc == 9) {
        const char *dataSetName = Tcl_GetString(objv[8]);
        if (!g_renderer->renderColorMap(colorMapName, dataSetName, legendType, fieldName, title,
                                        range, width, height, true, numLabels, imgData)) {
            Tcl_AppendResult(interp, "Color map \"",
                             colorMapName, "\" or dataset \"",
                             dataSetName, "\" was not found", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->renderColorMap(colorMapName, "all", legendType, fieldName, title,
                                        range, width, height, true, numLabels, imgData)) {
            Tcl_AppendResult(interp, "Color map \"",
                             colorMapName, "\" was not found", (char*)NULL);
            return TCL_ERROR;
        }
    }

#ifdef DEBUG
# ifdef RENDER_TARGA
    writeTGAFile("/tmp/legend.tga", imgData->GetPointer(0), width, height,
                 TARGA_BYTES_PER_PIXEL);
# else
    writeTGAFile("/tmp/legend.tga", imgData->GetPointer(0), width, height,
                 TARGA_BYTES_PER_PIXEL, true);
# endif
#else
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "nv>legend {%s} {%s} %g %g",
             colorMapName, title.c_str(), range[0], range[1]);

# ifdef USE_THREADS
#  ifdef RENDER_TARGA
    ResponseQueue *queue = (ResponseQueue *)clientData;
    queueTGA(queue, cmd, imgData->GetPointer(0), width, height,
             TARGA_BYTES_PER_PIXEL);
#  else
    ResponseQueue *queue = (ResponseQueue *)clientData;
    queuePPM(queue, cmd, imgData->GetPointer(0), width, height);
#  endif
# else
#  ifdef RENDER_TARGA
    writeTGA(g_fdOut, cmd, imgData->GetPointer(0), width, height,
             TARGA_BYTES_PER_PIXEL);
#  else
    writePPM(g_fdOut, cmd, imgData->GetPointer(0), width, height);
#  endif
# endif // USE_THREADS
#endif // DEBUG
    return TCL_OK;
}

static int
LICAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addGraphicsObject<LIC>(name)) {
            Tcl_AppendResult(interp, "Failed to create lic", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<LIC>("all")) {
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
        g_renderer->setGraphicsObjectColorMap<LIC>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<LIC>("all", colorMapName);
    }
    return TCL_OK;
}

static int
LICDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<LIC>(name);
    } else {
        g_renderer->deleteGraphicsObject<LIC>("all");
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
        g_renderer->setGraphicsObjectEdgeVisibility<LIC>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<LIC>("all", state);
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
        g_renderer->setGraphicsObjectLighting<LIC>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<LIC>("all", state);
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
        g_renderer->setGraphicsObjectEdgeColor<LIC>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<LIC>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<LIC>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<LIC>("all", width);
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
        g_renderer->setGraphicsObjectOpacity<LIC>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<LIC>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<LIC>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<LIC>("all", quat);
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
        g_renderer->setGraphicsObjectPosition<LIC>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<LIC>("all", pos);
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
        g_renderer->setGraphicsObjectScale<LIC>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<LIC>("all", scale);
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
        g_renderer->setGraphicsObjectVisibility<LIC>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<LIC>("all", state);
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
    Axis axis;
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        axis = X_AXIS;
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        axis = Y_AXIS;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        axis = Z_AXIS;
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName ratio", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setGraphicsObjectVolumeSlice<LIC>(name, axis, ratio);
    } else {
        g_renderer->setGraphicsObjectVolumeSlice<LIC>("all", axis, ratio);
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
    {"scale",       2, LICScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"slice",       2, LICVolumeSliceOp, 4, 5, "axis ratio ?dataSetName?"},
    {"visible",     1, LICVisibleOp, 3, 4, "bool ?dataSetName?"}
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
        if (!g_renderer->addGraphicsObject<Molecule>(name)) {
            Tcl_AppendResult(interp, "Failed to create molecule", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<Molecule>("all")) {
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
MoleculeAtomLabelVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeAtomLabelVisibility(name, state);
    } else {
        g_renderer->setMoleculeAtomLabelVisibility("all", state);
    }
    return TCL_OK;
}

static int
MoleculeAtomScaleFactorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    double scale;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeAtomRadiusScale(name, scale);
    } else {
        g_renderer->setMoleculeAtomRadiusScale("all", scale);
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
MoleculeBondColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    Molecule::BondColorMode mode;
    const char *colorMode = Tcl_GetString(objv[2]);
    if (colorMode[0] == 'c' && strcmp(colorMode, "constant") == 0) {
        mode = Molecule::BOND_COLOR_CONSTANT;
    } else if (colorMode[0] == 'b' && strcmp(colorMode, "by_elements") == 0) {
        mode = Molecule::BOND_COLOR_BY_ELEMENTS;
    } else {
        Tcl_AppendResult(interp, "bad bcmode option \"", colorMode,
                         "\": should be constant or by_elements", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeBondColorMode(name, mode);
    } else {
        g_renderer->setMoleculeBondColorMode("all", mode);
    }
    return TCL_OK;
}

static int
MoleculeBondColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setMoleculeBondColor(name, color);
    } else {
        g_renderer->setMoleculeBondColor("all", color);
    }
    return TCL_OK;
}

static int
MoleculeBondScaleFactorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    double scale;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeBondRadiusScale(name, scale);
    } else {
        g_renderer->setMoleculeBondRadiusScale("all", scale);
    }
    return TCL_OK;
}

static int
MoleculeBondStyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    Molecule::BondStyle style;
    const char *styleStr = Tcl_GetString(objv[2]);
    if (styleStr[0] == 'c' && strcmp(styleStr, "cylinder") == 0) {
        style = Molecule::BOND_STYLE_CYLINDER;
    } else if (styleStr[0] == 'l' && strcmp(styleStr, "line") == 0) {
        style = Molecule::BOND_STYLE_LINE;
    } else {
        Tcl_AppendResult(interp, "bad bstyle option \"", styleStr,
                         "\": should be cylinder or line", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeBondStyle(name, style);
    } else {
        g_renderer->setMoleculeBondStyle("all", style);
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
MoleculeColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Molecule>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Molecule>("all", color);
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
        g_renderer->setGraphicsObjectColorMap<Molecule>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Molecule>("all", colorMapName);
    }
    return TCL_OK;
}

static int
MoleculeColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    Molecule::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'b' && strcmp(str, "by_elements") == 0) {
        mode = Molecule::COLOR_BY_ELEMENTS;
    } else if (str[0] == 'c' && strcmp(str, "ccolor") == 0) {
        mode = Molecule::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = Molecule::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = Molecule::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = Molecule::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = Molecule::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = Molecule::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'by_elements', 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == Molecule::COLOR_CONSTANT) {
        fieldName = NULL;
    }
#if 0
    else if (mode == Molecule::COLOR_BY_ELEMENTS) {
        fieldName = "element";
    }
#endif
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setMoleculeColorMode(name, mode, fieldName);
    } else {
        g_renderer->setMoleculeColorMode("all", mode, fieldName);
    }
    return TCL_OK;
}

static int
MoleculeDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Molecule>(name);
    } else {
        g_renderer->deleteGraphicsObject<Molecule>("all");
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
        g_renderer->setGraphicsObjectEdgeVisibility<Molecule>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Molecule>("all", state);
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
        g_renderer->setGraphicsObjectLighting<Molecule>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Molecule>("all", state);
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
        g_renderer->setGraphicsObjectEdgeColor<Molecule>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Molecule>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<Molecule>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Molecule>("all", width);
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
        g_renderer->setGraphicsObjectOpacity<Molecule>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Molecule>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<Molecule>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Molecule>("all", quat);
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
        g_renderer->setGraphicsObjectPosition<Molecule>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Molecule>("all", pos);
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
        g_renderer->setGraphicsObjectScale<Molecule>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Molecule>("all", scale);
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
        g_renderer->setGraphicsObjectVisibility<Molecule>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Molecule>("all", state);
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
        g_renderer->setGraphicsObjectWireframe<Molecule>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Molecule>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec moleculeOps[] = {
    {"add",        2, MoleculeAddOp, 2, 3, "?dataSetName?"},
    {"ascale",     2, MoleculeAtomScaleFactorOp, 3, 4, "value ?dataSetName?"},
    {"atoms",      2, MoleculeAtomVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"bcmode",     3, MoleculeBondColorModeOp, 3, 4, "mode ?dataSetName?"},
    {"bcolor",     3, MoleculeBondColorOp, 5, 6, "r g b ?dataSetName?"},
    {"bonds",      2, MoleculeBondVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"bscale",     3, MoleculeBondScaleFactorOp, 3, 4, "value ?dataSetName?"},
    {"bstyle",     3, MoleculeBondStyleOp, 3, 4, "value ?dataSetName?"},
    {"ccolor",     2, MoleculeColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",   7, MoleculeColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",  7, MoleculeColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"delete",     1, MoleculeDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",      1, MoleculeEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"labels",     2, MoleculeAtomLabelVisibilityOp, 3, 4, "bool ?dataSetName?"},
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
        if (!g_renderer->addGraphicsObject<PolyData>(name)) {
            Tcl_AppendResult(interp, "Failed to create polydata", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<PolyData>("all")) {
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
        g_renderer->deleteGraphicsObject<PolyData>(name);
    } else {
        g_renderer->deleteGraphicsObject<PolyData>("all");
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
        g_renderer->setGraphicsObjectColor<PolyData>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<PolyData>("all", color);
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
        g_renderer->setGraphicsObjectEdgeVisibility<PolyData>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<PolyData>("all", state);
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
        g_renderer->setGraphicsObjectLighting<PolyData>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<PolyData>("all", state);
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
        g_renderer->setGraphicsObjectEdgeColor<PolyData>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<PolyData>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<PolyData>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<PolyData>("all", width);
    }
    return TCL_OK;
}

static int
PolyDataMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double ambient, diffuse, specCoeff, specPower;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &ambient) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &diffuse) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &specCoeff) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &specPower) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setGraphicsObjectAmbient<PolyData>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<PolyData>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<PolyData>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<PolyData>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<PolyData>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<PolyData>("all", specCoeff, specPower);
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
        g_renderer->setGraphicsObjectOpacity<PolyData>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<PolyData>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<PolyData>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<PolyData>("all", quat);
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
        g_renderer->setGraphicsObjectPointSize<PolyData>(name, size);
    } else {
        g_renderer->setGraphicsObjectPointSize<PolyData>("all", size);
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
        g_renderer->setGraphicsObjectPosition<PolyData>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<PolyData>("all", pos);
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
        g_renderer->setGraphicsObjectScale<PolyData>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<PolyData>("all", scale);
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
        g_renderer->setGraphicsObjectVisibility<PolyData>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<PolyData>("all", state);
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
        g_renderer->setGraphicsObjectWireframe<PolyData>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<PolyData>("all", state);
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
    {"material",  1, PolyDataMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?dataSetName?"},
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
        if (!g_renderer->addGraphicsObject<PseudoColor>(name)) {
            Tcl_AppendResult(interp, "Failed to create pseudocolor", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<PseudoColor>("all")) {
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
        g_renderer->setGraphicsObjectColor<PseudoColor>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<PseudoColor>("all", color);
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
        g_renderer->setGraphicsObjectColorMap<PseudoColor>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<PseudoColor>("all", colorMapName);
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
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == PseudoColor::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setPseudoColorColorMode(name, mode, fieldName);
    } else {
        g_renderer->setPseudoColorColorMode("all", mode, fieldName);
    }
    return TCL_OK;
}

static int
PseudoColorDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<PseudoColor>(name);
    } else {
        g_renderer->deleteGraphicsObject<PseudoColor>("all");
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
        g_renderer->setGraphicsObjectEdgeVisibility<PseudoColor>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<PseudoColor>("all", state);
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
        g_renderer->setGraphicsObjectLighting<PseudoColor>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<PseudoColor>("all", state);
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
        g_renderer->setGraphicsObjectEdgeColor<PseudoColor>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<PseudoColor>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<PseudoColor>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<PseudoColor>("all", width);
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
        g_renderer->setGraphicsObjectOpacity<PseudoColor>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<PseudoColor>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<PseudoColor>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<PseudoColor>("all", quat);
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
        g_renderer->setGraphicsObjectPosition<PseudoColor>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<PseudoColor>("all", pos);
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
        g_renderer->setGraphicsObjectScale<PseudoColor>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<PseudoColor>("all", scale);
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
        g_renderer->setGraphicsObjectVisibility<PseudoColor>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<PseudoColor>("all", state);
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
        g_renderer->setGraphicsObjectWireframe<PseudoColor>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<PseudoColor>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec pseudoColorOps[] = {
    {"add",       1, PseudoColorAddOp, 2, 3, "?dataSetName?"},
    {"ccolor",    2, PseudoColorColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  7, PseudoColorColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode", 7, PseudoColorColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
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
    Axis axis;
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        axis = X_AXIS;
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        axis = Y_AXIS;
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        axis = Z_AXIS;
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
SphereAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[2]);
    if (!g_renderer->addGraphicsObject<Sphere>(name)) {
        Tcl_AppendResult(interp, "Failed to create box", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
SphereDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Sphere>(name);
    } else {
        g_renderer->deleteGraphicsObject<Sphere>("all");
    }
    return TCL_OK;
}

static int
SphereColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Sphere>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Sphere>("all", color);
    }
    return TCL_OK;
}

static int
SphereEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Sphere>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Sphere>("all", state);
    }
    return TCL_OK;
}

static int
SphereLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Sphere>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Sphere>("all", state);
    }
    return TCL_OK;
}

static int
SphereLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Sphere>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Sphere>("all", color);
    }
    return TCL_OK;
}

static int
SphereLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Sphere>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Sphere>("all", width);
    }
    return TCL_OK;
}

static int
SphereMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double ambient, diffuse, specCoeff, specPower;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &ambient) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &diffuse) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &specCoeff) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &specPower) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setGraphicsObjectAmbient<Sphere>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Sphere>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Sphere>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Sphere>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Sphere>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Sphere>("all", specCoeff, specPower);
    }
    return TCL_OK;
}

static int
SphereOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Sphere>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Sphere>("all", opacity);
    }
    return TCL_OK;
}

static int
SphereOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Sphere>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Sphere>("all", quat);
    }
    return TCL_OK;
}

static int
SpherePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Sphere>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Sphere>("all", pos);
    }
    return TCL_OK;
}

static int
SphereResolutionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    int theta, phi;
    if (Tcl_GetIntFromObj(interp, objv[2], &theta) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &phi) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setSphereResolution(name, theta, phi);
    } else {
        g_renderer->setSphereResolution("all", theta, phi);
    }
    return TCL_OK;
}

static int
SphereScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Sphere>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Sphere>("all", scale);
    }
    return TCL_OK;
}

static int
SphereSectionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double thetaStart, thetaEnd, phiStart, phiEnd;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &thetaStart) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &thetaEnd) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &phiStart) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &phiEnd) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setSphereSection(name, thetaStart, thetaEnd, phiStart, phiEnd);
    } else {
        g_renderer->setSphereSection("all", thetaStart, thetaEnd, phiStart, phiEnd);
    }
    return TCL_OK;
}

static int
SphereVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Sphere>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Sphere>("all", state);
    }
    return TCL_OK;
}

static int
SphereWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Sphere>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Sphere>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec sphereOps[] = {
    {"add",       1, SphereAddOp, 3, 3, "name"},
    {"color",     1, SphereColorOp, 5, 6, "r g b ?name?"},
    {"delete",    1, SphereDeleteOp, 2, 3, "?name?"},
    {"edges",     1, SphereEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"lighting",  3, SphereLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, SphereLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, SphereLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, SphereMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, SphereOpacityOp, 3, 4, "value ?name?"},
    {"orient",    2, SphereOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"pos",       2, SpherePositionOp, 5, 6, "x y z ?name?"},
    {"resolution",1, SphereResolutionOp, 4, 5, "thetaRes phiRes ?name?"},
    {"scale",     2, SphereScaleOp, 5, 6, "sx sy sz ?name?"},
    {"section",   2, SphereSectionOp, 6, 7, "thetaStart thetaEnd phiStart phiEnd ?name?"},
    {"visible",   1, SphereVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, SphereWireframeOp, 3, 4, "bool ?name?"}
};
static int nSphereOps = NumCmdSpecs(sphereOps);

static int
SphereCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nSphereOps, sphereOps,
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
        if (!g_renderer->addGraphicsObject<Streamlines>(name)) {
            Tcl_AppendResult(interp, "Failed to create streamlines", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<Streamlines>("all")) {
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
        g_renderer->setGraphicsObjectColor<Streamlines>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Streamlines>("all", color);
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
        g_renderer->setGraphicsObjectColorMap<Streamlines>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Streamlines>("all", colorMapName);
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
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == Streamlines::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    if (objc == 5) {
        const char *dataSetName = Tcl_GetString(objv[4]);
        g_renderer->setStreamlinesColorMode(dataSetName, mode, fieldName);
    } else {
        g_renderer->setStreamlinesColorMode("all", mode, fieldName);
    }
    return TCL_OK;
}

static int
StreamlinesDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Streamlines>(name);
    } else {
        g_renderer->deleteGraphicsObject<Streamlines>("all");
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
        g_renderer->setGraphicsObjectEdgeVisibility<Streamlines>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Streamlines>("all", state);
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
        g_renderer->setGraphicsObjectLighting<Streamlines>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Streamlines>("all", state);
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
        g_renderer->setGraphicsObjectEdgeColor<Streamlines>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Streamlines>("all", color);
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
        g_renderer->setGraphicsObjectEdgeWidth<Streamlines>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Streamlines>("all", width);
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
        g_renderer->setGraphicsObjectOpacity<Streamlines>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Streamlines>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<Streamlines>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Streamlines>("all", quat);
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
        g_renderer->setGraphicsObjectPosition<Streamlines>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Streamlines>("all", pos);
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
        g_renderer->setGraphicsObjectScale<Streamlines>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Streamlines>("all", scale);
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
    if (!SocketRead(data, nbytes)) {
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
    if (!SocketRead(data, nbytes)) {
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
StreamlinesSeedNumPointsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                           Tcl_Obj *const *objv)
{
    int numPoints;
    if (Tcl_GetIntFromObj(interp, objv[3], &numPoints) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setStreamlinesNumberOfSeedPoints(name, numPoints);
    } else {
        g_renderer->setStreamlinesNumberOfSeedPoints("all", numPoints);
    }
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
    {"color",   1, StreamlinesSeedColorOp,         6, 7, "r g b ?dataSetName?"},
    {"disk",    1, StreamlinesSeedDiskOp,          12, 13, "centerX centerY centerZ normalX normalY normalZ radius innerRadius numPoints ?dataSetName?"},
    {"fmesh",   2, StreamlinesSeedFilledMeshOp,    7, 8, "numPoints data follows nbytes ?dataSetName?"},
    {"fpoly",   2, StreamlinesSeedFilledPolygonOp, 13, 14, "centerX centerY centerZ normalX normalY normalZ angle radius numSides numPoints ?dataSetName?"},
    {"mesh",    1, StreamlinesSeedMeshPointsOp,    6, 7, "data follows nbytes ?dataSetName?"},
    {"numpts",  1, StreamlinesSeedNumPointsOp,     4, 5, "numPoints ?dataSetName?"},
    {"points",  3, StreamlinesSeedPointsOp,        3, 4, "?dataSetName?"},
    {"polygon", 3, StreamlinesSeedPolygonOp,       12, 13, "centerX centerY centerZ normalX normalY normalZ angle radius numSides ?dataSetName?"},
    {"rake",    3, StreamlinesSeedRakeOp,          10, 11, "startX startY startZ endX endY endZ numPoints ?dataSetName?"},
    {"random",  3, StreamlinesSeedRandomOp,        4, 5, "numPoints ?dataSetName?"},
    {"visible", 1, StreamlinesSeedVisibleOp,       4, 5, "bool ?dataSetName?"}
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
        g_renderer->setGraphicsObjectVisibility<Streamlines>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Streamlines>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec streamlinesOps[] = {
    {"add",       1, StreamlinesAddOp,            2, 3, "?dataSetName?"},
    {"ccolor",    1, StreamlinesColorOp,          5, 6, "r g b ?dataSetName?"},
    {"colormap",  7, StreamlinesColorMapOp,       3, 4, "colorMapName ?dataSetName?"},
    {"colormode", 7, StreamlinesColorModeOp,      4, 5, "mode fieldName ?dataSetName?"},
    {"delete",    1, StreamlinesDeleteOp,         2, 3, "?dataSetName?"},
    {"edges",     1, StreamlinesEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"length",    2, StreamlinesLengthOp,         3, 4, "length ?dataSetName?"},
    {"lighting",  3, StreamlinesLightingOp,       3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, StreamlinesLineColorOp,      5, 6, "r g b ?dataSetName?"},
    {"lines",     5, StreamlinesLinesOp,          2, 3, "?dataSetName?"},
    {"linewidth", 5, StreamlinesLineWidthOp,      3, 4, "width ?dataSetName?"},
    {"opacity",   2, StreamlinesOpacityOp,        3, 4, "val ?dataSetName?"},
    {"orient",    2, StreamlinesOrientOp,         6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       1, StreamlinesPositionOp,       5, 6, "x y z ?dataSetName?"},
    {"ribbons",   1, StreamlinesRibbonsOp,        4, 5, "width angle ?dataSetName?"},
    {"scale",     2, StreamlinesScaleOp,          5, 6, "sx sy sz ?dataSetName?"},
    {"seed",      2, StreamlinesSeedOp,           3, 14, "op params... ?dataSetName?"},
    {"tubes",     1, StreamlinesTubesOp,          4, 5, "numSides radius ?dataSetName?"},
    {"visible",   1, StreamlinesVisibleOp,        3, 4, "bool ?dataSetName?"}
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
        if (!g_renderer->addGraphicsObject<Volume>(name)) {
            Tcl_AppendResult(interp, "Failed to create volume", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<Volume>("all")) {
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
        g_renderer->setGraphicsObjectColorMap<Volume>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Volume>("all", colorMapName);
    }
    return TCL_OK;
}

static int
VolumeDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Volume>(name);
    } else {
        g_renderer->deleteGraphicsObject<Volume>("all");
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
        g_renderer->setGraphicsObjectLighting<Volume>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Volume>("all", state);
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
        g_renderer->setGraphicsObjectOpacity<Volume>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Volume>("all", opacity);
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
        g_renderer->setGraphicsObjectOrientation<Volume>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Volume>("all", quat);
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
        g_renderer->setGraphicsObjectPosition<Volume>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Volume>("all", pos);
    }
    return TCL_OK;
}

static int
VolumeSampleRateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double quality;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quality) != TCL_OK) {
        return TCL_ERROR;
    }
    if (quality < 0.0 || quality > 1.0) {
        Tcl_AppendResult(interp, "bad volume quality value \"",
                         Tcl_GetString(objv[2]),
                         "\": should be in the range [0,1]", (char*)NULL);
        return TCL_ERROR;
    }
    double distance;
    double maxFactor = 4.0;
    if (quality >= 0.5) {
        distance = 1.0 / ((quality - 0.5) * (maxFactor - 1.0) * 2.0 + 1.0);
    } else {
        distance = ((0.5 - quality) * (maxFactor - 1.0) * 2.0 + 1.0);
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setVolumeSampleDistance(name, distance);
    } else {
        g_renderer->setVolumeSampleDistance("all", distance);
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
        g_renderer->setGraphicsObjectScale<Volume>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Volume>("all", scale);
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
        g_renderer->setGraphicsObjectAmbient<Volume>(name, coeff);
    } else {
        g_renderer->setGraphicsObjectAmbient<Volume>("all", coeff);
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
        g_renderer->setGraphicsObjectDiffuse<Volume>(name, coeff);
    } else {
        g_renderer->setGraphicsObjectDiffuse<Volume>("all", coeff);
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
        g_renderer->setGraphicsObjectSpecular<Volume>(name, coeff, power);
    } else {
        g_renderer->setGraphicsObjectSpecular<Volume>("all", coeff, power);
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeShadingOps[] = {
    {"ambient",  1, VolumeShadingAmbientOp,  4, 5, "coeff ?dataSetName?"},
    {"diffuse",  1, VolumeShadingDiffuseOp,  4, 5, "coeff ?dataSetName?"},
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
        g_renderer->setGraphicsObjectVisibility<Volume>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Volume>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec volumeOps[] = {
    {"add",      1, VolumeAddOp,        2, 3, "?dataSetName?"},
    {"colormap", 1, VolumeColorMapOp,   3, 4, "colorMapName ?dataSetName?"},
    {"delete",   1, VolumeDeleteOp,     2, 3, "?dataSetName?"},
    {"lighting", 1, VolumeLightingOp,   3, 4, "bool ?dataSetName?"},
    {"opacity",  2, VolumeOpacityOp,    3, 4, "val ?dataSetName?"},
    {"orient",   2, VolumeOrientOp,     6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",      1, VolumePositionOp,   5, 6, "x y z ?dataSetName?"},
    {"quality",  1, VolumeSampleRateOp, 3, 4, "val ?dataSetName?"},
    {"scale",    2, VolumeScaleOp,      5, 6, "sx sy sz ?dataSetName?"},
    {"shading",  2, VolumeShadingOp,    4, 6, "oper val ?dataSetName?"},
    {"visible",  1, VolumeVisibleOp,    3, 4, "bool ?dataSetName?"}
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

static int
WarpAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addGraphicsObject<Warp>(name)) {
            Tcl_AppendResult(interp, "Failed to create warp", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<Warp>("all")) {
            Tcl_AppendResult(interp, "Failed to create warp for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
WarpColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectColorMap<Warp>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Warp>("all", colorMapName);
    }
    return TCL_OK;
}

static int
WarpDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Warp>(name);
    } else {
        g_renderer->deleteGraphicsObject<Warp>("all");
    }
    return TCL_OK;
}

static int
WarpEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Warp>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Warp>("all", state);
    }
    return TCL_OK;
}

static int
WarpLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Warp>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Warp>("all", state);
    }
    return TCL_OK;
}

static int
WarpLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Warp>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Warp>("all", color);
    }
    return TCL_OK;
}

static int
WarpLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Warp>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Warp>("all", width);
    }
    return TCL_OK;
}

static int
WarpOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Warp>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Warp>("all", opacity);
    }
    return TCL_OK;
}

static int
WarpOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Warp>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Warp>("all", quat);
    }
    return TCL_OK;
}

static int
WarpPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Warp>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Warp>("all", pos);
    }
    return TCL_OK;
}

static int
WarpScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Warp>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Warp>("all", scale);
    }
    return TCL_OK;
}

static int
WarpVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Warp>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Warp>("all", state);
    }
    return TCL_OK;
}

static int
WarpWarpScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double scale;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &scale) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setWarpWarpScale(name, scale);
    } else {
        g_renderer->setWarpWarpScale("all", scale);
    }
    return TCL_OK;
}

static int
WarpWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Warp>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Warp>("all", state);
    }
    return TCL_OK;
}

static Rappture::CmdSpec warpOps[] = {
    {"add",          1, WarpAddOp, 2, 3, "?dataSetName?"},
    {"colormap",     1, WarpColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",       1, WarpDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, WarpEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",     3, WarpLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, WarpLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, WarpLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",      2, WarpOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, WarpOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, WarpPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",        2, WarpScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",      1, WarpVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"warpscale",    2, WarpWarpScaleOp, 3, 4, "value ?dataSetName?"},
    {"wireframe",    2, WarpWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nWarpOps = NumCmdSpecs(warpOps);

static int
WarpCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nWarpOps, warpOps,
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
Rappture::VtkVis::processCommands(Tcl_Interp *interp, ReadBuffer *inBufPtr, 
                                  int fdOut)
{
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
            status = ExecuteCommand(interp, &command);
            if (status == TCL_BREAK) {
                return 1;               /* This was caused by a "imgflush"
                                         * command. Break out of the read loop
                                         * and allow a new image to be
                                         * rendered. */
            }
        }
        tv.tv_sec = tv.tv_usec = 0L;    /* On successive reads, we break out
                                         * if no data is available. */
        FD_SET(inBufPtr->file(), &readFds);
        tvPtr = &tv;
    }

    if (status != TCL_OK) {
        const char *string;
        int nBytes;

        string = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
        TRACE("%s: status=%d ERROR errorInfo=(%s)", Tcl_DStringValue(&command),
              status, string);
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
            return -1;
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
void
Rappture::VtkVis::initTcl(Tcl_Interp *interp, ClientData clientData)
{
    Tcl_MakeSafe(interp);
    Tcl_CreateObjCommand(interp, "axis",        AxisCmd,        clientData, NULL);
    Tcl_CreateObjCommand(interp, "box",         BoxCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "camera",      CameraCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "colormap",    ColorMapCmd,    clientData, NULL);
    Tcl_CreateObjCommand(interp, "contour2d",   Contour2DCmd,   clientData, NULL);
    Tcl_CreateObjCommand(interp, "contour3d",   Contour3DCmd,   clientData, NULL);
    Tcl_CreateObjCommand(interp, "cutplane",    CutplaneCmd,    clientData, NULL);
    Tcl_CreateObjCommand(interp, "dataset",     DataSetCmd,     clientData, NULL);
    Tcl_CreateObjCommand(interp, "glyphs",      GlyphsCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "heightmap",   HeightMapCmd,   clientData, NULL);
    Tcl_CreateObjCommand(interp, "imgflush",    ImageFlushCmd,  clientData, NULL);
    Tcl_CreateObjCommand(interp, "legend",      LegendCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "lic",         LICCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "molecule",    MoleculeCmd,    clientData, NULL);
    Tcl_CreateObjCommand(interp, "polydata",    PolyDataCmd,    clientData, NULL);
    Tcl_CreateObjCommand(interp, "pseudocolor", PseudoColorCmd, clientData, NULL);
    Tcl_CreateObjCommand(interp, "renderer",    RendererCmd,    clientData, NULL);
    Tcl_CreateObjCommand(interp, "screen",      ScreenCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "sphere",      SphereCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "streamlines", StreamlinesCmd, clientData, NULL);
    Tcl_CreateObjCommand(interp, "volume",      VolumeCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "warp",        WarpCmd,        clientData, NULL);
}

/**
 * \brief Delete Tcl commands and interpreter
 */
void Rappture::VtkVis::exitTcl(Tcl_Interp *interp)
{

    Tcl_DeleteCommand(interp, "axis");
    Tcl_DeleteCommand(interp, "box");
    Tcl_DeleteCommand(interp, "camera");
    Tcl_DeleteCommand(interp, "colormap");
    Tcl_DeleteCommand(interp, "contour2d");
    Tcl_DeleteCommand(interp, "contour3d");
    Tcl_DeleteCommand(interp, "cutplane");
    Tcl_DeleteCommand(interp, "dataset");
    Tcl_DeleteCommand(interp, "glyphs");
    Tcl_DeleteCommand(interp, "heightmap");
    Tcl_DeleteCommand(interp, "imgflush");
    Tcl_DeleteCommand(interp, "legend");
    Tcl_DeleteCommand(interp, "lic");
    Tcl_DeleteCommand(interp, "molecule");
    Tcl_DeleteCommand(interp, "polydata");
    Tcl_DeleteCommand(interp, "pseudocolor");
    Tcl_DeleteCommand(interp, "renderer");
    Tcl_DeleteCommand(interp, "screen");
    Tcl_DeleteCommand(interp, "sphere");
    Tcl_DeleteCommand(interp, "streamlines");
    Tcl_DeleteCommand(interp, "volume");
    Tcl_DeleteCommand(interp, "warp");

    Tcl_DeleteInterp(interp);
}
