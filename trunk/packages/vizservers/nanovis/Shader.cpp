/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 */
#include <stdio.h>

#include <GL/glew.h>
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <util/FilePath.h>

#include "config.h"
#include "Shader.h"
#include "Trace.h"

using namespace nv;
using namespace nv::util;

/**
 * These correspond to NV_vertex_program3 and NV_fragment_program2:
 * CG_PROFILE_VP40
 * CG_PROFILE_FP40
 *
 * These correspond to ARB_vertex_program and ARB_fragment_program:
 * CG_PROFILE_ARBVP1
 * CG_PROFILE_ARBFP1
 *
 * Generic GLSL targets:
 * CG_PROFILE_GLSLV
 * CG_PROFILE_GLSLF
 */
#ifdef USE_ARB_PROGRAMS
CGprofile Shader::_defaultVertexProfile = CG_PROFILE_ARBVP1;
CGprofile Shader::_defaultFragmentProfile = CG_PROFILE_ARBFP1;
#else
CGprofile Shader::_defaultVertexProfile = CG_PROFILE_VP40;
CGprofile Shader::_defaultFragmentProfile = CG_PROFILE_FP40;
#endif
CGcontext Shader::_cgContext = NULL;

void Shader::init()
{
    _cgContext = cgCreateContext();
}

void Shader::exit()
{
    setErrorCallback(NULL);
    printErrorInfo();
    if (_cgContext != NULL) {
        TRACE("Before DestroyContext");
        cgDestroyContext(_cgContext);
        TRACE("After DestroyContext");
        _cgContext = NULL;
    }
}

bool Shader::printErrorInfo()
{
    CGerror lastError = cgGetError();

    if (lastError) {
        TRACE("Cg Error: %s", cgGetErrorString(lastError));
        if (getCgContext())
            TRACE("%s", cgGetLastListing(getCgContext()));
        return false;
    }
    return true;
}

CGprogram
Shader::loadCgSourceProgram(CGcontext context, const char *fileName,
                            CGprofile profile, const char *entryPoint)
{
    std::string path = FilePath::getInstance()->getPath(fileName);
    if (path.empty()) {
        ERROR("can't find program \"%s\"", fileName);
    }
    TRACE("cg program compiling: %s", path.c_str());
    CGprogram program;
    program = cgCreateProgramFromFile(context, CG_SOURCE, path.c_str(), profile, 
                                      entryPoint, NULL);
    cgGLLoadProgram(program);
    CGerror LastError = cgGetError();
    if (LastError) {
        ERROR("Error message: %s", cgGetLastListing(context));
    }
    TRACE("successfully compiled program: %s", path.c_str());
    return program;
}

Shader::Shader():
    _vertexProfile(_defaultVertexProfile),
    _fragmentProfile(_defaultFragmentProfile),
    _cgVP(NULL),
    _cgFP(NULL)
{
}

Shader::~Shader()
{
    TRACE("Enter");
    if (_cgContext == NULL) {
        TRACE("Lost Cg context: vp: %s, fp: %s", _vpFile.c_str(), _fpFile.c_str());
    } else {
        resetPrograms();
    }
}

void Shader::loadVertexProgram(const char *fileName)
{
    if (_cgVP != NULL) {
        cgDestroyProgram(_cgVP);
    }
    _cgVP = loadCgSourceProgram(_cgContext, fileName,
                                _vertexProfile, "main");
    _vpFile = fileName;
}

void Shader::loadFragmentProgram(const char *fileName)
{
    if (_cgFP != NULL) {
        cgDestroyProgram(_cgFP);
    }
    _cgFP = loadCgSourceProgram(_cgContext, fileName,
                                _fragmentProfile, "main");
    _fpFile = fileName;
}

void Shader::resetPrograms()
{
    if (_cgVP != NULL) {
        TRACE("Destroying vertex program: %s", _vpFile.c_str());
        cgDestroyProgram(_cgVP);
    }

    if (_cgFP != NULL) {
        TRACE("Destroying fragment program: %s", _fpFile.c_str());
        cgDestroyProgram(_cgFP);
    }
}

void Shader::setErrorCallback(ShaderCallbackFunction callback)
{
    TRACE("Shader setting error callback to: %p", callback);
    cgSetErrorCallback(callback);
}
