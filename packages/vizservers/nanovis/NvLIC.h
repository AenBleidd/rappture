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


#ifndef _NV_LIC_H_
#define _NV_LIC_H_

#include "GL/glew.h"
#include "Cg/cgGL.h"

#include "define.h"
#include "config.h"
#include "global.h"

#include "Renderable.h"
#include "Vector3.h"
#include "Volume.h"

#define NPN 256   //resolution of background pattern
#define NMESH 256 //resolution of flow mesh
#define DM ((float) (1.0/(NMESH-1.0))) //distance in world coords between mesh lines
#define NPIX  512 //display size  
#define SCALE 3.0 //scale for background pattern. small value -> fine texture

class NvLIC : public Renderable { 

private:
    GLuint disListID;

    int width, height;
    int size;				// The lic is a square of size, it can
					// be stretched
    float* slice_vector;		// Storage for the per slice vectors
					// driving the follow.
    Vector3 scale;			// Scaling factor stretching the lic
					// plane to fit the actual dimensions
    Vector3 origin;
    Vector3 offset;			// [0,1] offset could be x, y, or z
					// direction
    int axis;

    //some convolve variables. They can pretty much stay fixed
    int iframe;
    int Npat;
    int alpha;
    float sa;
    float tmax;
    float dmax;
    float max;
    
    //CG shader parameters
    CGcontext m_g_context;
    CGparameter m_vel_tex_param;
    CGparameter m_vel_tex_param_render_vel, m_plane_normal_param_render_vel;
    CGprogram m_render_vel_fprog;
    CGparameter m_max_param;
    
    GLuint color_tex, pattern_tex, mag_tex;
    GLuint fbo, vel_fbo, slice_vector_tex;  // For projecting 3d vector to 2d
					    // vector on a plane.
    GLuint _vectorFieldId;

    Volume* _vectorField; 
    /**
     * flag for rendering
     */
    bool _activate;
    bool _isHidden;			// Indicates if LIC plane is displayed.
public:
    Vector3 normal; //the normal vector of the NvLIC plane, 
    //the inherited Vector3 location is its center
    NvLIC(int _size, int _width, int _height, int axis, 
	  const Vector3& _offset, CGcontext _context);
    ~NvLIC();

    /**
     * @brief project 3D vectors to a 2D slice for line integral convolution
     */
    void convolve();

    void display(void);			// Display the convolution result
    void render(void);
    void make_patterns();
    void make_magnitudes();
    void get_velocity(float x, float y, float* px, float* py);
    void get_slice();
    void set_offset(float v);
    /** 
     * @brief Specify the perdicular axis
     * @brief 0 : x axis
     * @brief 1 : y axis
     * @brief 2 : z axis
     */
    void set_axis(int axis);

    void setVectorField(unsigned int texID, const Vector3& ori, float scaleX, float scaleY, float scaleZ, float max);

    void reset(void);
    void visible(bool state) {
	_isHidden = !state;
    }
    bool visible(void) {
	return (!_isHidden);
    }
    void active(bool state) {
	_activate = state;
    }
    bool active(void) {
	return _activate;
    }
};


#endif
