/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NV_PARTICLESYSTEMFACTORY_H
#define NV_PARTICLESYSTEMFACTORY_H

#include <string>

#include <expat.h>

namespace nv {

class ParticleSystem;

class ParticleSystemFactory
{
public:
    ParticleSystemFactory();
    ~ParticleSystemFactory();

    ParticleSystem *create(const std::string& fileName);

private:
    void parseParticleSysInfo(const char **attrs);
    void parseEmitterInfo(const char **attrs);

    static void endElement(void *userData, const char *name);
    static void startElement(void *userData, const char *elementName, const char **attrs);
    static void text(void *data, const XML_Char *txt, int len);

    ParticleSystem *_newParticleSystem;
};

}

#endif