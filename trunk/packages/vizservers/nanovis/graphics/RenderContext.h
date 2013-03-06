/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 */
#ifndef NV_GRAPHICS_RENDER_CONTEXT_H
#define NV_GRAPHICS_RENDER_CONTEXT_H

#include <GL/gl.h>

namespace nv {
namespace graphics {

class RenderContext
{
public:
    enum ShadingModel {
        FLAT = GL_FLAT ,	//!< Flat Shading
        SMOOTH = GL_SMOOTH	//!< Smooth shading (Gouraud shading model)
    };

    enum PolygonMode {
        LINE = GL_LINE,
        FILL = GL_FILL
    };

    enum CullMode {
        NO_CULL,		//!< No culling
        BACK= GL_BACK,		//!< Back face culling
        FRONT= GL_FRONT		//!< Front face culling
    };

    RenderContext();

    ~RenderContext();

    /**
     *@brief Set the shading model such as flat, smooth
     */
    void setShadingModel(const ShadingModel shadeModel)
    {
        _shadingModel = shadeModel;
    }

    ShadingModel getShadingModel() const
    {
        return _shadingModel;
    }

    void setCullMode(const CullMode cullMode)
    {
        _cullMode = cullMode;
    }

    CullMode getCullMode() const
    {
        return _cullMode;
    }

    void setPolygonMode(const PolygonMode fillMode)
    {
        _fillMode = fillMode;
    }

    PolygonMode getPolygonMode() const
    {
        return _fillMode;
    }

private :
    CullMode _cullMode;
    PolygonMode _fillMode;
    ShadingModel _shadingModel;
};

}
}

#endif
