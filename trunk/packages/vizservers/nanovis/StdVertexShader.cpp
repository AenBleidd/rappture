/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "StdVertexShader.h"

using namespace nv;

StdVertexShader::StdVertexShader()
{
    init();
}

StdVertexShader::~StdVertexShader()
{
}

void StdVertexShader::init()
{
    loadVertexProgram("vertex_std.cg", "main");
}

void StdVertexShader::bind(float *mvp, float *mvInv)
{
    if (mvp != NULL) {
        setVPMatrixParameterf("modelViewProjMatrix", mvp);
    } else {
        setGLStateMatrixVPParameter("modelViewProjMatrix", MODELVIEW_PROJECTION_MATRIX);
    }
    if (mvInv != NULL) {
        setVPMatrixParameterf("modelViewInv", mvInv);
    } else {
        setGLStateMatrixVPParameter("modelViewInv", MODELVIEW_MATRIX, MATRIX_INVERSE);
    }

    Shader::bind();
}
