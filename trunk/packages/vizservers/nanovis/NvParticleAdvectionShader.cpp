/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "NvParticleAdvectionShader.h"

using namespace nv;

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
}

void 
NvParticleAdvectionShader::bind(unsigned int texID, unsigned int initPosTexID)
{
    setFPTextureParameter("pos_tex", texID);
    setFPTextureParameter("init_pos_tex", initPosTexID);
    setFPTextureParameter("vel_tex", _velocityVolumeID);

    setFPParameter1f("timestep", _timeStep);
    setFPParameter1f("max", _max);
    setFPParameter1f("mode", _mode);
    setFPParameter3f("scale", _scale.x, _scale.y, _scale.z);

    NvShader::bind();
}

void
NvParticleAdvectionShader::unbind()
{
     disableFPTextureParameter("pos_tex");
     disableFPTextureParameter("init_pos_tex");
     disableFPTextureParameter("vel_tex");

     NvShader::unbind();
}
