/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
}

void NvStdVertexShader::bind()
{
    setGLStateMatrixVPParameter("modelViewProjMatrix", MODELVIEW_PROJECTION_MATRIX, MATRIX_IDENTITY);
    setGLStateMatrixVPParameter("modelViewInv", MODELVIEW_MATRIX, MATRIX_INVERSE);

    NvShader::bind();
}
