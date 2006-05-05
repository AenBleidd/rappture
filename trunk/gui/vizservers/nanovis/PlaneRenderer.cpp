/*
 * ----------------------------------------------------------------------
 * PlaneRenderer.cpp : PlaneRenderer class for volume visualization
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

#include "PlaneRenderer.h"

PlaneRenderer::PlaneRenderer(CGcontext _context, int _width, int _height):
  n_planes(0),
  g_context(_context),
  render_width(_width),
  render_height(_height),
  active_plane(-1)
{
  plane.clear();
  tf.clear();
  init_shaders();
}


PlaneRenderer::~PlaneRenderer(){}

//initialize the render shader
void PlaneRenderer::init_shaders(){
  
  //plane rendering shader
  m_fprog = loadProgram(g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/one_plane.cg");
  m_data_param = cgGetNamedParameter(m_fprog, "data");
  m_tf_param = cgGetNamedParameter(m_fprog, "tf");
  m_render_param = cgGetNamedParameter(m_fprog, "render_param");
}


int PlaneRenderer::add_plane(Texture2D* _p, TransferFunction* _tf){

  int ret = n_planes;

  plane.push_back(_p);
  tf.push_back(_tf);

  if(ret==0)
    active_plane = ret;

  n_planes++;
  return ret;
}


void PlaneRenderer::render(){
  if(n_planes == 0)
    return;

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  glViewport(0, 0, render_width, render_height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, render_width, 0, render_height);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  //glColor3f(1.,1.,1.);         //MUST HAVE THIS LINE!!!

  //if no active plane
  if(active_plane == -1)
    return;

  activate_shader(active_plane);
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2f(0, 0);
  glTexCoord2f(1, 0); glVertex2f(render_width, 0);
  glTexCoord2f(1, 1); glVertex2f(render_width, render_height);
  glTexCoord2f(0, 1); glVertex2f(0, render_height);
  glEnd();
  deactivate_shader();

}

void PlaneRenderer::activate_shader(int index){

  cgGLSetTextureParameter(m_data_param, plane[index]->id);
  cgGLSetTextureParameter(m_tf_param, tf[index]->id);
  cgGLEnableTextureParameter(m_data_param);
  cgGLEnableTextureParameter(m_tf_param);

  cgGLSetParameter4f(m_render_param, 0., 0., 0., 0.);

  cgGLBindProgram(m_fprog);
  cgGLEnableProfile(CG_PROFILE_FP30);
}


void PlaneRenderer::deactivate_shader(){
  cgGLDisableProfile(CG_PROFILE_FP30);
  cgGLDisableTextureParameter(m_data_param);
  cgGLDisableTextureParameter(m_tf_param);
}

void PlaneRenderer::set_active_plane(int index) { active_plane = index; }
void PlaneRenderer::set_screen_size(int w, int h) { render_width = w;  render_height = h;}

