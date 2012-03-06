/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "ParticleEmitter.h"

ParticleEmitter::ParticleEmitter() :
    _minLifeTime(30.0f), _maxLifeTime(100.0f),
    _maxPositionOffset(1, 1, 1),
    _minNumOfNewParticles(10), _maxNumOfNewParticles(20)
{
}

void ParticleEmitter::setVectorField()
{
}
