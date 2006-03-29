/*
 * ----------------------------------------------------------------------
 *  nanovis.h: package header
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

#include <GL/glew.h>
#include <GL/glut.h>
#include <Cg/cgGL.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <float.h>

#include "define.h"
#include "global.h"
#include "socket/Socket.h"
#include "RenderVertexArray.h"
#include "ConvexPolygon.h"
#include "Texture3D.h"
#include "Texture2D.h"
#include "Texture1D.h"
#include "TransferFunction.h"
#include "ConvexPolygon.h"
#include "Mat4x4.h"
#include "Volume.h"
#include "ParticleSystem.h"
#include "PerfQuery.h"
#include "Event.h"

#include "config.h"

#include <tcl.h>
#ifndef CONST84
# define CONST84 
#endif


//defines for the image based flow visualization
#define	NPN 256 	//resolution of background pattern
#define NMESH 256	//resolution of flow mesh
#define DM  ((float) (1.0/(NMESH-1.0)))	//distance in world coords between mesh lines
#define NPIX  512	//display size
#define SCALE 3.0	//scale for background pattern. small value -> fine texture
#define MAX_N_VOLUMES 10 //maximum of volumes the application can handle


typedef struct Vector2{
  float x,y;
  float mag(){
    return sqrt(x*x+y*y);
  }
};


typedef struct RegGrid2{
  int width, height;
  Vector2* field;

  RegGrid2(int w, int h){
    width = w;
    height = h;
    field = new Vector2[w*h];
  }

  void put(Vector2& v, int x ,int y){
    field[x+y*width] = v;
  }

  Vector2& get(int x, int y){
    return field[x+y*width];
  }
};


//variables for mouse events
float live_rot_x = 0.;		//object rotation angles
float live_rot_y = 0.;
float live_rot_z = 0.;

float live_obj_x = -0.0;	//object translation location from the origin
float live_obj_y = -0.0;
float live_obj_z = -2.5;

float live_diffuse = 1.;
float live_specular = 3.;

int left_last_x, left_last_y, right_last_x, right_last_y; 	//last locations mouse events
bool left_down = false;						
bool right_down = false;

float lic_slice_x=0, lic_slice_y=0, lic_slice_z=0.3;//image based flow visualization slice location

int win_width = NPIX;			//size of the render window
int win_height = NPIX;			//size of the render window


//image based flow visualization variables
int    iframe = 0; 
int    Npat   = 64;
int    alpha  = (0.12*255);
float  sa;
float  tmax   = NPIX/(SCALE*NPN);
float  dmax   = SCALE/NPIX;


//currently active shader, default renders one volume only
int cur_shader = 0;
