/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ----------------------------------------------------------------------
 * VolumeRenderer.h : VolumeRenderer class for volume visualization
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

#ifndef _VOLUME_RENDERER_H_
#define _VOLUME_RENDERER_H_

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "Mat4x4.h"
#include "Volume.h"
#include "VolumeInterpolator.h"
#include "NvRegularVolumeShader.h"
#include "NvZincBlendeVolumeShader.h"
#include "NvStdVertexShader.h"

class VolumeRenderer
{
    friend class NanoVis;

private:
    VolumeInterpolator* _volumeInterpolator;

    bool slice_mode;			//!<- enable cut planes
    bool volume_mode;			//!<- enable full volume rendering

    /** 
     * shader parameters for rendering a single cubic volume
     */
    NvRegularVolumeShader* _regularVolumeShader;

    /**
     * Shader parameters for rendering a single zincblende orbital.  A
     * simulation contains S, P, D and SS, total of 4 orbitals. A full
     * rendering requires 4 zincblende orbital volumes.  A zincblende orbital
     * volume is decomposed into two "interlocking" cubic volumes and passed
     * to the shader.  We render each orbital with its own independent
     * transfer functions then blend the result.
     * 
     * The engine is already capable of rendering multiple volumes and combine
     * them. Thus, we just invoke this shader on S, P, D and SS orbitals with
     * different transfor functions. The result is a multi-orbital rendering.
     * This is equivalent to rendering 4 unrelated data sets occupying the
     * same space.
     */
    NvZincBlendeVolumeShader* _zincBlendeShader;

    /**
     * standard vertex shader 
     */
    NvStdVertexShader* _stdVertexShader;

    //standard vertex shader parameters
    CGprogram m_vert_std_vprog;
    CGparameter m_mvp_vert_std_param;
    CGparameter m_mvi_vert_std_param;
    GLuint font_base;		// The base of the font display list.
    GLuint font_texture;	//the id of the font texture

    void init_shaders();

    void activate_volume_shader(Volume* vol, bool slice_mode);

    void deactivate_volume_shader();

    //draw bounding box
    void draw_bounding_box(float x0, float y0, float z0,
			   float x1, float y1, float z1,
			   float r, float g, float b, float line_width);

    void get_near_far_z(const Mat4x4& mv, double& zNear, double& zFar);

    bool init_font(const char* filename);

    void glPrint(char* string, int set); //there are two sets of font in the
					 //texture. 0, 1

    void draw_label(Volume* vol); //draw label using bitmap texture
				// the texture.

    void build_font();		// Register the location of each alphabet in

public:
    VolumeRenderer();

    ~VolumeRenderer();

    void render_all();			//render all enabled volumes;

    void specular(float val);

    void diffuse(float val);

    void set_slice_mode(bool val)	//control independently.
    {
        slice_mode = val;
    }

    void set_volume_mode(bool val)
    {
        volume_mode = val;
    }

    void switch_slice_mode()		//switch_cutplane_mode
    {
        slice_mode = (!slice_mode);
    }

    void switch_volume_mode()
    {
        volume_mode = (!volume_mode);
    }

    void clearAnimatedVolumeInfo()
    {
        _volumeInterpolator->clearAll();
    }

    void addAnimatedVolume(Volume* volume)
    {
        _volumeInterpolator->addVolume(volume);
    }

    void startVolumeAnimation()
    {
        _volumeInterpolator->start();
    }

    void stopVolumeAnimation()
    {
	_volumeInterpolator->stop();
    }

    VolumeInterpolator* getVolumeInterpolator()
    {
	return _volumeInterpolator;
    }
};

#endif	/*_VOLUME_RENDERER_H_*/
