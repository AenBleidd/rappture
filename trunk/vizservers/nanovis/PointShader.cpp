#include "PointShader.h"
#include <R2/R2FilePath.h>
#include <R2/R2string.h>

PointShader::PointShader()
: NvShader(), _normal(0)
{
	R2string path = R2FilePath::getInstance()->getPath("pointsvp.cg");
	if (path.getLength() == 0)
	{
		path = R2FilePath::getInstance()->getPath("../examples/points/pointsvp.cg");
		if (path.getLength() == 0)
		{
			printf("ERROR : file not found [%s]\n", "pointsvp.cg");
			exit(1);
		}

	}

	this->loadVertexProgram(path, "main");


	_modelviewVP = getNamedParameterFromVP("modelview");
	_projectionVP = getNamedParameterFromVP("projection");

	_attenVP = getNamedParameterFromVP("atten");
	_posoffsetVP = getNamedParameterFromVP("posoffset");
	_baseposVP = getNamedParameterFromVP("basepos");
	_scaleVP = getNamedParameterFromVP("scale");

	_normalParam = getNamedParameterFromVP("normal");
}

PointShader::~PointShader()
{
}

void PointShader::setParameters()
{
	cgGLSetStateMatrixParameter(_modelviewVP, CG_GL_MODELVIEW_MATRIX, CG_GL_MATRIX_IDENTITY);
    cgGLSetStateMatrixParameter(_projectionVP, CG_GL_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
	
	cgGLSetParameter1f(_attenVP, 1.0f);
	cgGLSetParameter4f(_posoffsetVP, 1.0f, 1.0f, 1.0f, 1.0f);
	cgGLSetParameter4f(_baseposVP, 1.0f, 1.0f, 1.0f, 1.0f);
	cgGLSetParameter4f(_scaleVP, 1.0f, 1.0f, 1.0f, 1.0f);

	//cgGLSetTextureParameter(_normalParam,_normal->getGraphicsObjectID());
	//cgGLEnableTextureParameter(_normalParam);
	
}

void PointShader::resetParameters()
{
	//cgGLDisableTextureParameter(_normalParam);
}
