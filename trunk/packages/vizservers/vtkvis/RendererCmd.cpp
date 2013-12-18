/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
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
#include "RendererGraphicsObjs.h"
#include "PPMWriter.h"
#include "TGAWriter.h"
#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

using namespace VtkVis;

static int lastCmdStatus;

#ifdef USE_THREADS
void
VtkVis::queueResponse(ClientData clientData,
                      const void *bytes, size_t len, 
                      Response::AllocationType allocType,
                      Response::ResponseType type)
{
    ResponseQueue *queue = (ResponseQueue *)clientData;

    Response *response = new Response(type);
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
ArcAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    double center[3], pt1[3], norm[3], angle;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &center[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &center[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &center[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &pt1[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[6], &pt1[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[7], &pt1[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[8], &norm[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[9], &norm[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[10], &norm[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[11], &angle) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[12]);
    if (!g_renderer->addArc(name, center, pt1, norm, angle)) {
        Tcl_AppendResult(interp, "Failed to create arc", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ArcDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Arc>(name);
    } else {
        g_renderer->deleteGraphicsObject<Arc>("all");
    }
    return TCL_OK;
}

static int
ArcColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Arc>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Arc>("all", color);
    }
    return TCL_OK;
}

static int
ArcLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Arc>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Arc>("all", width);
    }
    return TCL_OK;
}

static int
ArcOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Arc>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Arc>("all", opacity);
    }
    return TCL_OK;
}

static int
ArcOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Arc>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Arc>("all", quat);
    }
    return TCL_OK;
}

static int
ArcOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Arc>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Arc>("all", origin);
    }
    return TCL_OK;
}

static int
ArcPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Arc>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Arc>("all", pos);
    }
    return TCL_OK;
}

static int
ArcResolutionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    int res;
    if (Tcl_GetIntFromObj(interp, objv[2], &res) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setArcResolution(name, res);
    } else {
        g_renderer->setArcResolution("all", res);
    }
    return TCL_OK;
}

static int
ArcScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Arc>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Arc>("all", scale);
    }
    return TCL_OK;
}

static int
ArcVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Arc>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Arc>("all", state);
    }
    return TCL_OK;
}

static CmdSpec arcOps[] = {
    {"add",       1, ArcAddOp, 13, 13, "centerX centerY centerZ startX startY startZ normX normY normZ angle name"},
    {"color",     1, ArcColorOp, 5, 6, "r g b ?name?"},
    {"delete",    1, ArcDeleteOp, 2, 3, "?name?"},
    {"linecolor", 5, ArcColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, ArcLineWidthOp, 3, 4, "width ?name?"},
    {"opacity",   2, ArcOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, ArcOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, ArcOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       2, ArcPositionOp, 5, 6, "x y z ?name?"},
    {"resolution",1, ArcResolutionOp, 3, 4, "res ?name?"},
    {"scale",     2, ArcScaleOp, 5, 6, "sx sy sz ?name?"},
    {"visible",   1, ArcVisibleOp, 3, 4, "bool ?name?"}
};
static int nArcOps = NumCmdSpecs(arcOps);

static int
ArcCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nArcOps, arcOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
ArrowAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    double tipRadius, shaftRadius, tipLength;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &tipRadius) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &shaftRadius) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &tipLength) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[5]);
    if (!g_renderer->addArrow(name, tipRadius, shaftRadius, tipLength)) {
        Tcl_AppendResult(interp, "Failed to create arrow", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ArrowDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Arrow>(name);
    } else {
        g_renderer->deleteGraphicsObject<Arrow>("all");
    }
    return TCL_OK;
}

static int
ArrowColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Arrow>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Arrow>("all", color);
    }
    return TCL_OK;
}

static int
ArrowCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<Arrow>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<Arrow>("all", state);
    }
    return TCL_OK;
}

static int
ArrowEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Arrow>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Arrow>("all", state);
    }
    return TCL_OK;
}

static int
ArrowFlipNormalsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectFlipNormals<Arrow>(name, state);
    } else {
        g_renderer->setGraphicsObjectFlipNormals<Arrow>("all", state);
    }
    return TCL_OK;
}

static int
ArrowLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Arrow>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Arrow>("all", state);
    }
    return TCL_OK;
}

static int
ArrowLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Arrow>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Arrow>("all", color);
    }
    return TCL_OK;
}

static int
ArrowLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Arrow>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Arrow>("all", width);
    }
    return TCL_OK;
}

static int
ArrowMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectAmbient<Arrow>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Arrow>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Arrow>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Arrow>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Arrow>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Arrow>("all", specCoeff, specPower);
    }
    return TCL_OK;
}

static int
ArrowOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Arrow>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Arrow>("all", opacity);
    }
    return TCL_OK;
}

static int
ArrowOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Arrow>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Arrow>("all", quat);
    }
    return TCL_OK;
}

static int
ArrowOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Arrow>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Arrow>("all", origin);
    }
    return TCL_OK;
}

static int
ArrowPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Arrow>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Arrow>("all", pos);
    }
    return TCL_OK;
}

static int
ArrowResolutionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    int tipRes, shaftRes;
    if (Tcl_GetIntFromObj(interp, objv[2], &tipRes) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &shaftRes) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setArrowResolution(name, tipRes, shaftRes);
    } else {
        g_renderer->setArrowResolution("all", tipRes, shaftRes);
    }
    return TCL_OK;
}

static int
ArrowScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Arrow>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Arrow>("all", scale);
    }
    return TCL_OK;
}

static int
ArrowShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<Arrow>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<Arrow>("all", shadeModel);
    }
    return TCL_OK;
}

static int
ArrowVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Arrow>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Arrow>("all", state);
    }
    return TCL_OK;
}

static int
ArrowWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Arrow>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Arrow>("all", state);
    }
    return TCL_OK;
}

static CmdSpec arrowOps[] = {
    {"add",       1, ArrowAddOp, 6, 6, "tipRadius shaftRadius tipLength name"},
    {"color",     2, ArrowColorOp, 5, 6, "r g b ?name?"},
    {"culling",   2, ArrowCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, ArrowDeleteOp, 2, 3, "?name?"},
    {"edges",     1, ArrowEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"flipnorms", 1, ArrowFlipNormalsOp, 3, 4, "bool ?name?"},
    {"lighting",  3, ArrowLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, ArrowLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, ArrowLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, ArrowMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, ArrowOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, ArrowOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, ArrowOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, ArrowPositionOp, 5, 6, "x y z ?name?"},
    {"resolution",1, ArrowResolutionOp, 4, 5, "tipRes shaftRes ?name?"},
    {"scale",     2, ArrowScaleOp, 5, 6, "sx sy sz ?name?"},
    {"shading",   2, ArrowShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, ArrowVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, ArrowWireframeOp, 3, 4, "bool ?name?"}
};
static int nArrowOps = NumCmdSpecs(arrowOps);

static int
ArrowCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nArrowOps, arrowOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
AxisAutoBoundsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool enableAuto;
    if (GetBooleanFromObj(interp, objv[3], &enableAuto) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisAutoBounds(X_AXIS, enableAuto);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisAutoBounds(Y_AXIS, enableAuto);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisAutoBounds(Z_AXIS, enableAuto);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesAutoBounds(enableAuto);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisAutoRangeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool enableAuto;
    if (GetBooleanFromObj(interp, objv[3], &enableAuto) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisAutoRange(X_AXIS, enableAuto);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisAutoRange(Y_AXIS, enableAuto);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisAutoRange(Z_AXIS, enableAuto);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesAutoRange(enableAuto);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double color[3];
    if (Tcl_GetDoubleFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    double opacity = 1.0;
    if (objc > 6) {
        if (Tcl_GetDoubleFromObj(interp, objv[6], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisColor(X_AXIS, color, opacity);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisColor(Y_AXIS, color, opacity);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisColor(Z_AXIS, color, opacity);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesColor(color, opacity);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisBoundsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double min, max;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &min) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &max) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisBounds(X_AXIS, min, max);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisBounds(Y_AXIS, min, max);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisBounds(Z_AXIS, min, max);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesBounds(min, max);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisExponentOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    int xPow, yPow, zPow;
    bool useCustom = true;
    if (Tcl_GetIntFromObj(interp, objv[2], &xPow) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &yPow) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[4], &zPow) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 5) {
        if (GetBooleanFromObj(interp, objv[5], &useCustom) != TCL_OK) {
            return TCL_ERROR;
        }
        g_renderer->setAxesLabelPowerScaling(xPow, yPow, zPow, useCustom);
    } else {
        g_renderer->setAxesLabelPowerScaling(xPow, yPow, zPow);
    }
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
AxisFontSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double screenSize;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &screenSize) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setAxesPixelFontSize(screenSize);
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
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisGridColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double color[3];
    if (Tcl_GetDoubleFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    double opacity = 1.0;
    if (objc > 6) {
        if (Tcl_GetDoubleFromObj(interp, objv[6], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisGridlinesColor(X_AXIS, color, opacity);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisGridlinesColor(Y_AXIS, color, opacity);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisGridlinesColor(Z_AXIS, color, opacity);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesGridlinesColor(color, opacity);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisGridpolysOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisGridpolysVisibility(X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisGridpolysVisibility(Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisGridpolysVisibility(Z_AXIS, visible);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesGridpolysVisibility(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisGridpolysColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    double color[3];
    if (Tcl_GetDoubleFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    double opacity = 1.0;
    if (objc > 6) {
        if (Tcl_GetDoubleFromObj(interp, objv[6], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisGridpolysColor(X_AXIS, color, opacity);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisGridpolysColor(Y_AXIS, color, opacity);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisGridpolysColor(Z_AXIS, color, opacity);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesGridpolysColor(color, opacity);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisInnerGridOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisInnerGridVisibility(X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisInnerGridVisibility(Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisInnerGridVisibility(Z_AXIS, visible);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesInnerGridVisibility(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisInnerGridColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    double color[3];
    if (Tcl_GetDoubleFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    double opacity = 1.0;
    if (objc > 6) {
        if (Tcl_GetDoubleFromObj(interp, objv[6], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisInnerGridlinesColor(X_AXIS, color, opacity);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisInnerGridlinesColor(Y_AXIS, color, opacity);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisInnerGridlinesColor(Z_AXIS, color, opacity);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesInnerGridlinesColor(color, opacity);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisLabelColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double color[3];
    if (Tcl_GetDoubleFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    double opacity = 1.0;
    if (objc > 6) {
        if (Tcl_GetDoubleFromObj(interp, objv[6], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisLabelColor(X_AXIS, color, opacity);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisLabelColor(Y_AXIS, color, opacity);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisLabelColor(Z_AXIS, color, opacity);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesLabelColor(color, opacity);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisLabelFontOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    const char *fontName = Tcl_GetString(objv[3]);

    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisLabelFont(X_AXIS, fontName);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisLabelFont(Y_AXIS, fontName);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisLabelFont(Z_AXIS, fontName);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesLabelFont(fontName);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisLabelFormatOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    const char *format = Tcl_GetString(objv[3]);

    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisLabelFormat(X_AXIS, format);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisLabelFormat(Y_AXIS, format);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisLabelFormat(Z_AXIS, format);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesLabelFormat(format);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisLabelFontSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    int fontSize;
    if (Tcl_GetIntFromObj(interp, objv[3], &fontSize) != TCL_OK) {
        return TCL_ERROR;
    }

    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisLabelFontSize(X_AXIS, fontSize);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisLabelFontSize(Y_AXIS, fontSize);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisLabelFontSize(Z_AXIS, fontSize);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesLabelFontSize(fontSize);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisLabelOrientationOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double rot;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &rot) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisLabelOrientation(X_AXIS, rot);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisLabelOrientation(Y_AXIS, rot);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisLabelOrientation(Z_AXIS, rot);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesLabelOrientation(rot);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisLabelScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool autoScale;
    int xpow, ypow, zpow;
    if (GetBooleanFromObj(interp, objv[3], &autoScale) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[4], &xpow) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[5], &ypow) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[6], &zpow) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setAxesLabelScaling(autoScale, xpow, ypow, zpow);
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
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double color[3];
    if (Tcl_GetDoubleFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    double opacity = 1.0;
    if (objc > 6) {
        if (Tcl_GetDoubleFromObj(interp, objv[6], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisLinesColor(X_AXIS, color, opacity);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisLinesColor(Y_AXIS, color, opacity);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisLinesColor(Z_AXIS, color, opacity);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesLinesColor(color, opacity);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisNameOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    const char *title = Tcl_GetString(objv[3]);
    if (strlen(title) > 60) {
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
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double x, y, z;
    bool useCustom = true;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &z) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 5) {
        if (GetBooleanFromObj(interp, objv[5], &useCustom) != TCL_OK) {
            return TCL_ERROR;
        }
        g_renderer->setAxesOrigin(x, y, z, useCustom);
    } else {
        g_renderer->setAxesOrigin(x, y, z);
    }
    return TCL_OK;
}

static int
AxisRangeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double min, max;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &min) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &max) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisRange(X_AXIS, min, max);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisRange(Y_AXIS, min, max);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisRange(Z_AXIS, min, max);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesRange(min, max);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double scale;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &scale) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisScale(X_AXIS, scale);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisScale(Y_AXIS, scale);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisScale(Z_AXIS, scale);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesScale(scale);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
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
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisMinorTicksVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisMinorTickVisibility(X_AXIS, visible);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisMinorTickVisibility(Y_AXIS, visible);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisMinorTickVisibility(Z_AXIS, visible);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesMinorTickVisibility(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
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
AxisTitleColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double color[3];
    if (Tcl_GetDoubleFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    double opacity = 1.0;
    if (objc > 6) {
        if (Tcl_GetDoubleFromObj(interp, objv[6], &opacity) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisTitleColor(X_AXIS, color, opacity);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisTitleColor(Y_AXIS, color, opacity);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisTitleColor(Z_AXIS, color, opacity);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesTitleColor(color, opacity);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisTitleFontOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    const char *fontName = Tcl_GetString(objv[3]);

    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisTitleFont(X_AXIS, fontName);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisTitleFont(Y_AXIS, fontName);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisTitleFont(Z_AXIS, fontName);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesTitleFont(fontName);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisTitleFontSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    int fontSize;
    if (Tcl_GetIntFromObj(interp, objv[3], &fontSize) != TCL_OK) {
        return TCL_ERROR;
    }

    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisTitleFontSize(X_AXIS, fontSize);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisTitleFontSize(Y_AXIS, fontSize);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisTitleFontSize(Z_AXIS, fontSize);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesTitleFontSize(fontSize);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisTitleOrientationOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double rot;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &rot) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'x') && (strcmp(string, "x") == 0)) {
        g_renderer->setAxisTitleOrientation(X_AXIS, rot);
    } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
        g_renderer->setAxisTitleOrientation(Y_AXIS, rot);
    } else if ((c == 'z') && (strcmp(string, "z") == 0)) {
        g_renderer->setAxisTitleOrientation(Z_AXIS, rot);
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        g_renderer->setAxesTitleOrientation(rot);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
AxisUnitsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    const char *units = Tcl_GetString(objv[3]);
    if (strlen(units) > 20) {
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
                         "\": should be axisName", (char*)NULL);
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
                         "\": should be axisName", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static CmdSpec axisOps[] = {
    {"autobounds", 5, AxisAutoBoundsOp, 4, 4, "axis bool"},
    {"autorange",  5, AxisAutoRangeOp, 4, 4, "axis bool"},
    {"bounds",     1, AxisBoundsOp, 5, 5, "axis min max"},
    {"color",      1, AxisColorOp, 6, 7, "axis r g b ?opacity?"},
    {"exp",        1, AxisExponentOp, 5, 6, "xPow yPow zPow ?useCustom?"},
    {"flymode",    2, AxisFlyModeOp, 3, 3, "mode"},
    {"fontsz",     2, AxisFontSizeOp, 3, 3, "fontPixelSize"},
    {"gpcolor",    3, AxisGridpolysColorOp, 6, 7, "axis r g b ?opacity?"},
    {"gpolys",     3, AxisGridpolysOp, 4, 4, "axis bool"},
    {"grcolor",    3, AxisGridColorOp, 6, 7, "axis r g b ?opacity?"},
    {"grid",       3, AxisGridOp, 4, 4, "axis bool"},
    {"igcolor",    3, AxisInnerGridColorOp, 6, 7, "axis r g b ?opacity?"},
    {"igrid",      3, AxisInnerGridOp, 4, 4, "axis bool"},
    {"labels",     2, AxisLabelsVisibleOp, 4, 4, "axis bool"},
    {"lcolor",     2, AxisLabelColorOp, 6, 7, "axis r g b ?opacity?"},
    {"lfont",      4, AxisLabelFontOp, 4, 4, "axis font"},
    {"lformat",    4, AxisLabelFormatOp, 4, 4, "axis format"},
    {"lfsize",     3, AxisLabelFontSizeOp, 4, 4, "axis fontSize"},
    {"linecolor",  2, AxisLineColorOp, 6, 7, "axis r g b ?opacity?"},
    {"lrot",       2, AxisLabelOrientationOp, 4, 4, "axis rot"},
    {"lscale",     2, AxisLabelScaleOp, 7, 7, "axis bool xpow ypow zpow"},
    {"minticks",   1, AxisMinorTicksVisibleOp, 4, 4, "axis bool"},
    {"name",       1, AxisNameOp, 4, 4, "axis title"},
    {"origin",     1, AxisOriginOp, 5, 6, "x y z ?useCustom?"},
    {"range",      1, AxisRangeOp, 5, 5, "axis min max"},
    {"scale",      1, AxisScaleOp, 4, 4, "axis scale"},
    {"tcolor",     2, AxisTitleColorOp, 6, 7, "axis r g b ?opacity?"},
    {"tfont",      3, AxisTitleFontOp, 4, 4, "axis font"},
    {"tfsize",     3, AxisTitleFontSizeOp, 4, 4, "axis fontSize"},
    {"tickpos",    5, AxisTickPositionOp, 3, 3, "position"},
    {"ticks",      5, AxisTicksVisibleOp, 4, 4, "axis bool"},
    {"trot",       2, AxisTitleOrientationOp, 4, 4, "axis rot"},
    {"units",      1, AxisUnitsOp, 4, 4, "axis units"},
    {"visible",    1, AxisVisibleOp, 4, 4, "axis bool"}
};
static int nAxisOps = NumCmdSpecs(axisOps);

static int
AxisCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nAxisOps, axisOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
BoxAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    double xLen, yLen, zLen;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &xLen) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &yLen) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &zLen) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[5]);
    if (!g_renderer->addBox(name, xLen, yLen, zLen)) {
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
BoxCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<Box>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<Box>("all", state);
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
BoxFlipNormalsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectFlipNormals<Box>(name, state);
    } else {
        g_renderer->setGraphicsObjectFlipNormals<Box>("all", state);
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
BoxOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Box>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Box>("all", origin);
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
BoxShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<Box>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<Box>("all", shadeModel);
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

static CmdSpec boxOps[] = {
    {"add",       1, BoxAddOp, 6, 6, "xLen yLen zLen name"},
    {"color",     2, BoxColorOp, 5, 6, "r g b ?name?"},
    {"culling",   2, BoxCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, BoxDeleteOp, 2, 3, "?name?"},
    {"edges",     1, BoxEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"flipnorms", 1, BoxFlipNormalsOp, 3, 4, "bool ?name?"},
    {"lighting",  3, BoxLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, BoxLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, BoxLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, BoxMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, BoxOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, BoxOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, BoxOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, BoxPositionOp, 5, 6, "x y z ?name?"},
    {"scale",     2, BoxScaleOp, 5, 6, "sx sy sz ?name?"},
    {"shading",   2, BoxShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, BoxVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, BoxWireframeOp, 3, 4, "bool ?name?"}
};
static int nBoxOps = NumCmdSpecs(boxOps);

static int
BoxCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nBoxOps, boxOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
CameraAspectOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Renderer::Aspect aspect;
    const char *string = Tcl_GetString(objv[2]);
    if (string[0] == 'n' && (strcmp(string, "native") == 0)) {
        aspect = Renderer::ASPECT_NATIVE;
    } else if (string[0] == 's' && (strcmp(string, "square") == 0)) {
        aspect = Renderer::ASPECT_SQUARE;
    } else if (string[0] == 'w' && (strcmp(string, "window") == 0)) {
        aspect = Renderer::ASPECT_WINDOW;
    } else {
        Tcl_AppendResult(interp, "bad camera aspect option \"", string,
                         "\": should be native, square or window", (char*)NULL);
        return TCL_ERROR;
    }
    g_renderer->setCameraAspect(aspect);
    return TCL_OK;
}

static int
CameraModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Renderer::CameraMode mode;
    const char *string = Tcl_GetString(objv[2]);
    if (string[0] == 'p' && (strcmp(string, "persp") == 0)) {
        mode = Renderer::PERSPECTIVE;
    } else if (string[0] == 'o' && (strcmp(string, "ortho") == 0)) {
        mode = Renderer::ORTHO;
    } else if (string[0] == 'i' && (strcmp(string, "image") == 0)) {
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
    queueResponse(clientData, buf, strlen(buf), Response::VOLATILE);
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
        if (!g_renderer->setCameraZoomRegionPixels(x, y, width, height)) {
            Tcl_AppendResult(interp, "Camera mode is not image", (char*)NULL);
            return TCL_ERROR;
        }
    } else if (string[0] == 'w' && strcmp(string, "world") == 0) {
        double x, y, width, height;
        if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
            Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK ||
            Tcl_GetDoubleFromObj(interp, objv[5], &width) != TCL_OK ||
            Tcl_GetDoubleFromObj(interp, objv[6], &height) != TCL_OK) {
            return TCL_ERROR;
        }
        if (!g_renderer->setCameraZoomRegion(x, y, width, height)) {
            Tcl_AppendResult(interp, "Camera mode is not image", (char*)NULL);
            return TCL_ERROR;
        }
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

static CmdSpec cameraOps[] = {
    {"aspect",  1, CameraAspectOp, 3, 3, "aspect"},
    {"get",     1, CameraGetOp, 2, 2, ""},
    {"mode",    1, CameraModeOp, 3, 3, "mode"},
    {"orient",  3, CameraOrientOp, 6, 6, "qw qx qy qz"},
    {"ortho",   1, CameraOrthoOp, 7, 7, "coordMode x y width height"},
    {"pan",     1, CameraPanOp, 4, 4, "panX panY"},
    {"reset",   2, CameraResetOp, 2, 3, "?all?"},
    {"rotate",  2, CameraRotateOp, 5, 5, "angle angle angle"},
    {"set",     1, CameraSetOp, 11, 11, "posX posY posZ focalPtX focalPtY focalPtZ viewUpX viewUpY viewUpZ"},
    {"zoom",    1, CameraZoomOp, 3, 3, "zoomAmount"}
};
static int nCameraOps = NumCmdSpecs(cameraOps);

static int
CameraCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nCameraOps, cameraOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
    int fd = VtkVis::getStatsFile(interp, objv[1]);
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
        objPtr = Tcl_NewStringObj("vtkvis", 6);
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
    strcpy(buf, ctime(&VtkVis::g_stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(buf, -1));
    /* date_secs */
    Tcl_ListObjAppendElement(interp, listObjPtr, 
                             Tcl_NewStringObj("date_secs", 9));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
                             Tcl_NewLongObj(VtkVis::g_stats.start.tv_sec));
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
    result = VtkVis::writeToStatsFile(fd, Tcl_DStringValue(&ds), 
                                      Tcl_DStringLength(&ds));
    Tcl_DStringFree(&ds);
    Tcl_DecrRefCount(listObjPtr);
    return result;
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

static int
ColorMapNumTableEntriesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    int numEntries;
    if (Tcl_GetIntFromObj(interp, objv[2], &numEntries) != TCL_OK) {
        const char *str = Tcl_GetString(objv[2]);
        if (str[0] == 'd' && strcmp(str, "default") == 0) {
            numEntries = -1;
        } else {
            Tcl_AppendResult(interp, "bad colormap resolution value \"", str,
                             "\": should be a positive integer or \"default\"", (char*)NULL);
            return TCL_ERROR;
        }
    } else if (numEntries < 1) {
        Tcl_AppendResult(interp, "bad colormap resolution value \"", Tcl_GetString(objv[2]),
                         "\": should be a positive integer or \"default\"", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);

        g_renderer->setColorMapNumberOfTableEntries(name, numEntries);
    } else {
        g_renderer->setColorMapNumberOfTableEntries("all", numEntries);
    }
    return TCL_OK;
}

static CmdSpec colorMapOps[] = {
    {"add",    1, ColorMapAddOp,             5, 5, "colorMapName colormap alphamap"},
    {"define", 3, ColorMapAddOp,             5, 5, "colorMapName colormap alphamap"},
    {"delete", 3, ColorMapDeleteOp,          2, 3, "?colorMapName?"},
    {"res",    1, ColorMapNumTableEntriesOp, 3, 4, "numTableEntries ?colorMapName?"}
};
static int nColorMapOps = NumCmdSpecs(colorMapOps);

static int
ColorMapCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nColorMapOps, colorMapOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
ConeAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    double radius, height;
    bool cap;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &radius) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &height) != TCL_OK ||
        GetBooleanFromObj(interp, objv[4], &cap) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[5]);
    if (!g_renderer->addCone(name, radius, height, cap)) {
        Tcl_AppendResult(interp, "Failed to create cone", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ConeDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Cone>(name);
    } else {
        g_renderer->deleteGraphicsObject<Cone>("all");
    }
    return TCL_OK;
}

static int
ConeColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Cone>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Cone>("all", color);
    }
    return TCL_OK;
}

static int
ConeCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<Cone>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<Cone>("all", state);
    }
    return TCL_OK;
}

static int
ConeEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Cone>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Cone>("all", state);
    }
    return TCL_OK;
}

static int
ConeFlipNormalsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectFlipNormals<Cone>(name, state);
    } else {
        g_renderer->setGraphicsObjectFlipNormals<Cone>("all", state);
    }
    return TCL_OK;
}

static int
ConeLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Cone>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Cone>("all", state);
    }
    return TCL_OK;
}

static int
ConeLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Cone>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Cone>("all", color);
    }
    return TCL_OK;
}

static int
ConeLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Cone>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Cone>("all", width);
    }
    return TCL_OK;
}

static int
ConeMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectAmbient<Cone>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Cone>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Cone>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Cone>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Cone>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Cone>("all", specCoeff, specPower);
    }
    return TCL_OK;
}

static int
ConeOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Cone>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Cone>("all", opacity);
    }
    return TCL_OK;
}

static int
ConeOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Cone>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Cone>("all", quat);
    }
    return TCL_OK;
}

static int
ConeOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Cone>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Cone>("all", origin);
    }
    return TCL_OK;
}

static int
ConePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Cone>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Cone>("all", pos);
    }
    return TCL_OK;
}

static int
ConeResolutionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    int res;
    if (Tcl_GetIntFromObj(interp, objv[2], &res) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setConeResolution(name, res);
    } else {
        g_renderer->setConeResolution("all", res);
    }
    return TCL_OK;
}

static int
ConeScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Cone>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Cone>("all", scale);
    }
    return TCL_OK;
}

static int
ConeShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<Cone>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<Cone>("all", shadeModel);
    }
    return TCL_OK;
}

static int
ConeVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Cone>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Cone>("all", state);
    }
    return TCL_OK;
}

static int
ConeWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Cone>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Cone>("all", state);
    }
    return TCL_OK;
}

static CmdSpec coneOps[] = {
    {"add",       1, ConeAddOp, 6, 6, "radius height cap name"},
    {"color",     2, ConeColorOp, 5, 6, "r g b ?name?"},
    {"culling",   2, ConeCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, ConeDeleteOp, 2, 3, "?name?"},
    {"edges",     1, ConeEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"flipnorms", 1, ConeFlipNormalsOp, 3, 4, "bool ?name?"},
    {"lighting",  3, ConeLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, ConeLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, ConeLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, ConeMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, ConeOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, ConeOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, ConeOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, ConePositionOp, 5, 6, "x y z ?name?"},
    {"resolution",1, ConeResolutionOp, 3, 4, "res ?name?"},
    {"scale",     2, ConeScaleOp, 5, 6, "sx sy sz ?name?"},
    {"shading",   2, ConeShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, ConeVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, ConeWireframeOp, 3, 4, "bool ?name?"}
};
static int nConeOps = NumCmdSpecs(coneOps);

static int
ConeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nConeOps, coneOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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

static CmdSpec contour2dAddOps[] = {
    {"contourlist", 1, Contour2DAddContourListOp, 4, 5, "contourList ?dataSetName?"},
    {"numcontours", 1, Contour2DAddNumContoursOp, 4, 5, "numContours ?dataSetName?"}
};
static int nContour2dAddOps = NumCmdSpecs(contour2dAddOps);

static int
Contour2DAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nContour2dAddOps, contour2dAddOps,
                        CMDSPEC_ARG2, objc, objv, 0);
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
    if (str[0] == 'c' &&
        (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
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
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
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
Contour2DContourListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setContour2DContourList(name, contourList);
    } else {
        g_renderer->setContour2DContourList("all", contourList);
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
Contour2DNumContoursOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    int numContours;

    if (Tcl_GetIntFromObj(interp, objv[2], &numContours) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour2DNumContours(name, numContours);
    } else {
        g_renderer->setContour2DNumContours("all", numContours);
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

static CmdSpec contour2dOps[] = {
    {"add",          1, Contour2DAddOp, 4, 5, "oper value ?dataSetName?"},
    {"ccolor",       2, Contour2DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"color",        5, Contour2DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, Contour2DColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    7, Contour2DColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"contourlist",  3, Contour2DContourListOp, 3, 4, "contourList ?dataSetName?"},
    {"delete",       1, Contour2DDeleteOp, 2, 3, "?dataSetName?"},
    {"lighting",     3, Contour2DLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, Contour2DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, Contour2DLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"numcontours",  1, Contour2DNumContoursOp, 3, 4, "numContours ?dataSetName?"},
    {"opacity",      2, Contour2DOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, Contour2DOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, Contour2DPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",        1, Contour2DScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",      1, Contour2DVisibleOp, 3, 4, "bool ?dataSetName?"}
};
static int nContour2dOps = NumCmdSpecs(contour2dOps);

static int
Contour2DCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nContour2dOps, contour2dOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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

static CmdSpec contour3dAddOps[] = {
    {"contourlist", 1, Contour3DAddContourListOp, 4, 5, "contourList ?dataSetName?"},
    {"numcontours", 1, Contour3DAddNumContoursOp, 4, 5, "numContours ?dataSetName?"}
};
static int nContour3dAddOps = NumCmdSpecs(contour3dAddOps);

static int
Contour3DAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nContour3dAddOps, contour3dAddOps,
                        CMDSPEC_ARG2, objc, objv, 0);
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
Contour3DColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    Contour3D::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'c' &&
        (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
        mode = Contour3D::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = Contour3D::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = Contour3D::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = Contour3D::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = Contour3D::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = Contour3D::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == Contour3D::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setContour3DColorMode(name, mode, fieldName);
    } else {
        g_renderer->setContour3DColorMode("all", mode, fieldName);
    }
    return TCL_OK;
}

static int
Contour3DContourFieldOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    const char *fieldName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setContour3DContourField(dataSetName, fieldName);
    } else {
        g_renderer->setContour3DContourField("all", fieldName);
    }
    return TCL_OK;
}

static int
Contour3DContourListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setContour3DContourList(name, contourList);
    } else {
        g_renderer->setContour3DContourList("all", contourList);
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
Contour3DNumContoursOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    int numContours;

    if (Tcl_GetIntFromObj(interp, objv[2], &numContours) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setContour3DNumContours(name, numContours);
    } else {
        g_renderer->setContour3DNumContours("all", numContours);
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

static CmdSpec contour3dOps[] = {
    {"add",          1, Contour3DAddOp, 4, 5, "oper value ?dataSetName?"},
    {"ccolor",       2, Contour3DColorOp, 5, 6, "r g b ?dataSetName?"},
    {"color",        5, Contour3DColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, Contour3DColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    7, Contour3DColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"contourfield", 8, Contour3DContourFieldOp, 3, 4, "fieldName ?dataSetName?"},
    {"contourlist",  8, Contour3DContourListOp, 3, 4, "contourList ?dataSetName?"},
    {"delete",       1, Contour3DDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, Contour3DEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",     3, Contour3DLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, Contour3DLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, Contour3DLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"numcontours",  1, Contour3DNumContoursOp, 3, 4, "numContours ?dataSetName?"},
    {"opacity",      2, Contour3DOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, Contour3DOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, Contour3DPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",        1, Contour3DScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",      1, Contour3DVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe",    1, Contour3DWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nContour3dOps = NumCmdSpecs(contour3dOps);

static int
Contour3DCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nContour3dOps, contour3dOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
CutplaneCloudStyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    Cutplane::CloudStyle style;
    char *str =  Tcl_GetString(objv[2]);
    if (str[0] == 'm' && strcmp(str, "mesh") == 0) {
        style = Cutplane::CLOUD_MESH;
    } else if (str[0] == 's' && strcmp(str, "splat") == 0) {
        style = Cutplane::CLOUD_SPLAT;
    } else {
        Tcl_AppendResult(interp, "bad cloudstyle option \"", str,
                         "\": should be one of: 'mesh', 'splat'", (char*)NULL);
        return TCL_ERROR;
    }

    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setCutplaneCloudStyle(name, style);
    } else {
        g_renderer->setCutplaneCloudStyle("all", style);
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
CutplanePreInterpOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<Cutplane>(name, state);
    } else {
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<Cutplane>("all", state);
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

static CmdSpec cutplaneOps[] = {
    {"add",          2, CutplaneAddOp, 2, 3, "?dataSetName?"},
    {"axis",         2, CutplaneSliceVisibilityOp, 4, 5, "axis bool ?dataSetName?"},
    {"ccolor",       2, CutplaneColorOp, 5, 6, "r g b ?dataSetName?"},
    {"cloudstyle",   2, CutplaneCloudStyleOp, 3, 4, "style ?dataSetName?"},
    {"color",        5, CutplaneColorOp, 5, 6, "r g b ?dataSetName?"},
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
    {"pos",          2, CutplanePositionOp, 5, 6, "x y z ?dataSetName?"},
    {"preinterp",    2, CutplanePreInterpOp, 3, 4, "bool ?dataSetName?"},
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

    proc = GetOpFromObj(interp, nCutplaneOps, cutplaneOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
CylinderAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double radius, height;
    bool cap = true;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &radius) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &height) != TCL_OK ||
        GetBooleanFromObj(interp, objv[4], &cap) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[5]);
    if (!g_renderer->addCylinder(name, radius, height, cap)) {
        Tcl_AppendResult(interp, "Failed to create cylinder", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
CylinderDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Cylinder>(name);
    } else {
        g_renderer->deleteGraphicsObject<Cylinder>("all");
    }
    return TCL_OK;
}

static int
CylinderColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Cylinder>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Cylinder>("all", color);
    }
    return TCL_OK;
}

static int
CylinderCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<Cylinder>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<Cylinder>("all", state);
    }
    return TCL_OK;
}

static int
CylinderEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Cylinder>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Cylinder>("all", state);
    }
    return TCL_OK;
}

static int
CylinderFlipNormalsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectFlipNormals<Cylinder>(name, state);
    } else {
        g_renderer->setGraphicsObjectFlipNormals<Cylinder>("all", state);
    }
    return TCL_OK;
}

static int
CylinderLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Cylinder>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Cylinder>("all", state);
    }
    return TCL_OK;
}

static int
CylinderLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Cylinder>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Cylinder>("all", color);
    }
    return TCL_OK;
}

static int
CylinderLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Cylinder>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Cylinder>("all", width);
    }
    return TCL_OK;
}

static int
CylinderMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectAmbient<Cylinder>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Cylinder>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Cylinder>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Cylinder>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Cylinder>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Cylinder>("all", specCoeff, specPower);
    }
    return TCL_OK;
}

static int
CylinderOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Cylinder>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Cylinder>("all", opacity);
    }
    return TCL_OK;
}

static int
CylinderOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Cylinder>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Cylinder>("all", quat);
    }
    return TCL_OK;
}

static int
CylinderOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Cylinder>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Cylinder>("all", origin);
    }
    return TCL_OK;
}

static int
CylinderPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Cylinder>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Cylinder>("all", pos);
    }
    return TCL_OK;
}

static int
CylinderResolutionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    int res;
    if (Tcl_GetIntFromObj(interp, objv[2], &res) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setCylinderResolution(name, res);
    } else {
        g_renderer->setCylinderResolution("all", res);
    }
    return TCL_OK;
}

static int
CylinderScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Cylinder>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Cylinder>("all", scale);
    }
    return TCL_OK;
}

static int
CylinderShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<Cylinder>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<Cylinder>("all", shadeModel);
    }
    return TCL_OK;
}

static int
CylinderVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Cylinder>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Cylinder>("all", state);
    }
    return TCL_OK;
}

static int
CylinderWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Cylinder>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Cylinder>("all", state);
    }
    return TCL_OK;
}

static CmdSpec cylinderOps[] = {
    {"add",       1, CylinderAddOp, 6, 6, "radius height cap name"},
    {"color",     2, CylinderColorOp, 5, 6, "r g b ?name?"},
    {"culling",   2, CylinderCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, CylinderDeleteOp, 2, 3, "?name?"},
    {"edges",     1, CylinderEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"flipnorms", 1, CylinderFlipNormalsOp, 3, 4, "bool ?name?"},
    {"lighting",  3, CylinderLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, CylinderLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, CylinderLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, CylinderMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, CylinderOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, CylinderOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, CylinderOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, CylinderPositionOp, 5, 6, "x y z ?name?"},
    {"resolution",1, CylinderResolutionOp, 3, 4, "res ?name?"},
    {"scale",     2, CylinderScaleOp, 5, 6, "sx sy sz ?name?"},
    {"shading",   2, CylinderShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, CylinderVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, CylinderWireframeOp, 3, 4, "bool ?name?"}
};
static int nCylinderOps = NumCmdSpecs(cylinderOps);

static int
CylinderCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nCylinderOps, cylinderOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
    if (!g_renderer->setData(name, data, nbytes)) {
        USER_ERROR("Failed to load data for dataset \"%s\"", name);
        free(data);
        return TCL_ERROR;
    }
    g_stats.nDataSets++;
    g_stats.nDataBytes += nbytes;
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
    queueResponse(clientData, buf, length, Response::VOLATILE);
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
    queueResponse(clientData, buf, length, Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(buf, length);
    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif
    return TCL_OK;
}

static CmdSpec dataSetGetScalarOps[] = {
    {"pixel", 1, DataSetGetScalarPixelOp, 6, 6, "x y dataSetName"},
    {"world", 1, DataSetGetScalarWorldOp, 7, 7, "x y z dataSetName"}
};
static int nDataSetGetScalarOps = NumCmdSpecs(dataSetGetScalarOps);

static int
DataSetGetScalarOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nDataSetGetScalarOps, dataSetGetScalarOps,
                        CMDSPEC_ARG2, objc, objv, 0);
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
    queueResponse(clientData, buf, length, Response::VOLATILE);
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
    queueResponse(clientData, buf, length, Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(buf, length);

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif /*USE_THREADS*/
    return TCL_OK;
}

static CmdSpec dataSetGetVectorOps[] = {
    {"pixel", 1, DataSetGetVectorPixelOp, 6, 6, "x y dataSetName"},
    {"world", 1, DataSetGetVectorWorldOp, 7, 7, "x y z dataSetName"}
};
static int nDataSetGetVectorOps = NumCmdSpecs(dataSetGetVectorOps);

static int
DataSetGetVectorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nDataSetGetVectorOps, dataSetGetVectorOps,
                        CMDSPEC_ARG2, objc, objv, 0);
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
    if (value[0] == 'a' && strcmp(value, "all") == 0) {
        g_renderer->setUseCumulativeDataRange(true);
    } else if (value[0] == 'e' && strcmp(value, "explicit") == 0) {
        if (objc < 6 || objc > 9) {
            Tcl_AppendResult(interp, "wrong number of arguments for explicit maprange", (char*)NULL);
            return TCL_ERROR;
        }
        double range[2];
        if (Tcl_GetDoubleFromObj(interp, objv[3], &range[0]) != TCL_OK ||
            Tcl_GetDoubleFromObj(interp, objv[4], &range[1]) != TCL_OK) {
            return TCL_ERROR;
        }
        const char *fieldName = Tcl_GetString(objv[5]);

        DataSet::DataAttributeType type = DataSet::POINT_DATA;
        int numComponents = 1;
        int component = -1;
        if (objc > 6) {
            const char *fieldType = Tcl_GetString(objv[6]);
            if (fieldType[0] == 'p' && strcmp(fieldType, "point_data") == 0) {
                type = DataSet::POINT_DATA;
            } else if (fieldType[0] == 'c' && strcmp(fieldType, "cell_data") == 0) {
                type = DataSet::CELL_DATA;
            } else if (fieldType[0] == 'f' && strcmp(fieldType, "field_data") == 0) {
                type = DataSet::FIELD_DATA;
            } else {
                Tcl_AppendResult(interp, "bad field type option \"", fieldType,
                                 "\": should be point_data, cell_data or field_data", (char*)NULL);
                return TCL_ERROR;
            }
            if (objc > 7) {
                if (Tcl_GetIntFromObj(interp, objv[7], &numComponents) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (objc == 9) {
                    if (Tcl_GetIntFromObj(interp, objv[8], &component) != TCL_OK) {
                        return TCL_ERROR;
                    }
                }
            }
        }
        g_renderer->setCumulativeDataRange(range, fieldName, type, numComponents, component);
    } else if (value[0] == 's' && strcmp(value, "separate") == 0) {
        g_renderer->setUseCumulativeDataRange(false);
    } else if (value[0] == 'v' && strcmp(value, "visible") == 0) {
        g_renderer->setUseCumulativeDataRange(true, true);
    } else {
        Tcl_AppendResult(interp, "bad maprange option \"", value,
                         "\": should be all, explicit, separate or visible", (char*)NULL);
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
    queueResponse(clientData, oss.str().c_str(), len, Response::VOLATILE);
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

static CmdSpec dataSetOps[] = {
    {"add",       1, DataSetAddOp, 6, 6, "name data follows nBytes"},
    {"delete",    1, DataSetDeleteOp, 2, 3, "?name?"},
    {"getscalar", 4, DataSetGetScalarOp, 6, 7, "oper x y ?z? name"},
    {"getvector", 4, DataSetGetVectorOp, 6, 7, "oper x y ?z? name"},
    {"maprange",  1, DataSetMapRangeOp, 3, 9, "value ?min? ?max? ?fieldName? ?fieldType? ?fieldNumComp? ?compIdx?"},
    {"names",     1, DataSetNamesOp, 2, 2, ""},
    {"opacity",   2, DataSetOpacityOp, 3, 4, "value ?name?"},
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

    proc = GetOpFromObj(interp, nDataSetOps, dataSetOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
DiskAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    double innerRadius, outerRadius;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &innerRadius) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &outerRadius) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[4]);
    if (!g_renderer->addDisk(name, innerRadius, outerRadius)) {
        Tcl_AppendResult(interp, "Failed to create disk", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
DiskDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Disk>(name);
    } else {
        g_renderer->deleteGraphicsObject<Disk>("all");
    }
    return TCL_OK;
}

static int
DiskColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Disk>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Disk>("all", color);
    }
    return TCL_OK;
}

static int
DiskCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<Disk>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<Disk>("all", state);
    }
    return TCL_OK;
}

static int
DiskEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Disk>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Disk>("all", state);
    }
    return TCL_OK;
}

static int
DiskFlipNormalsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectFlipNormals<Disk>(name, state);
    } else {
        g_renderer->setGraphicsObjectFlipNormals<Disk>("all", state);
    }
    return TCL_OK;
}

static int
DiskLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Disk>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Disk>("all", state);
    }
    return TCL_OK;
}

static int
DiskLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Disk>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Disk>("all", color);
    }
    return TCL_OK;
}

static int
DiskLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Disk>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Disk>("all", width);
    }
    return TCL_OK;
}

static int
DiskMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectAmbient<Disk>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Disk>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Disk>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Disk>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Disk>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Disk>("all", specCoeff, specPower);
    }
    return TCL_OK;
}

static int
DiskOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Disk>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Disk>("all", opacity);
    }
    return TCL_OK;
}

static int
DiskOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Disk>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Disk>("all", quat);
    }
    return TCL_OK;
}

static int
DiskOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Disk>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Disk>("all", origin);
    }
    return TCL_OK;
}

static int
DiskPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Disk>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Disk>("all", pos);
    }
    return TCL_OK;
}

static int
DiskResolutionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    int radial, circum;
    if (Tcl_GetIntFromObj(interp, objv[2], &radial) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &circum) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setDiskResolution(name, radial, circum);
    } else {
        g_renderer->setDiskResolution("all", radial, circum);
    }
    return TCL_OK;
}

static int
DiskScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Disk>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Disk>("all", scale);
    }
    return TCL_OK;
}

static int
DiskShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<Disk>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<Disk>("all", shadeModel);
    }
    return TCL_OK;
}

static int
DiskVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Disk>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Disk>("all", state);
    }
    return TCL_OK;
}

static int
DiskWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Disk>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Disk>("all", state);
    }
    return TCL_OK;
}

static CmdSpec diskOps[] = {
    {"add",       1, DiskAddOp, 5, 5, "innerRadius outerRadius name"},
    {"color",     2, DiskColorOp, 5, 6, "r g b ?name?"},
    {"culling",   2, DiskCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, DiskDeleteOp, 2, 3, "?name?"},
    {"edges",     1, DiskEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"flipnorms", 1, DiskFlipNormalsOp, 3, 4, "bool ?name?"},
    {"lighting",  3, DiskLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, DiskLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, DiskLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, DiskMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, DiskOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, DiskOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, DiskOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, DiskPositionOp, 5, 6, "x y z ?name?"},
    {"resolution",1, DiskResolutionOp, 4, 5, "resRadial resCircum ?name?"},
    {"scale",     2, DiskScaleOp, 5, 6, "sx sy sz ?name?"},
    {"shading",   2, DiskShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, DiskVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, DiskWireframeOp, 3, 4, "bool ?name?"}
};
static int nDiskOps = NumCmdSpecs(diskOps);

static int
DiskCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nDiskOps, diskOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
    } else if (shapeOpt[0] == 'p' && strcmp(shapeOpt, "point") == 0) {
        shape = Glyphs::POINT;
    } else if (shapeOpt[0] == 's' && strcmp(shapeOpt, "sphere") == 0) {
        shape = Glyphs::SPHERE;
    } else if (shapeOpt[0] == 't' && strcmp(shapeOpt, "tetrahedron") == 0) {
        shape = Glyphs::TETRAHEDRON;
    } else {
        Tcl_AppendResult(interp, "bad shape option \"", shapeOpt,
                         "\": should be one of: 'arrow', 'cone', 'cube', 'cylinder', 'dodecahedron', 'icosahedron', 'octahedron', 'point', 'sphere', 'tetrahedron'", (char*)NULL);
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
    if (str[0] == 'c' &&
        (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
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
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
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
GlyphsMaxNumberOfGlyphsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    int max;
    bool random = true;
    int offset = 0;
    int ratio = 1;
    if (Tcl_GetIntFromObj(interp, objv[2], &max) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 3) {
        if (GetBooleanFromObj(interp, objv[3], &random) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[4], &offset) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[5], &ratio) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (objc == 7) {
        const char *name = Tcl_GetString(objv[6]);
        g_renderer->setGlyphsMaximumNumberOfGlyphs(name, max, random, offset, ratio);
    } else {
        g_renderer->setGlyphsMaximumNumberOfGlyphs("all", max, random, offset, ratio);
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
GlyphsPointSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    float size;
    if (GetFloatFromObj(interp, objv[2], &size) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectPointSize<Glyphs>(name, size);
    } else {
        g_renderer->setGraphicsObjectPointSize<Glyphs>("all", size);
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
GlyphsQualityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double quality;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quality) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGlyphsQuality(name, quality);
    } else {
        g_renderer->setGlyphsQuality("all", quality);
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
    } else if (shapeOpt[0] == 'p' && strcmp(shapeOpt, "point") == 0) {
        shape = Glyphs::POINT;
    } else if (shapeOpt[0] == 's' && strcmp(shapeOpt, "sphere") == 0) {
        shape = Glyphs::SPHERE;
    } else if (shapeOpt[0] == 't' && strcmp(shapeOpt, "tetrahedron") == 0) {
        shape = Glyphs::TETRAHEDRON;
    } else {
        Tcl_AppendResult(interp, "bad shape option \"", shapeOpt,
                         "\": should be one of: 'arrow', 'cone', 'cube', 'cylinder', 'dodecahedron', 'icosahedron', 'line', 'octahedron', 'point', 'sphere', 'tetrahedron'", (char*)NULL);
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

static CmdSpec glyphsOps[] = {
    {"add",          1, GlyphsAddOp, 3, 4, "shape ?dataSetName?"},
    {"ccolor",       2, GlyphsColorOp, 5, 6, "r g b ?dataSetName?"},
    {"color",        5, GlyphsColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, GlyphsColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    7, GlyphsColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"delete",       1, GlyphsDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, GlyphsEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"gorient",      2, GlyphsOrientGlyphsOp, 4, 5, "bool fieldName ?dataSetName?"},
    {"gscale",       2, GlyphsScaleFactorOp, 3, 4, "scaleFactor ?dataSetName?"},
    {"lighting",     3, GlyphsLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, GlyphsLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, GlyphsLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"normscale",    2, GlyphsNormalizeScaleOp, 3, 4, "bool ?dataSetName?"},
    {"numglyphs",    2, GlyphsMaxNumberOfGlyphsOp, 3, 7, "max ?random? ?offset? ?ratio? ?dataSetName?"},
    {"opacity",      2, GlyphsOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, GlyphsOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          2, GlyphsPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"ptsize",       2, GlyphsPointSizeOp, 3, 4, "size ?dataSetName?"},
    {"quality",      1, GlyphsQualityOp, 3, 4, "val ?dataSetName?"},
    {"scale",        2, GlyphsScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"shape",        2, GlyphsShapeOp, 3, 4, "shapeVal ?dataSetName?"},
    {"smode",        2, GlyphsScalingModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"visible",      1, GlyphsVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe",    1, GlyphsWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nGlyphsOps = NumCmdSpecs(glyphsOps);

static int
GlyphsCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nGlyphsOps, glyphsOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
GroupAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    int numNodes;
    Tcl_Obj **nodes = NULL;
    if (Tcl_ListObjGetElements(interp, objv[2], &numNodes, &nodes) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Group::NodeId> nodeList;
    for (int i = 0; i < numNodes; i++) {
        nodeList.push_back(Tcl_GetString(nodes[i]));
    }
    const char *name = Tcl_GetString(objv[3]);
    if (!g_renderer->addGroup(name, nodeList)) {
        Tcl_AppendResult(interp, "Failed to create group", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
GroupDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Group>(name);
    } else {
        g_renderer->deleteGraphicsObject<Group>("all");
    }
    return TCL_OK;
}

static int
GroupOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Group>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Group>("all", quat);
    }
    return TCL_OK;
}

static int
GroupOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Group>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Group>("all", origin);
    }
    return TCL_OK;
}

static int
GroupPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Group>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Group>("all", pos);
    }
    return TCL_OK;
}

static int
GroupRemoveChildOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    int numNodes;
    Tcl_Obj **nodes = NULL;
    if (Tcl_ListObjGetElements(interp, objv[2], &numNodes, &nodes) != TCL_OK) {
        return TCL_ERROR;
    }
    std::vector<Group::NodeId> nodeList;
    for (int i = 0; i < numNodes; i++) {
        nodeList.push_back(Tcl_GetString(nodes[i]));
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->removeChildrenFromGroup(name, nodeList);
    } else {
        g_renderer->removeChildrenFromGroup("all", nodeList);
    }
    return TCL_OK;
}

static int
GroupScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Group>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Group>("all", scale);
    }
    return TCL_OK;
}

static int
GroupVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Group>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Group>("all", state);
    }
    return TCL_OK;
}

static CmdSpec groupOps[] = {
    {"add",       1, GroupAddOp, 4, 4, "nodeList groupName"},
    {"delete",    1, GroupDeleteOp, 2, 3, "?name?"},
    {"orient",    4, GroupOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, GroupOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, GroupPositionOp, 5, 6, "x y z ?name?"},
    {"remove",    1, GroupRemoveChildOp, 3, 4, "nodeList ?name?"},
    {"scale",     1, GroupScaleOp, 5, 6, "sx sy sz ?name?"},
    {"visible",   1, GroupVisibleOp, 3, 4, "bool ?name?"},
};
static int nGroupOps = NumCmdSpecs(groupOps);

static int
GroupCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nGroupOps, groupOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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

static CmdSpec heightmapAddOps[] = {
    {"contourlist", 1, HeightMapAddContourListOp, 5, 6, "contourList heightscale ?dataSetName?"},
    {"numcontours", 1, HeightMapAddNumContoursOp, 5, 6, "numContours heightscale ?dataSetName?"}
};
static int nHeightmapAddOps = NumCmdSpecs(heightmapAddOps);

static int
HeightMapAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nHeightmapAddOps, heightmapAddOps,
                        CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
HeightMapAspectOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double aspect;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &aspect) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectAspect<HeightMap>(name, aspect);
    } else {
        g_renderer->setGraphicsObjectAspect<HeightMap>("all", aspect);
    }
    return TCL_OK;
}

static int
HeightMapCloudStyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    HeightMap::CloudStyle style;
    char *str =  Tcl_GetString(objv[2]);
    if (str[0] == 'm' && strcmp(str, "mesh") == 0) {
        style = HeightMap::CLOUD_MESH;
    } else if (str[0] == 's' && strcmp(str, "splat") == 0) {
        style = HeightMap::CLOUD_SPLAT;
    } else {
        Tcl_AppendResult(interp, "bad cloudstyle option \"", str,
                         "\": should be one of: 'mesh', 'splat'", (char*)NULL);
        return TCL_ERROR;
    }

    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapCloudStyle(name, style);
    } else {
        g_renderer->setHeightMapCloudStyle("all", style);
    }
    return TCL_OK;
}

static int
HeightMapColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<HeightMap>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<HeightMap>("all", color);
    }
    return TCL_OK;
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
HeightMapColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    HeightMap::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'c' &&
        (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
        mode = HeightMap::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = HeightMap::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = HeightMap::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = HeightMap::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = HeightMap::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = HeightMap::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == HeightMap::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setHeightMapColorMode(name, mode, fieldName);
    } else {
        g_renderer->setHeightMapColorMode("all", mode, fieldName);
    }
    return TCL_OK;
}

static int
HeightMapColorMode2Op(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    HeightMap::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'c' &&
        (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
        mode = HeightMap::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = HeightMap::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = HeightMap::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = HeightMap::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = HeightMap::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = HeightMap::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == HeightMap::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    str = Tcl_GetString(objv[4]);
    DataSet::DataAttributeType association = DataSet::POINT_DATA;
    bool haveAssoc = false;
    if (str[0] == 'p' && strcmp(str, "point_data") == 0) {
        association = DataSet::POINT_DATA;
        haveAssoc = true;
    } else if (str[0] == 'c' && strcmp(str, "cell_data") == 0) {
        association = DataSet::CELL_DATA;
        haveAssoc = true;
    } else if (str[0] == 'f' && strcmp(str, "field_data") == 0) {
        association = DataSet::FIELD_DATA;
        haveAssoc = true;
    } else if (str[0] == 'a' && strcmp(str, "any") == 0) {
        haveAssoc = false;
    } else {
        Tcl_AppendResult(interp, "bad color mode association option \"", str,
                         "\": should be one of: 'point_data', 'cell_data', 'field_data', 'any'", (char*)NULL);
        return TCL_ERROR;
    }
    double *range = NULL;
    if (objc >= 7) {
        range = new double[2];
        if (Tcl_GetDoubleFromObj(interp, objv[5], &range[0]) != TCL_OK ||
            Tcl_GetDoubleFromObj(interp, objv[6], &range[1]) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (objc == 6 || objc == 8) {
        const char *name = Tcl_GetString((objc == 6 ? objv[5] : objv[7]));
        if (haveAssoc) {
            g_renderer->setHeightMapColorMode(name, mode, association, fieldName, range);
        } else {
            g_renderer->setHeightMapColorMode(name, mode, fieldName, range);
        }
    } else {
        if (haveAssoc) {
            g_renderer->setHeightMapColorMode("all", mode, association, fieldName, range);
        } else {
            g_renderer->setHeightMapColorMode("all", mode, fieldName, range);
        }
    }
    if (range != NULL) {
        delete [] range;
    }
    return TCL_OK;
}

static int
HeightMapContourLineColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setHeightMapContourLineColorMapEnabled(name, state);
    } else {
        g_renderer->setHeightMapContourLineColorMapEnabled("all", state);
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
HeightMapPreInterpOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<HeightMap>(name, state);
    } else {
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<HeightMap>("all", state);
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

static CmdSpec heightmapOps[] = {
    {"add",          2, HeightMapAddOp, 5, 6, "oper value heightscale ?dataSetName?"},
    {"aspect",       2, HeightMapAspectOp, 3, 4, "aspect ?dataSetName?"},
    {"ccolor",       2, HeightMapColorOp, 5, 6, "r g b ?dataSetName?"},
    {"cloudstyle",   2, HeightMapCloudStyleOp, 3, 4, "style ?dataSetName?"},
    {"color",        5, HeightMapColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, HeightMapColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    9, HeightMapColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"colormode2",   10,HeightMapColorMode2Op, 5, 8, "mode fieldName association ?min max? ?dataSetName?"},
    {"contourlist",  3, HeightMapContourListOp, 3, 4, "contourList ?dataSetName?"},
    {"delete",       1, HeightMapDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, HeightMapEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"heightscale",  1, HeightMapHeightScaleOp, 3, 4, "value ?dataSetName?"},
    {"isolinecmap",  9, HeightMapContourLineColorMapOp, 3, 4, "bool ?dataSetName?"},
    {"isolinecolor", 9, HeightMapContourLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"isolines",     8, HeightMapContourLineVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"isolinewidth", 8, HeightMapContourLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"lighting",     3, HeightMapLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, HeightMapLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, HeightMapLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"numcontours",  1, HeightMapNumContoursOp, 3, 4, "numContours ?dataSetName?"},
    {"opacity",      2, HeightMapOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, HeightMapOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          2, HeightMapPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"preinterp",    2, HeightMapPreInterpOp, 3, 4, "bool ?dataSetName?"},
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

    proc = GetOpFromObj(interp, nHeightmapOps, heightmapOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
ImageAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addGraphicsObject<Image>(name)) {
            Tcl_AppendResult(interp, "Failed to create image", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<Image>("all")) {
            Tcl_AppendResult(interp, "Failed to create iamge for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
ImageBackgroundOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageBackground(name, state);
    } else {
        g_renderer->setImageBackground("all", state);
    }
    return TCL_OK;
}

static int
ImageBackingOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageBacking(name, state);
    } else {
        g_renderer->setImageBacking("all", state);
    }
    return TCL_OK;
}

static int
ImageBorderOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageBorder(name, state);
    } else {
        g_renderer->setImageBorder("all", state);
    }
    return TCL_OK;
}

static int
ImageColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Image>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Image>("all", color);
    }
    return TCL_OK;
}

static int
ImageColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectColorMap<Image>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<Image>("all", colorMapName);
    }
    return TCL_OK;
}

static int
ImageDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Image>(name);
    } else {
        g_renderer->deleteGraphicsObject<Image>("all");
    }
    return TCL_OK;
}

static int
ImageExtentsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    int extents[6];
    if (Tcl_GetIntFromObj(interp, objv[2], &extents[0]) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &extents[1]) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[4], &extents[2]) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[5], &extents[3]) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[6], &extents[4]) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[7], &extents[5]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 9) {
        const char *name = Tcl_GetString(objv[8]);
        g_renderer->setImageExtents(name, extents);
    } else {
        g_renderer->setImageExtents("all", extents);
    }
    return TCL_OK;
}

static int
ImageLevelOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double level;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &level) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageLevel(name, level);
    } else {
        g_renderer->setImageLevel("all", level);
    }
    return TCL_OK;
}

static int
ImageOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Image>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Image>("all", opacity);
    }
    return TCL_OK;
}

static int
ImageOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Image>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Image>("all", quat);
    }
    return TCL_OK;
}

static int
ImagePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Image>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Image>("all", pos);
    }
    return TCL_OK;
}

static int
ImageScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Image>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Image>("all", scale);
    }
    return TCL_OK;
}

static int
ImageVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Image>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Image>("all", state);
    }
    return TCL_OK;
}

static int
ImageWindowOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double window;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &window) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageWindow(name, window);
    } else {
        g_renderer->setImageWindow("all", window);
    }
    return TCL_OK;
}

static int
ImageZSliceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    int slice;
    if (Tcl_GetIntFromObj(interp, objv[2], &slice) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageZSlice(name, slice);
    } else {
        g_renderer->setImageZSlice("all", slice);
    }
    return TCL_OK;
}

static CmdSpec imageOps[] = {
    {"add",          1, ImageAddOp, 2, 3, "?dataSetName?"},
    {"backing",      2, ImageBackingOp, 3, 4, "bool ?dataSetName?"},
    {"bg",           2, ImageBackgroundOp, 3, 4, "bool ?dataSetName?"},
    {"border",       2, ImageBorderOp, 3, 4, "bool ?dataSetName?"},
    {"color",        5, ImageColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     6, ImageColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",       1, ImageDeleteOp, 2, 3, "?dataSetName?"},
    {"extents",      1, ImageExtentsOp, 8, 9, "xmin xmax ymin ymax zmin zmax ?dataSetName?"},
    {"level",        1, ImageLevelOp, 3, 4, "val ?dataSetName?"},
    {"opacity",      2, ImageOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, ImageOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, ImagePositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",        1, ImageScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",      1, ImageVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"window",       1, ImageWindowOp, 3, 4, "val ?dataSetName?"},
    {"zslice",       1, ImageZSliceOp, 3, 4, "val ?dataSetName?"}
};
static int nImageOps = NumCmdSpecs(imageOps);

static int
ImageCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nImageOps, imageOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
ImageCutplaneAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addGraphicsObject<ImageCutplane>(name)) {
            Tcl_AppendResult(interp, "Failed to create imgcutplane", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<ImageCutplane>("all")) {
            Tcl_AppendResult(interp, "Failed to create imgcutplane for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
ImageCutplaneColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<ImageCutplane>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<ImageCutplane>("all", color);
    }
    return TCL_OK;
}

static int
ImageCutplaneColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectColorMap<ImageCutplane>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<ImageCutplane>("all", colorMapName);
    }
    return TCL_OK;
}

static int
ImageCutplaneDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<ImageCutplane>(name);
    } else {
        g_renderer->deleteGraphicsObject<ImageCutplane>("all");
    }
    return TCL_OK;
}

static int
ImageCutplaneLevelOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    double level;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &level) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageCutplaneLevel(name, level);
    } else {
        g_renderer->setImageCutplaneLevel("all", level);
    }
    return TCL_OK;
}

static int
ImageCutplaneMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    double ambient, diffuse;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &ambient) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &diffuse) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setGraphicsObjectAmbient<ImageCutplane>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<ImageCutplane>(name, diffuse);
    } else {
        g_renderer->setGraphicsObjectAmbient<ImageCutplane>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<ImageCutplane>("all", diffuse);
    }
    return TCL_OK;
}

static int
ImageCutplaneOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<ImageCutplane>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<ImageCutplane>("all", opacity);
    }
    return TCL_OK;
}

static int
ImageCutplaneOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<ImageCutplane>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<ImageCutplane>("all", quat);
    }
    return TCL_OK;
}

static int
ImageCutplaneOutlineOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageCutplaneOutlineVisibility(name, state);
    } else {
        g_renderer->setImageCutplaneOutlineVisibility("all", state);
    }
    return TCL_OK;
}

static int
ImageCutplanePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<ImageCutplane>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<ImageCutplane>("all", pos);
    }
    return TCL_OK;
}

static int
ImageCutplaneScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<ImageCutplane>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<ImageCutplane>("all", scale);
    }
    return TCL_OK;
}

static int
ImageCutplaneSliceVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setImageCutplaneSliceVisibility(name, axis, state);
    } else {
        g_renderer->setImageCutplaneSliceVisibility("all", axis, state);
    }
    return TCL_OK;
}

static int
ImageCutplaneVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<ImageCutplane>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<ImageCutplane>("all", state);
    }
    return TCL_OK;
}

static int
ImageCutplaneVolumeSliceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectVolumeSlice<ImageCutplane>(name, axis, ratio);
    } else {
        g_renderer->setGraphicsObjectVolumeSlice<ImageCutplane>("all", axis, ratio);
    }
    return TCL_OK;
}

static int
ImageCutplaneWindowOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double window;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &window) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setImageCutplaneWindow(name, window);
    } else {
        g_renderer->setImageCutplaneWindow("all", window);
    }
    return TCL_OK;
}

static CmdSpec imageCutplaneOps[] = {
    {"add",          2, ImageCutplaneAddOp, 2, 3, "?dataSetName?"},
    {"axis",         2, ImageCutplaneSliceVisibilityOp, 4, 5, "axis bool ?dataSetName?"},
    {"color",        5, ImageCutplaneColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     6, ImageCutplaneColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",       1, ImageCutplaneDeleteOp, 2, 3, "?dataSetName?"},
    {"level",        1, ImageCutplaneLevelOp, 3, 4, "level ?dataSetName?"},
    {"material",     1, ImageCutplaneMaterialOp, 4, 5, "ambientCoeff diffuseCoeff ?dataSetName?"},
    {"opacity",      2, ImageCutplaneOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, ImageCutplaneOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"outline",      2, ImageCutplaneOutlineOp, 3, 4, "bool ?dataSetName?"},
    {"pos",          2, ImageCutplanePositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",        2, ImageCutplaneScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"slice",        2, ImageCutplaneVolumeSliceOp, 4, 5, "axis ratio ?dataSetName?"},
    {"visible",      1, ImageCutplaneVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"window",       1, ImageCutplaneWindowOp, 3, 4, "window ?dataSetName?"}
};
static int nImageCutplaneOps = NumCmdSpecs(imageCutplaneOps);

static int
ImageCutplaneCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nImageCutplaneOps, imageCutplaneOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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

    bool opaque = true;

    if (objc == 9) {
        const char *dataSetName = Tcl_GetString(objv[8]);
        if (!g_renderer->renderColorMap(colorMapName, dataSetName, legendType, fieldName, title,
                                        range, width, height, opaque, numLabels, imgData)) {
            Tcl_AppendResult(interp, "Color map \"",
                             colorMapName, "\" or dataset \"",
                             dataSetName, "\" was not found", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->renderColorMap(colorMapName, "all", legendType, fieldName, title,
                                        range, width, height, opaque, numLabels, imgData)) {
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
LegendSimpleCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    if (objc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " colormapName width height\"", (char*)NULL);
        return TCL_ERROR;
    }
    const char *colorMapName = Tcl_GetString(objv[1]);

    int width, height;
    if (Tcl_GetIntFromObj(interp, objv[2], &width) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &height) != TCL_OK) {
        return TCL_ERROR;
    }

    vtkSmartPointer<vtkUnsignedCharArray> imgData = 
        vtkSmartPointer<vtkUnsignedCharArray>::New();

    bool opaque = true;

    if (!g_renderer->renderColorMap(colorMapName, width, height, opaque, imgData)) {
        Tcl_AppendResult(interp, "Color map \"",
                         colorMapName, "\" was not found", (char*)NULL);
        return TCL_ERROR;
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
    snprintf(cmd, sizeof(cmd), "nv>legend {%s} {} 0 1", colorMapName);

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
LICPreInterpOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<LIC>(name, state);
    } else {
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<LIC>("all", state);
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

static CmdSpec licOps[] = {
    {"add",          1, LICAddOp, 2, 3, "?dataSetName?"},
    {"colormap",     1, LICColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"delete",       1, LICDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, LICEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",     3, LICLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, LICLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, LICLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",      2, LICOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, LICOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          2, LICPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"preinterp",    2, LICPreInterpOp, 3, 4, "bool ?dataSetName?"},
    {"scale",        2, LICScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"slice",        2, LICVolumeSliceOp, 4, 5, "axis ratio ?dataSetName?"},
    {"visible",      1, LICVisibleOp, 3, 4, "bool ?dataSetName?"}
};
static int nLICOps = NumCmdSpecs(licOps);

static int
LICCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nLICOps, licOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
LineAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    std::vector<double> points;
    int ptlistc;
    Tcl_Obj **ptlistv;
    if (Tcl_ListObjGetElements(interp, objv[2], &ptlistc, &ptlistv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ptlistc < 6 || ptlistc % 3 != 0) {
        Tcl_AppendResult(interp, "Points list size must be 6 or more and a multiple of 3", (char*)NULL);
        return TCL_ERROR;
    }
    for (int i = 0; i < ptlistc; i++) {
        double val;
        if (Tcl_GetDoubleFromObj(interp, ptlistv[i], &val) != TCL_OK) {
            return TCL_ERROR;
        }
        points.push_back(val);
    }
    const char *name = Tcl_GetString(objv[3]);
    if (!g_renderer->addLine(name, points)) {
        Tcl_AppendResult(interp, "Failed to create line", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
LineDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Line>(name);
    } else {
        g_renderer->deleteGraphicsObject<Line>("all");
    }
    return TCL_OK;
}

static int
LineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Line>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Line>("all", color);
    }
    return TCL_OK;
}

static int
LineLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Line>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Line>("all", width);
    }
    return TCL_OK;
}

static int
LineOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Line>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Line>("all", opacity);
    }
    return TCL_OK;
}

static int
LineOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Line>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Line>("all", quat);
    }
    return TCL_OK;
}

static int
LineOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Line>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Line>("all", origin);
    }
    return TCL_OK;
}

static int
LinePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Line>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Line>("all", pos);
    }
    return TCL_OK;
}

static int
LineScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Line>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Line>("all", scale);
    }
    return TCL_OK;
}

static int
LineVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Line>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Line>("all", state);
    }
    return TCL_OK;
}

static CmdSpec lineOps[] = {
    {"add",       1, LineAddOp, 4, 4, "points name"},
    {"color",     1, LineColorOp, 5, 6, "r g b ?name?"},
    {"delete",    1, LineDeleteOp, 2, 3, "?name?"},
    {"linecolor", 5, LineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, LineLineWidthOp, 3, 4, "width ?name?"},
    {"opacity",   2, LineOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, LineOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, LineOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, LinePositionOp, 5, 6, "x y z ?name?"},
    {"scale",     1, LineScaleOp, 5, 6, "sx sy sz ?name?"},
    {"visible",   1, LineVisibleOp, 3, 4, "bool ?name?"}
};
static int nLineOps = NumCmdSpecs(lineOps);

static int
LineCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nLineOps, lineOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
MoleculeAtomLabelFieldOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    const char *fieldName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeAtomLabelField(name, fieldName);
    } else {
        g_renderer->setMoleculeAtomLabelField("all", fieldName);
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
MoleculeAtomQualityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double quality;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quality) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeAtomQuality(name, quality);
    } else {
        g_renderer->setMoleculeAtomQuality("all", quality);
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
MoleculeBondQualityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double quality;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &quality) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setMoleculeBondQuality(name, quality);
    } else {
        g_renderer->setMoleculeBondQuality("all", quality);
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
    } else if (str[0] == 'c' &&
               (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
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
                         "\": should be one of: 'by_elements', 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
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
MoleculeMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectAmbient<Molecule>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Molecule>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Molecule>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Molecule>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Molecule>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Molecule>("all", specCoeff, specPower);
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

static CmdSpec moleculeOps[] = {
    {"add",          2, MoleculeAddOp, 2, 3, "?dataSetName?"},
    {"aquality",     2, MoleculeAtomQualityOp, 3, 4, "value ?dataSetName?"},
    {"ascale",       2, MoleculeAtomScaleFactorOp, 3, 4, "value ?dataSetName?"},
    {"atoms",        2, MoleculeAtomVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"bcmode",       3, MoleculeBondColorModeOp, 3, 4, "mode ?dataSetName?"},
    {"bcolor",       3, MoleculeBondColorOp, 5, 6, "r g b ?dataSetName?"},
    {"bonds",        2, MoleculeBondVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"bquality",     2, MoleculeBondQualityOp, 3, 4, "value ?dataSetName?"},
    {"bscale",       3, MoleculeBondScaleFactorOp, 3, 4, "value ?dataSetName?"},
    {"bstyle",       3, MoleculeBondStyleOp, 3, 4, "value ?dataSetName?"},
    {"ccolor",       2, MoleculeColorOp, 5, 6, "r g b ?dataSetName?"},
    {"color",        5, MoleculeColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, MoleculeColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    7, MoleculeColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"delete",       1, MoleculeDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, MoleculeEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"labels",       2, MoleculeAtomLabelVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lfield",       2, MoleculeAtomLabelFieldOp, 3, 4, "fieldName ?dataSetName?"},
    {"lighting",     3, MoleculeLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, MoleculeLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, MoleculeLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"material",     1, MoleculeMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",      2, MoleculeOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, MoleculeOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, MoleculePositionOp, 5, 6, "x y z ?dataSetName?"},
    {"rscale",       1, MoleculeAtomScalingOp, 3, 4, "scaling ?dataSetName?"},
    {"scale",        1, MoleculeScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",      1, MoleculeVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe",    1, MoleculeWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nMoleculeOps = NumCmdSpecs(moleculeOps);

static int
MoleculeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nMoleculeOps, moleculeOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
OutlineAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        if (!g_renderer->addGraphicsObject<Outline>(name)) {
            Tcl_AppendResult(interp, "Failed to create Outline", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        if (!g_renderer->addGraphicsObject<Outline>("all")) {
            Tcl_AppendResult(interp, "Failed to create Outline for one or more data sets", (char*)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
OutlineDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Outline>(name);
    } else {
        g_renderer->deleteGraphicsObject<Outline>("all");
    }
    return TCL_OK;
}

static int
OutlineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Outline>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Outline>("all", color);
    }
    return TCL_OK;
}

static int
OutlineLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Outline>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Outline>("all", width);
    }
    return TCL_OK;
}

static int
OutlineOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Outline>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Outline>("all", opacity);
    }
    return TCL_OK;
}

static int
OutlineOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Outline>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Outline>("all", quat);
    }
    return TCL_OK;
}

static int
OutlinePositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Outline>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Outline>("all", pos);
    }
    return TCL_OK;
}

static int
OutlineScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Outline>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Outline>("all", scale);
    }
    return TCL_OK;
}

static int
OutlineVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Outline>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Outline>("all", state);
    }
    return TCL_OK;
}

static CmdSpec outlineOps[] = {
    {"add",       1, OutlineAddOp, 2, 3, "?dataSetName?"},
    {"color",     1, OutlineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"delete",    1, OutlineDeleteOp, 2, 3, "?dataSetName?"},
    {"linecolor", 5, OutlineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, OutlineLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",   2, OutlineOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",    2, OutlineOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",       1, OutlinePositionOp, 5, 6, "x y z ?dataSetName?"},
    {"scale",     1, OutlineScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",   1, OutlineVisibleOp, 3, 4, "bool ?dataSetName?"}
};
static int nOutlineOps = NumCmdSpecs(outlineOps);

static int
OutlineCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nOutlineOps, outlineOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
ParallelepipedAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double vec1[3], vec2[3], vec3[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &vec1[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &vec1[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &vec1[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &vec2[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[6], &vec2[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[7], &vec2[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[8], &vec3[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[9], &vec3[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[10], &vec3[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[11]);
    if (!g_renderer->addParallelepiped(name, vec1, vec2, vec3)) {
        Tcl_AppendResult(interp, "Failed to create parallelepiped", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ParallelepipedDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Parallelepiped>(name);
    } else {
        g_renderer->deleteGraphicsObject<Parallelepiped>("all");
    }
    return TCL_OK;
}

static int
ParallelepipedColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Parallelepiped>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Parallelepiped>("all", color);
    }
    return TCL_OK;
}

static int
ParallelepipedCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<Parallelepiped>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<Parallelepiped>("all", state);
    }
    return TCL_OK;
}

static int
ParallelepipedEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Parallelepiped>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Parallelepiped>("all", state);
    }
    return TCL_OK;
}

static int
ParallelepipedFlipNormalsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                            Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectFlipNormals<Parallelepiped>(name, state);
    } else {
        g_renderer->setGraphicsObjectFlipNormals<Parallelepiped>("all", state);
    }
    return TCL_OK;
}

static int
ParallelepipedLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Parallelepiped>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Parallelepiped>("all", state);
    }
    return TCL_OK;
}

static int
ParallelepipedLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Parallelepiped>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Parallelepiped>("all", color);
    }
    return TCL_OK;
}

static int
ParallelepipedLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Parallelepiped>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Parallelepiped>("all", width);
    }
    return TCL_OK;
}

static int
ParallelepipedMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectAmbient<Parallelepiped>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Parallelepiped>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Parallelepiped>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Parallelepiped>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Parallelepiped>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Parallelepiped>("all", specCoeff, specPower);
    }
    return TCL_OK;
}

static int
ParallelepipedOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Parallelepiped>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Parallelepiped>("all", opacity);
    }
    return TCL_OK;
}

static int
ParallelepipedOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Parallelepiped>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Parallelepiped>("all", quat);
    }
    return TCL_OK;
}

static int
ParallelepipedOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Parallelepiped>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Parallelepiped>("all", origin);
    }
    return TCL_OK;
}

static int
ParallelepipedPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Parallelepiped>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Parallelepiped>("all", pos);
    }
    return TCL_OK;
}

static int
ParallelepipedScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Parallelepiped>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Parallelepiped>("all", scale);
    }
    return TCL_OK;
}

static int
ParallelepipedShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<Parallelepiped>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<Parallelepiped>("all", shadeModel);
    }
    return TCL_OK;
}

static int
ParallelepipedVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Parallelepiped>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Parallelepiped>("all", state);
    }
    return TCL_OK;
}

static int
ParallelepipedWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Parallelepiped>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Parallelepiped>("all", state);
    }
    return TCL_OK;
}

static CmdSpec parallelepipedOps[] = {
    {"add",       1, ParallelepipedAddOp, 12, 12, "v0x v0y v0z v1x v1y v1z v2x v2y v2z name"},
    {"color",     2, ParallelepipedColorOp, 5, 6, "r g b ?name?"},
    {"culling",   2, ParallelepipedCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, ParallelepipedDeleteOp, 2, 3, "?name?"},
    {"edges",     1, ParallelepipedEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"flipnorms", 1, ParallelepipedFlipNormalsOp, 3, 4, "bool ?name?"},
    {"lighting",  3, ParallelepipedLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, ParallelepipedLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, ParallelepipedLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, ParallelepipedMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, ParallelepipedOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, ParallelepipedOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, ParallelepipedOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, ParallelepipedPositionOp, 5, 6, "x y z ?name?"},
    {"scale",     2, ParallelepipedScaleOp, 5, 6, "sx sy sz ?name?"},
    {"shading",   2, ParallelepipedShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, ParallelepipedVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, ParallelepipedWireframeOp, 3, 4, "bool ?name?"}
};
static int nParallelepipedOps = NumCmdSpecs(parallelepipedOps);

static int
ParallelepipedCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nParallelepipedOps, parallelepipedOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
PolyDataCloudStyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    PolyData::CloudStyle style;
    char *str =  Tcl_GetString(objv[2]);
    if (str[0] == 'm' && strcmp(str, "mesh") == 0) {
        style = PolyData::CLOUD_MESH;
    } else if (str[0] == 'p' && strcmp(str, "points") == 0) {
        style = PolyData::CLOUD_POINTS;
    } else {
        Tcl_AppendResult(interp, "bad cloudstyle option \"", str,
                         "\": should be one of: 'mesh', 'points'", (char*)NULL);
        return TCL_ERROR;
    }

    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPolyDataCloudStyle(name, style);
    } else {
        g_renderer->setPolyDataCloudStyle("all", style);
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
PolyDataColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectColorMap<PolyData>(dataSetName, colorMapName);
    } else {
        g_renderer->setGraphicsObjectColorMap<PolyData>("all", colorMapName);
    }
    return TCL_OK;
}

static int
PolyDataColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    PolyData::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'c' &&
        (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
        mode = PolyData::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = PolyData::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = PolyData::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = PolyData::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = PolyData::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = PolyData::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == PolyData::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setPolyDataColorMode(name, mode, fieldName);
    } else {
        g_renderer->setPolyDataColorMode("all", mode, fieldName);
    }
    return TCL_OK;
}

static int
PolyDataCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<PolyData>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<PolyData>("all", state);
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
PolyDataOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<PolyData>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<PolyData>("all", origin);
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
PolyDataPreInterpOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<PolyData>(name, state);
    } else {
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<PolyData>("all", state);
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
PolyDataShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<PolyData>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<PolyData>("all", shadeModel);
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

static CmdSpec polyDataOps[] = {
    {"add",       1, PolyDataAddOp, 2, 3, "?dataSetName?"},
    {"ccolor",    2, PolyDataColorOp, 5, 6, "r g b ?dataSetName?"},
    {"cloudstyle",2, PolyDataCloudStyleOp, 3, 4, "style ?dataSetName?"},
    {"color",     5, PolyDataColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",  7, PolyDataColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode", 7, PolyDataColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"culling",   2, PolyDataCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, PolyDataDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",     1, PolyDataEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",  3, PolyDataLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor", 5, PolyDataLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth", 5, PolyDataLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"material",  1, PolyDataMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?dataSetName?"},
    {"opacity",   2, PolyDataOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",    4, PolyDataOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"origin",    4, PolyDataOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       2, PolyDataPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"preinterp", 2, PolyDataPreInterpOp, 3, 4, "bool ?dataSetName?"},
    {"ptsize",    2, PolyDataPointSizeOp, 3, 4, "size ?dataSetName?"},
    {"scale",     2, PolyDataScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"shading",   2, PolyDataShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, PolyDataVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe", 1, PolyDataWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nPolyDataOps = NumCmdSpecs(polyDataOps);

static int
PolyDataCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nPolyDataOps, polyDataOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
PolygonAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    int numSides;
    double center[3], normal[3], radius;
    if (Tcl_GetIntFromObj(interp, objv[2], &numSides) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &center[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &center[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &center[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[6], &normal[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[7], &normal[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[8], &normal[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[9], &radius) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[10]);
    if (!g_renderer->addPolygon(name, numSides, center, normal, radius)) {
        Tcl_AppendResult(interp, "Failed to create polygon", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
PolygonDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Polygon>(name);
    } else {
        g_renderer->deleteGraphicsObject<Polygon>("all");
    }
    return TCL_OK;
}

static int
PolygonColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Polygon>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Polygon>("all", color);
    }
    return TCL_OK;
}

static int
PolygonCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<Polygon>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<Polygon>("all", state);
    }
    return TCL_OK;
}

static int
PolygonEdgeVisibilityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeVisibility<Polygon>(name, state);
    } else {
        g_renderer->setGraphicsObjectEdgeVisibility<Polygon>("all", state);
    }
    return TCL_OK;
}

static int
PolygonFlipNormalsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectFlipNormals<Polygon>(name, state);
    } else {
        g_renderer->setGraphicsObjectFlipNormals<Polygon>("all", state);
    }
    return TCL_OK;
}

static int
PolygonLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectLighting<Polygon>(name, state);
    } else {
        g_renderer->setGraphicsObjectLighting<Polygon>("all", state);
    }
    return TCL_OK;
}

static int
PolygonLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectEdgeColor<Polygon>(name, color);
    } else {
        g_renderer->setGraphicsObjectEdgeColor<Polygon>("all", color);
    }
    return TCL_OK;
}

static int
PolygonLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectEdgeWidth<Polygon>(name, width);
    } else {
        g_renderer->setGraphicsObjectEdgeWidth<Polygon>("all", width);
    }
    return TCL_OK;
}

static int
PolygonMaterialOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectAmbient<Polygon>(name, ambient);
        g_renderer->setGraphicsObjectDiffuse<Polygon>(name, diffuse);
        g_renderer->setGraphicsObjectSpecular<Polygon>(name, specCoeff, specPower);
    } else {
        g_renderer->setGraphicsObjectAmbient<Polygon>("all", ambient);
        g_renderer->setGraphicsObjectDiffuse<Polygon>("all", diffuse);
        g_renderer->setGraphicsObjectSpecular<Polygon>("all", specCoeff, specPower);
    }
    return TCL_OK;
}

static int
PolygonOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Polygon>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Polygon>("all", opacity);
    }
    return TCL_OK;
}

static int
PolygonOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Polygon>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Polygon>("all", quat);
    }
    return TCL_OK;
}

static int
PolygonOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Polygon>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Polygon>("all", origin);
    }
    return TCL_OK;
}

static int
PolygonPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Polygon>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Polygon>("all", pos);
    }
    return TCL_OK;
}

static int
PolygonScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Polygon>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Polygon>("all", scale);
    }
    return TCL_OK;
}

static int
PolygonShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<Polygon>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<Polygon>("all", shadeModel);
    }
    return TCL_OK;
}

static int
PolygonVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Polygon>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Polygon>("all", state);
    }
    return TCL_OK;
}

static int
PolygonWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectWireframe<Polygon>(name, state);
    } else {
        g_renderer->setGraphicsObjectWireframe<Polygon>("all", state);
    }
    return TCL_OK;
}

static CmdSpec polygonOps[] = {
    {"add",       1, PolygonAddOp, 11, 11, "numSides centerX centerY centerZ normalX normalY normalZ radius name"},
    {"color",     1, PolygonColorOp, 5, 6, "r g b ?name?"},
    {"culling",   2, PolygonCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, PolygonDeleteOp, 2, 3, "?name?"},
    {"edges",     1, PolygonEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"flipnorms", 1, PolygonFlipNormalsOp, 3, 4, "bool ?name?"},
    {"lighting",  3, PolygonLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, PolygonLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, PolygonLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, PolygonMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, PolygonOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, PolygonOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, PolygonOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       2, PolygonPositionOp, 5, 6, "x y z ?name?"},
    {"scale",     2, PolygonScaleOp, 5, 6, "sx sy sz ?name?"},
    {"shading",   2, PolygonShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, PolygonVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, PolygonWireframeOp, 3, 4, "bool ?name?"}
};
static int nPolygonOps = NumCmdSpecs(polygonOps);

static int
PolygonCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nPolygonOps, polygonOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
PseudoColorCloudStyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    PseudoColor::CloudStyle style;
    char *str =  Tcl_GetString(objv[2]);
    if (str[0] == 'm' && strcmp(str, "mesh") == 0) {
        style = PseudoColor::CLOUD_MESH;
    } else if (str[0] == 'p' && strcmp(str, "points") == 0) {
        style = PseudoColor::CLOUD_POINTS;
    } else if (str[0] == 's' && strcmp(str, "splat") == 0) {
        style = PseudoColor::CLOUD_SPLAT;
    } else {
        Tcl_AppendResult(interp, "bad cloudstyle option \"", str,
                         "\": should be one of: 'mesh', 'points', 'splat'", (char*)NULL);
        return TCL_ERROR;
    }

    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setPseudoColorCloudStyle(name, style);
    } else {
        g_renderer->setPseudoColorCloudStyle("all", style);
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
    if (str[0] == 'c' &&
        (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
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
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
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
PseudoColorPointSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    float size;
    if (GetFloatFromObj(interp, objv[2], &size) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectPointSize<PseudoColor>(name, size);
    } else {
        g_renderer->setGraphicsObjectPointSize<PseudoColor>("all", size);
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
PseudoColorPreInterpOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                       Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<PseudoColor>(name, state);
    } else {
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<PseudoColor>("all", state);
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

static CmdSpec pseudoColorOps[] = {
    {"add",          1, PseudoColorAddOp, 2, 3, "?dataSetName?"},
    {"ccolor",       2, PseudoColorColorOp, 5, 6, "r g b ?dataSetName?"},
    {"cloudstyle",   2, PseudoColorCloudStyleOp, 3, 4, "style ?dataSetName?"},
    {"color",        5, PseudoColorColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, PseudoColorColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    7, PseudoColorColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"delete",       1, PseudoColorDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, PseudoColorEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",     3, PseudoColorLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, PseudoColorLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, PseudoColorLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",      2, PseudoColorOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, PseudoColorOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          2, PseudoColorPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"preinterp",    2, PseudoColorPreInterpOp, 3, 4, "bool ?dataSetName?"},
    {"ptsize",       2, PseudoColorPointSizeOp, 3, 4, "size ?dataSetName?"},
    {"scale",        1, PseudoColorScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
    {"visible",      1, PseudoColorVisibleOp, 3, 4, "bool ?dataSetName?"},
    {"wireframe",    1, PseudoColorWireframeOp, 3, 4, "bool ?dataSetName?"}
};
static int nPseudoColorOps = NumCmdSpecs(pseudoColorOps);

static int
PseudoColorCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nPseudoColorOps, pseudoColorOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
    if (objc > 3) {
        if (objc < 5) {
             Tcl_AppendResult(interp, "wrong # args: should be \"", Tcl_GetString(objv[0]),
                              " ", Tcl_GetString(objv[1]), " bool ?occlusionRatio maxPeels?\"", (char*)NULL);
            return TCL_ERROR;
        }
        double occlusionRatio;
        int maxPeels;
        if (Tcl_GetDoubleFromObj(interp, objv[3], &occlusionRatio) != TCL_OK) {
            return TCL_ERROR;
        }
        if (occlusionRatio < 0.0 || occlusionRatio > 0.5) {
            Tcl_AppendResult(interp, "bad occlusionRatio value \"", Tcl_GetString(objv[3]),
                             "\": should be [0,0.5]", (char*)NULL);
            return TCL_ERROR;
        }
        if (Tcl_GetIntFromObj(interp, objv[4], &maxPeels) != TCL_OK) {
            return TCL_ERROR;
        }
        if (maxPeels < 0) {
            Tcl_AppendResult(interp, "bad maxPeels value \"", Tcl_GetString(objv[3]),
                             "\": must be zero or greater", (char*)NULL);
            return TCL_ERROR;
        }
        g_renderer->setDepthPeelingParams(occlusionRatio, maxPeels);
    }
    return TCL_OK;
}

static int
RendererLightsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    int idx;
    bool state;
    if (Tcl_GetIntFromObj(interp, objv[2], &idx) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setLightSwitch(idx, state);
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

static CmdSpec rendererOps[] = {
    {"clipplane",  1, RendererClipPlaneOp, 5, 5, "axis ratio direction"},
    {"depthpeel",  1, RendererDepthPeelingOp, 3, 5, "bool ?occlusionRatio maxPeels?"},
    {"light2side", 6, RendererTwoSidedLightingOp, 3, 3, "bool"},
    {"lights",     6, RendererLightsOp, 4, 4, "idx bool"}, 
    {"render",     1, RendererRenderOp, 2, 2, ""}
};
static int nRendererOps = NumCmdSpecs(rendererOps);

static int
RendererCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nRendererOps, rendererOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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

static CmdSpec screenOps[] = {
    {"bgcolor", 1, ScreenBgColorOp, 5, 5, "r g b"},
    {"size", 1, ScreenSizeOp, 4, 4, "width height"}
};
static int nScreenOps = NumCmdSpecs(screenOps);

static int
ScreenCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nScreenOps, screenOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
SphereAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    double center[3], radius;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &center[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &center[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &center[2]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &radius) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *name = Tcl_GetString(objv[6]);
    if (!g_renderer->addSphere(name, center, radius)) {
        Tcl_AppendResult(interp, "Failed to create sphere", (char*)NULL);
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
SphereCullingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectCulling<Sphere>(name, state);
    } else {
        g_renderer->setGraphicsObjectCulling<Sphere>("all", state);
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
SphereFlipNormalsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectFlipNormals<Sphere>(name, state);
    } else {
        g_renderer->setGraphicsObjectFlipNormals<Sphere>("all", state);
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
SphereOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Sphere>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Sphere>("all", origin);
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
SphereShadingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    GraphicsObject::ShadingModel shadeModel;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'f' && strcmp(str, "flat") == 0) {
        shadeModel = GraphicsObject::SHADE_FLAT;
    } else if (str[0] == 's' && strcmp(str, "smooth") == 0) {
        shadeModel = GraphicsObject::SHADE_GOURAUD;
    } else {
         Tcl_AppendResult(interp, "bad shading option \"", str,
                         "\": should be one of: 'flat', 'smooth'", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectShadingModel<Sphere>(name, shadeModel);
    } else {
        g_renderer->setGraphicsObjectShadingModel<Sphere>("all", shadeModel);
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

static CmdSpec sphereOps[] = {
    {"add",       1, SphereAddOp, 7, 7, "centerX centerY centerZ radius name"},
    {"color",     2, SphereColorOp, 5, 6, "r g b ?name?"},
    {"culling",   2, SphereCullingOp, 3, 4, "bool ?name?"},
    {"delete",    1, SphereDeleteOp, 2, 3, "?name?"},
    {"edges",     1, SphereEdgeVisibilityOp, 3, 4, "bool ?name?"},
    {"flipnorms", 1, SphereFlipNormalsOp, 3, 4, "bool ?name?"},
    {"lighting",  3, SphereLightingOp, 3, 4, "bool ?name?"},
    {"linecolor", 5, SphereLineColorOp, 5, 6, "r g b ?name?"},
    {"linewidth", 5, SphereLineWidthOp, 3, 4, "width ?name?"},
    {"material",  1, SphereMaterialOp, 6, 7, "ambientCoeff diffuseCoeff specularCoeff specularPower ?name?"},
    {"opacity",   2, SphereOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, SphereOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, SphereOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       2, SpherePositionOp, 5, 6, "x y z ?name?"},
    {"resolution",1, SphereResolutionOp, 4, 5, "thetaRes phiRes ?name?"},
    {"scale",     2, SphereScaleOp, 5, 6, "sx sy sz ?name?"},
    {"section",   2, SphereSectionOp, 6, 7, "thetaStart thetaEnd phiStart phiEnd ?name?"},
    {"shading",   2, SphereShadingOp, 3, 4, "val ?name?"},
    {"visible",   1, SphereVisibleOp, 3, 4, "bool ?name?"},
    {"wireframe", 1, SphereWireframeOp, 3, 4, "bool ?name?"}
};
static int nSphereOps = NumCmdSpecs(sphereOps);

static int
SphereCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nSphereOps, sphereOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
    } else if (str[0] == 'c' &&
               (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
        mode = Streamlines::COLOR_CONSTANT;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
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
StreamlinesSeedPointSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                           Tcl_Obj *const *objv)
{
    float size;
    if (GetFloatFromObj(interp, objv[3], &size) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setStreamlinesSeedPointSize(name, size);
    } else {
        g_renderer->setStreamlinesSeedPointSize("all", size);
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

static CmdSpec streamlinesSeedOps[] = {
    {"color",   1, StreamlinesSeedColorOp,         6, 7, "r g b ?dataSetName?"},
    {"disk",    1, StreamlinesSeedDiskOp,          12, 13, "centerX centerY centerZ normalX normalY normalZ radius innerRadius numPoints ?dataSetName?"},
    {"fmesh",   2, StreamlinesSeedFilledMeshOp,    7, 8, "numPoints data follows nbytes ?dataSetName?"},
    {"fpoly",   2, StreamlinesSeedFilledPolygonOp, 13, 14, "centerX centerY centerZ normalX normalY normalZ angle radius numSides numPoints ?dataSetName?"},
    {"mesh",    1, StreamlinesSeedMeshPointsOp,    6, 7, "data follows nbytes ?dataSetName?"},
    {"numpts",  1, StreamlinesSeedNumPointsOp,     4, 5, "numPoints ?dataSetName?"},
    {"points",  3, StreamlinesSeedPointsOp,        3, 4, "?dataSetName?"},
    {"polygon", 3, StreamlinesSeedPolygonOp,       12, 13, "centerX centerY centerZ normalX normalY normalZ angle radius numSides ?dataSetName?"},
    {"ptsize",  2, StreamlinesSeedPointSizeOp,     3, 4, "size ?dataSetName?"},
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

    proc = GetOpFromObj(interp, nStreamlinesSeedOps, streamlinesSeedOps,
                        CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
StreamlinesTerminalSpeedOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                           Tcl_Obj *const *objv)
{
    double speed;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &speed) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setStreamlinesTerminalSpeed(name, speed);
    } else {
        g_renderer->setStreamlinesTerminalSpeed("all", speed);
    }
    return TCL_OK;
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

static CmdSpec streamlinesOps[] = {
    {"add",          1, StreamlinesAddOp,            2, 3, "?dataSetName?"},
    {"ccolor",       2, StreamlinesColorOp,          5, 6, "r g b ?dataSetName?"},
    {"color",        5, StreamlinesColorOp,          5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, StreamlinesColorMapOp,       3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    7, StreamlinesColorModeOp,      4, 5, "mode fieldName ?dataSetName?"},
    {"delete",       1, StreamlinesDeleteOp,         2, 3, "?dataSetName?"},
    {"edges",        1, StreamlinesEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"length",       2, StreamlinesLengthOp,         3, 4, "length ?dataSetName?"},
    {"lighting",     3, StreamlinesLightingOp,       3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, StreamlinesLineColorOp,      5, 6, "r g b ?dataSetName?"},
    {"lines",        5, StreamlinesLinesOp,          2, 3, "?dataSetName?"},
    {"linewidth",    5, StreamlinesLineWidthOp,      3, 4, "width ?dataSetName?"},
    {"opacity",      2, StreamlinesOpacityOp,        3, 4, "val ?dataSetName?"},
    {"orient",       2, StreamlinesOrientOp,         6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, StreamlinesPositionOp,       5, 6, "x y z ?dataSetName?"},
    {"ribbons",      1, StreamlinesRibbonsOp,        4, 5, "width angle ?dataSetName?"},
    {"scale",        2, StreamlinesScaleOp,          5, 6, "sx sy sz ?dataSetName?"},
    {"seed",         2, StreamlinesSeedOp,           3, 14, "op params... ?dataSetName?"},
    {"termspeed",    2, StreamlinesTerminalSpeedOp,  3, 4, "speed ?dataSetName?"},
    {"tubes",        2, StreamlinesTubesOp,          4, 5, "numSides radius ?dataSetName?"},
    {"visible",      1, StreamlinesVisibleOp,        3, 4, "bool ?dataSetName?"}
};
static int nStreamlinesOps = NumCmdSpecs(streamlinesOps);

static int
StreamlinesCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nStreamlinesOps, streamlinesOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
Text3DAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    int fontSize;
    const char *fontFamily = Tcl_GetString(objv[2]);
    if (Tcl_GetIntFromObj(interp, objv[3], &fontSize) != TCL_OK) {
        return TCL_ERROR;
    }
    const char *text = Tcl_GetString(objv[4]);
    const char *name = Tcl_GetString(objv[5]);
    if (!g_renderer->addText3D(name, text, fontFamily, fontSize)) {
        Tcl_AppendResult(interp, "Failed to create text3d", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
Text3DBoldOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setText3DBold(name, state);
    } else {
        g_renderer->setText3DBold("all", state);
    }
    return TCL_OK;
}

static int
Text3DColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Text3D>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Text3D>("all", color);
    }
    return TCL_OK;
}

static int
Text3DDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    if (objc == 3) {
        const char *name = Tcl_GetString(objv[2]);
        g_renderer->deleteGraphicsObject<Text3D>(name);
    } else {
        g_renderer->deleteGraphicsObject<Text3D>("all");
    }
    return TCL_OK;
}

static int
Text3DFollowCameraOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setText3DFollowCamera(name, state);
    } else {
        g_renderer->setText3DFollowCamera("all", state);
    }
    return TCL_OK;
}

static int
Text3DFontFamilyOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    const char *fontFamily = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setText3DFont(name, fontFamily);
    } else {
        g_renderer->setText3DFont("all", fontFamily);
    }
    return TCL_OK;
}

static int
Text3DFontSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    int fontSize;
    if (Tcl_GetIntFromObj(interp, objv[2], &fontSize) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setText3DFontSize(name, fontSize);
    } else {
        g_renderer->setText3DFontSize("all", fontSize);
    }
    return TCL_OK;
}

static int
Text3DItalicOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setText3DItalic(name, state);
    } else {
        g_renderer->setText3DItalic("all", state);
    }
    return TCL_OK;
}

static int
Text3DOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectOpacity<Text3D>(name, opacity);
    } else {
        g_renderer->setGraphicsObjectOpacity<Text3D>("all", opacity);
    }
    return TCL_OK;
}

static int
Text3DOrientOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectOrientation<Text3D>(name, quat);
    } else {
        g_renderer->setGraphicsObjectOrientation<Text3D>("all", quat);
    }
    return TCL_OK;
}

static int
Text3DOriginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double origin[3];
    if (Tcl_GetDoubleFromObj(interp, objv[2], &origin[0]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &origin[1]) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &origin[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 6) {
        const char *name = Tcl_GetString(objv[5]);
        g_renderer->setGraphicsObjectOrigin<Text3D>(name, origin);
    } else {
        g_renderer->setGraphicsObjectOrigin<Text3D>("all", origin);
    }
    return TCL_OK;
}

static int
Text3DPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectPosition<Text3D>(name, pos);
    } else {
        g_renderer->setGraphicsObjectPosition<Text3D>("all", pos);
    }
    return TCL_OK;
}

static int
Text3DScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectScale<Text3D>(name, scale);
    } else {
        g_renderer->setGraphicsObjectScale<Text3D>("all", scale);
    }
    return TCL_OK;
}

static int
Text3DShadowOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setText3DShadow(name, state);
    } else {
        g_renderer->setText3DShadow("all", state);
    }
    return TCL_OK;
}

static int
Text3DTextOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    char *str = Tcl_GetString(objv[2]);
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setText3DText(name, str);
    } else {
        g_renderer->setText3DText("all", str);
    }
    return TCL_OK;
}

static int
Text3DVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectVisibility<Text3D>(name, state);
    } else {
        g_renderer->setGraphicsObjectVisibility<Text3D>("all", state);
    }
    return TCL_OK;
}

static CmdSpec text3DOps[] = {
    {"add",       1, Text3DAddOp, 6, 6, "font size text name"},
    {"bold",      1, Text3DBoldOp, 3, 4, "bool ?name?"},
    {"color",     1, Text3DColorOp, 5, 6, "r g b ?name?"},
    {"delete",    1, Text3DDeleteOp, 2, 3, "?name?"},
    {"fntfamily", 4, Text3DFontFamilyOp, 3, 4, "font ?name?"},
    {"fntsize",   4, Text3DFontSizeOp, 3, 4, "size ?name?"},
    {"followcam", 2, Text3DFollowCameraOp, 3, 4, "bool ?name?"},
    {"italic",    1, Text3DItalicOp, 3, 4, "bool ?name?"},
    {"opacity",   2, Text3DOpacityOp, 3, 4, "value ?name?"},
    {"orient",    4, Text3DOrientOp, 6, 7, "qw qx qy qz ?name?"},
    {"origin",    4, Text3DOriginOp, 5, 6, "x y z ?name?"},
    {"pos",       1, Text3DPositionOp, 5, 6, "x y z ?name?"},
    {"scale",     1, Text3DScaleOp, 5, 6, "sx sy sz ?name?"},
    {"shadow",    1, Text3DShadowOp, 3, 4, "bool ?name?"},
    {"text",      1, Text3DTextOp, 3, 4, "text ?name?"},
    {"visible",   1, Text3DVisibleOp, 3, 4, "bool ?name?"},
};
static int nText3DOps = NumCmdSpecs(text3DOps);

static int
Text3DCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nText3DOps, text3DOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
VolumeBlendModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Volume::BlendMode mode;
    const char *string = Tcl_GetString(objv[2]);
    char c = string[0];
    if ((c == 'c') && (strcmp(string, "composite") == 0)) {
        mode = Volume::BLEND_COMPOSITE;
    } else if ((c == 'm') && (strcmp(string, "max_intensity") == 0)) {
        mode = Volume::BLEND_MAX_INTENSITY;
    } else if ((c == 'm') && (strcmp(string, "min_intensity") == 0)) {
        mode = Volume::BLEND_MIN_INTENSITY;
    } else if ((c == 'a') && (strcmp(string, "additive") == 0)) {
        mode = Volume::BLEND_ADDITIVE;
    } else {
        Tcl_AppendResult(interp, "bad blendmode option \"", string,
                         "\": should be copmosite, max_intensity, min_intensity, or additive", (char*)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *dataSetName = Tcl_GetString(objv[3]);
        g_renderer->setVolumeBlendMode(dataSetName, mode);
    } else {
        g_renderer->setVolumeBlendMode("all", mode);
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
#if 1
    distance = 1.0 / (quality * (maxFactor - 1.0) + 1.0);
#else
    if (quality >= 0.5) {
        distance = 1.0 / ((quality - 0.5) * (maxFactor - 1.0) * 2.0 + 1.0);
    } else {
        distance = ((0.5 - quality) * (maxFactor - 1.0) * 2.0 + 1.0);
    }
#endif
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

static CmdSpec volumeShadingOps[] = {
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

    proc = GetOpFromObj(interp, nVolumeShadingOps, volumeShadingOps,
                        CMDSPEC_ARG2, objc, objv, 0);
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

static CmdSpec volumeOps[] = {
    {"add",          1, VolumeAddOp,        2, 3, "?dataSetName?"},
    {"blendmode",    1, VolumeBlendModeOp,  3, 4, "blendMode ?dataSetName?"},
    {"colormap",     1, VolumeColorMapOp,   3, 4, "colorMapName ?dataSetName?"},
    {"delete",       1, VolumeDeleteOp,     2, 3, "?dataSetName?"},
    {"lighting",     1, VolumeLightingOp,   3, 4, "bool ?dataSetName?"},
    {"opacity",      2, VolumeOpacityOp,    3, 4, "val ?dataSetName?"},
    {"orient",       2, VolumeOrientOp,     6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          1, VolumePositionOp,   5, 6, "x y z ?dataSetName?"},
    {"quality",      1, VolumeSampleRateOp, 3, 4, "val ?dataSetName?"},
    {"scale",        2, VolumeScaleOp,      5, 6, "sx sy sz ?dataSetName?"},
    {"shading",      2, VolumeShadingOp,    4, 6, "oper val ?dataSetName?"},
    {"visible",      1, VolumeVisibleOp,    3, 4, "bool ?dataSetName?"}
};
static int nVolumeOps = NumCmdSpecs(volumeOps);

static int
VolumeCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nVolumeOps, volumeOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
WarpCloudStyleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    Warp::CloudStyle style;
    char *str =  Tcl_GetString(objv[2]);
    if (str[0] == 'm' && strcmp(str, "mesh") == 0) {
        style = Warp::CLOUD_MESH;
    } else if (str[0] == 'p' && strcmp(str, "points") == 0) {
        style = Warp::CLOUD_POINTS;
    } else {
        Tcl_AppendResult(interp, "bad cloudstyle option \"", str,
                         "\": should be one of: 'mesh', 'points'", (char*)NULL);
        return TCL_ERROR;
    }

    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setWarpCloudStyle(name, style);
    } else {
        g_renderer->setWarpCloudStyle("all", style);
    }
    return TCL_OK;
}

static int
WarpColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
        g_renderer->setGraphicsObjectColor<Warp>(name, color);
    } else {
        g_renderer->setGraphicsObjectColor<Warp>("all", color);
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
WarpColorModeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Warp::ColorMode mode;
    const char *str = Tcl_GetString(objv[2]);
    if (str[0] == 'c' &&
        (strcmp(str, "ccolor") == 0 || strcmp(str, "constant") == 0)) {
        mode = Warp::COLOR_CONSTANT;
    } else if (str[0] == 's' && strcmp(str, "scalar") == 0) {
        mode = Warp::COLOR_BY_SCALAR;
    } else if (str[0] == 'v' && strcmp(str, "vmag") == 0) {
        mode = Warp::COLOR_BY_VECTOR_MAGNITUDE;
    } else if (str[0] == 'v' && strcmp(str, "vx") == 0) {
        mode = Warp::COLOR_BY_VECTOR_X;
    } else if (str[0] == 'v' && strcmp(str, "vy") == 0) {
        mode = Warp::COLOR_BY_VECTOR_Y;
    } else if (str[0] == 'v' && strcmp(str, "vz") == 0) {
        mode = Warp::COLOR_BY_VECTOR_Z;
    } else {
        Tcl_AppendResult(interp, "bad color mode option \"", str,
                         "\": should be one of: 'scalar', 'vmag', 'vx', 'vy', 'vz', 'ccolor', 'constant'", (char*)NULL);
        return TCL_ERROR;
    }
    const char *fieldName = Tcl_GetString(objv[3]);
    if (mode == Warp::COLOR_CONSTANT) {
        fieldName = NULL;
    }
    if (objc == 5) {
        const char *name = Tcl_GetString(objv[4]);
        g_renderer->setWarpColorMode(name, mode, fieldName);
    } else {
        g_renderer->setWarpColorMode("all", mode, fieldName);
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
WarpPointSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    float size;
    if (GetFloatFromObj(interp, objv[2], &size) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectPointSize<Warp>(name, size);
    } else {
        g_renderer->setGraphicsObjectPointSize<Warp>("all", size);
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
WarpPreInterpOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        const char *name = Tcl_GetString(objv[3]);
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<Warp>(name, state);
    } else {
        g_renderer->setGraphicsObjectInterpolateBeforeMapping<Warp>("all", state);
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

static CmdSpec warpOps[] = {
    {"add",          1, WarpAddOp, 2, 3, "?dataSetName?"},
    {"ccolor",       2, WarpColorOp, 5, 6, "r g b ?dataSetName?"},
    {"cloudstyle",   2, WarpCloudStyleOp, 3, 4, "style ?dataSetName?"},
    {"color",        5, WarpColorOp, 5, 6, "r g b ?dataSetName?"},
    {"colormap",     7, WarpColorMapOp, 3, 4, "colorMapName ?dataSetName?"},
    {"colormode",    7, WarpColorModeOp, 4, 5, "mode fieldName ?dataSetName?"},
    {"delete",       1, WarpDeleteOp, 2, 3, "?dataSetName?"},
    {"edges",        1, WarpEdgeVisibilityOp, 3, 4, "bool ?dataSetName?"},
    {"lighting",     3, WarpLightingOp, 3, 4, "bool ?dataSetName?"},
    {"linecolor",    5, WarpLineColorOp, 5, 6, "r g b ?dataSetName?"},
    {"linewidth",    5, WarpLineWidthOp, 3, 4, "width ?dataSetName?"},
    {"opacity",      2, WarpOpacityOp, 3, 4, "value ?dataSetName?"},
    {"orient",       2, WarpOrientOp, 6, 7, "qw qx qy qz ?dataSetName?"},
    {"pos",          2, WarpPositionOp, 5, 6, "x y z ?dataSetName?"},
    {"preinterp",    2, WarpPreInterpOp, 3, 4, "bool ?dataSetName?"},
    {"ptsize",       2, WarpPointSizeOp, 3, 4, "size ?dataSetName?"},
    {"scale",        1, WarpScaleOp, 5, 6, "sx sy sz ?dataSetName?"},
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

    proc = GetOpFromObj(interp, nWarpOps, warpOps,
                        CMDSPEC_ARG1, objc, objv, 0);
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
VtkVis::processCommands(Tcl_Interp *interp,
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
VtkVis::handleError(Tcl_Interp *interp,
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

#ifdef USE_THREADS
            queueResponse(clientData, oss.str().c_str(), nBytes, Response::VOLATILE, Response::ERROR);
#else
            if (write(fdOut, oss.str().c_str(), nBytes) < 0) {
                ERROR("write failed: %s", strerror(errno));
                return -1;
            }
#endif
        }
    }

    string = getUserMessages();
    nBytes = strlen(string);
    if (nBytes > 0) {
        TRACE("userError=(%s)", string);

        std::ostringstream oss;
        oss << "nv>viserror -type error -token " << g_stats.nCommands << " -bytes " << nBytes << "\n" << string;
        nBytes = oss.str().length();

#ifdef USE_THREADS
        queueResponse(clientData, oss.str().c_str(), nBytes, Response::VOLATILE, Response::ERROR);
#else
        if (write(fdOut, oss.str().c_str(), nBytes) < 0) {
            ERROR("write failed: %s", strerror(errno));
            return -1;
        }
#endif
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
VtkVis::initTcl(Tcl_Interp *interp, ClientData clientData)
{
    Tcl_MakeSafe(interp);
    Tcl_CreateObjCommand(interp, "arc",            ArcCmd,            clientData, NULL);
    Tcl_CreateObjCommand(interp, "arrow",          ArrowCmd,          clientData, NULL);
    Tcl_CreateObjCommand(interp, "axis",           AxisCmd,           clientData, NULL);
    Tcl_CreateObjCommand(interp, "box",            BoxCmd,            clientData, NULL);
    Tcl_CreateObjCommand(interp, "camera",         CameraCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "clientinfo",     ClientInfoCmd,     clientData, NULL);
    Tcl_CreateObjCommand(interp, "colormap",       ColorMapCmd,       clientData, NULL);
    Tcl_CreateObjCommand(interp, "cone",           ConeCmd,           clientData, NULL);
    Tcl_CreateObjCommand(interp, "contour2d",      Contour2DCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "contour3d",      Contour3DCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "cutplane",       CutplaneCmd,       clientData, NULL);
    Tcl_CreateObjCommand(interp, "cylinder",       CylinderCmd,       clientData, NULL);
    Tcl_CreateObjCommand(interp, "dataset",        DataSetCmd,        clientData, NULL);
    Tcl_CreateObjCommand(interp, "disk",           DiskCmd,           clientData, NULL);
    Tcl_CreateObjCommand(interp, "glyphs",         GlyphsCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "group",          GroupCmd,          clientData, NULL);
    Tcl_CreateObjCommand(interp, "heightmap",      HeightMapCmd,      clientData, NULL);
    Tcl_CreateObjCommand(interp, "image",          ImageCmd,          clientData, NULL);
    Tcl_CreateObjCommand(interp, "imgcutplane",    ImageCutplaneCmd,  clientData, NULL);
    Tcl_CreateObjCommand(interp, "imgflush",       ImageFlushCmd,     clientData, NULL);
    Tcl_CreateObjCommand(interp, "legend",         LegendCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "legend2",        LegendSimpleCmd,   clientData, NULL);
    Tcl_CreateObjCommand(interp, "lic",            LICCmd,            clientData, NULL);
    Tcl_CreateObjCommand(interp, "line",           LineCmd,           clientData, NULL);
    Tcl_CreateObjCommand(interp, "molecule",       MoleculeCmd,       clientData, NULL);
    Tcl_CreateObjCommand(interp, "outline",        OutlineCmd,        clientData, NULL);
    Tcl_CreateObjCommand(interp, "parallelepiped", ParallelepipedCmd, clientData, NULL);
    Tcl_CreateObjCommand(interp, "polydata",       PolyDataCmd,       clientData, NULL);
    Tcl_CreateObjCommand(interp, "polygon",        PolygonCmd,        clientData, NULL);
    Tcl_CreateObjCommand(interp, "pseudocolor",    PseudoColorCmd,    clientData, NULL);
    Tcl_CreateObjCommand(interp, "renderer",       RendererCmd,       clientData, NULL);
    Tcl_CreateObjCommand(interp, "screen",         ScreenCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "sphere",         SphereCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "streamlines",    StreamlinesCmd,    clientData, NULL);
    Tcl_CreateObjCommand(interp, "text3d",         Text3DCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "volume",         VolumeCmd,         clientData, NULL);
    Tcl_CreateObjCommand(interp, "warp",           WarpCmd,           clientData, NULL);
}

/**
 * \brief Delete Tcl commands and interpreter
 */
void VtkVis::exitTcl(Tcl_Interp *interp)
{
    Tcl_DeleteCommand(interp, "arc");
    Tcl_DeleteCommand(interp, "arrow");
    Tcl_DeleteCommand(interp, "axis");
    Tcl_DeleteCommand(interp, "box");
    Tcl_DeleteCommand(interp, "camera");
    Tcl_DeleteCommand(interp, "clientinfo");
    Tcl_DeleteCommand(interp, "colormap");
    Tcl_DeleteCommand(interp, "cone");
    Tcl_DeleteCommand(interp, "contour2d");
    Tcl_DeleteCommand(interp, "contour3d");
    Tcl_DeleteCommand(interp, "cutplane");
    Tcl_DeleteCommand(interp, "cylinder");
    Tcl_DeleteCommand(interp, "dataset");
    Tcl_DeleteCommand(interp, "disk");
    Tcl_DeleteCommand(interp, "glyphs");
    Tcl_DeleteCommand(interp, "group");
    Tcl_DeleteCommand(interp, "heightmap");
    Tcl_DeleteCommand(interp, "image");
    Tcl_DeleteCommand(interp, "imgcutplane");
    Tcl_DeleteCommand(interp, "imgflush");
    Tcl_DeleteCommand(interp, "legend");
    Tcl_DeleteCommand(interp, "legend2");
    Tcl_DeleteCommand(interp, "lic");
    Tcl_DeleteCommand(interp, "line");
    Tcl_DeleteCommand(interp, "molecule");
    Tcl_DeleteCommand(interp, "outline");
    Tcl_DeleteCommand(interp, "parallelepiped");
    Tcl_DeleteCommand(interp, "polydata");
    Tcl_DeleteCommand(interp, "polygon");
    Tcl_DeleteCommand(interp, "pseudocolor");
    Tcl_DeleteCommand(interp, "renderer");
    Tcl_DeleteCommand(interp, "screen");
    Tcl_DeleteCommand(interp, "sphere");
    Tcl_DeleteCommand(interp, "streamlines");
    Tcl_DeleteCommand(interp, "text3d");
    Tcl_DeleteCommand(interp, "volume");
    Tcl_DeleteCommand(interp, "warp");

    Tcl_DeleteInterp(interp);
}
