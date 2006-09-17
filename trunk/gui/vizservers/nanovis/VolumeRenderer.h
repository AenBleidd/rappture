/*
 * ----------------------------------------------------------------------
 * VolumeRenderer.h : VolumeRenderer class for volume visualization
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

#ifndef _VOLUME_RENDERER_H_
#define _VOLUME_RENDERER_H_

#include <GL/glew.h>
#include <Cg/cgGL.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <vector>

#include "define.h"
#include "global.h"
#include "ConvexPolygon.h"
#include "TransferFunction.h"
#include "Mat4x4.h"
#include "Volume.h"
#include "ZincBlendeVolume.h"
#include "PerfQuery.h"

using namespace std;

class VolumeRenderer{

private:
  vector <Volume*> volume;	//array of volumes
  vector <TransferFunction*> tf;//array of corresponding transfer functions 
  int n_volumes;

  bool slice_mode;	//enable cut planes
  bool volume_mode;	//enable full volume rendering


  //cg related
  CGcontext g_context;	//the Nvidia cg context 

  //shader parameters for rendering a single cubic volume
  CGprogram m_one_volume_fprog;
  CGparameter m_vol_one_volume_param;
  CGparameter m_tf_one_volume_param;
  CGparameter m_mvi_one_volume_param;
  CGparameter m_mv_one_volume_param;
  CGparameter m_render_param_one_volume_param;


  //Shader parameters for rendering a single zincblende orbital.
  //A simulation contains S, P, D and SS, total of 4 orbitals. A full rendering requires 4 zincblende orbital volumes.
  //A zincblende orbital volume is decomposed into two "interlocking" cubic volumes and passed to the shader. 
  //We render each orbital with its own independent transfer functions then blend the result.
  //
  //The engine is already capable of rendering multiple volumes and combine them. Thus, we just invoke this shader on
  //S, P, D and SS orbitals with different transfor functions. The result is a multi-orbital rendering.
  //This is equivalent to rendering 4 unrelated data sets occupying the same space.
  CGprogram m_zincblende_volume_fprog;
  CGparameter m_zincblende_tf_param;  //four transfer functions
  CGparameter m_zincblende_volume_a_param, m_zincblende_volume_b_param; //two cubic volumes (one zincblende orbital)
  CGparameter m_zincblende_cell_size_param; //cell size in texture space
  CGparameter m_zincblende_mvi_param; //modelview inverse matrix
  CGparameter m_zincblende_render_param; //render parameters
  
  
  //standard vertex shader parameters
  CGprogram m_vert_std_vprog;
  CGparameter m_mvp_vert_std_param;
  CGparameter m_mvi_vert_std_param;

  void init_shaders();
  void activate_volume_shader(int volume_index, bool slice_mode);
  void deactivate_volume_shader();

  //draw bounding box
  void draw_bounding_box(float x0, float y0, float z0,
	                float x1, float y1, float z1,
	                float r, float g, float b, float line_width);

  void get_near_far_z(Mat4x4 mv, double &zNear, double &zFar);

  void init_font(char* filename);
  GLuint font_texture; 				//the id of the font texture
  void glPrint(char* string, int set);		//there are two sets of font in the texture. 0, 1
  void draw_label(int volume_index);		//draw label using bitmap texture
  GLuint font_base;				//the base of the font display list
  void build_font();				//register the location of each alphabet in the texture

public:
  VolumeRenderer(CGcontext _context);
  ~VolumeRenderer();

  int add_volume(Volume* _vol, TransferFunction* _tf); 
  						//add a volume and its transfer function
  						//we require a transfer function when a 
						//volume is added.
  void shade_volume(Volume* _vol, TransferFunction* _tf); 
  TransferFunction* get_volume_shading(Volume* _vol); 

  void render(int volume_index);
  void render_all();	//render all enabled volumes;
  void set_specular(float val);
  void set_diffuse(float val);
  void set_slice_mode(bool val); //control independently.
  void set_volume_mode(bool val);
  void switch_slice_mode(); //switch_cutplane_mode
  void switch_volume_mode();
  void enable_volume(int index); //enable a volume
  void disable_volume(int index); //disable a volume
};

#endif
