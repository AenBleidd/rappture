/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __POINTSHADER_H__
#define __POINTSHADER_H__

#include "Nv.h"
#include "NvShader.h"
#include "Texture3D.h"

class PointShader : public NvShader {
	CGparameter _modelviewVP;
	CGparameter _projectionVP;

	CGparameter _attenVP;
	CGparameter _posoffsetVP;
	CGparameter _baseposVP;
	CGparameter _scaleVP;
	CGparameter _normalParam;

	Texture3D* _normal;
public :
	PointShader();
	~PointShader();
protected :
	virtual void setParameters();
	virtual void resetParameters();

public :
	void setScale(float scale);
	void setNormalTexture(Texture3D* n);
    void bind();
    void unbind();
};

inline void PointShader::setNormalTexture(Texture3D* n)
{
	_normal = n;
}

inline void PointShader::setScale(float scale)
{
	cgGLSetParameter4f(_scaleVP, scale, 1.0f, 1.0f, 1.0f);
}

inline void PointShader::bind()
{
    setParameters();

    if (_cgVP)
    {
        cgGLBindProgram(_cgVP);
        cgGLEnableProfile((CGprofile) CG_PROFILE_VP30);
    }
    if (_cgFP)
    {
        cgGLBindProgram(_cgFP);
        cgGLEnableProfile((CGprofile) CG_PROFILE_FP30);
    }
}

inline void PointShader::unbind()
{
    if (_cgVP)
    {
        cgGLDisableProfile((CGprofile)CG_PROFILE_VP30 );
    }
    
    if (_cgFP)
    {
        cgGLDisableProfile((CGprofile)CG_PROFILE_FP30 );
    }
    
    resetParameters();
}

#endif // __POINTSHADER_H__
