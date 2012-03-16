/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>

#include "global.h"
#include "NvStdVertexShader.h"

NvStdVertexShader::NvStdVertexShader()
{
    init();
}

NvStdVertexShader::~NvStdVertexShader()
{
}

void NvStdVertexShader::init()
{
    _cgVP = LoadCgSourceProgram(g_context, "vertex_std.cg", CG_PROFILE_VP30, 
                                "main");
    _mvp_vert_std_param = cgGetNamedParameter(_cgVP, "modelViewProjMatrix");
    _mvi_vert_std_param = cgGetNamedParameter(_cgVP, "modelViewInv");
}

void NvStdVertexShader::bind()
{
    cgGLSetStateMatrixParameter(_mvp_vert_std_param, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
    cgGLSetStateMatrixParameter(_mvi_vert_std_param, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);
    cgGLBindProgram(_cgVP);
    cgGLEnableProfile(CG_PROFILE_VP30);
}

void NvStdVertexShader::unbind()
{
    cgGLDisableProfile(CG_PROFILE_VP30);
}
