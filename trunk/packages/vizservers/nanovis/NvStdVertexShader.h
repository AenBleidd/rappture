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

inline void NvStdVertexShader::bind()
{
    cgGLSetStateMatrixParameter(_mvp_vert_std_param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
    cgGLSetStateMatrixParameter(_mvi_vert_std_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
    cgGLBindProgram(_cgVP);
    cgGLEnableProfile(CG_PROFILE_VP30);
}

inline void NvStdVertexShader::unbind()
{
    cgGLDisableProfile(CG_PROFILE_VP30);
}

#endif 

