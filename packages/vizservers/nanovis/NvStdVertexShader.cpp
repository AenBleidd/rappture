/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>

#include <GL/glew.h>
#include <Cg/cgGL.h>

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
    loadVertexProgram("vertex_std.cg", "main");
    _mvpVertStdParam = getNamedParameterFromVP("modelViewProjMatrix");
    _mviVertStdParam = getNamedParameterFromVP("modelViewInv");
}

void NvStdVertexShader::bind()
{
    cgGLSetStateMatrixParameter(_mvpVertStdParam, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
    cgGLSetStateMatrixParameter(_mviVertStdParam, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_INVERSE);

    NvShader::bind();
}
