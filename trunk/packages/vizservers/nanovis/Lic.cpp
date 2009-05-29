/*
 * ----------------------------------------------------------------------
 * Lic.h: line integral convolution class
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


#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <R2/R2FilePath.h>
#include "Lic.h"

Lic::Lic(int _size, int _width, int _height, float _offset,
		CGcontext _context, NVISid _vector_field,
		float scalex, float scaley, float scalez):
        Renderable(Vector3(0.,0.,0.)),
	size(_size),
	offset(_offset),
	m_g_context(_context),
	width(_width),
	height(_height),
        iframe(0),
	Npat(64),
  	alpha(0.12*255),
	tmax(NPIX/(SCALE*NPN)),
	dmax(SCALE/NPIX)
{
  scale = Vector3(scalex, scaley, scalez);
  slice_vector = new float[size*size*4];

  //initialize the pattern texture
  glGenTextures(1, &pattern_tex);
  glBindTexture(GL_TEXTURE_2D, pattern_tex);
  glTexParameteri(GL_TEXTURE_2D,
                  GL_TEXTURE_WRAP_S, GL_REPEAT); 
  glTexParameteri(GL_TEXTURE_2D,
                  GL_TEXTURE_WRAP_T, GL_REPEAT); 
  glTexParameteri(GL_TEXTURE_2D,
                  GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,
                  GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  //initialize frame buffer objects

  //render buffer for projecting 3D velocity onto a 2D plane
  glGenFramebuffersEXT(1, &vel_fbo);
  glGenTextures(1, &slice_vector_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);

  glBindTexture(GL_TEXTURE_RECTANGLE_NV, slice_vector_tex);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 
		  GL_FLOAT_RGBA32_NV, size, size, 0, GL_RGBA, GL_FLOAT, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		  GL_TEXTURE_RECTANGLE_NV, slice_vector_tex, 0);


  //render buffer for the convolution
  glGenFramebuffersEXT(1, &fbo);
  glGenTextures(1, &color_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

  //initialize color texture for lic
  glBindTexture(GL_TEXTURE_2D, color_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0,
               GL_RGB, GL_INT, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                            GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, color_tex, 0);

  // Check framebuffer completeness at the end of initialization.
  CHECK_FRAMEBUFFER_STATUS();
  assert(glGetError()==0);

  m_render_vel_fprog = LoadProgram(m_g_context, CG_PROFILE_FP30, CG_SOURCE, 
	NULL, "render_vel.cg");
  m_vel_tex_param_render_vel = cgGetNamedParameter(m_render_vel_fprog, 
	"vel_tex");
  m_plane_normal_param_render_vel = cgGetNamedParameter(m_render_vel_fprog, 
	"plane_normal");
  cgGLSetTextureParameter(m_vel_tex_param_render_vel, _vector_field);

  get_slice();
  make_patterns();
}

Lic::~Lic(){
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);
  glDeleteTextures(1, &slice_vector_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
  glDeleteTextures(1, &color_tex);

  NVISid buffers[2] = {vel_fbo, fbo};
  glDeleteFramebuffersEXT(2, buffers);

  delete slice_vector;
}

void Lic::make_patterns() 
{ 
    int lut[256];
    int phase[NPN][NPN];
    GLubyte pat[NPN][NPN][4];
    int i, j, k, t;
    
    for (i = 0; i < 256; i++) lut[i] = i < 127 ? 0 : 255;
    for (i = 0; i < NPN; i++)
	for (j = 0; j < NPN; j++) phase[i][j] = rand() % 256; 
    
    for (k = 0; k < Npat; k++) {
	t = k*256/Npat;
	for (i = 0; i < NPN; i++) 
	    for (j = 0; j < NPN; j++) {
		pat[i][j][0] =
		    pat[i][j][1] =
		    pat[i][j][2] = lut[(t + phase[i][j]) % 255];
		pat[i][j][3] = alpha;
	    }
	
	glNewList(k + 1, GL_COMPILE);
	glBindTexture(GL_TEXTURE_2D, pattern_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, NPN, NPN, 0, 
		     GL_RGBA, GL_UNSIGNED_BYTE, pat);
	glEndList();
    }
}


void Lic::make_magnitudes()
{

  GLubyte mag[NMESH][NMESH][4];

  //read vector filed
  for(int i=0; i<NMESH; i++){
    for(int j=0; j<NMESH; j++){

      float x=DM*i;
      float y=DM*j;

      float magnitude = sqrt(x*x+y*y)/1.414;
      
      //from green to red
      GLubyte r = floor(magnitude*255);
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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NMESH, NMESH, 0, GL_RGBA, GL_UNSIGNED_BYTE, mag);

}

//project 3D vectors to a 2D slice for line integral convolution
void Lic::get_slice(){
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);
  glBindTexture(GL_TEXTURE_RECTANGLE_NV, slice_vector_tex);

  glClear(GL_COLOR_BUFFER_BIT);

  glViewport(0, 0, NMESH, NMESH);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, NMESH, 0, NMESH);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  cgGLBindProgram(m_render_vel_fprog);
  cgGLEnableTextureParameter(m_vel_tex_param_render_vel);
  cgGLSetParameter4f(m_plane_normal_param_render_vel, 1., 1., 0., 0);

  cgGLEnableProfile(CG_PROFILE_FP30);
  glBegin(GL_QUADS);
    glTexCoord3f(0., 0., offset); glVertex2f(0.,   0.);
    glTexCoord3f(1., 0., offset); glVertex2f(size, 0.);
    glTexCoord3f(1., 1., offset); glVertex2f(size, size);
    glTexCoord3f(0., 1., offset); glVertex2f(0.,   size);
  glEnd();
  cgGLDisableProfile(CG_PROFILE_FP30);
   
  cgGLDisableTextureParameter(m_vel_tex_param_render_vel);

  //read the vectors
  glReadPixels(0, 0, NMESH, NMESH, GL_RGBA, GL_FLOAT, slice_vector);
  /*
  for(int i=0; i<NMESH*NMESH; i++){
    fprintf(stderr, "%f,%f,%f,%f", slice_vector[4*i], slice_vector[4*i+1], slice_vector[4*i+2], slice_vector[4*i+3]);
  }
  */
  assert(glGetError()==0);
}


//line integral convolution
void Lic::convolve(){

   int   i, j; 
   float x1, x2, y, px, py;

   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

   glViewport(0, 0, (GLsizei) NPIX, (GLsizei) NPIX);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity(); 
   glTranslatef(-1.0, -1.0, 0.0); 
   glScalef(2.0, 2.0, 1.0);

   //sa = 0.010*cos(iframe*2.0*M_PI/200.0);
   glBindTexture(GL_TEXTURE_2D, pattern_tex);
   glEnable(GL_TEXTURE_2D);
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
   glCallList(iframe % Npat + 1);
   glBegin(GL_QUAD_STRIP);
      glTexCoord2f(0.0,  0.0);  glVertex2f(0.0, 0.0);
      glTexCoord2f(0.0,  tmax); glVertex2f(0.0, 1.0);
      glTexCoord2f(tmax, 0.0);  glVertex2f(1.0, 0.0);
      glTexCoord2f(tmax, tmax); glVertex2f(1.0, 1.0);
   glEnd();

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
}

void Lic::render(){ display(); }


void Lic::display(){

  glBindTexture(GL_TEXTURE_2D, color_tex);
  glEnable(GL_TEXTURE_2D);

  //draw line integral convolution quad
  glEnable(GL_DEPTH_TEST);
  glPushMatrix();
  glScalef(scale.x, scale.y, scale.z); 

  glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(0, 0, offset);
    glTexCoord2f(1, 0); glVertex3f(1, 0, offset);
    glTexCoord2f(1, 1); glVertex3f(1, 1, offset);
    glTexCoord2f(0, 1); glVertex3f(0, 1, offset);
  glEnd();

  glPopMatrix();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);
}


void Lic::get_velocity(float x, float y, float *px, float *py) 
{
   float dx, dy, vx, vy, r;

   int xi = x*NMESH;
   int yi = y*NMESH;

   vx = slice_vector[4*(xi+yi*NMESH)];
   vy = slice_vector[4*(xi+yi*NMESH)+1];
   r  = vx*vx + vy*vy;
   if (r > dmax*dmax) { 
      r  = sqrt(r); 
      vx *= dmax/r; 
      vy *= dmax/r; 
   }
   *px = x + vx;         
   *py = y + vy;
}

void Lic::set_offset(float v)
{
  offset = v;
  get_slice();
}
