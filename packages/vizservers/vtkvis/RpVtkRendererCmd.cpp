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
AxisCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    if (objc < 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " visible bool\"", (char*)NULL);
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[1]);
    char c = string[0];
    if ((c == 'v') && (strcmp(string, "visible") == 0)) {
        bool visible;

        if (GetBooleanFromObj(interp, objv[2], &visible) != TCL_OK) {
            return TCL_ERROR;
        }

        g_renderer->setAxesVisibility(visible);
    } else {
        Tcl_AppendResult(interp, "bad axis option \"", string,
                         "\": should be visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
CameraCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    if (objc < 6) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " ortho x y width height\"", (char*)NULL);
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[1]);
    char c = string[0];
    if ((c == 'o') && (strcmp(string, "ortho") == 0)) {
        float x, y, width, height;

        if (GetFloatFromObj(interp, objv[2], &x) != TCL_OK ||
            GetFloatFromObj(interp, objv[3], &y) != TCL_OK ||
            GetFloatFromObj(interp, objv[4], &width) != TCL_OK ||
            GetFloatFromObj(interp, objv[5], &height) != TCL_OK) {
            return TCL_ERROR;
        }

        g_renderer->setZoomRegion(x, y, width, height);
    } else {
        Tcl_AppendResult(interp, "bad camera option \"", string,
                         "\": should be ortho", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
ColorMapAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[2]);
    int cmapc;
    Tcl_Obj **cmapv = NULL;

    if (Tcl_ListObjGetElements(interp, objv[3], &cmapc, &cmapv) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((cmapc % 4) != 0) {
        Tcl_AppendResult(interp, "wrong # elements is colormap: should be ",
                         "{ r g b a ... }", (char*)NULL);
        return TCL_ERROR;
    }
    int numEntries = cmapc / 4;
    vtkLookupTable *lut = vtkLookupTable::New();
    lut->Allocate(numEntries, numEntries);
    for (int i = 0; i < cmapc; i += 4) {
        double val[4];
        for (int j = 0; j < 4; j++) {
            if (Tcl_GetDoubleFromObj(interp, cmapv[i+j], &val[j]) != TCL_OK) {
                return TCL_ERROR;
            }
            if ((val[j] < 0.0) || (val[j] > 1.0)) {
                Tcl_AppendResult(interp, "bad colormap value \"",
                                 Tcl_GetString(cmapv[i+j]),
                                 "\": should be in the range [0,1]", (char*)NULL);
                return TCL_ERROR;
            }
        }
        lut->SetTableValue(i/4, val);
    }
    g_renderer->addColorMap(name, lut);
    return TCL_OK;
}

static int
ColorMapDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[2]);
    g_renderer->deleteColorMap(name);
    return TCL_OK;
}

static Rappture::CmdSpec colorMapOps[] = {
    {"add", 1, ColorMapAddOp, 4, 4, "colorMapName map"},
    {"delete", 1, ColorMapDeleteOp, 3, 3, "colorMapName"}
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
    const char *name = Tcl_GetString(objv[4]);
    g_renderer->addContour2D(name);
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

    g_renderer->setContourList(name, contourList);
    return TCL_OK;
}

static int
Contour2DAddNumContoursOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                          Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[4]);
    g_renderer->addContour2D(name);
    int numContours;
    if (Tcl_GetIntFromObj(interp, objv[3], &numContours) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setContours(name, numContours);
    return TCL_OK;
}

static Rappture::CmdSpec contour2dAddOps[] = {
    {"contourlist", 1, Contour2DAddContourListOp, 5, 5, "contourList dataSetName"},
    {"numcontours", 1, Contour2DAddNumContoursOp, 5, 5, "numContours dataSetName"}
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
    const char *name = Tcl_GetString(objv[2]);
    g_renderer->deleteContour2D(name);
    return TCL_OK;
}

static int
Contour2DLineColorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[5]);
    float color[3];
    if (GetFloatFromObj(interp, objv[2], &color[0]) != TCL_OK ||
        GetFloatFromObj(interp, objv[3], &color[1]) != TCL_OK ||
        GetFloatFromObj(interp, objv[4], &color[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setContourEdgeColor(name, color);
    return TCL_OK;
}

static int
Contour2DLineWidthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[3]);
    float width;
    if (GetFloatFromObj(interp, objv[2], &width) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setContourEdgeWidth(name, width);
    return TCL_OK;
}

static int
Contour2DVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[3]);
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setContourVisibility(name, state);
    return TCL_OK;
}

static Rappture::CmdSpec contour2dOps[] = {
    {"add", 1, Contour2DAddOp, 5, 5, "oper value dataSetName"},
    {"delete", 1, Contour2DDeleteOp, 3, 3, "dataSetName"},
    {"linecolor", 5, Contour2DLineColorOp, 6, 6, "r g b dataSetName"},
    {"linewidth", 5, Contour2DLineWidthOp, 4, 4, "width dataSetName"},
    {"visible", 1, Contour2DVisibleOp, 4, 4, "bool dataSetName"},
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
    const char *name = Tcl_GetString(objv[2]);
    g_renderer->deleteDataSet(name);
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
    const char *name = Tcl_GetString(objv[3]);
    double opacity;
    if (Tcl_GetDoubleFromObj(interp, objv[2], &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setOpacity(name, opacity);
    return TCL_OK;
}

static int
DataSetVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[3]);
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setVisibility(name, state);
    return TCL_OK;
}

static Rappture::CmdSpec dataSetOps[] = {
    {"add", 1, DataSetAddOp, 6, 6, "name data follows nBytes"},
    {"delete", 1, DataSetDeleteOp, 3, 3, "name"},
    {"getvalue", 1, DataSetGetValueOp, 6, 7, "oper x y ?z? name"},
    {"opacity", 1, DataSetOpacityOp, 4, 4, "value name"},
    {"visible", 1, DataSetVisibleOp, 4, 4, "bool name"}
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
GridCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    if (objc < 3) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " visible bool\"", (char*)NULL);
        return TCL_ERROR;
    }
    const char *string = Tcl_GetString(objv[1]);
    char c = string[0];
    if ((c == 'v') && (strcmp(string, "visible") == 0)) {
        bool visible;

        if (GetBooleanFromObj(interp, objv[2], &visible) != TCL_OK) {
            return TCL_ERROR;
        }
        Tcl_AppendResult(interp, "Command not yet implemented", (char*)NULL);
        return TCL_ERROR; // TODO: handle command
    } else {
        Tcl_AppendResult(interp, "bad grid option \"", string,
                         "\": should be visible", (char*)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
LegendCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    if (objc < 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " colormapName title width height\"", (char*)NULL);
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

    g_renderer->renderColorMap(name, title, width, height, imgData);

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
    const char *name = Tcl_GetString(objv[2]);
    g_renderer->addPseudoColor(name);
    return TCL_OK;
}

static int
PseudoColorColorMapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                      Tcl_Obj *const *objv)
{
    const char *colorMapName = Tcl_GetString(objv[2]);
    const char *dataSetName = Tcl_GetString(objv[3]);
    g_renderer->setPseudoColorColorMap(dataSetName, colorMapName);
    return TCL_OK;
}

static int
PseudoColorDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[2]);
    g_renderer->deletePseudoColor(name);
    return TCL_OK;
}

static int
PseudoColorVisibleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                     Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[3]);
    bool state;
    if (GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
        return TCL_ERROR;
    }
    g_renderer->setPseudoColorVisibility(name, state);
    return TCL_OK;
}

static Rappture::CmdSpec pseudoColorOps[] = {
    {"add", 1, PseudoColorAddOp, 3, 3, "dataSetName"},
    {"colormap", 1, PseudoColorColorMapOp, 4, 4, "colorMapName dataSetName"},
    {"delete", 1, PseudoColorDeleteOp, 3, 3, "dataSetName"},
    {"visible", 1, PseudoColorVisibleOp, 4, 4, "bool dataSetName"}
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
        iov[0].iov_base = (char *)"NanoVis Server Error: ";
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
    Tcl_CreateObjCommand(interp, "grid",        GridCmd,        NULL, NULL);
    Tcl_CreateObjCommand(interp, "legend",      LegendCmd,      NULL, NULL);
    Tcl_CreateObjCommand(interp, "pseudocolor", PseudoColorCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "screen",      ScreenCmd,      NULL, NULL);
    return interp;
}
