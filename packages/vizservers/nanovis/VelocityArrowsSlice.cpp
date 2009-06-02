#include <GL/glew.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include "VelocityArrowsSlice.h"
#include "global.h"
#include "Nv.h"

VelocityArrowsSlice::VelocityArrowsSlice()
{
	_context = cgCreateContext();
	_vectorFieldGraphicsID = 0;
	_vfXscale = _vfYscale = _vfZscale = 0;
	_slicePos = 0.5f;
        _enabled =true;

	axis(2);

	
	_tickCountForMinSizeAxis = 10;

	_queryVelocityFP = LoadCgSourceProgram(_context, "queryvelocity.cg", 
	        CG_PROFILE_FP30, "main");
	_ipVectorFieldParam = cgGetNamedParameter(_queryVelocityFP, "vfield");

	_dirty = true;
	_dirtySamplingPosition = true;
	_dirtyRenderTarget = true;
	_maxVelocityScale.x = _maxVelocityScale.y = _maxVelocityScale.z = 1.0f;

	_renderTargetWidth = 128;
	_renderTargetHeight = 128;
	_velocities = new Vector3[_renderTargetWidth * _renderTargetHeight];
	createRenderTarget();
	_pointCount = 0;
	_vectorFieldGraphicsID = 0;
}

void VelocityArrowsSlice::createRenderTarget()
{
	glGenFramebuffersEXT(1, &_fbo);
    glGenTextures(1, &_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
    
 	glViewport(0, 0, _renderTargetWidth, _renderTargetHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _renderTargetWidth, 0, _renderTargetHeight, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, _tex);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
	_renderTargetWidth, _renderTargetHeight, 0, GL_RGBA, GL_FLOAT, NULL);

 
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		GL_TEXTURE_RECTANGLE_NV, _tex, 0);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void VelocityArrowsSlice::axis(int axis)
{
	_axis = axis;
	switch (_axis)
	{
	case 0 :
		_projectionVector.x = 0;
		_projectionVector.y = 1;
		_projectionVector.z = 1;
		break;
	case 1 :
		_projectionVector.x = 1;
		_projectionVector.y = 0;
		_projectionVector.z = 1;
		break;
	case 2 :
		_projectionVector.x = 1;
		_projectionVector.y = 1;
		_projectionVector.z = 0;
		break;
	}
	_dirtySamplingPosition = true;
	_dirtyRenderTarget = true;
        _dirty = true;
}


void VelocityArrowsSlice::queryVelocity()
{
	if (!_enabled) return;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
	glDisable(GL_DEPTH_TEST);
	cgGLBindProgram(_queryVelocityFP);
	cgGLEnableProfile(CG_PROFILE_FP30);
	cgGLSetTextureParameter(_ipVectorFieldParam, _vectorFieldGraphicsID);
	cgGLEnableTextureParameter(_ipVectorFieldParam);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _renderTargetWidth, _renderTargetHeight);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _renderTargetWidth, 0, _renderTargetHeight, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glBegin(GL_POINTS);

	for (int index = 0; index < _samplingPositions.size(); ++index)
	{
		glMultiTexCoord3f(0, _samplingPositions[index].x,_samplingPositions[index].y,_samplingPositions[index].z);
		glVertex2f(index % _renderTargetWidth,
			index / _renderTargetHeight);
	}

	glEnd();

	glDisable(GL_TEXTURE_RECTANGLE_NV);
	cgGLDisableTextureParameter(_ipVectorFieldParam);
	cgGLDisableProfile(CG_PROFILE_FP30);

	glReadPixels(0, 0, _renderTargetWidth, _renderTargetHeight, GL_RGB, GL_FLOAT, _velocities);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void VelocityArrowsSlice::render()
{
	if (!_enabled || _vectorFieldGraphicsID == 0) return;	
	if (_dirty)
	{
		computeSamplingTicks();
		queryVelocity();
		_dirty = false;
	}


	glPushMatrix();
	glScalef(_vfXscale,_vfYscale, _vfZscale);
	glTranslatef(-0.5f, -0.5f, -0.5f);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(2.0);
	glColor3f(0, 0, 1);
	glBegin(GL_LINES);
	Vector3 pos;
	Vector3 pos2;
	Vector3 vel(1, 0, 0);
	Vector3 v, v2;
	if (_axis == 2)
	{
		int index = 0;
		for (int y = 1; y <= _tickCountY; ++y)
			for (int x = 1; x <= _tickCountX; ++x, ++index)
			{
				pos = this->_samplingPositions[index];
				pos2 = this->_velocities[index].scale(_projectionVector).scale(_maxVelocityScale) + pos;
				glVertex3f(pos.x, pos.y, pos.z);
				glVertex3f(pos2.x, pos2.y, pos2.z);

			}
	}
	else if (_axis == 1)
	{
		int index = 0;
		for (int z = 1; z <= _tickCountZ; ++z)
			for (int x = 1; x <= _tickCountX; ++x, ++index)
			{
				pos = _samplingPositions[index];
				pos2 = _velocities[index].scale(_projectionVector).scale(_maxVelocityScale) + pos;

				glVertex3f(pos.x, pos.y, pos.z);
				glVertex3f(pos2.x, pos2.y, pos2.z);
			}
	}
	else if (_axis == 0)
	{
		int index = 0;
		for (int z = 1; z <= _tickCountZ; ++z)
			for (int y = 1; y <= _tickCountY; ++y, ++index)
			{
				pos = _samplingPositions[index];
				pos2 = _velocities[index].scale(_projectionVector).scale(_maxVelocityScale) + pos;

				glVertex3f(pos.x, pos.y, pos.z);
				glVertex3f(pos2.x, pos2.y, pos2.z);
			}
	}
	
	glEnd();
	glPopMatrix();
	glLineWidth(1.0);
}

void VelocityArrowsSlice::enabled(bool e)
{
	_enabled = e;
}

void VelocityArrowsSlice::vectorField(unsigned int vfGraphicsID, 
				float xScale, float yScale, float zScale)
{
	_vectorFieldGraphicsID = vfGraphicsID;
	_vfXscale = xScale;
	_vfYscale = yScale;
	_vfZscale = zScale;
}

void VelocityArrowsSlice::tickCountForMinSizeAxis(int tickCount)
{
	_tickCountForMinSizeAxis = tickCount;
	
}


void VelocityArrowsSlice::computeSamplingTicks()
{
	if (_vfXscale < _vfYscale)
	{
		if (_vfXscale < _vfZscale)
		{
			// vfXscale
			_tickCountX = _tickCountForMinSizeAxis;
			
			float step = _vfXscale / (_tickCountX + 1);
			
			_tickCountY = (int) (_vfYscale/step);
			_tickCountZ = (int) (_vfZscale/step);
                        //vscalex = 1;
                        //vscaley = _vfXscale / _vfXscale;
                        //vscalez = _vfZscale / _vfXscale;
		}
		else
		{
			// vfZscale
			_tickCountZ = _tickCountForMinSizeAxis;

			float step = _vfZscale / (_tickCountZ + 1);
			_tickCountX = (int) (_vfXscale/step);
			_tickCountY = (int) (_vfYscale/step);
                        //vscalex = _vfXscale / _vfZscale;
                        //vscaley = _vfYscale / _vfZscale;
                        //vscalez = 1;
		}

	}
	else 
	{
		if (_vfYscale < _vfZscale)
		{
			// _vfYscale
			_tickCountY = _tickCountForMinSizeAxis;

			float step = _vfYscale / (_tickCountY + 1);
			_tickCountX = (int) (_vfXscale/step);
			_tickCountZ = (int) (_vfZscale/step);

                        //vscalex = _vfXscale / _vfYscale;
                        //vscaley = 1;
                        //vscalez = _vfZscale / _vfYscale;
		}
		else
		{
			// vfZscale
			_tickCountZ = _tickCountForMinSizeAxis;
			
			float step = _vfZscale / (_tickCountZ + 1);
			_tickCountX = (int) (_vfXscale/step);
			_tickCountY = (int) (_vfYscale/step);

                        //vscalex = _vfXscale / _vfZscale;
                        //vscaley = _vfYscale / _vfZscale;
                        //vscalez = 1;
		
			
		}
	}
	
	_maxVelocityScale.x = 1.0f / _tickCountX * 0.8f;
	_maxVelocityScale.y = 1.0f / _tickCountY * 0.8f;
	_maxVelocityScale.z = 1.0f / _tickCountZ * 0.8f;
	
	int pointCount = _tickCountX * _tickCountY * _tickCountZ;
	if (_pointCount != pointCount)
	{
		_samplingPositions.clear();
		_samplingPositions.reserve(pointCount);
		_pointCount = pointCount;
	}


	Vector3 pos;
	if (_axis == 2)
	{
		for (int y = 1; y <= _tickCountY; ++y)
			for (int x = 1; x <= _tickCountX; ++x)
			{
				pos.x = (1.0f / (_tickCountX + 1)) * x;
				pos.y = (1.0f / (_tickCountY + 1)) * y;
				pos.z = _slicePos;
				_samplingPositions.push_back(pos);
			}
	}
	else if (_axis == 1)
	{
		for (int z = 1; z <= _tickCountZ; ++z)
			for (int x = 1; x <= _tickCountX; ++x)
			{
				pos.x = (1.0f / (_tickCountX + 1)) * x;
				pos.y = _slicePos;
				pos.z = (1.0f / (_tickCountZ + 1)) * z;
				_samplingPositions.push_back(pos);
			}
	}
	else if (_axis == 0)
	{
		for (int z = 1; z <= _tickCountZ; ++z)
			for (int y = 1; y <= _tickCountY; ++y)
			{
				pos.x = _slicePos;
				pos.y = (1.0f / (_tickCountY + 1)) * y;
				pos.z = (1.0f / (_tickCountZ + 1)) * z;
				_samplingPositions.push_back(pos);
			}
	}
}

void VelocityArrowsSlice::slicePos(float pos) 
{
    // TBD..
    //_slicePos = pos;
    _dirty = true;
}
