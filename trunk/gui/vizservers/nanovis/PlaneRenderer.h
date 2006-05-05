/*
 * ----------------------------------------------------------------------
 * PlaneRenderer.h : PlaneRenderer class for 2D visualization
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

#ifndef _PLANE_RENDERER_H_
#define _PLANE_RENDERER_H_

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
#include "TransferFunction.h"
#include "Texture2D.h"

using namespace std;

class PlaneRenderer{

private:
  vector <Texture2D*> plane;	 //array of volumes
  vector <TransferFunction*> tf; //array of corresponding transfer functions 
  int active_plane;		 //the active plane, only one is rendered
  int n_planes;

  int render_width;	//render size
  int render_height;	

  //cg related
  CGcontext g_context;		//the Nvidia cg context 
  CGprogram m_fprog;
  CGparameter m_data_param;
  CGparameter m_tf_param;
  CGparameter m_render_param;

  void init_shaders();
  void activate_shader(int plane_index);
  void deactivate_shader();

public:
  PlaneRenderer(CGcontext _context, int width, int height);
  ~PlaneRenderer();

  int add_plane(Texture2D* _p, TransferFunction* _tf);
  						//add a plane and its transfer function
  						//we require a transfer function when a 
						//plane is added.
  void set_active_plane(int index);		//set the active plane to be rendered
  void render();
};

#endif
