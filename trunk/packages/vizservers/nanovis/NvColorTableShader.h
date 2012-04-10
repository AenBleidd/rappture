/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef NV_COLORTABLE_SHADER_H
#define NV_COLORTABLE_SHADER_H

#include "NvShader.h"
#include "Texture2D.h"
#include "TransferFunction.h"

class NvColorTableShader : public NvShader
{
public:
    NvColorTableShader();

    virtual ~NvColorTableShader();

    virtual void bind(Texture2D *plane, TransferFunction *tf);

    virtual void unbind();

private :
    void init();
};

#endif 
