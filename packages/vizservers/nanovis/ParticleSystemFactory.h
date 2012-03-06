/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#pragma once

#include <string>
#ifdef _WIN32
#include <expat/expat.h>
#else
#include <expat.h>
#endif

class ParticleSystem;

class ParticleSystemFactory
{
    ParticleSystem* _newParticleSystem;
public :
    ParticleSystemFactory();
    ~ParticleSystemFactory();

    ParticleSystem* create(const std::string& fileName);

private :
    void parseParticleSysInfo(const char** attrs);
    void parseEmitterInfo(const char**attrs);

    static void endElement(void *userData, const char *name);
    static void startElement(void *userData, const char *elementName, const char **attrs);
    static void text(void *data, const XML_Char *txt, int len);
};
