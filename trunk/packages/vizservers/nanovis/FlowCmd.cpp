/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <assert.h>
#define _OPEN_SYS
#include <fcntl.h>
#define _XOPEN_SOURCE_EXTENDED 1
#include <sys/uio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>

#include <sstream>

#include <tcl.h>

#include <RpBuffer.h>

#include <vrmath/Vector3f.h>

#include "nvconf.h"

#include "nanovisServer.h"
#include "nanovis.h"
#include "CmdProc.h"
#include "Command.h"
#include "PPMWriter.h"
#ifdef USE_VTK
#include "VtkDataSetReader.h"
#else
#include "VtkReader.h"
#endif
#include "FlowCmd.h"
#include "FlowTypes.h"
#include "Flow.h"
#include "FlowBox.h"
#include "FlowParticles.h"
#include "Switch.h"
#include "TransferFunction.h"
#include "LIC.h"
#include "Unirect.h"
#include "VelocityArrowsSlice.h"
#include "Volume.h"
#include "Trace.h"

using namespace nv;
using namespace vrmath;

static SwitchParseProc AxisSwitchProc;
static SwitchCustom axisSwitch = {
    AxisSwitchProc, NULL, 0,
};

static SwitchParseProc ColorSwitchProc;
static SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, 0,
};

static SwitchParseProc PointSwitchProc;
static SwitchCustom pointSwitch = {
    PointSwitchProc, NULL, 0,
};

static SwitchParseProc PositionSwitchProc;
static SwitchCustom positionSwitch = {
    PositionSwitchProc, NULL, 0,
};

static SwitchParseProc TransferFunctionSwitchProc;
static SwitchCustom transferFunctionSwitch = {
    TransferFunctionSwitchProc, NULL, 0,
};

SwitchSpec Flow::_switches[] = {
    {SWITCH_FLOAT, "-ambient", "value",
     offsetof(FlowValues, ambient), 0},
    {SWITCH_BOOLEAN, "-arrows", "boolean",
     offsetof(FlowValues, showArrows), 0},
    {SWITCH_CUSTOM, "-axis", "axis",
     offsetof(FlowValues, slicePos.axis), 0, 0, &axisSwitch},
    {SWITCH_FLOAT, "-diffuse", "value",
     offsetof(FlowValues, diffuse), 0},
    {SWITCH_BOOLEAN, "-hide", "boolean",
     offsetof(FlowValues, isHidden), 0},
    {SWITCH_BOOLEAN, "-light2side", "boolean",
     offsetof(FlowValues, twoSidedLighting), 0},
    {SWITCH_FLOAT, "-opacity", "value",
     offsetof(FlowValues, opacity), 0},
    {SWITCH_BOOLEAN, "-outline", "boolean",
     offsetof(FlowValues, showOutline), 0},
    {SWITCH_CUSTOM, "-position", "number",
     offsetof(FlowValues, slicePos), 0, 0, &positionSwitch},
    {SWITCH_BOOLEAN, "-slice", "boolean",
     offsetof(FlowValues, sliceVisible), 0},
    {SWITCH_FLOAT, "-specularExp", "value",
     offsetof(FlowValues, specularExp), 0},
    {SWITCH_FLOAT, "-specularLevel", "value",
     offsetof(FlowValues, specular), 0},
    {SWITCH_CUSTOM, "-transferfunction", "name",
     offsetof(FlowValues, transferFunction), 0, 0, &transferFunctionSwitch},
    {SWITCH_BOOLEAN, "-volume", "boolean",
     offsetof(FlowValues, showVolume), 0},
    {SWITCH_END}
};

SwitchSpec FlowParticles::_switches[] = {
    {SWITCH_CUSTOM, "-axis", "axis",
     offsetof(FlowParticlesValues, position.axis), 0, 0, &axisSwitch},
    {SWITCH_CUSTOM, "-color", "{r g b a}",
     offsetof(FlowParticlesValues, color), 0, 0,  &colorSwitch},
    {SWITCH_BOOLEAN, "-hide", "boolean",
     offsetof(FlowParticlesValues, isHidden), 0},
    {SWITCH_CUSTOM, "-position", "number",
     offsetof(FlowParticlesValues, position), 0, 0, &positionSwitch},
    {SWITCH_FLOAT, "-size", "float",
     offsetof(FlowParticlesValues, particleSize), 0},
    {SWITCH_END}
};

SwitchSpec FlowBox::_switches[] = {
    {SWITCH_CUSTOM, "-color", "{r g b a}",
     offsetof(FlowBoxValues, color), 0, 0,  &colorSwitch},
    {SWITCH_CUSTOM, "-corner1", "{x y z}",
     offsetof(FlowBoxValues, corner1), 0, 0, &pointSwitch},
    {SWITCH_CUSTOM, "-corner2", "{x y z}",
     offsetof(FlowBoxValues, corner2), 0, 0, &pointSwitch},
    {SWITCH_BOOLEAN, "-hide", "boolean",
     offsetof(FlowBoxValues, isHidden), 0},
    {SWITCH_FLOAT, "-linewidth", "number",
     offsetof(FlowBoxValues, lineWidth), 0},
    {SWITCH_END}
};

/**
 * $flow data follows nbytes nComponents
 */
static int
FlowDataFollowsOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    TRACE("Enter");

    int nBytes;
    if (Tcl_GetIntFromObj(interp, objv[3], &nBytes) != TCL_OK) {
        ERROR("Bad nBytes \"%s\"", Tcl_GetString(objv[3]));
        return TCL_ERROR;
    }
    if (nBytes <= 0) {
        Tcl_AppendResult(interp, "bad # bytes request \"", 
                         Tcl_GetString(objv[3]), "\" for \"data follows\"", (char *)NULL);
        ERROR("Bad nbytes %d", nBytes);
        return TCL_ERROR;
    }
    int nComponents;
    if (Tcl_GetIntFromObj(interp, objv[4], &nComponents) != TCL_OK) {
        ERROR("Bad # of components \"%s\"", Tcl_GetString(objv[4]));
        return TCL_ERROR;
    }
    if (nComponents <= 0) {
        Tcl_AppendResult(interp, "bad # of components request \"", 
                         Tcl_GetString(objv[4]), "\" for \"data follows\"", (char *)NULL);
        ERROR("Bad # of components %d", nComponents);
        return TCL_ERROR;
    }
    Rappture::Buffer buf(nBytes);
    TRACE("Flow data loading bytes: %d components: %d", nBytes, nComponents);
    if (GetDataStream(interp, buf, nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    char *bytes = (char *)buf.bytes();
    size_t length = buf.size();

    Unirect3d *unirect = NULL;
    Volume *volume = NULL;

    Flow *flow = (Flow *)clientData;
    if ((length > 4) && (strncmp(bytes, "<DX>", 4) == 0)) {
#ifdef USE_DX_READER
        unirect = new Unirect3d(nComponents);
        if (!unirect->importDx(nComponents, length - 4, bytes + 4)) {
            Tcl_AppendResult(interp, "Failed to load DX file", (char *)NULL);
            delete unirect;
            return TCL_ERROR;
        }
#else
        Tcl_AppendResult(interp, "Loading DX files is not supported by this server", (char*)NULL);
        return TCL_ERROR;
#endif
    } else if ((length > 10) && (strncmp(bytes, "unirect3d ", 10) == 0)) {
        unirect = new Unirect3d(nComponents);
        if (unirect->parseBuffer(interp, bytes, length) != TCL_OK) {
            delete unirect;
            return TCL_ERROR;
        }
    } else if ((length > 10) && (strncmp(bytes, "unirect2d ", 10) == 0)) {
        unirect = new Unirect3d(nComponents);
        Unirect2d *u2dPtr;
        u2dPtr = new Unirect2d(nComponents);
        if (u2dPtr->parseBuffer(interp, bytes, length) != TCL_OK) {
            delete unirect;
            delete u2dPtr;
            return TCL_ERROR;
        }
        unirect->convert(u2dPtr);
        delete u2dPtr;
    } else if ((length > 14) && (strncmp(bytes, "# vtk DataFile", 14) == 0)) {
        TRACE("VTK loading...");
        if (length <= 0) {
            ERROR("data buffer is empty");
            abort();
        }
#ifdef USE_VTK
        volume = load_vtk_volume_stream(flow->name(), bytes, length);
#else
        std::stringstream fdata;
        fdata.write(bytes, length);
        volume = load_vtk_volume_stream(flow->name(), fdata);
#endif
        if (volume == NULL) {
            Tcl_AppendResult(interp, "Failed to load VTK file", (char*)NULL);
            return TCL_ERROR;
        }
    } else {
#ifdef USE_DX_READER
        TRACE("header is %.14s", buf.bytes());
        unirect = new Unirect3d(nComponents);
        if (!unirect->importDx(nComponents, length, bytes)) {
            Tcl_AppendResult(interp, "Failed to load DX file", (char *)NULL);
            delete unirect;
            return TCL_ERROR;
        }
#else
        Tcl_AppendResult(interp, "Loading DX files is not supported by this server", (char*)NULL);
        return TCL_ERROR;
#endif
    }
    if (unirect != NULL && unirect->nValues() == 0) {
        delete unirect;
        Tcl_AppendResult(interp, "no data found in stream", (char *)NULL);
        return TCL_ERROR;
    }
    if (unirect != NULL) {
        TRACE("nx = %d ny = %d nz = %d",
              unirect->xNum(), unirect->yNum(), unirect->zNum());
        TRACE("x0 = %lg y0 = %lg z0 = %lg",
              unirect->xMin(), unirect->yMin(), unirect->zMin());
        TRACE("lx = %lg ly = %lg lz = %lg",
              unirect->xMax() - unirect->xMin(),
              unirect->yMax() - unirect->yMin(),
              unirect->zMax() - unirect->zMin());
        TRACE("dx = %lg dy = %lg dz = %lg",
              unirect->xNum() > 1 ? (unirect->xMax() - unirect->xMin())/(unirect->xNum()-1) : 0,
              unirect->yNum() > 1 ? (unirect->yMax() - unirect->yMin())/(unirect->yNum()-1) : 0,
              unirect->zNum() > 1 ? (unirect->zMax() - unirect->zMin())/(unirect->zNum()-1) : 0);
        TRACE("magMin = %lg magMax = %lg",
              unirect->magMin(), unirect->magMax());

        flow->data(unirect);
    } else {
        flow->data(volume);
    }
    double range[2];
    flow->getVectorRange(range);
    {
        char info[1024];
        int length = 
            sprintf(info, "nv>data tag %s min %g max %g\n",
                    flow->name(), range[0], range[1]);
#ifdef USE_THREADS
        queueResponse(info, (size_t)length, Response::VOLATILE);
#else
        if (SocketWrite(info, (size_t)length) < 0) {
            return TCL_ERROR;
        }
#endif
    }
    Flow::updatePending = true;
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static CmdSpec flowDataOps[] = {
    {"follows", 2, FlowDataFollowsOp, 5, 5, "size nComponents",},
};
static int nFlowDataOps = NumCmdSpecs(flowDataOps);

static int
FlowDataOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nFlowDataOps, flowDataOps,
                        CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/**
 * \brief Convert a Tcl_Obj representing the label of a child node into its
 * integer node id.
 *
 * \param clientData Flag indicating if the node is considered before or 
 * after the insertion position.
 * \param interp Interpreter to send results back to
 * \param switchName Not used
 * \param objPtr String representation
 * \param record Structure record
 * \param offset Not used
 * \param flags Not used
 *
 * \return The return value is a standard Tcl result.
 */
static int
AxisSwitchProc(ClientData clientData, Tcl_Interp *interp,
               const char *switchName, Tcl_Obj *objPtr,
               char *record, int offset, int flags)
{
    const char *string = Tcl_GetString(objPtr);
    if (string[1] == '\0') {
        FlowSliceAxis *axisPtr = (FlowSliceAxis *)(record + offset);
        char c;
        c = tolower((unsigned char)string[0]);
        if (c == 'x') {
            *axisPtr = AXIS_X;
            return TCL_OK;
        } else if (c == 'y') {
            *axisPtr = AXIS_Y;
            return TCL_OK;
        } else if (c == 'z') {
            *axisPtr = AXIS_Z;
            return TCL_OK;
        }
        /*FALLTHRU*/
    }
    Tcl_AppendResult(interp, "bad axis \"", string,
                     "\": should be x, y, or z", (char*)NULL);
    return TCL_ERROR;
}

/**
 * \brief Convert a Tcl_Obj representing the label of a list of four color 
 * components in to a RGBA color value.
 *
 * \param clientData Flag indicating if the node is considered before or 
 * after the insertion position.
 * \param interp Interpreter to send results back to
 * \param switchName Not used
 * \param objPtr String representation
 * \param record Structure record
 * \param offset Not used
 * \param flags Not used
 * 
 * \return The return value is a standard Tcl result.
 */
static int
ColorSwitchProc(ClientData clientData, Tcl_Interp *interp,
                const char *switchName, Tcl_Obj *objPtr,
                char *record, int offset, int flags)
{
    Tcl_Obj **objv;
    int objc;
    FlowColor *color = (FlowColor *)(record + offset);

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc < 3) || (objc > 4)) {
        Tcl_AppendResult(interp, "wrong # of elements in color definition",
                         (char *)NULL);
        return TCL_ERROR;
    }
    float values[4];
    int i;
    values[3] = 1.0f;
    for (i = 0; i < objc; i++) {
        float value;

        if (GetFloatFromObj(interp, objv[i], &value) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((value < 0.0) || (value > 1.0)) {
            Tcl_AppendResult(interp, "bad component value in \"",
                             Tcl_GetString(objPtr), "\": color values must be [0..1]",
                             (char *)NULL);
            return TCL_ERROR;
        }
        values[i] = value;
    }
    color->r = values[0];
    color->g = values[1];
    color->b = values[2];
    color->a = values[3];
    return TCL_OK;
}

/**
 * \brief Convert a Tcl_Obj representing the a 3-D coordinate into a point.
 *
 * \param clientData Flag indicating if the node is considered before or 
 * after the insertion position.
 * \param interp Interpreter to send results back to
 * \param switchName Not used
 * \param objPtr String representation
 * \param record Structure record
 * \param offset Not used
 * \param flags Not used
 *
 * \return The return value is a standard Tcl result.
 */
static int
PointSwitchProc(ClientData clientData, Tcl_Interp *interp,
                const char *switchName, Tcl_Obj *objPtr,
                char *record, int offset, int flags)
{
    FlowPoint *point = (FlowPoint *)(record + offset);
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 3) {
        Tcl_AppendResult(interp, "wrong # of elements for box coordinates: "
                         " should be \"x y z\"", (char *)NULL);
        return TCL_ERROR;
    }
    float values[3];
    int i;
    for (i = 0; i < objc; i++) {
        float value;

        if (GetFloatFromObj(interp, objv[i], &value) != TCL_OK) {
            return TCL_ERROR;
        }
        values[i] = value;
    }
    point->x = values[0];
    point->y = values[1];
    point->z = values[2];
    return TCL_OK;
}

/**
 * \brief Convert a Tcl_Obj representing the a 3-D coordinate into a point.
 *
 * \param clientData Flag indicating if the node is considered before or 
 * after the insertion position.
 * \param interp Interpreter to send results back to
 * \param switchName Not used
 * \param objPtr String representation
 * \param record Structure record
 * \param offset Not used
 * \param flags Not used
 *
 * \return The return value is a standard Tcl result.
 */
static int
PositionSwitchProc(ClientData clientData, Tcl_Interp *interp,
                   const char *switchName, Tcl_Obj *objPtr,
                   char *record, int offset, int flags)
{
    FlowPosition *posPtr = (FlowPosition *)(record + offset);
    const char *string;
    char *p;

    string = Tcl_GetString(objPtr);
    p = strrchr((char *)string, '%');
    if (p == NULL) {
        float value;

        if (GetFloatFromObj(interp, objPtr, &value) != TCL_OK) {
            return TCL_ERROR;
        }
        posPtr->value = value;
        posPtr->flags = ABSPOS;
    } else {
        double value;

        *p = '\0';
        if (Tcl_GetDouble(interp, string, &value) != TCL_OK) {
            return TCL_ERROR;
        }
        posPtr->value = (float)value * 0.01;
        posPtr->flags = RELPOS;
    }
    return TCL_OK;
}

/**
 * \brief Convert a Tcl_Obj representing the transfer function into a
 *  TransferFunction pointer.  
 *
 * The transfer function must have been previously defined.
 *
 * \param clientData Flag indicating if the node is considered before or 
 * after the insertion position.
 * \param interp Interpreter to send results back to
 * \param switchName Not used
 * \param objPtr String representation
 * \param record Structure record
 * \param offset Not used
 * \param flags Not used
 *
 * \return The return value is a standard Tcl result.
 */
static int
TransferFunctionSwitchProc(ClientData clientData, Tcl_Interp *interp,
                           const char *switchName, Tcl_Obj *objPtr,
                           char *record, int offset, int flags)
{
    TransferFunction **funcPtrPtr = (TransferFunction **)(record + offset);
    TransferFunction *tf = NanoVis::getTransferFunction(Tcl_GetString(objPtr));
    if (tf == NULL) {
        Tcl_AppendResult(interp, "transfer function \"", Tcl_GetString(objPtr),
                         "\" is not defined", (char*)NULL);
        return TCL_ERROR;
    }
    *funcPtrPtr = tf;
    return TCL_OK;
}

static int
FlowConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;

    if (flow->parseSwitches(interp, objc - 2, objv + 2) != TCL_OK) {
        return TCL_ERROR;
    }
    if (flow->configure()) {
        Flow::updatePending = true;
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowParticlesAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    const char *particlesName = Tcl_GetString(objv[3]);
    FlowParticles *particles = flow->createParticles(particlesName);
    if (particles == NULL) {
        Tcl_AppendResult(interp, "Flow particle injection plane \"",
                         particlesName,
                         "\" already exists or could not be created",
                         (char*)NULL);
        return TCL_ERROR;
    }
    if (particles->parseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
        flow->deleteParticles(particlesName);
        return TCL_ERROR;
    }
    particles->configure();
    Flow::updatePending = true;
    NanoVis::eventuallyRedraw();
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

static int
FlowParticlesConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    const char *particlesName = Tcl_GetString(objv[3]);
    FlowParticles *particles = flow->getParticles(particlesName);
    if (particles == NULL) {
        Tcl_AppendResult(interp, "Flow particle injection plane \"",
                         particlesName, "\" not found",
                         (char*)NULL);
        return TCL_ERROR;
    }
    if (particles->parseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
        return TCL_ERROR;
    }
    if (particles->configure()) {
        Flow::updatePending = true;
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowParticlesDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    for (int i = 3; i < objc; i++) {
        flow->deleteParticles(Tcl_GetString(objv[i]));
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowParticlesNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    std::vector<std::string> names;
    flow->getParticlesNames(names);
    for (std::vector<std::string>::iterator itr = names.begin();
         itr != names.end(); ++itr) {
        Tcl_Obj *objPtr = Tcl_NewStringObj(itr->c_str(), -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static CmdSpec flowParticlesOps[] = {
    {"add",        1, FlowParticlesAddOp,        4, 0, "name ?switches?",},
    {"configure",  1, FlowParticlesConfigureOp,  4, 0, "name ?switches?",},
    {"delete",     1, FlowParticlesDeleteOp,     4, 0, "name ?name...?"},
    {"names",      1, FlowParticlesNamesOp,      3, 3, ""},
};

static int nFlowParticlesOps = NumCmdSpecs(flowParticlesOps);

/**
 * \brief This procedure is invoked to process commands on behalf of the flow
 * object.
 * 
 * Side effects: See the user documentation.
 * 
 * $flow particles oper $name 
 *
 * \return A standard Tcl result.
 */
static int
FlowParticlesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = GetOpFromObj(interp, nFlowParticlesOps, flowParticlesOps, 
                        CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Flow *flow = (Flow *)clientData;
    Tcl_Preserve(flow);
    int result;
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flow);
    return result;
}

static int
FlowBoxAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    const char *boxName = Tcl_GetString(objv[3]);
    FlowBox *box = flow->createBox(boxName);
    if (box == NULL) {
        Tcl_AppendResult(interp, "Flow box \"", boxName, 
                         "\" already exists or could not be created",
                         (char*)NULL);
        return TCL_ERROR;
    }
    if (box->parseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
        flow->deleteBox(boxName);
        return TCL_ERROR;
    }
    NanoVis::eventuallyRedraw();
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

static int
FlowBoxConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    const char *boxName = Tcl_GetString(objv[3]);
    FlowBox *box = flow->getBox(boxName);
    if (box == NULL) {
        Tcl_AppendResult(interp, "Flow box \"", boxName, "\" not found",
                         (char*)NULL);
        return TCL_ERROR;
    }
    if (box->parseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowBoxDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    for (int i = 3; i < objc; i++) {
        flow->deleteBox(Tcl_GetString(objv[i]));
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowBoxNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    Tcl_Obj *listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    std::vector<std::string> names;
    flow->getBoxNames(names);
    for (std::vector<std::string>::iterator itr = names.begin();
         itr != names.end(); ++itr) {
        Tcl_Obj *objPtr = Tcl_NewStringObj(itr->c_str(), -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static CmdSpec flowBoxOps[] = {
    {"add",        1, FlowBoxAddOp,        4, 0, "name ?switches?",},
    {"configure",  1, FlowBoxConfigureOp,  4, 0, "name ?switches?",},
    {"delete",     1, FlowBoxDeleteOp,     4, 0, "name ?name...?"},
    {"names",      1, FlowBoxNamesOp,      3, 3, ""},
};

static int nFlowBoxOps = NumCmdSpecs(flowBoxOps);

/**
 * \brief This procedure is invoked to process commands on behalf of the flow
 *         object.
 *
 * Side effects:  See the user documentation.
 *
 * \return A standard Tcl result.
 */
static int
FlowBoxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = GetOpFromObj(interp, nFlowBoxOps, flowBoxOps, 
                        CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Flow *flow = (Flow *)clientData;
    Tcl_Preserve(flow);
    int result;
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flow);
    return result;
}

/*
 * CLIENT COMMAND:
 *   $flow legend <width> <height>
 *
 * Clients use this to generate a legend image for the specified
 * transfer function.  The legend image is a color gradient from 0
 * to one, drawn in the given transfer function.  The resulting image
 * is returned in the size <width> x <height>.
 */
static int
FlowLegendOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Flow *flow = (Flow *)clientData;
    
    const char *string = Tcl_GetString(objv[1]);
    TransferFunction *tf;
    tf = flow->getTransferFunction();
    if (tf == NULL) {
        Tcl_AppendResult(interp, "unknown transfer function \"", string, "\"",
                         (char*)NULL);
        return TCL_ERROR;
    }
    const char *label;
    label = Tcl_GetString(objv[0]);
    int w, h;
    if ((Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (Flow::updatePending) {
        NanoVis::setFlowRanges();
    }
    NanoVis::renderLegend(tf, Flow::magMin, Flow::magMax, w, h, label);
    return TCL_OK;
}

static CmdSpec flowInstOps[] = {
    {"box",         1, FlowBoxOp,        2, 0, "oper ?args?"},
    {"configure",   1, FlowConfigureOp,  2, 0, "?switches?"},
    {"data",        1, FlowDataOp,       2, 0, "oper ?args?"},
    {"legend",      1, FlowLegendOp,     4, 4, "w h"},
    {"particles",   1, FlowParticlesOp,  2, 0, "oper ?args?"}
};
static int nFlowInstOps = NumCmdSpecs(flowInstOps);

/**
 * \brief This procedure is invoked to process commands on behalf of the flow
 * object.
 *
 * Side effects: See the user documentation.
 *
 * \return A standard Tcl result.
 */
int
nv::FlowInstObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = GetOpFromObj(interp, nFlowInstOps, flowInstOps, 
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    assert(CheckGL(AT));
    Flow *flow = (Flow *)clientData;
    Tcl_Preserve(flow);
    int result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flow);
    return result;
}

/**
 * \brief Deletes the command associated with the flow
 *
 * This is called only when the command associated with the flow is destroyed.
 */
void
nv::FlowInstDeleteProc(ClientData clientData)
{
    Flow *flow = (Flow *)clientData;
    NanoVis::deleteFlow(flow->name());
}

static int
FlowAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    const char *name = Tcl_GetString(objv[2]);
    Tcl_CmdInfo cmdInfo;
    if (Tcl_GetCommandInfo(interp, name, &cmdInfo)) {
        Tcl_AppendResult(interp, "an another command \"", name, 
                         "\" already exists.", (char *)NULL);
        return TCL_ERROR;
    }
    Flow *flow = NanoVis::createFlow(interp, name);
    if (flow == NULL) {
        Tcl_AppendResult(interp, "Flow \"", name, "\" already exists",
                         (char*)NULL);
        return TCL_ERROR;
    }
    if (flow->parseSwitches(interp, objc - 3, objv + 3) != TCL_OK) {
        Tcl_DeleteCommand(interp, flow->name());
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[2]);
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    for (int i = 2; i < objc; i++) {
        Flow *flow = NanoVis::getFlow(Tcl_GetString(objv[i]));
        if (flow != NULL) {
            Tcl_DeleteCommandFromToken(interp, flow->getCommandToken());
        }
    }
    Flow::updatePending = true;
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    bool value = false;
    Flow *flow = NanoVis::getFlow(Tcl_GetString(objv[2]));
    if (flow != NULL) {
        value = true;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (int)value);
    return TCL_OK;
}

/**
 * \brief flow goto number
 */
static int
FlowGotoOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    int nSteps;
    if (Tcl_GetIntFromObj(interp, objv[2], &nSteps) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((nSteps < 0) || (nSteps > SHRT_MAX)) {
        Tcl_AppendResult(interp, "flow goto: bad # of steps \"",
                         Tcl_GetString(objv[2]), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    NanoVis::resetFlows();
    if (Flow::updatePending) {
        NanoVis::setFlowRanges();
    }
    for (int i = 0; i < nSteps; i++) {
        NanoVis::licRenderer->convolve();
        NanoVis::advectFlows();
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (NanoVis::FlowHashmap::iterator itr = NanoVis::flowTable.begin();
         itr != NanoVis::flowTable.end(); ++itr) {
        Flow *flow = itr->second;
        Tcl_Obj *objPtr = Tcl_NewStringObj(flow->name(), -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
FlowNextOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    assert(NanoVis::licRenderer != NULL);
    if (Flow::updatePending) {
        NanoVis::setFlowRanges();
    }
    NanoVis::licRenderer->convolve();
    NanoVis::advectFlows();
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowResetOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    NanoVis::resetFlows();
    return TCL_OK;
}

#ifdef HAVE_FFMPEG

/**
 * \brief Convert a Tcl_Obj representing the video format into its
 * integer id.
 *
 * \param clientData Not used
 * \param interp Interpreter to send results back to
 * \param switchName Not used
 * \param objPtr String representation
 * \param record Structure record
 * \param offset Not used
 * \param flags Not used
 *
 * \return The return value is a standard Tcl result.
 */
static int
VideoFormatSwitchProc(ClientData clientData, Tcl_Interp *interp,
                      const char *switchName, Tcl_Obj *objPtr,
                      char *record, int offset, int flags)
{
    Tcl_Obj **formatObjPtr = (Tcl_Obj **)(record + offset);
    Tcl_Obj *fmtObjPtr;
    const char *string;
    char c; 
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'm') && (length > 1) && 
        (strncmp(string, "mpeg", length) == 0)) {
        fmtObjPtr =  Tcl_NewStringObj("mpeg1video", 10);
    } else if ((c == 't') && (strncmp(string, "theora", length) == 0)) {
        fmtObjPtr =  Tcl_NewStringObj("theora", 6);
    } else if ((c == 'm') && (length > 1) &&
               (strncmp(string, "mov", length) == 0)) {
        fmtObjPtr =  Tcl_NewStringObj("mov", 3);
    } else {
        Tcl_AppendResult(interp, "bad video format \"", string,
                         "\": should be mpeg, theora, or mov", (char*)NULL);
        return TCL_ERROR;
    }
    if (*formatObjPtr != NULL) {
        Tcl_DecrRefCount(*formatObjPtr);
    }
    Tcl_IncrRefCount(fmtObjPtr);
    *formatObjPtr = fmtObjPtr;
    return TCL_OK;
}

struct FlowVideoSwitches {
    float frameRate;         /**< Frame rate */
    int bitRate;             /**< Video bitrate */
    int width, height;       /**< Dimensions of video frame. */
    int numFrames;
    Tcl_Obj *formatObjPtr;
};

static SwitchParseProc VideoFormatSwitchProc;
static SwitchCustom videoFormatSwitch = {
    VideoFormatSwitchProc, NULL, 0,
};

static SwitchSpec flowVideoSwitches[] = {
    {SWITCH_INT, "-bitrate", "value",
     offsetof(FlowVideoSwitches, bitRate), 0},
    {SWITCH_CUSTOM, "-format", "string",
     offsetof(FlowVideoSwitches, formatObjPtr), 0, 0, &videoFormatSwitch},
    {SWITCH_FLOAT, "-framerate", "value",
     offsetof(FlowVideoSwitches, frameRate), 0},
    {SWITCH_INT, "-height", "integer",
     offsetof(FlowVideoSwitches, height), 0},
    {SWITCH_INT, "-numframes", "count",
     offsetof(FlowVideoSwitches, numFrames), 0},
    {SWITCH_INT, "-width", "integer",
     offsetof(FlowVideoSwitches, width), 0},
    {SWITCH_END}
};

static bool
MakeImageFiles(char *tmpFileName,
               int width, int height, int numFrames,
               bool *cancelPtr)
{
    struct pollfd pollResults;
    pollResults.fd = nv::g_fdIn;
    pollResults.events = POLLIN;
#define PENDING_TIMEOUT          10  /* milliseconds. */
    int timeout = PENDING_TIMEOUT;

    int oldWidth, oldHeight;
    oldWidth = NanoVis::winWidth;
    oldHeight = NanoVis::winHeight;

    if (width != oldWidth ||
        height != oldHeight) {
        // Resize to the requested size.
        NanoVis::resizeOffscreenBuffer(width, height);
    }
    NanoVis::resetFlows();
    *cancelPtr = false;
    bool result = true;
    size_t length = strlen(tmpFileName);
    for (int i = 1; i <= numFrames; i++) {
        if (((i & 0xF) == 0) && (poll(&pollResults, 1, timeout) > 0)) {
            /* If there's another command on stdin, that means the client is
             * trying to cancel this operation. */
            *cancelPtr = true;
            break;
        }

        NanoVis::licRenderer->convolve();
        NanoVis::advectFlows();

        int fboOrig;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

        NanoVis::bindOffscreenBuffer();
        NanoVis::render();
        NanoVis::readScreen();

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

        sprintf(tmpFileName + length, "/image%d.ppm", i);
        result = nv::writePPMFile(tmpFileName, NanoVis::screenBuffer,
                                  width, height);
        if (!result) {
            break;
        }
    }
    if (width != oldWidth || 
        height != oldHeight) {
        NanoVis::resizeOffscreenBuffer(oldWidth, oldHeight);
    }
    tmpFileName[length] = '\0';
    NanoVis::resetFlows();
    return result;
}

static int
MakeMovie(Tcl_Interp *interp, char *tmpFileName, const char *token,
          FlowVideoSwitches *switchesPtr)
{
#ifndef FFMPEG 
#  define FFMPEG "/usr/bin/ffmpeg"
#endif
    /* Generate the movie from the frame images by exec-ing ffmpeg */
    /* The ffmpeg command is
     *   ffmpeg -f image2 -i /var/tmp/xxxxx/image%d.ppm                 \
     *      -b bitrate -f framerate /var/tmp/xxxxx/movie.mpeg 
     */
    char cmd[BUFSIZ];
    sprintf(cmd, "%s -f image2 -i %s/image%%d.ppm -f %s -b %d -r %f -",
            FFMPEG, tmpFileName, Tcl_GetString(switchesPtr->formatObjPtr), 
            switchesPtr->bitRate, switchesPtr->frameRate);
    TRACE("Enter: %s", cmd);
    FILE *f;
    f = popen(cmd, "r");
    if (f == NULL) {
        Tcl_AppendResult(interp, "can't run ffmpeg: ", 
                         Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    char *data = NULL;
    size_t total = 0;
    for (;;) {
        ssize_t numRead;
        char buffer[BUFSIZ]; 
        
        numRead = fread(buffer, sizeof(char), BUFSIZ, f);
        total += numRead;
        if (numRead == 0) {             // EOF
            break;
        }
        if (numRead < 0) {              // Error
            ERROR("Can't read movie data: %s",
                  Tcl_PosixError(interp));
            Tcl_AppendResult(interp, "can't read movie data: ", 
                Tcl_PosixError(interp), (char *)NULL);
            return TCL_ERROR;
        }
        data = (char *)realloc(data, total);
        if (data == NULL) {
            ERROR("Can't append movie data to buffer %d bytes",
                  numRead);
            Tcl_AppendResult(interp, "can't append movie data to buffer",
                             (char *)NULL);
            return TCL_ERROR;
        }
        memcpy(data + (total - numRead), buffer, numRead);
    }
    if (total == 0) {
        ERROR("ffmpeg returned 0 bytes");
    }
    // Send zero length to client so it can deal with error
    sprintf(cmd,"nv>image -type movie -token \"%s\" -bytes %lu\n", 
            token, total);
    // Memory is freed by this call
    nv::sendDataToClient(cmd, data, total);
    return TCL_OK;
}

static int
FlowVideoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    FlowVideoSwitches switches;
    const char *token;

    token = Tcl_GetString(objv[2]);
    switches.frameRate = 25.0f;                // Default frame rate 25 fps
    switches.bitRate = 6000000;                // Default video bit rate.
    switches.width = NanoVis::winWidth;
    switches.height = NanoVis::winHeight;
    switches.numFrames = 100;
    switches.formatObjPtr = Tcl_NewStringObj("mpeg1video", 10);
    Tcl_IncrRefCount(switches.formatObjPtr);
    if (ParseSwitches(interp, flowVideoSwitches, 
                objc - 3, objv + 3, &switches, SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((switches.width < 0) || (switches.width > SHRT_MAX) || 
        (switches.height < 0) || (switches.height > SHRT_MAX)) {
        Tcl_AppendResult(interp, "bad dimensions for video", (char *)NULL);
        return TCL_ERROR;
    }
    if ((switches.frameRate < 0.0f) || (switches.frameRate > 30.0f)) {
        Tcl_AppendResult(interp, "bad frame rate.", (char *)NULL);
        return TCL_ERROR;
    }
    if (switches.bitRate < 0) {
        Tcl_AppendResult(interp, "bad bit rate.", (char *)NULL);
        return TCL_ERROR;
    }
    if (NanoVis::licRenderer == NULL) {
        Tcl_AppendResult(interp, "no lic renderer.", (char *)NULL);
        return TCL_ERROR;
    }
    TRACE("FLOW started");

    char *tmpFileName;
    char nameTemplate[200];
    strcpy(nameTemplate,"/var/tmp/flowXXXXXX");
    tmpFileName = mkdtemp(nameTemplate);
    int result = TCL_OK;
    if (tmpFileName == NULL) {
        Tcl_AppendResult(interp, "can't create temporary directory \"",
                         nameTemplate, "\" for frame image files: ", 
                         Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    size_t length = strlen(tmpFileName);
    bool canceled = false;
    if (MakeImageFiles(tmpFileName,
                       switches.width, switches.height, switches.numFrames,
                       &canceled)) {
        result = TCL_OK;
    } else {
        result = TCL_ERROR;
    }
    if ((result == TCL_OK) && (!canceled)) {
        result = MakeMovie(interp, tmpFileName, token, &switches);
    }
    for (int i = 1; i <= switches.numFrames; i++) {
        sprintf(tmpFileName + length, "/image%d.ppm", i);
        unlink(tmpFileName);
    }
    tmpFileName[length] = '\0';
    rmdir(tmpFileName);
    FreeSwitches(flowVideoSwitches, &switches, 0);
    return result;
}
#else
/**
 *  Not implemented 
 */
static int
FlowVideoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    return TCL_OK;
}
#endif /* HAVE_FFMPEG */

static CmdSpec flowCmdOps[] = {
    {"add",      1, FlowAddOp,     3, 0, "name ?option value...?",},
    {"delete",   1, FlowDeleteOp,  3, 0, "name ?name...?",},
    {"exists",   1, FlowExistsOp,  3, 3, "name",},
    {"goto",     1, FlowGotoOp,    3, 3, "nSteps",},
    {"names",    1, FlowNamesOp,   2, 2, "",},
    {"next",     2, FlowNextOp,    2, 2, "",},
    {"reset",    1, FlowResetOp,   2, 2, "",},
    {"video",    1, FlowVideoOp,   3, 0, "token ?switches...?",},
};
static int nFlowCmdOps = NumCmdSpecs(flowCmdOps);

static int
FlowCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = GetOpFromObj(interp, nFlowCmdOps, flowCmdOps, 
                        CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/**
 *\brief This procedure is invoked to initialize the "flow" command.
 *
 * Side effects:
 *    Creates the new command and adds a new entry into a global Tcl
 *    associative array.
 */
void
nv::FlowCmdInitProc(Tcl_Interp *interp, ClientData clientData)
{
    Tcl_CreateObjCommand(interp, "flow", FlowCmdProc, clientData, NULL);
}