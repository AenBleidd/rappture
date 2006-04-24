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
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <vector>

#include "define.h"
#include "global.h"
#include "Camera.h"
#include "ConvexPolygon.h"
#include "TransferFunction.h"
#include "Mat4x4.h"
#include "Volume.h"
#include "PerfQuery.h"

using namespace std;

class VolumeRenderer{

private:
  Camera* cam;
  vector <Volume*> volume;	//array of volumes
  vector <TransferFunction*> tf;//array of corresponding transfer functions 
  vector <int> slice;		//array of corresponding number of slices
  vector <bool> render_bit;	//bits marking which volume(s) to render
  int n_volumes;

  //shading parameters
  float live_diffuse;
  float live_specular;

  //cg related
  CGcontext g_context;		//the Nvidia cg context 
  CGprogram m_one_volume_fprog;
  CGparameter m_vol_one_volume_param;
  CGparameter m_tf_one_volume_param;
  CGparameter m_mvi_one_volume_param;
  CGparameter m_mv_one_volume_param;
  CGparameter m_render_param_one_volume_param;

  CGprogram m_vert_std_vprog;
  CGparameter m_mvp_vert_std_param;
  CGparameter m_mvi_vert_std_param;

  void activate_one_volume_shader(int volume_index, int n_actual_slices);
  void deactivate_one_volume_shader();

  //draw bounding box
  void draw_bounding_box(float x0, float y0, float z0,
	                float x1, float y1, float z1,
	                float r, float g, float b, float line_width);

  void get_near_far_z(Mat4x4 mv, double &zNear, double &zFar);

public:
  VolumeRenderer();
  VolumeRenderer(Camera* cam, Volume* _vol, TransferFunction* _tf, CGcontext _context);
  ~VolumeRenderer();

  void add_volume(Volume* _vol, TransferFunction* _tf, int _slice); 
  						//add a volume and its transfer function
  						//we require a transfer function when a 
						//volume is added.
  void render(int volume_index);
  void set_specular(float val);
  void set_diffuse(float val);
};

#endif
