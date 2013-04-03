/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "NvStdVertexShader.h"

using namespace nv;

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

void NvStdVertexShader::bind(float *mvp, float *mvInv)
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

    NvShader::bind();
}
