/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvLIC.h: line integral convolution class
 *
 * ======================================================================
 *  AUTHOR:  Insoo Woo <iwoo@purdue.edu, Wei Qiao <qiaow@purdue.edu>
 *           Purdue Rendering and Perceptualization Lab (PURPL)
 *
 *  Copyright (c) 2004-2006  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */


#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <R2/R2FilePath.h>
#include "NvLIC.h"
#include <Trace.h>
#include "global.h"

NvLIC::NvLIC(int _size, int _width, int _height, int _axis, 
	     const Vector3& _offset, CGcontext _context) : 
    Renderable(Vector3(0.0f,0.0f,0.0f)),
    disListID(0),
    width(_width),
    height(_height),
    size(_size),
    scale(1.0f, 1.0f, 1.0f),
    offset(_offset),
    iframe(0),
    Npat(64),
    alpha((int) 0.12*255),
    tmax(NPIX/(SCALE*NPN)),
    dmax(SCALE/NPIX),
    max(1.0f),
    m_g_context(_context),
    _vectorFieldId(0),
    _activate(false)
{

    axis = _axis;
    slice_vector = new float[size*size*4];
    memset(slice_vector, 0, sizeof(float) * size * size * 4);

    origin.set(0, 0, 0);

    //initialize the pattern texture
    glGenTextures(1, &pattern_tex);
    glBindTexture(GL_TEXTURE_2D, pattern_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    //initialize frame buffer objects
    //render buffer for projecting 3D velocity onto a 2D plane
    glGenFramebuffersEXT(1, &vel_fbo);
    glGenTextures(1, &slice_vector_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, slice_vector_tex);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // ADD INSOO
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, 
		    GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, 
		    GL_CLAMP_TO_EDGE); 

    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, size, size, 
		0, GL_RGBA, GL_FLOAT, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                  GL_TEXTURE_RECTANGLE_NV, slice_vector_tex, 0);


    //render buffer for the convolution
    glGenFramebuffersEXT(1, &fbo);
    glGenTextures(1, &color_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    //initialize color texture for lic
    glBindTexture(GL_TEXTURE_2D, color_tex);
    // INSOO
    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0,
                 GL_RGB, GL_INT, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                            GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, color_tex, 0);


    // Check framebuffer completeness at the end of initialization.
    CHECK_FRAMEBUFFER_STATUS();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    m_render_vel_fprog = LoadCgSourceProgram(m_g_context, "render_vel.cg", 
	CG_PROFILE_FP30, "main");
    m_vel_tex_param_render_vel = cgGetNamedParameter(m_render_vel_fprog, 
	"vel_tex");
    m_plane_normal_param_render_vel = cgGetNamedParameter(m_render_vel_fprog, 
	"plane_normal");
    m_max_param = cgGetNamedParameter(m_render_vel_fprog, "vmax");


    make_patterns();
}

NvLIC::~NvLIC()
{
    glDeleteTextures(1, &pattern_tex);
    glDeleteTextures(1, &mag_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);
    glDeleteTextures(1, &slice_vector_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    glDeleteTextures(1, &color_tex);

    GLuint buffers[2] = {vel_fbo, fbo};
    glDeleteFramebuffersEXT(2, buffers);

    glDeleteLists(disListID, Npat);
    
/*
    TBD..
    cgDestroyParameter(m_vel_tex_param_render_vel);
    cgDestroyParameter(m_plane_normal_param_render_vel);
    cgDestroyParameter(m_max_param);
*/
    cgDestroyProgram(m_render_vel_fprog);

    delete [] slice_vector;
}

void 
NvLIC::make_patterns() 
{ 
    TRACE("begin make_patterns\n");
    if (disListID > 0) {
	glDeleteLists(disListID, Npat);
    }
    disListID = glGenLists(Npat);
    
    TRACE("DisplayList : %d\n", disListID);
    
    int lut[256];
    int phase[NPN][NPN];
    GLubyte pat[NPN][NPN][4];
    int i, j, k, t;
    
    for (i = 0; i < 256; i++) {
        lut[i] = i < 127 ? 0 : 255;
    }
    for (i = 0; i < NPN; i++) {
        for (j = 0; j < NPN; j++) {
            phase[i][j] = rand() >> 8; 
        }
    }
    for (k = 0; k < Npat; k++) {
        t = (k << 8) / Npat;
        for (i = 0; i < NPN; i++) {
            for (j = 0; j < NPN; j++) {
                pat[i][j][0] = pat[i][j][1] = pat[i][j][2] = 
                    lut[(t + phase[i][j]) % 255];
                pat[i][j][3] = alpha;
            }
        }
        glNewList(disListID + k, GL_COMPILE);
        glBindTexture(GL_TEXTURE_2D, pattern_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, 4, NPN, NPN, 0, GL_RGBA, 
                     GL_UNSIGNED_BYTE, pat);
        glEndList();
    }
    
    glBindTexture(GL_TEXTURE_2D, pattern_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, NPN, NPN, 0, 
                 GL_RGBA, GL_UNSIGNED_BYTE, pat);
    TRACE("finish make_patterns\n");
}


void NvLIC::make_magnitudes()
{
    GLubyte mag[NMESH][NMESH][4];

    //read vector filed
    for(int i=0; i<NMESH; i++){
        for(int j=0; j<NMESH; j++){
            float x=DM*i;
            float y=DM*j;
            
            float magnitude = sqrt(x*x+y*y)/1.414;
            
            //from green to red
            GLubyte r = (GLubyte) floor(magnitude*255);
            GLubyte g = 0;
            GLubyte b = 255 - r;
            GLubyte a = 122;
            
            mag[i][j][0] = r;
            mag[i][j][1] = g;
            mag[i][j][2] = b;
            mag[i][j][3] = a;
        }
    }
    glGenTextures(1, &mag_tex);
    glBindTexture(GL_TEXTURE_2D, mag_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NMESH, NMESH, 0, GL_RGBA, 
                 GL_UNSIGNED_BYTE, mag);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void 
NvLIC::get_slice()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);

    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, size, size);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, size, 0, size);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, _vectorFieldId);
    cgGLBindProgram(m_render_vel_fprog);

    cgGLSetTextureParameter(m_vel_tex_param_render_vel, _vectorFieldId);
    cgGLEnableTextureParameter(m_vel_tex_param_render_vel);
    cgGLSetParameter4f(m_plane_normal_param_render_vel, 1., 1., 0., 0);
    cgGLSetParameter1f(m_max_param, max);

    cgGLEnableProfile(CG_PROFILE_FP30);

    glBegin(GL_QUADS);
    {
	switch (axis) {
	case 0 :
	    // TBD..
            glTexCoord3f(offset.x, 0., 0.); glVertex2f(0.,   0.);
            glTexCoord3f(offset.x, 1., 0.); glVertex2f(size, 0.);
            glTexCoord3f(offset.x, 1., 1.); glVertex2f(size, size);
            glTexCoord3f(offset.x, 0., 1.); glVertex2f(0.,   size);
		break;
	case 1 :
	    // TBD..
            glTexCoord3f(0., offset.y, 0.); glVertex2f(0.,   0.);
            glTexCoord3f(1., offset.y, 0.); glVertex2f(size, 0.);
            glTexCoord3f(1., offset.y, 1.); glVertex2f(size, size);
            glTexCoord3f(0., offset.y, 1.); glVertex2f(0.,   size);
		break;
	case 2 :
            glTexCoord3f(0., 0., offset.z); glVertex2f(0.,   0.);
            glTexCoord3f(1., 0., offset.z); glVertex2f(size, 0.);
            glTexCoord3f(1., 1., offset.z); glVertex2f(size, size);
            glTexCoord3f(0., 1., offset.z); glVertex2f(0.,   size);
	    break;	
	}
    }
    glEnd();
    
    cgGLDisableProfile(CG_PROFILE_FP30);
   
    cgGLDisableTextureParameter(m_vel_tex_param_render_vel);

    glBindTexture(GL_TEXTURE_3D, 0);
    glDisable(GL_TEXTURE_3D);

    //read the vectors
    glReadPixels(0, 0, size, size, GL_RGBA, GL_FLOAT, slice_vector);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    int lim = size * size * 4;
    float* v= slice_vector;
    for (int i = 0; i < lim; ++i) {
        if (isnan(*v)) {
            *v = 0.0f;    
        }
        ++v;
    }
}

//line integral convolution
void 
NvLIC::convolve()
{
    if (_vectorFieldId == 0) {
	return;
    }

    int   i, j; 
    float x1, x2, y, px, py;
    
    glPushMatrix();
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity(); 
    glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT);

    /*glPushMatrix();*/
    glViewport(0, 0, (GLsizei) NPIX, (GLsizei) NPIX);
    //glTranslatef(-1.0, -1.0, 0.0); 
    //glScalef(2.0, 2.0, 1.0);
    glOrtho(0.0f, 1.0f, 0.0f, 1.0f, -10.0f, 10.0f);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //sa = 0.010*cos(iframe*2.0*M_PI/200.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pattern_tex);
    sa = 0.01;
    
    for (i = 0; i < NMESH-1; i++) {
        x1 = DM*i; x2 = x1 + DM;
        glBegin(GL_QUAD_STRIP);
        for (j = 0; j < NMESH-1; j++) {
            y = DM*j;
            glTexCoord2f(x1, y); 
            get_velocity(x1, y, &px, &py);
            glVertex2f(px, py);
            
            glTexCoord2f(x2, y); 
            get_velocity(x2, y, &px, &py); 
            glVertex2f(px, py);
        }
        glEnd();
    }
    iframe = iframe + 1;
    
    glEnable(GL_BLEND); 
    
    // INSOO ADD
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_TEXTURE_2D);
    glCallList(iframe % Npat + disListID);
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2f(0.0,  0.0);  glVertex2f(0.0, 0.0);
        glTexCoord2f(0.0,  tmax); glVertex2f(0.0, 1.0);
        glTexCoord2f(tmax, 0.0);  glVertex2f(1.0, 0.0);
        glTexCoord2f(tmax, tmax); glVertex2f(1.0, 1.0);
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
    
    /*
    //inject dye
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.,0.8,0.,1.);
    glBegin(GL_QUADS);
    glVertex2d(0.6, 0.6);
    glVertex2d(0.6, 0.62);
    glVertex2d(0.62, 0.62);
    glVertex2d(0.62, 0.6);
    glEnd();
    */
    
    /// INSOO ADDED
    glDisable(GL_ALPHA_TEST);
    
    glDisable(GL_BLEND);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, NPIX, NPIX, 0);
    
    /*
    //blend magnitude texture
    glBindTexture(GL_TEXTURE_2D, mag_tex);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0,  0.0);  glVertex2f(0.0, 0.0);
    glTexCoord2f(0.0,  1.0); glVertex2f(0.0, 1.);
    glTexCoord2f(1.0, 1.0);  glVertex2f(1., 1.);
    glTexCoord2f(1.0, 0.0); glVertex2f(1., 0.0);
    glEnd();
    */

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glPopAttrib();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    /*glPopMatrix();*/
    glMatrixMode(GL_MODELVIEW);
}

void NvLIC::render(){ display(); }


void 
NvLIC::display()
{
    if (_vectorFieldId == 0) {
	return;
    }

    glBindTexture(GL_TEXTURE_2D, color_tex);
    glEnable(GL_TEXTURE_2D);
    
    //draw line integral convolution quad
    glEnable(GL_DEPTH_TEST);
    
    glPushMatrix();
    
    //glScalef(scale.x, scale.y, scale.z); 
    float w = 1.0f / scale.x;
    glTranslatef(origin.x, origin.y, origin.z);
    glScalef(1.0f, 1.0f / scale.y / w, 1.0f / scale.z / w); 
    
    glBegin(GL_QUADS);
    switch (axis)
    {
    case 0:
        glTexCoord2f(0, 0); glVertex3f(offset.x, 0, 0);
        glTexCoord2f(1, 0); glVertex3f(offset.x, 1, 0);
        glTexCoord2f(1, 1); glVertex3f(offset.x, 1, 1);
        glTexCoord2f(0, 1); glVertex3f(offset.x, 0, 1);
	break;
    case 1:
        glTexCoord2f(0, 0); glVertex3f(0, offset.y, 0);
        glTexCoord2f(1, 0); glVertex3f(1, offset.y, 0);
        glTexCoord2f(1, 1); glVertex3f(1, offset.y, 1);
        glTexCoord2f(0, 1); glVertex3f(0, offset.y, 1);
	break;
    case 2:
        glTexCoord2f(0, 0); glVertex3f(0, 0, offset.z);
        glTexCoord2f(1, 0); glVertex3f(1, 0, offset.z);
        glTexCoord2f(1, 1); glVertex3f(1, 1, offset.z);
        glTexCoord2f(0, 1); glVertex3f(0, 1, offset.z);
	break;
    }
    glEnd();
    
    glPopMatrix();
    
    glBindTexture(GL_TEXTURE_2D,0);
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
}

/*
{
    //TRACE("RENDER LIC\n");
   //glBindTexture(GL_TEXTURE_2D, pattern_tex);
   glCallList(1 % Npat + disListID);
   glEnable(GL_TEXTURE_2D);

    //draw line integral convolution quad
    glEnable(GL_DEPTH_TEST);

  glPushMatrix();

  glScalef(scale.x, scale.y, scale.z); 

    glColor3f(1, 0, 1);
  glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(0, 0, offset);
    glTexCoord2f(1, 0); glVertex3f(1, 0, offset);
    glTexCoord2f(1, 1); glVertex3f(1, 1, offset);
    glTexCoord2f(0, 1); glVertex3f(0, 1, offset);
  glEnd();

  glPopMatrix();

  glBindTexture(GL_TEXTURE_2D,0);
  glDisable(GL_TEXTURE_2D);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_RECTANGLE_NV);
}
*/


void 
NvLIC::setVectorField(unsigned int texID, const Vector3& ori, 
		      float scaleX, float scaleY, float scaleZ, float max)
{
    TRACE("NvLIC: vector field is assigned [%d]\n", texID);
    _vectorFieldId = texID;
    origin = ori;
    scale = Vector3(scaleX, scaleY, scaleZ);
    this->max = max;

    make_patterns();
  
    get_slice();
}

void 
NvLIC::get_velocity(float x, float y, float *px, float *py) 
{
   float vx, vy, r;

   int xi = (int) (x*size);
   int yi = (int) (y*size);

    //TRACE("(xi yi) = (%d %d), ", xi, yi);
   vx = slice_vector[4 * (xi+yi*size)];
   vy = slice_vector[4 * (xi+yi*size)+1];
   r  = vx*vx + vy*vy;

    //TRACE("(vx vx) = (%f %f), r=%f, ", vx, vy, r);
   if (r > (dmax*dmax)) { 
      r  = sqrt(r); 
      vx *= dmax/r; 
      vy *= dmax/r; 
   }

   *px = x + vx;         
   *py = y + vy;

    //TRACE("vel %f %f -> %f %f, (dmax = %f)\n", x, y, *px, *py, dmax);
}

void 
NvLIC::set_offset(float v)
{
    switch (axis) {
    case 0 : offset.x = v; break;
    case 1 : offset.y = v; break;
    case 2 : offset.z = v; break;
    }
    get_slice();
}

void NvLIC::set_axis(int axis)
{
    this->axis = axis;
}

void NvLIC::reset()
{
    make_patterns();
}
