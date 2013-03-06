/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <limits>

#include <Cg/cg.h>

#include <vrmath/Vector3f.h>

#include "ParticleEmitter.h"
#include "RenderVertexArray.h"
#include "Texture2D.h"
#include "Texture3D.h"

#include "CircularQueue.h"

struct NewParticle {
    int index;
    vrVector3f position;
    vrVector3f velocity;
    float timeOfDeath;
    float initTimeStep;
};

template <typename Type, typename Compare=std::less<Type> >
class priority_queue_vector : public std::priority_queue<Type, std::vector<Type>, Compare>
{
public:
    void reserve(typename priority_queue_vector<Type>::size_type count)
    {
        //priority_queue<Type, std::vector<Type>, Compare>::reserve(count);
    }
};

struct ActiveParticle {
    float timeOfDeath;
    int index;
};

namespace std
{
    template <>
    struct greater<ActiveParticle> {
        bool operator() (const ActiveParticle& left, const ActiveParticle& right)
        {
            return left.timeOfDeath > right.timeOfDeath;
        }
    };
}

struct color4 {
    float r, g, b, a;
};

struct Particle {
    float timeOfBirth;
    float lifeTime;
    float attributeType;
    float index;
};

class ParticleSystem
{
public:
    enum EnableEnum {
        PS_SORT = 1,
        PS_GLYPH = 2,
        PS_DRAW_BBOX = 4,
        PS_ADVECTION = 8,
        PS_STREAMLINE = 16,
    };

    ParticleSystem(int width, int height, const std::string& fileName,
                   int fieldWidth, int fieldHeight, int fieldDepth, 
                   bool timeVaryingData, int flowFileStartIndex, int flowFileEndIndex);
    ~ParticleSystem();
    void createRenderTargets();
    void createSortRenderTargets();
    void createStreamlineRenderTargets();
    void advectStreamlines();
    void renderStreamlines();

    bool advect(float deltaT, float camx, float camy, float camz);
    void enable(EnableEnum enabled);
    void disable(EnableEnum enabled);
    bool isEnabled(EnableEnum enabled);

    ////////////////////////////////////////////
    void addEmitter(ParticleEmitter *emitter);
    ParticleEmitter *getEmitter(int index);
    unsigned int getNumEmitters() const;
    void removeEmitter(int index);
    void removeAllEmitters();

    int getMaxSize() const;

    void render();

    void reset();

    void setDefaultPointSize(float size);

    unsigned int getNumOfActiveParticles() const;

    void setScreenSize(int sreenWidth, int screenHeight);

    void setFOV(float fov);

    void setUserDefinedNumOfParticles(int numParticles);

    float getScaleX() const;
    float getScaleY() const;
    float getScaleZ() const;
    unsigned int getVectorFieldGraphicsID() const;

    bool isTimeVaryingField() const;
    void setTimeVaryingField(bool timeVarying);

    void setDepthCueEnabled(bool enabled);
    bool getDepthCueEnabled() const;

    static void *dataLoadMain(void *data);
    // TEMP
    static void callbackForCgError();

    const int _width;
    const int _height;
    /**
     * @brief The total number of particles (= _width * _height)
     */
    int _particleMaxCount;
    /**
     * @brief The total number of particles avaiable (1 <= _userDefinedParticleMaxCount <= _width * _height)
     */
    unsigned int _userDefinedParticleMaxCount;

    int _currentSortPass;
    int _maxSortPasses;
    int _sortPassesPerFrame;

    int _sortBegin;
    int _sortEnd;

    float _pointSize;

    /**
     * @brief The index for the source render target
     */
    int _currentSortIndex;

    /**
     * @brief The index for the destination render target
     */
    int _destSortIndex;

    int _currentPosIndex;
    int _destPosIndex;

    std::vector<ParticleEmitter *> _emitters;

    float _currentTime;

    priority_queue_vector<int, std::greater<int> > _availableIndices;
    priority_queue_vector<ActiveParticle, std::greater<ActiveParticle> > _activeParticles;
    std::vector<NewParticle> _newParticles;

    /**
     * @brief frame buffer objects: two are defined, flip them as input output every step
     */
    unsigned int psys_fbo[2]; 	

    /**
     * @brief color textures attached to frame buffer objects
     */
    unsigned int psys_tex[2];	

    unsigned int sort_fbo[2]; 
    unsigned int sort_tex[2];

    unsigned int velocity_fbo[2]; 
    unsigned int velocity_tex[2];

    ///////////////////////////////////////////////////
    // TIME SERIES
    std::vector<unsigned int> _vectorFieldIDs;
    std::vector<Texture3D *> _vectorFields;
    unsigned int _curVectorFieldID;
    float _time_series_vel_mag_min;
    float _time_series_vel_mag_max;
    CircularFifo<float *, 10> _queue;
    int _flowFileStartIndex;
    int _flowFileEndIndex;
    ///////////////////////////////////////////////////
    int _skipByte;

    float _maxVelocityScale;
	
    RenderVertexArray* _vertices;

    Particle *_particles;
    vrVector3f *_positionBuffer;
    color4 *_colorBuffer;
    unsigned _colorBufferID;
    //////////////////////////////////////////
    Texture2D *_arrows;

    float _camx;
    float _camy;
    float _camz;

    float _scalex, _scaley, _scalez;

    bool _sortEnabled;
    bool _glyphEnabled;
    bool _drawBBoxEnabled;
    bool _advectionEnabled;
    bool _streamlineEnabled;
    bool _depthCueEnabled;

    CGprogram _distanceInitFP;

    CGprogram _distanceSortFP;
    CGparameter _viewPosParam;

    CGprogram _distanceSortLookupFP;

    CGprogram _passthroughFP;
    CGparameter _scaleParam;
    CGparameter _biasParam;

    CGprogram _sortRecursionFP;
    CGparameter _srSizeParam;
    CGparameter _srStepParam;
    CGparameter _srCountParam;

    CGprogram _sortEndFP;
    CGparameter _seSizeParam;
    CGparameter _seStepParam;

    CGprogram _moveParticlesFP;
    CGparameter _mpTimeScale;
    CGparameter _mpVectorField;
    CGparameter _mpUseInitTex;
    CGparameter _mpCurrentTime;
    CGparameter _mpMaxScale;
    CGparameter _mpScale;

    CGprogram _initParticlePosFP;
    CGparameter _ipVectorFieldParam;

    CGprogram _particleVP;
    CGparameter _mvpParticleParam;
    CGparameter _mvParticleParam;
    CGparameter _mvTanHalfFOVParam;
    CGparameter _mvCurrentTimeParam;
	
    CGprogram _particleFP;
    CGparameter _vectorParticleParam;

    //////////////////////////////////////////
    // STREAMLINE
    unsigned int _maxAtlasCount;
    unsigned int _currentStreamlineIndex;
    int _currentStreamlineOffset;
    int _particleCount;
    int _stepSizeStreamline;
    int _curStepCount;
    int _atlasWidth, _atlasHeight;
    unsigned int _atlas_fbo;
    unsigned int _atlas_tex;
    RenderVertexArray *_streamVertices;
    unsigned int _atlasIndexBufferID;

    unsigned int _maxIndexBufferSize;
    unsigned int *_indexBuffer;
    int _atlas_x, _atlas_y;

    //////////////////////////////////////////
    // Initial Position
    unsigned int _initPos_fbo;
    unsigned int _initPosTex;

    /////////////////////////////////////////
    // Particle Info Vertex Buffer
    unsigned int _particleInfoBufferID;

    int _screenWidth;
    int _screenHeight;
    float _fov;

    bool _isTimeVaryingField;

    int _flowWidth;
    int _flowHeight;
    int _flowDepth;

    std::string _fileNameFormat;

    // INSOO
    // TEST
    std::vector<vrVector3f> *_criticalPoints;

    // TEMP
    static CGcontext _context;

protected:
    void mergeSort(int count);
    void merge(int count, int step);
    void sort();
    void drawQuad();
    void drawUnitBox();
    void drawQuad(int x1, int y1, int x2, int y2);
    void initShaders();
    void passThoughPositions();
    void initStreamlines(int width, int height);
    void resetStreamlines();
    void initInitPosTex();

    void allocateParticle(const vrVector3f&, const vrVector3f&, float, float);
    void initNewParticles();
    void cleanUpParticles();
};

inline unsigned int ParticleSystem::getNumEmitters() const
{
    return _emitters.size();
}

inline int ParticleSystem::getMaxSize() const
{
    return _width * _height;
}

inline void ParticleSystem::setDefaultPointSize(float size)
{
    _pointSize = size;
}

inline unsigned int ParticleSystem::getNumOfActiveParticles() const
{
    return _activeParticles.size();
}

inline ParticleEmitter* ParticleSystem::getEmitter(int index)
{
    if ((unsigned int) index < _emitters.size())
        return _emitters[index];
    return 0;
}

inline void ParticleSystem::setScreenSize(int screenWidth, int screenHeight)
{
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
}

inline void ParticleSystem::setFOV(float fov)
{
    _fov = fov;
}

inline float ParticleSystem::getScaleX()const
{
    return _scalex;
}

inline float ParticleSystem::getScaleY()const
{
    return _scaley;
}

inline float ParticleSystem::getScaleZ()const
{
    return _scalez;
}

inline unsigned int ParticleSystem::getVectorFieldGraphicsID()const
{
    return _curVectorFieldID;
}

inline bool ParticleSystem::isTimeVaryingField() const
{
    return _isTimeVaryingField;
}

inline void ParticleSystem::setTimeVaryingField(bool timeVarying)
{
    _isTimeVaryingField = timeVarying;
}

inline void ParticleSystem::setDepthCueEnabled(bool enabled)
{
    _depthCueEnabled = enabled;
}

inline bool ParticleSystem::getDepthCueEnabled() const
{
    return _depthCueEnabled;
}

#endif
