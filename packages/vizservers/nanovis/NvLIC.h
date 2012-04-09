/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvLIC.h: line integral convolution class
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
#ifndef NV_LIC_H
#define NV_LIC_H

#include <GL/glew.h>
#include <Cg/cg.h>

#include "config.h"
#include "nanovis.h"

#include "Renderable.h"
#include "Vector3.h"
#include "Volume.h"
#include "NvShader.h"

class NvLIC : public Renderable
{ 
public:
    NvLIC(int size, int width, int height, int axis, 
	  float offset);
    ~NvLIC();

    /// project 3D vectors to a 2D slice for line integral convolution
    void convolve();

    /// Display the convolution result
    void render();

    void makePatterns();

    void makeMagnitudes();

    void getVelocity(float x, float y, float *px, float *py);

    void getSlice();

    void setOffset(float v);

    /** 
     * @brief Specify the perpendicular axis
     *
     * 0 : x axis<br>
     * 1 : y axis<br>
     * 2 : z axis<br>
     */
    void setAxis(int axis);

    void setVectorField(unsigned int texID, const Vector3& origin,
                        float scaleX, float scaleY, float scaleZ, float max);

    void reset();

    void visible(bool state)
    {
	_isHidden = !state;
    }

    bool visible() const
    {
	return (!_isHidden);
    }

    void active(bool state)
    {
	_activate = state;
    }

    bool active() const
    {
	return _activate;
    }

private:
    /**
     * @brief the normal vector of the NvLIC plane, 
     * the inherited Vector3 location is its center
     */
    Vector3 _normal;

    int _width, _height;
    int _size;				// The lic is a square of size, it can
					// be stretched
    float *_sliceVector;		// Storage for the per slice vectors
					// driving the follow.
    Vector3 _scale;			// Scaling factor stretching the lic
					// plane to fit the actual dimensions
    Vector3 _origin;
    float _offset;			// [0,1] offset of _axis plane
    int _axis;

    //some convolve variables. They can pretty much stay fixed
    int _iframe;
    int _Npat;
    int _alpha;
    float _sa;
    float _tmax;
    float _dmax;
    float _max;

    GLuint _disListID;

    NvShader *_renderVelShader;
    //CG shader parameters
    CGparameter _velTexParam;
    CGparameter _velTexParamRenderVel;
    CGparameter _planeNormalParamRenderVel;
    CGparameter _maxParam;
 
    GLuint _colorTex, _patternTex, _magTex;
    GLuint _fbo, _velFbo, _sliceVectorTex;  // For projecting 3d vector to 2d
					    // vector on a plane.
    GLuint _vectorFieldId;

    Volume *_vectorField; 
    /**
     * flag for rendering
     */
    bool _activate;
    bool _isHidden;			// Indicates if LIC plane is displayed.
};

#endif
