/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_COLORTABLE_SHADER_H
#define NV_COLORTABLE_SHADER_H

#include <Cg/cg.h>

#include "Texture2D.h"
#include "TransferFunction.h"
#include "NvShader.h"

class NvColorTableShader : public NvShader
{
public:
    NvColorTableShader();

    ~NvColorTableShader();

    void bind(Texture2D *plane, TransferFunction *tf);

    void unbind();

private :
    void init();

    CGparameter _dataParam;
    CGparameter _tfParam;
    CGparameter _renderParam;
};

#endif 
