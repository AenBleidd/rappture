/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <poll.h>

#include <tcl.h>

#include <RpField1D.h>
#include <RpFieldRect3D.h>
#include <RpFieldPrism3D.h>
#include <RpOutcome.h>

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>
#include <vrmath/Matrix4x4d.h>

#include "nvconf.h"

#if defined(HAVE_LIBAVCODEC) || defined(HAVE_LIBAVFORMAT)
#define HAVE_FFMPEG 1
#endif

#ifdef HAVE_FFMPEG
#include "RpAVTranslate.h"
#endif

#include "nanovis.h"
#include "FlowCmd.h"
#include "CmdProc.h"
#include "Switch.h"
#include "TransferFunction.h"
#include "NvLIC.h"
#include "Trace.h"
#include "Unirect.h"
#include "VelocityArrowsSlice.h"
#include "Volume.h"

#define RELPOS 0
#define ABSPOS 1

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
     offsetof(FlowValues, tfPtr), 0, 0, &transferFunctionSwitch},
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

FlowParticles::FlowParticles(const char *name, Tcl_HashEntry *hPtr) :
    _name(name),
    _hashPtr(hPtr),
    _rendererPtr(new NvParticleRenderer(NMESH, NMESH))
{
    _sv.position.value = 0.0f;
    _sv.position.flags = RELPOS;
    _sv.position.axis = 0; // X_AXIS
    _sv.color.r = _sv.color.g = _sv.color.b = _sv.color.a = 1.0f;
    _sv.isHidden = false;
    _sv.particleSize = 1.2;
}

FlowParticles::~FlowParticles() 
{
    if (_rendererPtr != NULL) {
        delete _rendererPtr;
    }
    if (_hashPtr != NULL) {
        Tcl_DeleteHashEntry(_hashPtr);
    }
    Rappture::FreeSwitches(_switches, &_sv, 0);
}

void
FlowParticles::render() 
{
    TRACE("rendering particles %s", _name);
    TRACE("rendering particles %s axis=%d", _name, _sv.position.axis);
    TRACE("rendering particles %s position=%g", _name, _sv.position.value);
    TRACE("rendering particles %s position=%g", _name, 
          FlowCmd::GetRelativePosition(&_sv.position));

    _rendererPtr->setPos(FlowCmd::GetRelativePosition(&_sv.position));
    _rendererPtr->setAxis(_sv.position.axis);
    assert(_rendererPtr->active());
    _rendererPtr->render();
}

void 
FlowParticles::configure() 
{
    _rendererPtr->setPos(FlowCmd::GetRelativePosition(&_sv.position));
    _rendererPtr->setColor(Vector4f(_sv.color.r, _sv.color.g, _sv.color.b, 
                                    _sv.color.a));
    _rendererPtr->particleSize(_sv.particleSize);
    _rendererPtr->setAxis(_sv.position.axis);
    _rendererPtr->active(!_sv.isHidden);
}

FlowBox::FlowBox(const char *name, Tcl_HashEntry *hPtr) 
{
    _name = name;
    _hashPtr = hPtr;
    _sv.isHidden = false;
    _sv.corner1.x = 0.0f;
    _sv.corner1.y = 0.0f;
    _sv.corner1.z = 0.0f;
    _sv.corner2.x = 1.0f;
    _sv.corner2.y = 1.0f;
    _sv.corner2.z = 1.0f;
    _sv.lineWidth = 1.2f;
    _sv.color.r = _sv.color.b = _sv.color.g = _sv.color.a = 1.0f;
}

void
FlowBox::getWorldSpaceBounds(Vector3f& bboxMin,
                             Vector3f& bboxMax,
                             const Volume *vol) const
{
    bboxMin.set(FLT_MAX, FLT_MAX, FLT_MAX);
    bboxMax.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    Vector3f origin = vol->location();
    Vector3f scale = vol->getPhysicalScaling();

    Matrix4x4d mat;
    mat.makeTranslation(origin);
    Matrix4x4d mat2;
    mat2.makeScale(scale);

    mat.multiply(mat2);

    Vector3f min, max;
    min.x = vol->xAxis.min();
    min.y = vol->yAxis.min();
    min.z = vol->zAxis.min();
    max.x = vol->xAxis.max();
    max.y = vol->yAxis.max();
    max.z = vol->zAxis.max();

    float x0, y0, z0, x1, y1, z1;
    x0 = y0 = z0 = 0.0f;
    x1 = y1 = z1 = 0.0f;
    if (max.x > min.x) {
        x0 = (_sv.corner1.x - min.x) / (max.x - min.x);
        x1 = (_sv.corner2.x - min.x) / (max.x - min.x);
    }
    if (max.y > min.y) {
        y0 = (_sv.corner1.y - min.y) / (max.y - min.y);
        y1 = (_sv.corner2.y - min.y) / (max.y - min.y);
    }
    if (max.z > min.z) {
        z0 = (_sv.corner1.z - min.z) / (max.z - min.z);
        z1 = (_sv.corner2.z - min.z) / (max.z - min.z);
    }

    TRACE("Box model bounds: (%g,%g,%g) - (%g,%g,%g)", x0, y0, z0, x1, y1, z1);

    Vector3f modelMin(x0, y0, z0);
    Vector3f modelMax(x1, y1, z1);

    Vector4f bvert[8];
    bvert[0] = Vector4f(modelMin.x, modelMin.y, modelMin.z, 1);
    bvert[1] = Vector4f(modelMax.x, modelMin.y, modelMin.z, 1);
    bvert[2] = Vector4f(modelMin.x, modelMax.y, modelMin.z, 1);
    bvert[3] = Vector4f(modelMin.x, modelMin.y, modelMax.z, 1);
    bvert[4] = Vector4f(modelMax.x, modelMax.y, modelMin.z, 1);
    bvert[5] = Vector4f(modelMax.x, modelMin.y, modelMax.z, 1);
    bvert[6] = Vector4f(modelMin.x, modelMax.y, modelMax.z, 1);
    bvert[7] = Vector4f(modelMax.x, modelMax.y, modelMax.z, 1);

    for (int i = 0; i < 8; i++) {
        Vector4f worldVert = mat.transform(bvert[i]);
        if (worldVert.x < bboxMin.x) bboxMin.x = worldVert.x;
        if (worldVert.x > bboxMax.x) bboxMax.x = worldVert.x;
        if (worldVert.y < bboxMin.y) bboxMin.y = worldVert.y;
        if (worldVert.y > bboxMax.y) bboxMax.y = worldVert.y;
        if (worldVert.z < bboxMin.z) bboxMin.z = worldVert.z;
        if (worldVert.z > bboxMax.z) bboxMax.z = worldVert.z;
    }

    TRACE("Box world bounds: (%g,%g,%g) - (%g,%g,%g)",
          bboxMin.x, bboxMin.y, bboxMin.z,
          bboxMax.x, bboxMax.y, bboxMax.z);
}

void 
FlowBox::Render(Volume *vol)
{
    TRACE("Rendering box %s", _name);

    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    Vector3f origin = vol->location();
    glTranslatef(origin.x, origin.y, origin.z);

    Vector3f scale = vol->getPhysicalScaling();
    glScalef(scale.x, scale.y, scale.z);

    Vector3f min, max;
    min.x = vol->xAxis.min();
    min.y = vol->yAxis.min();
    min.z = vol->zAxis.min();
    max.x = vol->xAxis.max();
    max.y = vol->yAxis.max();
    max.z = vol->zAxis.max();

    TRACE("box is %g,%g %g,%g %g,%g", 
          _sv.corner1.x, _sv.corner2.x,
          _sv.corner1.y, _sv.corner2.y,
          _sv.corner1.z, _sv.corner2.z);
    TRACE("world is %g,%g %g,%g %g,%g", 
          min.x, max.x, min.y, max.y, min.z, max.z);

    float x0, y0, z0, x1, y1, z1;
    x0 = y0 = z0 = 0.0f;
    x1 = y1 = z1 = 0.0f;
    if (max.x > min.x) {
        x0 = (_sv.corner1.x - min.x) / (max.x - min.x);
        x1 = (_sv.corner2.x - min.x) / (max.x - min.x);
    }
    if (max.y > min.y) {
        y0 = (_sv.corner1.y - min.y) / (max.y - min.y);
        y1 = (_sv.corner2.y - min.y) / (max.y - min.y);
    }
    if (max.z > min.z) {
        z0 = (_sv.corner1.z - min.z) / (max.z - min.z);
        z1 = (_sv.corner2.z - min.z) / (max.z - min.z);
    }
    TRACE("rendering box %g,%g %g,%g %g,%g", x0, x1, y0, y1, z0, z1);

    glColor4d(_sv.color.r, _sv.color.g, _sv.color.b, _sv.color.a);
    glLineWidth(_sv.lineWidth);
    glBegin(GL_LINE_LOOP); 
    {
        glVertex3d(x0, y0, z0);
        glVertex3d(x1, y0, z0);
        glVertex3d(x1, y1, z0);
        glVertex3d(x0, y1, z0);
    }
    glEnd();
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z1);
        glVertex3d(x1, y0, z1);
        glVertex3d(x1, y1, z1);
        glVertex3d(x0, y1, z1);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x0, y0, z0);
        glVertex3d(x0, y0, z1);
        glVertex3d(x0, y1, z1);
        glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
        glVertex3d(x1, y0, z0);
        glVertex3d(x1, y0, z1);
        glVertex3d(x1, y1, z1);
        glVertex3d(x1, y1, z0);
    }
    glEnd();

    glPopMatrix();
    glPopAttrib();

    assert(CheckGL(AT));
}

FlowCmd::FlowCmd(Tcl_Interp *interp, const char *name, Tcl_HashEntry *hPtr) :
    _interp(interp),
    _hashPtr(hPtr),
    _name(name),
    _dataPtr(NULL),
    _volPtr(NULL),
    _fieldPtr(NULL)
{
    memset(&_sv, 0, sizeof(FlowValues));
    _sv.sliceVisible = 1;
    _sv.tfPtr = NanoVis::getTransfunc("default");

    Tcl_InitHashTable(&_particlesTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&_boxTable, TCL_STRING_KEYS);

    _cmdToken = Tcl_CreateObjCommand(_interp, (char *)_name, 
                                     (Tcl_ObjCmdProc *)FlowInstObjCmd,
                                     this, FlowInstDeleteProc);
}

FlowCmd::~FlowCmd()
{
    Rappture::FreeSwitches(_switches, &_sv, 0);
    if (_hashPtr != NULL) {
        Tcl_DeleteHashEntry(_hashPtr);
    }
    if (_fieldPtr != NULL) {
        delete _fieldPtr;
    }
    if (_dataPtr != NULL) {
        delete _dataPtr;
    }
    if (_volPtr != NULL) {
        NanoVis::removeVolume(_volPtr);
        _volPtr = NULL;
    }

    FlowBox *boxPtr;
    FlowBoxIterator boxIter;
    for (boxPtr = FirstBox(&boxIter); boxPtr != NULL; 
         boxPtr = NextBox(&boxIter)) {
        boxPtr->disconnect();
        delete boxPtr;
    }
    FlowParticles *particlesPtr;
    FlowParticlesIterator partIter;
    for (particlesPtr = FirstParticles(&partIter); particlesPtr != NULL;
         particlesPtr = NextParticles(&partIter)) {
        particlesPtr->disconnect();
        delete particlesPtr;
    }
    Tcl_DeleteHashTable(&_particlesTable);
    Tcl_DeleteHashTable(&_boxTable);
}

void
FlowCmd::ResetParticles()
{
    FlowParticlesIterator iter;
    for (FlowParticles *particlesPtr = FirstParticles(&iter);
         particlesPtr != NULL;
         particlesPtr = NextParticles(&iter)) {
        particlesPtr->reset();
    }
}

void
FlowCmd::Advect()
{
    NvVectorField *fieldPtr = VectorField();
    fieldPtr->active(true);
    FlowParticlesIterator iter;
    for (FlowParticles *particlesPtr = FirstParticles(&iter);
         particlesPtr != NULL;
         particlesPtr = NextParticles(&iter)) {
        if (particlesPtr->visible()) {
            particlesPtr->advect();
        }
    }
}

void
FlowCmd::Render()
{
    _fieldPtr->active(true);
    _fieldPtr->render();
    FlowParticlesIterator iter;
    for (FlowParticles *particlesPtr = FirstParticles(&iter);
         particlesPtr != NULL; 
         particlesPtr = NextParticles(&iter)) {
        if (particlesPtr->visible()) {
            particlesPtr->render();
        }
    }
    TRACE("in Render before boxes %s", _name);
    RenderBoxes();
}

int
FlowCmd::CreateParticles(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_HashEntry *hPtr;
    int isNew;
    const char *particlesName = Tcl_GetString(objPtr);
    hPtr = Tcl_CreateHashEntry(&_particlesTable, particlesName, &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "particle injection plane \"",
                         particlesName, "\" already exists.", (char *)NULL);
        return TCL_ERROR;
    }
    particlesName = Tcl_GetHashKey(&_particlesTable, hPtr);
    FlowParticles *particlesPtr;
    particlesPtr = new FlowParticles(particlesName, hPtr);
    if (particlesPtr == NULL) {
        Tcl_AppendResult(interp, "can't allocate particle injection plane",
                         (char *)NULL);
        Tcl_DeleteHashEntry(hPtr);
        return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, particlesPtr);
    return TCL_OK;
}

int
FlowCmd::GetParticles(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                      FlowParticles **particlesPtrPtr)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&_particlesTable, Tcl_GetString(objPtr));
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find a particle injection plane \"",
                             Tcl_GetString(objPtr), "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    *particlesPtrPtr = (FlowParticles *)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}

FlowParticles *
FlowCmd::FirstParticles(FlowParticlesIterator *iterPtr) 
{
    iterPtr->hashPtr = Tcl_FirstHashEntry(&_particlesTable, 
                                          &iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    return (FlowParticles *)Tcl_GetHashValue(iterPtr->hashPtr);
}

FlowParticles *
FlowCmd::NextParticles(FlowParticlesIterator *iterPtr) 
{
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    iterPtr->hashPtr = Tcl_NextHashEntry(&iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    return (FlowParticles *)Tcl_GetHashValue(iterPtr->hashPtr);
}

int
FlowCmd::CreateBox(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_HashEntry *hPtr;
    int isNew;
    hPtr = Tcl_CreateHashEntry(&_boxTable, Tcl_GetString(objPtr), &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "box \"", Tcl_GetString(objPtr),
                         "\" already exists in flow \"", name(), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    const char *boxName;
    boxName = Tcl_GetHashKey(&_boxTable, hPtr);
    FlowBox *boxPtr;
    boxPtr = new FlowBox(boxName, hPtr);
    if (boxPtr == NULL) {
        Tcl_AppendResult(interp, "can't allocate box \"", boxName, "\"",
                         (char *)NULL);
        Tcl_DeleteHashEntry(hPtr);
        return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, boxPtr);
    return TCL_OK;
}

int
FlowCmd::GetBox(Tcl_Interp *interp, Tcl_Obj *objPtr, FlowBox **boxPtrPtr)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&_boxTable, Tcl_GetString(objPtr));
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find a box \"", 
                             Tcl_GetString(objPtr), "\" in flow \"", name(), "\"", 
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    *boxPtrPtr = (FlowBox *)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}

FlowBox *
FlowCmd::FirstBox(FlowBoxIterator *iterPtr) 
{
    iterPtr->hashPtr = Tcl_FirstHashEntry(&_boxTable, &iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    return (FlowBox *)Tcl_GetHashValue(iterPtr->hashPtr);
}

FlowBox *
FlowCmd::NextBox(FlowBoxIterator *iterPtr) 
{
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    iterPtr->hashPtr = Tcl_NextHashEntry(&iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    return (FlowBox *)Tcl_GetHashValue(iterPtr->hashPtr);
}

void
FlowCmd::InitializeParticles()
{
    FlowParticlesIterator iter;
    for (FlowParticles *particlesPtr = FirstParticles(&iter);
         particlesPtr != NULL;
         particlesPtr = NextParticles(&iter)) {
        particlesPtr->initialize();
    }
}

bool
FlowCmd::ScaleVectorField()
{
    if (_volPtr != NULL) {
        TRACE("from ScaleVectorField volId=%s", _volPtr->name());
        NanoVis::removeVolume(_volPtr);
        _volPtr = NULL;
    }
    float *vdata;
    vdata = GetScaledVector();
    if (vdata == NULL) {
        return false;
    }
    Volume *volPtr;
    volPtr = MakeVolume(vdata);
    delete [] vdata;
    if (volPtr == NULL) {
        return false;
    }
    _volPtr = volPtr;

    // Remove the associated vector field.
    if (_fieldPtr != NULL) {
        delete _fieldPtr;
    }
    _fieldPtr = new NvVectorField();
    if (_fieldPtr == NULL) {
        return false;
    }

    Vector3f scale = volPtr->getPhysicalScaling();
    Vector3f location = _volPtr->location();

    _fieldPtr->setVectorField(_volPtr,
                              location,
                              scale.x,
                              scale.y,
                              scale.z,
                              NanoVis::magMax);

    if (NanoVis::licRenderer != NULL) {
        NanoVis::licRenderer->
            setVectorField(_volPtr->textureID(),
                           location,
                           scale.x,
                           scale.y,
                           scale.z,
                           _volPtr->wAxis.max());
        SetCurrentPosition();
        SetAxis();
        SetActive();
    }

    if (NanoVis::velocityArrowsSlice != NULL) {
        NanoVis::velocityArrowsSlice->
            setVectorField(_volPtr->textureID(),
                           location,
                           scale.x,
                           scale.y,
                           scale.z,
                           _volPtr->wAxis.max());
        NanoVis::velocityArrowsSlice->axis(_sv.slicePos.axis);
        NanoVis::velocityArrowsSlice->slicePos(_sv.slicePos.value);
        NanoVis::velocityArrowsSlice->enabled(_sv.showArrows);
    }

    FlowParticlesIterator partIter;
    for (FlowParticles *particlesPtr = FirstParticles(&partIter);
         particlesPtr != NULL;
         particlesPtr = NextParticles(&partIter)) {
        particlesPtr->setVectorField(_volPtr,
                                     location,
                                     scale.x,
                                     scale.y,
                                     scale.z,
                                     _volPtr->wAxis.max());
    }
    return true;
}

void
FlowCmd::RenderBoxes()
{
    FlowBoxIterator iter;
    FlowBox *boxPtr;
    for (boxPtr = FirstBox(&iter); boxPtr != NULL; boxPtr = NextBox(&iter)) {
        TRACE("found box %s", boxPtr->name());
        if (boxPtr->visible()) {
            boxPtr->Render(_volPtr);
        }
    }
}

float *
FlowCmd::GetScaledVector()
{
    assert(_dataPtr->nComponents() == 3);
    size_t n = _dataPtr->nValues() / _dataPtr->nComponents() * 4;
    float *data = new float[n];
    if (data == NULL) {
        return NULL;
    }
    memset(data, 0, sizeof(float) * n);
    float *destPtr = data;
    const float *values = _dataPtr->values();
    for (size_t iz = 0; iz < _dataPtr->zNum(); iz++) {
        for (size_t iy = 0; iy < _dataPtr->yNum(); iy++) {
            for (size_t ix = 0; ix < _dataPtr->xNum(); ix++) {
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
FlowCmd::MakeVolume(float *data)
{
    Volume *volPtr;

    volPtr = NanoVis::loadVolume(_name,
                                 _dataPtr->xNum(),
                                 _dataPtr->yNum(), 
                                 _dataPtr->zNum(),
                                 4, data, 
                                 NanoVis::magMin, NanoVis::magMax, 0);
    volPtr->xAxis.setRange(_dataPtr->xMin(), _dataPtr->xMax());
    volPtr->yAxis.setRange(_dataPtr->yMin(), _dataPtr->yMax());
    volPtr->zAxis.setRange(_dataPtr->zMin(), _dataPtr->zMax());

    TRACE("min=%g %g %g max=%g %g %g mag=%g %g", 
          NanoVis::xMin, NanoVis::yMin, NanoVis::zMin,
          NanoVis::xMax, NanoVis::yMax, NanoVis::zMax,
          NanoVis::magMin, NanoVis::magMax);

    volPtr->disableCutplane(0);
    volPtr->disableCutplane(1);
    volPtr->disableCutplane(2);

    /* Initialize the volume with the previously configured values. */
    volPtr->transferFunction(_sv.tfPtr);
    volPtr->dataEnabled(_sv.showVolume);
    volPtr->twoSidedLighting(_sv.twoSidedLighting);
    volPtr->outline(_sv.showOutline);
    volPtr->opacityScale(_sv.opacity);
    volPtr->ambient(_sv.ambient);
    volPtr->diffuse(_sv.diffuse);
    volPtr->specularLevel(_sv.specular);
    volPtr->specularExponent(_sv.specularExp);
    volPtr->visible(_sv.showVolume);

    Vector3f volScaling = volPtr->getPhysicalScaling();
    Vector3f loc(volScaling);
    loc *= -0.5;
    volPtr->location(loc);

    Volume::updatePending = true;
    return volPtr;
}

static int
FlowDataFileOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    Rappture::Outcome result;
    
    const char *fileName;
    fileName = Tcl_GetString(objv[3]);
    TRACE("Flow loading data from file %s", fileName);

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

    TRACE("Flow Data Loading");

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
    TRACE("Flow Data Loading %d %d", nBytes, nComponents);
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
    TRACE("nx = %d ny = %d nz = %d", dataPtr->xNum(), dataPtr->yNum(), dataPtr->zNum());
    TRACE("x0 = %lg y0 = %lg z0 = %lg", dataPtr->xMin(), dataPtr->yMin(), dataPtr->zMin());
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
FlowCmd::GetRelativePosition(FlowPosition *posPtr)
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
FlowCmd::GetRelativePosition() 
{
    return FlowCmd::GetRelativePosition(&_sv.slicePos);
}

/* Static NanoVis class commands. */

void
NanoVis::InitFlows() 
{
    Tcl_InitHashTable(&flowTable, TCL_STRING_KEYS);
}

FlowCmd *
NanoVis::FirstFlow(FlowIterator *iterPtr) 
{
    iterPtr->hashPtr = Tcl_FirstHashEntry(&flowTable, &iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    return (FlowCmd *)Tcl_GetHashValue(iterPtr->hashPtr);
}

FlowCmd *
NanoVis::NextFlow(FlowIterator *iterPtr) 
{
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    iterPtr->hashPtr = Tcl_NextHashEntry(&iterPtr->hashSearch);
    if (iterPtr->hashPtr == NULL) {
        return NULL;
    }
    return (FlowCmd *)Tcl_GetHashValue(iterPtr->hashPtr);
}

int
NanoVis::GetFlow(Tcl_Interp *interp, Tcl_Obj *objPtr, FlowCmd **flowPtrPtr)
{
    Tcl_HashEntry *hPtr;
    hPtr = Tcl_FindHashEntry(&flowTable, Tcl_GetString(objPtr));
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find a flow \"", 
                             Tcl_GetString(objPtr), "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    *flowPtrPtr = (FlowCmd *)Tcl_GetHashValue(hPtr);
    return TCL_OK;
}

int
NanoVis::CreateFlow(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Tcl_HashEntry *hPtr;
    int isNew;
    const char *name;
    name = Tcl_GetString(objPtr);
    hPtr = Tcl_CreateHashEntry(&flowTable, name, &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "flow \"", name, "\" already exists.",
                         (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_CmdInfo cmdInfo;
    if (Tcl_GetCommandInfo(interp, name, &cmdInfo)) {
        Tcl_AppendResult(interp, "an another command \"", name, 
                         "\" already exists.", (char *)NULL);
        return TCL_ERROR;
    }
    FlowCmd *flowPtr;
    name = Tcl_GetHashKey(&flowTable, hPtr);
    flowPtr = new FlowCmd(interp, name, hPtr);
    if (flowPtr == NULL) {
        Tcl_AppendResult(interp, "can't allocate a flow object \"", name, 
                         "\"", (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_SetHashValue(hPtr, flowPtr);
    return TCL_OK;
}

void
NanoVis::DeleteFlows(Tcl_Interp *interp)
{
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
         flowPtr = NextFlow(&iter)) {
        flowPtr->disconnect();                /* Don't disrupt the hash walk */
        Tcl_DeleteCommand(interp, flowPtr->name());
    }
    Tcl_DeleteHashTable(&flowTable);
}

bool
NanoVis::MapFlows()
{
    flags &= ~MAP_FLOWS;
    TRACE("Enter");

    /* 
     * Step 1. Get the overall min and max magnitudes of all the 
     *         flow vectors.
     */
    magMin = DBL_MAX, magMax = -DBL_MAX;

    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
         flowPtr = NextFlow(&iter)) {
        double min, max;
        if (!flowPtr->isDataLoaded()) {
            continue;
        }
        Rappture::Unirect3d *dataPtr = flowPtr->data();
        min = dataPtr->magMin();
        max = dataPtr->magMax();
        if (min < magMin) {
            magMin = min;
        } 
        if (max > magMax) {
            magMax = max;
        }
        if (dataPtr->xMin() < xMin) {
            xMin = dataPtr->xMin();
        }
        if (dataPtr->yMin() < yMin) {
            yMin = dataPtr->yMin();
        }
        if (dataPtr->zMin() < zMin) {
            zMin = dataPtr->zMin();
        }
        if (dataPtr->xMax() > xMax) {
            xMax = dataPtr->xMax();
        }
        if (dataPtr->yMax() > yMax) {
            yMax = dataPtr->yMax();
        }
        if (dataPtr->zMax() > zMax) {
            zMax = dataPtr->zMax();
        }
    }

    TRACE("MapFlows magMin=%g magMax=%g", NanoVis::magMin, NanoVis::magMax);

    /* 
     * Step 2. Generate the vector field from each data set. 
     */
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
         flowPtr = NextFlow(&iter)) {
        if (!flowPtr->isDataLoaded()) {
            continue; // Flow exists, but no data has been loaded yet.
        }
        if (flowPtr->visible()) {
            flowPtr->InitializeParticles();
        }
        if (!flowPtr->ScaleVectorField()) {
            return false;
        }
        // FIXME: This doesn't work when there is more than one flow.
        licRenderer->setOffset(flowPtr->GetRelativePosition());
        velocityArrowsSlice->slicePos(flowPtr->GetRelativePosition());
    }
    AdvectFlows();
    return true;
}

void
NanoVis::GetFlowBounds(Vector3f& min,
                       Vector3f& max,
                       bool onlyVisible)
{
    TRACE("Enter");

    min.set(FLT_MAX, FLT_MAX, FLT_MAX);
    max.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    FlowCmd *flow;
    FlowIterator iter;
    for (flow = FirstFlow(&iter); flow != NULL; 
         flow = NextFlow(&iter)) {
        if (onlyVisible && !flow->visible())
            continue;
 #if 0  // Using volume bounds instead of these
        if (flow->isDataLoaded()) {
            Vector3f umin, umax;
            Rappture::Unirect3d *unirect = flow->data();
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
        FlowBox *box;
        FlowBoxIterator iter;
        for (box = flow->FirstBox(&iter); box != NULL;
             box = flow->NextBox(&iter)) {
            TRACE("found box %s", box->name());
            if (!onlyVisible || box->visible()) {
                Vector3f fbmin, fbmax;
                box->getWorldSpaceBounds(fbmin, fbmax,
                                         flow->getVolume());
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
}

void
NanoVis::RenderFlows()
{
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
         flowPtr = NextFlow(&iter)) {
        if ((flowPtr->isDataLoaded()) && (flowPtr->visible())) {
            flowPtr->Render();
        }
    }
    flags &= ~REDRAW_PENDING;
}

void
NanoVis::ResetFlows()
{
    FlowCmd *flowPtr;
    FlowIterator iter;

    if (licRenderer->active()) {
        NanoVis::licRenderer->reset();
    }
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
         flowPtr = NextFlow(&iter)) {
        if ((flowPtr->isDataLoaded()) && (flowPtr->visible())) {
            flowPtr->ResetParticles();
        }
    }
}    

void
NanoVis::AdvectFlows()
{
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = FirstFlow(&iter); flowPtr != NULL; 
         flowPtr = NextFlow(&iter)) {
        if ((flowPtr->isDataLoaded()) && (flowPtr->visible())) {
            flowPtr->Advect();
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
    FlowColor *colorPtr = (FlowColor *)(record + offset);

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
    colorPtr->r = values[0];
    colorPtr->g = values[1];
    colorPtr->b = values[2];
    colorPtr->a = values[3];
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
    FlowPoint *pointPtr = (FlowPoint *)(record + offset);
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
    pointPtr->x = values[0];
    pointPtr->y = values[1];
    pointPtr->z = values[2];
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
    TransferFunction *funcPtr;
    funcPtr = NanoVis::getTransfunc(Tcl_GetString(objPtr));
    if (funcPtr == NULL) {
        Tcl_AppendResult(interp, "transfer function \"", Tcl_GetString(objPtr),
                         "\" is not defined", (char*)NULL);
        return TCL_ERROR;
    }
    *funcPtrPtr = funcPtr;
    return TCL_OK;
}

static int
FlowConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->ParseSwitches(interp, objc - 2, objv + 2) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::eventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static int
FlowParticlesAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->CreateParticles(interp, objv[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    FlowParticles *particlesPtr;
    if (flowPtr->GetParticles(interp, objv[3], &particlesPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (particlesPtr->parseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
        delete particlesPtr;
        return TCL_ERROR;
    }
    particlesPtr->configure();
    NanoVis::eventuallyRedraw();
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

static int
FlowParticlesConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                         Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    FlowParticles *particlesPtr;
    if (flowPtr->GetParticles(interp, objv[3], &particlesPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (particlesPtr->parseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
        return TCL_ERROR;
    }
    particlesPtr->configure();
    NanoVis::eventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static int
FlowParticlesDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    int i;
    for (i = 3; i < objc; i++) {
        FlowParticles *particlesPtr;

        if (flowPtr->GetParticles(NULL, objv[i], &particlesPtr) == TCL_OK) {
            delete particlesPtr;
        }
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowParticlesNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    FlowParticlesIterator iter;
    FlowParticles *particlesPtr;
    for (particlesPtr = flowPtr->FirstParticles(&iter); particlesPtr != NULL; 
         particlesPtr = flowPtr->NextParticles(&iter)) {
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewStringObj(particlesPtr->name(), -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static Rappture::CmdSpec flowParticlesOps[] = {
    {"add",        1, FlowParticlesAddOp,        4, 0, "name ?switches?",},
    {"configure",  1, FlowParticlesConfigureOp,  4, 0, "name ?switches?",},
    {"delete",     1, FlowParticlesDeleteOp,     4, 0, "?name...?"},
    {"names",      1, FlowParticlesNamesOp,      3, 4, "?pattern?"},
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
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    if (flowPtr->CreateBox(interp, objv[3]) != TCL_OK) {
        return TCL_ERROR;
    }
    FlowBox *boxPtr;
    if (flowPtr->GetBox(interp, objv[3], &boxPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (boxPtr->ParseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
        delete boxPtr;
        return TCL_ERROR;
    }
    NanoVis::eventuallyRedraw();
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

static int
FlowBoxDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    int i;
    for (i = 3; i < objc; i++) {
        FlowBox *boxPtr;

        if (flowPtr->GetBox(NULL, objv[i], &boxPtr) == TCL_OK) {
            delete boxPtr;
        }
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static int
FlowBoxNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Obj *listObjPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    FlowBoxIterator iter;
    FlowBox *boxPtr;
    for (boxPtr = flowPtr->FirstBox(&iter); boxPtr != NULL; 
         boxPtr = flowPtr->NextBox(&iter)) {
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewStringObj(boxPtr->name(), -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
FlowBoxConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;

    FlowBox *boxPtr;
    if (flowPtr->GetBox(interp, objv[3], &boxPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (boxPtr->ParseSwitches(interp, objc - 4, objv + 4) != TCL_OK) {
        return TCL_ERROR;
    }
    NanoVis::eventuallyRedraw();
    return TCL_OK;
}

static Rappture::CmdSpec flowBoxOps[] = {
    {"add",        1, FlowBoxAddOp,        4, 0, "name ?switches?",},
    {"configure",  1, FlowBoxConfigureOp,  4, 0, "name ?switches?",},
    {"delete",     1, FlowBoxDeleteOp,     3, 0, "?name...?"},
    {"names",      1, FlowBoxNamesOp,      3, 0, "?pattern?"},
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
    tf = flowPtr->GetTransferFunction();
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
        NanoVis::MapFlows();
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
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    Tcl_Preserve(flowPtr);
    int result;
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(flowPtr);
    return result;
}

/**
 * \brief Deletes the command associated with the tree.
 *
 * This is called only when the command associated with the tree is destroyed.
 */
static void
FlowInstDeleteProc(ClientData clientData)
{
    FlowCmd *flowPtr = (FlowCmd *)clientData;
    delete flowPtr;
}

static int
FlowAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    if (NanoVis::CreateFlow(interp, objv[2]) != TCL_OK) {
        return TCL_ERROR;
    }
    FlowCmd *flowPtr;
    if (NanoVis::GetFlow(interp, objv[2], &flowPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (flowPtr->ParseSwitches(interp, objc - 3, objv + 3) != TCL_OK) {
        Tcl_DeleteCommand(interp, flowPtr->name());
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
    int i;

    for (i = 2; i < objc; i++) {
        FlowCmd *flowPtr;

        if (NanoVis::GetFlow(interp, objv[i], &flowPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        Tcl_DeleteCommand(interp, flowPtr->name());
    }
    NanoVis::eventuallyRedraw(NanoVis::MAP_FLOWS);
    return TCL_OK;
}

static int
FlowExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    bool value;
    FlowCmd *flowPtr;

    value = false;
    if (NanoVis::GetFlow(NULL, objv[2], &flowPtr) == TCL_OK) {
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
    NanoVis::ResetFlows();
    if (NanoVis::flags & NanoVis::MAP_FLOWS) {
        NanoVis::MapFlows();
    }
    int i;
    NanoVis::AdvectFlows();
    for (i = 0; i < nSteps; i++) {
        if (NanoVis::licRenderer->active()) {
            NanoVis::licRenderer->convolve();
        }
        NanoVis::AdvectFlows();
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
    FlowCmd *flowPtr;
    FlowIterator iter;
    for (flowPtr = NanoVis::FirstFlow(&iter); flowPtr != NULL; 
         flowPtr = NanoVis::NextFlow(&iter)) {
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewStringObj(flowPtr->name(), -1);
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
        NanoVis::MapFlows();
    }
    NanoVis::eventuallyRedraw();
    NanoVis::licRenderer->convolve();
    NanoVis::AdvectFlows();
    return TCL_OK;
}

static int
FlowResetOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    NanoVis::ResetFlows();
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
    Rappture::AVTranslate::VideoFormats *formatPtr = 
        (Rappture::AVTranslate::VideoFormats *)(record + offset);
    const char *string;
    char c; 

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'm') && (strcmp(string, "mpeg") == 0)) {
        *formatPtr =  Rappture::AVTranslate::MPEG1;
    } else if ((c == 't') && (strcmp(string, "theora") == 0)) {
        *formatPtr = Rappture::AVTranslate::THEORA;
    } else if ((c == 'm') && (strcmp(string, "mov") == 0)) {
        *formatPtr = Rappture::AVTranslate::QUICKTIME;
    } else {
        Tcl_AppendResult(interp, "bad video format \"", string,
                         "\": should be mpeg, theora, or mov", (char*)NULL);
    }
    return TCL_ERROR;
}

struct FlowVideoValues {
    float frameRate;         /**< Frame rate */
    int bitRate;             /**< Video bitrate */
    int width, height;       /**< Dimensions of video frame. */
    int nFrames;
    Rappture::AVTranslate::VideoFormats format;
};

static Rappture::SwitchParseProc VideoFormatSwitchProc;
static Rappture::SwitchCustom videoFormatSwitch = {
    VideoFormatSwitchProc, NULL, 0,
};

Rappture::SwitchSpec FlowCmd::videoSwitches[] = {
    {Rappture::SWITCH_INT, "-bitrate", "value",
     offsetof(FlowVideoValues, bitRate), 0},
    {Rappture::SWITCH_CUSTOM, "-format", "string",
     offsetof(FlowVideoValues, format), 0, 0, &videoFormatSwitch},
    {Rappture::SWITCH_FLOAT, "-framerate", "value",
     offsetof(FlowVideoValues, frameRate), 0},
    {Rappture::SWITCH_INT, "-height", "integer",
     offsetof(FlowVideoValues, height), 0},
    {Rappture::SWITCH_INT, "-numframes", "count",
     offsetof(FlowVideoValues, nFrames), 0},
    {Rappture::SWITCH_INT, "-width", "integer",
     offsetof(FlowVideoValues, width), 0},
    {Rappture::SWITCH_END}
};

static int
FlowVideoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    struct pollfd pollResults;
    int timeout;

    pollResults.fd = fileno(NanoVis::stdin);
    pollResults.events = POLLIN;

#define PENDING_TIMEOUT          10  /* milliseconds. */
    timeout = PENDING_TIMEOUT;

    FlowVideoValues values;
    const char *token;

    token = Tcl_GetString(objv[2]);
    values.frameRate = 25.0f;                // Default frame rate 25 fps
    values.bitRate = 6000000;                // Default video bit rate.
    values.width = NanoVis::winWidth;
    values.height = NanoVis::winHeight;
    values.nFrames = 100;
    values.format = Rappture::AVTranslate::MPEG1;
    if (Rappture::ParseSwitches(interp, FlowCmd::videoSwitches, 
                                objc - 3, objv + 3, &values, SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((values.width < 0) || (values.width > SHRT_MAX) || 
        (values.height < 0) || (values.height > SHRT_MAX)) {
        Tcl_AppendResult(interp, "bad dimensions for video", (char *)NULL);
        return TCL_ERROR;
    }
    if ((values.frameRate < 0.0f) || (values.frameRate > 30.0f)) {
        Tcl_AppendResult(interp, "bad frame rate.", (char *)NULL);
        return TCL_ERROR;
    }
    if (values.bitRate < 0) {
        Tcl_AppendResult(interp, "bad bit rate.", (char *)NULL);
        return TCL_ERROR;
    }
    if (NanoVis::licRenderer == NULL) {
        Tcl_AppendResult(interp, "no lic renderer.", (char *)NULL);
        return TCL_ERROR;
    }
    // Save the old dimensions of the offscreen buffer.
    int oldWidth, oldHeight;
    oldWidth = NanoVis::winWidth;
    oldHeight = NanoVis::winHeight;

    TRACE("FLOW started");

    Rappture::Outcome context;

    Rappture::AVTranslate movie(values.width, values.height,
                                values.bitRate, 
                                values.frameRate);
    char tmpFileName[200];
    sprintf(tmpFileName,"/tmp/flow%d.mpeg", getpid());
    if (!movie.init(context, tmpFileName)) {
        Tcl_AppendResult(interp, "can't initialized movie \"", tmpFileName, 
                         "\": ", context.remark(), (char *)NULL);
        return TCL_ERROR;
    }
    if ((values.width != oldWidth) || (values.height != oldHeight)) {
        // Resize to the requested size.
        NanoVis::resizeOffscreenBuffer(values.width, values.height);
    }
    // Now compute the line padding for the offscreen buffer.
    int pad = 0;
    if (( 3 * values.width) % 4 > 0) {
        pad = 4 - ((3* values.width) % 4);
    }
    NanoVis::ResetFlows();
    bool canceled = false;
    for (int i = 1; i <= values.nFrames; i++) {
        if (((i & 0xF) == 0) && (poll(&pollResults, 1, 0) > 0)) {
            /* If there's another command on stdin, that means the client is
             * trying to cancel this operation. */
            canceled = true;
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

        movie.append(context, NanoVis::screenBuffer, pad);
    }
    movie.done(context);
    TRACE("FLOW end");
    if (!canceled) {
        Rappture::Buffer data;

        /* FIXME: find a way to get the data from the movie object as a
         * void* */
        if (!data.load(context, tmpFileName)) {
            Tcl_AppendResult(interp, "can't load data from temporary file \"",
                             tmpFileName, "\": ", context.remark(), (char *)NULL);
            return TCL_ERROR;
        }

        char command[200];
        sprintf(command,"nv>image -type movie -token \"%s\" -bytes %lu\n", 
                token, (unsigned long)data.size());
        NanoVis::sendDataToClient(command, data.bytes(), data.size());
    }
    if ((values.width != oldWidth) || (values.height != oldHeight)) {
        NanoVis::resizeOffscreenBuffer(oldWidth, oldHeight);
    }
    NanoVis::ResetFlows();
    if (unlink(tmpFileName) != 0) {
        Tcl_AppendResult(interp, "can't unlink temporary movie file \"",
                         tmpFileName, "\": ", Tcl_PosixError(interp), (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
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
    {"delete",   1, FlowDeleteOp,  2, 0, "name...",},
    {"exists",   1, FlowExistsOp,  3, 3, "name",},
    {"goto",     1, FlowGotoOp,    3, 3, "nSteps",},
    {"names",    1, FlowNamesOp,   2, 3, "?pattern?",},
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
 *\brief This procedure is invoked to initialize the "tree" command.
 *
 * Side effects:
 *    Creates the new command and adds a new entry into a global Tcl
 *    associative array.
 */
int
FlowCmdInitProc(Tcl_Interp *interp)
{
    Tcl_CreateObjCommand(interp, "flow", FlowCmdProc, NULL, NULL);
    NanoVis::InitFlows();
    return TCL_OK;
}

#ifdef notdef

// Read the header of a vtk data file. Returns 0 if error.
bool 
VtkReadHeader()
{
    char *p, *endPtr;

    line = getline(&p, endPtr);
    if (line == endPtr) {
        vtkErrorMacro(<<"Premature EOF reading first line! " << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return false;
    }
    if (sscanf(line, "# vtk DataFile Version %s", version) != 1) {
        vtkErrorMacro(<< "Unrecognized file type: "<< line << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return false;
    }

    // Read title
    line = getline(&p, endPtr);
    if (line == endPtr) {
        vtkErrorMacro(<<"Premature EOF reading title! " << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return false;
    }
    if (_title != NULL) {
        delete [] _title;
    }
    _title = new char[strlen(line) + 1];
    strcpy(_title, line);

    // Read type
    line = getline(&p, endPtr);
    if (line == endPtr) {
        vtkErrorMacro(<<"Premature EOF reading file type!" << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return false;
    }
    word = GetWord(line, &endPtr);
    if (strncasecmp(word, "ascii", 5) == 0) {
        _fileType = VTK_ASCII;
    } else if (strcasecmp(word, "binary", 6) == 0) {
        _fileType = VTK_BINARY;
    } else {
        vtkErrorMacro(<< "Unrecognized file type: "<< line << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        _fileType = 0;
        return false;
    }

    // Read dataset type
    line = getline(&p, endPtr);
    if (line == endPtr) {
        vtkErrorMacro(<<"Premature EOF reading file type!" << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return false;
    }
    word = GetWord(line, &endPtr);
    if (strncasecmp(word, "dataset", 7) == 0) {
        // Read dataset type
        line = getline(&p, endPtr);
        if (line == endPtr) {
            // EOF
        }
        type = GetWord(line, &endPtr);
        if (strncasecmp(word, "structured_grid", 15) == 0) {
            vtkErrorMacro(<< "Cannot read dataset type: " << line);
            return 1;
        }
        // Read keyword and dimensions
        //
        while (!done) {
            if (!this->ReadString(line)) {
                break;
            }

            // Have to read field data because it may be binary.
            if (! strncmp(this->LowerCase(line), "field", 5)) {
                vtkFieldData* fd = this->ReadFieldData();
                fd->Delete(); 
            }

            if ( ! strncmp(this->LowerCase(line),"dimensions",10) ) {
                int ext[6];
                if (!(this->Read(ext+1) && 
                      this->Read(ext+3) && 
                      this->Read(ext+5))) {
                    vtkErrorMacro(<<"Error reading dimensions!");
                    this->CloseVTKFile ();
                    return 1;
                }
                // read dimensions, change to extent;
                ext[0] = ext[2] = ext[4] = 0;
                --ext[1];
                --ext[3];
                --ext[5];
                outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                             ext, 6);
                // That is all we wanted !!!!!!!!!!!!!!!
                this->CloseVTKFile();
                return 1;
            }
        }
    }

    float progress = this->GetProgress();
    this->UpdateProgress(progress + 0.5*(1.0 - progress));

    return 1;
}
#endif
