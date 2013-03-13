/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NVFLOWVISRENDERER_H
#define NVFLOWVISRENDERER_H

#include <map>
#include <string>

#include <vrmath/Vector3f.h>
#include <vrmath/Vector4f.h>

#include "NvVectorField.h"

class NvParticleRenderer;

class NvFlowVisRenderer
{
public:
    NvFlowVisRenderer(int w, int h);

    ~NvFlowVisRenderer();

    void initialize();

    void initialize(const std::string& vfName);

    void advect();

    void reset();

    void render();

    void addVectorField(Volume *volPtr, const vrmath::Vector3f& ori, float scaleX, 
                        float scaleY, float scaleZ, float max);

    void addVectorField(const std::string& vfName, Volume *volPtr, const vrmath::Vector3f& ori,
                        float scaleX, float scaleY, float scaleZ, float max);

    void removeVectorField(const std::string& vfName);

    void addPlane(const std::string& vfName, const std::string& name);

    void removePlane(const std::string& vfName, const std::string& name);

    /** 
     * @brief Specify the axis of the index plane
     */
    void setPlaneAxis(const std::string& vfName, const std::string& name, int axis);

    /**
     * @param pos Specify the position of slice
     */
    void setPlanePos(const std::string& vfName, const std::string& name, float pos);

    void setParticleColor(const std::string& vfName, const std::string& name, const vrmath::Vector4f& color);

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

    bool active()
    {
	return _activated;
    }

    void active(bool state)
    {
	_activated = state;
    }

private:
    int _psys_width;
    int _psys_height;

    std::map<std::string, NvVectorField *> _vectorFieldMap;

    bool _activated;
};

#endif
