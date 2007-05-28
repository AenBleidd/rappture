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
#include <malloc.h>
#include <string.h>

#include "ParticleSystem.h"


ParticleSystem::ParticleSystem(int w, int h, CGcontext context, NVISid volume, float scale_x, 
		float scale_y, float scale_z)
{

    fprintf(stderr, "%f, %f, %f\n", scale_x, scale_y, scale_z);
    scale = Vector3(scale_x, scale_y, scale_z);
  
  psys_width = w;
  psys_height = h;

  psys_frame = 0;
  reborn = true;
  flip = true;
  max_life = 500;

  data = (Particle*) malloc(w*h*sizeof(Particle));

  m_vertex_array = new RenderVertexArray(psys_width*psys_height, 3, GL_FLOAT);
  assert(glGetError()==0);

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
  /*
  m_g_context = context;

  m_pos_fprog = loadProgram(m_g_context, CG_PROFILE_FP30, CG_SOURCE, "/opt/nanovis/lib/shaders/update_pos.cg");
  m_pos_timestep_param  = cgGetNamedParameter(m_pos_fprog, "timestep");
  m_vel_tex_param = cgGetNamedParameter(m_pos_fprog, "vel_tex");
  m_pos_tex_param = cgGetNamedParameter(m_pos_fprog, "pos_tex");
  m_scale_param = cgGetNamedParameter(m_pos_fprog, "scale");
  cgGLSetTextureParameter(m_vel_tex_param, volume);
  cgGLSetParameter3f(m_scale_param, scale_x, scale_y, scale_z);
  */
  _advectionShader = new NvParticleAdvectionShader(scale);

  fprintf(stderr, "init_psys\n");
}

ParticleSystem::~ParticleSystem()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);
    glDeleteTextures(1, psys_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);
    glDeleteTextures(1, psys_tex+1);

    glDeleteFramebuffersEXT(2, psys_fbo);

    free(data);
}

void ParticleSystem::initialize(Particle* p)
{
    //also store the data on main memory for next initialization
    memcpy(data, p, psys_width*psys_height*sizeof(Particle));

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, (float*)p);
  
    assert(glGetError()==0);

    flip = true;
    reborn = false;

    //fprintf(stderr, "init particles\n");
}

void ParticleSystem::reset()
{
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, (float*)data);
  
    assert(glGetError()==0);

    flip = true;
    reborn = false;
    psys_frame = 0;
}


void ParticleSystem::advect()
{
    if (reborn) 
        reset();

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

        _advectionShader->bind(psys_tex[0]);
     
     /*
     cgGLBindProgram(m_pos_fprog);
     cgGLSetParameter1f(m_pos_timestep_param, 0.05);
     cgGLEnableTextureParameter(m_vel_tex_param);
     cgGLSetTextureParameter(m_pos_tex_param, psys_tex[0]);
     cgGLEnableTextureParameter(m_pos_tex_param);

     cgGLEnableProfile(CG_PROFILE_FP30);
     */
        draw_quad(psys_width, psys_height, psys_width, psys_height);
     /*
     cgGLDisableProfile(CG_PROFILE_FP30);
     cgGLDisableTextureParameter(m_vel_tex_param);
     cgGLDisableTextureParameter(m_pos_tex_param);
     */
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

        _advectionShader->bind(psys_tex[1]);

     /*
     cgGLBindProgram(m_pos_fprog);
     cgGLSetParameter1f(m_pos_timestep_param, 0.05);
     cgGLEnableTextureParameter(m_vel_tex_param);
     cgGLSetTextureParameter(m_pos_tex_param, psys_tex[1]);
     cgGLEnableTextureParameter(m_pos_tex_param);
     cgGLEnableProfile(CG_PROFILE_FP30);
     */

        draw_quad(psys_width, psys_height, psys_width, psys_height);

     /*
     cgGLDisableProfile(CG_PROFILE_FP30);
     cgGLDisableTextureParameter(m_vel_tex_param);
     cgGLDisableTextureParameter(m_pos_tex_param);
     */

    }

    _advectionShader->unbind();

   assert(glGetError()==0);

   //soft_read_verts();
   update_vertex_buffer();

    flip = (!flip);

    psys_frame++;
    if(psys_frame==max_life)
    {
        psys_frame=0;
        reborn = true;
    }

   //fprintf(stderr, "advect: %d ", psys_frame);
}

void ParticleSystem::update_vertex_buffer()
{
    m_vertex_array->Read(psys_width, psys_height);
  //m_vertex_array->LoadData(vert);     //does not work??
    assert(glGetError()==0);
}

void ParticleSystem::render()
{ 
    display_vertices(); 
}

void ParticleSystem::display_vertices()
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPointSize(1.2);
    glColor4f(.2,.2,.8,1.);
  
    glPushMatrix();
    glScaled(1./scale.x, 1./scale.y, 1./scale.z);

    m_vertex_array->SetPointer(0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_POINTS, 0, psys_width*psys_height);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
  
    glDisable(GL_DEPTH_TEST);
    assert(glGetError()==0);
}


