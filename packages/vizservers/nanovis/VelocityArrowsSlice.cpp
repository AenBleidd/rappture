#include <GL/glew.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include "VelocityArrowsSlice.h"
#ifdef WIN32
#include <GL/glaux.h>
#else
//#include <opencv/cv.h>
//#include <opencv/highgui.h>
#endif

#ifdef USE_NANOVIS_LIB
#include "global.h"
#include "Nv.h"
#include "R2/R2FilePath.h"
#endif 

#define USE_VERTEX_BUFFER

VelocityArrowsSlice::VelocityArrowsSlice()
{
    printf("test\n");
    _enabled = true;
	_context = cgCreateContext();
	_vectorFieldGraphicsID = 0;
	_vfXscale = _vfYscale = _vfZscale = 0;
	_slicePos = 0.5f;
	_vertexBufferGraphicsID = 0;
	axis(2);
	//_renderMode = GLYPHES;
	_renderMode = LINES;
	
	_tickCountForMinSizeAxis = 10;

	_queryVelocityFP = 
#ifdef USE_NANOVIS_LIB
            LoadCgSourceProgram(_context, "queryvelocity.cg", CG_PROFILE_FP30, "main");
#else
	    cgCreateProgramFromFile(_context, CG_SOURCE, "queryvelocity.cg",
							CG_PROFILE_FP30, "main", NULL);
	    cgGLLoadProgram(_queryVelocityFP);
#endif
	_qvVectorFieldParam = cgGetNamedParameter(_queryVelocityFP, "vfield");

	_dirty = true;
	_dirtySamplingPosition = true;
	_dirtyRenderTarget = true;
	_maxVelocityScale.x = _maxVelocityScale.y = _maxVelocityScale.z = 1.0f;

	_renderTargetWidth = 128;
	_renderTargetHeight = 128;
	_velocities = new Vector3[_renderTargetWidth * _renderTargetHeight];

	::glGenBuffers(1, &_vertexBufferGraphicsID);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, _vertexBufferGraphicsID);
	glBufferDataARB(GL_ARRAY_BUFFER, _renderTargetWidth * _renderTargetHeight * 3 * sizeof(float), 
		0, GL_DYNAMIC_DRAW_ARB);
	glBindBufferARB(GL_ARRAY_BUFFER, 0);
	createRenderTarget();
	_pointCount = 0;

/*
	_particleVP = 
#ifdef USE_NANOVIS_LIB
                LoadCgSourceProgram(_context, "velocityslicevp.cg", CG_PROFILE_VP40, "vpmain");
#else
		cgCreateProgramFromFile(_context, CG_SOURCE, "velocityslicevp.cg",
							CG_PROFILE_VP40, "vpmain", NULL);
	        cgGLLoadProgram(_particleVP);
#endif
	_mvpParticleParam = cgGetNamedParameter(_particleVP, "mvp");
	_mvParticleParam = cgGetNamedParameter(_particleVP, "modelview");
	_mvTanHalfFOVParam = cgGetNamedParameter(_particleVP, "tanHalfFOV");

	// TBD..
	//_particleFP = 
#ifdef USE_NANOVIS_LIB
//                LoadCgSourceProgram(_context, "velocityslicefp.cg", CG_PROFILE_FP40, "fpmain");
#else
		cgCreateProgramFromFile(_context, CG_SOURCE, "velocityslicefp.cg",
							CG_PROFILE_FP40, "fpmain", NULL);
	        cgGLLoadProgram(_particleFP);
#endif
	_vectorParticleParam = cgGetNamedParameter(_particleVP, "vfield");
*/

#ifdef USE_NANOVIS_LIB

#ifdef WIN32
	AUX_RGBImageRec *pTextureImage = auxDIBImageLoad("arrows.bmp");
	_arrowsTex->setPixels(CF_RGB, DT_UBYTE, pTextureImage->sizeX, pTextureImage->sizeY, (void*) pTextureImage->data);

        _arrowsTex = new Texture2D(pTextureImage->sizeX, pTextureImage->sizeY, GL_FLOAT, GL_LINEAR, 3, (void*) pTextureImage->data);

	//delete pTextureImage;
#else
    /*
        printf("test1\n");
        const char *path = R2FilePath::getInstance()->getPath("arrows_flip2.png");
        if (path) printf("test2 %s\n", path);
        else printf("tt\n");
	IplImage* pTextureImage = cvLoadImage(path);
        printf("test3\n");
        if (pTextureImage)
        {
            printf("file(%s) has been loaded\n", path);
            _arrowsTex = new Texture2D(pTextureImage->width, pTextureImage->height, GL_FLOAT, GL_LINEAR, 3, (float*) pTextureImage->imageData);
            printf("file(%s) has been loaded\n", path);
	    //cvReleaseImage(&pTextureImage);
        }
        else
        {
            printf("not found\n");
        }
        if (path) delete [] path;
*/

#endif

#else
	_arrowsTex = new Texture2D();
	_arrowsTex->setWrapS(TW_MIRROR);
	_arrowsTex->setWrapT(TW_MIRROR);
#ifdef WIN32
	AUX_RGBImageRec *pTextureImage = auxDIBImageLoad("arrows.bmp");
	_arrowsTex->setPixels(CF_RGB, DT_UBYTE, pTextureImage->sizeX, pTextureImage->sizeY, (void*) pTextureImage->data);

	//delete pTextureImage;
#else
	IplImage* pTextureImage = cvLoadImage("arrows_flip2.png");
	_arrowsTex->setPixels(CF_RGB, DT_UBYTE, pTextureImage->width, pTextureImage->height, (void*) pTextureImage->imageData);

	//cvReleaseImage(&pTextureImage);
#endif

#endif
    _arrowColor.set(1, 1, 0);

    printf("test1\n");
}

VelocityArrowsSlice::~VelocityArrowsSlice()
{
    cgDestroyProgram(_queryVelocityFP);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
    glDeleteTextures(1, &_tex);
    glDeleteFramebuffersEXT(1, &_fbo);

#ifdef USE_NANOVIS_LIB
	delete _arrowsTex;
#else
	_arrowsTex->unref();
#endif
	cgDestroyProgram(_particleFP);
	cgDestroyProgram(_particleVP);

        glDeleteBuffers(1, &_vertexBufferGraphicsID);
	delete [] _velocities;
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
    //_dirty = true;
}


void VelocityArrowsSlice::queryVelocity()
{
	if (!_enabled) return;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);
	glDisable(GL_DEPTH_TEST);
	cgGLBindProgram(_queryVelocityFP);
	cgGLEnableProfile(CG_PROFILE_FP30);
	cgGLSetTextureParameter(_qvVectorFieldParam, _vectorFieldGraphicsID);
	cgGLEnableTextureParameter(_qvVectorFieldParam);

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

	for (unsigned int index = 0; index < _samplingPositions.size(); ++index)
	{
		glMultiTexCoord3f(0, _samplingPositions[index].x,_samplingPositions[index].y,_samplingPositions[index].z);
		glVertex2f(index % _renderTargetWidth,
			index / _renderTargetHeight);
	}

	glEnd();

	glDisable(GL_TEXTURE_RECTANGLE_NV);
	cgGLDisableTextureParameter(_qvVectorFieldParam);
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
	//if (!_enabled || (_vectorFieldGraphicsID == 0)) return;	
	if (!_enabled) return;

	if (_dirty)
	{
		computeSamplingTicks();
		queryVelocity();
		_dirty = false;
	}


	glPushMatrix();
	
	glScalef(_vfXscale,_vfYscale, _vfZscale);
	glTranslatef(-0.5f, -0.5f, -0.5f);
	if (_renderMode == LINES)
	{

		glDisable(GL_TEXTURE_2D);
		glLineWidth(2.0);
		glColor3f(_arrowColor.x, _arrowColor.y, _arrowColor.z);
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
					/*v = pos - pos2;
					
					
					v2.x = 1;
					v2.y = 1;
					v2.z = (-(x1-x2)-(y1-y2))/(z1-z2)
					adj = v.length() / (4 * sqrt((x3^2)+(y3^2)+(z3^2)));
					x3 *= adj
					y3 *= adj
					z3 *= adj
					*/

				}
		}
		else if (_axis == 1)
		{
			int index = 0;
			for (int z = 1; z <= _tickCountZ; ++z)
				for (int x = 1; x <= _tickCountX; ++x, ++index)
				{
					pos = _samplingPositions[index];
					pos2 = this->_velocities[index].scale(_projectionVector).scale(_maxVelocityScale) + pos;

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
					pos2 = this->_velocities[index].scale(_projectionVector).scale(_maxVelocityScale) + pos;

					glVertex3f(pos.x, pos.y, pos.z);
					glVertex3f(pos2.x, pos2.y, pos2.z);
				}
		}
		
		glEnd();
		glLineWidth(1.0);
	}
	else 
	{
		glColor3f(_arrowColor.x, _arrowColor.y, _arrowColor.z);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glEnable(GL_POINT_SPRITE_NV);
		glPointSize(20);				
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);

#ifdef USE_NANOVIS_LIB
                _arrowsTex->activate(); 
#else
		_arrowsTex->bind(0);
		glEnable(GL_TEXTURE_2D);
#endif
		glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, 0.0f );

		glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 1.0f);
		glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, 100.0f);
		glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );

		cgGLBindProgram(_particleVP);
		cgGLBindProgram(_particleFP);
		cgGLEnableProfile(CG_PROFILE_VP40);
		cgGLEnableProfile(CG_PROFILE_FP40);

		cgGLSetTextureParameter(_vectorParticleParam, _vectorFieldGraphicsID);
		cgGLEnableTextureParameter(_vectorParticleParam);


		//cgSetParameter1f(_mvTanHalfFOVParam, -tan(_fov * 0.5) * _screenHeight * 0.5);
		
		cgGLSetStateMatrixParameter(_mvpParticleParam,
						CG_GL_MODELVIEW_PROJECTION_MATRIX,
						CG_GL_MATRIX_IDENTITY);
		cgGLSetStateMatrixParameter(_mvParticleParam,
						CG_GL_MODELVIEW_MATRIX,
						CG_GL_MATRIX_IDENTITY);

		glEnableClientState(GL_VERTEX_ARRAY);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, _vertexBufferGraphicsID);
		glVertexPointer(3, GL_FLOAT, 0, 0);
		//glEnableClientState(GL_COLOR_ARRAY);

		// TBD..
		glDrawArrays(GL_POINTS, 0, _pointCount);
		glPointSize(1);
		glDrawArrays(GL_POINTS, 0, _pointCount);

		glDisableClientState(GL_VERTEX_ARRAY);
		
		cgGLDisableProfile(CG_PROFILE_VP40);
		cgGLDisableProfile(CG_PROFILE_FP40);

		glDepthMask(GL_TRUE);
		
		glDisable(GL_POINT_SPRITE_NV);
		glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
#ifdef USE_NANOVIS_LIB
                _arrowsTex->deactivate(); 
#else
                _arrowsTex->unbind();
#endif
	}
	glPopMatrix();
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

    //_dirty = true;
}

void VelocityArrowsSlice::tickCountForMinSizeAxis(int tickCount)
{
	_tickCountForMinSizeAxis = tickCount;
	
    //_dirty = true;
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

	if (_renderMode == LINES)
	{
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
	else
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, _vertexBufferGraphicsID);
		Vector3* pinfo = (Vector3*) glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);

		Vector3 pos;
		if (_axis == 2)
		{
			for (int y = 1; y <= _tickCountY; ++y)
				for (int x = 1; x <= _tickCountX; ++x)
				{
					pos.x = (1.0f / (_tickCountX + 1)) * x;
					pos.y = (1.0f / (_tickCountY + 1)) * y;
					pos.z = _slicePos;

					*pinfo = pos;
					++pinfo;
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
	
					*pinfo = pos;
					++pinfo;
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
					
					*pinfo = pos;
					++pinfo;
				}
		}

		glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	}
}

void VelocityArrowsSlice::slicePos(float pos)
{
    _slicePos = pos;
    _dirty = true;
}

