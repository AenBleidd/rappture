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

#include <tcl.h>

#include <vrmath/Vector3f.h>

#include "nanovis.h"
#include "Flow.h"
#include "FlowCmd.h"
#include "FlowTypes.h"
#include "FlowParticles.h"
#include "FlowBox.h"
#include "NvLIC.h"
#include "VelocityArrowsSlice.h"
#include "NvParticleRenderer.h"
#include "Switch.h"
#include "Unirect.h"
#include "Volume.h"
#include "Trace.h"
#include "TransferFunction.h"

using namespace vrmath;

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

float
Flow::getRelativePosition(FlowPosition *position)
{
    if (position->flags == RELPOS) {
        return position->value;
    }
    switch (position->axis) {
    case AXIS_X:  
        return (position->value - NanoVis::xMin) / 
            (NanoVis::xMax - NanoVis::xMin); 
    case AXIS_Y:  
        return (position->value - NanoVis::yMin) / 
            (NanoVis::yMax - NanoVis::yMin); 
    case AXIS_Z:  
        return (position->value - NanoVis::zMin) / 
            (NanoVis::zMax - NanoVis::zMin); 
    }
    return 0.0;
}

float
Flow::getRelativePosition() 
{
    return getRelativePosition(&_sv.slicePos);
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
Flow::scaleVectorField()
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

    Vector3f scale = volume->getPhysicalScaling();
    Vector3f location = _volume->location();

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
    if (data == NULL) {
        return NULL;
    }
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
                dest[0] = vm / NanoVis::magMax;
                dest[1] = vx /(2.0*NanoVis::magMax) + 0.5;
                dest[2] = vy /(2.0*NanoVis::magMax) + 0.5;
                dest[3] = vz /(2.0*NanoVis::magMax) + 0.5;
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
