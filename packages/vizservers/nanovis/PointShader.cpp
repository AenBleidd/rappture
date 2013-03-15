/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "PointShader.h"

PointShader::PointShader() : 
    NvShader(),
    _scale(1.0f),
    _normal(NULL)
{
    loadVertexProgram("pointsvp.cg", "main");
}

PointShader::~PointShader()
{
}

void PointShader::bind()
{
    setGLStateMatrixVPParameter("modelview", MODELVIEW_MATRIX, MATRIX_IDENTITY);
    setGLStateMatrixVPParameter("projection", PROJECTION_MATRIX, MATRIX_IDENTITY);

    setVPParameter1f("atten", 1.0f);
    setVPParameter4f("posoffset", 1.0f, 1.0f, 1.0f, 1.0f);
    setVPParameter4f("basepos", 1.0f, 1.0f, 1.0f, 1.0f);
    setVPParameter4f("scale", _scale, 1.0f, 1.0f, 1.0f);
    //setVPTextureParameter("normal", _normal->getGraphicsObjectID());

    NvShader::bind();
}

void PointShader::unbind()
{
    //disableVPTextureParameter("normal");

    NvShader::unbind();
}
