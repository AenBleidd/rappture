/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_STD_VERTEX_SHADER_H
#define NV_STD_VERTEX_SHADER_H

#include "NvShader.h"

class NvStdVertexShader : public NvShader
{
public:
    NvStdVertexShader();

    virtual ~NvStdVertexShader();

    virtual void bind();

    virtual void unbind()
    {
        NvShader::unbind();
    }

private:
    void init();

    /// A parameter id for ModelViewProjection matrix of Cg program
    CGparameter _mvpVertStdParam;

    /// A parameter id for ModelViewInverse matrix of Cg program
    CGparameter _mviVertStdParam;
};

#endif 

