/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "nanovis.h"

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

#include "NvVectorField.h"
#include "NvParticleRenderer.h"

using namespace vrmath;

NvVectorField::NvVectorField() :
    _vectorFieldId(0),
    _volPtr(NULL),
    _activated(true),
    _origin(0, 0, 0),
    _scaleX(1),
    _scaleY(1),
    _scaleZ(1),
    _max(1),
    _deviceVisible(false)
{
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
NvVectorField::setVectorField(Volume *volPtr, const Vector3f& origin, 
			      float scaleX, float scaleY, float scaleZ, 
			      float max)
{
    _volPtr = volPtr;
    _origin = origin;
    _scaleX = scaleX;
    _scaleY = scaleY;
    _scaleZ = scaleZ;
    _max = max;
    _vectorFieldId = volPtr->textureID();
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

void NvVectorField::setParticleColor(const std::string& name, const vrmath::Vector4f& color)
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
	if ((*iter).second) (*iter).second->setColor(vrmath::Vector4f(r,g,b,a));
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
    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    Vector3f origin = _volPtr->location();
    glTranslatef(origin.x, origin.y, origin.z);

    Vector3f scale = _volPtr->getPhysicalScaling();
    glScaled(scale.x, scale.y, scale.z);

    Vector3f min, max;
    min.x = _volPtr->xAxis.min();
    min.y = _volPtr->yAxis.min();
    min.z = _volPtr->zAxis.min();
    max.x = _volPtr->xAxis.max();
    max.y = _volPtr->yAxis.max();
    max.z = _volPtr->zAxis.max();

    std::map<std::string, NvDeviceShape>::iterator iterShape;

    for (iterShape = _shapeMap.begin(); iterShape != _shapeMap.end(); 
	 ++iterShape) {
	NvDeviceShape& shape = (*iterShape).second;

	if (!shape.visible) continue;

        float x0, y0, z0, x1, y1, z1;
        x0 = y0 = z0 = 0.0f;
        x1 = y1 = z1 = 0.0f;
        if (max.x > min.x) {
            x0 = (shape.min.x - min.x) / (max.x - min.x);
            x1 = (shape.max.x - min.x) / (max.x - min.x);
        }
        if (max.y > min.y) {
            y0 = (shape.min.y - min.y) / (max.y - min.y);
            y1 = (shape.max.y - min.y) / (max.y - min.y);
        }
        if (max.z > min.z) {
            z0 = (shape.min.z - min.z) / (max.z - min.z);
            z1 = (shape.max.z - min.z) / (max.z - min.z);
        }

        TRACE("rendering box %g,%g %g,%g %g,%g", x0, x1, y0, y1, z0, z1);

	glColor4d(shape.color.x, shape.color.y, shape.color.z, shape.color.w);
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
    glPopAttrib();
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
