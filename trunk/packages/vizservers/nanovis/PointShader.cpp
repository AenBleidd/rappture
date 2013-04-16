/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include "PointShader.h"

using namespace nv;

PointShader::PointShader() : 
    Shader(),
    _scale(1.0f),
    _normal(NULL)
{
    loadVertexProgram("pointsvp.cg");
}

PointShader::~PointShader()
{
}

void PointShader::bind()
{
    setGLStateMatrixVPParameter("modelView", MODELVIEW_MATRIX, MATRIX_IDENTITY);
    setGLStateMatrixVPParameter("modelViewProj", MODELVIEW_PROJECTION_MATRIX, MATRIX_IDENTITY);

    setVPParameter1f("atten", 1.0f);
    setVPParameter4f("posoffset", 1.0f, 1.0f, 1.0f, 1.0f);
    setVPParameter4f("basepos", 1.0f, 1.0f, 1.0f, 1.0f);
    setVPParameter4f("scale", _scale, 1.0f, 1.0f, 1.0f);
    //setVPTextureParameter("normal", _normal->getGraphicsObjectID());

    Shader::bind();
}

void PointShader::unbind()
{
    //disableVPTextureParameter("normal");

    Shader::unbind();
}
