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
  int width, height;
  int size; 		//the lic is a square of size, it can be stretched
  float* slice_vector;  //storage for the per slice vectors driving the follow
  Vector3 scale;	//scaling factor stretching the lic plane to fit the actual dimensions
  float offset;		//[0,1] offset could be x, y, or z direction

  //some convolve variables. They can pretty much stay fixed
  int iframe;
  int Npat;
  int alpha;
  float sa;
  float tmax;
  float dmax;

  //CG shader parameters
  CGcontext m_g_context;
  CGparameter m_vel_tex_param;
  CGparameter m_vel_tex_param_render_vel, m_plane_normal_param_render_vel;
  CGprogram m_render_vel_fprog;

  NVISid color_tex, pattern_tex, mag_tex;
  NVISid fbo, vel_fbo, slice_vector_tex;  //for projecting 3d vector to 2d vector on a plane

  Volume* vector_field; 

public:
  Vector3 normal; //the normal vector of the NvLIC plane, 
  		  //the inherited Vector3 location is its center
  NvLIC(int _size, int _width, int _height, float _offset,
	  CGcontext _context, NVISid _vector_field,
	  float scalex, float scaley, float scalez);
  ~NvLIC();

  void convolve();
  void display();	//display the convolution result
  void render();
  void make_patterns();
  void make_magnitudes();
  void get_velocity(float x, float y, float* px, float* py);
  void get_slice();
  void set_offset(float v);

};

#endif
