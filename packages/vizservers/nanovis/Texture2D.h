/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Texture2D.h: 2d texture class
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

#ifndef _TEXTURE_2D_H_
#define _TEXTURE_2D_H_

#include <GL/glew.h>

class Texture2D{
	
public:
  int width;
  int height;

  GLuint type;
  GLuint id;
  GLuint interp_type;
  int n_components;

  Texture2D();
  Texture2D(int width, int height, GLuint type, GLuint interp, int n, float* data);
  ~Texture2D();
	
  void activate();
  void deactivate();
  void enable();
  void disable();
  GLuint initialize(float* data);
  static void check_max_size();
  static void check_max_unit();
};

#endif
