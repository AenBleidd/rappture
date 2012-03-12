/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * NvParticleRenderer.h: particle system class
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
#ifndef NVPARTICLERENDERER_H
#define NVPARTICLERENDERER_H

#include <vector>

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "define.h"
#include "config.h"
#include "global.h"

#include "Renderable.h"
#include "RenderVertexArray.h"
#include "Vector3.h"
#include "NvParticleAdvectionShader.h"

struct Particle {
    float x;
    float y;
    float z;
    float aux;

    Particle()
    {}

    Particle(float _x, float _y, float _z, float _life) :
	x(_x), y(_y), z(_z), aux(_life)
    {}
};

class NvParticleRenderer : public Renderable
{
public:
    NvParticleRenderer(int w, int h, CGcontext context);

    ~NvParticleRenderer();

    void setVectorField(unsigned int texID, const Vector3& ori,
                        float scaleX, float scaleY, float scaleZ, float max);

    void initialize();

    void advect();

    void update_vertex_buffer();

    void display_vertices();

    void reset();

    void render();

    bool active() const
    {
	return _activate;
    }

    void active(bool state)
    {
	_activate = state;
    }

    void setColor(const Vector4& color)
    {
	_color = color;
    }

    void setAxis(int axis);

    void setPos(float pos);

    void draw_bounding_box(float x0, float y0, float z0, 
			   float x1, float y1, float z1, 
			   float r, float g, float b, float line_width);

    void initializeDataArray();

    void particleSize(float size)
    {
	_particleSize = size;
    }

    float particleSize() const
    {
	return _particleSize;
    }

   static NvParticleAdvectionShader *_advectionShader;

private:
    /// frame buffer objects: two are defined, flip them as input output every step
    GLuint psys_fbo[2]; 	

    /// color textures attached to frame buffer objects
    GLuint psys_tex[2];	
    GLuint initPosTex;	
    Particle *data;

    /// Count the frame number of particle system iteration
    int psys_frame;	    

    /// Reinitiate particles
    bool reborn;			

    /// flip the source and destination render targets
    bool flip;			

    float max_life;

    /// Size of the particle: default is 1.2
    float _particleSize;

    /// vertex array for display particles
    RenderVertexArray *_vertex_array;	

    /// scale of flow data 
    Vector3 scale;

    Vector3 origin;

    bool _activate;

    float _slice_pos;
    int _slice_axis;

    Vector4 _color;

    //the storage of particles is implemented as a 2D array.
    int psys_width;
    int psys_height;
};

#endif
