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
#include <tcl.h>
#include <float.h>

#include "config.h"


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
