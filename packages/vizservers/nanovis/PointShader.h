/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
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

    virtual void unbind();

private:
    float _scale;
    float _scale;
    Texture3D *_normal;
};

#endif
