/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_COLORTABLE_SHADER_H
#define NV_COLORTABLE_SHADER_H

#include "Shader.h"
#include "Texture2D.h"
#include "TransferFunction.h"

namespace nv {

class ColorTableShader : public Shader
{
public:
    ColorTableShader();

    virtual ~ColorTableShader();

    virtual void bind(Texture2D *plane, TransferFunction *tf);

    virtual void unbind();

private :
    void init();
};

}

#endif 