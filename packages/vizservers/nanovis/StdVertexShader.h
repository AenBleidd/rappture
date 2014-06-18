/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_STD_VERTEX_SHADER_H
#define NV_STD_VERTEX_SHADER_H

#include <vrmath/Vector4f.h>

#include "Shader.h"

namespace nv {

class StdVertexShader : public Shader
{
public:
    StdVertexShader();

    virtual ~StdVertexShader();

    virtual void bind(const vrmath::Vector4f& objPlaneS,
                      const vrmath::Vector4f& objPlaneT,
                      const vrmath::Vector4f& objPlaneR,
                      float *mvp = NULL, float *mvInv = NULL);

    virtual void unbind()
    {
        Shader::unbind();
    }

private:
    void init();
};

}

#endif 
