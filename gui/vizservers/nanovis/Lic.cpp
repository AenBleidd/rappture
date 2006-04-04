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



#include "Lic.h"


Lic::Lic(int _size, int _width, int _height):
	size(_size),
	display_width(_width),
	display_height(_height)
{

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
  glGenFramebuffersEXT(1, &vel_fbo);
  glGenTextures(1, &slice_vector_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, vel_fbo);

  //initialize texture storing per slice vectors
  glBindTexture(GL_TEXTURE_RECTANGLE_NV, slice_vector_tex);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 
		  GL_FLOAT_RGBA32_NV, size, size, 0, GL_RGBA, GL_FLOAT, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		  GL_TEXTURE_RECTANGLE_NV, slice_vector_tex, 0);


  //lic result fbo
  glGenFramebuffersEXT(1, &fbo);
  glGenTextures(1, &color_tex);

  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

  //initialize color texture for lic
  glBindTexture(GL_TEXTURE_2D, color_tex);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, display_width, display_height, 0,
               GL_RGB, GL_INT, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                            GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, color_tex, 0);

  // Check framebuffer completeness at the end of initialization.
  CHECK_FRAMEBUFFER_STATUS();
  assert(glGetError()==0);

}


void Lic::MakePatterns(void) 
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


void Lic::MakeMagnitudes()
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


//line integral convolution
void Lic::convolve(){
   int   i, j; 
   float x1, x2, y, px, py;

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
          getDP(x1, y, &px, &py);
          glVertex2f(px, py);

          glTexCoord2f(x2, y); 
          getDP(x2, y, &px, &py); 
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
}

void display(){


}

