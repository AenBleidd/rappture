/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <vr3d/vr3d.h>
#include <vr3d/vrTexture3D.h>

#include <Cg/cgGL.h>

#include <vrutil/vrFilePath.h>

#include <vrmath/vrMatrix4x4f.h>
#include <vrmath/vrVector3f.h>
#include <vrmath/vrVector4f.h>

#include "particlemacro.h"
#ifdef _WIN32
#include <GL/glaux.h>
#else
#ifdef notdef
#include <opencv/cv.h>
#include <opencv/highgui.h>
#endif
#endif

#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "Trace.h"

#define USE_RGBA_ARROW

//#define TEST
// TEST
extern void* LoadFlowDx(const char* fname, int& width, int& height, int&depth, float& min, float& max, float& nonzero_min, 
			 float& axisScaleX, float& axisScaleY, float& axisScaleZ);
extern void* LoadFlowRaw(const char* fname, int width, int height, int depth, float& min, float& max, float& nonzero_min, 
			 float& axisScaleX, float& axisScaleY, float& axisScaleZ, int skipByte, bool bigEndian);
extern void* LoadProcessedFlowRaw(const char* fname, int width, int height, int depth, float min, float max, float& axisScaleX, float& axisScaleY, float& axisScaleZ, int skipByte);
extern void LoadProcessedFlowRaw(const char* fname, int width, int height, int depth, float* data);
inline float randomRange(float rangeMin, float rangeMax)
{
	return ((float)rand() / RAND_MAX) * (rangeMax - rangeMin) + rangeMin ;
}

inline int logd(int value)
{
	int l;
	for (l = 0; value >> l; l++);
	return l - 1;
}

inline void swap(int& x, int& y)
{
	if (x != y)
	{
		x ^= y;
		y ^= x;
		x ^= y;
	}
}

void* ParticleSystem::dataLoadMain(void* data)
{
	
	ParticleSystem* particleSystem = (ParticleSystem*) data;
	CircularFifo<float*, 10>& queue = particleSystem->_queue;
	
	// TBD..
	//float min = particleSystem->_time_series_vel_mag_min;
	//float max = particleSystem->_time_series_vel_mag_max;

	int startIndex = particleSystem->_flowFileStartIndex;
	int endIndex = particleSystem->_flowFileEndIndex;

	int curIndex = startIndex;
	float flowWidth = particleSystem->_flowWidth;
	float flowHeight = particleSystem->_flowHeight;
	float flowDepth = particleSystem->_flowDepth;
	
	const char* fileNameFormat = particleSystem->_fileNameFormat.c_str();
	char buff[256];

	bool done = false;
	while (!done)
	{
		if (curIndex > endIndex) 
		{
			curIndex = startIndex;
		}

		if (!queue.isFull())
		{
			int tail = queue.tailIndex();
			if (tail != -1)
			{
				sprintf(buff, fileNameFormat, curIndex);
				//std::string path = vrFilePath::getInstance()->getPath(buff);
				float t = clock() /(float) CLOCKS_PER_SEC;
				LoadProcessedFlowRaw(buff, flowWidth, flowHeight, flowDepth, queue.array[tail]);
				float ti = clock() / (float) CLOCKS_PER_SEC;
				printf("%f\n",ti - t);
				queue.push();
				TRACE("%d loaded\n", curIndex);
				++curIndex;
			}
		}
		else
		{
			//TRACE("full\n");
		}
	}

	return 0;
}


CGcontext ParticleSystem::_context = 0;

unsigned int SpotTexID = 0;

void MakeSphereTexture()
{

    const int DIM = 128;
    const int DIM2 = 63;
    //const float TEX_SCALE = 6.0;

    glGenTextures(1, (GLuint *)&SpotTexID);
    glBindTexture(GL_TEXTURE_2D, SpotTexID);

	float col[4] = {1.f, 1.f, 1.f, 1.f};
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, col);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


    float *img = new float[DIM*DIM];

    vrVector3f light(1,1,3);
    light.normalize();

    for(int y=0; y<DIM; y++) {
        for(int x=0; x<DIM; x++) {
            // Clamping the edges to zero allows Nvidia's blend optimizations to do their thing.
            if(x==0 || x==DIM-1 || y==0 || y==DIM-1)
                img[y*DIM+x] = 0;
            else {
                vrVector3f p(x, y, 0);
                p = p - vrVector3f(DIM2, DIM2, 0);
                float len = p.length();
                float z = sqrt(DIM2*DIM2 - len*len);
                p.z = z;
                if(len >= DIM2) {
                    img[y*DIM+x] = 0.0;
                    continue;
                }

                p.normalize();
                float v = p.dot(light);
                if(v < 0) v = 0;

                img[y*DIM+x] = v;
            }
        }
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);


    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_ALPHA16, DIM, DIM, GL_ALPHA, GL_FLOAT, img);
}

//extern void algorithm_test(vrVector4f* data, int flowWidth, int flowHeight, int flowDepth);
//extern std::vector<vrVector3f>* find_critical_points(vrVector4f* data, int flowWidth, int flowHeight, int flowDepth);
ParticleSystem::ParticleSystem(int width, int height, const std::string& fileName, int fieldWidth, int fieldHeight, int fieldDepth, 
							   bool timeVaryingData, int flowFileStartIndex, int flowFileEndIndex)
: _width(width), _height(height)
{
	////////////////////////////////
	// TIME-VARYING SERIES
	_isTimeVaryingField = timeVaryingData;
	_flowFileStartIndex = flowFileStartIndex;
	_flowFileEndIndex = flowFileEndIndex;
	_userDefinedParticleMaxCount = width * height;


	_skipByte = 0;

	_screenWidth = _screenHeight = 1;
	_fov = 60.0f;

	_particleMaxCount = _width * _height;

	_pointSize = 10.0f;
	_camx = 0.0f;
	_camy = 0.0f;
	_camz = 0.0f;

	_scalex = _scaley = _scalez = 1.0f;
	_currentSortIndex = 0;
	_destSortIndex = 1;

	_currentPosIndex = 0;
	_destPosIndex = 1;

	_currentSortPass = 0;
	// TBD
	//_sortPassesPerFrame = 5;
	_sortPassesPerFrame = 4;
	_maxSortPasses = 0;

	_sortBegin = 0;
	_sortEnd = _sortPassesPerFrame;
	_currentTime = 0;
	_sortEnabled = false;
	_glypEnabled = false;
	_advectionEnabled = false;
	_streamlineEnabled = false;
	_particles = new Particle[_particleMaxCount];
	memset(_particles, 0, sizeof(Particle) * _particleMaxCount);

	_colorBuffer = new color4[_particleMaxCount];
	memset(_colorBuffer, 0, sizeof(color4) * _particleMaxCount);

	_positionBuffer = new float3[_particleMaxCount];
	_vertices = new RenderVertexArray(_particleMaxCount, 3, GL_FLOAT);

	srand(time(0));

	/*
	// TBD..
	glGenBuffersARB(1, &_colorBufferID);
	glBindBufferARB(GL_ARRAY_BUFFER, _colorBufferID);
	glBufferDataARB(GL_ARRAY_BUFFER, _width * _height * 4, _colorBuffer, GL_STREAM_DRAW_ARB); // undefined data
	glBindBufferARB(GL_ARRAY_BUFFER, 0);
	*/
	
	_flowWidth = fieldWidth;
	_flowHeight = fieldHeight;
	_flowDepth = fieldDepth;

	float min, max, nonzero_min, axisScaleX, axisScaleY, axisScaleZ;
	int index = fileName.rfind('.');
	std::string ext = fileName.substr(index + 1, fileName.size() - (index + 1));
	float* data = 0;

	
	if (this->isTimeVaryingField())
	{
		axisScaleX = _flowWidth;
		axisScaleY = _flowHeight;
		axisScaleZ = _flowDepth;
		_fileNameFormat = fileName;
		for (int i = 0; i < CircularFifo<float*, 10>::Capacity; ++i)
		{
			_queue.array[i] = new float[_flowWidth * _flowHeight * _flowDepth * 4];
		}
		
		
		// INSOO
		// TBD
		min = 0;
		max = 234.99;
	}
	else
	{
		if (ext == "dx")
		{
			data = (float*) LoadFlowDx(fileName.c_str(), _flowWidth, _flowHeight, _flowDepth, min, max, nonzero_min, axisScaleX, axisScaleY, axisScaleZ);
		}
		else if (ext == "raw")
		{
			data = (float*) LoadFlowRaw(fileName.c_str(), _flowWidth, _flowHeight, _flowDepth, min, max, nonzero_min, axisScaleX, axisScaleY, axisScaleZ, 0, false);
			//data = (float*) LoadProcessedFlowRaw(fileName.c_str(), _flowWidth, _flowHeight, _flowDepth, min, max, axisScaleX, axisScaleY, axisScaleZ);
		}
		/*
		else if (ext == "raws")
		{
			data = (float*) LoadFlowRaw(fileName.c_str(), _flowWidth, _flowHeight, _flowDepth, min, max, nonzero_min, axisScaleX, axisScaleY, axisScaleZ, 5 * sizeof(int), true);
		}
		else if (ext == "data")
		{
			data = (float*) LoadFlowRaw(fileName.c_str(), _flowWidth, _flowHeight, _flowDepth, min, max, nonzero_min, axisScaleX, axisScaleY, axisScaleZ, 0, true);
		}
		*/
	

		//_criticalPoints = find_critical_points((vrVector4f*) data, _flowWidth,  _flowHeight,  _flowDepth);
		//algorithm_test((vrVector4f*) data,  _flowWidth,  _flowHeight,  _flowDepth);
	}

	_scalex = 1;
	_scaley = axisScaleY / axisScaleX;
	_scalez = axisScaleZ / axisScaleX;
	_maxVelocityScale = max;

	if (!this->isTimeVaryingField())
	{
		vrTexture3D* flow = new vrTexture3D();
		flow->setMinFilter(TF_LINEAR);
		flow->setMagFilter(TF_LINEAR);
		flow->setPixels(CF_RGBA, DT_FLOAT, _flowWidth, _flowHeight, _flowDepth, data);
		this->_curVectorFieldID = flow->getGraphicsObjectID();
		_vectorFieldIDs.push_back(_curVectorFieldID);
	}
	else
	{
		for (int i = 0; i < 3; ++i)
		{
			vrTexture3D* flow = new vrTexture3D();
			flow->setMinFilter(TF_LINEAR);
			flow->setMagFilter(TF_LINEAR);
			flow->setPixels(CF_RGBA, DT_FLOAT, _flowWidth, _flowHeight, _flowDepth, data);
			_vectorFields.push_back(flow);
			_vectorFieldIDs.push_back(flow->getGraphicsObjectID());
		}

		this->_curVectorFieldID = _vectorFieldIDs[0];
	}

	_arrows = new vrTexture2D();
	_arrows->setWrapS(TW_MIRROR);
	_arrows->setWrapT(TW_MIRROR);

#ifdef _WIN32
#ifndef USE_RGBA_ARROW
	std::string path = vrFilePath::getInstance()->getPath("arrows.bmp");
	AUX_RGBImageRec *pTextureImage = auxDIBImageLoad(path.c_str());
	_arrows->setPixels(CF_RGB, DT_UBYTE, pTextureImage->sizeX, pTextureImage->sizeY, (void*) pTextureImage->data);
#else
	std::string path = vrFilePath::getInstance()->getPath("arrows_red_bg.bmp");
	AUX_RGBImageRec *pTextureImage = auxDIBImageLoad(path.c_str());
	unsigned char* pixels = new unsigned char [pTextureImage->sizeX * pTextureImage->sizeY * sizeof(unsigned char) * 4];
	unsigned char* srcPixels = pTextureImage->data;
	unsigned char* dstPixels = pixels;
	for (int i = 0; i < pTextureImage->sizeX * pTextureImage->sizeY; ++i)
	{
		*dstPixels = *srcPixels; ++srcPixels;
		*(dstPixels + 1) = *srcPixels; ++srcPixels;
		*(dstPixels + 2) = *srcPixels; ++srcPixels;

		if ((*dstPixels > 127) && (*(dstPixels + 1) < 127) && (*(dstPixels + 2) < 127))
		{
			*(dstPixels + 3) = 0;
		}
		else
		{
			*(dstPixels + 3) = 255;
		}

		dstPixels += 4;
	}
	_arrows->setPixels(CF_RGBA, DT_UBYTE, pTextureImage->sizeX, pTextureImage->sizeY, (void*) pixels);
#endif

#else
#ifndef USE_RGBA_ARROW
	IplImage* pTextureImage = cvLoadImage("arrows_flip2.png");
	_arrows->setPixels(CF_RGB, DT_UBYTE, pTextureImage->width, pTextureImage->height, (void*) pTextureImage->imageData);
#else
	// TBD..
	/*
	std::string path = vrFilePath::getInstance()->getPath("arrows_red_bg.bmp");
	AUX_RGBImageRec *pTextureImage = auxDIBImageLoad(path.c_str());
	unsigned char* pixels = new unsigned char [pTextureImage->sizeX * pTextureImage->sizeY * sizeof(unsigned char) * 4];
	unsigned char* srcPixels = pTextureImage->data;
	unsigned char* dstPixels = pixels;
	for (int i = 0; i < pTextureImage->sizeX * pTextureImage->sizeY; ++i)
	{
		*dstPixels = *srcPixels; ++srcPixels;
		*(dstPixels + 1) = *srcPixels; ++srcPixels;
		*(dstPixels + 2) = *srcPixels; ++srcPixels;

		if ((*dstPixels > 127) && (*(dstPixels + 1) < 127) && (*(dstPixels + 2) < 127))
		{
			*(dstPixels + 3) = 0;
		}
		else
		{
			*(dstPixels + 3) = 255;
		}

		dstPixels += 4;
	}

	_arrows->setPixels(CF_RGBA, DT_UBYTE, pTextureImage->sizeX, pTextureImage->sizeY, (void*) pixels);
	*/

#endif // USE_RGBA_ARROW
#endif // _WIN32

#ifdef TEST
	MakeSphereTexture();
#endif
	
	glGenBuffersARB(1, &_particleInfoBufferID);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, _particleInfoBufferID);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, _particleMaxCount * 4 * sizeof(float), 0, GL_DYNAMIC_DRAW);
	glVertexAttribPointerARB(8, 4, GL_FLOAT, GL_FALSE, 0, 0);  // 8 = texcoord0
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	
	
	// TBD..
	initStreamlines(_width, _height);
	initInitPosTex();

	initShaders();
	createRenderTargets();
	createSortRenderTargets();
	createStreamlineRenderTargets();

	if (this->isTimeVaryingField())
	{
		pthread_t _threads;
		pthread_attr_t pthread_custom_attr;
		pthread_attr_init(&pthread_custom_attr);
		pthread_create(&_threads, &pthread_custom_attr, dataLoadMain, (void *) this);
	}
}

void ParticleSystem::initInitPosTex()
{
	glGenFramebuffersEXT(1, &_initPos_fbo);
    glGenTextures(1, &_initPosTex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _initPos_fbo);
    
 	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, _initPosTex);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGB_NV,
        		_width, _height, 0, GL_RGB, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 0);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		GL_TEXTURE_RECTANGLE_NV, _initPosTex, 0);
	
  
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void ParticleSystem::initStreamlines(int width, int height)
{
	_atlas_x = 32;
	_atlas_y = 32;

	_atlasWidth = width * _atlas_x;
	_atlasHeight = height * _atlas_y;
	_maxAtlasCount = _atlas_x * _atlas_y;

	_stepSizeStreamline = 1;
	_curStepCount = 0;
	_particleCount = width * height;

	_currentStreamlineOffset = 0;
	_currentStreamlineIndex = 0;
	_streamVertices = new RenderVertexArray(_atlasWidth*_atlasHeight * 2, 3, GL_FLOAT);

	_maxIndexBufferSize = _width * _height * _atlas_x * _atlas_y;
	_indexBuffer = new unsigned int[_maxIndexBufferSize * 2];
	unsigned int* pindex = _indexBuffer;
	int ax, ay, old_ax = 0, old_ay = 0;
	for (unsigned int i = 1; i < _maxAtlasCount; ++i)
	{
		ax = (i % _atlas_x) * width;	
		ay = (i / _atlas_x) * height;
		
		for (int y = 0; y < height ; ++y)
			for (int x = 0; x < width; ++x)
			{
				*pindex = (old_ay + y) * _atlasWidth + old_ax + x; ++pindex;
				*pindex = (ay + y) * _atlasWidth + ax + x; ++pindex;
			}

		old_ax = ax;
		old_ay = ay;
	}
}

void ParticleSystem::callbackForCgError()
{
	CGerror lastError = cgGetError();
    if(lastError) {
        const char *listing = cgGetLastListing(_context);
        ERROR("\n---------------------------------------------------\n");
        ERROR("%s\n\n", cgGetErrorString(lastError));
        ERROR("%s\n", listing);
        ERROR("-----------------------------------------------------\n");
        ERROR("Cg error, exiting...\n");

        cgDestroyContext(_context);
        getchar();
        exit(-1);
    }
}

void ParticleSystem::initShaders()
{
	// TBD...
	_context = cgCreateContext();
	cgSetErrorCallback(callbackForCgError);

	std::string path = vrFilePath::getInstance()->getPath("distance.cg");
	_distanceInitFP =  
		cgCreateProgramFromFile(_context, CG_SOURCE, path.c_str(),
							CG_PROFILE_FP30, "initSortIndex", NULL);
	cgGLLoadProgram(_distanceInitFP);

	_distanceSortFP = 
		cgCreateProgramFromFile(_context, CG_SOURCE,  path.c_str(),
							CG_PROFILE_FP30, "computeDistance", NULL);
	_viewPosParam = cgGetNamedParameter(_distanceSortFP, "viewerPosition");
	cgGLLoadProgram(_distanceSortFP);

	_distanceSortLookupFP = 
		cgCreateProgramFromFile(_context, CG_SOURCE,  path.c_str(),
							CG_PROFILE_FP30, "lookupPosition", NULL);
	cgGLLoadProgram(_distanceSortLookupFP);

	path = vrFilePath::getInstance()->getPath("mergesort.cg");
	_sortRecursionFP = 
		cgCreateProgramFromFile(_context, CG_SOURCE, path.c_str(),
							CG_PROFILE_FP30, "mergeSortRecursion", NULL);
	cgGLLoadProgram(_sortRecursionFP);

	_srSizeParam = cgGetNamedParameter(_sortRecursionFP, "_Size");
	_srStepParam = cgGetNamedParameter(_sortRecursionFP, "_Step");
	_srCountParam = cgGetNamedParameter(_sortRecursionFP, "_Count");

	_sortEndFP = 
		cgCreateProgramFromFile(_context, CG_SOURCE, path.c_str(),
							CG_PROFILE_FP30, "mergeSortEnd", NULL);
	cgGLLoadProgram(_sortEndFP);

	_seSizeParam = cgGetNamedParameter(_sortEndFP, "size");
	_seStepParam = cgGetNamedParameter(_sortEndFP, "step");

	path = vrFilePath::getInstance()->getPath("passthrough.cg");
	_passthroughFP = 
		cgCreateProgramFromFile(_context, CG_SOURCE, path.c_str(),
							CG_PROFILE_FP30, "main", NULL);
	cgGLLoadProgram(_passthroughFP);
	
	_scaleParam = cgGetNamedParameter(_passthroughFP, "scale");
	_biasParam = cgGetNamedParameter(_passthroughFP, "bias");


	path = vrFilePath::getInstance()->getPath("moveparticles.cg");
	_moveParticlesFP = 
		cgCreateProgramFromFile(_context, CG_SOURCE, path.c_str(),
							CG_PROFILE_FP30, "main", NULL);
	cgGLLoadProgram(_moveParticlesFP);
	_mpTimeScale = cgGetNamedParameter(_moveParticlesFP, "timeStep");
	_mpVectorField = cgGetNamedParameter(_moveParticlesFP, "vfield");
	_mpUseInitTex= cgGetNamedParameter(_moveParticlesFP, "useInitPos");
	_mpCurrentTime= cgGetNamedParameter(_moveParticlesFP, "currentTime");
	_mpMaxScale = cgGetNamedParameter(_moveParticlesFP, "maxScale");
	_mpScale= cgGetNamedParameter(_moveParticlesFP, "scale");

	
	path = vrFilePath::getInstance()->getPath("particlevp.cg");
	_particleVP = 
		cgCreateProgramFromFile(_context, CG_SOURCE, path.c_str(),
							CG_PROFILE_VP40, "vpmain", NULL);
	cgGLLoadProgram(_particleVP);
	_mvpParticleParam = cgGetNamedParameter(_particleVP, "mvp");
	_mvParticleParam = cgGetNamedParameter(_particleVP, "modelview");
	_mvTanHalfFOVParam = cgGetNamedParameter(_particleVP, "tanHalfFOV");
	_mvCurrentTimeParam =  cgGetNamedParameter(_particleVP, "currentTime");

	path = vrFilePath::getInstance()->getPath("particlefp.cg");
	_particleFP = 
		cgCreateProgramFromFile(_context, CG_SOURCE, path.c_str(),
							CG_PROFILE_FP40, "fpmain", NULL);
	cgGLLoadProgram(_particleFP);
	_vectorParticleParam = cgGetNamedParameter(_particleVP, "vfield");
	
	path = vrFilePath::getInstance()->getPath("moveparticles.cg");
	_initParticlePosFP = 
		cgCreateProgramFromFile(_context, CG_SOURCE, path.c_str(),
				CG_PROFILE_FP30, "initParticlePosMain", NULL);
	cgGLLoadProgram(_initParticlePosFP);
	_ipVectorFieldParam = cgGetNamedParameter(_moveParticlesFP, "vfield");
}

void ParticleSystem::passThoughPositions()
{
	cgGLBindProgram(_passthroughFP);
	cgGLEnableProfile(CG_PROFILE_FP30);

	cgGLSetParameter4f(_scaleParam, 1.0, 1.0, 1.0, 1.0);
	cgGLSetParameter4f(_biasParam, 0.0, 0.0, 0.0, 0.0);

	
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGB_NV, 
		_width, _height, 0, GL_RGB, GL_FLOAT, (GLfloat *) _positionBuffer);
	
	drawQuad();

	cgGLDisableProfile(CG_PROFILE_FP30);
}


void ParticleSystem::resetStreamlines()
{
	_currentStreamlineIndex = 0;
	_currentStreamlineOffset = 0;
	_curStepCount = 0;
}

void ParticleSystem::reset()
{
	_currentTime = 0.0f;
	_sortBegin = 0;
	_sortEnd = _sortPassesPerFrame;

	glPushAttrib(GL_VIEWPORT_BIT);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);

	resetStreamlines();

	unsigned int maxsize = _userDefinedParticleMaxCount;

	while (!_availableIndices.empty())
		_availableIndices.pop();
	while (!_activeParticles.empty())
		_activeParticles.pop();

	_activeParticles.reserve(maxsize);
	_availableIndices.reserve(maxsize);
	

	for (unsigned int i = 0; i < maxsize; i++)
		_availableIndices.push(i);

	for (int i = 0; i < _particleMaxCount; i++)
	{
		_particles[i].timeOfBirth = -1.0f;
		_particles[i].lifeTime = 0.0f;
		_particles[i].attributeType = 0.0f;
		_particles[i].index = (float)i;
	}

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, _particleInfoBufferID);
	Particle* pinfo = (Particle*) glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);

	for (int i = 0; i < _particleMaxCount; i++)
	{
		pinfo[i] = _particles[i];
	}
	
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);

	// POSITION
	memset(_positionBuffer, 0, sizeof(float3) * _width * _height);
	for(int y = 0; y < _height; y++) 
	{
		for(int x = 0; x < _width; x++) 
		{
			_positionBuffer[_width * y + x].x = (float)rand() / (float) RAND_MAX;
			//_positionBuffer[_width * y + x].x = 0.9;
			_positionBuffer[_width * y + x].y = (float)rand() / (float) RAND_MAX;
			_positionBuffer[_width * y + x].z = (float)rand() / (float) RAND_MAX;
		}
	}

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, _initPosTex);
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGB_NV,
                 _width, _height, 0, GL_RGB, GL_FLOAT, (float*)_positionBuffer);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, 0);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_currentPosIndex]);
	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	passThoughPositions();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, velocity_fbo[_currentPosIndex]);
	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	// TBD..
	// velocity
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);


	/// TBD...
	//DeathTimePBuffer->BeginCapture();
	//glClear(GL_COLOR_BUFFER_BIT);
	//DeathTimePBuffer->EndCapture();

	if (_sortEnabled)
	{
		glDisable(GL_BLEND);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[_currentSortIndex]);
		glViewport(0, 0, _width, _height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		cgGLBindProgram(_distanceInitFP);
		cgGLEnableProfile(CG_PROFILE_FP30);
		
		drawQuad();

		cgGLDisableProfile(CG_PROFILE_FP30);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);		

		_currentSortPass = 0;
		this->_maxSortPasses = 100000;
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopAttrib();
}

void ParticleSystem::createRenderTargets()
{
	glGenFramebuffersEXT(2, psys_fbo);
    glGenTextures(2, psys_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);
    
 	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[0]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
	_width, _height, 0, GL_RGBA, GL_FLOAT, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
		GL_TEXTURE_RECTANGLE_NV, psys_tex[0], 0);
	
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[1]);
 	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_FLOAT_RGBA32_NV, 
	_width, _height, 0, GL_RGBA, GL_FLOAT, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
	GL_TEXTURE_RECTANGLE_NV, psys_tex[1], 0);
 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

//#define ATLAS_TEXTURE_TARGET GL_TEXTURE_RECTANGLE_NV
//#define ATLAS_INTERNAL_TYPE GL_FLOAT_RGBA32_NV
#define ATLAS_TEXTURE_TARGET GL_TEXTURE_2D
#define ATLAS_INTERNAL_TYPE GL_RGBA

void ParticleSystem::createStreamlineRenderTargets()
{
	glGenFramebuffersEXT(1, &_atlas_fbo);
	glGenTextures(1, &_atlas_tex);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _atlas_fbo);
 	glViewport(0, 0, _atlasWidth, _atlasHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _atlasWidth, 0, _atlasHeight, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBindTexture(ATLAS_TEXTURE_TARGET, _atlas_tex);
	glTexParameterf(ATLAS_TEXTURE_TARGET, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(ATLAS_TEXTURE_TARGET, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(ATLAS_TEXTURE_TARGET, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(ATLAS_TEXTURE_TARGET, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(ATLAS_TEXTURE_TARGET, 0, ATLAS_INTERNAL_TYPE, 
					_atlasWidth, _atlasHeight, 0, GL_RGBA, GL_FLOAT, NULL);

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
						ATLAS_TEXTURE_TARGET, _atlas_tex, 0);
 
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

//#define SORT_TEXTURE_FORMAT  GL_LUMINANCE_ALPHA
//#define SORT_TEXTURE_INNER_FORMAT GL_LUMINANCE_ALPHA_FLOAT32_ATI
#define SORT_TEXTURE_FORMAT  GL_RGB
#define SORT_TEXTURE_INNER_FORMAT GL_FLOAT_RGB32_NV

void ParticleSystem::createSortRenderTargets()
{
	glGenFramebuffersEXT(2, sort_fbo);
    glGenTextures(2, sort_tex);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[0]);
 	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, sort_tex[0]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, SORT_TEXTURE_INNER_FORMAT, 
	_width, _height, 0, SORT_TEXTURE_FORMAT, GL_FLOAT, NULL);
	
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
	GL_TEXTURE_RECTANGLE_NV, sort_tex[0], 0);


    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[1]);
 	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

    glBindTexture(GL_TEXTURE_RECTANGLE_NV, sort_tex[1]);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, SORT_TEXTURE_INNER_FORMAT, 
	_width, _height, 0, SORT_TEXTURE_FORMAT, GL_FLOAT, NULL);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
					GL_TEXTURE_RECTANGLE_NV, sort_tex[1], 0);
 
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

ParticleSystem::~ParticleSystem()
{
	std::vector<ParticleEmitter*>::iterator iter;
	for (iter = _emitters.begin(); iter != _emitters.end(); ++iter)
		delete (*iter);

	// Particle Position
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[0]);
    glDeleteTextures(1, psys_tex);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[1]);
    glDeleteTextures(1, psys_tex+1);
	glDeleteFramebuffersEXT(2, psys_fbo);

	// Sort
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[0]);
    glDeleteTextures(1, sort_tex);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[1]);
    glDeleteTextures(1, sort_tex+1);
	glDeleteFramebuffersEXT(2, sort_fbo);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, velocity_fbo[0]);
    glDeleteTextures(1, velocity_tex);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, velocity_fbo[1]);
    glDeleteTextures(1, velocity_tex+1);
	glDeleteFramebuffersEXT(2, velocity_fbo);

	if (_vertices) delete _vertices;

	delete [] _particles;
	delete [] _positionBuffer;
	delete [] _colorBuffer;

	if (_arrows) delete _arrows;

	// STREAM LINE
	if (_streamVertices) delete _streamVertices;
	if (_indexBuffer) delete _indexBuffer;


	////////////////////////////////////////
	cgDestroyProgram(_distanceInitFP);

	////////////////////////////////////////
	cgDestroyParameter(_viewPosParam);
	cgDestroyProgram(_distanceSortFP);

	////////////////////////////////////////
	cgDestroyProgram(_distanceSortLookupFP);

	////////////////////////////////////////
	cgDestroyParameter(_scaleParam);
	cgDestroyParameter(_biasParam);
	cgDestroyProgram(_passthroughFP);

	////////////////////////////////////////
	
	cgDestroyParameter(_srSizeParam);
	cgDestroyParameter(_srStepParam);
	cgDestroyParameter(_srCountParam);
	
	cgDestroyProgram(_sortRecursionFP);

	
	cgDestroyParameter(_seSizeParam);
	cgDestroyParameter(_seStepParam);
	cgDestroyProgram(_sortEndFP);

	
	cgDestroyParameter(_mpTimeScale);
	cgDestroyParameter(_mpVectorField);
	cgDestroyParameter(_mpUseInitTex);
	cgDestroyParameter(_mpCurrentTime);
	cgDestroyProgram(_moveParticlesFP);

	
	cgDestroyParameter(_ipVectorFieldParam);
	cgDestroyProgram(_initParticlePosFP);
	
	cgDestroyParameter(_mvpParticleParam);
	cgDestroyParameter(_mvParticleParam);
	cgDestroyParameter(_mvTanHalfFOVParam);
	cgDestroyParameter(_mvCurrentTimeParam);
	cgDestroyProgram(_particleFP);

	cgDestroyParameter(_vectorParticleParam);
	cgDestroyProgram(_particleVP);
}

bool ParticleSystem::isEnabled(EnableEnum enabled)
{
	switch (enabled)
	{
	case PS_SORT :
		return _sortEnabled;
		break;
	case PS_GLYPE :
		return _glypEnabled;
		break;
	case PS_DRAW_BBOX :
		return _drawBBoxEnabled;
		break;
	case PS_ADVECTION :
		return _advectionEnabled;
	case PS_STREAMLINE :
		return _streamlineEnabled;
	}
	return false;
}

void ParticleSystem::enable(EnableEnum enabled)
{
	switch (enabled)
	{
	case PS_SORT :
		_sortEnabled = true;
		break;
	case PS_GLYPE :
		_glypEnabled = true;
		break;
	case PS_DRAW_BBOX :
		_drawBBoxEnabled = true;
		break;
	case PS_ADVECTION :
		_advectionEnabled = true;
		break;
	case PS_STREAMLINE :
		_streamlineEnabled = true;
		break;
	}
}

void ParticleSystem::disable(EnableEnum enabled)
{
	switch (enabled)
	{
	case PS_SORT :
		_sortEnabled = false;
		break;
	case PS_GLYPE :
		_glypEnabled = false;
		break;
	case PS_DRAW_BBOX :
		_drawBBoxEnabled = false;
		break;
	case PS_ADVECTION :
		_advectionEnabled = false;
		break;
	case PS_STREAMLINE :
		_streamlineEnabled = false;
		break;
	}
}

void ParticleSystem::advectStreamlines()
{
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
       
	
	//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_destPosIndex]);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glEnable(GL_TEXTURE_RECTANGLE_NV);

	cgGLBindProgram(_moveParticlesFP);
	cgGLEnableProfile(CG_PROFILE_FP30);
	cgGLSetParameter1f(_mpTimeScale, 0.005);
	cgGLSetParameter1f(_mpUseInitTex, 1.0);
	cgGLSetParameter1f(_mpCurrentTime, _currentTime);
	cgGLSetParameter3f(_mpScale, _scalex, _scaley, _scalez);
	cgGLSetParameter1f(_mpMaxScale, _maxVelocityScale);
	cgGLSetTextureParameter(_mpVectorField, _curVectorFieldID);
	cgGLEnableTextureParameter(_mpVectorField);

	
	for (int i = 0; i < _stepSizeStreamline; ++i)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_destPosIndex]);
	 	glClear(GL_COLOR_BUFFER_BIT);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[_currentPosIndex]);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, _initPosTex);
		drawQuad();
		swap(_currentPosIndex, _destPosIndex);
	}

	cgGLDisableTextureParameter(_mpVectorField);
	glDisable(GL_TEXTURE_RECTANGLE_NV);
	cgGLDisableProfile(CG_PROFILE_FP30);
    
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	if (_currentStreamlineIndex < _maxAtlasCount)
	{
		int xoffset = (_currentStreamlineIndex % _atlas_x) * _width;
		int yoffset = (_currentStreamlineIndex / _atlas_x) * _height;
		
		
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(ATLAS_TEXTURE_TARGET);
		glBindTexture(ATLAS_TEXTURE_TARGET, _atlas_tex);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_destPosIndex]);
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_currentPosIndex]);
		glCopyTexSubImage2D(ATLAS_TEXTURE_TARGET, 0, xoffset, yoffset, 
						0, 0, _width, _height);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(ATLAS_TEXTURE_TARGET, 0);
		glDisable(ATLAS_TEXTURE_TARGET);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _atlas_fbo);
		_streamVertices->Read(_atlasWidth, _atlasHeight);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		_currentStreamlineIndex++;

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	
}

bool ParticleSystem::advect(float deltaT, float camx, float camy, float camz)
{
	static bool firstLoad = true;
	if (this->isTimeVaryingField())
	{
		static float oldTime = vrGetTimeStamp();
		static int index = 0;
		float time = vrGetTimeStamp();
		if ((time - oldTime) > 2.0)
		{
			if (!_queue.isEmpty())
			{
				float* data = 0;
				if (_queue.top(data))
				{
					float t = clock() /(float) CLOCKS_PER_SEC;
					_vectorFields[0]->updatePixels(data);
					float ti = clock() / (float) CLOCKS_PER_SEC;
					TRACE("pixels %f\n",ti - t);
					_queue.pop();
					oldTime = time;

					firstLoad = false;
					TRACE("%d bound\n", index++);
				}
			}		
		}
		if (firstLoad) return false;
	}

	if (!_advectionEnabled) return true;
	if (this->_streamlineEnabled)
	{
		advectStreamlines();
		_currentTime += deltaT;
	}
	else
	{

		_camx = camx;
		_camy = camy;
		_camz = camz;

		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
	       
		
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0, 0, _width, _height);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		
		glEnable(GL_TEXTURE_RECTANGLE_NV);


		cgGLBindProgram(_moveParticlesFP);
		cgGLEnableProfile(CG_PROFILE_FP30);

// INSOO
// TIME SCALE
		
#ifdef USE_TORNADOR
		cgGLSetParameter1f(_mpTimeScale, 1.0);
#else
		cgGLSetParameter1f(_mpTimeScale, 0.005);
		//cgGLSetParameter1f(_mpTimeScale, 0.5);

#endif
		//cgGLSetParameter1f(_mpTimeScale, 0.1);

		//////////////////////////////////////////////////////
		// UPDATE POS
		cgGLSetTextureParameter(_mpVectorField, _curVectorFieldID);
		cgGLEnableTextureParameter(_mpVectorField);
		cgGLSetParameter3f(_mpScale, _scalex, _scaley, _scalez);
		cgGLSetParameter1f(_mpMaxScale, _maxVelocityScale);
		for (int i = 0; i < _stepSizeStreamline; ++i)
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_destPosIndex]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[_currentPosIndex]);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_RECTANGLE_NV, _initPosTex);
			drawQuad();
			swap(_currentPosIndex, _destPosIndex);
		}
		cgGLDisableTextureParameter(_mpVectorField);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_RECTANGLE_NV);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glDisable(GL_TEXTURE_RECTANGLE_NV);

		glDisable(GL_TEXTURE_RECTANGLE_NV);
		cgGLDisableProfile(CG_PROFILE_FP30);
	    
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPopAttrib();
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		if (_sortEnabled)
		{
			sort();

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_destPosIndex]);
			_vertices->Read(_width, _height);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}
		else
		{
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_currentPosIndex]);
			_vertices->Read(_width, _height);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}


		// NEW PARTICLES
		for (unsigned int i = 0; i < _emitters.size(); ++i)
		{
			// TBD..
			unsigned int numOfNewParticles = randomRange(_emitters[i]->_minNumOfNewParticles, _emitters[i]->_maxNumOfNewParticles) * deltaT;
			for (unsigned int k = 0; k < numOfNewParticles; ++k)
			{
				float3 position = _emitters[i]->_position;
				position += _emitters[i]->_maxPositionOffset * 
					float3(randomRange(-1.0f, 1.0f), randomRange(-1.0f, 1.0f), randomRange(-1.0f, 1.0f));


				float lifetime = randomRange(_emitters[i]->_minLifeTime, _emitters[i]->_maxLifeTime);

				// TBD..
				allocateParticle(position, float3(0.0f, 0.0f, 0.0f),  lifetime, 1 - (float) k / numOfNewParticles);
			}
		}

		
		initNewParticles();
		
		_currentTime += deltaT;

		cleanUpParticles();
	}
	
	return true;
}

void ParticleSystem::allocateParticle(const float3& position, const float3& velocity, float lifeTime, float initTimeStep)
{
	if (_availableIndices.empty())
	{
		return;
	}

	int index = _availableIndices.top(); 
	_availableIndices.pop();
	
	ActiveParticle p;
	p.index = index;
	p.timeOfDeath = _currentTime + lifeTime;
	_activeParticles.push(p);

	NewParticle newParticle;
	newParticle.index = index;
	newParticle.position = position;
	newParticle.velocity = velocity;
	newParticle.timeOfDeath = p.timeOfDeath;
	newParticle.initTimeStep = initTimeStep;

	_newParticles.push_back(newParticle);

	_particles[index].timeOfBirth = _currentTime;
	_particles[index].lifeTime = lifeTime;
}

void ParticleSystem::initNewParticles()
{
	/////////////////////////////
	// INIT NEW PARTICLE
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, _particleInfoBufferID);
	Particle* pinfo = (Particle*) glMapBufferARB(GL_ARRAY_BUFFER, GL_WRITE_ONLY_ARB);

	std::vector<NewParticle>::iterator iter;
	for (iter = _newParticles.begin(); iter != _newParticles.end(); ++iter)
	{
		pinfo[iter->index] = _particles[iter->index];
	}
	
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_currentPosIndex]);
	
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	cgGLBindProgram(_initParticlePosFP);
	cgGLEnableProfile(CG_PROFILE_FP30);
	cgGLSetTextureParameter(_ipVectorFieldParam, _curVectorFieldID);
	cgGLEnableTextureParameter(_ipVectorFieldParam);
	// TBD..
	//glActiveTextureARB(GL_TEXTURE0_ARB);
	//VelocityPBuffers[VelocityCurrent]->Bind();

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[_destPosIndex]);

	glBegin(GL_POINTS);
	for (iter = _newParticles.begin(); iter != _newParticles.end(); iter++)
	{
	    //TRACE("[%d] %f %f %f\n", iter->index, iter->position.x,iter->position.y,iter->position.z);
		glMultiTexCoord3f(GL_TEXTURE0, iter->position.x,iter->position.y,iter->position.z);
		
		// TBD..
		//glMultiTexCoord2f(GL_TEXTURE1_ARB, (float)(i->Index % Width), (float)(i->Index / Height));
		//glMultiTexCoord1f(GL_TEXTURE2_ARB, iter->initTimeStep);
		
		glVertex2f(	(float)(iter->index % _width),
			(float)(iter->index / _height) + 1/* + offsetY*/ /* + offsetY shouldn't be */);
		//TRACE("%f %f\n", (float) (iter->index % _width), (float)(iter->index / _width));
	}
	glEnd();

	glDisable(GL_TEXTURE_RECTANGLE_NV);
	cgGLDisableTextureParameter(_ipVectorFieldParam);
	cgGLDisableProfile(CG_PROFILE_FP30);
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	
	// INIT NEW END
	/////////////////////////////
#if 1
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _initPos_fbo);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	cgGLBindProgram(_initParticlePosFP);
	cgGLEnableProfile(CG_PROFILE_FP30);
	cgGLSetTextureParameter(_ipVectorFieldParam, _curVectorFieldID);
	cgGLEnableTextureParameter(_ipVectorFieldParam);
	// TBD..
	//glActiveTextureARB(GL_TEXTURE0_ARB);
	//VelocityPBuffers[VelocityCurrent]->Bind();

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	//glBindTexture(GL_TEXTURE_RECTANGLE_NV, _initPosTex);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[_destPosIndex]);

	glBegin(GL_POINTS);
	for (iter = _newParticles.begin(); iter != _newParticles.end(); iter++)
	{
		//TRACE("[%d] %f %f %f\n", iter->index, iter->position.x,iter->position.y,iter->position.z);
		glMultiTexCoord3f(GL_TEXTURE0, iter->position.x,iter->position.y,iter->position.z);
		
		// TBD..
		//glMultiTexCoord2f(GL_TEXTURE1_ARB, (float)(i->Index % Width), (float)(i->Index / Height));
		//glMultiTexCoord1f(GL_TEXTURE2_ARB, iter->initTimeStep);
		
		glVertex2f(	(float)(iter->index % _width),
			(float)(iter->index / _height) + 1/* + offsetY*/ /* + offsetY shouldn't be */);
		//TRACE("%f %f\n", (float) (iter->index % _width), (float)(iter->index / _width));
	}
	glEnd();

	glDisable(GL_TEXTURE_RECTANGLE_NV);
	cgGLDisableTextureParameter(_ipVectorFieldParam);
	cgGLDisableProfile(CG_PROFILE_FP30);
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif

	// Free array.
	_newParticles.clear();
}

void ParticleSystem::cleanUpParticles()
{
	if (! _activeParticles.empty())
	{
		while (_activeParticles.top().timeOfDeath < _currentTime)
		{
			unsigned int i = _activeParticles.top().index;
			_activeParticles.pop();
			_availableIndices.push(i);
			//_usedIndices[i] = false;

			//if (i >= _maxUsedIndex)
			//	while (!_usedIndices[_maxUsedIndex])
			//		_maxUsedIndex--;

		}
	}
}

void ParticleSystem::sort()
{
	
	///////////////////////////////////////////////////////////
	// BEGIN - COMPUTE DISTANCE	
	vrVector3f pos;
	vrMatrix4x4f mat;
	glPushMatrix();
	glLoadIdentity();
		glScalef(_scalex, _scaley, _scalez);
		glTranslatef(-0.5f, -0.5f, -0.5f);
		glGetFloatv(GL_MODELVIEW_MATRIX, mat.get());
	glPopMatrix();
	mat.invert();
	mat.getTranslation(pos);

	cgGLBindProgram(_distanceSortFP);
	cgGLEnableProfile(CG_PROFILE_FP30);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, sort_tex[_currentSortIndex]);
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[_currentPosIndex]);

	cgGLSetParameter3f(_viewPosParam, pos.x, pos.y, pos.z);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[_destSortIndex]);
	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	drawQuad();

	cgGLDisableProfile(CG_PROFILE_FP30);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	///////////////////////////////
	// _DEBUG
	/*
	{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[_destSortIndex]);
	static float debug[256];
	glReadPixels(0, 0, _width, _height, GL_RGB, GL_FLOAT, (float*)debug);
	TRACE("[%d]", _currentSortIndex);
	for (int i = 0; i < _width * _height * 3; i += 3)
		TRACE("%.2f, %.2f, %.2f\n", debug[i], debug[i+1], debug[i+2]);
	TRACE("\n");
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	}
	*/
	///////////////////////////////
	// _DEBUG
	/*
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_currentPosIndex]);
		static float debug[256];
		glReadPixels(0, 0, _width, _height, GL_RGB, GL_FLOAT, (float*)debug);
		TRACE("\n");
		TRACE("currentPos[%d]", _currentPosIndex);
		for (int i = 0; i < _width * _height * 3; i += 3)
			TRACE("%.2f, %.2f, %.2f\n", debug[i], debug[i+1], debug[i+2]);
			TRACE("\n");
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	*/
	
	
	///////////////////////////////////////////////////////////


	// compute distance
	swap(_currentSortIndex, _destSortIndex);

	// END - COMPUTE DISTANCE
	///////////////////////////////////////////////////////////
	
	///////////////////////////////////////////////////////////
	// BEGIN - MERGE SORT
	_currentSortPass = 0;

	int logdSize = logd(_particleMaxCount);
	_maxSortPasses = (logdSize + 1) * logdSize / 2;

	_sortBegin = _sortEnd;
	_sortEnd = (_sortBegin + _sortPassesPerFrame) % _maxSortPasses;

 	cgGLSetParameter2f(_srSizeParam, (float)_width, (float)_height);
	cgGLSetParameter2f(_seSizeParam, (float)_width, (float)_height);

	mergeSort(this->_particleMaxCount);

	///////////////////////////////
	// _DEBUG
	/*
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[_currentSortIndex]);
		static float debug[256];
		glReadPixels(0, 0, _width, _height, GL_RGB, GL_FLOAT, (float*)debug);
		TRACE("\n");
		TRACE("[%d]", _currentSortIndex);
		for (int i = 0; i < _width * _height * 3; i += 3)
		TRACE("%.2f, %.2f, %.2f\n", debug[i], debug[i+1], debug[i+2]);
		TRACE("\n");
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	*/
	
	///////////////////////////////////////////////////////////


	
	// END - MERGE SORT
	///////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////
	// POSITION LOOKUP
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_destPosIndex]);
	cgGLBindProgram(_distanceSortLookupFP);
	cgGLEnableProfile(CG_PROFILE_FP30);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, sort_tex[_currentSortIndex]);
	
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, psys_tex[_currentPosIndex]);

	glPushAttrib(GL_VIEWPORT_BIT);
	glViewport(0, 0, _width, _height);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	drawQuad();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	cgGLDisableProfile(CG_PROFILE_FP30);
	
	// POSITION LOOKUP
	///////////////////////////////////////////////////////////
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_RECTANGLE_NV);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_RECTANGLE_NV);

	///////////////////////////////
	// _DEBUG
	/*
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, psys_fbo[_destPosIndex]);
		static float debug[256];
		glReadPixels(0, 0, _width, _height, GL_RGB, GL_FLOAT, (float*)debug);
		TRACE("\n");
		TRACE("[%d]", _currentSortIndex);
		for (int i = 0; i < _width * _height * 3; i += 3)
		TRACE("%.2f, %.2f, %.2f\n", debug[i], debug[i+1], debug[i+2]);
		TRACE("\n");
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	*/
	
}

void ParticleSystem::mergeSort(int count)
{
	if (count > 1)
	{
		mergeSort(count >> 1); 
		merge(count, 1);
	}
}

void ParticleSystem::merge(int count, int step)
{
	if (count > 2)
	{
		merge(count >> 1, step << 1);

		++_currentSortPass;

		if (_sortBegin < _sortEnd)
		{	
			if (_currentSortPass < _sortBegin || _currentSortPass >= _sortEnd)
				return;
		}
		else
		{
			if (_currentSortPass < _sortBegin && _currentSortPass >= _sortEnd)
				return;
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[_destSortIndex]);
		cgGLBindProgram(_sortRecursionFP);
		cgGLEnableProfile(CG_PROFILE_FP30);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV , sort_tex[_currentSortIndex]);

		cgGLSetParameter1f(_srStepParam, (float)step);
		cgGLSetParameter1f(_srCountParam, (float)count);

		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0, 0, _width, _height);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		drawQuad();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPopAttrib();

		cgGLDisableProfile(CG_PROFILE_FP30);

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV , 0);
	}
	else
	{
		_currentSortPass++;

		if (_sortBegin < _sortEnd)
		{
			if (_currentSortPass < _sortBegin || _currentSortPass >= _sortEnd)
				return;
		}
		else
		{
			if (_currentSortPass < _sortBegin && _currentSortPass >= _sortEnd)
				return;
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sort_fbo[_destSortIndex]);
		cgGLBindProgram(_sortEndFP);
		cgGLEnableProfile(CG_PROFILE_FP30);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV , sort_tex[_currentSortIndex]);

		cgGLSetParameter1f(_seStepParam, (float)step);

		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0, 0, _width, _height);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, _width, 0, _height, -10.0f, 10.0f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		drawQuad();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPopAttrib();

		cgGLDisableProfile(CG_PROFILE_FP30);

		glBindTexture(GL_TEXTURE_RECTANGLE_NV , 0);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	swap(_currentSortIndex, _destSortIndex);
}

void ParticleSystem::renderStreamlines()
{
	//return;
	if (_currentStreamlineIndex > 2)
	{
		glPushMatrix();
		glScalef(_scalex, _scaley, _scalez);
		glTranslatef(-0.5f, -0.5f, -0.5f);

		if (_drawBBoxEnabled) drawUnitBox();

		glColor3f(1.0f, 0.0f, 0.0f);
		glEnableClientState(GL_VERTEX_ARRAY);
		//glEnableClientState(GL_COLOR_ARRAY);
		
		_streamVertices->SetPointer(0);
		//glColorPointer(4, GL_FLOAT, 0, 0);
		
		::glDrawElements(GL_LINES, _particleCount * 2 * (_currentStreamlineIndex - 1), GL_UNSIGNED_INT, _indexBuffer);
		glDisableClientState(GL_VERTEX_ARRAY);
		glPopMatrix();
	}
}

void ParticleSystem::render()
{
	glPushMatrix();

	if (_streamlineEnabled)
	{
		renderStreamlines();
	}
	else {
		glScalef(_scalex, _scaley, _scalez);
		glTranslatef(-0.5f, -0.5f, -0.5f);
		if (_glypEnabled)
		{
			
			if (_drawBBoxEnabled) drawUnitBox();
			glColor3f(1, 1, 1);
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			
#ifndef USE_RGBA_ARROW
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#else
			glEnable(GL_ALPHA_TEST);
			//glAlphaFunc(GL_GEQUAL, 0.5);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
			glEnable(GL_POINT_SPRITE_NV);
			glPointSize(_pointSize);
				
			glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);

#ifndef TEST
			_arrows->bind(0);
			glEnable(GL_TEXTURE_2D);
			//glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, 60.0f );
			glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, 0.0f );

			glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 1.0f);
			glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, 100.0f);
			glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );

			cgGLBindProgram(_particleVP);
			cgGLBindProgram(_particleFP);
			cgGLEnableProfile(CG_PROFILE_VP40);
			cgGLEnableProfile(CG_PROFILE_FP40);


			cgSetParameter1f(_mvCurrentTimeParam, _currentTime);
			//TRACE("%f %f, %f %d\n", _fov, tan(_fov), tan(_fov / 2.0), _screenHeight);
			//cgSetParameter1f(_mvTanHalfFOVParam, -tan(_fov * PI / 180 * 0.5) * _screenHeight * 0.5);
			//float v = tan(_fov * PI / 180 * 0.5) * _screenHeight * 0.5;
			cgSetParameter1f(_mvTanHalfFOVParam, tan(_fov * PI / 180 * 0.5) * _screenHeight * 0.5);
			//cgSetParameter1f(_mvTanHalfFOVParam,  _screenHeight * 0.5 / tan(_fov * PI / 180 * 0.5));
			
			cgGLSetStateMatrixParameter(_mvpParticleParam,
							CG_GL_MODELVIEW_PROJECTION_MATRIX,
							CG_GL_MATRIX_IDENTITY);
			cgGLSetStateMatrixParameter(_mvParticleParam,
							CG_GL_MODELVIEW_MATRIX,
							CG_GL_MATRIX_IDENTITY);

			

		
		
			// TBD..
			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);			
			glEnableVertexAttribArrayARB(8);
			glEnableClientState(GL_VERTEX_ARRAY);
			//glEnableClientState(GL_COLOR_ARRAY);
			_vertices->SetPointer(0);
			//glBindBufferARB(GL_ARRAY_BUFFER, _colorBufferID);
			//glColorPointer(4, GL_FLOAT, 0, 0);
			//glBindBufferARB(GL_ARRAY_BUFFER, 0);

			// TBD...
			glDrawArrays(GL_POINTS, 0, this->_particleMaxCount);
			glPointSize(1);
	

			glDisableClientState(GL_VERTEX_ARRAY);
			//glDisableClientState(GL_COLOR_ARRAY);
			
			// TBD...
			::glDisableVertexAttribArray(8);
			glPopClientAttrib();
			
			cgGLDisableProfile(CG_PROFILE_VP40);
			cgGLDisableProfile(CG_PROFILE_FP40);

			
			
			glDepthMask(GL_TRUE);
			
			glDisable(GL_POINT_SPRITE_NV);
			glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_BLEND);
#else
			glBindTexture(GL_TEXTURE_2D, ::SpotTexID);
			glEnable(GL_TEXTURE_2D);
			//glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, 60.0f );
			glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE_ARB, 0.0f );

			glPointParameterfARB( GL_POINT_SIZE_MIN_ARB, 1.0f);
			glPointParameterfARB( GL_POINT_SIZE_MAX_ARB, 100.0f);
			glTexEnvf( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE );

			glEnableClientState(GL_VERTEX_ARRAY);
			//glEnableClientState(GL_COLOR_ARRAY);
			_vertices->SetPointer(0);
			//glBindBufferARB(GL_ARRAY_BUFFER, _colorBufferID);
			//glColorPointer(4, GL_FLOAT, 0, 0);
			//glBindBufferARB(GL_ARRAY_BUFFER, 0);

			// TBD...
			glDrawArrays(GL_POINTS, 0, this->_particleMaxCount);
			glPointSize(1);
			glDrawArrays(GL_POINTS, 0, this->_particleMaxCount);
	

			glDisableClientState(GL_VERTEX_ARRAY);
			
			
			glDepthMask(GL_TRUE);
			
			glDisable(GL_POINT_SPRITE_NV);
			glDisable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_BLEND);
#endif
		}
		else
		{
			
			if (_drawBBoxEnabled) drawUnitBox();
			
			
		
			glColor3f(1, 1, 1);
			glEnableClientState(GL_VERTEX_ARRAY);
			//glEnableClientState(GL_COLOR_ARRAY);
			glPointSize(10);
			_vertices->SetPointer(0);
			//glBindBufferARB(GL_ARRAY_BUFFER, _colorBufferID);
			//glColorPointer(4, GL_FLOAT, 0, 0);
			//glBindBufferARB(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_POINTS, 0, _particleMaxCount);

			glPointSize(1);
			_vertices->SetPointer(0);
			//glBindBufferARB(GL_ARRAY_BUFFER, _colorBufferID);
			//glColorPointer(4, GL_FLOAT, 0, 0);
			//glBindBufferARB(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_POINTS, 0, _particleMaxCount);

			glDisableClientState(GL_VERTEX_ARRAY);
		}

		/*if (_criticalPoints && _criticalPoints->size())
		{
			std::vector<vrVector3f>::iterator iter;
			glColor4f(1, 1, 1, 1);
			glPointSize(10);
			glBegin(GL_POINTS);
			for (iter = _criticalPoints->begin(); iter != _criticalPoints->end(); ++iter)
			{
				glVertex3f((*iter).x, (*iter).y, (*iter).z);
				
			}
			glEnd();
			glPointSize(1);
		}
		*/
	}

	glPopMatrix();
}

void ParticleSystem::drawQuad(int x1, int y1, int x2, int y2)
{
	glBegin(GL_QUADS);

	glTexCoord2f(x1, y1);
	glVertex2f(x1, y1);

	glTexCoord2f(x2, y1);
	glVertex2f(x2, y1);

	glTexCoord2f(x2, y2); 
	glVertex2f(x2, y2);

	glTexCoord2f(x1, y2);
	glVertex2f(x1, y2);

	glEnd();
}




void ParticleSystem::drawQuad()
{
	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glVertex2f(0, 0);

	glTexCoord2f((float)_width, 0);
	glVertex2f((float)_width, 0);

	glTexCoord2f((float)_width, (float)_height); 
	glVertex2f((float)_width, (float)_height);

	glTexCoord2f(0, (float)_height);
	glVertex2f(0, (float)_height);

	glEnd();
}



void ParticleSystem::addEmitter(ParticleEmitter* emitter)
{
	_emitters.push_back(emitter);
}

void ParticleSystem::removeEmitter(int index)
{
	// TBD...
}

void ParticleSystem::removeAllEmitters()
{
	std::vector<ParticleEmitter*>::iterator iter;
	for (iter = _emitters.begin(); iter != _emitters.end(); ++iter)
	{
		delete *iter;
	}
	_emitters.clear();
}

void ParticleSystem::drawUnitBox()
{
	float x0 = 0.0f, y0 = 0.0f, z0 = 0.0f;
	float x1 = 1.0f, y1 = 1.0f, z1 = 1.0f;
	float r = 1.0f, g = 1.0f, b = 0.0f;
	float line_width = 2.0f;
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glColor4d(r, g, b, 1.0);
	glLineWidth(line_width);

	glBegin(GL_LINE_LOOP);
	{
		glVertex3d(x0, y0, z0);
		glVertex3d(x1, y0, z0);
		glVertex3d(x1, y1, z0);
		glVertex3d(x0, y1, z0);
	}
	glEnd();

	glBegin(GL_LINE_LOOP);
	{
		glVertex3d(x0, y0, z1);
		glVertex3d(x1, y0, z1);
		glVertex3d(x1, y1, z1);
		glVertex3d(x0, y1, z1);
	}
	glEnd();


	glBegin(GL_LINE_LOOP);
	{
		glVertex3d(x0, y0, z0);
		glVertex3d(x0, y0, z1);
		glVertex3d(x0, y1, z1);
		glVertex3d(x0, y1, z0);
	}
	glEnd();

	glBegin(GL_LINE_LOOP);
	{
		glVertex3d(x1, y0, z0);
		glVertex3d(x1, y0, z1);
		glVertex3d(x1, y1, z1);
		glVertex3d(x1, y1, z0);
	}
	glEnd();
	glLineWidth(1.0f);
}

void ParticleSystem::setUserDefinedNumOfParticles(int numParticles)
{
	if (numParticles < _particleMaxCount)
		_userDefinedParticleMaxCount = numParticles;
	else
		_userDefinedParticleMaxCount = _particleMaxCount;
	reset();
}
