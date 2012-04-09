/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvFlowVisRenderer.cpp: particle system class
 *
 * ======================================================================
 *  AUTHOR:  Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include "NvFlowVisRenderer.h"

#define NV_32

NvFlowVisRenderer::NvFlowVisRenderer(int w, int h) :
    _activated(true)
{
    _psys_width = w;
    _psys_height = h;
}

NvFlowVisRenderer::~NvFlowVisRenderer()
{
    std::map<std::string, NvVectorField *>::iterator iter;
    for (iter = _vectorFieldMap.begin(); iter != _vectorFieldMap.end(); ++iter) {
        delete (*iter).second;
    }
}

void 
NvFlowVisRenderer::initialize()
{
    std::map<std::string, NvVectorField *>::iterator iter;
    for (iter = _vectorFieldMap.begin(); iter != _vectorFieldMap.end(); ++iter) {
        (*iter).second->initialize();
    }
}

void 
NvFlowVisRenderer::reset()
{
    std::map<std::string, NvVectorField *>::iterator iter;
    for (iter = _vectorFieldMap.begin(); iter != _vectorFieldMap.end(); ++iter) {
        (*iter).second->reset();
    }
}

void 
NvFlowVisRenderer::initialize(const std::string& vfName)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
        if ((*iter).second) {
            (*iter).second->initialize();
        }
    }
}

void 
NvFlowVisRenderer::advect()
{
    std::map<std::string, NvVectorField *>::iterator iter;
    for (iter = _vectorFieldMap.begin(); iter != _vectorFieldMap.end(); ++iter) {
        if ((*iter).second && (*iter).second->active())
            (*iter).second->advect();
    }
}

void 
NvFlowVisRenderer::render()
{ 
    std::map<std::string, NvVectorField *>::iterator iter;
    for (iter = _vectorFieldMap.begin(); iter != _vectorFieldMap.end(); ++iter) {
        if ((*iter).second && (*iter).second->active())
            (*iter).second->render();
    }
}

void 
NvFlowVisRenderer::addVectorField(const std::string& vfName, Volume* volPtr,
                                  const Vector3& ori,
                                  float scaleX, float scaleY, float scaleZ, float max)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second) {
	    ((*iter).second)->setVectorField(volPtr, ori, scaleX, scaleY, scaleZ, max);
	} else {
	    NvVectorField *vf = new NvVectorField();
	    _vectorFieldMap[vfName] = vf;
	    vf->setVectorField(volPtr, ori, scaleX, scaleY, scaleZ, max);
	}
    } else {
	NvVectorField *vf = new NvVectorField();
	_vectorFieldMap[vfName] = vf;
	vf->setVectorField(volPtr, ori, scaleX, scaleY, scaleZ, max);
    }
}

void 
NvFlowVisRenderer::activateVectorField(const std::string& vfName)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->active(true);
    }
}

void 
NvFlowVisRenderer::deactivateVectorField(const std::string& vfName)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->active(false);
    }
}

void 
NvFlowVisRenderer::activatePlane(const std::string& vfName, const std::string& name)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
        if ((*iter).second)
            (*iter).second->activatePlane(name);
    }
}

void 
NvFlowVisRenderer::deactivatePlane(const std::string& vfName, const std::string& name)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
        if ((*iter).second)
            (*iter).second->deactivatePlane(name);
    }
}

void
NvFlowVisRenderer::setPlaneAxis(const std::string& vfName, const std::string& planeName, int axis)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->setPlaneAxis(planeName, axis);
    }
}

void
NvFlowVisRenderer::setPlanePos(const std::string& vfName, const std::string& name, float pos)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->setPlanePos(name, pos);
    }
}

void
NvFlowVisRenderer::setParticleColor(const std::string& vfName, const std::string& name, const Vector4& color)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->setParticleColor(name, color);
    }
}

void 
NvFlowVisRenderer::removeVectorField(const std::string& vfName)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	delete (*iter).second;
	_vectorFieldMap.erase(iter);
    }
}

void 
NvFlowVisRenderer::addPlane(const std::string& vfName, const std::string& name)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->addPlane(name);
    }
}

void 
NvFlowVisRenderer::removePlane(const std::string& vfName, const std::string& name)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->removePlane(name);
    }
}

void 
NvFlowVisRenderer::activateDeviceShape(const std::string& vfName, const std::string& name)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->activateDeviceShape(name);
    }
}

void 
NvFlowVisRenderer::deactivateDeviceShape(const std::string& vfName, const std::string& name)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->deactivateDeviceShape(name);
    }
}

void 
NvFlowVisRenderer::activateDeviceShape(const std::string& vfName)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->activateDeviceShape();
    }
}

void 
NvFlowVisRenderer::deactivateDeviceShape(const std::string& vfName)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->deactivateDeviceShape();
    }
}

void 
NvFlowVisRenderer::addDeviceShape(const std::string& vfName, const std::string& name, const NvDeviceShape& shape)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->addDeviceShape(name, shape);
    }
}

void 
NvFlowVisRenderer::removeDeviceShape(const std::string& vfName, const std::string& name)
{
    std::map<std::string, NvVectorField *>::iterator iter = _vectorFieldMap.find(vfName);
    if (iter != _vectorFieldMap.end()) {
	if ((*iter).second)
            (*iter).second->removeDeviceShape(name);
    }
}
