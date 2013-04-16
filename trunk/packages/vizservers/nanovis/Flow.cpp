/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 *   Insoo Woo <iwoo@purdue.edu>
 *   George A. Howlett <gah@purdue.edu>
 *   Leif Delgass <ldelgass@purdue.edu>
 */

#include <float.h>

#include <tcl.h>

#include <vrmath/BBox.h>
#include <vrmath/Vector3f.h>

#include "nanovis.h"
#include "Flow.h"
#include "FlowCmd.h"
#include "FlowTypes.h"
#include "FlowParticles.h"
#include "FlowBox.h"
#include "LIC.h"
#include "VelocityArrowsSlice.h"
#include "Switch.h"
#include "Unirect.h"
#include "Volume.h"
#include "Trace.h"
#include "TransferFunction.h"

using namespace nv;
using namespace vrmath;

bool Flow::updatePending = false;
double Flow::magMin = DBL_MAX;
double Flow::magMax = -DBL_MAX;

Flow::Flow(Tcl_Interp *interp, const char *name) :
    _interp(interp),
    _name(name),
    _data(NULL),
    _volume(NULL)
{
    memset(&_sv, 0, sizeof(FlowValues));
    _sv.sliceVisible = 1;
    _sv.transferFunction = NanoVis::getTransferFunction("default");

    _cmdToken = Tcl_CreateObjCommand(_interp, (char *)name, 
                                     (Tcl_ObjCmdProc *)FlowInstObjCmd,
                                     this, FlowInstDeleteProc);
}

Flow::~Flow()
{
    TRACE("Enter");

    Rappture::FreeSwitches(_switches, &_sv, 0);

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
Flow::getBounds(Vector3f& min,
                Vector3f& max,
                bool onlyVisible)
{
    TRACE("Enter");

    if (onlyVisible && !visible())
        return;

    BBox allBounds;
    if (isDataLoaded()) {
        BBox bbox;
        data()->getBounds(bbox.min, bbox.max);
        allBounds.extend(bbox);
    }
    for (BoxHashmap::iterator itr = _boxTable.begin();
         itr != _boxTable.end(); ++itr) {
        FlowBox *box = itr->second;
        if (!onlyVisible || box->visible()) {
            BBox bbox;
            box->getBounds(bbox.min, bbox.max);
            allBounds.extend(bbox);
        }
    }
    min = allBounds.min;
    max = allBounds.max;
}

float
Flow::getRelativePosition(FlowPosition *position)
{
    if (position->flags == RELPOS) {
        return position->value;
    }
    switch (position->axis) {
    case AXIS_X:  
        return (position->value - _data->xMin()) / 
            (_data->xMax() - _data->xMin()); 
    case AXIS_Y:  
        return (position->value - _data->yMin()) / 
            (_data->yMax() - _data->yMin()); 
    case AXIS_Z:  
        return (position->value - _data->zMin()) / 
            (_data->zMax() - _data->zMin()); 
    }
    return 0.0;
}

void
Flow::resetParticles()
{
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        itr->second->reset();
    }
}

void
Flow::advect()
{
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        if (itr->second->visible()) {
            itr->second->advect();
        }
    }
}

void
Flow::render()
{
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        if (itr->second->visible()) {
            itr->second->render();
        }
    }
    renderBoxes();
}

FlowParticles *
Flow::createParticles(const char *particlesName)
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
Flow::getParticles(const char *particlesName)
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
Flow::deleteParticles(const char *particlesName)
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
Flow::getParticlesNames(std::vector<std::string>& names)
{
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        names.push_back(std::string(itr->second->name()));
    }
}

FlowBox *
Flow::createBox(const char *boxName)
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
Flow::getBox(const char *boxName)
{
    BoxHashmap::iterator itr = _boxTable.find(boxName);
    if (itr == _boxTable.end()) {
        TRACE("Can't find box '%s' in '%s'", boxName, name());
        return NULL;
    }
    return itr->second;
}

void
Flow::deleteBox(const char *boxName)
{
    BoxHashmap::iterator itr = _boxTable.find(boxName);
    if (itr == _boxTable.end()) {
        TRACE("Can't find box '%s' in '%s'", boxName, name());
        return;
    }
    delete itr->second;
    _boxTable.erase(itr);
}

void Flow::getBoxNames(std::vector<std::string>& names)
{
    for (BoxHashmap::iterator itr = _boxTable.begin();
         itr != _boxTable.end(); ++itr) {
        names.push_back(std::string(itr->second->name()));
    }
}

void
Flow::initializeParticles()
{
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        itr->second->initialize();
    }
}

bool
Flow::configure()
{
    bool needReset = false;

    if (_volume != NULL) {
        _volume->transferFunction(_sv.transferFunction);
        _volume->dataEnabled(_sv.showVolume);
        _volume->twoSidedLighting(_sv.twoSidedLighting);
        _volume->outline(_sv.showOutline);
        _volume->opacityScale(_sv.opacity);
        _volume->ambient(_sv.ambient);
        _volume->diffuse(_sv.diffuse);
        _volume->specularLevel(_sv.specular);
        _volume->specularExponent(_sv.specularExp);
    }

    float slicePos = getRelativePosition(&_sv.slicePos);

    // FIXME: LIC and arrows should be per-flow
    if (NanoVis::licRenderer != NULL) {
        if (NanoVis::licRenderer->getSliceAxis() != _sv.slicePos.axis) {
            needReset = true;
            NanoVis::licRenderer->setSliceAxis(_sv.slicePos.axis);
        }
        if (NanoVis::licRenderer->getSlicePosition() != slicePos) {
            needReset = true;
            NanoVis::licRenderer->setSlicePosition(slicePos);
        }
        NanoVis::licRenderer->visible(_sv.sliceVisible);
    }
    if (NanoVis::velocityArrowsSlice != NULL) {
        NanoVis::velocityArrowsSlice->setSliceAxis(_sv.slicePos.axis);
        NanoVis::velocityArrowsSlice->setSlicePosition(slicePos);
        NanoVis::velocityArrowsSlice->visible(_sv.showArrows);
    }

    return needReset;
}

bool
Flow::scaleVectorField()
{
    float *vdata = getScaledVector();
    if (_volume != NULL) {
        TRACE("Updating existing volume: %s", _volume->name());
        _volume->setData(vdata, magMin, magMax, 0);
    } else {
        _volume = makeVolume(vdata);
        if (_volume == NULL) {
            return false;
        }
    }
    delete [] vdata;
    // FIXME: LIC and arrows should be per-flow
    if (NanoVis::licRenderer != NULL) {
        NanoVis::licRenderer->setVectorField(_volume);
        NanoVis::licRenderer->setSliceAxis(_sv.slicePos.axis);
        NanoVis::licRenderer->setSlicePosition(getRelativePosition(&_sv.slicePos));
        NanoVis::licRenderer->visible(_sv.sliceVisible);
    }
    if (NanoVis::velocityArrowsSlice != NULL) {
        NanoVis::velocityArrowsSlice->setVectorField(_volume);
        NanoVis::velocityArrowsSlice->setSliceAxis(_sv.slicePos.axis);
        NanoVis::velocityArrowsSlice->setSlicePosition(getRelativePosition(&_sv.slicePos));
        NanoVis::velocityArrowsSlice->visible(_sv.showArrows);
    }
    for (ParticlesHashmap::iterator itr = _particlesTable.begin();
         itr != _particlesTable.end(); ++itr) {
        itr->second->setVectorField(_volume);
    }
    return true;
}

void
Flow::renderBoxes()
{
    for (BoxHashmap::iterator itr = _boxTable.begin();
         itr != _boxTable.end(); ++itr) {
        if (itr->second->visible()) {
            itr->second->render(_volume);
        }
    }
}

float *
Flow::getScaledVector()
{
    assert(_data->nComponents() == 3);
    size_t n = _data->nValues() / _data->nComponents() * 4;
    float *data = new float[n];
    memset(data, 0, sizeof(float) * n);
    float *dest = data;
    const float *values = _data->values();
    for (size_t iz = 0; iz < _data->zNum(); iz++) {
        for (size_t iy = 0; iy < _data->yNum(); iy++) {
            for (size_t ix = 0; ix < _data->xNum(); ix++) {
                double vx, vy, vz, vm;
                vx = values[0];
                vy = values[1];
                vz = values[2];
                vm = sqrt(vx*vx + vy*vy + vz*vz);
                dest[0] = vm / magMax;
                dest[1] = vx /(2.0 * magMax) + 0.5;
                dest[2] = vy /(2.0 * magMax) + 0.5;
                dest[3] = vz /(2.0 * magMax) + 0.5;
                values += 3;
                dest += 4;
            }
        }
    }
    return data;
}

Volume *
Flow::makeVolume(float *data)
{
    Volume *volume =
        NanoVis::loadVolume(_name.c_str(),
                            _data->xNum(),
                            _data->yNum(), 
                            _data->zNum(),
                            4, data, 
                            magMin, magMax, 0);
    volume->xAxis.setRange(_data->xMin(), _data->xMax());
    volume->yAxis.setRange(_data->yMin(), _data->yMax());
    volume->zAxis.setRange(_data->zMin(), _data->zMax());

    TRACE("mag=%g %g", magMin, magMax);

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

    Volume::updatePending = true;
    return volume;
}
