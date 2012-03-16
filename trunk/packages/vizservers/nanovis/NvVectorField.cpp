/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "nanovis.h"

#include "NvVectorField.h"
#include "NvParticleRenderer.h"

NvVectorField::NvVectorField() :
    _vectorFieldId(0),
    _activated(true),
    _scaleX(1),
    _scaleY(1),
    _scaleZ(1),
    _max(1)
{
    _deviceVisible = false;
    _volPtr = 0;
    _physicalSize.set(1.0f, 1.0f, 1.0f);
}

NvVectorField::~NvVectorField()
{
    std::map<std::string, NvParticleRenderer *>::iterator iter;
    for (iter = _particleRendererMap.begin(); 
	 iter != _particleRendererMap.end(); iter++) {
	delete (*iter).second;
    }
}

void 
NvVectorField::setVectorField(Volume *volPtr, const Vector3& ori, 
			      float scaleX, float scaleY, float scaleZ, 
			      float max)
{
    _volPtr = volPtr;
    _origin = ori;
    _scaleX = scaleX;
    _scaleY = scaleY;
    _scaleZ = scaleZ;
    _max = max;
    _vectorFieldId = volPtr->id;
    _physicalMin = volPtr->getPhysicalBBoxMin();
    TRACE("_pysicalMin %f %f %f\n", _physicalMin.x, _physicalMin.y, _physicalMin.z);
    _physicalSize = volPtr->getPhysicalBBoxMax() - _physicalMin;
    TRACE("_pysicalSize %f %f %f\n", 
	   _physicalSize.x, _physicalSize.y, _physicalSize.z);
}

void 
NvVectorField::addDeviceShape(const std::string& name, 
			      const NvDeviceShape& shape)
{
    _shapeMap[name] = shape;
}

void NvVectorField::removeDeviceShape(const std::string& name)
{
    std::map<std::string, NvDeviceShape>::iterator iter = _shapeMap.find(name);
    if (iter != _shapeMap.end()) _shapeMap.erase(iter);
}

void NvVectorField::initialize()
{
    std::map<std::string, NvParticleRenderer *>::iterator iter;
    for (iter = _particleRendererMap.begin(); 
	 iter != _particleRendererMap.end(); iter++) {
	if ((*iter).second) (*iter).second->initialize();
    }
}

void NvVectorField::reset()
{
    std::map<std::string, NvParticleRenderer *>::iterator iter;
    for (iter = _particleRendererMap.begin(); 
	 iter != _particleRendererMap.end(); iter++) {
	if ((*iter).second) (*iter).second->reset();
    }
}

void NvVectorField::setPlaneAxis(const std::string& name, int axis)
{
    std::map<std::string, NvParticleRenderer *>::iterator iter = _particleRendererMap.find(name);
    if (iter != _particleRendererMap.end()) {
	(*iter).second->setAxis(axis);
    }
    
}

void NvVectorField::setPlanePos(const std::string& name, float pos)
{
    std::map<std::string, NvParticleRenderer *>::iterator iter = _particleRendererMap.find(name);
    if (iter != _particleRendererMap.end()) {
	(*iter).second->setPos(pos);
    }
}

void NvVectorField::addPlane(const std::string& name)
{
    std::map<std::string, NvParticleRenderer *>::iterator iter = _particleRendererMap.find(name);
    NvParticleRenderer *renderer = NULL;
    if (iter != _particleRendererMap.end()) {
	if ((*iter).second != 0) {
	    renderer = (*iter).second;
	} else {
	    renderer = (*iter).second = new NvParticleRenderer(NMESH, NMESH);
	}
    } else {
	renderer = new NvParticleRenderer(NMESH, NMESH);
	_particleRendererMap[name] = renderer;
    }

    renderer->setVectorField(_vectorFieldId, _origin, _scaleX, _scaleY, _scaleZ, _max);
    if (renderer) {
	renderer->initialize();
    }
}

void NvVectorField::removePlane(const std::string& name)
{
    std::map<std::string, NvParticleRenderer* >::iterator iter = _particleRendererMap.find(name);
    if (iter != _particleRendererMap.end()) {
	delete (*iter).second;
	_particleRendererMap.erase(iter);
    }
}

void NvVectorField::activatePlane(const std::string& name)
{
    std::map<std::string, NvParticleRenderer *>::iterator iter = _particleRendererMap.find(name);
    if (iter != _particleRendererMap.end()) {
	(*iter).second->active(true);
    }
}

void NvVectorField::deactivatePlane(const std::string& name)
{
    std::map<std::string, NvParticleRenderer *>::iterator iter = _particleRendererMap.find(name);
    if (iter != _particleRendererMap.end()) {
	(*iter).second->active(false);
    }
}

void NvVectorField::setParticleColor(const std::string& name, const Vector4& color)
{
    std::map<std::string, NvParticleRenderer *>::iterator iter = _particleRendererMap.find(name);
    if (iter != _particleRendererMap.end()) {
	(*iter).second->setColor(color);
    }
}

void NvVectorField::setParticleColor(const std::string& name, float r, float g, float b, float a)
{
    std::map<std::string, NvParticleRenderer *>::iterator iter = _particleRendererMap.find(name);
    if (iter != _particleRendererMap.end()) {
	if ((*iter).second) (*iter).second->setColor(Vector4(r,g,b,a));
    }
}

void NvVectorField::advect()
{
    std::map<std::string, NvParticleRenderer *>::iterator iter;
    for (iter = _particleRendererMap.begin(); 
	 iter != _particleRendererMap.end(); ++iter) {
	if ((*iter).second && (*iter).second->active()) 
	    (*iter).second->advect();
    }
}

void NvVectorField::render()
{
    std::map<std::string, NvParticleRenderer *>::iterator iter;
    for (iter = _particleRendererMap.begin(); 
	 iter != _particleRendererMap.end(); ++iter) {
	if ((*iter).second && (*iter).second->active()) {
	    (*iter).second->render();
	}
    }
    if (_deviceVisible) {
	drawDeviceShape();
    }
}

void 
NvVectorField::drawDeviceShape()
{
    glPushMatrix();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    float x0, y0, z0, x1, y1, z1;
    std::map<std::string, NvDeviceShape>::iterator iterShape;
    
    glPushMatrix();
    glTranslatef(_origin.x, _origin.y, _origin.z);
    glScaled(_scaleX, _scaleY, _scaleZ);
    for (iterShape = _shapeMap.begin(); iterShape != _shapeMap.end(); 
	 ++iterShape) {
	NvDeviceShape& shape = (*iterShape).second;
	
	if (!shape.visible) continue;
	
	
	glColor4d(shape.color.x, shape.color.y, shape.color.z, shape.color.w);
#if 0
	x0 = _physicalMin.x + (shape.min.x - _physicalMin.x) / _physicalSize.x;
	y0 = _physicalMin.y + (shape.min.y - _physicalMin.y) / _physicalSize.y;
	z0 = _physicalMin.z + (shape.min.z - _physicalMin.z) / _physicalSize.z;
	x1 = _physicalMin.x + (shape.max.x - _physicalMin.x) / _physicalSize.x;
	y1 = _physicalMin.y + (shape.max.y - _physicalMin.y) / _physicalSize.y;
	z1 = _physicalMin.z + (shape.max.z - _physicalMin.z) / _physicalSize.z;
#endif
	x0 = (shape.min.x - _physicalMin.x) / _physicalSize.x;
	y0 = (shape.min.y - _physicalMin.y) / _physicalSize.y;
	z0 = (shape.min.z - _physicalMin.z) / _physicalSize.z;
	x1 = (shape.max.x - _physicalMin.x) / _physicalSize.x;
	y1 = (shape.max.y - _physicalMin.y) / _physicalSize.y;
	z1 = (shape.max.z - _physicalMin.z) / _physicalSize.z;
	
	glLineWidth(1.2);
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
    }
    glPopMatrix();
    
    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}

void 
NvVectorField::activateDeviceShape(const std::string& name)
{
    std::map<std::string, NvDeviceShape>::iterator iter = _shapeMap.find(name);
    if (iter != _shapeMap.end()) {
	(*iter).second.visible = true;
    }
}

void 
NvVectorField::deactivateDeviceShape(const std::string& name)
{
    std::map<std::string, NvDeviceShape>::iterator iter = _shapeMap.find(name);
    if (iter != _shapeMap.end()) {
	(*iter).second.visible = false;
    }
}
