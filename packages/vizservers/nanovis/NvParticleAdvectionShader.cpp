/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "NvParticleAdvectionShader.h"
#include "Trace.h"

NvParticleAdvectionShader::NvParticleAdvectionShader() : 
    _velocityVolumeID(0), 
    _scale(1.0f, 1.0f, 1.0f), 
    _max(1.0f), 
    _timeStep(0.0005f),
    _mode(1)
{
    init();
}

NvParticleAdvectionShader::~NvParticleAdvectionShader()
{
}

void NvParticleAdvectionShader::init()
{
    loadFragmentProgram("update_pos.cg", "main");
    _posTexParam       = getNamedParameterFromFP("pos_tex");
    _initPosTexParam   = getNamedParameterFromFP("init_pos_tex");
    _velTexParam       = getNamedParameterFromFP("vel_tex");
    _posTimestepParam  = getNamedParameterFromFP("timestep");
    _maxParam          = getNamedParameterFromFP("max");
    _modeParam         = getNamedParameterFromFP("mode");
    _scaleParam        = getNamedParameterFromFP("scale");
}

void 
NvParticleAdvectionShader::bind(unsigned int texID, unsigned int initPosTexID)
{
    cgGLSetTextureParameter(_posTexParam, texID);
    cgGLEnableTextureParameter(_posTexParam);

    cgGLSetTextureParameter(_initPosTexParam, initPosTexID);
    cgGLEnableTextureParameter(_initPosTexParam);

    cgGLSetTextureParameter(_velTexParam, _velocityVolumeID);
    cgGLEnableTextureParameter(_velTexParam);

    cgGLSetParameter1f(_posTimestepParam, _timeStep);
    cgGLSetParameter1f(_maxParam, _max);
    cgGLSetParameter1f(_modeParam, _mode);
    cgGLSetParameter3f(_scaleParam, _scale.x, _scale.y, _scale.z);

    NvShader::bind();
}

void
NvParticleAdvectionShader::unbind()
{
     cgGLDisableTextureParameter(_posTexParam);
     cgGLDisableTextureParameter(_initPosTexParam);
     cgGLDisableTextureParameter(_velTexParam);

     NvShader::unbind();
}
