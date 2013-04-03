/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "ParticleEmitter.h"

using namespace nv;

ParticleEmitter::ParticleEmitter() :
    _minLifeTime(30.0f), _maxLifeTime(100.0f),
    _maxPositionOffset(1, 1, 1),
    _minNumOfNewParticles(10), _maxNumOfNewParticles(20)
{
}

void ParticleEmitter::setVectorField()
{
}
