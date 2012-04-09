/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef POINTSHADER_H
#define POINTSHADER_H

#include "NvShader.h"
#include "Texture3D.h"

class PointShader : public NvShader
{
public:
    PointShader();

    virtual ~PointShader();

    void setScale(float scale)
    {
        _scale = scale;
    }

    void setNormalTexture(Texture3D *normal)
    {
        _normal = normal;
    }

    virtual void bind();

    virtual  void unbind();

private:
    CGparameter _modelviewVP;
    CGparameter _projectionVP;

    CGparameter _attenVP;
    CGparameter _posoffsetVP;
    CGparameter _baseposVP;
    CGparameter _scaleVP;
    CGparameter _normalParam;

    float _scale;
    Texture3D *_normal;
};

#endif
