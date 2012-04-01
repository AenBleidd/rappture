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
        cgGLSetParameter4f(_scaleVP, scale, 1.0f, 1.0f, 1.0f);
    }

    void setNormalTexture(Texture3D *n)
    {
        _normal = n;
    }

    virtual void bind()
    {
        setParameters();

        NvShader::bind();
    }

    virtual  void unbind()
    {
        resetParameters();

        NvShader::unbind();
    }

protected:
    virtual void setParameters();
    virtual void resetParameters();

private:
    CGparameter _modelviewVP;
    CGparameter _projectionVP;

    CGparameter _attenVP;
    CGparameter _posoffsetVP;
    CGparameter _baseposVP;
    CGparameter _scaleVP;
    CGparameter _normalParam;

    Texture3D *_normal;
};

#endif
