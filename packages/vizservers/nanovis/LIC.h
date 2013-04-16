/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 *  Authors:
 *    Wei Qiao <qiaow@purdue.edu>
 */
#ifndef NV_LIC_H
#define NV_LIC_H

#include <GL/glew.h>

#include <vrmath/Vector3f.h>

#include "FlowTypes.h"
#include "Volume.h"
#include "Shader.h"

namespace nv {

class LIC
{ 
public:
    LIC(FlowSliceAxis axis = AXIS_Z, float offset = 0.f);
    ~LIC();

    /// project 3D vectors to a 2D slice for line integral convolution
    void convolve();

    /// Display the convolution result
    void render();

    void setSlicePosition(float pos);

    float getSlicePosition() const
    {
        return _offset;
    }

    /** 
     * @brief Specify the perpendicular axis
     */
    void setSliceAxis(FlowSliceAxis axis);

    FlowSliceAxis getSliceAxis() const
    {
        return _axis;
    }

    void setVectorField(Volume *volume);

    void reset();

    void visible(bool state)
    {
	_visible = state;
    }

    bool visible() const
    {
	return _visible;
    }

private:
    void getVelocity(float x, float y, float *px, float *py);

    void getSlice();

    void makePatterns();

    void makeMagnitudes();

    /**
     * @brief the normal vector of the LIC plane, 
     * the inherited Vector3 location is its center
     */
    vrmath::Vector3f _normal;

    int _width, _height;
    int _size;			/**< The lic is a square of size, it can
                                   be stretched */
    float *_sliceVector;        /**< Storage for the per slice vectors
                                   driving the flow */
    vrmath::Vector3f _scale;             /**< Scaling factor stretching the lic
                                   plane to fit the actual dimensions */
    vrmath::Vector3f _origin;
    float _offset;              ///< [0,1] offset of slice plane
    FlowSliceAxis _axis;        ///< Axis normal to slice plane

    //some convolve variables. They can pretty much stay fixed
    int _iframe;
    int _Npat;
    int _alpha;
    float _sa;
    float _tmax;
    float _dmax;
    float _max;

    GLuint _disListID;

    Shader *_renderVelShader;

    GLuint _colorTex, _patternTex, _magTex;
    GLuint _fbo, _velFbo, _sliceVectorTex;  // For projecting 3d vector to 2d
					    // vector on a plane.
    GLuint _vectorFieldId;

    Volume *_vectorField; 
    /**
     * flag for rendering
     */
    bool _visible;			// Indicates if LIC plane is displayed.
};

}

#endif
