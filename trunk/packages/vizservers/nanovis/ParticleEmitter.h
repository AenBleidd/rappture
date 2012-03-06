/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include "datatype.h"
#include <string>

class ParticleEmitter
{
public:
    std::string _name;
    float3 _position;

    float3 _oldPosition;

    float _minLifeTime;
    float _maxLifeTime;
	
    // [0..1] * _maxPositionOffset;
    float3 _maxPositionOffset;

    int _minNumOfNewParticles;
    int _maxNumOfNewParticles;

    bool _enabled;

    ParticleEmitter();

    void setName(const std::string& name);
    void setPosition(float x, float y, float z);
    void setMinMaxLifeTime(float minLifeTime, float maxLifeTime);
    void setMaxPositionOffset(float offsetX, float offsetY, float offsetZ);
    void setMinMaxNumOfNewParticles(int minNum, int maxNum);

    void setVectorField();
    void setEnabled(bool enabled);
    bool isEnabled() const;
};

inline void ParticleEmitter::setName(const std::string& name)
{
    _name = name;
}

inline void ParticleEmitter::setPosition(float x, float y, float z)
{
    _position.x = x;
    _position.y = y;
    _position.z = z;
}

inline void ParticleEmitter::setMinMaxLifeTime(float minLifeTime, float maxLifeTime)
{
    _minLifeTime = minLifeTime;
    _maxLifeTime = maxLifeTime;
}

inline void ParticleEmitter::setMaxPositionOffset(float offsetX, float offsetY, float offsetZ)
{
    _maxPositionOffset.x = offsetX;
    _maxPositionOffset.y = offsetY;
    _maxPositionOffset.z = offsetZ;
}

inline void ParticleEmitter::setMinMaxNumOfNewParticles(int minNum, int maxNum)
{
    _minNumOfNewParticles = minNum;
    _maxNumOfNewParticles = maxNum;
}

inline void ParticleEmitter::setEnabled(bool enabled)
{
    _enabled = enabled;
}

inline bool ParticleEmitter::isEnabled() const
{
    return _enabled;
}
