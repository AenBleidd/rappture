/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <R2/R2FilePath.h>

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "NvParticleAdvectionShader.h"
#include "Trace.h"

NvParticleAdvectionShader::NvParticleAdvectionShader() : 
    _velocityVolumeID(0), 
    _scale(1.0f, 1.0f, 1.0f), 
    _max(1.0f), 
    _timeStep(0.005f)
{
    _mode = 1;
    init();
}

NvParticleAdvectionShader::~NvParticleAdvectionShader()
{
}

void NvParticleAdvectionShader::init()
{
    loadFragmentProgram("update_pos.cg", "main");
    _posTimestepParam  = cgGetNamedParameter(_cgFP, "timestep");
    _maxParam          = cgGetNamedParameter(_cgFP, "max");
    _velTexParam       = cgGetNamedParameter(_cgFP, "vel_tex");
    _posTexParam       = cgGetNamedParameter(_cgFP, "pos_tex");
    _initPosTexParam   = cgGetNamedParameter(_cgFP, "init_pos_tex");
    _scaleParam        = cgGetNamedParameter(_cgFP, "scale");
    _modeParam         = cgGetNamedParameter(_cgFP, "mode");
}

void 
NvParticleAdvectionShader::bind(unsigned int texID, unsigned int initPosTexID)
{
    cgGLBindProgram(_cgFP);
    cgGLSetParameter1f(_posTimestepParam, _timeStep);
    cgGLSetParameter1f(_maxParam, _max);
    cgGLSetParameter1f(_modeParam, _mode);
    cgGLSetParameter3f(_scaleParam, _scale.x, _scale.y, _scale.z);
    cgGLSetTextureParameter(_velTexParam, _velocityVolumeID);
    cgGLEnableTextureParameter(_velTexParam);

    cgGLSetTextureParameter(_posTexParam, texID);
    cgGLEnableTextureParameter(_posTexParam);

    cgGLSetTextureParameter(_initPosTexParam, initPosTexID);
    cgGLEnableTextureParameter(_initPosTexParam);

    cgGLEnableProfile(CG_PROFILE_FP40);
}

void
NvParticleAdvectionShader::unbind()
{
     cgGLDisableProfile(CG_PROFILE_FP40);
   
     cgGLDisableTextureParameter(_velTexParam);
     cgGLDisableTextureParameter(_posTexParam);
     cgGLDisableTextureParameter(_initPosTexParam);
}
