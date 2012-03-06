/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * Renderable.h: abstract class, a drawable thing 
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


#ifndef _RENDERABLE_H_
#define _RENDERABLE_H_


#include "Vector3.h"

struct BoundBox{
  Vector3 low; //lower coordinates
  Vector3 high; //higher coordinates

  BoundBox(){}
  BoundBox(float low_x, float low_y, float low_z,
	  float high_x, float high_y, float high_z):
	  low(Vector3(low_x, low_y, low_z)),
	  high(Vector3(high_x, high_y, high_z)){}
};

class Renderable{
protected:
  Vector3 location;	//the location (x,y,z) of the object
  bool enabled;		//display is enabled
  BoundBox boundary;	//the bounding box

public:
  explicit Renderable(Vector3 loc);
  Renderable();
  virtual ~Renderable();

  void move(Vector3 new_loc);
  virtual void render() = 0;
  void enable();
  void disable();
  bool is_enabled();
};


#endif
