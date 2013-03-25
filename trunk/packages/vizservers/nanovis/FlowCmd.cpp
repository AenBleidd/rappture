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
#include <tcl.h>

#include <RpField1D.h>
#include <RpFieldRect3D.h>
#include <RpFieldPrism3D.h>
#include <RpOutcome.h>

#include <vrmath/Vector3f.h>

#include "nvconf.h"

#include "nanovis.h"
#include "CmdProc.h"
#include "FlowCmd.h"
#include "FlowTypes.h"
#include "FlowBox.h"
#include "FlowParticles.h"
#include "Switch.h"
#include "TransferFunction.h"
#include "NvLIC.h"
#include "Trace.h"
#include "Unirect.h"
#include "VelocityArrowsSlice.h"
#include "Volume.h"

using namespace vrmath;

static Rappture::SwitchParseProc AxisSwitchProc;
static Rappture::SwitchCustom axisSwitch = {
    AxisSwitchProc, NULL, 0,
};

static Rappture::SwitchParseProc ColorSwitchProc;
static Rappture::SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, 0,
};

static Rappture::SwitchParseProc PointSwitchProc;
static Rappture::SwitchCustom pointSwitch = {
    PointSwitchProc, NULL, 0,
};

static Rappture::SwitchParseProc PositionSwitchProc;
static Rappture::SwitchCustom positionSwitch = {
    PositionSwitchProc, NULL, 0,
};

static Rappture::SwitchParseProc TransferFunctionSwitchProc;
static Rappture::SwitchCustom transferFunctionSwitch = {
    TransferFunctionSwitchProc, NULL, 0,
};

Rappture::SwitchSpec FlowCmd::_switches[] = {
    {Rappture::SWITCH_FLOAT, "-ambient", "value",
     offsetof(FlowValues, ambient), 0},
    {Rappture::SWITCH_BOOLEAN, "-arrows", "boolean",
     offsetof(FlowValues, showArrows), 0},
    {Rappture::SWITCH_CUSTOM, "-axis", "axis",
     offsetof(FlowValues, slicePos.axis), 0, 0, &axisSwitch},
    {Rappture::SWITCH_FLOAT, "-diffuse", "value",
     offsetof(FlowValues, diffuse), 0},
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean",
     offsetof(FlowValues, isHidden), 0},
    {Rappture::SWITCH_BOOLEAN, "-light2side", "boolean",
     offsetof(FlowValues, twoSidedLighting), 0},
    {Rappture::SWITCH_FLOAT, "-opacity", "value",
     offsetof(FlowValues, opacity), 0},
    {Rappture::SWITCH_BOOLEAN, "-outline", "boolean",
     offsetof(FlowValues, showOutline), 0},
    {Rappture::SWITCH_CUSTOM, "-position", "number",
     offsetof(FlowValues, slicePos), 0, 0, &positionSwitch},
    {Rappture::SWITCH_BOOLEAN, "-slice", "boolean",
     offsetof(FlowValues, sliceVisible), 0},
    {Rappture::SWITCH_FLOAT, "-specularExp", "value",
     offsetof(FlowValues, specularExp), 0},
    {Rappture::SWITCH_FLOAT, "-specularLevel", "value",
     offsetof(FlowValues, specular), 0},
    {Rappture::SWITCH_CUSTOM, "-transferfunction", "name",
     offsetof(FlowValues, transferFunction), 0, 0, &transferFunctionSwitch},
    {Rappture::SWITCH_BOOLEAN, "-volume", "boolean",
     offsetof(FlowValues, showVolume), 0},
    {Rappture::SWITCH_END}
};

Rappture::SwitchSpec FlowParticles::_switches[] = {
    {Rappture::SWITCH_CUSTOM, "-axis", "string",
     offsetof(FlowParticlesValues, position.axis), 0, 0, &axisSwitch},
    {Rappture::SWITCH_CUSTOM, "-color", "{r g b a}",
     offsetof(FlowParticlesValues, color), 0, 0,  &colorSwitch},
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean",
     offsetof(FlowParticlesValues, isHidden), 0},
    {Rappture::SWITCH_CUSTOM, "-position", "number",
     offsetof(FlowParticlesValues, position), 0, 0, &positionSwitch},
    {Rappture::SWITCH_FLOAT, "-size", "float",
     offsetof(FlowParticlesValues, particleSize), 0},
    {Rappture::SWITCH_END}
};

Rappture::SwitchSpec FlowBox::_switches[] = {
    {Rappture::SWITCH_CUSTOM, "-color", "{r g b a}",
     offsetof(FlowBoxValues, color), 0, 0,  &colorSwitch},
    {Rappture::SWITCH_CUSTOM, "-corner1", "{x y z}",
     offsetof(FlowBoxValues, corner1), 0, 0, &pointSwitch},
    {Rappture::SWITCH_CUSTOM, "-corner2", "{x y z}",
     offsetof(FlowBoxValues, corner2), 0, 0, &pointSwitch},
    {Rappture::SWITCH_BOOLEAN, "-hide", "boolean",
     offsetof(FlowBoxValues, isHidden), 0},
    {Rappture::SWITCH_FLOAT, "-linewidth", "number",
     offsetof(FlowBoxValues, lineWidth), 0},
    {Rappture::SWITCH_END}
};

static Tcl_ObjCmdProc FlowInstObjCmd;
static Tcl_CmdDeleteProc FlowInstDeleteProc;

FlowCmd::FlowCmd(Tcl_Interp *interp, const char *name) :
    _interp(interp),
    _name(name),
    _data(NULL),
    _volume(NULL),
    _field(NULL)
{
    memset(&_sv, 0, sizeof(FlowValues));
    _sv.sliceVisible = 1;
    _sv.transferFunction = NanoVis::getTransferFunction("default");

    _cmdToken = Tcl_CreateObjCommand(_interp, (char *)name, 
                                     (Tcl_ObjCmdProc *)FlowInstObjCmd,
                                     this, FlowInstDeleteProc);
}

FlowCmd::~FlowCmd()
{
    TRACE("Enter");

    Rappture::FreeSwitches(_switches, &_sv, 0);
    if (_field != NULL) {
        delete _field;
    }
    if (_data != NULL) {
        delete _data;
    }
    if (_volume != NULL) {
        NanoVis::removeVolume(_volume);
        _volume = NULL;
    }
    for (BoxHashmap::iterator itr = _boxTable.begin();
         itr != _boxTable.end(); ++itr) {
        delete itr->second;
    }
    _boxTable.clear();
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        delete itr->second;
    }
    _particlesTable.clear();
}

void
FlowCmd::getBounds(Vector3f& min,
                   Vector3f& max,
                   bool onlyVisible)
{
    TRACE("Enter");

    if (onlyVisible && !visible())
        return;

#if 0  // Using volume bounds instead of these
    if (isDataLoaded()) {
        Vector3f umin, umax;
        Rappture::Unirect3d *unirect = data();
        unirect->getWorldSpaceBounds(umin, umax);
        if (min.x > umin.x) {
            min.x = umin.x;
        }
        if (max.x < umax.x) {
            max.x = umax.x;
        }
        if (min.y > umin.y) {
            min.y = umin.y;
        }
        if (max.y < umax.y) {
            max.y = umax.y;
        }
        if (min.z > umin.z) {
            min.z = umin.z;
        }
        if (max.z < umax.z) {
            max.z = umax.z;
        }
    }
#endif
    for (BoxHashmap::iterator itr = _boxTable.begin();
         itr != _boxTable.end(); ++itr) {
        FlowBox *box = itr->second;
        if (!onlyVisible || box->visible()) {
            Vector3f fbmin, fbmax;
            box->getWorldSpaceBounds(fbmin, fbmax,
                                     getVolume());
            if (min.x > fbmin.x) {
                min.x = fbmin.x;
            }
            if (max.x < fbmax.x) {
                max.x = fbmax.x;
            }
            if (min.y > fbmin.y) {
                min.y = fbmin.y;
            }
            if (max.y < fbmax.y) {
                max.y = fbmax.y;
            }
            if (min.z > fbmin.z) {
                min.z = fbmin.z;
            }
            if (max.z < fbmax.z) {
                max.z = fbmax.z;
            }
        }
    }
}

void
FlowCmd::resetParticles()
{
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        itr->second->reset();
    }
}

void
FlowCmd::advect()
{
    NvVectorField *fieldPtr = getVectorField();
    fieldPtr->active(true);
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        if (itr->second->visible()) {
            itr->second->advect();
        }
    }
}

void
FlowCmd::render()
{
    _field->active(true);
    _field->render();
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        if (itr->second->visible()) {
            itr->second->render();
        }
    }
    renderBoxes();
}

FlowParticles *
FlowCmd::createParticles(const char *particlesName)
{
    ParticlesHashmap::iterator itr = _particlesTable.find(particlesName);
    if (itr != _particlesTable.end()) {
        TRACE("Deleting existing particle injection plane '%s'", particlesName);
        delete itr->second;
        _particlesTable.erase(itr);
    }
    FlowParticles *particles = new FlowParticles(particlesName);
    _particlesTable[particlesName] = particles;
    return particles;
}

FlowParticles *
FlowCmd::getParticles(const char *particlesName)
{
    ParticlesHashmap::iterator itr;
    itr = _particlesTable.find(particlesName);
    if (itr == _particlesTable.end()) {
        TRACE("Can't find particle injection plane '%s' in '%s'", particlesName, name());
        return NULL;
    }
    return itr->second;
}

void
FlowCmd::deleteParticles(const char *particlesName)
{
    ParticlesHashmap::iterator itr = _particlesTable.find(particlesName);
    if (itr == _particlesTable.end()) {
        TRACE("Can't find particle injection plane '%s' in '%s'", particlesName, name());
        return;
    }
    delete itr->second;
    _particlesTable.erase(itr);
}

void
FlowCmd::getParticlesNames(std::vector<std::string>& names)
{
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        names.push_back(std::string(itr->second->name()));
    }
}

FlowBox *
FlowCmd::createBox(const char *boxName)
{
    BoxHashmap::iterator itr = _boxTable.find(boxName);
    if (itr != _boxTable.end()) {
        TRACE("Deleting existing box '%s'", boxName);
        delete itr->second;
        _boxTable.erase(itr);
    }
    FlowBox *box = new FlowBox(boxName);
    _boxTable[boxName] = box;
    return box;
}

FlowBox *
FlowCmd::getBox(const char *boxName)
{
    BoxHashmap::iterator itr = _boxTable.find(boxName);
    if (itr == _boxTable.end()) {
        TRACE("Can't find box '%s' in '%s'", boxName, name());
        return NULL;
    }
    return itr->second;
}

void
FlowCmd::deleteBox(const char *boxName)
{
    BoxHashmap::iterator itr = _boxTable.find(boxName);
    if (itr == _boxTable.end()) {
        TRACE("Can't find box '%s' in '%s'", boxName, name());
        return;
    }
    delete itr->second;
    _boxTable.erase(itr);
}

void FlowCmd::getBoxNames(std::vector<std::string>& names)
{
    for (BoxHashmap::iterator itr = _boxTable.begin();
         itr != _boxTable.end(); ++itr) {
        names.push_back(std::string(itr->second->name()));
    }
}

void
FlowCmd::initializeParticles()
{
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        itr->second->initialize();
    }
}

bool
FlowCmd::scaleVectorField()
{
    if (_volume != NULL) {
        TRACE("Removing existing volume: %s", _volume->name());
        NanoVis::removeVolume(_volume);
        _volume = NULL;
    }
    float *vdata = getScaledVector();
    if (vdata == NULL) {
        return false;
    }
    Volume *volume = makeVolume(vdata);
    delete [] vdata;
    if (volume == NULL) {
        return false;
    }
    _volume = volume;

    // Remove the associated vector field.
    if (_field != NULL) {
        delete _field;
    }
    _field = new NvVectorField();
    if (_field == NULL) {
        return false;
    }

    Vector3f scale = volume->getPhysicalScaling();
    Vector3f location = _volume->location();

    _field->setVectorField(_volume,
                           location,
                           scale.x,
                           scale.y,
                           scale.z,
                           NanoVis::magMax);

    if (NanoVis::licRenderer != NULL) {
        NanoVis::licRenderer->
            setVectorField(_volume->textureID(),
                           location,
                           scale.x,
                           scale.y,
                           scale.z,
                           _volume->wAxis.max());
        setCurrentPosition();
        setAxis();
        setActive();
    }

    if (NanoVis::velocityArrowsSlice != NULL) {
        NanoVis::velocityArrowsSlice->
            setVectorField(_volume->textureID(),
                           location,
                           scale.x,
                           scale.y,
                           scale.z,
                           _volume->wAxis.max());
        NanoVis::velocityArrowsSlice->axis(_sv.slicePos.axis);
        NanoVis::velocityArrowsSlice->slicePos(_sv.slicePos.value);
        NanoVis::velocityArrowsSlice->enabled(_sv.showArrows);
    }

    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        itr->second->setVectorField(_volume,
                                    location,
                                    scale.x,
                                    scale.y,
                                    scale.z,
                                    _volume->wAxis.max());
    }
    return true;
}

void
FlowCmd::renderBoxes()
{
    for (BoxHashmap::iterator itr = _boxTable.begin();
         itr != _boxTable.end(); ++itr) {
        if (itr->second->visible()) {
            itr->second->render(_volume);
        }
    }
}

float *
FlowCmd::getScaledVector()
{
    assert(_data->nComponents() == 3);
    size_t n = _data->nValues() / _data->nComponents() * 4;
    float *data = new float[n];
    if (data == NULL) {
        return NULL;
    }
    memset(data, 0, sizeof(float) * n);
    float *destPtr = data;
    const float *values = _data->values();
    for (size_t iz = 0; iz < _data->zNum(); iz++) {
        for (size_t iy = 0; iy < _data->yNum(); iy++) {
            for (size_t ix = 0; ix < _data->xNum(); ix++) {
                double vx, vy, vz, vm;
                vx = values[0];
                vy = values[1];
                vz = values[2];
                vm = sqrt(vx*vx + vy*vy + vz*vz);
                destPtr[0] = vm / NanoVis::magMax;
                destPtr[1] = vx /(2.0*NanoVis::magMax) + 0.5;
                destPtr[2] = vy /(2.0*NanoVis::magMax) + 0.5;
                destPtr[3] = vz /(2.0*NanoVis::magMax) + 0.5;
                values += 3;
                destPtr += 4;
            }
        }
    }
    return data;
}

Volume *
FlowCmd::makeVolume(float *data)
{
    Volume *volume =
        NanoVis::loadVolume(_name.c_str(),
                            _data->xNum(),
                            _data->yNum(), 
                            _data->zNum(),
                            4, data, 
                            NanoVis::magMin, NanoVis::magMax, 0);
    volume->xAxis.setRange(_data->xMin(), _data->xMax());
    volume->yAxis.setRange(_data->yMin(), _data->yMax());
    volume->zAxis.setRange(_data->zMin(), _data->zMax());

    TRACE("min=%g %g %g max=%g %g %g mag=%g %g",
          NanoVis::xMin, NanoVis::yMin, NanoVis::zMin,
          NanoVis::xMax, NanoVis::yMax, NanoVis::zMax,
          NanoVis::magMin, NanoVis::magMax);

    volume->disableCutplane(0);
    volume->disableCutplane(1);
    volume->disableCutplane(2);

    /* Initialize the volume with the previously configured values. */
    volume->transferFunction(_sv.transferFunction);
    volume->dataEnabled(_sv.showVolume);
    volume->twoSidedLighting(_sv.twoSidedLighting);
    volume->outline(_sv.showOutline);
    volume->opacityScale(_sv.opacity);
    volume->ambient(_sv.ambient);
    volume->diffuse(_sv.diffuse);
    volume->specularLevel(_sv.specular);
    volume->specularExponent(_sv.specularExp);
    volume->visible(_sv.showVolume);

    Vector3f volScaling = volume->getPhysicalScaling();
    Vector3f loc(volScaling);
    loc *= -0.5;
    volume->location(loc);

    Volume::updatePending = true;
    return volume;
}

static int
FlowDataFileOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    Rappture::Outcome result;

    const char *fileName;
    fileName = Tcl_GetString(objv[3]);
    TRACE("File: %s", fileName);

    int nComponents;
    if (Tcl_GetIntFromObj(interp, objv[4], &nComponents) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((nComponents < 1) || (nComponents > 4)) {
        Tcl_AppendResult(interp, "bad # of components \"", 
                         Tcl_GetString(objv[4]), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    Rappture::Buffer buf;
    if (!buf.load(result, fileName)) {
        Tcl_AppendResult(interp, "can't load data from \"", fileName, "\": ",
                         result.remark(), (char *)NULL);
        return TCL_ERROR;
    }

    Rappture::Unirect3d *dataPtr;
    dataPtr = new Rappture::Unirect3d(nComponents);
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    size_t length = buf.size();
    char *bytes = (char *)buf.bytes();
    if ((length > 4) && (strncmp(bytes, "<DX>", 4) == 0)) {
        if (!dataPtr->importDx(result, nComponents, length-4, bytes+4)) {
            Tcl_AppendResult(interp, result.remark(), (char *)NULL);
            delete dataPtr;
            return TCL_ERROR;
        }
    } else if ((length > 10) && (strncmp(bytes, "unirect3d ", 10) == 0)) {
        if (dataPtr->parseBuffer(interp, buf) != TCL_OK) {
            delete dataPtr;
            return TCL_ERROR;
        }
    } else if ((length > 10) && (strncmp(bytes, "unirect2d ", 10) == 0)) {
        Rappture::Unirect2d *u2dPtr;
        u2dPtr = new Rappture::Unirect2d(nComponents);
        if (u2dPtr->parseBuffer(interp, buf) != TCL_OK) {
            delete u2dPtr;
            return TCL_ERROR;
        }
        dataPtr->convert(u2dPtr);
        delete u2dPtr;
    } else {
        TRACE("header is %.14s", buf.bytes());
        if (!dataPtr->importDx(result, nComponents, length, bytes)) {
            Tcl_AppendResult(interp, result.remark(), (char *)NULL);
            delete dataPtr;
            return TCL_ERROR;
        }
    }
    if (dataPtr->nValues() == 0) {
        delete dataPtr;
        Tcl_AppendResult(interp, "no data found in \"", fileName, "\"",
                         (char *)NULL);
        return TCL_ERROR;
    }
    flowPtr->data(dataPtr);
    NanoVis::eventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

/**
 * $flow data follows nbytes nComponents
 */
static int
FlowDataFollowsOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    Rappture::Outcome result;

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
    Rappture::Buffer buf;
    TRACE("Flow data loading bytes: %d components: %d", nBytes, nComponents);
    if (GetDataStream(interp, buf, nBytes) != TCL_OK) {
        return TCL_ERROR;
    }
    Rappture::Unirect3d *dataPtr;
    dataPtr = new Rappture::Unirect3d(nComponents);

    FlowCmd *flowPtr = (FlowCmd *)clientData;
    size_t length = buf.size();
    char *bytes = (char *)buf.bytes();
    if ((length > 4) && (strncmp(bytes, "<DX>", 4) == 0)) {
        if (!dataPtr->importDx(result, nComponents, length - 4, bytes + 4)) {
            Tcl_AppendResult(interp, result.remark(), (char *)NULL);
            delete dataPtr;
            return TCL_ERROR;
        }
    } else if ((length > 10) && (strncmp(bytes, "unirect3d ", 10) == 0)) {
        if (dataPtr->parseBuffer(interp, buf) != TCL_OK) {
            delete dataPtr;
            return TCL_ERROR;
        }
    } else if ((length > 10) && (strncmp(bytes, "unirect2d ", 10) == 0)) {
        Rappture::Unirect2d *u2dPtr;
        u2dPtr = new Rappture::Unirect2d(nComponents);
        if (u2dPtr->parseBuffer(interp, buf) != TCL_OK) {
            delete u2dPtr;
            return TCL_ERROR;
        }
        dataPtr->convert(u2dPtr);
        delete u2dPtr;
    } else {
        TRACE("header is %.14s", buf.bytes());
        if (!dataPtr->importDx(result, nComponents, length, bytes)) {
            Tcl_AppendResult(interp, result.remark(), (char *)NULL);
            delete dataPtr;
            return TCL_ERROR;
        }
    }
    if (dataPtr->nValues() == 0) {
        delete dataPtr;
        Tcl_AppendResult(interp, "no data found in stream", (char *)NULL);
        return TCL_ERROR;
    }
    TRACE("nx = %d ny = %d nz = %d",
          dataPtr->xNum(), dataPtr->yNum(), dataPtr->zNum());
    TRACE("x0 = %lg y0 = %lg z0 = %lg",
          dataPtr->xMin(), dataPtr->yMin(), dataPtr->zMin());
    TRACE("lx = %lg ly = %lg lz = %lg",
          dataPtr->xMax() - dataPtr->xMin(),
          dataPtr->yMax() - dataPtr->yMin(),
          dataPtr->zMax() - dataPtr->zMin());
    TRACE("dx = %lg dy = %lg dz = %lg",
          dataPtr->xNum() > 1 ? (dataPtr->xMax() - dataPtr->xMin())/(dataPtr->xNum()-1) : 0,
          dataPtr->yNum() > 1 ? (dataPtr->yMax() - dataPtr->yMin())/(dataPtr->yNum()-1) : 0,
          dataPtr->zNum() > 1 ? (dataPtr->zMax() - dataPtr->zMin())/(dataPtr->zNum()-1) : 0);
    TRACE("magMin = %lg magMax = %lg",
          dataPtr->magMin(), dataPtr->magMax());
    flowPtr->data(dataPtr);
    {
        char info[1024];
        ssize_t nWritten;
        size_t length;

        length = sprintf(info, "nv>data tag %s min %g max %g\n",
                         flowPtr->name(), dataPtr->magMin(), dataPtr->magMax());
        nWritten  = write(1, info, length);
        assert(nWritten == (ssize_t)strlen(info));
    }
    NanoVis::eventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static Rappture::CmdSpec flowDataOps[] = {
    {"file",    2, FlowDataFileOp,    5, 5, "fileName nComponents",},
    {"follows", 2, FlowDataFollowsOp, 5, 5, "size nComponents",},
};
static int nFlowDataOps = NumCmdSpecs(flowDataOps);

static int
FlowDataOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Rappture::GetOpFromObj(interp, nFlowDataOps, flowDataOps,
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

float
FlowCmd::getRelativePosition(FlowPosition *posPtr)
{
    if (posPtr->flags == RELPOS) {
        return posPtr->value;
    }
    switch (posPtr->axis) {
    case AXIS_X:  
        return (posPtr->value - NanoVis::xMin) / 
            (NanoVis::xMax - NanoVis::xMin); 
    case AXIS_Y:  
        return (posPtr->value - NanoVis::yMin) / 
            (NanoVis::yMax - NanoVis::yMin); 
    case AXIS_Z:  
        return (posPtr->value - NanoVis::zMin) / 
            (NanoVis::zMax - NanoVis::zMin); 
    }
    return 0.0;
}

float
FlowCmd::getRelativePosition() 
{
    return FlowCmd::getRelativePosition(&_sv.slicePos);
}

/* Static NanoVis class commands. */

FlowCmd *
NanoVis::getFlow(const char *name)
{
    FlowHashmap::iterator itr = flowTable.find(name);
    if (itr == flowTable.end()) {
        TRACE("Can't find flow '%s'", name);
        return NULL;
    }
    return itr->second;
}

FlowCmd *
NanoVis::createFlow(Tcl_Interp *interp, const char *name)
{
    FlowHashmap::iterator itr = flowTable.find(name);
    if (itr != flowTable.end()) {
        ERROR("Flow '%s' already exists", name);
        return NULL;
    }
    FlowCmd *flow = new FlowCmd(interp, name);
    flowTable[name] = flow;
    return flow;
}

/**
 * \brief Delete flow object and hash table entry
 *
 * This is called by the flow command instance delete callback
 */
void
NanoVis::deleteFlow(const char *name)
{
    FlowHashmap::iterator itr = flowTable.find(name);
    if (itr != flowTable.end()) {
        delete itr->second;
        flowTable.erase(itr);
    }
}

/**
 * \brief Delete all flow object commands
 *
 * This will also delete the flow objects and hash table entries
 */
void
NanoVis::deleteFlows(Tcl_Interp *interp)
{
    FlowHashmap::iterator itr;
    for (itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        Tcl_DeleteCommandFromToken(interp, itr->second->getCommandToken());
    }
    flowTable.clear();
}

bool
NanoVis::mapFlows()
{
    TRACE("Enter");

    flags &= ~MAP_FLOWS;

    /* 
     * Step 1. Get the overall min and max magnitudes of all the 
     *         flow vectors.
     */
    magMin = DBL_MAX, magMax = -DBL_MAX;

    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        FlowCmd *flow = itr->second;
        double min, max;
        if (!flow->isDataLoaded()) {
            continue;
        }
        Rappture::Unirect3d *data = flow->data();
        min = data->magMin();
        max = data->magMax();
        if (min < magMin) {
            magMin = min;
        } 
        if (max > magMax) {
            magMax = max;
        }
        if (data->xMin() < xMin) {
            xMin = data->xMin();
        }
        if (data->yMin() < yMin) {
            yMin = data->yMin();
        }
        if (data->zMin() < zMin) {
            zMin = data->zMin();
        }
        if (data->xMax() > xMax) {
            xMax = data->xMax();
        }
        if (data->yMax() > yMax) {
            yMax = data->yMax();
        }
        if (data->zMax() > zMax) {
            zMax = data->zMax();
        }
    }

    TRACE("magMin=%g magMax=%g", NanoVis::magMin, NanoVis::magMax);

    /* 
     * Step 2. Generate the vector field from each data set. 
     */
    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        FlowCmd *flow = itr->second;
        if (!flow->isDataLoaded()) {
            continue; // Flow exists, but no data has been loaded yet.
        }
        if (flow->visible()) {
            flow->initializeParticles();
        }
        if (!flow->scaleVectorField()) {
            return false;
        }
        // FIXME: This doesn't work when there is more than one flow.
        licRenderer->setOffset(flow->getRelativePosition());
        velocityArrowsSlice->slicePos(flow->getRelativePosition());
    }
    advectFlows();
    return true;
}

void
NanoVis::getFlowBounds(Vector3f& min,
                       Vector3f& max,
                       bool onlyVisible)
{
    TRACE("Enter");

    min.set(FLT_MAX, FLT_MAX, FLT_MAX);
    max.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        itr->second->getBounds(min, max, onlyVisible);
    }
}

void
NanoVis::renderFlows()
{
    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        FlowCmd *flow = itr->second;
        if (flow->isDataLoaded() && flow->visible()) {
            flow->render();
        }
    }
    flags &= ~REDRAW_PENDING;
}

void
NanoVis::resetFlows()
{
    if (licRenderer->active()) {
        NanoVis::licRenderer->reset();
    }
    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        FlowCmd *flow = itr->second;
        if (flow->isDataLoaded() && flow->visible()) {
            flow->resetParticles();
        }
    }
}    

void
NanoVis::advectFlows()
{
    for (FlowHashmap::iterator itr = flowTable.begin();
         itr != flowTable.end(); ++itr) {
        FlowCmd *flow = itr->second;
        if (flow->isDataLoaded() && flow->visible()) {
            flow->advect();
        }
    }
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
        FlowCmd::SliceAxis *axisPtr = (FlowCmd::SliceAxis *)(record + offset);
        char c;
        c = tolower((unsigned char)string[0]);
        if (c == 'x') {
            *axisPtr = FlowCmd::AXIS_X;
            return TCL_OK;
        } else if (c == 'y') {
            *axisPtr = FlowCmd::AXIS_Y;
            return TCL_OK;
        } else if (c == 'z') {
            *axisPtr = FlowCmd::AXIS_Z;
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
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->parseSwitches(interp, objc - 2, objv + 2) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::eventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static int
FlowParticlesAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    FlowCmd *flow = (FlowCmd *)clientData;
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
    NanoVis::eventuallyRedraw();
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

static int
FlowParticlesConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *const *objv)
{
    FlowCmd *flow = (FlowCmd *)clientData;
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
    particles->configure();
    NanoVis::eventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static int
FlowParticlesDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const *objv)
{
    FlowCmd *flow = (FlowCmd *)clientData;
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
    FlowCmd *flow = (FlowCmd *)clientData;
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

static Rappture::CmdSpec flowParticlesOps[] = {
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
    proc = Rappture::GetOpFromObj(interp, nFlowParticlesOps, flowParticlesOps, 
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Preserve(flowPtr);
    int result;
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flowPtr);
    return result;
}

static int
FlowBoxAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    FlowCmd *flow = (FlowCmd *)clientData;
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
    FlowCmd *flow = (FlowCmd *)clientData;
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
    FlowCmd *flow = (FlowCmd *)clientData;
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
    FlowCmd *flow = (FlowCmd *)clientData;
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

static Rappture::CmdSpec flowBoxOps[] = {
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
    proc = Rappture::GetOpFromObj(interp, nFlowBoxOps, flowBoxOps, 
                                  Rappture::CMDSPEC_ARG2, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Preserve(flowPtr);
    int result;
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flowPtr);
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
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    
    const char *string = Tcl_GetString(objv[1]);
    TransferFunction *tf;
    tf = flowPtr->getTransferFunction();
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
    if (NanoVis::flags & NanoVis::MAP_FLOWS) {
        NanoVis::mapFlows();
    }
    NanoVis::renderLegend(tf, NanoVis::magMin, NanoVis::magMax, w, h, label);
    return TCL_OK;
}

static Rappture::CmdSpec flowInstOps[] = {
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
static int
FlowInstObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    proc = Rappture::GetOpFromObj(interp, nFlowInstOps, flowInstOps, 
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    assert(CheckGL(AT));
    FlowCmd *flow = (FlowCmd *)clientData;
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
static void
FlowInstDeleteProc(ClientData clientData)
{
    FlowCmd *flow = (FlowCmd *)clientData;
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
        return NULL;
    }
    FlowCmd *flow = NanoVis::createFlow(interp, name);
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
        FlowCmd *flow = NanoVis::getFlow(Tcl_GetString(objv[i]));
        if (flow != NULL) {
            Tcl_DeleteCommandFromToken(interp, flow->getCommandToken());
        }
    }
    NanoVis::eventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static int
FlowExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    bool value = false;
    FlowCmd *flow = NanoVis::getFlow(Tcl_GetString(objv[2]));
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
    if (NanoVis::flags & NanoVis::MAP_FLOWS) {
        NanoVis::mapFlows();
    }
    NanoVis::advectFlows();
    for (int i = 0; i < nSteps; i++) {
        if (NanoVis::licRenderer->active()) {
            NanoVis::licRenderer->convolve();
        }
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
        FlowCmd *flow = itr->second;
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
    if (NanoVis::flags & NanoVis::MAP_FLOWS) {
        NanoVis::mapFlows();
    }
    NanoVis::eventuallyRedraw();
    NanoVis::licRenderer->convolve();
    NanoVis::advectFlows();
    return TCL_OK;
}

static int
FlowResetOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    NanoVis::resetFlows();
    return TCL_OK;
}

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

static Rappture::SwitchParseProc VideoFormatSwitchProc;
static Rappture::SwitchCustom videoFormatSwitch = {
    VideoFormatSwitchProc, NULL, 0,
};

Rappture::SwitchSpec FlowCmd::videoSwitches[] = {
    {Rappture::SWITCH_INT, "-bitrate", "value",
     offsetof(FlowVideoSwitches, bitRate), 0},
    {Rappture::SWITCH_CUSTOM, "-format", "string",
     offsetof(FlowVideoSwitches, formatObjPtr), 0, 0, &videoFormatSwitch},
    {Rappture::SWITCH_FLOAT, "-framerate", "value",
     offsetof(FlowVideoSwitches, frameRate), 0},
    {Rappture::SWITCH_INT, "-height", "integer",
     offsetof(FlowVideoSwitches, height), 0},
    {Rappture::SWITCH_INT, "-numframes", "count",
     offsetof(FlowVideoSwitches, numFrames), 0},
    {Rappture::SWITCH_INT, "-width", "integer",
     offsetof(FlowVideoSwitches, width), 0},
    {Rappture::SWITCH_END}
};

#ifdef HAVE_FFMPEG

static int
ppmWriteToFile(Tcl_Interp *interp, const char *path, FlowVideoSwitches *switchesPtr)
{
    int f;
    
    /* Open the named file for writing. */
    f = creat(path, 0600);
    if (f < 0) {
        Tcl_AppendResult(interp, "can't open temporary image file \"", path,
                         "\": ", Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    // Generate the PPM binary file header
    char header[200];
#define PPM_MAXVAL 255
    sprintf(header, "P6 %d %d %d\n", switchesPtr->width, switchesPtr->height, 
        PPM_MAXVAL);

    size_t header_length = strlen(header);
    size_t wordsPerRow = (switchesPtr->width * 24 + 31) / 32;
    size_t bytesPerRow = wordsPerRow * 4;
    size_t rowLength = switchesPtr->width * 3;
    size_t numRecords = switchesPtr->height + 1;

    struct iovec *iov;
    iov = (struct iovec *)malloc(sizeof(struct iovec) * numRecords);

    // Add the PPM image header.
    iov[0].iov_base = header;
    iov[0].iov_len = header_length;

    // Now add the image data, reversing the order of the rows.
    int y;
    unsigned char *srcRowPtr = NanoVis::screenBuffer;
    /* Reversing the pointers for the image rows.  PPM is top-to-bottom. */
    for (y = switchesPtr->height; y >= 1; y--) {
        iov[y].iov_base = srcRowPtr;
        iov[y].iov_len = rowLength;
        srcRowPtr += bytesPerRow;
    }
    if (writev(f, iov, numRecords) < 0) {
        Tcl_AppendResult(interp, "writing image to \"", path, "\" failed: ", 
                         Tcl_PosixError(interp), (char *)NULL);
        free(iov);
        close(f);
        return TCL_ERROR;
    }
    close(f);
    free(iov);
    return TCL_OK;
}

static int
MakeImageFiles(Tcl_Interp *interp, char *tmpFileName, 
               FlowVideoSwitches *switchesPtr, bool *cancelPtr)
{
    struct pollfd pollResults;
    pollResults.fd = fileno(NanoVis::stdin);
    pollResults.events = POLLIN;
#define PENDING_TIMEOUT          10  /* milliseconds. */
    int timeout = PENDING_TIMEOUT;

    int oldWidth, oldHeight;
    oldWidth = NanoVis::winWidth;
    oldHeight = NanoVis::winHeight;

    if ((switchesPtr->width != oldWidth) || 
        (switchesPtr->height != oldHeight)) {
        // Resize to the requested size.
        NanoVis::resizeOffscreenBuffer(switchesPtr->width, switchesPtr->height);
    }
    NanoVis::ResetFlows();
    *cancelPtr = false;
    int result = TCL_OK;
    size_t length = strlen(tmpFileName);
    for (int i = 1; i <= switchesPtr->numFrames; i++) {
        if (((i & 0xF) == 0) && (poll(&pollResults, 1, timeout) > 0)) {
            /* If there's another command on stdin, that means the client is
             * trying to cancel this operation. */
            *cancelPtr = true;
            break;
        }
        if (NanoVis::licRenderer->active()) {
            NanoVis::licRenderer->convolve();
        }
        NanoVis::AdvectFlows();

        int fboOrig;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fboOrig);

        NanoVis::bindOffscreenBuffer();
        NanoVis::render();
        NanoVis::readScreen();

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboOrig);

        sprintf(tmpFileName + length, "/image%d.ppm", i);
        result = ppmWriteToFile(interp, tmpFileName, switchesPtr);
        if (result != TCL_OK) {
            break;
        }
    }
    if ((switchesPtr->width != oldWidth) || 
        (switchesPtr->height != oldHeight)) {
        NanoVis::resizeOffscreenBuffer(oldWidth, oldHeight);
    }
    tmpFileName[length] = '\0';
    NanoVis::ResetFlows();
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
    sprintf(cmd, "%s -f image2 -i %s/image%%d.ppm -f %s -b:v %d -r %f -",
            FFMPEG, tmpFileName, Tcl_GetString(switchesPtr->formatObjPtr), 
            switchesPtr->bitRate, switchesPtr->frameRate);
    TRACE("MakeMovie %s", cmd);
    FILE *f;
    f = popen(cmd, "r");
    if (f == NULL) {
        Tcl_AppendResult(interp, "can't run ffmpeg: ", 
                         Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    Rappture::Buffer data;
    size_t total = 0;
    for (;;) {
        ssize_t numRead;
        char buffer[BUFSIZ]; 
        
        numRead = fread(buffer, sizeof(unsigned char), BUFSIZ, f);
        total += numRead;
        if (numRead == 0) {             // EOF
            break;
        }
        if (numRead < 0) {              // Error
            ERROR("MakeMovie: can't read movie data: %s",
                  Tcl_PosixError(interp));
            Tcl_AppendResult(interp, "can't read movie data: ", 
                Tcl_PosixError(interp), (char *)NULL);
            return TCL_ERROR;
        }
        if (!data.append(buffer, numRead)) {
            ERROR("MakeMovie: can't append movie data to buffer %d bytes",
                  numRead);
            Tcl_AppendResult(interp, "can't append movie data to buffer",
                             (char *)NULL);
            return TCL_ERROR;
        }
    }
    sprintf(cmd,"nv>image -type movie -token \"%s\" -bytes %lu\n", 
            token, (unsigned long)data.size());
    NanoVis::sendDataToClient(cmd, data.bytes(), data.size());
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
    if (Rappture::ParseSwitches(interp, FlowCmd::videoSwitches, 
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
    result = MakeImageFiles(interp, tmpFileName, &switches, &canceled);
    if ((result == TCL_OK) && (!canceled)) {
        result = MakeMovie(interp, tmpFileName, token, &switches);
    }
    for (int i = 1; i <= switches.numFrames; i++) {
        sprintf(tmpFileName + length, "/image%d.ppm", i);
        unlink(tmpFileName);
    }        
    tmpFileName[length] = '\0';
    rmdir(tmpFileName);
    Rappture::FreeSwitches(FlowCmd::videoSwitches, &switches, 0);
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

static Rappture::CmdSpec flowCmdOps[] = {
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

    proc = Rappture::GetOpFromObj(interp, nFlowCmdOps, flowCmdOps, 
                                  Rappture::CMDSPEC_ARG1, objc, objv, 0);
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
int
FlowCmdInitProc(Tcl_Interp *interp)
{
    Tcl_CreateObjCommand(interp, "flow", FlowCmdProc, NULL, NULL);
    return TCL_OK;
}
