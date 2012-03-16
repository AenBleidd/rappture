/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_STD_VERTEX_SHADER_H
#define NV_STD_VERTEX_SHADER_H

#include "NvShader.h"

class NvStdVertexShader : public NvShader
{
public:
    NvStdVertexShader();

    ~NvStdVertexShader();

    void bind();
    void unbind();

private:
    void init();

    /// A parameter id for ModelViewProjection matrix of Cg program
    CGparameter _mvp_vert_std_param;

    /// A parameter id for ModelViewInverse matrix of Cg program
    CGparameter _mvi_vert_std_param;
};

#endif 

