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
};

inline void PointShader::setNormalTexture(Texture3D* n)
{
	_normal = n;
}

inline void PointShader::setScale(float scale)
{
	cgGLSetParameter4f(_scaleVP, scale, 1.0f, 1.0f, 1.0f);
}

#endif // __POINTSHADER_H__
