/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvParticleRenderer.cpp: particle system class
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
#include <stdlib.h>

#include <R2/R2FilePath.h>

#include "NvParticleRenderer.h"
#include "Trace.h"

#define NV_32

NvParticleAdvectionShader* NvParticleRenderer::_advectionShader = NULL;

class NvParticleAdvectionShaderInstance {
public :
    NvParticleAdvectionShaderInstance() {
    }
    ~NvParticleAdvectionShaderInstance() {
	if (NvParticleRenderer::_advectionShader) {
	    delete NvParticleRenderer::_advectionShader;
	}
    }
};
NvParticleAdvectionShaderInstance shaderInstance;

NvParticleRenderer::NvParticleRenderer(int w, int h, CGcontext context) : 
    _particleSize(1.2),
    scale(1, 1, 1), 
    origin(0, 0, 0),
    _activate(false)
{
    psys_width = w;
    psys_height = h;

    psys_frame = 0;
    reborn = true;
    flip = true;

    _color.set(0.2, 0.2, 1.0, 1.0);
    max_life = 500;

    _slice_axis = 0;
    _slice_pos = 0.0;

    data = new Particle[w*h];
    memset(data, 0, sizeof(Particle) * w * h);

    m_vertex_array = new RenderVertexArray(psys_width*psys_height, 3, GL_FLOAT);

    assert(CheckGL(AT));

    glGenFramebuffersEXT(2, psys_fbo);
    glGenTextures(2, psys_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#ifdef NV_32
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
	psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, psys_width, psys_height, 
	0, GL_RGBA, GL_FLOAT, NULL);
#endif
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
	GL_TEXTURE_RECTANGLE_NV, psys_tex[0], 0);


    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[1]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef NV_32
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
	psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, psys_width, psys_height, 
	0, GL_RGBA, GL_FLOAT, NULL);
#endif
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
	GL_TEXTURE_RECTANGLE_NV, psys_tex[1], 0);
 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    glGenTextures(1, &initPosTex);
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, initPosTex);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef NV_32
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
	psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, NULL);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, psys_width, 
	psys_height, 0, GL_RGBA, GL_FLOAT, NULL);
#endif

    CHECK_FRAMEBUFFER_STATUS();

    //load related shaders
    /*
      m_g_context = context;

      m_pos_fprog = LoadCgSourceProgram(m_g_context, "update_pos.cg", 
	CG_PROFILE_FP30, NULL);
      m_pos_timestep_param  = cgGetNamedParameter(m_pos_fprog, "timestep");
      m_vel_tex_param = cgGetNamedParameter(m_pos_fprog, "vel_tex");
      m_pos_tex_param = cgGetNamedParameter(m_pos_fprog, "pos_tex");
      m_scale_param = cgGetNamedParameter(m_pos_fprog, "scale");
      cgGLSetTextureParameter(m_vel_tex_param, volume);
      cgGLSetParameter3f(m_scale_param, scale_x, scale_y, scale_z);
    */
    if (_advectionShader == NULL) {
	_advectionShader = new NvParticleAdvectionShader();
    }
}

NvParticleRenderer::~NvParticleRenderer()
{
    glDeleteTextures(1, &initPosTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);
    glDeleteTextures(1, psys_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);
    glDeleteTextures(1, psys_tex+1);

    glDeleteFramebuffersEXT(2, psys_fbo);

    delete m_vertex_array;
    delete [] data;
}

void NvParticleRenderer::initializeDataArray()
{
    size_t n = psys_width * psys_height * 4;
    memset(data, 0, sizeof(float)* n);

    int index;
    bool particle;
    float* p = (float*) data;
    for (int i=0; i<psys_width; i++) {
        for (int j=0; j<psys_height; j++) {
            index = i + psys_height*j;
            particle = (rand() % 256) > 150; 
            if(particle) {
                //assign any location (x,y,z) in range [0,1]
		switch (_slice_axis) {
		case 0 :
		    p[4*index] = _slice_pos;
		    p[4*index+1]= j/float(psys_height);
		    p[4*index+2]= i/float(psys_width);
		    break;
		case 1 :
		    p[4*index]= j/float(psys_height);
		    p[4*index+1] = _slice_pos;
		    p[4*index+2]= i/float(psys_width);
		    break;
		case 2 :
		    p[4*index]= j/float(psys_height);
		    p[4*index+1]= i/float(psys_width);
		    p[4*index+2] = _slice_pos;
		    break;
		default :
		    p[4*index] = 0;
		    p[4*index+1]= 0;
		    p[4*index+2]= 0;
		    p[4*index+3]= 0;
		}
		
		//shorter life span, quicker iterations
               	p[4*index+3]= rand() / ((float) RAND_MAX) * 0.5  + 0.5f; 
            } else {
	    	p[4*index] = 0;
	    	p[4*index+1]= 0;
	    	p[4*index+2]= 0;
	    	p[4*index+3]= 0;
	    }
        }
    }
}

void NvParticleRenderer::initialize()
{
    initializeDataArray();

    //also store the data on main memory for next initialization
    //memcpy(data, p, psys_width*psys_height*sizeof(Particle));

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
    // I need to find out why GL_FLOAT_RGBA32_NV doesn't work
#ifdef NV_32
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
		 psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, (float*)data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, psys_width, 
		 psys_height, 0, GL_RGBA, GL_FLOAT, (float*)data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, 0);
  
    flip = true;
    reborn = false;

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, initPosTex);
    // I need to find out why GL_FLOAT_RGBA32_NV doesn't work
#ifdef NV_32
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
		 psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, (float*)data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, psys_width, psys_height, 
		 0, GL_RGBA, GL_FLOAT, (float*)p);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, 0);

    TRACE("init particles\n");
}

void NvParticleRenderer::reset()
{
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
#ifdef NV_32
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
		 psys_width, psys_height, 0, GL_RGBA, GL_FLOAT, (float*)data);
#else
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, psys_width, psys_height, 
		 0, GL_RGBA, GL_FLOAT, (float*)data);
#endif
    glBindTexture(GL_TEXTURE_RECTANGLE_NV, 0);
    flip = true;
    reborn = false;
    psys_frame = 0;
}

void 
NvParticleRenderer::advect()
{
    if (reborn) 
        reset();
    
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
   
    if (flip) {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClear(GL_COLOR_BUFFER_BIT);
	
	glViewport(0, 0, psys_width, psys_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluOrtho2D(0, psys_width, 0, psys_height);
	glOrtho(0, psys_width, 0, psys_height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	_advectionShader->bind(psys_tex[0], initPosTex);
	
	//cgGLBindProgram(m_pos_fprog);
	//cgGLSetParameter1f(m_pos_timestep_param, 0.05);
	//cgGLEnableTextureParameter(m_vel_tex_param);
	//cgGLSetTextureParameter(m_pos_tex_param, psys_tex[0]);
	//cgGLEnableTextureParameter(m_pos_tex_param);
	//cgGLEnableProfile(CG_PROFILE_FP30);
	
	draw_quad(psys_width, psys_height, psys_width, psys_height);
	
	//cgGLDisableProfile(CG_PROFILE_FP30);
	//cgGLDisableTextureParameter(m_vel_tex_param);
	//cgGLDisableTextureParameter(m_pos_tex_param);
	glDisable(GL_TEXTURE_RECTANGLE_NV);
    } else {
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[1]);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClear(GL_COLOR_BUFFER_BIT);
	
	glViewport(0, 0, psys_width, psys_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluOrtho2D(0, psys_width, 0, psys_height);
	glOrtho(0, psys_width, 0, psys_height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	_advectionShader->bind(psys_tex[1], initPosTex);
	
	//cgGLBindProgram(m_pos_fprog);
	//cgGLSetParameter1f(m_pos_timestep_param, 0.05);
	//cgGLEnableTextureParameter(m_vel_tex_param);
	//cgGLSetTextureParameter(m_pos_tex_param, psys_tex[1]);
	//cgGLEnableTextureParameter(m_pos_tex_param);
	//cgGLEnableProfile(CG_PROFILE_FP30);
	
	draw_quad(psys_width, psys_height, psys_width, psys_height);
	//draw_quad(psys_width, psys_height, 1.0f, 1.0f);
	
	//cgGLDisableProfile(CG_PROFILE_FP30);
	//cgGLDisableTextureParameter(m_vel_tex_param);
	//cgGLDisableTextureParameter(m_pos_tex_param);
    }
    
    _advectionShader->unbind();
    
    //soft_read_verts();
    
    update_vertex_buffer();
    
    flip = (!flip);
    
    psys_frame++;
    if(psys_frame==max_life) {
	psys_frame=0;
//	reborn = true;
    }
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void 
NvParticleRenderer::update_vertex_buffer()
{
    m_vertex_array->Read(psys_width, psys_height);

    //m_vertex_array->LoadData(vert);     //does not work??
    //assert(glGetError()==0);
}

void 
NvParticleRenderer::render()
{ 
    display_vertices(); 
}

void 
NvParticleRenderer::draw_bounding_box(float x0, float y0, float z0,
				      float x1, float y1, float z1,
				      float r, float g, float b, 
				      float line_width)
{
    glPushMatrix();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glColor4d(r, g, b, 1.0);
    glLineWidth(line_width);
    
    glBegin(GL_LINE_LOOP); 
    {
	glVertex3d(x0, y0, z0);
	glVertex3d(x1, y0, z0);
	glVertex3d(x1, y1, z0);
	glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x0, y0, z1);
	glVertex3d(x1, y0, z1);
	glVertex3d(x1, y1, z1);
	glVertex3d(x0, y1, z1);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x0, y0, z0);
	glVertex3d(x0, y0, z1);
	glVertex3d(x0, y1, z1);
	glVertex3d(x0, y1, z0);
    }
    glEnd();
    
    glBegin(GL_LINE_LOOP);
    {
	glVertex3d(x1, y0, z0);
	glVertex3d(x1, y0, z1);
	glVertex3d(x1, y1, z1);
	glVertex3d(x1, y1, z0);
    }
    glEnd();

    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
}

void 
NvParticleRenderer::display_vertices()
{
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_COLOR_MATERIAL);

    glPushMatrix();

    glTranslatef(origin.x, origin.y, origin.z);
    glScaled(scale.x, scale.y, scale.z);

// TBD...
/*
    draw_bounding_box(
    	0, 0, 0, 
    	1, 1, 1, 
    	1, 1, 1, 2);

    draw_bounding_box(
    	0, 0.5f / 4.5f, 0.5f / 4.5,
    	1, 4.0f / 4.5f, 4.0f / 4.5,
    	1, 0, 0, 2);

    draw_bounding_box(
    	1/3.0f, 1.0f / 4.5f, 0.5f / 4.5,
    	2/3.0f, 3.5f / 4.5f, 3.5f / 4.5,
    	1, 1, 0, 2);
*/

    glPointSize(_particleSize);
    //glColor4f(.2,.2,.8,1.);
    glColor4f(_color.x, _color.y, _color.z, _color.w);
    glEnableClientState(GL_VERTEX_ARRAY);
    m_vertex_array->SetPointer(0);
    glDrawArrays(GL_POINTS, 0, psys_width*psys_height);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
  
    glDisable(GL_DEPTH_TEST);

    //assert(glGetError()==0);
}

void 
NvParticleRenderer::setVectorField(unsigned int texID, const Vector3& ori, 
				   float scaleX, float scaleY, float scaleZ, 
				   float max)
{
    origin = ori;
    scale.set(scaleX, scaleY, scaleZ);
    _advectionShader->setScale(scale);
    _advectionShader->setVelocityVolume(texID, max);
}

void NvParticleRenderer::setAxis(int axis)
{
	_slice_axis = axis;
    	initializeDataArray();
}

void NvParticleRenderer::setPos(float pos)
{
	_slice_pos = pos;
    	initializeDataArray();
}


