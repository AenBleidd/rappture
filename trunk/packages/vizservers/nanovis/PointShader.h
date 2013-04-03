/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_POINTSHADER_H
#define NV_POINTSHADER_H

#include "Shader.h"
#include "Texture3D.h"

namespace nv {

class PointShader : public Shader
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

    virtual void unbind();

private:
    float _scale;
    Texture3D *_normal;
};

}

#endif
