/*
 * ----------------------------------------------------------------------
 * ParticleSystem.h: particle system class
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


#ifndef _PARTICLE_SYSTEM_H_
#define _PARTICLE_SYSTEM_H_

#include "GL/glew.h"
#include "Cg/cgGL.h"

#include "define.h"
#include "config.h"
#include "global.h"

#include "Renderable.h"
#include "RenderVertexArray.h"
#include "Vector3.h"

typedef struct Particle{
  float x;
  float y;
  float z;
  float aux;

  Particle(){};
  Particle(float _x, float _y, float _z, float _life) :
   x(_x), y(_y), z(_z), aux(_life){}
};


class ParticleSystem : public Renderable{

  NVISid psys_fbo[2]; 	//frame buffer objects: two are defined, flip them as input output every step
  NVISid psys_tex[2];	//color textures attached to frame buffer objects

  Particle* data;

  int psys_frame;	       	//count the frame number of particle system iteration
  bool reborn;			//reinitiate particles
  bool flip;			//flip the source and destination render targets 
  float max_life;

  RenderVertexArray* m_vertex_array;	//vertex array for display particles

  //Nvidia CG shaders and their parameters
  CGcontext m_g_context;
  CGprogram m_pos_fprog;
  CGparameter m_vel_tex_param, m_pos_tex_param, m_scale_param;
  CGparameter m_pos_timestep_param, m_pos_spherePos_param;

  Vector3 scale;

public:
  int psys_width;	//the storage of particles is implemented as a 2D array.
  int psys_height;

  ParticleSystem(int w, int h, CGcontext context, NVISid vector_field, 
		  float scalex, float scaley, float scalez);
  ~ParticleSystem();
  void initialize(Particle* data);
  void advect();
  void update_vertex_buffer();
  void display_vertices();
  void reset();
  void render();

};


#endif
