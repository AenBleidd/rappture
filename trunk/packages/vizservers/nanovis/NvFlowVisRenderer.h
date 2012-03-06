/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _NVFLOWVISRENDERER_H
#define  _NVFLOWVISRENDERER_H
#pragma once

#include "GL/glew.h"
#include "Cg/cgGL.h"

#include "define.h"
#include "config.h"
#include "global.h"

#include "Renderable.h"
#include "RenderVertexArray.h"
#include "Vector3.h"

#include <vector>
#include <map>
#include <string>

#include "NvParticleAdvectionShader.h"
#include "NvVectorField.h"

class NvParticleRenderer;

class NvFlowVisRenderer {
    enum {
	MAX_PARTICLE_RENDERER = 4,
    };

    int _psys_width;
    int _psys_height;
    CGcontext context;

    std::map<std::string, NvVectorField*> _vectorFieldMap;

    bool _activated;

    //NvLIC* licRenderer[3];
public:
    NvFlowVisRenderer(int w, int h, CGcontext context);
    ~NvFlowVisRenderer();

    void initialize();
    void initialize(const std::string& vfName);
    void advect();
    void reset();
    void render();

    void addVectorField(Volume* volPtr, const Vector3& ori, float scaleX, 
			float scaleY, float scaleZ, float max);

    void addVectorField(const std::string& vfName, Volume* volPtr, const Vector3& ori, float scaleX, float scaleY, float scaleZ, float max);
    void removeVectorField(const std::string& vfName);

    void addPlane(const std::string& vfName, const std::string& name);
    void removePlane(const std::string& vfName, const std::string& name);

    /** 
     * @brief Specify the axis of the index plane
     * @param index the index of the array of injection planes
     */
    void setPlaneAxis(const std::string& vfName, const std::string& name, int axis);

    /**
     * @param pos : Specify the position of slice
     */
    void setPlanePos(const std::string& vfName, const std::string& name, float pos);
    void setParticleColor(const std::string& vfName, const std::string& name, const Vector4& color);
    void activatePlane(const std::string& vfName, const std::string& name);
    void deactivatePlane(const std::string& vfName, const std::string& name);

    void activateVectorField(const std::string& vfName);
    void deactivateVectorField(const std::string& vfName);

    ////////////////////
    // DEVICE SHAPE
    void addDeviceShape(const std::string& vfName, const std::string& name, const NvDeviceShape& shape);
    void removeDeviceShape(const std::string& vfName, const std::string& name);
    void activateDeviceShape(const std::string& vfName);
    void deactivateDeviceShape(const std::string& vfName);
    void activateDeviceShape(const std::string& vfName, const std::string& name);
    void deactivateDeviceShape(const std::string& vfName, const std::string& name);
    bool active(void) {
	return _activated;
    }
    void active(bool state) {
	_activated = state;
    }
};


#endif /* _NVFLOWVISRENDERER_H */
