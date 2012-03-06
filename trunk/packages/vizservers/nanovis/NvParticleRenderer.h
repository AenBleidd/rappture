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


#ifndef _NV_PARTICLE_SYSTEM_H_
#define _NV_PARTICLE_SYSTEM_H_

#include "GL/glew.h"
#include "Cg/cgGL.h"

#include "define.h"
#include "config.h"
#include "global.h"

#include "Renderable.h"
#include "RenderVertexArray.h"
#include "Vector3.h"

#include <vector>

#include "NvParticleAdvectionShader.h"

struct Particle {
    float x;
    float y;
    float z;
    float aux;

    Particle(){};
    Particle(float _x, float _y, float _z, float _life) :
	x(_x), y(_y), z(_z), aux(_life){}
};

class NvParticleRenderer : public Renderable {
public :
    /**
     * @brief frame buffer objects: two are defined, flip them as input output every step
     */
    GLuint psys_fbo[2]; 	

    /**
     * @brief color textures attached to frame buffer objects
     */
    GLuint psys_tex[2];	
    GLuint initPosTex;	
    Particle* data;
    
    /**
     *@brief Count the frame number of particle system iteration
     */
    int psys_frame;	    
    
    /**
     * @brief Reinitiate particles   	
     */
    bool reborn;			

    /**
     * @brief flip the source and destination render targets 
     */
    bool flip;			

    float max_life;

    float _particleSize;		// Size of the particle: default is 1.2

    /**
     * @brief vertex array for display particles
     */
    RenderVertexArray* m_vertex_array;	

    //Nvidia CG shaders and their parameters
    /*
      CGcontext m_g_context;
      CGprogram m_pos_fprog;
      CGparameter m_vel_tex_param, m_pos_tex_param, m_scale_param;
      CGparameter m_pos_timestep_param, m_pos_spherePos_param;
    */
    static NvParticleAdvectionShader* _advectionShader;

    /**
     * @brief scale of flow data 
     */
    Vector3 scale;

    Vector3 origin;

    bool _activate;

    float _slice_pos;
    int _slice_axis;
    Vector4 _color;

public:
    int psys_width;	//the storage of particles is implemented as a 2D array.
    int psys_height;

    NvParticleRenderer(int w, int h, CGcontext context);
    ~NvParticleRenderer();
    void setVectorField(unsigned int texID, const Vector3& ori, float scaleX, float scaleY, float scaleZ, float max);
    void initialize();
    void advect();
    void update_vertex_buffer();
    void display_vertices();
    void reset();
    void render();

    bool active(void) {
	return _activate;
    }
    void active(bool state) {
	_activate = state;
    }
    void setColor(const Vector4& color) {
	_color = color;
    }
    void setAxis(int axis);
    void setPos(float pos);
    void draw_bounding_box(float x0, float y0, float z0, 
			   float x1, float y1, float z1, 
			   float r, float g, float b, float line_width);
    void initializeDataArray();
    void particleSize(float size) {
	_particleSize = size;
    }
    float particleSize(void) {
	return _particleSize;
    }
};

#endif
