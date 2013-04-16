/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "ParticleAdvectionShader.h"

using namespace nv;

ParticleAdvectionShader::ParticleAdvectionShader() : 
    _velocityVolumeID(0), 
    _scale(1.0f, 1.0f, 1.0f), 
    _max(1.0f), 
    _timeStep(0.0005f)
{
    init();
}

ParticleAdvectionShader::~ParticleAdvectionShader()
{
}

void ParticleAdvectionShader::init()
{
    loadFragmentProgram("update_pos.cg");
}

void 
ParticleAdvectionShader::bind(unsigned int texID, unsigned int initPosTexID, bool init)
{
    setFPTextureParameter("pos_tex", texID);
    setFPTextureParameter("init_pos_tex", initPosTexID);
    setFPTextureParameter("vel_tex", _velocityVolumeID);

    setFPParameter1f("timestep", _timeStep);
    setFPParameter1f("max", init ? 0 : _max);
    setFPParameter3f("scale", _scale.x, _scale.y, _scale.z);

    Shader::bind();
}

void
ParticleAdvectionShader::unbind()
{
     disableFPTextureParameter("pos_tex");
     disableFPTextureParameter("init_pos_tex");
     disableFPTextureParameter("vel_tex");

     Shader::unbind();
}
