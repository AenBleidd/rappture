/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_STD_VERTEX_SHADER_H
#define NV_STD_VERTEX_SHADER_H

#include "NvShader.h"

class NvStdVertexShader : public NvShader
{
public:
    NvStdVertexShader();

    virtual ~NvStdVertexShader();

    virtual void bind(float *mvp = NULL, float *mvInv = NULL);

    virtual void unbind()
    {
        NvShader::unbind();
    }

private:
    void init();
};

#endif 

