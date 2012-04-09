/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "PointShader.h"

PointShader::PointShader() : 
    NvShader(),
    _scale(1.0f),
    _normal(NULL)
{
    loadVertexProgram("pointsvp.cg", "main");
    _modelviewVP  = getNamedParameterFromVP("modelview");
    _projectionVP = getNamedParameterFromVP("projection");
    _attenVP      = getNamedParameterFromVP("atten");
    _posoffsetVP  = getNamedParameterFromVP("posoffset");
    _baseposVP    = getNamedParameterFromVP("basepos");
    _scaleVP      = getNamedParameterFromVP("scale");
    _normalParam  = getNamedParameterFromVP("normal");
}

PointShader::~PointShader()
{
}

void PointShader::bind()
{
    cgGLSetStateMatrixParameter(_modelviewVP, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);
    cgGLSetStateMatrixParameter(_projectionVP, CG_GL_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);

    cgGLSetParameter1f(_attenVP, 1.0f);
    cgGLSetParameter4f(_posoffsetVP, 1.0f, 1.0f, 1.0f, 1.0f);
    cgGLSetParameter4f(_baseposVP, 1.0f, 1.0f, 1.0f, 1.0f);
    cgGLSetParameter4f(_scaleVP, _scale, 1.0f, 1.0f, 1.0f);

    //cgGLSetTextureParameter(_normalParam, _normal->getGraphicsObjectID());
    //cgGLEnableTextureParameter(_normalParam);

    NvShader::bind();
}

void PointShader::unbind()
{
    //cgGLDisableTextureParameter(_normalParam);

    NvShader::unbind();
}
