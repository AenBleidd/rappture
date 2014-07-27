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

#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>

#include <osgEarth/Registry>
#include <osgEarthFeatures/FeatureModelSource>
#include <osgEarthSymbology/Color>
#include <osgEarthSymbology/Style>
#include <osgEarthSymbology/StyleSheet>
#include <osgEarthSymbology/LineSymbol>
#include <osgEarthSymbology/RenderSymbol>

#include <osgEarthDrivers/debug/DebugOptions>
#include <osgEarthDrivers/gdal/GDALOptions>
#include <osgEarthDrivers/tms/TMSOptions>
#include <osgEarthDrivers/wms/WMSOptions>
#include <osgEarthDrivers/xyz/XYZOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>
#include <osgEarthDrivers/feature_ogr/OGRFeatureOptions>

#include "Trace.h"
#include "CmdProc.h"
#include "ReadBuffer.h"
#include "Types.h"
#include "RendererCmd.h"
#include "RenderServer.h"
#include "Renderer.h"
#include "Stats.h"
#include "PPMWriter.h"
#include "TGAWriter.h"
#include "ResponseQueue.h"
#ifdef USE_READ_THREAD
#include "CommandQueue.h"
#endif

using namespace GeoVis;

static int lastCmdStatus;

#ifndef USE_THREADS
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
#endif

static bool
SocketRead(char *bytes, size_t len)
{
    ReadBuffer::BufferStatus status;
    status = g_inBufPtr->followingData((unsigned char *)bytes, len);
    TRACE("followingData status: %d", status);
    return (status == ReadBuffer::OK);
}

ssize_t
GeoVis::queueResponse(const void *bytes, size_t len, 
                      Response::AllocationType allocType,
                      Response::ResponseType type)
{
#ifdef USE_THREADS
    Response *response = new Response(type);
    response->setMessage((unsigned char *)bytes, len, allocType);
    g_outQueue->enqueue(response);
    return (ssize_t)len;
#else
    return SocketWrite(bytes, len);
#endif
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
CameraDeleteViewpointOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                        Tcl_Obj *const *objv)
{
    char *name = Tcl_GetString(objv[2]);

    g_renderer->removeNamedViewpoint(name);
    return TCL_OK;
}

static int
CameraGetViewpointOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
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

static int
CameraGoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    int x, y;
    if (Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK) {
        return TCL_ERROR;
    }
    double zoom = 1.0;
    if (objc > 4) {
        if (Tcl_GetDoubleFromObj(interp, objv[4], &zoom) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    double duration = 1.0;
    if (objc > 5) {
        if (Tcl_GetDoubleFromObj(interp, objv[5], &duration) != TCL_OK) {
            return TCL_ERROR;
        }
    }

    osgEarth::GeoPoint mapPoint;
    if (g_renderer->mapMouseCoords(x, y, mapPoint)) {
        osgEarth::Viewpoint vpt = g_renderer->getViewpoint();
        vpt.x() = mapPoint.x();
        vpt.y() = mapPoint.y();
        vpt.z() = mapPoint.z();
        vpt.setRange(vpt.getRange() * zoom);
        g_renderer->setViewpoint(vpt, duration);
    } else {
        // Out of map bounds
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
CameraRestoreViewpointOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                         Tcl_Obj *const *objv)
{
    char *name = Tcl_GetString(objv[2]);

    double duration = 0.0;
    if (objc > 3) {
        if (Tcl_GetDoubleFromObj(interp, objv[3], &duration) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (!g_renderer->restoreNamedViewpoint(name, duration)) {
        Tcl_AppendResult(interp, "camera viewpoint \"", name,
                         "\" not found", (char*)NULL);
        return TCL_ERROR;
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
CameraSaveViewpointOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    char *name = Tcl_GetString(objv[2]);

    g_renderer->saveNamedViewpoint(name);
    return TCL_OK;
}

static int
CameraSetDistanceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                    Tcl_Obj *const *objv)
{
    double dist;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &dist) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setCameraDistance(dist);
    return TCL_OK;
}

static int
CameraSetViewpointOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    double x, y, z, heading, pitch, distance;
    double duration = 0.0;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &z) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[5], &heading) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[6], &pitch) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[7], &distance) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 8) {
        if (Tcl_GetDoubleFromObj(interp, objv[8], &duration) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (objc > 9) {
        char *srsInit = Tcl_GetString(objv[9]);
        if (strlen(srsInit) > 0) {
            osgEarth::SpatialReference *srs = NULL;
            if (objc > 10) {
                char *vertDatum = Tcl_GetString(objv[10]);
                srs = osgEarth::SpatialReference::get(srsInit, vertDatum);
            } else {
                srs = osgEarth::SpatialReference::get(srsInit);
            }
            if (srs == NULL) {
                return TCL_ERROR;
            }
            osgEarth::Viewpoint view(x, y, z, heading, pitch, distance, srs);
            g_renderer->setViewpoint(view, duration);
        } else {
            osgEarth::Viewpoint view(x, y, z, heading, pitch, distance);
            g_renderer->setViewpoint(view, duration);
        }
    } else {
        osgEarth::Viewpoint view(x, y, z, heading, pitch, distance);
        g_renderer->setViewpoint(view, duration);
    }
    return TCL_OK;
}

static int
CameraThrowOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;

    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setThrowingEnabled(state);
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
    {"delete",  2, CameraDeleteViewpointOp,  3, 3, "name"},
    {"dist",    2, CameraSetDistanceOp,      3, 3, "distance"},
    {"get",     2, CameraGetViewpointOp,     2, 2, ""},
    {"go",      2, CameraGoOp,               4, 6, "x y ?zoomFactor? ?duration?"},
    {"orient",  1, CameraOrientOp,           6, 6, "qw qx qy qz"},
    {"pan",     1, CameraPanOp,              4, 4, "panX panY"},
    {"reset",   4, CameraResetOp,            2, 3, "?all?"},
    {"restore", 4, CameraRestoreViewpointOp, 3, 4, "name ?duration?"},
    {"rotate",  2, CameraRotateOp,           4, 4, "azimuth elevation"},
    {"save",    2, CameraSaveViewpointOp,    3, 3, "name"},
    {"set",     2, CameraSetViewpointOp,     8, 11, "x y z heading pitch distance ?duration? ?srs? ?vertDatum?"},
    {"throw",   1, CameraThrowOp,            3, 3, "bool"},
    {"zoom",    1, CameraZoomOp,             3, 3, "zoomAmount"}
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

    /* Client arguments. */
    if (Tcl_ListObjGetElements(interp, objv[1], &numItems, &items) != TCL_OK) {
	return TCL_ERROR;
    }

    /* First try to see if we already have an open descriptor */
    int fd = getStatsFile();
    if (fd < 0) {
        /* Use the initial client key value pairs as the parts for generating
         * a unique file name. */
        fd = GeoVis::getStatsFile(Tcl_GetString(objv[1]));
        if (fd < 0) {
            Tcl_AppendResult(interp, "can't open stats file: ", 
                             Tcl_PosixError(interp), (char *)NULL);
            return TCL_ERROR;
        }
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_IncrRefCount(listObjPtr);
    if (first) {
        first = false;
        objPtr = Tcl_NewStringObj("render_start", 12);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        /* renderer */
        objPtr = Tcl_NewStringObj("renderer", 8);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("geovis", 6);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        /* pid */
        objPtr = Tcl_NewStringObj("pid", 3);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(getpid()));
        /* host */
        objPtr = Tcl_NewStringObj("host", 4);
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
    /* client items */
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

    osg::TransferFunction1D *colorMap = new osg::TransferFunction1D;
    colorMap->allocate(256);

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
        colorMap->setColor(val[0], osg::Vec4f(val[1], val[2], val[3], 1.0), false);
    }

    colorMap->updateImage();

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
#if 0
        ColorMap::OpacityControlPoint ocp;
        ocp.value = val[0];
        ocp.alpha = val[1];
        colorMap->addOpacityControlPoint(ocp);
#endif
    }

    //colorMap->build();
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
ImageFlushCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    lastCmdStatus = TCL_BREAK;
    return TCL_OK;
}

static int
KeyPressOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    int key;
    if (Tcl_GetIntFromObj(interp, objv[2], &key) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->keyPress(key);
    return TCL_OK;
}

static int
KeyReleaseOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    int key;
    if (Tcl_GetIntFromObj(interp, objv[2], &key) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->keyRelease(key);
    return TCL_OK;
}

static CmdSpec keyOps[] = {
    {"press",    1, KeyPressOp,       3, 3, "key"},
    {"release",  1, KeyReleaseOp,     3, 3, "key"},
};
static int nKeyOps = NumCmdSpecs(keyOps);

static int
KeyCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nKeyOps, keyOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
MapCoordsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    int x, y;
    if (Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    osgEarth::GeoPoint mapPoint;
    size_t length;
    char mesg[256];
    if (g_renderer->mapMouseCoords(x, y, mapPoint)) {
        std::string srsInit;
        std::string verticalDatum;
        if (objc > 4) {
            srsInit = Tcl_GetString(objv[4]);
            if (objc > 5) {
                verticalDatum = Tcl_GetString(objv[5]);
            }
            osgEarth::SpatialReference *outSRS =
                osgEarth::SpatialReference::get(srsInit, verticalDatum);
            if (outSRS == NULL) {
                Tcl_AppendResult(interp, "bad SRS \"", srsInit.c_str(), "\"", (char*)NULL);
                return TCL_ERROR;
            }
            mapPoint = mapPoint.transform(outSRS);
            if (!mapPoint.isValid()) {
                Tcl_AppendResult(interp, "Could not transform map point to requested SRS", (char*)NULL);
                return TCL_ERROR;
            }
        }
        // send coords to client
        length = snprintf(mesg, sizeof(mesg),
                          "nv>map coords %g %g %g %d %d {%s} {%s}\n",
                          mapPoint.x(), mapPoint.y(), mapPoint.z(),
                          x, y,
                          mapPoint.getSRS()->getHorizInitString().c_str(),
                          mapPoint.getSRS()->getVertInitString().c_str());

        queueResponse(mesg, length, Response::VOLATILE);
    } else {
        // Out of range
        length = snprintf(mesg, sizeof(mesg),
                          "nv>map coords invalid %d %d\n", x, y);

        queueResponse(mesg, length, Response::VOLATILE);
    }
    return TCL_OK;
}

static int
MapGraticuleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 3) {
        Renderer::GraticuleType type;
        char *typeStr = Tcl_GetString(objv[3]);
        if (typeStr[0] == 'g' && strcmp(typeStr, "geodetic") == 0) {
            type = Renderer::GRATICULE_GEODETIC;
        } else if (typeStr[0] == 'u' && strcmp(typeStr, "utm") == 0) {
            type = Renderer::GRATICULE_UTM;
        } else if (typeStr[0] == 'm' && strcmp(typeStr, "mgrs") == 0) {
            type = Renderer::GRATICULE_MGRS;
        } else {
            return TCL_ERROR;
        }
        g_renderer->setGraticule(state, type);
    } else {
        g_renderer->setGraticule(state);
    }
    return TCL_OK;
}

static int
MapLayerAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    char *type = Tcl_GetString(objv[3]);
    if (type[0] == 'i' && strcmp(type, "image") == 0) {
        char *driver = Tcl_GetString(objv[4]);
        std::string url;
        if (objc > 6) {
            char *urlIn = Tcl_GetString(objv[5]);
            url = g_renderer->getCanonicalPath(std::string(urlIn));
            if (url.empty()) {
                Tcl_AppendResult(interp, "file not found: \"",
                                 urlIn, "\"", (char*)NULL);
                return TCL_ERROR;
            }
        }
        if (driver[0] == 'd' && strcmp(driver, "debug") == 0) {
            osgEarth::Drivers::DebugOptions opts;
            char *name = Tcl_GetString(objv[5]);
            g_renderer->addImageLayer(name, opts);
        } else if (driver[0] == 'g' && strcmp(driver, "gdal") == 0) {
            osgEarth::Drivers::GDALOptions opts;
            opts.url() = url;
            char *name = Tcl_GetString(objv[6]);
            g_renderer->addImageLayer(name, opts);
        } else if (driver[0] == 't' && strcmp(driver, "tms") == 0) {
            osgEarth::Drivers::TMSOptions opts;
            //char *tmsType = Tcl_GetString(objv[5]);
            //char *format = Tcl_GetString(objv[6]);
            opts.url() = url;
            //opts.tmsType() = tmsType;
            //opts.format() = format;
            char *name = Tcl_GetString(objv[6]);
            g_renderer->addImageLayer(name, opts);
        } else if (driver[0] == 'w' && strcmp(driver, "wms") == 0) {
            osgEarth::Drivers::WMSOptions opts;
            char *wmsLayers = Tcl_GetString(objv[6]);
            char *format = Tcl_GetString(objv[7]);
            bool transparent;
            if (GetBooleanFromObj(interp, objv[8], &transparent) != TCL_OK) {
                return TCL_ERROR;
            }
            opts.url() = url;
            opts.layers() = wmsLayers;
            opts.format() = format;
            opts.transparent() = transparent;

            char *name = Tcl_GetString(objv[9]);
            g_renderer->addImageLayer(name, opts);
        } else if (driver[0] == 'x' && strcmp(driver, "xyz") == 0) {
            osgEarth::Drivers::XYZOptions opts;
            opts.url() = url;
            opts.profile() = osgEarth::ProfileOptions("global-mercator");
            //bool invertY = false;
            //opts.invertY() = invertY;
            //opts.format() = Tcl_GetString(objv[6]);
            char *name = Tcl_GetString(objv[6]);
            g_renderer->addImageLayer(name, opts);
        } else {
            Tcl_AppendResult(interp, "unknown image driver \"", driver,
                             "\": should be 'debug', 'gdal', 'tms', 'wms' or 'xyz'", (char*)NULL);
            return TCL_ERROR;
        }
    } else if (type[0] == 'e' && strcmp(type, "elevation") == 0) {
        char *driver = Tcl_GetString(objv[4]);
        char *urlIn = Tcl_GetString(objv[5]);
        std::string url = g_renderer->getCanonicalPath(std::string(urlIn));
        if (url.empty()) {
            Tcl_AppendResult(interp, "file not found: \"",
                             urlIn, "\"", (char*)NULL);
            return TCL_ERROR;
        }

        if (driver[0] == 'g' && strcmp(driver, "gdal") == 0) {
            osgEarth::Drivers::GDALOptions opts;
            opts.url() = url;
            char *name = Tcl_GetString(objv[6]);
            g_renderer->addElevationLayer(name, opts);
        } else if (driver[0] == 't' && strcmp(driver, "tms") == 0) {
            osgEarth::Drivers::TMSOptions opts;
            //char *tmsType = Tcl_GetString(objv[6]);
            //char *format = Tcl_GetString(objv[7]);
            opts.url() = url;
            //opts.tmsType() = tmsType;
            //opts.format() = format;
            char *name = Tcl_GetString(objv[6]);
            g_renderer->addElevationLayer(name, opts);
        } else {
            Tcl_AppendResult(interp, "unknown elevation driver \"", driver,
                             "\": should be 'gdal' or 'tms'", (char*)NULL);
            return TCL_ERROR;
        }
    } else if (type[0] == 'p' && strcmp(type, "point") == 0) {
        osgEarth::Drivers::OGRFeatureOptions opts;
        char *urlIn = Tcl_GetString(objv[4]);
        std::string url = g_renderer->getCanonicalPath(std::string(urlIn));
        if (url.empty()) {
            Tcl_AppendResult(interp, "file not found: \"",
                             urlIn, "\"", (char*)NULL);
            return TCL_ERROR;
        }
        char *name = Tcl_GetString(objv[5]);

        opts.url() = url;

        osgEarth::Symbology::Style style;
        osgEarth::Symbology::PointSymbol *ls = style.getOrCreateSymbol<osgEarth::Symbology::PointSymbol>();
        ls->fill()->color() = osgEarth::Symbology::Color::Black;
        ls->size() = 2.0f;

        osgEarth::Symbology::RenderSymbol* rs = style.getOrCreateSymbol<osgEarth::Symbology::RenderSymbol>();
        rs->depthOffset()->enabled() = true;
        rs->depthOffset()->minBias() = 1000;

        osgEarth::Drivers::FeatureGeomModelOptions geomOpts;
        geomOpts.featureOptions() = opts;
        geomOpts.styles() = new osgEarth::Symbology::StyleSheet();
        geomOpts.styles()->addStyle(style);
        geomOpts.enableLighting() = false;
        g_renderer->addModelLayer(name, geomOpts);
    } else if (type[0] == 'p' && strcmp(type, "polygon") == 0) {
        osgEarth::Drivers::OGRFeatureOptions opts;
        char *urlIn = Tcl_GetString(objv[4]);
        std::string url = g_renderer->getCanonicalPath(std::string(urlIn));
        if (url.empty()) {
            Tcl_AppendResult(interp, "file not found: \"",
                             urlIn, "\"", (char*)NULL);
            return TCL_ERROR;
        }
        char *name = Tcl_GetString(objv[5]);
        opts.url() = url;

        osgEarth::Symbology::Style style;
        osgEarth::Symbology::PolygonSymbol *ls = style.getOrCreateSymbol<osgEarth::Symbology::PolygonSymbol>();
        ls->fill()->color() = osgEarth::Symbology::Color::White;

        osgEarth::Symbology::RenderSymbol* rs = style.getOrCreateSymbol<osgEarth::Symbology::RenderSymbol>();
        rs->depthOffset()->enabled() = true;
        rs->depthOffset()->minBias() = 1000;

        osgEarth::Drivers::FeatureGeomModelOptions geomOpts;
        geomOpts.featureOptions() = opts;
        geomOpts.styles() = new osgEarth::Symbology::StyleSheet();
        geomOpts.styles()->addStyle(style);
        geomOpts.enableLighting() = false;
        g_renderer->addModelLayer(name, geomOpts);
    } else if (type[0] == 'l' && strcmp(type, "line") == 0) {
        osgEarth::Drivers::OGRFeatureOptions opts;
        char *urlIn = Tcl_GetString(objv[4]);
        std::string url = g_renderer->getCanonicalPath(std::string(urlIn));
        if (url.empty()) {
            Tcl_AppendResult(interp, "file not found: \"",
                             urlIn, "\"", (char*)NULL);
            return TCL_ERROR;
        }
        char *name = Tcl_GetString(objv[5]);
        opts.url() = url;

        osgEarth::Symbology::Style style;
        osgEarth::Symbology::LineSymbol *ls = style.getOrCreateSymbol<osgEarth::Symbology::LineSymbol>();
        ls->stroke()->color() = osgEarth::Symbology::Color::Black;
        ls->stroke()->width() = 2.0f;
#if 0
        osgEarth::Symbology::AltitudeSymbol *alt = style.getOrCreateSymbol<osgEarth::Symbology::AltitudeSymbol>();
        alt->clamping() = osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN;
        //alt->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_DRAPE;
        alt->technique() = osgEarth::Symbology::AltitudeSymbol::TECHNIQUE_GPU;
#endif
#if 1
        osgEarth::Symbology::RenderSymbol* rs = style.getOrCreateSymbol<osgEarth::Symbology::RenderSymbol>();
        rs->depthOffset()->enabled() = true;
        rs->depthOffset()->minBias() = 1000;
#endif
        osgEarth::Drivers::FeatureGeomModelOptions geomOpts;
        geomOpts.featureOptions() = opts;
        geomOpts.styles() = new osgEarth::Symbology::StyleSheet();
        geomOpts.styles()->addStyle(style);
        geomOpts.enableLighting() = false;
        g_renderer->addModelLayer(name, geomOpts);
   } else if (type[0] == 't' && strcmp(type, "text") == 0) {
        osgEarth::Drivers::OGRFeatureOptions opts;
        char *urlIn = Tcl_GetString(objv[4]);
        std::string url = g_renderer->getCanonicalPath(std::string(urlIn));
        if (url.empty()) {
            Tcl_AppendResult(interp, "file not found: \"",
                             urlIn, "\"", (char*)NULL);
            return TCL_ERROR;
        }
        char *content = Tcl_GetString(objv[5]);
        char *priority = Tcl_GetString(objv[6]);
        char *name = Tcl_GetString(objv[7]);

#if 0
        double fgR = 1.0, fgG = 1.0, fgB = 1.0;
        double bgR = 0.0, bgG = 0.0, bgB = 0.0;
        if (objc > 8) {
            if (Tcl_GetDoubleFromObj(interp, objv[8], &fgR) != TCL_OK ||
                Tcl_GetDoubleFromObj(interp, objv[9], &fgG) != TCL_OK ||
                Tcl_GetDoubleFromObj(interp, objv[10], &fgB) != TCL_OK ||
                Tcl_GetDoubleFromObj(interp, objv[11], &bgR) != TCL_OK ||
                Tcl_GetDoubleFromObj(interp, objv[12], &bgG) != TCL_OK ||
                Tcl_GetDoubleFromObj(interp, objv[13], &bgB) != TCL_OK) {
                return TCL_ERROR;
            }
        }
#endif
        opts.url() = url;

        osgEarth::Symbology::Style style;
        osgEarth::Symbology::TextSymbol *ts = style.getOrCreateSymbol<osgEarth::Symbology::TextSymbol>();
        ts->halo()->color() = osgEarth::Symbology::Color::Black; //::Color(bgR, bgG, bgB);
        ts->halo()->width() = 2.0f;
        ts->fill()->color() = osgEarth::Symbology::Color::White; //::Color(fgR, fgG, fgB);
        ts->content() = osgEarth::Symbology::StringExpression(content);
        ts->priority() = osgEarth::Symbology::NumericExpression(priority);
        ts->removeDuplicateLabels() = true;
        ts->size() = 16.0f;
        ts->alignment() = osgEarth::Symbology::TextSymbol::ALIGN_CENTER_CENTER;
        ts->declutter() = true;

        osgEarth::Symbology::RenderSymbol* rs = style.getOrCreateSymbol<osgEarth::Symbology::RenderSymbol>();
        rs->depthOffset()->enabled() = true;
        rs->depthOffset()->minBias() = 1000;

        osgEarth::Drivers::FeatureGeomModelOptions geomOpts;
        geomOpts.featureOptions() = opts;
        geomOpts.styles() = new osgEarth::Symbology::StyleSheet();
        geomOpts.styles()->addStyle(style);
        geomOpts.enableLighting() = false;
        g_renderer->addModelLayer(name, geomOpts);
    } else {
        Tcl_AppendResult(interp, "unknown map layer type \"", type,
                         "\": should be 'image', 'elevation' or 'model'", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
MapLayerDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    if (objc > 3) {
        char *name = Tcl_GetString(objv[3]);
        g_renderer->removeImageLayer(name);
        g_renderer->removeElevationLayer(name);
        g_renderer->removeModelLayer(name);
    } else {
        g_renderer->clearMap();
    }

    return TCL_OK;
}

static int
MapLayerMoveOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    int pos;
    if (Tcl_GetIntFromObj(interp, objv[3], &pos) != TCL_OK) {
        return TCL_ERROR;
    }
    char *name = Tcl_GetString(objv[4]);
    if (pos < 0) {
        Tcl_AppendResult(interp, "bad layer pos ", pos,
                         ": must be positive", (char*)NULL);
        return TCL_ERROR;
    }
    g_renderer->moveImageLayer(name, (unsigned int)pos);
    g_renderer->moveElevationLayer(name, (unsigned int)pos);
    g_renderer->moveModelLayer(name, (unsigned int)pos);

    return TCL_OK;
}

static int
MapLayerOpacityOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    char *name = Tcl_GetString(objv[4]);
    if (opacity < 0.0 || opacity > 1.0) {
        Tcl_AppendResult(interp, "bad layer opacity ", opacity,
                         ": must be [0,1]", (char*)NULL);
        return TCL_ERROR;
    }
    g_renderer->setImageLayerOpacity(name, opacity);
    g_renderer->setModelLayerOpacity(name, opacity);

    return TCL_OK;
}

static int
MapLayerNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    std::vector<std::string> layers;
    if (objc < 4) {
        g_renderer->getImageLayerNames(layers);
        g_renderer->getElevationLayerNames(layers);
        g_renderer->getModelLayerNames(layers);
    } else {
        char *type = Tcl_GetString(objv[3]);
        if (type[0] == 'i' && strcmp(type, "image") == 0) {
            g_renderer->getImageLayerNames(layers);
        } else if (type[0] == 'e' && strcmp(type, "elevation") == 0) {
            g_renderer->getElevationLayerNames(layers);
        } else if (type[0] == 'm' && strcmp(type, "model") == 0) {
            g_renderer->getModelLayerNames(layers);
        } else {
            Tcl_AppendResult(interp, "uknown type \"", type,
                         "\": must be image, elevation or model", (char*)NULL);
            return TCL_ERROR;
        }
    }
    std::ostringstream oss;
    size_t len = 0;
    oss << "nv>map names {";
    len += 18;
    for (size_t i = 0; i < layers.size(); i++) {
        oss << "\"" << layers[i] << "\"";
        len += 2 + layers[i].length();
        if (i < layers.size() - 1) {
            oss << " ";
            len++;
        }
    }
    oss << "}\n";
    len += 2;
#ifdef USE_THREADS
    queueResponse(oss.str().c_str(), len, Response::VOLATILE);
#else 
    ssize_t bytesWritten = SocketWrite(oss.str().c_str(), len);

    if (bytesWritten < 0) {
        return TCL_ERROR;
    }
#endif /*USE_THREADS*/
    return TCL_OK;
}

static int
MapLayerVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool visible;
    if (GetBooleanFromObj(interp, objv[3], &visible) != TCL_OK) {
        return TCL_ERROR;
    }
    char *name = Tcl_GetString(objv[4]);

    g_renderer->setImageLayerVisibility(name, visible);
    g_renderer->setElevationLayerVisibility(name, visible);
    g_renderer->setModelLayerVisibility(name, visible);

    return TCL_OK;
}

static CmdSpec mapLayerOps[] = {
    {"add",     1, MapLayerAddOp,       6, 0, "type driver ?url? ?args? name"},
    {"delete",  1, MapLayerDeleteOp,    3, 4, "?name?"},
    {"move",    1, MapLayerMoveOp,      5, 5, "pos name"},
    {"names",   1, MapLayerNamesOp,     3, 4, "?type?"},
    {"opacity", 1, MapLayerOpacityOp,   5, 5, "opacity ?name?"},
    {"visible", 1, MapLayerVisibleOp,   5, 5, "bool ?name?"},
};
static int nMapLayerOps = NumCmdSpecs(mapLayerOps);

static int
MapLayerOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nMapLayerOps, mapLayerOps,
                        CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
MapLoadOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    char *opt = Tcl_GetString(objv[2]);
    if (opt[0] == 'f' && strcmp(opt, "file") == 0) {
        g_renderer->loadEarthFile(Tcl_GetString(objv[3]));
    } else if (opt[0] == 'u' && strcmp(opt, "url") == 0) {
        std::ostringstream path;
        path << "server:" << Tcl_GetString(objv[3]);
        g_renderer->loadEarthFile(path.str().c_str());
    } else if (opt[0] == 'd' && strcmp(opt, "data") == 0) {
        opt = Tcl_GetString(objv[3]);
        if (opt[0] != 'f' || strcmp(opt, "follows") != 0) {
            return TCL_ERROR;
        }
        int len;
        if (Tcl_GetIntFromObj(interp, objv[4], &len) != TCL_OK) {
            return TCL_ERROR;
        }
        // Read Earth file from socket
        char *buf = (char *)malloc((size_t)len);
        SocketRead(buf, (size_t)len);
        std::ostringstream path;
        path << "/tmp/tmp" << getpid() << ".earth";
        const char *pathStr = path.str().c_str();
        FILE *tmpFile = fopen(pathStr, "w");
        fwrite(buf, len, 1, tmpFile);
        fclose(tmpFile);
        g_renderer->loadEarthFile(pathStr);
        unlink(pathStr);
        free(buf);
    } else {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
MapPositionDisplayOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    Renderer::CoordinateDisplayType type = Renderer::COORDS_LATLONG_DECIMAL_DEGREES;
    if (state && objc > 3) {
        const char *str = Tcl_GetString(objv[3]);
        if (str[0] == 'l' && strcmp(str, "latlong_decimal_degrees") == 0) {
            type = Renderer::COORDS_LATLONG_DECIMAL_DEGREES;
        } else if (str[0] == 'l' && strcmp(str, "latlong_degrees_decimal_minutes") == 0) {
            type = Renderer::COORDS_LATLONG_DEGREES_DECIMAL_MINUTES;
        } else if (str[0] == 'l' && strcmp(str, "latlong_degrees_minutes_seconds") == 0) {
            type = Renderer::COORDS_LATLONG_DEGREES_MINUTES_SECONDS;
        } else if (str[0] == 'm' && strcmp(str, "mgrs") == 0) {
            type = Renderer::COORDS_MGRS;
        } else {
            Tcl_AppendResult(interp, "invalid type: \"", str,
                             "\": should be 'latlong_decimal_degrees', 'latlong_degrees_decimal_minutes', 'latlong_degrees_minutes_seconds', or 'mgrs'",
                             (char*)NULL);
            return TCL_ERROR;
        }
    }
    if (state && objc > 4) {
        int precision;
        if (Tcl_GetIntFromObj(interp, objv[4], &precision) != TCL_OK) {
            return TCL_ERROR;
        }
        g_renderer->setCoordinateReadout(state, type, precision);
    } else {
        g_renderer->setCoordinateReadout(state, type);
    }

    return TCL_OK;
}

static int
MapResetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    char *typeStr = Tcl_GetString(objv[2]);
    osgEarth::MapOptions::CoordinateSystemType type;
    if (typeStr[0] == 'g' && strcmp(typeStr, "geocentric") == 0) {
        type = osgEarth::MapOptions::CSTYPE_GEOCENTRIC;
    } else if (typeStr[0] == 'g' && strcmp(typeStr, "geocentric_cube") == 0) {
        type = osgEarth::MapOptions::CSTYPE_GEOCENTRIC_CUBE;
    } else if (typeStr[0] == 'p' && strcmp(typeStr, "projected") == 0) {
        type = osgEarth::MapOptions::CSTYPE_PROJECTED;
    } else {
        Tcl_AppendResult(interp, "bad map type \"", typeStr,
                         "\": must be geocentric or projected", (char*)NULL);
        return TCL_ERROR;
    }

    if (type == osgEarth::MapOptions::CSTYPE_PROJECTED) {
        if (objc < 4) {
            Tcl_AppendResult(interp, "wrong # arguments: profile required for projected maps", (char*)NULL);
            return TCL_ERROR;
        }
        char *profile = Tcl_GetString(objv[3]);
        if (objc > 4) {
            if (objc < 8) {
                Tcl_AppendResult(interp, "wrong # arguments: 4 bounds arguments required", (char*)NULL);
                return TCL_ERROR;
            }
            double bounds[4];
            for (int i = 0; i < 4; i++) {
                if (Tcl_GetDoubleFromObj(interp, objv[4+i], &bounds[i]) != TCL_OK) {
                    return TCL_ERROR;
                }
            }
            // Check if min > max
            if (bounds[0] > bounds[2] ||
                bounds[1] > bounds[3]) {
                Tcl_AppendResult(interp, "invalid bounds", (char*)NULL);
                return TCL_ERROR;
            }
            // Note: plate-carre generates same SRS as others, but with 
            // _is_plate_carre flag set
            // In map profile, _is_plate_carre is forced on for
            // geographic+projected SRS
            if (strcmp(profile, "geodetic") == 0 ||
                strcmp(profile, "epsg:4326") == 0 ||
                strcmp(profile, "wgs84") == 0 ||
                strcmp(profile, "plate-carre") == 0) {
                if (bounds[0] < -180. || bounds[0] > 180. ||
                    bounds[2] < -180. || bounds[2] > 180. ||
                    bounds[1] < -90. || bounds[1] > 90. ||
                    bounds[3] < -90. || bounds[3] > 90.) {
                    Tcl_AppendResult(interp, "invalid bounds", (char*)NULL);
                    return TCL_ERROR;
                }
            } else if (strcmp(profile, "spherical-mercator") == 0 ||
                       strcmp(profile, "epsg:900913") == 0 ||
                       strcmp(profile, "epsg:3785") == 0 ||
                       strcmp(profile, "epsg:3857") == 0 ||
                       strcmp(profile, "epsg:102113") == 0) {
                for (int i = 0; i < 4; i++) {
                    if (bounds[i] < -20037508.34 || bounds[i] > 20037508.34) {
                        Tcl_AppendResult(interp, "invalid bounds", (char*)NULL);
                        return TCL_ERROR;
                    }
                }
            } else if (strcmp(profile, "epsg:32662") == 0 ||
                       strcmp(profile, "epsg:32663") == 0) {
                if (bounds[0] < -20037508.3428 || bounds[0] > 20037508.3428 ||
                    bounds[2] < -20037508.3428 || bounds[2] > 20037508.3428 ||
                    bounds[1] < -10018754.1714 || bounds[1] > 10018754.1714 ||
                    bounds[3] < -10018754.1714 || bounds[3] > 10018754.1714) {
                    Tcl_AppendResult(interp, "invalid bounds", (char*)NULL);
                    return TCL_ERROR;
                }
            }
            g_renderer->resetMap(type, profile, bounds);
        } else {
            if (osgEarth::Registry::instance()->getNamedProfile(profile) == NULL) {
                Tcl_AppendResult(interp, "bad named profile \"", profile,
                                 "\": must be e.g. 'global-geodetic', 'global-mercator'...", (char*)NULL);
                return TCL_ERROR;
            }
            g_renderer->resetMap(type, profile);
        }
    } else {
        // No profile required for geocentric (3D) maps
        g_renderer->resetMap(type);
    }

    return TCL_OK;
}

static int
MapScaleBarOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setScaleBar(state);
    if (state && objc > 3) {
        const char *unitStr = Tcl_GetString(objv[3]);
        ScaleBarUnits units;
        if (unitStr[0] == 'm' && strcmp(unitStr, "meters") == 0) {
            units = UNITS_METERS;
        } else if (unitStr[0] == 'f' && strcmp(unitStr, "feet") == 0) {
            units = UNITS_INTL_FEET;
        } else if (unitStr[0] == 'u' && strcmp(unitStr, "us_survey_feet") == 0) {
            units = UNITS_US_SURVEY_FEET;
        } else if (unitStr[0] == 'n' && strcmp(unitStr, "nautical_miles") == 0) {
            units = UNITS_NAUTICAL_MILES;
        } else {
            Tcl_AppendResult(interp, "bad units \"", unitStr,
                             "\": must be 'meters', 'feet', 'us_survey_feet' or 'nautical_miles'", (char*)NULL);
            return TCL_ERROR;
        }
        g_renderer->setScaleBarUnits(units);
    }
    return TCL_OK;
}

static int
MapSetPositionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    if (objc < 3) {
        g_renderer->clearReadout();
    } else {
        int x, y;
        if (Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK ||
            Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK) {
            return TCL_ERROR;
        }
        g_renderer->setReadout(x, y);
    }
    return TCL_OK;
}

static int
MapTerrainEdgesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    TRACE("Not implemented");
    //g_renderer->setTerrainEdges(state);
    return TCL_OK;
}

static int
MapTerrainLightingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setTerrainLighting(state);
    return TCL_OK;
}

static int
MapTerrainLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    float color[3];
    if (GetFloatFromObj(interp, objv[3], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[5], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    TRACE("Not implemented");
    //g_renderer->setTerrainLineColor(color);
    return TCL_OK;
}

static int
MapTerrainVertScaleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    double scale;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &scale) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setTerrainVerticalScale(scale);
    return TCL_OK;
}

static int
MapTerrainWireframeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    bool state;
    if (GetBooleanFromObj(interp, objv[3], &state) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->setTerrainWireframe(state);
    return TCL_OK;
}

static CmdSpec mapTerrainOps[] = {
    {"edges",     1, MapTerrainEdgesOp,     4, 4, "bool"},
    {"lighting",  2, MapTerrainLightingOp,  4, 4, "bool"},
    {"linecolor", 2, MapTerrainLineColorOp, 6, 6, "r g b"},
    {"vertscale", 1, MapTerrainVertScaleOp, 4, 4, "val"},
    {"wireframe", 1, MapTerrainWireframeOp, 4, 4, "bool"},
};
static int nMapTerrainOps = NumCmdSpecs(mapTerrainOps);

static int
MapTerrainOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nMapTerrainOps, mapTerrainOps,
                        CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static CmdSpec mapOps[] = {
    {"coords",   1, MapCoordsOp,          4, 6, "x y ?srs? ?verticalDatum?"},
    {"grid",     1, MapGraticuleOp,       3, 4, "bool ?type?"},
    {"layer",    2, MapLayerOp,           3, 0, "op ?params...?"},
    {"load",     2, MapLoadOp,            4, 5, "options"},
    {"posdisp",  1, MapPositionDisplayOp, 3, 5, "bool ?format? ?precision?"},
    {"reset",    1, MapResetOp,           3, 8, "type ?profile xmin ymin xmax ymax?"},
    {"scalebar", 1, MapScaleBarOp,        3, 4, "bool ?units?"},
    {"setpos",   1, MapSetPositionOp,     2, 4, "x y"},
    {"terrain",  1, MapTerrainOp,         3, 0, "op ?params...?"},
};
static int nMapOps = NumCmdSpecs(mapOps);

static int
MapCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nMapOps, mapOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
MouseClickOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    int button;
    double x, y;

    if (Tcl_GetIntFromObj(interp, objv[2], &button) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->mouseClick(button, x, y);
    return TCL_OK;
}

static int
MouseDoubleClickOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    int button;
    double x, y;

    if (Tcl_GetIntFromObj(interp, objv[2], &button) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->mouseDoubleClick(button, x, y);
    return TCL_OK;
}

static int
MouseDragOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    int button;
    double x, y;

    if (Tcl_GetIntFromObj(interp, objv[2], &button) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->mouseDrag(button, x, y);
    return TCL_OK;
}

static int
MouseMotionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    double x, y;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->mouseMotion(x, y);
    return TCL_OK;
}

static int
MouseReleaseOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    int button;
    double x, y;

    if (Tcl_GetIntFromObj(interp, objv[2], &button) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->mouseRelease(button, x, y);
    return TCL_OK;
}

static int
MouseScrollOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    int direction;

    if (Tcl_GetIntFromObj(interp, objv[2], &direction) != TCL_OK) {
        return TCL_ERROR;
    }

    g_renderer->mouseScroll(direction);
    return TCL_OK;
}

static CmdSpec mouseOps[] = {
    {"click",    1, MouseClickOp,       5, 5, "button x y"},
    {"dblclick", 2, MouseDoubleClickOp, 5, 5, "button x y"},
    {"drag",     2, MouseDragOp,        5, 5, "button x y"},
    {"motion",   1, MouseMotionOp,      4, 4, "x y"},
    {"release",  1, MouseReleaseOp,     5, 5, "button x y"},
    {"scroll",   1, MouseScrollOp,      3, 3, "direction"},
};
static int nMouseOps = NumCmdSpecs(mouseOps);

static int
MouseCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nMouseOps, mouseOps,
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static int
RendererRenderOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    g_renderer->eventuallyRender();
    return TCL_OK;
}

static CmdSpec rendererOps[] = {
    {"render",     1, RendererRenderOp, 2, 2, ""},
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
ScreenCoordsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    double x, y, z;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK ||
        Tcl_GetDoubleFromObj(interp, objv[4], &z) != TCL_OK) {
        return TCL_ERROR;
    }
    const osgEarth::SpatialReference *srs = NULL;
    if (objc < 6) {
        srs = g_renderer->getMapSRS();
        if (srs == NULL) {
            Tcl_AppendResult(interp, "Could not determine map SRS", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
        std::string srsInit(Tcl_GetString(objv[5]));
        std::string verticalDatum;
        if (objc > 6) {
            verticalDatum = Tcl_GetString(objv[6]);
        }
        srs = osgEarth::SpatialReference::get(srsInit, verticalDatum);
        if (srs == NULL) {
            Tcl_AppendResult(interp, "bad SRS \"", srsInit.c_str(), "\"", (char*)NULL);
            return TCL_ERROR;
        }
    }
    // ALTMODE_RELATIVE is height above terrain, ALTMODE_ABSOLUTE means
    // relative to the vertical datum
    osgEarth::GeoPoint mapPoint(srs, x, y, z, osgEarth::ALTMODE_ABSOLUTE);
    size_t length;
    char mesg[256];
    osg::Vec3d world;
    osg::Vec3d screen;
    if (g_renderer->getWorldCoords(mapPoint, &world) &&
        g_renderer->worldToScreen(world, &screen)) {
        // send coords to client
        length = snprintf(mesg, sizeof(mesg),
                          "nv>screen coords %g %g %g %g %g %g\n",
                          screen.x(), screen.y(), screen.z(),
                          x, y, z);

        queueResponse(mesg, length, Response::VOLATILE);
    } else {
        // Out of range
        length = snprintf(mesg, sizeof(mesg),
                          "nv>screen coords invalid %g %g %g\n", x, y, z);

        queueResponse(mesg, length, Response::VOLATILE);
    }
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
    {"coords",  1, ScreenCoordsOp, 5, 7, "x y z ?srs? ?verticalDatum?"},
    {"size",    1, ScreenSizeOp, 4, 4, "width height"}
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

#ifdef USE_READ_THREAD
int
GeoVis::queueCommands(Tcl_Interp *interp,
                      ClientData clientData,
                      ReadBuffer *inBufPtr)
{
    Tcl_DString commandString;
    Tcl_DStringInit(&commandString);
    fd_set readFds;

    FD_ZERO(&readFds);
    FD_SET(inBufPtr->file(), &readFds);
    while (inBufPtr->isLineAvailable() || 
           (select(inBufPtr->file()+1, &readFds, NULL, NULL, NULL) > 0)) {
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
        Tcl_DStringAppend(&commandString, (char *)buffer, numBytes);
        if (Tcl_CommandComplete(Tcl_DStringValue(&commandString))) {
            // Add to queue
            Command *command = new Command(Command::COMMAND);
            command->setMessage((unsigned char *)Tcl_DStringValue(&commandString),
                                Tcl_DStringLength(&commandString), Command::VOLATILE);
            g_inQueue->enqueue(command);
            Tcl_DStringSetLength(&commandString, 0);
        }
        FD_SET(inBufPtr->file(), &readFds);
    }

    return 1;
}
#endif

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
                        int fdOut,
                        long timeout)
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
    if (timeout >= 0L) {
        tv.tv_sec = 0L;
        tv.tv_usec = timeout;
        tvPtr = &tv;
    } else {
        TRACE("Blocking on select()");
    }
    while (inBufPtr->isLineAvailable() || 
           (ret = select(inBufPtr->file()+1, &readFds, NULL, NULL, tvPtr)) > 0) {
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
                return 2;               /* This was caused by a "imgflush"
                                         * command. Break out of the read loop
                                         * and allow a new image to be
                                         * rendered. */
            } else { //if (status != TCL_OK) {
                ret = 0;
                if (handleError(interp, clientData, status, fdOut) < 0) {
                    return -1;
                }
            }
            if (status == TCL_OK) {
                ret = 3;
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
            std::string ostr = oss.str();
            nBytes = ostr.length();

            if (queueResponse(ostr.c_str(), nBytes, Response::VOLATILE, Response::ERROR) < 0) {
                return -1;
            }
        }
    }

    std::string msg = getUserMessages();
    nBytes = msg.length();
    if (nBytes > 0) {
        string = msg.c_str();
        TRACE("userError=(%s)", string);

        std::ostringstream oss;
        oss << "nv>viserror -type error -token " << g_stats.nCommands << " -bytes " << nBytes << "\n" << string;
        std::string ostr = oss.str();
        nBytes = ostr.length();

        if (queueResponse(ostr.c_str(), nBytes, Response::VOLATILE, Response::ERROR) < 0) {
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
    Tcl_CreateObjCommand(interp, "colormap",       ColorMapCmd,       clientData, NULL);
    Tcl_CreateObjCommand(interp, "imgflush",       ImageFlushCmd,     clientData, NULL);
    Tcl_CreateObjCommand(interp, "key",            KeyCmd,            clientData, NULL);
    Tcl_CreateObjCommand(interp, "map",            MapCmd,            clientData, NULL);
    Tcl_CreateObjCommand(interp, "mouse",          MouseCmd,          clientData, NULL);
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
    Tcl_DeleteCommand(interp, "colormap");
    Tcl_DeleteCommand(interp, "imgflush");
    Tcl_DeleteCommand(interp, "key");
    Tcl_DeleteCommand(interp, "map");
    Tcl_DeleteCommand(interp, "mouse");
    Tcl_DeleteCommand(interp, "renderer");
    Tcl_DeleteCommand(interp, "screen");

    Tcl_DeleteInterp(interp);
}
