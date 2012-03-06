/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __RENDER_CONTEXT_H__
#define __RENDER_CONTEXT_H__

#include <GL/gl.h>

namespace graphics {

class RenderContext {
public :
    enum ShadingModel {
        FLAT = GL_FLAT ,	//!< Flat Shading
        SMOOTH = GL_SMOOTH	//!< Smooth shading (Goraud shading model)
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

private :
    CullMode _cullMode;
    PolygonMode _fillMode;
    ShadingModel _shadingModel;
    
public : 
    /**
     *@brief constructor
     */
    RenderContext();

    /**
     *@brief Destructor
     */
    ~RenderContext();

public :
    /**
     *@brief Set the shading model such as flat, smooth
     */
    void setShadingModel(const ShadingModel shadeModel);
    ShadingModel getShadingModel() const;

    void setCullMode(const CullMode cullMode);
    CullMode getCullMode() const;

    void setPolygonMode(const PolygonMode fillMode);
    PolygonMode getPolygonMode() const;

};

inline void RenderContext::setShadingModel(const RenderContext::ShadingModel shadeModel)
{
    _shadingModel = shadeModel;
}

inline RenderContext::ShadingModel RenderContext::getShadingModel() const
{
    return _shadingModel;
}

inline void RenderContext::setCullMode(const RenderContext::CullMode cullMode)
{
    _cullMode = cullMode;
}

inline RenderContext::CullMode RenderContext::getCullMode() const
{
    return _cullMode;
}

inline void RenderContext::setPolygonMode(const RenderContext::PolygonMode fillMode)
{
    _fillMode = fillMode;
}

inline RenderContext::PolygonMode RenderContext::getPolygonMode() const
{
    return _fillMode;
}

}

#endif //
