/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string.h>

#include <expat.h>

#include <R2/R2FilePath.h>

#include "ParticleSystemFactory.h"
#include "ParticleSystem.h"
#include "Trace.h"

#define BUFSIZE 4096

void ParticleSystemFactory::text(void *data, const XML_Char *txt, int len)
{
}

void ParticleSystemFactory::startElement(void *userData, const char *elementName, const char **attrs)
{
    ParticleSystemFactory* This = reinterpret_cast<ParticleSystemFactory*>(userData);

    if (!strcmp(elementName, "particle-sys-info")) {
        This->parseParticleSysInfo(attrs);
    } else if (!strcmp(elementName, "emitter"))	{
        This->parseEmitterInfo(attrs);
    }
}

void ParticleSystemFactory::endElement(void *userData, const char *name)
{
}

ParticleSystemFactory::ParticleSystemFactory() :
    _newParticleSystem(0)
{
}

ParticleSystemFactory::~ParticleSystemFactory()
{
}

ParticleSystem* ParticleSystemFactory::create(const std::string& fileName)
{
    FILE* fp = fopen(fileName.c_str(), "rb");

    if (fp == 0) {
        return 0;
    }
    XML_Parser parser = XML_ParserCreate(NULL);

    XML_SetUserData(parser, this);
    XML_SetElementHandler(parser, startElement, endElement);

    size_t stat;
    size_t cnt;

    while (! feof(fp)) {
        void *buff = XML_GetBuffer(parser, BUFSIZE);
        if (! buff) {
            break;
        }
        cnt = fread(buff, 1, BUFSIZE, fp);
        stat = XML_ParseBuffer(parser, (int) cnt, 0);
        if (!stat) {
            //TRACE("Parse error at line %d\n", XML_GetCurrentLineNumber(parser));
            break;
        }
    }

    fclose(fp);

    XML_ParserFree(parser);

    return _newParticleSystem;
}

void ParticleSystemFactory::parseParticleSysInfo(const char** attrs)
{
    int width = 2, height = 2; // default
    std::string fileName = "J-wire-vec.dx";
    float pointSize = -1.0f;
    int numOfUsedParticles = -1;
    bool sortEnabled = false;
    bool glyphEnabled = false;
    bool bboxVisible = false;
    bool advectionEnabled = false;
    bool streamlineEnabled = false;
    bool timeVaryingData = false;
    int fieldWidth = 1;
    int fieldHeight = 1;
    int fieldDepth = 1;
    // TBD..
    //float timeSeries_vel_mag_min;
    //float timeSeries_vel_mag_max;
    //float timeSeriesVelMagMax;
    int startIndex = -1, endIndex = -1;
    for (int i = 0; attrs[i]; i += 2) {
        if (!strcmp(attrs[i], "rendertarget-width")) {
            width = atoi(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "rendertarget-height")) {
            height = atoi(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "particle-point-size")) {
            pointSize = (float) atof(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "vector-field-x")) {
            fieldWidth = atoi(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "vector-field-y")) {
            fieldHeight = atoi(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "vector-field-z")) {
            fieldDepth = atoi(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "sort-enabled")) {
            if (!strcmp(attrs[i + 1], "true"))
                sortEnabled = true;
        } else if (!strcmp(attrs[i], "glyph-enabled")) {
            if (!strcmp(attrs[i + 1], "true"))
                glyphEnabled = true;
        } else if (!strcmp(attrs[i], "bbox-draw-enabled")) {
            if (!strcmp(attrs[i + 1], "true"))
                bboxVisible = true;
        } else if (!strcmp(attrs[i], "advection-enabled")) {
            if (!strcmp(attrs[i + 1], "true"))
                advectionEnabled = true;
        } else if (!strcmp(attrs[i], "stream-line-enabled")) {
            if (!strcmp(attrs[i + 1], "true"))
                streamlineEnabled = true;
        } else if (!strcmp(attrs[i], "vector-field")) {
            fileName = attrs[i + 1];
        } else if (!strcmp(attrs[i], "vector-field")) {
            if (!strcmp(attrs[i + 1], "true"))
                timeVaryingData = true;
        } else if (!strcmp(attrs[i], "particle-user-num")) {
            numOfUsedParticles = atoi(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "time-series-start-index")) {
            startIndex = atoi(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "time-series-end-index")) {
            endIndex = atoi(attrs[i + 1]);
        } else if (!strcmp(attrs[i], "time-varying")) {
            if (!strcmp(attrs[i + 1], "true"))
                timeVaryingData = true;
        }
    }

    if (timeVaryingData) {
        char buff[256];
        sprintf(buff, fileName.c_str(), startIndex);
        std::string path = R2FilePath::getInstance()->getPath(buff);
        if (path.size()) {
            std::string dir;
            int index = path.rfind('/');
            if (index == -1) {
                index = path.rfind('\\');
                if (index == -1) {
                    TRACE("file not found\n");
                }
            }

            dir = path.substr(0, index + 1);
            path = dir + fileName;
		
            _newParticleSystem =
                new ParticleSystem(width, height, path.c_str(),
                                   fieldWidth, fieldHeight, fieldDepth,
                                   timeVaryingData, startIndex, endIndex);
        } else {
            _newParticleSystem =
                new ParticleSystem(width, height, fileName.c_str(),
                                   fieldWidth, fieldHeight, fieldDepth,
                                   timeVaryingData, startIndex, endIndex);
        }
    } else {
        std::string path = R2FilePath::getInstance()->getPath(fileName.c_str());
        _newParticleSystem =
            new ParticleSystem(width, height, path.c_str(),
                               fieldWidth, fieldHeight, fieldDepth,
                               timeVaryingData, startIndex, endIndex);
    }

    if (pointSize != -1.0f) _newParticleSystem->setDefaultPointSize(pointSize);
    if (sortEnabled) _newParticleSystem->enable(ParticleSystem::PS_SORT);
    if (glyphEnabled) _newParticleSystem->enable(ParticleSystem::PS_GLYPH);
    if (bboxVisible) _newParticleSystem->enable(ParticleSystem::PS_DRAW_BBOX);
    if (advectionEnabled) _newParticleSystem->enable(ParticleSystem::PS_ADVECTION);
    if (streamlineEnabled) _newParticleSystem->enable(ParticleSystem::PS_STREAMLINE);
    if (numOfUsedParticles != -1) _newParticleSystem->setUserDefinedNumOfParticles(numOfUsedParticles);
}

void ParticleSystemFactory::parseEmitterInfo(const char** attrs)
{
    ParticleEmitter* emitter = new ParticleEmitter;
    for (int i = 0; attrs[i]; i += 2) {
        if (!strcmp(attrs[i], "max-position-offset")) {
            float x = 0, y = 0, z = 0;
            sscanf(attrs[i+1], "%f%f%f",&x, &y, &z);
            emitter->setMaxPositionOffset(x, y, z);
        } else if (!strcmp(attrs[i], "position")) {
            float x = 0, y = 0, z = 0;
            sscanf(attrs[i+1], "%f%f%f",&x, &y, &z);
            emitter->setPosition(x, y, z);
        } else if (!strcmp(attrs[i], "min-max-life-time")) {
            float min = 0, max = 0;
            sscanf(attrs[i+1], "%f%f",&min, &max);
            emitter->setMinMaxLifeTime(min, max);
        } else if (!strcmp(attrs[i], "min-max-new-particles")) {
            int min = 0, max = 0;
            sscanf(attrs[i+1], "%d%d",&min, &max);
            emitter->setMinMaxNumOfNewParticles(min, max);
        } else if (!strcmp(attrs[i], "enabled")) {
            if (!strcmp(attrs[i + 1], "true"))
                emitter->setEnabled(true);
            else
                emitter->setEnabled(false);
        }
    }
    _newParticleSystem->addEmitter(emitter);
}
