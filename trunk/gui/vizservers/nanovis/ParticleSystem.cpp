/*
 * ----------------------------------------------------------------------
 * ParticleSystem.cpp: particle system class
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

#include <stdio.h>
#include <assert.h>

#include "ParticleSystem.h"


ParticleSystem::ParticleSystem(int w, int h, CGcontext context, NVISid volume){

  psys_width = w;
  psys_height = h;

  psys_frame = 0;
  reborn = true;
  flip = true;
  max_life = 30;

  glGenFramebuffersEXT(2, psys_fbo);
  glGenTextures(2, psys_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);

  glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_NV, psys_tex[0], 0);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);

  glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[1]);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_NV, psys_tex[1], 0);

  CHECK_FRAMEBUFFER_STATUS();
  assert(glGetError()==0);

  //load related shaders
  m_g_context = context;

  m_pos_fprog = loadProgram(m_g_context, CG_PROFILE_FP30, CG_SOURCE, "./shaders/update_pos.cg");
  m_pos_timestep_param  = cgGetNamedParameter(m_pos_fprog, "timestep");
  m_vel_tex_param = cgGetNamedParameter(m_pos_fprog, "vel_tex");
  m_pos_tex_param = cgGetNamedParameter(m_pos_fprog, "pos_tex");
  cgGLSetTextureParameter(m_vel_tex_param, volume);

  fprintf(stderr, "init_psys\n");
}


ParticleSystem::~ParticleSystem(){
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);
  glDeleteTextures(1, psys_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);
  glDeleteTextures(1, psys_tex+1);

  glDeleteFramebuffersEXT(2, psys_fbo);
}


void ParticleSystem::initialize(Particle* data){
  glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
  glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, (float*)data);
  
  assert(glGetError()==0);

  flip = true;
  reborn = false;

  fprintf(stderr, "init particles\n");
}

void ParticleSystem::advect(){
   glDisable(GL_BLEND);
   
   if(flip){

     glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);

     glClear(GL_COLOR_BUFFER_BIT);

     glViewport(0, 0, psys_width, psys_height);
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     gluOrtho2D(0, psys_width, 0, psys_height);
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();

     cgGLBindProgram(m_pos_fprog);
     cgGLSetParameter1f(m_pos_timestep_param, 0.1);
     cgGLEnableTextureParameter(m_vel_tex_param);
     cgGLSetTextureParameter(m_pos_tex_param, psys_tex[0]);
     cgGLEnableTextureParameter(m_pos_tex_param);

     cgGLEnableProfile(CG_PROFILE_FP30);
     draw_quad(psys_width, psys_height, psys_width, psys_height);
     cgGLDisableProfile(CG_PROFILE_FP30);
   
     cgGLDisableTextureParameter(m_vel_tex_param);
     cgGLDisableTextureParameter(m_pos_tex_param);
   }
   else
   {

     glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[1]);

     glClear(GL_COLOR_BUFFER_BIT);

     glViewport(0, 0, psys_width, psys_height);
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     gluOrtho2D(0, psys_width, 0, psys_height);
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();

     cgGLBindProgram(m_pos_fprog);
     cgGLSetParameter1f(m_pos_timestep_param, 0.1);
     cgGLEnableTextureParameter(m_vel_tex_param);
     cgGLSetTextureParameter(m_pos_tex_param, psys_tex[1]);
     cgGLEnableTextureParameter(m_pos_tex_param);

     cgGLEnableProfile(CG_PROFILE_FP30);
     draw_quad(psys_width, psys_height, psys_width, psys_height);
     cgGLDisableProfile(CG_PROFILE_FP30);
   
     cgGLDisableTextureParameter(m_vel_tex_param);
     cgGLDisableTextureParameter(m_pos_tex_param);
   }

   assert(glGetError()==0);

   //soft_read_verts();

   //hard_read_verts();

   flip = (!flip);

   psys_frame++;
   if(psys_frame==max_life){
     psys_frame=0;
     reborn = true;
   }

   fprintf(stderr, "advect: %d ", psys_frame);
}
